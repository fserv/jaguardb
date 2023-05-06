#!/bin/bash

### copy myserv, ServerCfg.txt to all nodes and start the server
### ServerCfg6.txt contains all servers
for n in `cut -d: -f1 ServerCfg6.txt`
do
    echo "host $n ..."
    ssh $n "mkdir -p ~/consistent_hash_db"

    hostport="$n:8898 1"

    echo "host $n start myserv ..."
    ssh $n "cd ~/consistent_hash_db; echo $hostport > ServerCfg.txt; ./start.sh" 
    echo "host $n start myserv done"
done

sleep 3
for n in `cut -d: -f1 ServerCfg6.txt`
do
    echo "node $n ..."
    ssh $n "cd ~/consistent_hash_db; tail -100 myserv.log"
done

