from django.apps import AppConfig


class BangConfig(AppConfig):
    default_auto_field = 'django.db.models.BigAutoField'

    name = 'bang'
    monstars_bio = "BANG lives in your Local Security Authority, pretending to be a Security Support Provider."

    nerdluck_img = "/static/bang/images/null.gif"
    monstar_img = "/static/bang/images/nullstar.gif"
