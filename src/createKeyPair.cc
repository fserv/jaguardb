/*
 * Copyright JaguarDB
 *
 * This file is part of JaguarDB.
 *
 * JaguarDB is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * JaguarDB is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with JaguarDB (LICENSE.txt). If not, see <http://www.gnu.org/licenses/>.
 */
#include  <stdio.h>
#include  <stdlib.h>
#include  <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string>

#include  <abax.h>
#include  "JagStrSplit.h"
#include  "JagMD5lib.h"
#include  "JagCrypt.h"

void usage();
int  parseArgs( int argc, char *argv[], Jstr &publickeyfile, Jstr &privatekeyfile );

/****************************************************************************************
	printf("Usage: makeclicense.exe <numberofNodes> <applyid> <months> \n");
	printf("months: 0 for prod/forever\n");
*****************************************************************************************/

int main( int argc, char *argv[] )
{
	srand(time(NULL) );

	ecc_key ecckey;
	Jstr pubkey, privkey;
	ecc_key *ptrkey = JagMakeEccKey( &ecckey, pubkey, privkey );
	if ( NULL == ptrkey ) {
		printf("error make keys\n");
		exit(1);
	}

	Jstr publickeyfile = "public.key";
	Jstr privatekeyfile = "private.key";
	parseArgs( argc, argv, publickeyfile, privatekeyfile );

	FILE *fp = fopen( publickeyfile.c_str(), "w");
	if ( ! fp ) {
		printf("Error open publickey file %s to write, exit\n", publickeyfile.c_str());
		exit(1);
	}
	fprintf(fp, "%s", pubkey.c_str() );
	fclose(fp );
	printf("Public key is saved in %s\n", publickeyfile.c_str());

	fp = fopen( privatekeyfile.c_str(), "w");
	if ( ! fp ) {
		printf("Error open private key file %s to write, exit\n", privatekeyfile.c_str());
		exit(1);
	}
	fprintf(fp, "%s", privkey.c_str() );
	fclose(fp );
	printf("Private key is saved in %s\n", privatekeyfile.c_str() );
	return 0;
}

int  parseArgs( int argc, char *argv[], Jstr &publickeyfile, Jstr &privatekeyfile )
{
	int i = 0;
	for ( i = 1; i < argc; ++i )
	{
		if ( 0 == strcmp( argv[i], "-publickey" ) ) {
			if ( (i+1) <= (argc-1) ) {
				publickeyfile = argv[i+1];
			} 
		} else if ( 0 == strcmp( argv[i], "-privatekey"  )  ) {
			if ( (i+1) <= (argc-1) ) {
				privatekeyfile = argv[i+1];
			} 
		} else if ( 0==strncmp( argv[i], "-h", 2 ) ) {
			printf("%s  -publickey <OUTPUT_PUBLICKEY_FILE> -privatekey <OUTPUT_PRIVATEKEY_FILE>\n", argv[0] );
			exit(1);
		}
	}

	return 1;
}
