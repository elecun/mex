
'''
User global variable definitions
'''
def context_processors(request):
    return {
        'system':{ 
            'title':'JSTEC MEX',
            'mqtt_broker_ip':'168.126.66.23',
            'mqtt_broker_port':9001
            },
        'frontend':{
            'window_background':'linear-gradient(#010b12, #1f010f)',
            'charm_background':'#010b12', 
            'target_origin':'*' # target origin string for postmessage (generally http://~~)
        },
        'backend':{

        }
    }


