from django.urls import path, include
from django.conf.urls import url

from api_data import views
from api_data import api_query
from api_data import api_query_date

# api views
urlpatterns = [

    path("q/", api_query.API.as_view(), name="api_query"),
    path("qd/<int:id>/", api_query_date.API.as_view(), name="api_query_date"),
]