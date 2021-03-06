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
from pytz import timezone, utc

def convert(tup, di):
    for a, b in tup:
        di.setdefault(a, []).append(b)
    return di

class API(APIView):

    permission_classes = [AllowAny]

    def get(self, request, *args, **kwargs):
        try :
            _id = kwargs["id"]
            _settings = Settings.objects.get(id=_id)

            if _settings is not None:

                # stores in Asia/Seoul timezone
                start_datetime = _settings.steps_start_at
                end_datetime = _settings.steps_stop_at
                interval = _settings.update_interval

                # influxdb stores data in UTC
                start_datetime_utc = start_datetime.astimezone(utc).strftime("%Y-%m-%dT%H:%M:%SZ")
                end_datetime_utc = end_datetime.astimezone(utc).strftime("%Y-%m-%dT%H:%M:%SZ")
                
                client = InfluxDBClient(url=settings.INFLUXDB_V2_URL, token=settings.INFLUXDB_V2_TOKEN, org=settings.INFLUXDB_V2_ORG)
                query_api = client.query_api()

                q = '''
                from(bucket: "jstec")
                |> range(start: {s}, stop: {e})
                |> filter(fn: (r) => r["_measurement"] == "mqtt_consumer")
                |> filter(fn: (r) => r["_field"] == "rpm" or r["_field"] == "temperature_1" or r["_field"] == "temperature_2" or r["_field"] == "temperature_3" or r["_field"] == "load")
                |> aggregateWindow(every: {i}s, fn: mean, createEmpty: false)
                |> yield(name: "mean")
                '''.format(s=start_datetime_utc, e=end_datetime_utc, i=interval)
                
                tables = query_api.query(q)
                results = []
                single_datetime = True
                for table in tables:
                    for record in table.records:
                        if single_datetime==True:
                            record_time = record.get_time().astimezone(timezone('Asia/Seoul'))
                            time_diff = (record_time.replace(tzinfo=None)-start_datetime)
                            #timestring = record.get_time().astimezone(timezone('Asia/Seoul')).strftime("%Y-%m-%d %H:%M:%S") #iso formatted
                            timestring = time_diff.total_seconds() # in seconds from start time
                            results.append(("datetime", timestring))
                        results.append((record.get_field(), record.get_value()))
                    single_datetime = False

                dict_data = {}
                for field, value in results:
                    dict_data.setdefault(field, []).append(value)

                client.close()
                return Response({"data":dict_data}, status=status.HTTP_200_OK)
            return Response({}, status=status.HTTP_204_NO_CONTENT)
        
        except Exception as e:
            print("exception : ", str(e))
            return Response({"message":str(e)}, status=status.HTTP_500_INTERNAL_SERVER_ERROR)

    def post(self, request, *args, **kwargs):
        return Response({}, status=status.HTTP_400_BAD_REQUEST)

