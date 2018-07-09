#!/bin/bash

cd /home/centos/wenhan/qianjia

C8Y_LIB_PATH=/home/centos/wenhan/cumulocity-sdk-c

export LD_LIBRARY_PATH=$C8Y_LIB_PATH/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

./bin/qianjia_agent
#./bin/srwatchdogd ./bin/dtu_agent 100 > log/dtu.log 2>&1 &
