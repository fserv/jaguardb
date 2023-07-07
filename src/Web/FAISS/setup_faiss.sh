#!/bin/sh


### prerequisites

echo "cd to oneMKL and setup oneMKL "
echo "cd to cmake to update cmake"
echo "cd to openblas to install OpenBlas"


wget https://github.com/facebookresearch/faiss/archive/refs/tags/v1.7.4.tar.gz
tar zxf v1.7.4.tar.gz

cd faiss-1.7.4
mkdir -p install build

faiss_dir=`pwd -P`
install_dir=$faiss_dir/install


cd build

cmake ..  \
     -DFAISS_ENABLE_GPU=OFF \
     -DBLA_VENDOR=Intel10_64_dyn \
     -DMKL_LIBRARIES=/opt/intel/oneapi/mkl/latest/lib/intel64/libmkl_intel_lp64.a \
     -DFAISS_ENABLE_PYTHON=OFF \
     -DCMAKE_BUILD_TYPE=Release \
     -DBUILD_SHARED_LIBS=OFF \
     -DBUILD_TESTING=OFF  \
     -DCMAKE_POLICY_DEFAULT_CMP0135=NEW \
     -DCMAKE_INSTALL_PREFIX=$install_dir
     
  make -j10 faiss
  make install

  # ls -l faiss/libfaiss.a


