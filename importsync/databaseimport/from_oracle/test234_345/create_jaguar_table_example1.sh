#!/bin/sh

#arguments: <oraclehost> <oracleport> <oracleservice>  <table> <oracle_uid> <oracle_password> <jaguar_database> <jaguar_admin_password> 
../create_jaguar_table.sh 192.168.7.120 1522 test table234 test test test jaguar
../create_jaguar_table.sh 192.168.7.120 1522 test table345 test test test jaguar

