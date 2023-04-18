/bin/rm -f *.o *.so
gcc -c -Wall -Werror -fpic -I../../.. cxxJaguar.cc
gcc -c -Wall -Werror -fpic -I../../.. cjaguar.cc
gcc -shared -o libjaguargo.so cxxJaguar.o cjaguar.o
mkdir -p $HOME/jaguar/lib
/bin/cp -f libjaguargo.so $HOME/jaguar/lib
/bin/cp -f libjaguargo.so /tmp
