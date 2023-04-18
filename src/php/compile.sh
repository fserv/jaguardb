cp -f ../JaguarAPI.h .
make clean
phpize --clean
phpize

#CXXFLAGS="-g" LDFLAGS="-g"  ./configure --enable-jaguarphp 
CXXFLAGS="-O3" LDFLAGS="-O3"  ./configure --enable-jaguarphp 

make

#sudo make install


