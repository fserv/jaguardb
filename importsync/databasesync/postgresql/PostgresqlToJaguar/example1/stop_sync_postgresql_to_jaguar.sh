#!/bin/bash


sed -i "s/.*stop=.*/stop=true/g" appconf.postgresql
cat appconf.postgresql |grep stop

