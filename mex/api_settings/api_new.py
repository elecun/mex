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
            if 'name' in request.data:
                # new settings
                _new_settings = Settings()
                _new_settings.uid = uuid.uuid4().hex
                _new_settings.name = request.data["name"]
                _new_settings.note = request.data["note"]
                _new_settings.save()
                print("Registered New Settings : ", _new_settings.uid)
                _new_settings_object = model_to_dict(_new_settings)
                return Response({"data":_new_settings_object}, status=status.HTTP_200_OK)
            else:
                print("Setting name must be required")
                return Response({}, status=status.HTTP_400_BAD_REQUEST)

        except Exception as e:
            print("Exception : Regist new setting ", str(e))
            return Response({"message":str(e)}, status=status.HTTP_500_INTERNAL_SERVER_ERROR)

