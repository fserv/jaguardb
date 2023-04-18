#!/bin/bash

pd=`pwd -P`

if [[ ! -d "gmp-6.2.1" ]]; then
	 wget https://ftp.gnu.org/gnu/gmp/gmp-6.2.1.tar.xz
	 tar -xf gmp-6.2.1.tar.xz
fi


install_dir="$pd/gmp-6.2.1_install"
mkdir -p $install_dir

cd gmp-6.2.1
./configure --prefix=$install_dir --disable-shared

make
make install

