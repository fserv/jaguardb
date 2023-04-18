#!/bin/bash

unset _JAVA_OPTIONS
LIB=../../../jdbc/lib
JARS=$LIB/ojdbc6_11g.jar:$LIB/jaguar-jdbc-2.0.jar:$LIB/jdbcsql.jar 
nohup java -cp $JARS -Dappconf=appconf.oracle  com.jaguar.jdbcsql.Importer > import_oracle.log &

