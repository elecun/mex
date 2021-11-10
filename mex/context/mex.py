
from django.conf import settings

'''
User global variable definitions
'''
def context_processors(request):
    return {
        'system':{ 
            'title':'JSTEC',
            'company':"JINSUNG T.E.C",
            'version':"0.1.0",
            'mqtt_broker_ip':str(settings.MQTT_BROKER_ADDRESS),
            'mqtt_broker_port':int(settings.MQTT_BROKER_WEBSCOKET)
            },
        'frontend':{
            'status_load_min':0,
            'status_load_max':1000,
            'status_rpm_min':0,
            'status_rpm_max':1000,
            'status_temperature1_min':0,
            'status_temperature1_max':100,
            'status_temperature2_min':0,
            'status_temperature2_max':100,
            'status_temperature3_min':0,
            'status_temperature3_max':100,
        },
        'backend':{

        },
        'menu_ko':{
            'settings_details':"상세설정",
            'settings_general':"일반설정",
            'settings_load':"불러오기",
            'settings_save' : "설정저장",
            'settings_new' : "새 설정",
            'settings_reserve' : "시험예약",
            'start_step' : "시험시작",
            'pause_step' : "시험일시정지",
            'stop_step' : "시험중지",
            'zeroset' : "영점설정",
            'settings_name':"설정명",
            'settings_general_machinename':"장비명",
            'settings_general_jsno':"JS No.",
            'settings_general_productsize':"제품 사이즈(ø)",
            'settings_general_rollersize':"구동롤러 사이즈(ø)",
            'settings_general_jsno':"JS No.",
            'settings_general_wtime':"시험시간(hr)",
            'settings_general_update_interval':"측정 주기(sec)",
            'settings_general_rpm':"제한속도(rpm)",
            'settings_general_rpm_max':"max",
            'settings_general_rpm_max_count':"count",
            'settings_general_temp':"제한온도(℃)",
            'settings_general_temp_min':"min",
            'settings_general_temp_max':"max",
            'settings_general_temp_min_count':"count",
            'settings_general_temp_max_count':"count",
            'settings_general_vload':"수직하중(kgf)",
            'settings_general_load':"제한하중(kgf)",
            'settings_general_load_min':"min",
            'settings_general_load_max':"max",
            'settings_general_load_min_count':"count",
            'settings_general_load_max_count':"count",
            'status_current':"현재 상태",
            'status_manual_control':"수동 제어",
            'status_manual_control_forward':"정회전",
            'status_manual_control_reverse':"역회전",
            'status_manual_control_stop':"중지",
            'status_manual_control_motor':"모터 ON/OFF",
            'status_manual_control_sideload':"측면하중 ON/OFF",
            'status_load':"현재 하중(kgf)",
            'status_rpm':"현재 속도(rpm)",
            'status_temperature_1':"P1 온도(℃)",
            'status_temperature_2':"P2 온도(℃)",
            'status_temperature_3':"P3 온도(℃)",
            'status_inprogress_step':"현재 STEP",
            'status_inprogress_time':"현재 진행시간",
            'chart_open':"데이터 열기",
            'chart_raw_download':"CSV 내보내기",
            'chart_image_download':"이미지 내보내기",
            'settings_reserve_datetime':"실행 날짜/시각",
            'settings_reserve_add':"예약 적용",
            'settings_reserve_list':"예약 리스트",
            'message_update_success':"정상적으로 설정을 저장 및 업데이트 하였습니다.",
            'message_update_error_no_target':"업데이트할 설정 대상이 지정되지 않았습니다.",
            'message_load_selected':"선택한 설정을 불러오기 하시겠습니까?",
            'message_delete_setting_selected':"선택한 설정을 삭제 하시겠습니까?",
            'message_delete_reserve_selected':"선택한 예약을 삭제 하시겠습니까?",
            'message_start_steps':"시험을 시작하시겠습니까?",
            'message_stopped_steps':"시험이 종료되었습니다.",
            'message_paused_steps':"시험이 일시 정지 되었습니다.",
            'chart_title':"측정 데이터",
        }
    }


