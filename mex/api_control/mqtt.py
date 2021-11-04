

# import paho.mqtt.client as mqtt
# from django.conf import settings
# import json

# # for MQTT callback
# def on_connect(client, obj, flags, rc):
#     print("> mqtt connect rc: "+str(rc))
#     mqtt_client.

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
# # mqtt_client.loop_start()