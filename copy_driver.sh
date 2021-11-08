#!/bin/bash

if [ -z "$1" ]
  then
    echo "No Arguments (Target Machine IPv4 Address)"
    echo "Usage : ./copy_driver.sh <IP x.x.x.x> <ID> <Remote Path>"
    exit 1
fi

#copy execution file to the remote
echo "copying files to target machine"
sshpass -p 'qwer1234' scp -p -r ./dist/driver/Linux_COM_2020-12-30.tar $2@"$1":$3
echo "copied"