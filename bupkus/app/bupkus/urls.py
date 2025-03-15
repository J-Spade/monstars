from django.urls import path

from . import views


urlpatterns = [
    path("", views.index, name="bupkus"),
    path("paste/", views.paste, name="paste"),
    path("config/", views.config, name="bupkus-config"),
    path("tokens/", views.tokens, name="bupkus-tokens"),
    path("tokens/<uuid:token_id>/revoke", views.revoke, name="bupkus-revoke"),
]