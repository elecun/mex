
'''
User global variable definitions
'''
def context_processors(request):
    return {
        'system':{ 
            'title':'JSTEC',
            'company':"JINSUNG T.E.C",
            'version':"0.1.0",
            'mqtt_broker_ip':'192.168.11.24',
            'mqtt_broker_port':8083
            },
        'frontend':{
            'window_background':'linear-gradient(#010b12, #1f010f)',
            'charm_background':'#010b12', 
            'target_origin':'*' # target origin string for postmessage (generally http://~~)
        },
        'backend':{

        }
    }


