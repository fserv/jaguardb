#!/bin/bash

pd=`pwd -P`

if [[ ! -d "mpfr-4.2.0" ]]; then
	 wget https://www.mpfr.org/mpfr-current/mpfr-4.2.0.tar.xz
	 tar -xf mpfr-4.2.0.tar.xz
fi


install_dir="$pd/mpfr-4.2.0_install"
mkdir -p $install_dir

cd mpfr-4.2.0
./configure --prefix=$install_dir --disable-shared --with-gmp=$pd/../GMP/gmp-6.2.1_install

make
make install
