from django.urls import path, include
from django.conf.urls import url

from api_settings import views
from api_settings import api_uid
from api_settings import api_new
from api_settings import api_list
from api_settings import api_delete
from api_settings import api_load
from api_settings import api_update
from api_settings import api_reserve_list

# api views
urlpatterns = [
    path("uid/", api_uid.API.as_view(), name="api_settings_uid_generate"),
    path("new/", api_new.API.as_view(), name="api_settings_new"),
    path("list/", api_list.API.as_view(), name="api_settings_list"),
    path("delete/", api_delete.API.as_view(), name="api_settings_delete"),
    path("load/<int:id>/", api_load.API.as_view(), name="api_settings_load"),
    path("update/<int:id>/", api_update.API.as_view(), name="api_settings_update"),

    path("rlist/", api_reserve_list.API.as_view(), name="api_reserve_list"),
]