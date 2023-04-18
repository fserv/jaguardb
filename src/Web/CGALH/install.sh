#!/bin/bash

pd=`pwd -P`

PKG="CGAL-5.5.1"
TARF="${PKG}.tar.xz"

if [[ ! -d "$PKG" ]]; then
	if [[ ! -f "$TARF" ]]
	then
	   wget https://github.com/CGAL/cgal/releases/download/v5.5.1/CGAL-5.5.1.tar.xz
	fi
	tar -xf $TARF
fi


install_dir="$pd/${PKG}_install"
mkdir -p $install_dir

cd $PKG
mkdir -p build
cd build

cmake -DCMAKE_INSTALL_PREFIX=${install_dir} -DCMAKE_BUILD_TYPE=Release ..

make
make install

