from django.contrib import admin
from django.contrib.staticfiles.urls import staticfiles_urlpatterns
from django.urls import include, path

from . import views


urlpatterns = [
    # main site
    path("", views.index, name="index"),
    path("signin/", views.signin, name="signin"),
    path("signout/", views.signout, name="signout"),
    # admin
    path("admin/", admin.site.urls),
] + staticfiles_urlpatterns()
