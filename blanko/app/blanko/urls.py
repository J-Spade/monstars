from django.urls import path, re_path

from . import views


urlpatterns = [
    path("", views.index, name="blanko"),
    path("<int:player_id>/", views.stats, name="stats"),
    path("<int:player_id>/makeplay/", views.makeplay, name="makeplay"),
    path("<int:player_id>/fire/", views.fire, name="fire"),
    path("hire/", views.hire, name="hire"),
    path("config/", views.config, name="blankoconfig"),
]
