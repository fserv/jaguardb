#!/bin/bash


sed -i "s/.*stop=.*/stop=true/g" appconf.oracle
cat appconf.oracle |grep stop

