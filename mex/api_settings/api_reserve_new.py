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
from api_settings.models import Settings, Command, Reserve
from datetime import datetime

class API(APIView):

    def get(self, request, *args, **kwargs):
        return Response({}, status=status.HTTP_400_BAD_REQUEST)
        

    def post(self, request, *args, **kwargs):
        try :
            if 'datetime' in request.data:
                # new reservation
                _new_reserve = Reserve()
                _new_reserve.uid = uuid.uuid4().hex
                _new_reserve.start_at = datetime.strptime(request.data["datetime"], '%Y-%m-%d ')
                print(request.data["datetime"])
                _new_reserve.save()
                print("Registered New Reservation : ", _new_reserve.uid)
                _new_settings_object = model_to_dict(_new_reserve)
                return Response({"data":_new_settings_object}, status=status.HTTP_200_OK)
            else:
                print("Reservation date & time must be required")
                return Response({}, status=status.HTTP_400_BAD_REQUEST)
        except ValueError as e:
            print("Exception : ", str(e))
        except Exception as e:
            print("Exception : ", str(e))
            return Response({"message":str(e)}, status=status.HTTP_500_INTERNAL_SERVER_ERROR)

