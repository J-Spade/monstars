from django.db import models


class BlankoPlayer(models.Model):
    hostname = models.CharField(max_length=128)
    address = models.CharField(max_length=64)
    kernel = models.CharField(max_length=64)
    active = models.BooleanField("whether the player is still active", default=False)
    birthday = models.DateTimeField("date installed")

    def __str__(self):
        return self.hostname


class BlankoPlay(models.Model):
    player = models.ForeignKey(BlankoPlayer, on_delete=models.CASCADE)
    play_time = models.DateTimeField("when the play was sent to the player")
    verb = models.CharField(max_length=16)
    scored = models.BooleanField("whether the play was successful")
    detail = models.CharField(max_length=20000, default="")
    penalty = models.CharField(max_length=255, default="")
    filepath = models.CharField(max_length=300, default="")

    def __str__(self):
        return f"{self.verb} :: {str(self.id)}"
