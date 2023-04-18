#!/bin/sh

git clone https://github.com/hanslub42/rlwrap.git

cd rlwrap

# sudo apt install autoconf
autoreconf --install

RLWRAP_HOME="$HOME/rlwrap_install"
./configure --prefix=$RLWRAP_HOME CFLAGS=-I$RLWRAP_HOME/include CPPFLAGS=-I$RLWRAP_HOME/include LDFLAGS=-L$RLWRAP_HOME/lib' -static'
make
make install

ls -l $RLWRAP_HOME/bin/rlwrap


