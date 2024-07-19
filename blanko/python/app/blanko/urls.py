from django.contrib.staticfiles.urls import staticfiles_urlpatterns
from django.urls import path, re_path

from . import views


urlpatterns = [
    path("", views.index, name="blankoindex"),
    path("<int:player_id>/", views.stats, name="stats"),
    path("<int:player_id>/makeplay/", views.makeplay, name="makeplay"),
    path("<int:player_id>/fire/", views.fire, name="fire"),
    path("hire/", views.hire, name="hire"),
    path("prizes/<uuid:prize_id>/", views.prize, name="prize"),
] + staticfiles_urlpatterns()
