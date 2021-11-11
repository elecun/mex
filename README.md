# mex
Machine Expert for JinsungTEC

# Setup on Ubuntu 18.04.6 LTS


```
$ sudo apt-get install ssh net-tools build-essential curl libboost-all-dev mosquitto mosquitto-clients mosquitto-dev
$ sudo apt-get install python3.8 python3.8-venv git libmysqlclient-dev sqlite3 python3-venv libmosquitto-dev python3-pip gunicorn
$ pip3 install -r requirements.txt
$ pip3 install requests
$ sudo apt-get install g++-8 gcc-8
$ wget https://dl.influxdata.com/influxdb/releases/influxdb2-2.0.9-amd64.deb
$ sudo dpkg -i influxdb2-2.0.9-amd64.deb
$ wget https://dl.influxdata.com/telegraf/releases/telegraf_1.20.3-1_amd64.deb
$ sudo dpkg -i telegraf_1.20.3-1_amd64.deb
$ sudo dpkg -i mex-0.0.2_amd64.deb
$ sudo update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.8 1
$ curl -sL https://deb.nodesource.com/setup_14.x -o nodesource_14_setup.sh
$ chmod +x nodesource_14_setup.sh
$ sudo bash nodesource_setup.sh
$ sudo apt-get install -y nodejs
```
- boost library 1.65
- python3.8
- gcc 7.5

## Config Mosquitto.conf (/etc/mosquitto/mosquitto.conf)
```
listener 1883
protocol mqtt

listener 8083
protocol websockets

allow_anonymous true
```
```
$ sudo systemctl restart mosquitto.service
```

## Config Telegraf.conf (/etc/telegraf/telegraf.conf)
```
$ sudo systemctl start influxd.service
```
* influxdb(http://x.x.x.x:8086)에서 username:jstec2, password:qwer1234, organization:jstec, bucket:jstec 으로 설정
```
[[inputs.mqtt_consumer]]
servers = ["tcp://127.0.0.1:1883"]
topics = [
    "mex/sensor/#"
]
qos = 2
data_format = "json"

[[outputs.influxdb_v2]]
urls=["http://127.0.0.1:8086"]
token="<from influxdb>"
organization="jstec"
bucket="jstec"
user_agent="telegraf"

```
```
$ sudo systemctl start telegraf
```

## setup from source
```
$ git clone https://github.com/elecun/mex.git
mex/$ python3 -m venv venv
(venv)$ pip3 install -r requirements.txt
(venv)$ gunicorn --bind 0:8000 mex.wsgi
```

## register service all applications
* /etc/init.d/에 mex_server 생성
```
#!/bin/sh
### BEGIN INIT INFO
# Provides:          mex_server
# Required-Start:    $all
# Required-Stop:
# Default-Start:     3 4 5
# Default-Stop:
# Short-Description: your description here
### END INIT INFO

PATH=/bin:/usr/bin:/sbin:/usr/sbin:/home/jstec2/software/mex/mex
DESC="mex_server daemon"
NAME=mex_server
DAEMON=/usr/bin/gunicorn
PIDFILE=/var/run/mex_serverd.pid
SCRIPTNAME=/etc/init.d/"$NAME"


case "$1" in
start)  log_daemon_msg "Starting mex_server" "mex_server"
        start_daemon -p $PIDFILE $DAEMON $EXTRA_OPTS
        log_end_msg $?
        ;;
stop)   log_daemon_msg "Stopping mex_server" "mex_server"
        killproc -p $PIDFILE $DAEMON
        RETVAL=$?
        [ $RETVAL -eq 0 ] && [ -e "$PIDFILE" ] && rm -f $PIDFILE
        log_end_msg $RETVAL
        ;;
restart) log_daemon_msg "Restarting mex_server" "mex_server" 
        $0 stop
        $0 start
        ;;
status)
        status_of_proc -p $PIDFILE $DAEMON $NAME && exit 0 || exit $?
        ;;
*)      log_action_msg "Usage: /etc/init.d/mex_server {start|stop|status|restart|reload}"
        exit 2
        ;;
esac
exit 0
```

* /etc/init.d/에 mex_load 생성
```
#!/bin/sh
### BEGIN INIT INFO
# Provides:          mex_load
# Required-Start:    $all
# Required-Stop:
# Default-Start:     3 4 5
# Default-Stop:
# Short-Description: your description here
### END INIT INFO

PATH=/bin:/usr/bin:/sbin:/usr/sbin:/home/jstec2/software/mex/middleware/dist
DESC="mex_load daemon"
NAME=mex_load
DAEMON=/home/jstec2/software/mex/middleware/dist/mex_load
PIDFILE=/var/run/mex_loadd.pid
SCRIPTNAME=/etc/init.d/"$NAME"


case "$1" in
start)  log_daemon_msg "Starting mex_load" "mex_load"
        start_daemon -p $PIDFILE $DAEMON $EXTRA_OPTS
        log_end_msg $?
        ;;
stop)   log_daemon_msg "Stopping mex_load" "mex_load"
        killproc -p $PIDFILE $DAEMON
        RETVAL=$?
        [ $RETVAL -eq 0 ] && [ -e "$PIDFILE" ] && rm -f $PIDFILE
        log_end_msg $RETVAL
        ;;
restart) log_daemon_msg "Restarting mex_load" "mex_load" 
        $0 stop
        $0 start
        ;;
status)
        status_of_proc -p $PIDFILE $DAEMON $NAME && exit 0 || exit $?
        ;;
*)      log_action_msg "Usage: /etc/init.d/mex_load {start|stop|status|restart|reload}"
        exit 2
        ;;
esac
exit 0
```

* /etc/init.d/에 mex_relay 생성
```
#!/bin/sh
### BEGIN INIT INFO
# Provides:          mex_relay
# Required-Start:    $all
# Required-Stop:
# Default-Start:     3 4 5
# Default-Stop:
# Short-Description: your description here
### END INIT INFO

PATH=/bin:/usr/bin:/sbin:/usr/sbin:/home/jstec2/software/mex/middleware/dist
DESC="mex_relay daemon"
NAME=mex_relay
DAEMON=/home/jstec2/software/mex/middleware/dist/mex_relay
PIDFILE=/var/run/mex_relayd.pid
SCRIPTNAME=/etc/init.d/"$NAME"


case "$1" in
start)  log_daemon_msg "Starting mex_relay" "mex_relay"
        start_daemon -p $PIDFILE $DAEMON $EXTRA_OPTS
        log_end_msg $?
        ;;
stop)   log_daemon_msg "Stopping mex_relay" "mex_relay"
        killproc -p $PIDFILE $DAEMON
        RETVAL=$?
        [ $RETVAL -eq 0 ] && [ -e "$PIDFILE" ] && rm -f $PIDFILE
        log_end_msg $RETVAL
        ;;
restart) log_daemon_msg "Restarting mex_relay" "mex_relay" 
        $0 stop
        $0 start
        ;;
status)
        status_of_proc -p $PIDFILE $DAEMON $NAME && exit 0 || exit $?
        ;;
*)      log_action_msg "Usage: /etc/init.d/mex_relay {start|stop|status|restart|reload}"
        exit 2
        ;;
esac
exit 0
```
* /etc/init.d/에 mex_tpm 생성
```
#!/bin/sh
### BEGIN INIT INFO
# Provides:          mex_tpm
# Required-Start:    $all
# Required-Stop:
# Default-Start:     3 4 5
# Default-Stop:
# Short-Description: your description here
### END INIT INFO

PATH=/bin:/usr/bin:/sbin:/usr/sbin:/home/jstec2/software/mex/middleware/dist
DESC="mex_tpm daemon"
NAME=mex_tpm
DAEMON=/home/jstec2/software/mex/middleware/dist/mex_tpm
PIDFILE=/var/run/mex_tpmd.pid
SCRIPTNAME=/etc/init.d/"$NAME"


case "$1" in
start)  log_daemon_msg "Starting mex_tpm" "mex_tpm"
        start_daemon -p $PIDFILE $DAEMON $EXTRA_OPTS
        log_end_msg $?
        ;;
stop)   log_daemon_msg "Stopping mex_tpm" "mex_tpm"
        killproc -p $PIDFILE $DAEMON
        RETVAL=$?
        [ $RETVAL -eq 0 ] && [ -e "$PIDFILE" ] && rm -f $PIDFILE
        log_end_msg $RETVAL
        ;;
restart) log_daemon_msg "Restarting mex_tpm" "mex_tpm" 
        $0 stop
        $0 start
        ;;
status)
        status_of_proc -p $PIDFILE $DAEMON $NAME && exit 0 || exit $?
        ;;
*)      log_action_msg "Usage: /etc/init.d/mex_tpm {start|stop|status|restart|reload}"
        exit 2
        ;;
esac
exit 0
```

* /etc/init.d/에 mex_plc 생성
```
#!/bin/sh
### BEGIN INIT INFO
# Provides:          mex_plc
# Required-Start:    $all
# Required-Stop:
# Default-Start:     3 4 5
# Default-Stop:
# Short-Description: your description here
### END INIT INFO

PATH=/bin:/usr/bin:/sbin:/usr/sbin:/home/jstec2/software/mex/middleware/dist
DESC="mex_plc daemon"
NAME=mex_plc
DAEMON=/home/jstec2/software/mex/middleware/dist/mex_plc
PIDFILE=/var/run/mex_plcd.pid
SCRIPTNAME=/etc/init.d/"$NAME"


case "$1" in
start)  log_daemon_msg "Starting mex_plc" "mex_plc"
        start_daemon -p $PIDFILE $DAEMON $EXTRA_OPTS
        log_end_msg $?
        ;;
stop)   log_daemon_msg "Stopping mex_plc" "mex_plc"
        killproc -p $PIDFILE $DAEMON
        RETVAL=$?
        [ $RETVAL -eq 0 ] && [ -e "$PIDFILE" ] && rm -f $PIDFILE
        log_end_msg $RETVAL
        ;;
restart) log_daemon_msg "Restarting mex_plc" "mex_plc" 
        $0 stop
        $0 start
        ;;
status)
        status_of_proc -p $PIDFILE $DAEMON $NAME && exit 0 || exit $?
        ;;
*)      log_action_msg "Usage: /etc/init.d/mex_plc {start|stop|status|restart|reload}"
        exit 2
        ;;
esac
exit 0
```

* /etc/init.d/에 mex_scheduler 생성
```
#!/bin/sh
### BEGIN INIT INFO
# Provides:          mex_scheduler
# Required-Start:    $all
# Required-Stop:
# Default-Start:     3 4 5
# Default-Stop:
# Short-Description: your description here
### END INIT INFO

PATH=/bin:/usr/bin:/sbin:/usr/sbin:/home/jstec1/software/mex/middleware/dist
DESC="mex_scheduler daemon"
NAME=mex_scheduler
DAEMON=/home/jstec1/software/mex/middleware/dist/mex_scheduler
PIDFILE=/var/run/mex_schedulerd.pid
SCRIPTNAME=/etc/init.d/"$NAME"


case "$1" in
start)  log_daemon_msg "Starting mex_scheduler" "mex_scheduler"
        start_daemon -p $PIDFILE $DAEMON $EXTRA_OPTS
        log_end_msg $?
        ;;
stop)   log_daemon_msg "Stopping mex_scheduler" "mex_scheduler"
        killproc -p $PIDFILE $DAEMON
        RETVAL=$?
        [ $RETVAL -eq 0 ] && [ -e "$PIDFILE" ] && rm -f $PIDFILE
        log_end_msg $RETVAL
        ;;
restart) log_daemon_msg "Restarting mex_scheduler" "mex_scheduler"
        $0 stop
        $0 start
        ;;
status)
        status_of_proc -p $PIDFILE $DAEMON $NAME && exit 0 || exit $?
        ;;
*)      log_action_msg "Usage: /etc/init.d/mex_scheduler {start|stop|status|restart|reload}"
        exit 2
        ;;
esac
exit 0

```

* 부팅시 실행되도록 설정
```
$ sudo chmod 755 /etc/init.d/mex_load
$ sudo chmod 755 /etc/init.d/mex_relay
$ sudo chmod 755 /etc/init.d/mex_tpm
$ sudo chmod 755 /etc/init.d/mex_plc
$ sudo update-rc.d mex_load defaults
$ sudo update-rc.d mex_relay defaults
$ sudo update-rc.d mex_tpm defaults
$ sudo update-rc.d mex_plc defaults
```

* /etc/systemd/system에 mex_load.service 생성
```
$ sudo nano /etc/systemd/system/mex_load.service

[Unit]
Description=MEX Load Middleware
Wants=network-online.target
After=network-online.target
 
[Service]
ExecStart=/home/jstec2/software/mex/middleware/dist/mex_load -p /dev/ttyS0 -b 9600 -t 127.0.0.1 -i 1
WorkingDirectory=/home/jstec2/software/mex/middleware/dist
 
[Install]
WantedBy=default.target
```
```
$ sudo nano /etc/systemd/system/mex_relay.service

[Unit]
Description=MEX Relay Middleware
Wants=network-online.target
After=network-online.target
 
[Service]
ExecStart=/home/jstec1/software/mex/middleware/dist/mex_relay -p /dev/ttyUSB0 -b 9600 -t 127.0.0.1 -i 1
WorkingDirectory=/home/jstec1/software/mex/middleware/dist
 
[Install]
WantedBy=default.target
```

```
$ sudo nano /etc/systemd/system/mex_tpm.service

[Unit]
Description=MEX Temperature and RPM Middleware
Wants=network-online.target
After=network-online.target
 
[Service]
ExecStart=/home/jstec1/software/mex/middleware/dist/mex_tpm -p /dev/ttyAP0 -b 9600 -t 127.0.0.1 -i 1
WorkingDirectory=/home/jstec1/software/mex/middleware/dist
 
[Install]
WantedBy=default.target
```

```
$ sudo nano /etc/systemd/system/mex_plc.service

[Unit]
Description=MEX PLC Middleware
Wants=network-online.target
After=network-online.target
 
[Service]
ExecStart=/home/jstec1/software/mex/middleware/dist/mex_plc -p /dev/ttyAP1 -b 38400 -t 127.0.0.1 -i 1
WorkingDirectory=/home/jstec1/software/mex/middleware/dist
 
[Install]
WantedBy=default.target
```

```
$ sudo nano /etc/systemd/system/mex_scheduler.service

[Unit]
Description=MEX Scheduler Middleware
Wants=network-online.target
After=network-online.target
 
[Service]
ExecStart=/home/jstec1/software/mex/middleware/dist/mex_scheduler -t 127.0.0.1
WorkingDirectory=/home/jstec1/software/mex/middleware/dist
 
[Install]
WantedBy=default.target
```
```
$ sudo nano /etc/systemd/system/mex_server.service

[Unit]
Description=MEX Server
Wants=network-online.target
After=network-online.target
 
[Service]
ExecStart=gunicorn --bind 0.0.0.0:8000 mex.wsgi
WorkingDirectory=/home/jstec2/software/mex/mex
 
[Install]
WantedBy=default.target
```

* 서비스 등록
```
$ sudo systemctl daemon-reload
$ sudo systemctl enable edge (부팅시 자동 실행되도록..)
$ sudo systemctl start edge
```

# Setup (on Ubuntu)
```
$ source ./venv/bin/activate
(venv)$ sudo apt-get install curl
(venv)$ curl -fsSL https://deb.nodesource.com/setup_14.x | sudo -E bash -
(venv)$ sudo apt-get install -y nodejs 
(venv)$ sudo apt-get install -y npm
```
* if it shows problems for package dependency,
```
(venv)$ sudo apt-get install aptitude
(venv)$ sudo aptitude install -y nodejs npm
```
```
(venv)$ sudo apt-get install libmysqlclient-dev git build-essential python3.8-dev mariadb-server mariadb-client libmysqlclient-dev
(venv)$ npm init (create only, do not type this command!!)
(venv)$ npm install --save-dev electron electron-builder electron-packager electron-installer-squirrel-windows
(venv)$ npm install --save python-shell
(venv)$ npm install electron-prebuilt
```

# Setup (on Raspberry pi)
* 
```
$ sudo curl -sL https://deb.nodesource.com/setup_14.x | sudo -E bash -
$ apt-get install -y nodejs
```

# Build & copy to target machine
```
(venv) $ npm run build:linux64 (for amd64)
$ npm run build:arm64 fpr(armv7l)
(venv)$ ./copy 192.168.x.x jstec2 /home/jstec2
```

# Install viewer on Ubuntu(remote or local)
```
$ dpkg -i mex_0.0.2_amd64.deb
```


# Edit viewer config file(config.json)
```
$ sudo nano /opt/mex/resources/app/config.json
```

# Install viewer on Raspberry pi(armv7l)
```
$ sudo apt-get upgrade
$ sudo apt-get install libappindicator3-1
$ dpkg -i mex_0.0.01_amd64.deb
```

# (for dev)
```
$ django-admin startproject mex
$ cd mex
$ python3 manage.py startapp app_mex
```





# install timeseries database
1. install influxdb v2 (2.0.9) for AMD64
```
$ wget https://dl.influxdata.com/influxdb/releases/influxdb2-2.0.9-amd64.deb
$ sudo dpkg -i influxdb2-2.0.9-amd64.deb
```

# install relational database
1. install mariadb
```
$ sudo apt-get install mariadb-server mariadb-client
```

# install data collector
1. install telegraf 1.20.2
```
$ wget https://dl.influxdata.com/telegraf/releases/telegraf_1.20.2-1_amd64.deb
$ sudo dpkg -i telegraf_1.20.2-1_amd64.deb
```

# setup rdbms
1. setup for account and security of RDBMS
```
$ sudo mysql_secure_installation
- root password : <password>
- remove anonymous users ? y
- disallow root login remotely? n
- remove test database and access to it? y
- reload pribilege tables now? y
$ (for access) sudo mysql -u root -p
```

# run server with gunicorn
```
$ (venv) root@machine:~/jtecmex/mex$ gunicorn --bind 0:8000 mex.wsgi
```

# Django Backend
1. (for linux) $ sudo apt-get install libmysqlclient-dev
1. (for mac) $ brew install mysql sqlite python@3.8
2. $ source ./venv/bin/activate
3. $ pip3 install -r requirements.txt
4. (for mac) add belows into ~/.zshrc
```
export PATH="/usr/local/opt/sqlite/bin:$PATH"
export LDFLAGS="-L/usr/local/opt/sqlite/lib"
export CPPFLAGS="-I/usr/local/opt/sqlite/include"
export PATH="/usr/local/opt/python@3.8/bin:$PATH"
export LDFLAGS="-L/usr/local/opt/python@3.8/lib"
```