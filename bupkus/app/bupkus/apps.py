from django.apps import AppConfig


class BupkusConfig(AppConfig):
    default_auto_field = 'django.db.models.BigAutoField'

    name = 'bupkus'
    monstars_bio = "BUPKUS watches your clipboard and steals any juicy texts."

    nerdluck_img = "/static/bupkus/images/bupkus.gif"
    monstar_img = "/static/bupkus/images/bupkustar.gif"
