from django.urls import path, include
from django.conf.urls import url

from api_data import views

# api views
urlpatterns = [
    # apis for program
    # path("pc/<str:command>/", api_program.API.as_view(), name="program_run"),

    # apis for manual
    # path("mc/", api_manual.API.as_view(), name="manual_control"),
]