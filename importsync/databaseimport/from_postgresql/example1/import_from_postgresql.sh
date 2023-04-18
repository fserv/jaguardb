#!/bin/bash

LIB=../../../jdbc/lib
unset _JAVA_OPTIONS

JARS=$LIB/postgresql-42.1.4.jar:$LIB/jaguar-jdbc-2.0.jar:$LIB/jdbcsql.jar 

nohup java -cp $JARS -Dappconf=appconf.postgresql com.jaguar.jdbcsql.Importer > import_postgresql.log &

