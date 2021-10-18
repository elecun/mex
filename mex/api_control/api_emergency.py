#-*- coding:utf-8 -*-

from django.shortcuts import render
from django.contrib.auth.models import User, Group
from rest_framework import viewsets, status
from rest_framework.response import Response
from rest_framework.decorators import api_view
from rest_framework.views import APIView
from rest_framework.permissions import IsAuthenticated, AllowAny
from django.views.decorators.csrf import csrf_protect
from django.forms.models import model_to_dict
from datetime import date
from django.conf import settings
from django.core.paginator import Paginator, EmptyPage, PageNotAnInteger
from django.middleware.csrf import get_token
from django.db.models import Count
import uuid
import json
from django.core import serializers
from datetime import datetime
from django.utils.dateparse import parse_datetime
from django.utils.encoding import force_text, smart_str
from django.http import HttpResponse
import mimetypes
from django.contrib.auth.models import User
from influxdb_client import InfluxDBClient
from influxdb_client.client.util.date_utils_pandas import PandasDateTimeHelper
from influxdb_client.client.write_api import SYNCHRONOUS


# Emergency Stop Control
class API(APIView):

    def get(self, request, *args, **kwargs):
        return Response({}, status=status.HTTP_400_BAD_REQUEST)
        

    def post(self, request, *args, **kwargs):
        print("emergency stop posted")
        client = InfluxDBClient(url="http://168.126.66.23:8086", token="3htY_F8UjYljX2O7gBR1QKYOOeoiF1cTV0_IKXFqwGLUAZMvxvtL-gi4BAz92aSv0ch-zBobIJOkAoLO9n8MNQ==", org="iae")
        #query_api = client.query_api()
        query = '''
        from(bucket: "hostdb")
        |> range(start: -10m)
        |> filter(fn: (r) => r["_measurement"] == "cpu")
        |> filter(fn: (r) => r["_field"] == "usage_system")
        |> filter(fn: (r) => r["cpu"] == "cpu-total")
        '''
        result = client.query_api().query(org="iae", query=query)
        results = []
        for table in result:
            for record in table.records:
                results.append((record.get_value(), record.get_field()))
        print(results)

        client.close()

        return Response({}, status=status.HTTP_200_OK)

