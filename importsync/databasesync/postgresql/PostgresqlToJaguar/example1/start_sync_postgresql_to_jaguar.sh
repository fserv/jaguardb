#!/bin/bash

unset _JAVA_OPTIONS

JDBC=../../../../jdbc
LIB=$JDBC/lib

pd=`pwd`
cd $JDBC
./compile.sh
cd $pd

echo ls -l $LIB
ls -l $LIB/

export LD_LIBRARY_PATH=$HOME/jaguar/lib

if [[ -f "java.lock" ]]; then
	echo "Lock file java.lock exists, sync server is running"
	echo "If the sync server is not running, please remove the java.lock "
	echo "and try again"
	/bin/ps aux|grep jdbcsql.Sync |grep -v grep
	exit 1
fi

touch java.lock
sed -i "s/.*stop=.*/#stop=true/g" appconf.postgresql

nohup java -cp $LIB/postgresql-42.1.4.jar:$LIB/jaguar-jdbc-2.0.jar:$LIB/jdbcsql.jar \
    -Dappconf=appconf.postgresql com.jaguar.jdbcsql.Sync > sync_postgresql_to_jaguar.log &

