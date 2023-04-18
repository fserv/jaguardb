#!/bin/bash

##############################################################################################################
##
##  Installs server and client package on a node
##  It will call the server/insall.sh and client/install.sh
##
##  ./install.sh                    (Install to $HOME directory. You should install it on each host in your cluster)
##  ./install.sh -d <INSTALL_DIR>   (Install to INSTALL_DIR. You should install it on each host in your cluster)
##
##############################################################################################################

function usage
{
    echo "Usage:  ./install.sh -d <JAGUAR_HOME> -r <RANDOM_STRING>"
    echo "Usage:  ./install.sh"
	
}

JAGUAR_HOME=$HOME/jaguar

## Traverse all options given for this shell script
i=0
for arg in "$@"
do
    i=`expr $i + 1`
    case $arg in
        "-r")
            i_next=`expr $i + 1`
            if [ $i_next -le $# ]; then
                randomstring=${!i_next}
            else
				usage
                exit 1
            fi
            ;;

        "-d")
            i_next=`expr $i + 1`
            if [ $i_next -le $# ]; then
                jaguarhome=${!i_next}
				JAGUAR_HOME=$jaguarhome
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
            if [ ${!i_previous} != "-r" -a ${!i_previous} != "-d" ]; then
            	usage
                exit 1
            fi
            ;;
    esac
done


RANDSTR=""
if [[ "x$randomstring" != "x" ]]; then
	RANDSTR="-r $randomstring"
fi
echo $JAGUAR_HOME > $HOME/.jaguarhome

dname=`dirname $0`
if [[ "x$dname" = "x." ]] ; then
	dname=`pwd -P`
fi

cd $dname/server
if [[ "x$JAGUAR_HOME" != "x$HOME" ]]; then
	if ! mkdir -p $JAGUAR_HOME; then
		hn=`hostname`
		echo "$h Unable to mkdir $JAGUAR_HOME"
		exit 1
	fi
	./install.sh -d $JAGUAR_HOME $RANDSTR
else
	./install.sh $RANDSTR
fi

cd $dname/client
./install.sh -d $JAGUAR_HOME

echo "Jaguar installation complete"

