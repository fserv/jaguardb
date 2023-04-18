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
#include <JagMD5lib.h>
#include <JagFamilyKeyChecker.h>
#include <JagMath.h>

JagFamilyKeyChecker::JagFamilyKeyChecker( const Jstr &fpathname, int klen, int vlen )
{
	_KLEN = klen;
	_VLEN = vlen; // jdb file k/v len
	_pathName = fpathname;

    if ( klen <= JAG_KEYCHECKER_KLEN ) {
        _UKLEN = klen;
        _useHash = 0;
    } else {
        _UKLEN = JAG_KEYCHECKER_KLEN;
        _useHash = 1;
    }
}

JagFamilyKeyChecker::~JagFamilyKeyChecker()
{
	//delete _lock;
}


// key can be any length
// buf:  [kkk\0\0][vv]  
void JagFamilyKeyChecker::getUniqueKey( const char *buf, char *ukey ) const
{
	// for short keys (<= JAG_KEYCHECKER_KLEN), use original key
	if ( !_useHash ) {
		memcpy( ukey, buf, _UKLEN );
		ukey[_UKLEN] = '\0';
		return;
	}

    unsigned long mdu = MD5ULong( buf, _KLEN );
    Jstr es;
    // JAG_B62_ULONG_SIZE == 11
    JagMath::base62FromULong(es, mdu, JAG_B62_ULONG_SIZE);
	memcpy( ukey, es.s(), JAG_B62_ULONG_SIZE);

	unsigned int hash[4];
	unsigned int seed = 42;
	MurmurHash3_x64_128( buf, _KLEN, seed, hash);
	uint64_t mhash = ((uint64_t*)hash)[0]; 

    JagMath::base62FromULong(es, mhash, JAG_KEYCHECKER_KLEN - JAG_B62_ULONG_SIZE);
	sprintf( ukey+JAG_B62_ULONG_SIZE, "%s", es.s());  // 62^5 = 916 million

	ukey[JAG_KEYCHECKER_KLEN] = '\0'; // JAG_KEYCHECKER_KLEN==16
}
