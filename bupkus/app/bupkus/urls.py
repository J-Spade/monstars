from django.urls import path

from . import views


urlpatterns = [
    path("", views.index, name="bupkus"),
    path("paste/", views.paste, name="paste"),
    path("config/", views.config, name="config"),
    path("tokens/", views.tokens, name="tokens"),
    path("tokens/<uuid:token_id>/revoke", views.revoke, name="revoke"),
]