import pathlib

from django.apps import AppConfig
from django.conf import settings


class BlankoConfig(AppConfig):
    default_auto_field = 'django.db.models.BigAutoField'

    name = 'blanko'
    path = str(pathlib.Path(__file__).resolve().parent)
    monstars_bio = "BLANKO adds netfilter callbacks to your network stack, then waits for the signal to take action."

    nerdluck_img = "/static/blanko/images/void.gif"
    monstar_img = "/static/blanko/images/voidstar.gif"
