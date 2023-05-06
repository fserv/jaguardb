all: rmlog lib prog createKeyPair glibcversion jql
progonly: prog
test: rmlog testjdb

buildenv=${BUILD_ENV}
ifeq ($(buildenv),PROD)
	USER_DEFINE=-DDEBUG_PRINT1 -DDEBUG_STACK1 -DDEBUG_TABLE1 -DDEBUG_LATENCY1 -DDEBUG_LOCK1
else
	USER_DEFINE=-DDEBUG_PRINT -DDEBUG_STACK1 -DDEBUG_TABLE -DDEBUG_LATENCY1 -DDEBUG_LOCK1
endif

GCC=gcc
GPP=g++
CPP=$(GPP)  -pg
CPP=$(GPP)  -O3 -g
CPP=$(GPP) 
CPP=$(GPP)  -ggdb  -O0
CPP=$(GPP) -O3
CPP=$(GPP) -g


BOOST_INC=Web/Boost/boost_1_81_0_install/include
BOOST_LIB=Web/Boost/boost_1_81_0_install/lib

CGAL_INC=Web/CGALH/CGAL-5.5.1_install/include

GMP_INC=Web/GMP/gmp-6.2.1_install/include
GMP_LIB=Web/GMP/gmp-6.2.1_install/lib

MPFR_INC=Web/MPFR/mpfr-4.2.0_install/include
MPFR_LIB=Web/MPFR/mpfr-4.2.0_install/lib

GEOGRAPH_LIB=Web/GeographicLib-1.49/lib

BASICFLAGS=-I. -IWeb/GeographicLib-1.49/include  \
		  -IWeb/Tencent/rapidjson/include -IWeb/TomCrypt/include \
		  -IWeb/Snappy -IWeb/UTF -I$(BOOST_INC) \
		  -IWeb/Google/Btree/include \
		  -I$(CGAL_INC) -I$(GMP_INC) -I$(MPFR_INC) \
		  -IWeb/ConcaveHull \
	      -D_REENTRANT -std=c++14 -Wno-register -Wunused-variable \
          -Wno-unused-result \
		  -DTFM_DESC  $(USER_DEFINE)

CCFLAGS_STATIC=$(BASICFLAGS)  -static-libgcc -static-libstdc++ -static  
CCFLAGS=$(CCFLAGS_STATIC)

STDCPPLIB=/lib/gcc/x86_64-linux-gnu/11/libstdc++.a
THREADLIB=/lib/x86_64-linux-gnu/libpthread.a

STDLIBS_STATIC =  $(GEOGRAPH_LIB)/libGeographic.a  \
				  $(BOOST_LIB)/libboost_system.a \
				  $(BOOST_LIB)/libboost_thread.a \
				  $(BOOST_LIB)/libboost_system.a \
				  $(GMP_LIB)/libgmp.a \
				  $(STDCPPLIB) $(THREADLIB)

STDLIBS=$(STDLIBS_STATIC)


#### makefile.obj has $(OBJS)
include makefile.obj

EXTOBJS=Web/TomCrypt/static/lib/*.o Web/Snappy/static/*.o 
ALLCLIENTOBJS=$(CLIENTOBJS) jclist.obj
ALLEXTOBJS=$(EXTOBJS) jclist.obj

JagParseExpr.o: JagExprStack.h

build/%.o:%.cc
	$(CPP) $(CCFLAGS) -c -o $@ $<  

build/JagParseExprClient.o: JagParseExpr.cc
build/JagParseExprServer.o: JagParseExpr.cc

rmlog:
	/bin/rm -f build/JagLog.o

clean:
	/bin/rm -f *.class core.* build/*.o jaguar jql.bin ~/jaguar/lib/jagserv.o ~/jaguar/lib/jql.o

glibcversion: build/glibcversion.o
	$(CPP) $(CCFLAGS) -o glibcversion build/glibcversion.o


lib: $(ALLCLIENTOBJS) $(EXTOBJS)
	date
	@/bin/mkdir -p build
	ar cr libJaguarClient.a $(ALLCLIENTOBJS) $(EXTOBJS) 
	date


jql:  $(ALLCLIENTOBJS) $(SERVEROBJS) build/jagserv.o build/jql.o
	$(CPP) $(CCFLAGS) -o jql.bin build/jql.o $(ALLCLIENTOBJS) $(EXTOBJS) $(THREADLIB)
	/bin/cp -f jql.bin ~/jbin
	/bin/cp -f jql.bin ~/jaguar/bin/ 

prog:  $(ALLCLIENTOBJS) $(SERVEROBJS) build/jagserv.o build/jql.o
	$(CPP) $(CCFLAGS) -o jql.bin build/jql.o $(ALLCLIENTOBJS) $(EXTOBJS) $(THREADLIB)
	$(CPP) $(CCFLAGS) -o jaguar.bin build/jagserv.o $(SERVEROBJS) $(ALLEXTOBJS) $(STDLIBS) 
	/bin/cp -f jaguar.bin ~/jaguar/bin

createKeyPair: build/createKeyPair.o
	$(CPP) $(CCFLAGS) -o createKeyPair.bin  build/createKeyPair.o $(SERVEROBJS) $(ALLEXTOBJS) $(STDLIBS)

testjdb: build/testjdb.o $(OBJS) $(SERVEROBJS) $(ALLEXTOBJS) 
	$(CPP) $(CCFLAGS) -o testjdb build/testjdb.o $(SERVEROBJS) $(ALLEXTOBJS) $(STDLIBS) 

