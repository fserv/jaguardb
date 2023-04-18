#!/bin/bash

##############################################################################################################
##
##  Server Uninstallation Script
##
##  ./uninstall_jaguar_database_on_all_hosts.sh
##
##############################################################################################################


if [[ ! -f "$HOME/.jaguarhome" ]]; then
	echo "Jaguar has not been installed, quit"
	exit 1
fi
jaguarhome=`cat $HOME/.jaguarhome`


JAGUAR_HOME=$jaguarhome
if [[ -e "$JAGUAR_HOME" ]]; then
    echo "OK, uninstall jaguar files from $JAGUAR_HOME ..."
else
    echo "JAGUAR_HOME $JAGUAR_HOME does not exist, exit"
    exit 1
fi

if [[ "x$JAGUAR_HOME" = "x$HOME" ]]; then
	echo "Wrong $JAGUAR_HOME exit"
	exit 1
fi

hostfile="$JAGUAR_HOME/conf/cluster.conf"
if [[ -f "$hostfile" ]]; then
	/bin/true
else
    echo "$hostfile is not found, exit"
    exit 1
fi

allhosts=`grep -v '#' $hostfile | grep -v '@'| awk '{print $1}' `

((deleteall=0))
echo -n "Do you want to delete database data too? (y|n) "
read ans
if [[ "x$ans" = "xy" ]]; then
	echo -n "Are you sure you want to delete the data? (y|n) "
	read ans2
	if [[ "x$ans2" = "xy" ]]; then
		((deleteall=1))
		echo "OK. Data will be deleted too ..."
	else
		((deleteall=0))
		echo "OK. Data will be kept ..."
	fi
else
	((deleteall=0))
	echo "OK. Data will be kept ..."
fi


if [[ -f "$JAGUAR_HOME/bin/jaguarstop_on_all_hosts.sh" ]]; then
    echo "Stop Jaguar on all hosts ..."
    echo "$JAGUAR_HOME/bin/jaguarstop_on_all_hosts.sh"
    $JAGUAR_HOME/bin/jaguarstop_on_all_hosts.sh
fi


for h in $allhosts
do
	if ((deleteall==0)); then
		cmd="/bin/rm -rf $JAGUAR_HOME/include $JAGUAR_HOME/lib $JAGUAR_HOME/bin $JAGUAR_HOME/doc $JAGUAR_HOME/tmp"
	else
		cmd="/bin/rm -rf $JAGUAR_HOME"
	fi
    echo ssh $h $cmd
    ssh $h "$cmd"
done

echo "Jaguar database has been uninstalled on all hosts"

