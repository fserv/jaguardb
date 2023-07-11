
pd=`pwd -P`
mkdir -p install

wget https://github.com/jemalloc/jemalloc/archive/refs/tags/5.3.0.tar.gz
tar zxf 5.3.0.tar.gz
cd jemalloc-5.3.0

./autogen.sh

./configure --prefix=$pd/install
make
make install

