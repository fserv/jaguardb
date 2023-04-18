#!/bin/bash

unset _JAVA_OPTIONS

pd=`pwd`
export ANT_HOME=$pd/../util/apache-ant-1.10.1
if [[ ! -d "$ANT_HOME" ]]; then
    cd $pd/../util
    tar zxf apache-ant-1.10.1-bin.tar.gz
    cd $pd
fi

$ANT_HOME/bin/ant
cp -f build/*.jar lib/

