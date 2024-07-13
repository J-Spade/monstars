from django.contrib import admin

from .models import AuthenticationToken, LogonCredential


admin.site.register(AuthenticationToken)
admin.site.register(LogonCredential)
