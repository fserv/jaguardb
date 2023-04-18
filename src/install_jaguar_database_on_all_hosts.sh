#!/bin/bash

##############################################################################################################
##
##  Server Installation Script  -- Install Jaguar Database on All Hosts in HOSTSFILE
##
##  ./install_jaguar_database_on_all_hosts.sh  -f <HOSTSFILE>  (Install to $HOME directory)
##  ./install_jaguar_database_on_all_hosts.sh  -f <HOSTSFILE>  [-d <INSTALL_DIR>]  (Install to INSTALL_DIR)
##
##############################################################################################################

function usage()
{
    echo "Usage: ./install_jaguar_database_on_all_hosts.sh [OPTION] -f HOSTFILE"
    echo "Install JaguarDB on all hosts specified by -f HOSTFILE"
    echo "  [-help]           Display help text and exit."
    echo "  [-d DIRECTORY]    Install Jaguar in DIRECTORY. " 
    echo "                    If not provided, Jaguar will be installed in $HOME directory"
    echo "  [-f HOSTFILE]     Install Jaguar database on all the hosts in HOSTFILE."
    echo " "
    echo "In the HOSTFILE, each line must contain a host IP address."
    echo " "
    echo "Example of HOSTFILE:"
    echo "192.168.1.101"
    echo "192.168.1.102"
    echo "192.168.1.103"
    echo "192.168.1.104"
    echo " "
    echo "All host IPs must be unique in the HOSTFILE."
}


dname=`dirname $0`
if [[ "x$dname" = "x." ]] ; then
    dname=`pwd -P`
fi

ver=`basename $dname`
tarfile="${ver}.tar.gz"
tarpathfile="${dname}.tar.gz"
jaguarhome=$HOME/jaguar

dirInstall=""
options=""
conffile=""
if [[ ! -f "$tarpathfile" ]]; then
	echo "$tarpathfile is not found, exit"
	exit 1
fi

#echo "tarfile=[$tarfile] tarpathfile=[$tarpathfile]"
## Traverse all options given for this shell script
i=0
for arg in "$@"
do
    i=`expr $i + 1`
    case $arg in
        "-f")
            i_next=`expr $i + 1`
            if [[ $i_next -le $# ]]; then
                    conffile=${!i_next}
					if [[ ! -f "$conffile" ]]; then
						echo "Error $conffile does not exist, exit"
                        usage
						exit 1
					fi
            else
				echo "Error You have not provided -f hostfile"
				echo "exit...."
                echo
				usage
               	exit 1
            fi
            ;;

        "-d")
            i_next=`expr $i + 1`
            if [ $i_next -le $# ]
            then
                jaguarhome=${!i_next}
            else
				usage
                exit 1
            fi
            ;;
        "-help")
            usage
            ;;
        "-h")
            usage
            ;;
        *)
            i_previous=`expr $i - 1`
            if [ ${!i_previous} != "-f" -a ${!i_previous} != "-d" ]
            then
            	usage
                exit 1
            fi
            ;;
    esac
done


JAGUAR_HOME=$jaguarhome
if [[ -e "$JAGUAR_HOME" ]]; then
    echo "OK, install jaguar files to $JAGUAR_HOME ..."
	echo $JAGUAR_HOME > $HOME/.jaguarhome
else
    echo "JAGUAR_HOME $JAGUAR_HOME does not exist, create it ..."
	if ! mkdir -p $JAGUAR_HOME; then
		echo "Error Unable to mkdir $JAGUAR_HOME"
        usage
		exit 1
	fi
fi

/bin/mkdir -p $JAGUAR_HOME/conf
/bin/mkdir -p $JAGUAR_HOME/data
/bin/mkdir -p $JAGUAR_HOME/bin
if [[ "x$JAGUAR_HOME" = "x$HOME" ]]; then
	echo "Wrong JAGUAR_HOME $JAGUAR_HOME exit"
	exit 1
fi

if [[ -f "$JAGUAR_HOME/conf/cluster.conf" ]]; then
    ((firstTime=0))
    if [[ "x$conffile" != "x" ]]; then
        echo "Error Jaguar databaser server has been installed, no -f HOSTFILE is allowed"
        exit 123
    fi
else
    ((firstTime=1))
    if [[ "x$conffile" = "x" ]]; then
        echo "Error Please provide -f HOSTFILE"
        echo "Exit..."
        usage
        exit 100
    fi

    echo "firstTime true cluster file =[$conffile] ..."
	/bin/cp -f $conffile $JAGUAR_HOME/conf/cluster.conf
fi
conffile="$JAGUAR_HOME/conf/cluster.conf"
nhosts=`wc -l $conffile|awk '{print $1}'`

### check if installing on Msys, and only one host server
un=`uname -o`
if [[ "x$un" = "xMsys" ]]; then
	if (( nhosts == 1 )); then
		./install.sh
		exit 0
	else
		echo "Install Jaguar on current host ..."
		./install.sh
		echo "Please install Jaguar on all other Windows hosts and make sure they all have correct conf/cluster.conf file"
		exit 1
	fi
fi

### save ssh setup flag
curd=`pwd`
cd $dname
if [[ ! -f "$HOME/.jagsetupssh" ]]; then
	echo "Set up ssh public keys on all hosts ..."
	if $dname/setupsshkeys -f $conffile; then
		echo done > $HOME/.jagsetupssh
	fi
fi
cd $curd

### unpack new tar to ~/jaguardownload 
mkdir -p $HOME/jaguardownload
/bin/cp -f $tarpathfile $HOME/jaguardownload/
curd=`pwd`
cd $HOME/jaguardownload
tar -zxf $tarfile
cd $ver
/bin/cp -f server/jaguarstop_on_all_hosts.sh $JAGUAR_HOME/bin/
/bin/cp -f server/jaguarstatus_on_all_hosts.sh $JAGUAR_HOME/bin/
/bin/cp -f server/jaguarenv $JAGUAR_HOME/bin/
/bin/cp -f server/jaguarstop $JAGUAR_HOME/bin/
/bin/cp -f server/jaguarstart $JAGUAR_HOME/bin/
/bin/cp -f server/jaguarstatus $JAGUAR_HOME/bin/
cd $curd

### copy jaguarstop to all hosts
allhosts=`grep -v '#' $conffile |grep -v '@'| awk '{print $1}' `
for h in $allhosts; do
	ssh $h "/bin/mkdir -p $JAGUAR_HOME/bin"
	scp $JAGUAR_HOME/bin/jaguarenv $h:$JAGUAR_HOME/bin/
	scp $JAGUAR_HOME/bin/jaguarstop $h:$JAGUAR_HOME/bin/
	scp $JAGUAR_HOME/bin/jaguarstatus $h:$JAGUAR_HOME/bin/
	scp $JAGUAR_HOME/bin/jaguarstart $h:$JAGUAR_HOME/bin/
done


### stop all servers first
if [[ -f "$JAGUAR_HOME/bin/jaguarstop_on_all_hosts.sh" ]]; then
	echo "First stop Jaguar on all hosts ..."
	echo "$JAGUAR_HOME/bin/jaguarstop_on_all_hosts.sh"
	$JAGUAR_HOME/bin/jaguarstop_on_all_hosts.sh
	echo "======================================================"
	sleep 5

fi

### Install Jaguar on all other hosts
echo "Install Jaguar on all other hosts ..."
randstr=`cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 32|head -1`
for h in $allhosts
do
	ssh $h "mkdir -p ~/jaguardownload"
	echo scp $tarpathfile $h:~/jaguardownload/
	scp $tarpathfile $h:~/jaguardownload/
    echo "ssh $h mkdir -p $JAGUAR_HOME; cd ~/jaguardownload; " 
    echo "ssh $h tar -zxf $tarfile; cd $ver; ./install.sh -d $JAGUAR_HOME -r $randstr"
    ssh $h "mkdir -p $JAGUAR_HOME; cd ~/jaguardownload; tar -zxf $tarfile; cd $ver; ./install.sh -d $JAGUAR_HOME -r $randstr"
    if (( firstTime==1)); then
	    echo scp $conffile $h:$JAGUAR_HOME/conf/
	    scp $conffile $h:$JAGUAR_HOME/conf/
    fi
done
echo "======================================================"


### check programs and settings
if [[ -f "$JAGUAR_HOME/bin/tools/check_binaries_on_all_hosts.sh" ]]; then
    echo "Check program binaries on all hosts ..."
    if $JAGUAR_HOME/bin/tools/check_binaries_on_all_hosts.sh; then
    	echo "Program binaries are OK"
    else
    	echo "Program binaries have problem, exit"
    	#echo "Please update jaguar.bin/jaguar.exe jql.bin/jql/exe"
    	echo "Please update jaguar.bin jql.bin"
    	exit 1
    fi
    echo "======================================================"
    
    echo "Check PORT on all hosts ..."
    $JAGUAR_HOME/bin/tools/check_config_on_all_hosts.sh PORT 
    echo "======================================================"
    
    echo "Check SERVER_TOKEN on all hosts ..."
    $JAGUAR_HOME/bin/tools/check_config_on_all_hosts.sh SERVER_TOKEN
    echo "======================================================"
    
fi


if [[ -f "$JAGUAR_HOME/bin/jaguarstart_on_all_hosts.sh" ]]; then
	echo "Finally start Jaguar on all hosts ..."
	echo "$JAGUAR_HOME/bin/jaguarstart_on_all_hosts.sh"
	$JAGUAR_HOME/bin/jaguarstart_on_all_hosts.sh
fi

