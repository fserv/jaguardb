#!/bin/bash

##########################################################################################
## Create changelog table for a table in Oralce. Also creates triggers
## to track DML (insert, update, delete in Oracle table
##
## Usage:   ./oracle_create_changelog_trigger.sh  <ORACLE_TABLE>  [host:port/service]
##
##  where ORACLE_TABLE is oracle table name,
##  host is oracle host name or IP, port is oracle listener port, service is its service name
##  [host:port/service] is optional and is not necessary. 
##
##########################################################################################

table=$1
remotecfg=$2

if [[ "x$table" = "x" ]]; then
	echo "Usage:     $0  <TABLE_NAME> [host:port/service]"
	echo "Example:   $0  table123"
	echo "Example:   $0  table123 192.168.7.120:1522/orclservice"
	exit 1
fi

changelog="${table}_jagchangelog"

pd=`pwd`
dirn="tmpdir$$"
/bin/mkdir -p $dirn
cd $dirn

if [[ "x$remotecfg" != "x" ]]; then
	remotecfg="@$remotecfg"
fi

cmd="tmpcmd.sql"
log="describe.log"
echo "spool $log;" > $cmd
echo "describe $table;" >> $cmd
echo "spool off;" >> $cmd

echo -n "Eenter Oracle user name: "
read uid

echo -n "Eenter Oracle user password: "
read -s pass
echo

echo "sqlplus -S $uid/$pass$remotecfg < $cmd  "
sqlplus -S $uid/$pass$remotecfg < $cmd  >/dev/null
/bin/rm -f $cmd
descrc="describe_${table}.txt"
desccolrc="describe_${table}_colname.txt"
cat $log|grep -v 'SQL>' |grep -vi 'null?'|grep -v '\-\-\-\-\-\-\-'|sed -e 's/not.*null//gI' -e 's/ \+/ /g' -e '/^$/d' > $descrc
/bin/rm -f $log
awk '{print $1}' $descrc > $desccolrc

echo "describe $changelog;" | sqlplus -S $uid/$pass$remotecfg > $log 2>&1
((changelogExist=0))
if grep -i error $log; then
	echo "OK, $changelog does not exist, use $changelog as changelog table"
	((changelogExist=0))
else
	((changelogExist=1))
	echo "Table $changelog exists already."
	echo -n "Are you sure you want to drop it and create a new table with the same name? (y|n) "
	read ans
	if [[ "x$ans" != "xy" ]]; then
		echo "quit, please try again with a different changelog table name"
		/bin/rm -f $log
		cd $pd
		/bin/rm -rf $dirn
		exit 1
	fi
fi

/bin/rm -f $log

##################### create changelog table
/bin/rm -f $cmd
if ((changelogExist==1)); then
    echo "drop table $changelog;" >> $cmd
fi
echo "create table $changelog (" >> $cmd
echo "    id_ number primary key," >> $cmd
echo "    ts_ timestamp, " >> $cmd
echo "    action_ char(1), " >> $cmd
echo "    status_ char(1), " >> $cmd
numlines=`wc -l $descrc|cut -d' ' -f1`
((n=1))
while read line
do
	if ((n<numlines)); then
		echo "    $line," >> $cmd
	else
		echo "    $line" >> $cmd
	fi
	((n=n+1))
done < $descrc
echo  "); " >> $cmd

cmd2="${cmd}.tmprc"
tr '[:upper:]' '[:lower:]' < $cmd > $cmd2
/bin/mv -f $cmd2 $cmd

sqlplus -S $uid/$pass$remotecfg < $cmd
echo "Created $changelog"
echo "describe $changelog;" | sqlplus -S $uid/$pass$remotecfg 


##################### create oracle sequence
echo "drop sequence ${changelog}_jidseq;" > $cmd
echo "create sequence ${changelog}_jidseq start with 1;" >> $cmd 
sqlplus -S $uid/$pass$remotecfg < $cmd > /dev/null 2>&1
echo "Created sequence ${changelog}_jidseq"


##################### create oracle insert trigger
echo "CREATE OR REPLACE TRIGGER ${table}_jagtrgins AFTER INSERT ON ${table}" > $cmd
echo "  FOR EACH ROW" >> $cmd
echo "  BEGIN" >> $cmd
echo "    INSERT INTO $changelog values (" >> $cmd
echo "        ${changelog}_jidseq.nextval," >> $cmd
echo "        sysdate," >> $cmd
echo "        'I'," >> $cmd
echo "        'N'," >> $cmd
((n=1))
while read line
do
	if ((n<numlines)); then
		echo "        :new.$line," >> $cmd
	else
		echo "        :new.$line" >> $cmd
	fi
	((n=n+1))
done < $desccolrc
echo  "        );" >> $cmd
echo  " END;" >> $cmd
echo  " /" >> $cmd
sqlplus -S $uid/$pass$remotecfg < $cmd
echo "Created insert trigger ${table}_jagtrgins"


##################### create oracle update trigger
echo "CREATE OR REPLACE TRIGGER ${table}_jagtrgupd AFTER UPDATE ON ${table}" > $cmd
echo "  FOR EACH ROW" >> $cmd
echo "  BEGIN" >> $cmd

echo "    INSERT INTO $changelog values (" >> $cmd
echo "        ${changelog}_jidseq.nextval," >> $cmd
echo "        sysdate," >> $cmd
echo "        'D'," >> $cmd
echo "        'N'," >> $cmd
((n=1))
while read line
do
	if ((n<numlines)); then
		echo "        :old.$line," >> $cmd
	else
		echo "        :old.$line" >> $cmd
	fi
	((n=n+1))
done < $desccolrc
echo  "        );" >> $cmd

echo "    INSERT INTO $changelog values (" >> $cmd
echo "        ${changelog}_jidseq.nextval," >> $cmd
echo "        sysdate," >> $cmd
echo "        'I'," >> $cmd
echo "        'N'," >> $cmd
((n=1))
while read line
do
	if ((n<numlines)); then
		echo "        :new.$line," >> $cmd
	else
		echo "        :new.$line" >> $cmd
	fi
	((n=n+1))
done < $desccolrc
echo  "        );" >> $cmd


echo  " END;" >> $cmd
echo  " /" >> $cmd
sqlplus -S $uid/$pass$remotecfg < $cmd
echo "Created update trigger ${table}_jagtrgupd"


##################### create oracle delete trigger
echo "CREATE OR REPLACE TRIGGER ${table}_jagdel BEFORE DELETE ON ${table}" > $cmd
echo "  FOR EACH ROW" >> $cmd
echo "  BEGIN" >> $cmd
echo "    INSERT INTO $changelog values (" >> $cmd
echo "        ${changelog}_jidseq.nextval," >> $cmd
echo "        sysdate," >> $cmd
echo "        'D'," >> $cmd
echo "        'N'," >> $cmd
((n=1))
while read line
do
	if ((n<numlines)); then
		echo "        :old.$line," >> $cmd
	else
		echo "        :old.$line" >> $cmd
	fi
	((n=n+1))
done < $desccolrc
echo  "        );" >> $cmd
echo  " END;" >> $cmd
echo  " /" >> $cmd
sqlplus -S $uid/$pass$remotecfg < $cmd
echo "Created delete trigger ${table}_jagdel"


/bin/rm -f $cmd
/bin/rm -f $descrc
/bin/rm -f $desccolrc
cd $pd
/bin/rm -rf $dirn
