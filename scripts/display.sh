#!/bin/bash

IP_ADDRESS=`head -n 1 config.ini`

cd ../build

./mcamip -a $IP_ADDRESS -x
