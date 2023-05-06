#!/bin/bash

for i in `cut -d: -f1 ServerCfg6.txt`
do
    ssh $i "mkdir -p ~/consistent_hash_db"
    echo "scp myserv to $i ..."
    scp myserv start.sh stop.sh $i:~/consistent_hash_db

done
