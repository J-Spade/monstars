from django.urls import path

from . import views


urlpatterns = [
    path("", views.index, name="index"),
    path("signin/", views.signin, name="signin"),
    path("signout/", views.signout, name="signout"),
    path("players/", views.players, name="players"),
    path("players/<int:player_id>/", views.stats, name="stats"),
    path("players/<int:player_id>/makeplay/", views.makeplay, name="makeplay"),
    path("players/<int:player_id>/fire/", views.fire, name="fire"),
    path("players/hire/", views.hire, name="hire"),
    path("players/rollcall/", views.rollcall, name="rollcall"),
    path("prizes/<uuid:prize_id>/", views.prize, name="prize"),
]

