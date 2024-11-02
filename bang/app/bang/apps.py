from django.apps import AppConfig


class BangConfig(AppConfig):
    default_auto_field = 'django.db.models.BigAutoField'

    name = 'bang'
    monstars_bio = "BANG lives in your authentication stack, pretending to be an authentication module."

    nerdluck_img = "/static/bang/images/null.gif"
    monstar_img = "/static/bang/images/nullstar.gif"
