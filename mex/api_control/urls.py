from django.urls import path, include
from django.conf.urls import url

from api_control import views
from api_control import api_zeroset
from api_control import api_emergency

# api views
urlpatterns = [
    # apis for program
    # path("pc/<str:command>/", api_program.API.as_view(), name="program_run"),

    
    path("ctrl/zeroset/", api_zeroset.API.as_view(), name="api_control_zeroset"),

    # api for direct control
    path("ctrl/emergency/", api_emergency.API.as_view(), name="api_control_emergency"),
]