from django.contrib import admin

from .models import AuthenticationToken, ClipboardData


admin.site.register(AuthenticationToken)
admin.site.register(ClipboardData)
