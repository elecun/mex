from django.urls import path, include
from django.conf.urls import url

from api_program import api_load

# api views
urlpatterns = [
    # apis for program
    # path("load/", api_load.API.as_view(), name="program_load"),
]