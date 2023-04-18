#include <stdio.h>
#include "xxHash/xxhash.h"

int main(int argc, char *argv[])
{
	XXH64_hash_t hash1 = XXH64("1", 1, 1);
	printf("hash of 1 seed=1 %lu\n", hash1 );
	hash1 = XXH64("1", 1, 2);
	printf("hash of 1 seed=2 %lu\n", hash1 );
	hash1 = XXH64("1", 1, 22938);
	printf("hash of 1 seed=22938 %lu\n", hash1 );


	int sz = 32;
	char buf[sz+1];
	for ( int i=0; i < sz; ++i)
	{
		buf[i] = i;
	}
	buf[sz] = '\0';

	int sz2 = 64;
	char buf2[sz2+1];
	for ( int i=0; i < sz2; ++i)
	{
		buf2[i] = 2*i;
	}
	buf2[sz2] = '\0';
	buf2[23]='A';
	buf2[28]='Y';
	buf2[32]=234;

	//XXH64_hash_t hash = XXH64(buffer, size, seed);
	printf("size of XXH64_hash_t=%ld\n", sizeof(XXH64_hash_t));
	XXH64_hash_t hash = XXH64(buf, sz, 1234);
	printf("hash=%lu\n", hash );

	XXH64_hash_t hash2 = XXH64(buf2, sz2, 234);
	printf("hash2=%lu\n", hash2 );

	int sz3 = 640;
	unsigned int total = 1000000000; // a billion hashes
	char buf3[sz3];
	XXH64_hash_t hash3;
	for ( unsigned int k=0; k < total; ++k ) {
		buf3[k%sz3] = k%256;
		hash3 = XXH64(buf3, sz3, k);
	}
	printf("done %u hashes\n", total );
}
