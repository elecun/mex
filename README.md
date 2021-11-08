# mex
Machine Expert for JinsungTEC

# Setup on Ubuntu 18.04.6 LTS


```
$ sudo apt-get install ssh net-tools build-essential curl libboost-all-dev mosquitto mosquitto-clients mosquitto-dev python3.8 python3.8-venv git libmysqlclient-dev sqlite3 python3-venv
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