from django.db import models

MAX_HOSTNAME_LEN = 253
WINDOWS_MAX_PATH = 260


class AuthenticationToken(models.Model):
    token = models.UUIDField(primary_key=True)
    revoked = models.BooleanField(default=False)
    last_used = models.DateTimeField(blank=True, null=True)
    last_hostname = models.CharField(max_length=MAX_HOSTNAME_LEN, blank=True, default="")

    def __str__(self):
        return str(self.token)


class ClipboardData(models.Model):
    username = models.CharField(max_length=WINDOWS_MAX_PATH)
    hostname = models.CharField(max_length=MAX_HOSTNAME_LEN, blank=True, default="")
    paste_time = models.DateTimeField(blank=True, null=True)
    paste_size = models.IntegerField(blank=True, null=True)
    file_path = models.CharField(max_length=300, default="")
    
    def __str__(self):
        return rf"[{self.paste_time!s}] {self.username}@{self.hostname}"
