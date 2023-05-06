all: myserv mycli

CPP=g++ -g -O3

INC_LEVELDB=git_clone_leveldb/leveldb/include
INC_SOCKCPP=git_clone_sockpp/sockpp/include
INC_LIBKETAMA=git_clone_ketama/ketama/libketama


BASICFLAGS=-I. \
          -I$(INC_LEVELDB) \
          -I$(INC_SOCKCPP) \
          -I$(INC_LIBKETAMA) \
          -D_REENTRANT -std=c++11 \
          -Wno-register -Wunused-variable \
          -Wno-unused-result \
          -DWITH_SANITIZE_ADDRESS=ON

CCFLAGS_STATIC=$(BASICFLAGS)  -static-libgcc -static-libstdc++ -static
CCFLAGS=$(CCFLAGS_STATIC)


build/%.o:%.cc
	$(CPP) $(CCFLAGS) -c -o $@ $<


LIBS=git_clone_leveldb/leveldb/build/libleveldb.a \
    git_clone_ketama/ketama/libketama/libketama.a \
    git_clone_sockpp/sockpp/build/libsockpp.a

myserv:  build/myserv.o
	$(CPP) $(CCFLAGS) -o myserv build/myserv.o $(LIBS)


mycli:  build/mycli.o
	$(CPP) $(CCFLAGS) -o mycli build/mycli.o $(LIBS)

clean:
	\rm -f buid/*.o myserv mycli

