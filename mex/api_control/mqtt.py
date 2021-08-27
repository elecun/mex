

import paho.mqtt.client as mqtt
from django.conf import settings

'''
for singleton pattern
'''
class Singleton:
  __instance = None

  @classmethod
  def __getInstance(cls):
    return cls.__instance

  @classmethod
  def instance(cls, *args, **kargs):
    cls.__instance = cls(*args, **kargs)
    cls.instance = cls.__getInstance
    return cls.__instance

'''
singleton controlMqtt class
'''
class controlMqtt(mqtt.Client):

    def __init__(self, *args, **kwargs):
        self._connected = False
        super().__init__(*args, **kwargs)

    def __del__(self):
        self.loop_stop()
        self.disconnect()

    def is_connected(self):
        return self._connected

    def on_connect(self, client, obj, flags, rc):
        if rc==0:
            self._connected = True
        print("MQTT connected: "+str(rc))

    def on_disconnect(self, client, userdata, rc):
        print("mqtt disconnected")

    def on_message(self, client, obj, msg):
        print(msg.topic+" "+str(msg.qos)+" "+str(msg.payload))

    def on_publish(self, client, obj, mid):
        print("mid: "+str(mid))

    def on_subscribe(self, client, obj, mid, granted_qos):
        print("Subscribed: "+str(mid)+" "+str(granted_qos))

    def on_log(self, client, obj, level, string):
        print(string)

    def run(self, address, port):
        self.connect(address, port, 60)
        self.publish("aop/uvlc/data", "test", 2)
        # self.subscribe("aop/uvlc/monitor", 2)