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
from datetime import datetime, timedelta

class API(APIView):

    permission_classes = [AllowAny]

    def get(self, request, *args, **kwargs):
        return Response({}, status=status.HTTP_400_BAD_REQUEST)
        

    def post(self, request, *args, **kwargs):
        try :
            _id = kwargs["id"]
            _setting = Settings.objects.get(id=_id)

            if _setting is not None:
                _setting.steps_start_at = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                _setting.steps_stop_at = (datetime.now() + timedelta(seconds=60*60*_setting.wtime)).strftime("%Y-%m-%d %H:%M:%S")
                _setting.save()
                return Response({}, status=status.HTTP_200_OK)
            else:
                return Response({}, status=status.HTTP_204_NO_CONTENT)
            
        except Settings.DoesNotExist:
            return Response({}, status=status.HTTP_404_NOT_FOUND)
        except Exception as e:
            print("Exception : update setting ", str(e))
            return Response({"message":str(e)}, status=status.HTTP_500_INTERNAL_SERVER_ERROR)

