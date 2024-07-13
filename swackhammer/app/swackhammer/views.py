import logging

from django.apps import apps
from django.contrib.auth import authenticate, login, logout
from django.contrib.auth.decorators import login_required
from django.http import HttpResponse, HttpResponseRedirect
from django.shortcuts import render
from django.urls import reverse

LOGGER = logging.getLogger("django")

def index(request):
    if not request.user.is_authenticated:
        return HttpResponseRedirect(reverse("signin"))
    monstar_apps = {app: config for app, config in apps.app_configs.items() if hasattr(config, "nerdluck_img")}
    return render(request, "swackhammer/index.html", {"monstar_apps": monstar_apps})


def signin(request):
    if request.user.is_authenticated:
        return HttpResponseRedirect(reverse("index"))
    try:
        username = request.POST["username"]
        password = request.POST["password"]
    except KeyError:
        return render(request, "swackhammer/signin.html")
    else:
        user = authenticate(request, username=username, password=password)
        if user is not None:
            login(request, user)
            return HttpResponseRedirect(reverse("index"))
    return render(request, "swackhammer/signin.html", {"fail_msg": "Signin Failed!"}, status=403)


def signout(request):
    logout(request)
    return HttpResponseRedirect(reverse("signin"))
