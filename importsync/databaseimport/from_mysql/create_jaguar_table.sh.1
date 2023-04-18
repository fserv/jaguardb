#!/bin/bash

##########################################################################################
##
## Usage:   ./create_jaguar_table.sh  <DATABASE> <TABLE>
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
	g_col=`echo $line|awk '{print $1}'`
	typ=`echo $line|awk '{print $2}'`
	#echo "s2920 g_col=$g_col   typ=$typ"
	### typ "number or number(32) or number(21, 4) or char(21) or timestamp 
	if echo $typ |grep -q '('; then
		otype=`echo $typ|awk -F'(' '{print $1}'`
		t1=`echo $typ|awk -F'(' '{print $2}'`
		t2=`echo $t1|tr -d ')'`
		#echo "s8281 otype=$otype  t2=$t2"
		## t2 is  32 or 12,4 
		if echo $t2|grep -q ',' 
		then
			g_precision=`echo $t2| awk -F',' '{print $1}'| tr -d ' '`
			if [[ "x*" = "x$g_precision" ]]; then
				g_precision=38
			fi
			g_scale=`echo $t2| awk -F',' '{print $2}' | tr -d ' '`
		else
			g_precision=`echo $t2|tr -d  ' '`
			g_scale=0
		fi
	else
		otype=$typ
		g_precision=0
		g_scale=0
		#echo "s8283 otype=$otype  g_precision=$g_precision g_scale=$g_scale"
	fi

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

######################## main ###########################

db=$1
table=$2

if [[ "x$table" = "x" ]]; then
	echo "Usage:     $0  <DATABASE> <TABLE_NAME>"
	echo
	echo "Example:   $0  mydb table123 "
	exit 1
fi

if type sqlplus; then
	echo "sqlplus is found, continue ..."
else
	echo "sqlplus is not found, quit"
	exit 1
fi

pd=`pwd`
dirn="tmpdir$$"
/bin/mkdir -p $dirn
cd $dirn

echo -n "Enter MySQL user name: "
read uid
echo -n "Enter $uid password: "
read -s pass
echo

cmd="tmpcmd.sql"
log="describe.log"
echo "describe $table;" >> $cmd

mysql -s -u$uid -p$pass $db < $cmd  > $log 2>&1
/bin/rm -f $cmd

if grep -i error $log; then
	echo "Table $table does not exists in mysql/$db"
	exit 1
fi

descrc="describe_${table}.txt"
desccolrc="describe_${table}_colname.txt"
desctyperc="describe_${table}_coltype.txt"
desckeyrc="describe_${table}_colkey.txt"
cat $log|tr '[:upper:]' '[:lower:]'|grep -vi 'warning'|grep -v 'Default Extra'|sed -e 's/ \+/ /g' -e '/^$/d' > $descrc
/bin/rm -f $log
awk '{print $1}' $descrc > $desccolrc
awk '{print $2}' $descrc > $desctyperc
awk '{print $4}' $descrc > $desckeyrc


##################### get key columns of oracle table
TABLE=`echo $table | tr '[:lower:]' '[:upper:]'`
lowtable=`echo $table | tr '[:upper:]' '[:lower:]'`
cat $descrc | grep PRI | awk '{print $1}' > keycols.txt


##################### create jaguar table
/bin/rm -f $cmd
numlines=`wc -l $descrc|cut -d' ' -f1`
echo "drop table if exists $lowtable;" >> $cmd
echo "create table $lowtable (" >> $cmd
### key cols first
((nkeys=0))
while read line
do
	col=`echo $line|awk '{print $1}'`
	typ=`echo $line|awk '{print $2}'`
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
	col=`echo $line|awk '{print $1}'`
	typ=`echo $line|awk '{print $2}'`
	if grep -q $col keycols.txt; then
		continue
	fi 
    getColType "$line"

	if ((n==1)); then
		if ((nkeys>0)); then
			echo "  value: " >> $cmd
		fi
		echo "    $col $g_typestr, " >> $cmd
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

#cat $cmd
echo "Creating $table in Jaguar database"
uid=admin
echo -n "Enter jaguar admin password: "
read -s pass
echo
port=`cat $JAGUAR_HOME/jaguar/conf/server.conf |grep PORT|grep -v '#'|cut -d= -f2`
uo=`uname -o`
if [[ "x$uo" = "xMsys" ]] || [[ "x$uo" = "xCygwin" ]]; then
	jql="jql.exe"
else
	jql="jql.bin"
fi

echo "Creating database $db and table $table, please wait a few seconds ..."
$JAGUAR_HOME/jaguar/bin/$jql -u admin -p $pass -h :$port -d test -e "createdb $db" 
sleep 3
cat $cmd
$JAGUAR_HOME/jaguar/bin/$jql -u admin -p $pass -h :$port -d $db -f $cmd -q

cd $pd
/bin/rm -rf $dirn
