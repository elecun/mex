# mex
Machine Expert for JinsungTEC

# Setup (on Ubuntu)
```
$ sudo apt-get install curl 
$ curl -fsSL https://deb.nodesource.com/setup_14.x | sudo -E bash -
$ sudo apt-get install -y nodejs npm python3.8-venv libmysqlclient-dev git build-essential python3.8-dev mariadb-server mariadb-client libmysqlclient-dev
$ npm init
$ npm install --save-dev electron electron-builder electron-packager electron-installer-squirrel-windows
$ npm install --save python-shell
$ npm install electron-prebuilt
```

# Setup (on Raspberry pi)
* 
```
$ sudo curl -sL https://deb.nodesource.com/setup_14.x | sudo -E bash -
$ apt-get install -y nodejs
```

# Build
```
$ npm run build:linux64 (for amd64)
$ npm run build:arm64 fpr(armv7l)
```

# Install viewer on Ubuntu(amd64)
```
$ cd release
$ dpkg -i mex_0.0.01_amd64.deb
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
1. install influxdb v2 (2.0.8) for AMD64
```
$ wget https://dl.influxdata.com/influxdb/releases/influxdb2-2.0.8-amd64.deb
$ sudo dpkg -i influxdb2-2.0.8-amd64.deb
```

# install relational database
1. install mariadb
```
$ sudo apt-get install mariadb-server mariadb-client
```

# install data collector
1. install telegraf 1.19.2
```
$ wget https://dl.influxdata.com/telegraf/releases/telegraf_1.19.2-1_amd64.deb
$ sudo dpkg -i telegraf_1.19.2-1_amd64.deb
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