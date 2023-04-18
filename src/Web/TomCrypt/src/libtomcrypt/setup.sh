#!/bin/sh

tar zxf libtomcrypt-1.17.tar.gz
/bin/cp -f makefile.shared libtomcrypt-1.17
cd libtomcrypt-1.17
CFLAGS="-DTFM_DESC -DUSE_TFM" make -f makefile.shared
objs=`find . -name "*.o"`
/bin/cp -f $objs ../../../lib
