# mex
Machine Expert for JinsungTEC

# Setup (on Ubuntu)
```
$ sudo apt-get install curl 
$ curl -fsSL https://deb.nodesource.com/setup_14.x | sudo -E bash -
$ sudo apt-get install -y nodejs npm python3.8-venv libmysqlclient-dev git build-essential python3.8-dev
$ npm init
$ npm install --save-dev electron electron-builder electron-packager electron-installer-squirrel-windows
$ npm install --save python-shell
$ npm install electron-prebuilt
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