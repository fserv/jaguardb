#!/bin/bash

export MALLOC_CHECK=3
ulimit -c unlimited

cd ~/consistent_hash_db
./myserv > myserv.log 2>&1 &

