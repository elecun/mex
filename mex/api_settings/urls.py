from django.urls import path, include
from django.conf.urls import url

from api_settings import views
from api_settings import api_uid
from api_settings import api_new

# api views
urlpatterns = [
    # apis for program
    # path("pc/<str:command>/", api_program.API.as_view(), name="program_run"),

    # apis for manual
    # path("mc/", api_manual.API.as_view(), name="manual_control"),
    path("uid/", api_uid.API.as_view(), name="api_settings_uid_generate"),
    path("new/", api_new.API.as_view(), name="api_settings_new"),
]