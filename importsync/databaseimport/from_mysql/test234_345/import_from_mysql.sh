#!/bin/bash

LIB=../../../jdbc/lib
JARS=$LIB/mysql-connector-java-5.1.43-bin.jar:$LIB/jaguar-jdbc-2.0.jar:$LIB/jdbcsql.jar

nohup java -cp $JARS -Dappconf=appconf.mysql com.jaguar.jdbcsql.Importer >> import_mysql.log &

