from django.contrib import admin
from django.contrib.staticfiles.urls import staticfiles_urlpatterns
from django.urls import include, path

from . import views


urlpatterns = [
    path("", views.index, name="index"),
    path("signin/", views.signin, name="signin"),
    path("signout/", views.signout, name="signout"),
] + staticfiles_urlpatterns()
