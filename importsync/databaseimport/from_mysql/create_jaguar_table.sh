#!/bin/bash

##########################################################################################
##
## Uaage: $0  <mysql_host> <mysql_db> <table> <mysql_uid> <mysql_password> <jaguar_db> <jaguar_admin_password>
##
##########################################################################################

### col  type(precision,scale)   (*,scale)==(38,scale)
## precision is total length in number, scale is length after the dot
g_col=""
g_type=""
g_precision=""
g_scale=""
g_typestr=""
function getColType()
{
	line=$1
	#echo "s9390 line=$line"
	g_col=`echo $line|awk -F'|' '{print $1}'`
	otype=`echo $line|awk -F'|'  '{print $2}'`
	g_precision=`echo $line|awk -F'|'  '{print $3}'`
	g_scale=`echo $line|awk -F'|'  '{print $4}'`
	#echo "s2920 g_col=$g_col   typ=$typ"
	### typ "number or number(32) or number(21, 4) or char(21) or timestamp 

	if [[ "x$otype" = "xvarchar" ]]; then
		g_type="char"
		g_typestr="char($g_precision)"
	elif [[ "x$otype" = "xchar" ]]; then
		g_type="char"
		g_typestr="char($g_precision)"
	elif [[ "x$otype" = "xdecimal" ]] || [[ "x$otype" = "xnumeric" ]]; then
			g_type="double"
			g_typestr="double($g_precision,$g_scale)"
	elif echo $typ |grep -qiE "blob|text"; then
		g_type="char"
		((g_precision=10000))
		echo "$g_col BLOB, please take care of it"
		g_typestr="char($g_precision)"
	elif [[ "x$typ" = "xtimestamp" ]]; then
		g_type="timestamp"
		g_typestr="timestamp"
	elif [[ "x$typ" = "xfloat" ]]; then
			g_type="float"
			g_typestr="float($g_precision,$g_scale)"
	elif [[ "x$typ" = "xdouble" ]]; then
			g_type="double"
			g_typestr="double($g_precision,$g_scale)"
	elif [[ "x$typ" = "xbit" ]]; then
			g_type="char"
			g_typestr="char(1)"
	elif [[ "x$typ" = "xyear" ]]; then
			g_type="char"
			g_typestr="char(4)"
	elif [[ "x$typ" = "xenum" ]]; then
			g_type="char"
			g_typestr="char(8)"
	else
		g_type=$otype
		g_typestr=$otype
	fi

	#echo "s3372 g_typestr=$g_typestr"

}

######################## main ################################################
## $0  mysqlhost mysqldb table   uid password jagdb jagadminpass
### echo " $0  <mysql_host> <mysql_db> <table> <mysql_uid> <mysql_password> <jaguar_db> <jaguar_admin_password>"
myhost=$1
mydb=$2
table=$3
myuid=$4
mypass=$5
jagdb=$6
jagpass=$7

myport=3306

if [[ "x$jagpass" = "x" ]]; then
	echo "Usage:  $0  <mysql_host> <mysql_db> <table> <mysql_uid> <mysql_password> <jaguar_db> <jaguar_admin_password>"
	exit 1
fi

pd=`pwd`
dn=`dirname $0`
dirn="tmpdir$$"
/bin/mkdir -p $dirn
cd $dirn

exec_cmd="$dn/../../../util/exec_jdbc_command.sh"
echo "exec_cmd=$exec_cmd"

cmd="tmpcmd.sql"
log="describe.log"
appconf="appconf.mysql.$$"
echo "source_jdbcurl=jdbc:mysql://${myhost}:$myport/$mydb" > $appconf
echo "source_table=$table" >> $appconf
echo "source_user=$myuid" >> $appconf
echo "source_password=$mypass" >> $appconf
echo "command=desc" >> $appconf
cat $appconf


descrc="describe_${table}.txt"
$exec_cmd $appconf > $descrc
desccolrc="describe_${table}_colname.txt"
desctyperc="describe_${table}_coltype.txt"
awk -F'|' '{print $1}' $descrc > $desccolrc
awk -F'|' '{print $2}' $descrc > $desctyperc


##################### get key columns of mysql table
lowtable=`echo $table | tr '[:upper:]' '[:lower:]'`
sed -i "s/command=.*/command=pkey/g" $appconf
$exec_cmd $appconf > keycols.txt
cat $appconf


##################### create jaguar table
/bin/rm -f $cmd
numlines=`wc -l $descrc|cut -d' ' -f1`
echo "drop table if exists $lowtable;" >> $cmd
echo "create table $lowtable (" >> $cmd
### key cols first
((nkeys=0))
while read line
do
	col=`echo $line|awk -F'|' '{print $1}'`
	typ=`echo $line|awk -F'|' '{print $2}'`
	#echo "s7320 col=[$col] typ=[$typ]"
	if ! grep -q $col keycols.txt; then
		#echo "s4372 keycols.txt has no $col skipp..."
		continue
	fi 
    #echo "s1282 getColType $line"
    getColType "$line"

	if ((nkeys==0)); then
		echo "  key:" >> $cmd
	fi
	echo "    $col $g_typestr," >> $cmd
	((nkeys=nkeys+1))
done < $descrc



### value columns
((n=1))
while read line
do
	col=`echo $line|awk -F'|' '{print $1}'`
	typ=`echo $line|awk -F'|' '{print $2}'`
	if grep -q $col keycols.txt; then
		continue
	fi 
    getColType "$line"

	if ((n==1)); then
		if (( nkeys > 0 )); then
			echo "  value: " >> $cmd
		fi
		echo "    $col $g_typestr," >> $cmd
	elif ((n<numlines)); then
		echo "    $col $g_typestr," >> $cmd
	else
		echo "    $col $g_typestr" >> $cmd
	fi
	((n=n+1))
done < $descrc

echo  "); " >> $cmd

cmd2="${cmd}.tmprc"
tr '[:upper:]' '[:lower:]' < $cmd > $cmd2
/bin/mv -f $cmd2 $cmd

if [[ -f $HOME/.jaguarhome ]]; then
	export JAGUAR_HOME=`cat $HOME/.jaguarhome`
else
	export JAGUAR_HOME=$HOME
fi

cat $cmd
jaguid=admin
jagport=`cat $JAGUAR_HOME/jaguar/conf/server.conf |grep PORT|grep -v '#'|cut -d= -f2`
uo=`uname -o`

if [[ "x$uo" = "xMsys" ]] || [[ "x$uo" = "xCygwin" ]]; then
    jql="jql.exe"
else
    jql="jql.bin"
fi

echo "Creating database $jagdb, please wait ..."
$JAGUAR_HOME/jaguar/bin/$jql -u $jaguid -p $jagpass -h :$jagport -d test -e "createdb $jagdb"
echo "Creating table $lowtable, please wait a few seconds ..."
$JAGUAR_HOME/jaguar/bin/$jql -u $jaguid -p $jagpass -h :$jagport -d $jagdb -f $cmd -q

cd $pd
/bin/rm -rf $dirn
