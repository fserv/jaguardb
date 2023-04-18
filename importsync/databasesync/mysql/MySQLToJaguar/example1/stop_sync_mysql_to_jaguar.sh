#!/bin/bash


sed -i "s/.*stop=.*/stop=true/g" appconf.mysql
cat appconf.mysql |grep stop

