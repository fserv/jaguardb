#!/bin/bash

##########################################################################################
##
## Usage:   ./create_jaguar_table.sh  oraclehost oracleport  oracleservice  table   uid password jaguardb jagadminpass
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
	elif [[ "x$otype" = "xvarchar2" ]]; then
		g_type="char"
		g_typestr="char($g_precision)"
	elif [[ "x$otype" = "xnumber" ]]; then
		if ((g_precision>0)) && ((g_scale<1)); then
			g_type="bigint"
			g_typestr="bigint"
		elif ((g_precision>0)) && ((g_scale>0)); then
			g_type="double"
			g_typestr="double($g_precision,$g_scale)"
		else
			g_type="bigint"
			g_typestr="bigint"
		fi
	elif [[ "x$typ" = "xdate" ]]; then
		g_type="date"
		g_typestr="date"
	elif [[ "x$typ" = "xtime" ]]; then
		g_type="time"
		g_typestr="time"
	elif [[ "x$typ" = "xnchar" ]]; then
		((g_precision=6*g_precision))
		g_type="char"
		g_typestr="char($g_precision)"
	elif [[ "x$typ" = "xnvarchar2" ]]; then
		g_type="char"
		((g_precision=6*g_precision))
		g_typestr="char($g_precision)"
	elif [[ "x$typ" = "xblob" ]]; then
		g_type="char"
		((g_precision=10000))
		echo "$g_col BLOB, please take care of it"
		g_typestr="char($g_precision)"
	elif [[ "x$typ" = "xclob" ]]; then
		g_type="char"
		((g_precision=10000))
		echo "$g_col CLOB, please take care of it"
		g_typestr="char($g_precision)"
	elif [[ "x$typ" = "xtimestamp" ]]; then
		if ((g_precision==9)); then
			g_type="datetimenano"
			g_typestr="datetimenano"
		else
			g_type="datetime"
			g_typestr="datetime"
		fi
	elif [[ "x$typ" = "xbinary_float" ]]; then
			g_type="float"
			g_typestr="float(20,6)"
	elif [[ "x$typ" = "xbinary_double" ]]; then
			g_type="float"
			g_typestr="float(38,12)"
	elif [[ "x$typ" = "xraw" ]]; then
			g_type="char"
			g_typestr="char($g_precision)"
	elif [[ "x$typ" = "xrowid" ]]; then
			g_type="uuid"
			g_typestr="uuid"
	elif [[ "x$typ" = "xint" ]]; then
			g_type="int"
			g_typestr="int"
	else
		g_type=$otype
		g_typestr=$otype
	fi

	#echo "s3372 g_typestr=$g_typestr"

}

######################## main ################################################
## $0  oraclehost oracleport  oracleservice  table   uid password jagdb jagadminpass
ohost=$1
oport=$2
oservice=$3
table=$4
ouid=$5
opass=$6
jagdb=$7
jagpass=$8

if [[ "x$jagpass" = "x" ]]; then
	echo "Usage:  $0  <oracle_host> <oracle_port> <oracle_service> <table> <oracle_uid> <oracle_password> <jaguar_db> <jaguar_admin_password>"
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
appconf="appconf.oracle.$$"
echo "source_jdbcurl=jdbc:oracle:thin:@//${ohost}:$oport/$oservice" > $appconf
echo "source_table=$table" >> $appconf
echo "source_user=$ouid" >> $appconf
echo "source_password=$opass" >> $appconf
echo "command=desc" >> $appconf
cat $appconf


descrc="describe_${table}.txt"
$exec_cmd $appconf > $descrc
desccolrc="describe_${table}_colname.txt"
desctyperc="describe_${table}_coltype.txt"
awk -F'|' '{print $1}' $descrc > $desccolrc
awk -F'|' '{print $2}' $descrc > $desctyperc


##################### get key columns of oracle table
TABLE=`echo $table | tr '[:lower:]' '[:upper:]'`
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
echo "Creating table, please wait a few seconds ..."
if [[ "x$uo" = "xMsys" ]] || [[ "x$uo" = "xCygwin" ]]; then
	$JAGUAR_HOME/jaguar/bin/jql.exe -u $jaguid -p $jagpass -h :$jagport -d $jagdb -f $cmd -q
else
	$JAGUAR_HOME/jaguar/bin/jql.bin -u $jaguid -p $jagpass -h :$jagport -d $jagdb -f $cmd -q
fi

cd $pd
/bin/rm -rf $dirn
