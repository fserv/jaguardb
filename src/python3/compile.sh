#!/bin/bash

# DataJaguar, Inc Copyright
# This scripte generates jaguarpy.so for Python3

PY=/usr/local/src/pycxx-7.0.0
PYCXX=/usr/local/src/pycxx-7.0.0/CXX
PYHXX=/usr/local/src/pycxx-7.0.0/CXX/Python3
SRCTOP=/usr/local/src/pycxx-7.0.0/Src
SRC=$SRCTOP/Python3

/bin/rm -f *.obj

OPT="-g"
OPT="-O3"

PYINC=/usr/local/python-3.10.6/include/python3.10
/bin/cp -f ../libJaguarClient.so $HOME/commit/java/

for i in jaguarpy.cxx $SRC/cxxsupport.cxx $SRC/cxx_extensions.cxx $SRC/cxxextensions.c \
    $SRC/cxx_exceptions.cxx $SRCTOP/IndirectPythonInterface.cxx 
do
	ib=$(basename $i)
	b=$(echo $ib |cut -d. -f 1)
	obj="$b.obj"
	#g++ $OPT -c -fPIC -I$PY -I$PYCXX -I$PYHXX -I/usr/local/include/python3.4m -I$HOME/commit -DNDEBUG -o $obj $i
	g++ $OPT -c -fPIC -I$PY -I$PYCXX -I$PYHXX -I$PYINC -I$HOME/commit -DNDEBUG -o $obj $i
done

g++ -shared $OPT -o jaguarpy3.so -Wall -fPIC  \
  jaguarpy.obj cxxsupport.obj cxx_extensions.obj cxxextensions.obj \
  IndirectPythonInterface.obj cxx_exceptions.obj \
  -L$HOME/commit/java -lJaguarClient 

/bin/cp -f jaguarpy3.so jaguarpy.so
/bin/cp -f jaguarpy3.so $HOME/jaguar/lib/jaguarpy.so
