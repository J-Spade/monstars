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


class LogonCredential(models.Model):
    domain = models.CharField(max_length=WINDOWS_MAX_PATH)
    username = models.CharField(max_length=WINDOWS_MAX_PATH)
    password = models.CharField(max_length=WINDOWS_MAX_PATH)
    last_used = models.DateTimeField(blank=True, null=True)
    last_changed = models.DateTimeField(blank=True, null=True)
    last_hostname = models.CharField(max_length=MAX_HOSTNAME_LEN, blank=True, default="")

    def __str__(self):
        return rf"{self.domain}\{self.username}:{self.password}"
