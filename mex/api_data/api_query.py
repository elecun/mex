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
from django.conf import settings
import uuid
import json
from django.http import HttpResponse
import mimetypes
from api_settings.models import Settings, Command
from influxdb_client import InfluxDBClient
from influxdb_client.client.util.date_utils_pandas import PandasDateTimeHelper
from influxdb_client.client.write_api import SYNCHRONOUS
from datetime import datetime
from pytz import timezone

def convert(tup, di):
    for a, b in tup:
        di.setdefault(a, []).append(b)
    return di

class API(APIView):

    def get(self, request, *args, **kwargs):
        try :
            #client = InfluxDBClient(url=settings.INFLUXDB_V2_URL, token=settings.INFLUXDB_V2_TOEKN, org=settings.INFLUXDB_V2_ORG)
            client = InfluxDBClient(url="http://192.168.100.96:8086", token="LTF2gC6QFuhbTldZqSRrMwGisyQu1kUtmC2Cwar00ALOTQcULL1gFTuwGpf6zh_yVv18nrE35w7K6XFnO8s0ag==", org="jstec")
            query_api = client.query_api()

            q = '''
            from(bucket: "jstec")
            |> range(start: -5m)
            |> filter(fn: (r) => r["_measurement"] == "mqtt_consumer")
            |> filter(fn: (r) => r["_field"] == "rpm" or r["_field"] == "temperature_1" or r["_field"] == "temperature_2" or r["_field"] == "temperature_3" or r["_field"] == "load")
            |> aggregateWindow(every: 5s, fn: mean, createEmpty: false)
            |> yield(name: "mean")
            '''

            tables = query_api.query(q)
            
            results = []
            single_datetime = True
            for table in tables:
                for record in table.records:
                    if single_datetime==True:
                        time = record.get_time().astimezone(timezone('Asia/Seoul'))
                        timestring = time.strftime("%Y-%m-%d %H:%M:%S")
                        results.append(("datetime", timestring))
                    results.append((record.get_field(), record.get_value()))
                single_datetime = False

            dict_data = {}
            for field, value in results:
                dict_data.setdefault(field, []).append(value)
            client.close()
            return Response({"data":dict_data}, status=status.HTTP_200_OK)
        
        except Exception as e:
            print("exception : ", str(e))
            return Response({"message":str(e)}, status=status.HTTP_500_INTERNAL_SERVER_ERROR)

    def post(self, request, *args, **kwargs):
        return Response({}, status=status.HTTP_400_BAD_REQUEST)

