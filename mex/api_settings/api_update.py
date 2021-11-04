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

class API(APIView):

    def get(self, request, *args, **kwargs):
        return Response({}, status=status.HTTP_400_BAD_REQUEST)
        

    def post(self, request, *args, **kwargs):
        try :
            _id = kwargs["id"]
            _setting = Settings.objects.get(id=_id)

            if _setting is not None:
                print(request.data)
                _setting.machine_name = request.data["machine_name"]
                _setting.jsno = request.data["jsno"]
                _setting.product_size = float(request.data["product_size"])
                _setting.vload = float(request.data["vload"])
                _setting.roller_size = float(request.data["roller_size"])
                _setting.wtime = float(request.data["wtime"])
                _setting.update_interval = int(request.data["update_interval"])
                _setting.limit_temperature_min = int(request.data["limit_temperature_min"])
                _setting.limit_temperature_max = int(request.data["limit_temperature_max"])
                _setting.limit_temperature_min_count = int(request.data["limit_temperature_min_count"])
                _setting.limit_temperature_max_count = int(request.data["limit_temperature_max_count"])
                _setting.limit_rpm_max = int(request.data["limit_rpm_max"])
                _setting.limit_rpm_max_count = int(request.data["limit_rpm_max_count"])
                _setting.limit_load_min = float(request.data["limit_load_min"])
                _setting.limit_load_max = float(request.data["limit_load_max"])
                _setting.limit_load_min_count = int(request.data["limit_load_min_count"])
                _setting.limit_load_max_count = int(request.data["limit_load_max_count"])
                _setting.steps = request.data["steps"]

                _setting.save()
                return Response({}, status=status.HTTP_200_OK)
            
        except Settings.DoesNotExist:
            return Response({}, status=status.HTTP_404_NOT_FOUND)
        except Exception as e:
            print("Exception : update setting ", str(e))
            return Response({"message":str(e)}, status=status.HTTP_500_INTERNAL_SERVER_ERROR)

