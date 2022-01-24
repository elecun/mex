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


class API(APIView):

    permission_classes = [AllowAny]

    def get(self, request, *args, **kwargs):
        try :
            rdata = {}
            rdata["did"] = uuid.uuid4().hex
            return Response({"info":rdata, "rcode":200, "message":"successfully removed"}, status=status.HTTP_200_OK)

        except Exception as e:
            return Response({"rcode":500, "message":str(e)}, status=status.HTTP_500_INTERNAL_SERVER_ERROR)
        

    def post(self, request, *args, **kwargs):
        return Response({}, status=status.HTTP_400_BAD_REQUEST)

