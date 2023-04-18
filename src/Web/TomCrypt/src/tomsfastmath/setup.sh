#!/bin/sh

tzr zxf tomsfastmath-0.12.tar.gz
/bin/cp makefile.shared tomsfastmath-0.12
cd tomsfastmath-0.12
make -f makefile.shared
objs=`find src/ -name "*.o"`
/bin/cp -f $objs ../../../lib

