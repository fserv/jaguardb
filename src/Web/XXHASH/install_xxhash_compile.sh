#!/bin/bash

if [[ ! -d xxHash ]]; then
	git clone https://github.com/Cyan4973/xxHash.git
	cd xxHash
	make libxxhash.a
	cd ..
fi

g++ -O3  -o hashtest  xxhashmain.cc xxHash/libxxhash.a
./hashtest
