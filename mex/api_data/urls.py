from django.urls import path, include
from django.conf.urls import url

from api_data import views
from api_data import api_query

# api views
urlpatterns = [

    path("q/", api_query.API.as_view(), name="api_query"),
]