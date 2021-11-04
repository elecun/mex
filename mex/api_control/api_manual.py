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

# # for MQTT callback
# def on_connect(client, obj, flags, rc):
#     print("mqtt connect rc: "+str(rc))

# def on_message(client, userdata, msg):
#     print(msg.topic+" "+str(msg.payload))

# def on_publish(client, obj, mid):
#     print("mid: "+str(mid))

# def on_subscribe(client, obj, mid, granted_qos):
#     print("Subscribed: "+str(mid)+" "+str(granted_qos))

# def on_log(client, obj, level, string):
#     print(string)

# def on_disconnect(client, userdata, rc):
#     print("uvlc control mqtt on_disconnect")


# mqtt_client = mqtt.Client()
# mqtt_client.on_connect = on_connect
# mqtt_client.on_disconnect = on_disconnect
# mqtt_client.on_message = on_message
# mqtt_client.connect(settings.MQTT_BROKER_ADDRESS, settings.MQTT_BROKER_PORT, 60)
# mqtt_client.loop_start()

# class API(APIView):

#     def get(self, request, *args, **kwargs):
#         return Response({}, status=status.HTTP_400_BAD_REQUEST)

#     def mc_forward(self):
#         mqtt_client.publish("mex/mc/forward", json.dumps(self.request.data), 2)
#     def mc_reverse(self):
#         mqtt_client.publish("mex/mc/reverse", json.dumps(self.request.data), 2)
#     def mc_stop(self):
#         mqtt_client.publish("mex/mc/stop", json.dumps(self.request.data), 2)
#     def mc_motor_on(self):
#         mqtt_client.publish("mex/mc/motor_on", json.dumps(self.request.data), 2)
#     def mc_motor_off(self):
#         mqtt_client.publish("mex/mc/motor_off", json.dumps(self.request.data), 2)
#     def mc_load_on(self):
#         mqtt_client.publish("mex/mc/load_on", json.dumps(self.request.data), 2)
#     def mc_load_off(self):
#         mqtt_client.publish("mex/mc/load_off", json.dumps(self.request.data), 2)
#     def mc_zero_set(self):
#         mqtt_client.publish("mex/mc/zero_set", json.dumps(self.request.data), 2)
#     def mc_emg_stop(self):
#         mqtt_client.publish("mex/emg", json.dumps(self.request.data), 2)

#     def post(self, request, *args, **kwargs):
#         try:
#             function = { 
#                 'mc_forward': self.mc_forward,
#                 "mc_reverse" : self.mc_reverse,
#                 "mc_stop": self.mc_stop,
#                 "mc_motor_on": self.mc_motor_on,
#                 "mc_motor_off": self.mc_motor_off,
#                 "mc_load_on": self.mc_load_on,
#                 "mc_load_off": self.mc_load_off, 
#                 "mc_zero_set": self.mc_zero_set,
#                 "mc_emg_stop": self.mc_emg_stop }
#             print("call ", request.data["command"])
#             call_func = function[request.data["command"]]
#             call_func()

#             return Response({"message":"ok"}, status=status.HTTP_200_OK)

#         except Exception as e:
#             return Response({"message":str(e)}, status=status.HTTP_500_INTERNAL_SERVER_ERROR)
