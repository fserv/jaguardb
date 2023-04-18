#!/bin/bash

##########################################################################################
##        DataJaguar, Inc Copy Right
##
## Create changelog table for a table in Oralce. Also creates triggers
## to track DML (insert, update, delete in PostgreSQL table
##
## Usage:   ./postgresql_create_changelog_trigger.sh  <POSTGRESQL_DATABASE> <POSTGRESQL_TABLE> 
##
##  where POSTGRESQL_DATABASE is postgreSQL database, POSTGRESQL_TABLE is postgresql table name,
##
##########################################################################################

db=$1
table=$2

if [[ "x$table" = "x" ]]; then
	echo "Usage:     $0  <DB_NAME> <TABLE_NAME>"
	echo
	echo "Example:   $0  mydb table123"
	exit 1
fi

if type psql 2>/dev/null; then
	echo "OK  found psql"
else
	echo "psql not found, quit"
	exit 1
fi

uid=$USER
PSQL="psql -d $db -U $uid -q -t"
changelog="${table}_jagchangelog"

pd=`pwd`
dirn="tmpdir$$"
/bin/mkdir -p $dirn
cd $dirn

cmd="tmpcmd.sql"
log="describe.log"
echo "\o $log;" > $cmd
echo "\d $table" >> $cmd
echo "\o" >> $cmd

$PSQL  < $cmd > err.log
if [[ ! -f $log ]]; then
	cat err.log
	cd ..
	/bin/rm -rf $dirn
	exit 1
fi

/bin/rm -f $cmd
descrc="describe_${table}.txt"
desccolrc="describe_${table}_colname.txt"
grep -v 'Table' $log |grep -vi 'null?'|grep -v '\-\-\-\-\-\-\-'|sed -e 's/not.*null//gI' -e 's/ \+/ /g' -e '/^$/d' > $descrc
/bin/rm -f $log
awk -F'|' '{print $1}' $descrc > $desccolrc

echo "\d $changelog;" | $PSQL > $log 2>&1
((changelogExist=0))
if grep -i 'Did not find' $log; then
	echo "OK, $changelog does not exist, use $changelog as changelog table"
	((changelogExist=0))
else
	((changelogExist=1))
	echo "Table $changelog exists already."
	echo "Table $changelog will be dropped and recreated."
fi

/bin/rm -f $log

##################### create changelog table
/bin/rm -f $cmd
if ((changelogExist==1)); then
    echo "drop table $changelog;" >> $cmd
fi
echo "create table if not exists $changelog (" >> $cmd
echo "    id_ bigint primary key," >> $cmd
echo "    ts_ timestamp, " >> $cmd
echo "    action_ char(1), " >> $cmd
echo "    status_ char(1), " >> $cmd
numlines=`wc -l $descrc|cut -d' ' -f1`
((n=1))
while read line
do
	col=`echo $line|awk -F'|' '{print $1}'`
	typ=`echo $line|awk -F'|' '{print $2}'`
	if ((n<numlines)); then
		echo "    $col $typ," >> $cmd
	else
		echo "    $col $typ" >> $cmd
	fi
	((n=n+1))
done < $descrc
echo  "); " >> $cmd

cmd2="${cmd}.tmprc"
tr '[:upper:]' '[:lower:]' < $cmd > $cmd2
/bin/mv -f $cmd2 $cmd

$PSQL < $cmd
echo "Created $changelog"
echo "\d $changelog" | $PSQL

### sequence
echo "DROP SEQUENCE ${table}_jagseq;" > $cmd
echo "CREATE SEQUENCE ${table}_jagseq START 1;" >> $cmd
$PSQL < $cmd > /dev/null 2>&1

##################### create postgresql trigger function
echo "DROP TRIGGER IF EXISTS ${table}_jagtrg ON ${table};" > $cmd
echo "DROP FUNCTION IF EXISTS ${table}_jagfunc();" >> $cmd
echo "CREATE FUNCTION ${table}_jagfunc() RETURNS TRIGGER AS \$jagfunc\$ " >> $cmd
echo " BEGIN" >> $cmd
echo "  IF ( TG_OP = 'INSERT' ) THEN " >> $cmd
echo "      INSERT INTO $changelog  SELECT nextval('${table}_jagseq'), now(), 'I', 'N', NEW.*;" >> $cmd
echo "      RETURN NEW; " >> $cmd
echo "  ELSIF ( TG_OP = 'DELETE' ) THEN " >> $cmd
echo "      INSERT INTO $changelog  SELECT nextval('${table}_jagseq'), now(), 'D', 'N', OLD.*;" >> $cmd
echo "      RETURN OLD; " >> $cmd
echo "  ELSIF ( TG_OP = 'UPDATE' ) THEN " >> $cmd
echo "      INSERT INTO $changelog  SELECT nextval('${table}_jagseq'), now(), 'D', 'N', OLD.*;" >> $cmd
echo "      INSERT INTO $changelog  SELECT nextval('${table}_jagseq'), now(), 'I', 'N', NEW.*;" >> $cmd
echo "      RETURN NEW; " >> $cmd
echo "  END IF;" >> $cmd
echo "      RETURN NULL;" >> $cmd
echo  "END;" >> $cmd
echo  "\$jagfunc\$ LANGUAGE plpgsql;" >> $cmd
$PSQL < $cmd
echo "Created insert function ${table}_jagfunc"

##################### create postgresql insert trigger
echo "CREATE TRIGGER ${table}_jagtrg AFTER INSERT OR UPDATE OR DELETE ON ${table}" > $cmd
echo "  FOR EACH ROW" >> $cmd
echo "  EXECUTE PROCEDURE ${table}_jagfunc();" >> $cmd
$PSQL < $cmd
echo "Created trigger ${table}_jagtrg"

/bin/rm -f $cmd
/bin/rm -f $descrc
/bin/rm -f $desccolrc
cd $pd
/bin/rm -rf $dirn
