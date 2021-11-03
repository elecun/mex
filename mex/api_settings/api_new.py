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
            if 'uid' in request.data:
                Settings.objects.get(uid=request.data["uid"])
                print("This Setting is already exist")
                return Response({"status":status.HTTP_500_INTERNAL_SERVER_ERROR, "message":"This setting is already exist"}, status=status.HTTP_500_INTERNAL_SERVER_ERROR)
        except Settings.DoesNotExist:
            # new settings
            _new_settings = Settings()
            _new_settings.uid = request.data["uid"]
            _new_settings.name = request.data["name"]
            _new_settings.save()
            print("Registered New Settings")
            return Response({"status":status.HTTP_200_OK}, status=status.HTTP_200_OK)
            
        except Exception as e:
            print("Exception : Regist new setting ", str(e))
            return Response({"status":status.HTTP_500_INTERNAL_SERVER_ERROR, "message":str(e)}, status=status.HTTP_500_INTERNAL_SERVER_ERROR)

