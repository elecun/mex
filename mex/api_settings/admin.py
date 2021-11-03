from django.contrib import admin

from django.contrib import admin
from api_settings.models import Command, Settings

admin.site.register(Command)
admin.site.register(Settings)