#!/bin/bash

for n in `cut -d: -f1 ServerCfg6.txt`
do
    echo "$n ..."
    ssh $n "pkill myserv"
done

for n in `cut -d: -f1 ServerCfg6.txt`
do
    echo "$n ..."
    ssh $n "ps aux|grep myserv|grep -v grep"
done
