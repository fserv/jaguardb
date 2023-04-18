#!/bin/bash

##########################################################################################
##
## Uaage: $0  <postgresql_host> <postgresql_db> <table> <postgresql_uid> <postgresql_password> <jaguar_db> <jaguar_admin_password>
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
	g_col=`echo $line|awk -F'|' '{print $1}'`
	otype=`echo $line|awk -F'|'  '{print $2}'`
	g_precision=`echo $line|awk -F'|'  '{print $3}'`
	g_scale=`echo $line|awk -F'|'  '{print $4}'`

	if [[ "x$otype" = "xvarchar" ]] || [[ "x$otype" = "xcharacter varying" ]]
	then
		g_type="char"
		g_typestr="char($g_precision)"
	elif [[ "x$otype" = "xchar" ]] || [[ "x$otype" = "xcharacter" ]] ; then
		g_type="char"
		g_typestr="char($g_precision)"
	elif [[ "x$otype" = "xtext" ]]; then
			g_type="char"
			g_typestr="char(1000)"
			echo "Text field, please handle it"
	elif [[ "x$otype" = "xdecimal" ]] || [[ "x$otype" = "xnumeric" ]] ; then
			g_type="double"
			g_typestr="double($g_precision,$g_scale)"
	elif [[ "x$typ" = "xreal" ]]; then
		g_type="float"
		g_typestr="float(15,6)"
	elif [[ "x$typ" = "xsmallserial" ]]; then
		g_type="smallint"
		g_typestr="smallint"
		echo "smallserial field, please handle it"
	elif [[ "x$typ" = "xserial" ]]; then
		g_type="int"
		g_typestr="int"
		echo "serial field, please handle it"
	elif [[ "x$typ" = "xbigserial" ]]; then
		g_type="bigint"
		g_typestr="bigint"
		echo "bigserial field, please handle it"
	elif [[ "x$typ" = "xdouble precision" ]]; then
		g_type="double"
		g_typestr="double(30,15)"
	elif [[ "x$typ" = "xint4" ]]; then
		g_type="int"
		g_typestr="int"
	elif [[ "x$typ" = "xtimestamp" ]]; then
		g_type="timestamp"
		g_typestr="timestamp"
	elif [[ "x$typ" = "xfloat" ]]; then
			g_type="float"
			g_typestr="float($g_precision,$g_scale)"
	elif [[ "x$typ" = "xdouble" ]]; then
			g_type="double"
			g_typestr="double($g_precision,$g_scale)"
	elif [[ "x$typ" = "xmoney" ]]; then
			g_type="double"
			g_typestr="double(20,2)"
	elif [[ "x$typ" = "xbit" ]]; then
			g_type="char"
			g_typestr="char(1)"
	elif [[ "x$typ" = "xbytea" ]]; then
			g_type="char"
			g_typestr="char(4)"
	elif [[ "x$typ" = "xyear" ]]; then
			g_type="char"
			g_typestr="char(4)"
	elif [[ "x$typ" = "xboolean" ]]; then
			g_type="char"
			g_typestr="char(1)"
	else
		g_type=$otype
		g_typestr=$otype
	fi

	#echo "s3372 g_typestr=$g_typestr"

}

######################## main ################################################
## $0  postgresqlhost postgresqldb table   uid password jagdb jagadminpass
### echo " $0  <postgresql_host> <postgresql_db> <table> <postgresql_uid> <postgresql_password> <jaguar_db> <jaguar_admin_password>"
myhost=$1
mydb=$2
table=$3
myuid=$4
mypass=$5
jagdb=$6
jagpass=$7

myport=5432

if [[ "x$jagpass" = "x" ]]; then
	echo "Usage:  $0  <postgresql_host> <postgresql_db> <table> <postgresql_uid> <postgresql_password> <jaguar_db> <jaguar_admin_password>"
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
appconf="appconf.postgresql.$$"
echo "source_jdbcurl=jdbc:postgresql://${myhost}:$myport/$mydb" > $appconf
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


##################### get key columns of postgresql table
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
jagport=`cat $JAGUAR_HOME/conf/server.conf |grep PORT|grep -v '#'|cut -d= -f2`
uo=`uname -o`

if [[ "x$uo" = "xMsys" ]] || [[ "x$uo" = "xCygwin" ]]; then
    jql="jql.exe"
else
    jql="jql.bin"
fi

echo "Creating database $jagdb, please wait ..."
$JAGUAR_HOME/bin/$jql -u $jaguid -p $jagpass -h :$jagport -d test -e "createdb $jagdb"
echo "Creating table $lowtable, please wait a few seconds ..."
$JAGUAR_HOME/bin/$jql -u $jaguid -p $jagpass -h :$jagport -d $jagdb -f $cmd -q

cd $pd
/bin/rm -rf $dirn
