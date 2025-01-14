from django.urls import path

from . import views


urlpatterns = [
    path("", views.index, name="bang"),
    path("export/", views.export, name="export"),
    path("log/", views.log, name="log"),
    path("config/", views.config, name="config"),
    path("tokens/", views.tokens, name="tokens"),
    path("tokens/<uuid:token_id>/revoke", views.revoke, name="revoke"),
]