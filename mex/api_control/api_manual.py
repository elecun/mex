#-*- coding:utf-8 -*-

from django.shortcuts import render
from django.contrib.auth.models import User, Group
from rest_framework import viewsets, status
from rest_framework.response import Response
from rest_framework.decorators import api_view
from rest_framework.views import APIView
from rest_framework.permissions import IsAuthenticated, AllowAny
from django.conf import settings

import paho.mqtt.client as mqtt
import json

# for MQTT callback
def on_connect(client, obj, flags, rc):
    print("mqtt connect rc: "+str(rc))

def on_message(client, userdata, msg):
    print(msg.topic+" "+str(msg.payload))

def on_publish(client, obj, mid):
    print("mid: "+str(mid))

def on_subscribe(client, obj, mid, granted_qos):
    print("Subscribed: "+str(mid)+" "+str(granted_qos))

def on_log(client, obj, level, string):
    print(string)

def on_disconnect(client, userdata, rc):
    print("uvlc control mqtt on_disconnect")


mqtt_client = mqtt.Client()
mqtt_client.on_connect = on_connect
mqtt_client.on_disconnect = on_disconnect
mqtt_client.on_message = on_message
mqtt_client.connect(settings.MQTT_BROKER_ADDRESS, settings.MQTT_BROKER_PORT, 60)
mqtt_client.subscribe("test", 2)
mqtt_client.loop_start()
print("mqtt client connecting")

class API(APIView):

    def get(self, request, *args, **kwargs):
        return Response({}, status=status.HTTP_400_BAD_REQUEST)

    def post(self, request, *args, **kwargs):
        try:
            print(request.data)
            mqtt_client.publish("test", json.dump({"data":"test"}), 2)
            return Response({"message":"ok"}, status=status.HTTP_200_OK)

        except Exception as e:
            return Response({"message":str(e)}, status=status.HTTP_500_INTERNAL_SERVER_ERROR)
