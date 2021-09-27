
#-*- coding:utf-8 -*-
from django.shortcuts import render
from django.contrib.auth.models import User, Group
from django.contrib.auth.forms import UserCreationForm

from rest_framework import viewsets, status
from rest_framework.response import Response
from rest_framework.decorators import api_view
from rest_framework.views import APIView
from django.forms.models import model_to_dict
from rest_framework.permissions import IsAuthenticated
from django.db.models import Q

from django.contrib.auth.models import User

class API(APIView):
    # permission_classes = [IsAuthenticated]

    def get(self, request, *args, **kwargs):
        try:
            _id = int(request.GET.get("id"))
            _user = User.objects.get(id=_id)
            
            _dict_object = model_to_dict(_user)
            _dict_object.update(model_to_dict(_user.profile))
            print(_dict_object)

            return Response({"info":_dict_object, "rcode":200, "message":"successfully removed"}, status=status.HTTP_200_OK)

        except User.DoesNotExist:
            return Response({"rcode":404, "message":"Not Found"}, status=status.HTTP_404_NOT_FOUND)
        except Exception as e:
            return Response({"rcode":500, "message":str(e)}, status=status.HTTP_500_INTERNAL_SERVER_ERROR)


    def post(self, request, *args, **kwargs):
        return Response({"rcode":400, "message":"Bad Request"}, status=status.HTTP_400_BAD_REQUEST)
            