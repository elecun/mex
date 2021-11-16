
import paho.mqtt.client as mqtt
import json
from api_settings.models import Reserve, Settings
from django.utils import timezone
from datetime import datetime, timedelta
from django.forms.models import model_to_dict
from api_data import api_query_date

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
Check reserved step task
'''
def step_schedule():

    # getting list of scheduled jobs
    _now = timezone.now().replace(second=0, microsecond=0)
    _end = _now+timedelta(minutes=1)

    _reserved_list = Reserve.objects.filter(start_at__gte=_now, start_at__lt=_now+timedelta(minutes=1))

    if _reserved_list.exists():
        rlist = [item for item in _reserved_list.values()]
        # print(rlist[0]["target_setting_id"]) # only first settings works
        # print(rlist[0])
        
        _setting_id = rlist[0]["target_setting_id"]
        _setting = Settings.objects.get(id=_setting_id)
        if _setting is not None:
            _setting.steps_start_at = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            _setting.steps_stop_at = (datetime.now() + timedelta(seconds=60*60*_setting.wtime)).strftime("%Y-%m-%d %H:%M:%S")
            _setting.save()

        if _setting is not None:
            # publish scheduled job
            client = mqtt.Client()
            client.connect('localhost', 1883)
            client.loop_start()
            data_dict = model_to_dict(_setting)
            data_dict["steps"] = json.loads(data_dict["steps"]) # converting string to list
            print(data_dict)
            # client.publish('mex/step/program', json.dumps({"now":_now.strftime("%Y-%m-%d %H:%M:%S"), "end":_end.strftime("%Y-%m-%d %H:%M:%S"), "data":data_dict, "command":"start"}), 2)
            client.publish('mex/step/program', json.dumps({"data":data_dict, "command":"start"}), 2)
            client.loop_stop()
            client.disconnect()
    else:
        print("No Reserved Jobs")



'''
Check data collect from database, and push to broker
'''
def chart_schedule():
    _now = timezone.now().replace(second=0, microsecond=0)

    _chart_parameter = Chart.objects.get(id=1)
    if _chart_parameter is not None:
        _start_date = _chart_parameter["start_at"]
        print(_start_date)
    else:
        print("No start date in common chart parameter")