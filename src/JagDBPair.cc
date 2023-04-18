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
#include "JagGlobalDef.h"
#include "JagDBPair.h"
#include "JagLog.h"
#include "JagMath.h"

int JagDBPair::compareKeys( const JagDBPair &d2 ) const 
{
	if ( key.addr() == NULL || key.addr()[0] == '\0' ) {
		if ( d2.key.size()<1 || d2.key.addr() == NULL || d2.key.addr()[0] == '\0' ) {
			return 0;
		} else {
			return -1;
		}
	} else {
		if ( d2.key.addr() == NULL || d2.key.addr()[0] == '\0' ) {
			return 1;
		} else {
			if ( key.addr()[0] == '*' && d2.key.addr()[0] == '*' ) {
				return 0;
			} else {
                //dn("p08288 compareKeys key=[%s] d2.key=[%s] memcmp=%d", key.addr(), d2.key.addr(), memcmp(key.addr(), d2.key.addr(), key.size() ) );
                //dn("p08288 compareKeys key.size=[%d] d2.key.size=[%d]", key.size(), d2.key.size() );
				return ( memcmp(key.addr(), d2.key.addr(), key.size() ) );
			}
		}
	}
}

void JagDBPair::print() const 
{
	int i;
	printf("K: ");
	for (i=0; i<key.size(); ++i ) {
		printf("[%d]%c ", i, key.c_str()[i] );
	}
	printf("\n");
	printf("V: ");
	for (i=0; i<value.size(); ++i ) {
		if ( value.c_str()[i] ) {
			printf("[%d]%c ", i, value.c_str()[i] );
		} else {
			printf("[%d]@ ", i );
		}
	}
	printf("\n");
	fflush(stdout);
}

void JagDBPair::p( bool newline) const 
{
    printkv( newline );
}

void JagDBPair::printkv( bool newline) const 
{
    printf("key:%lld=[", key.size() );
    dumpmem(key.c_str(), key.size(), false);

    if ( newline ) {
        printf("]\nval:%lld=[", value.size() );
    } else {
        printf("] val:%lld=[", value.size() );
    }
    dumpmem(value.c_str(), value.size(), false);

    if ( newline ) {
        printf("]\n");
    }

	fflush(stdout);
}

void JagDBPair::toBuffer(char *buffer) const 
{
	memcpy(buffer, key.c_str(), key.size() );
	memcpy(buffer + key.size(), value.c_str(), value.size() );
}

char *JagDBPair::newBuffer() const 
{
	char *buffer = jagmalloc( key.size() + value.size() +1 );
	buffer [ key.size() + value.size() ] = '\0';
	memcpy(buffer, key.c_str(), key.size() );
	memcpy(buffer + key.size(), value.c_str(), value.size() );
	return buffer;
}

void JagDBPair::printColumns() const
{
    if ( ! _tabRec ) {
        dn("p03381 printColumns _tabRec=NULL");
        return;
    }

    char *buf = newBuffer();
    printColumns( buf );
    free( buf );
}

void JagDBPair::printColumns( const char *buf ) const
{
    if ( ! _tabRec ) {
        dn("p03381 printColumns _tabRec=NULL");
        return;
    }

    const JagVector<JagColumn> &cv = *(_tabRec->columnVector);
    Jstr norm;

    printf("JagDBPair::printColumns():\n");
    for ( int i = 0; i < cv.size(); ++i ) {
        printf("i=%d iskey=%d col=[%s] ", i, cv[i].iskey, cv[i].name.s() );
        printf("offset=%d length=%d ", cv[i].offset, cv[i].length );
        printf("issubcol=%d ", cv[i].issubcol );
        printf("datadump: ");
        dumpmem( buf + cv[i].offset, cv[i].length, false );

        printf(" datavalue:");
        JagMath::fromBase254Len(norm, buf + cv[i].offset, cv[i].length );
        printf("%s", norm.s() );

        printf("\n");
    }

}
