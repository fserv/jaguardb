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
#include <JagGlobalDef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <abax.h>
#include <JagCrypt.h>
#include <JagStrSplit.h>
#include <JagMD5lib.h>
#include <JagNet.h>
#include <base64.h>
#include <JagMD5lib.h>
#include <JagStrSplit.h>
#include "JagFileMgr.h"
#include "JagUtil.h"

Jstr JagDecryptStr( const Jstr &privkey, const Jstr &src64 )
{
	unsigned char 	plainmsg[512];
	ecc_key  		ecckey;
	unsigned long 	outlen;
    Jstr 	pkey = abaxDecodeBase64( privkey );
    Jstr 	src = abaxDecodeBase64( src64 );

	ltc_mp = tfm_desc;
	register_hash(&sha512_desc);

	int rc = ecc_import( (unsigned char*)pkey.c_str(), pkey.size(), &ecckey );
	if ( rc != CRYPT_OK ) {
		return "";
	}

	outlen = 512;
	memset( plainmsg, 0, outlen);
	rc = ecc_decrypt_key( (unsigned char*)src.c_str(), src.size(), plainmsg, &outlen, &ecckey );
	if ( rc != CRYPT_OK ) {
		return "";
	}

	Jstr astr( (const char*)plainmsg, outlen, outlen );
	return astr;
}

ecc_key *JagMakeEccKey( ecc_key *pecckey, Jstr &pubkey, Jstr &privkey )
{
	int wprng;
	int hash;
	int rc;
	prng_state  prng;
	unsigned long outlen;
	unsigned char pub[512];
	unsigned char priv[512];

	register_prng( &sprng_desc );
	register_prng( &yarrow_desc );
	wprng = find_prng( "sprng" );
	ltc_mp = tfm_desc;
	register_hash(&sha512_desc);
	hash = find_hash ( "sha512");

	rc = rng_make_prng( 256, wprng, &prng, NULL);  
	if (rc != CRYPT_OK) {
          return NULL;
    }

	rc = ecc_make_key( &prng, wprng, 48, pecckey ); 
	if ( rc != CRYPT_OK ) {
		return NULL;
	}

	outlen = 512;
	rc = ecc_export( pub, &outlen, PK_PUBLIC, pecckey );
	if ( rc != CRYPT_OK ) {
		return NULL;
	}

	Jstr newp( (const char*)pub, outlen, outlen );
	pubkey = abaxEncodeBase64 ( newp );

	outlen = 512;
	rc = ecc_export( priv, &outlen, PK_PRIVATE, pecckey );
	if ( rc != CRYPT_OK ) {
		return NULL;
	}

	Jstr newv( (const char*)priv, outlen, outlen );
	privkey = abaxEncodeBase64 ( newv );

	return pecckey;
}

Jstr JagEncryptStr( const Jstr &pubkey, const Jstr &src )
{
	ecc_key  		ecckey;
    Jstr 	pkey = abaxDecodeBase64( pubkey );

	ltc_mp = tfm_desc;

	int rc = ecc_import( (unsigned char*)pkey.c_str(), pkey.size(), &ecckey );
	if ( rc != CRYPT_OK ) {
		return "";
	}

	return JagEncryptZFC( &ecckey, src );
}

Jstr JagEncryptZFC( ecc_key *pecckey, const Jstr &src )
{
	int rc;
	int hash;
	int wprng;
	prng_state  prng;
	unsigned char ciphermsg[512];
	unsigned long outlen;

	ltc_mp = tfm_desc;

	register_prng( &sprng_desc );
	register_prng( &yarrow_desc );
	wprng = find_prng( "sprng" );

	register_hash(&sha512_desc);
	hash = find_hash ( "sha512");

	rc = rng_make_prng( 256, wprng, &prng, NULL);  
	if (rc != CRYPT_OK) {
          return "";
    }

	outlen = 512;
    rc = ecc_encrypt_key( (unsigned char*)src.c_str(), src.size(), ciphermsg, &outlen, &prng,  wprng, hash, pecckey );
	if (rc != CRYPT_OK) {
          return "";
    }

	Jstr enc( (const char*)ciphermsg, outlen, outlen );
	return ( abaxEncodeBase64 ( enc ) );
}

