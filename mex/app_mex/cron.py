
import paho.mqtt.client as mqtt
import json
from api_settings.models import Reserve, Settings
from django.utils import timezone
from datetime import datetime, timedelta
from django.forms.models import model_to_dict

def on_connect(client, userdata, flags, rc):
    if rc==0:
        print("Connected")
    else:
        print("Bad connection")

def on_disconnect(client, userdata, flags, rc):
    print(str(rc))

def on_publish(client, userdata, mid):
    print("Published : ", mid)



'''
Timed Jobs
'''
def scheduled():

    # getting list of scheduled jobs
    _now = timezone.now().replace(second=0, microsecond=0)
    _end = _now+timedelta(minutes=1)
    # print(_now)
    # print(_end)
    _reserved_list = Reserve.objects.filter(start_at__gte=_now, start_at__lt=_now+timedelta(minutes=1))

    if _reserved_list.exists():
        rlist = [item for item in _reserved_list.values()]
        # print(rlist[0]["target_setting_id"]) # only first settings works
        # print(rlist[0])
        
        _setting_id = rlist[0]["target_setting_id"]
        _setting = Settings.objects.get(id=_setting_id)

        if _setting is not None:
            print(model_to_dict(_setting))


            # publish scheduled job
            client = mqtt.Client()
            client.connect('localhost', 1883)
            client.loop_start()
            client.publish('mex/step/program', json.dumps({"now":_now.strftime("%Y-%m-%d %H:%M:%S"), "end":_end.strftime("%Y-%m-%d %H:%M:%S"), "program":model_to_dict(_setting)}), 2)
            client.loop_stop()
            client.disconnect()
    else:
        print("No Reserved Jobs")
