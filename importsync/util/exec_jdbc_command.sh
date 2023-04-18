#!/bin/bash

#########################################################
## DataJaguar, Inc  CopyRight
##
## Execute commands on databaser server by JDBC
##
#########################################################

conf=$1
if [[ "x$conf" = "x" ]]; then
	echo "Usage:  $0  <appconf_file>"
	exit 1
fi

unset _JAVA_OPTIONS
dn=`dirname $0`
LIB="$dn/../jdbc/lib"
JAR="$LIB/ojdbc6_11g.jar"
JAR="$JAR:$LIB/mysql-connector-java-5.1.44-bin.jar"
JAR="$JAR:$LIB/postgresql-42.1.4.jar"
JAR="$JAR:$LIB/jaguar-jdbc-2.0.jar"
JAR="$JAR:$LIB/jdbcsql.jar"
java -cp $JAR -Dappconf=$conf  com.jaguar.jdbcsql.Command 
