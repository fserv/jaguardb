#!/bin/bash

######################################################################################
#
#  This script downloads and install boost 1.81.0
#
#  Usage: ./install_boost_1_81_0.sh
#
######################################################################################

date
echo "Download source files ..."

if [[ ! -f "boost_1_81_0.tar.gz" ]]; then
	wget https://boostorg.jfrog.io/artifactory/main/release/1.81.0/source/boost_1_81_0.tar.gz
	echo "Unpack source files ..."
	tar zxf boost_1_81_0.tar.gz
fi

pd=`pwd -P`
install_dir="$pd/boost_1_81_0_install"

echo "Boost will be installed in $install_dir"

cd boost_1_81_0

echo "./bootstrap.sh ... "
./bootstrap.sh --prefix=$install_dir --with-python=python3 

echo "b2 stage ..."
./b2 stage -j8 threading=multi link=static

echo "b2 install ..."
./b2 install threading=multi link=static

echo "Done"
echo
echo "Boost header files are in $install_dir/include"
echo "Boost library files are in $install_dir/lib"
date
echo
