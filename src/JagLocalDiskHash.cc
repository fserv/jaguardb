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

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <JagLocalDiskHash.h>
#include <JagSingleBuffReader.h>
#include <JagSingleBuffWriter.h>
#include <JagUtil.h>
#include <JagSortLinePoints.h>
		
JagLocalDiskHash::JagLocalDiskHash( const Jstr &filepath, int keylength, int vallength, int arrlength )
{
	KEYLEN = keylength;
	VALLEN = vallength;
	KVLEN = KEYLEN+VALLEN;
	_NullKeyValBuf = (char*)jagmalloc(KVLEN+1);
	_schemaRecord = NULL;
	init( filepath, arrlength );
}

void JagLocalDiskHash::drop( )
{
	jagunlink(_filePath.c_str());
}

void JagLocalDiskHash::init( const Jstr &filePathPrefix, int length )
{
	memset(_NullKeyValBuf, 0, KVLEN+1);
	JagDBPair t_JagDBPair = JagDBPair::NULLVALUE;
	
	_filePath = filePathPrefix + ".hdb"; 
	_newhashname = filePathPrefix + "_tmpajskeurhd.hdb"; 

	_fdHash = -1;
	int exist = 0;
	if ( 0 == jagaccess( _filePath.c_str(), R_OK|W_OK  ) ) { exist = 1; }
	_fdHash = jagopen((char *)_filePath.c_str(), O_CREAT|O_RDWR, S_IRWXU);
	if ( ! exist  ) {
		_arrlen = length;
		size_t bytesHash = _arrlen*KVLEN;
   		jagftruncate( _fdHash, bytesHash);
		_elements = 0;
	} else {
		_arrlen = countCells();
	}
}

void  JagLocalDiskHash::setConcurrent( bool flag )
{
}

void JagLocalDiskHash::destroy( )
{
	if ( _fdHash > 0 ) {
		jagclose(_fdHash);
		_fdHash = -1;
	}
}

JagLocalDiskHash::~JagLocalDiskHash( )
{
	destroy();
	free( _NullKeyValBuf );
}

void JagLocalDiskHash::reAllocDistribute()
{
	jagint i;
	size_t bytesHash2;

	char *kvbuf = (char*)jagmalloc(KVLEN+1);
	memset( kvbuf, 0, KVLEN+1 );

	_newarrlen = _GEO*_arrlen;
	bytesHash2 = _newarrlen*KVLEN;

	_fdHash2 = jagopen((char *)_newhashname.c_str(), O_CREAT|O_RDWR, S_IRWXU );
	jagftruncate(_fdHash2, bytesHash2 );

	JagSingleBuffReader navig( _fdHash, _arrlen, KEYLEN, VALLEN );
	int rc2;
	while ( navig.getNext( kvbuf, KVLEN, i ) ) {
		JagDBPair pair( kvbuf, KEYLEN, kvbuf+KEYLEN, VALLEN );
		rc2= _insertHash( pair, 0 );
	}

	jagclose(_fdHash);
	jagclose(_fdHash2);

	free( kvbuf );

	jagunlink(_filePath.c_str());
	
	jagrename(_newhashname.c_str(), _filePath.c_str());
	_fdHash = jagopen((char *)_filePath.c_str(), O_CREAT|O_RDWR, S_IRWXU );

	_arrlen = _newarrlen;

}


void JagLocalDiskHash::reAllocShrink()
{
	jagint i;
	size_t bytesHash2; 
	
	char *kvbuf = (char*)jagmalloc(KVLEN+1);
	memset( kvbuf, 0, KVLEN+1 );

	_newarrlen  = _arrlen/_GEO;
	bytesHash2 = _newarrlen*KVLEN;

	_fdHash2 = jagopen((char *)_newhashname.c_str(), O_CREAT|O_RDWR, S_IRWXU);
	jagftruncate( _fdHash2, bytesHash2 );

	JagSingleBuffReader navig( _fdHash, _arrlen, KEYLEN, VALLEN );
	while ( navig.getNext( kvbuf, KVLEN, i ) ) {
		JagDBPair pair( kvbuf, KEYLEN, kvbuf+KEYLEN, VALLEN );
		_insertHash( pair, 0 );
	}

	jagclose(_fdHash);
	jagclose(_fdHash2);

	free( kvbuf );

	jagunlink(_filePath.c_str());
	
	jagrename(_newhashname.c_str(), _filePath.c_str());
	_fdHash = jagopen((char *)_filePath.c_str(), O_CREAT|O_RDWR, S_IRWXU );

	_arrlen = _newarrlen;

}

bool JagLocalDiskHash::remove( const JagDBPair &jagDBPair )
{
	jagint  hloc;
	bool rc = _exist( 1, jagDBPair, &hloc );
	if ( ! rc ) {
		return false;
	}

	-- _elements;
	rehashCluster( hloc );

	if ( _arrlen >= 64 ) {
    	jagint loadfactor  = 100 * (jagint)_elements / _arrlen;
    	if (  loadfactor < 15 ) {
    		reAllocShrink();
    	}
	} 

	return true;
}

bool JagLocalDiskHash::_exist( int current,  const JagDBPair &search, jagint *hloc )
{
	int fdHash;
	jagint arrlen;
	if ( current ) {
		fdHash = _fdHash;
		arrlen = _arrlen;
	} else {
		fdHash = _fdHash2;
		arrlen = _newarrlen;
	}

	char *kvbuf = (char*)jagmalloc(KVLEN+1);
	memset( kvbuf, 0, KVLEN+1 );
	if ( arrlen < 1 ) {
		printf("JagLocalDiskHash::exist() arrlen<1  this=%p\n", this );
		free( kvbuf );
		return 0;
	}

    jagint hc = hashKey( search, arrlen );
	ssize_t n = raysafepread( fdHash, (char *)kvbuf, KVLEN,  hc*KVLEN );
	if ( n <= 0 ) {  
		free( kvbuf ); 
		return 0; 
	}

    if ( '\0' == *kvbuf ) {
		free( kvbuf );
    	return 0;
    }
           
	JagDBPair t_JagDBPair(kvbuf, KEYLEN );
    if ( search != t_JagDBPair  ) {
       	hc = findProbedLocation( fdHash, arrlen, search, hc );
       	if ( hc < 0 ) {
			free( kvbuf );
       		return 0;
       	}
    }
        
    *hloc = hc;
	free( kvbuf );
    return 1;
}


bool JagLocalDiskHash::get( JagDBPair &jagDBPair )
{
	jagint hloc;
	bool rc;

	rc = _exist( 1, jagDBPair, &hloc );
	if ( ! rc ) {
		return false;
	}

	char *kvbuf = (char*)jagmalloc(KVLEN+1);
	memset( kvbuf, 0, KVLEN+1 );
	ssize_t n = raysafepread( _fdHash, kvbuf, KVLEN, hloc*KVLEN );
	if ( n <= 0 ) {
		free( kvbuf );
		return 0;
	}

	jagDBPair = JagDBPair( kvbuf, KEYLEN, kvbuf+KEYLEN, VALLEN);
	free( kvbuf );
	return true;
}

bool JagLocalDiskHash::set( const JagDBPair &jagDBPair )
{
	jagint hloc;
	bool rc;

	if ( jagDBPair.value.size() < 1 ) { return false; }

	rc = _exist( 1, jagDBPair, &hloc );
	if ( ! rc ) return false;

	char *valbuf = (char*)jagmalloc(VALLEN+1);
	memset( valbuf, 0, VALLEN+1 );
	const char *pval = (const char*)jagDBPair.value.addr();
	memcpy( valbuf, pval, VALLEN );
	raysafepwrite( _fdHash, valbuf, VALLEN, (hloc*KVLEN)+KEYLEN );
	free( valbuf );
	return true;
}

bool JagLocalDiskHash::setforce( const JagDBPair &jagDBPair )
{
	if ( jagDBPair.value.size() < 1 ) { return false; }

	jagint hloc;
	bool rc;
	rc = _exist( 1, jagDBPair, &hloc );
	if ( ! rc ) {
		rc = _insertAt( _fdHash, jagDBPair, hloc );
		return rc;
	}

	char *valbuf = (char*)jagmalloc(VALLEN+1);
	memset( valbuf, 0, VALLEN+1 );
	const char *ptr = jagDBPair.value.addr();
	memcpy( valbuf, ptr, VALLEN );
	raysafepwrite( _fdHash, valbuf, VALLEN, ( hloc*KVLEN)+KEYLEN );
	free( valbuf );

	return true;
}

bool JagLocalDiskHash::_insertHash( const JagDBPair &jagDBPair, int current )
{
	const char *p = jagDBPair.key.c_str();
	if ( *p == '\0' ) {
		return 0;
	}

	jagint hloc;
	bool rc = _exist( current, jagDBPair, &hloc );
	if ( rc ) {
		return false;
	}

	jagint arrlen = _arrlen;
	int  fdHash = _fdHash;
	if ( ! current ) {
		arrlen = _newarrlen;
		fdHash = _fdHash2;
	}

	char *kvbuf = (char*)jagmalloc(KVLEN+1);
	memset( kvbuf, 0, KVLEN+1 );
	jagint hc = hashKey( jagDBPair, arrlen );
	ssize_t n = raysafepread( fdHash, (char *)kvbuf, KVLEN, hc*KVLEN );
	if ( n <= 0 ) { 
		free( kvbuf ); 
		return 0; 
	}

	if ( '\0' != *kvbuf ) {
		hc = probeLocation( hc, fdHash, arrlen );
	}
	free( kvbuf );

	char *newkvbuf = makeKeyValueBuffer( jagDBPair );
	raysafepwrite(fdHash, (char*)newkvbuf, KVLEN, hc*KVLEN );
	free( newkvbuf );

	if ( ! current ) {
		return 1;
	}

	++ _elements;

	if ( ( 10*_GEO*_elements ) >=  7*_arrlen ) {
		reAllocDistribute();
	}

	return 1;
}

jagint JagLocalDiskHash::probeLocation( jagint hc, const int fd_Hash, jagint arrlen )
{
	int cnt = 0;
	char *kvbuf = (char*)jagmalloc(KVLEN+1);
	memset( kvbuf, 0, KVLEN+1 );
	ssize_t n = 0;
    while ( 1 ) {
    	hc = nextHC( hc, arrlen );
		n = raysafepread(fd_Hash, (char *)kvbuf, KVLEN, hc*KVLEN );
		if ( n <= 0 ) { 
			jd(JAG_LOG_LOW, "s238802 error n=%d fd_Hash=%d hc=%d arrlen=%d KVLEN=%d return -1\n", 
					  n, fd_Hash, hc, arrlen, KVLEN );
			free( kvbuf ); 
			return -1; 
		}

		if ( '\0' == *kvbuf ) {
			free( kvbuf );
			return hc;
		}

		++cnt;
		if ( cnt > 100000 ) {
			printf("e5492 error probe exit\n");
			fflush(stdout);
			exit(41);
		}
   	}
	free( kvbuf );
	jd(JAG_LOG_LOW, "s238804 raysafepread error fd_Hash=%d arrlen=%d return -2\n", fd_Hash, arrlen );
   	return -2;
}
    
jagint JagLocalDiskHash::findProbedLocation( int fdHash, jagint arrlen, const JagDBPair &search, jagint hc ) 
{
	char *kvbuf = (char*)jagmalloc(KVLEN+1);
	memset( kvbuf, 0, KVLEN+1 );
	ssize_t n;

   	while ( 1 ) {
   		hc = nextHC( hc, arrlen );
		n = raysafepread( fdHash, (char *)kvbuf, KVLEN, hc*KVLEN ); 		
		if ( n <= 0 ) { free( kvbuf ); return -1; }

   		if ( *kvbuf == '\0' ) {
			free( kvbuf );
   			return -1;
   		}
    
   		if ( 0 == strncmp( kvbuf, search.key.addr(), KEYLEN ) ) { 
			free( kvbuf );
   			return hc;
   		}
   	}

	free( kvbuf );
   	return -1;
}
    
void JagLocalDiskHash::findCluster( jagint hc, jagint *start, jagint *end )
{
   	jagint i;
   	i = hc;

	char *kvbuf = (char*)jagmalloc(KVLEN+1);
	memset( kvbuf, 0, KVLEN+1 );
	ssize_t n;
   
   	while ( 1 ) {
   		i = prevHC( i, _arrlen );
		n = raysafepread( _fdHash, (char *)kvbuf, KVLEN, i*KVLEN );
		if ( n <= 0 ) {  *start = -1; free(kvbuf); return;} 
 		if ( 0 == *kvbuf ) {
    		*start = nextHC( i, _arrlen );
    		break;
    	}
    }
    
    i = hc;
    while ( 1 ) {
    	i = nextHC( i, _arrlen );
		n = raysafepread( _fdHash, (char *)kvbuf, KVLEN, i*KVLEN );
		if ( n <= 0 ) {  *start = -1; free(kvbuf); return;} 
 		if ( 0 == *kvbuf ) {
    		*end = prevHC( i, _arrlen );
    		break;
    	}

   	}

	free( kvbuf );
}
    
jagint JagLocalDiskHash::prevHC ( jagint hc, jagint arrlen )
{
   	--hc; 
   	if ( hc < 0 ) hc = arrlen-1;
   	return hc;
}
    
jagint JagLocalDiskHash::nextHC( jagint hc, jagint arrlen )
{
 	++ hc;
   	if ( hc == arrlen ) hc = 0;
   	return hc;
}

 bool JagLocalDiskHash::aboveq( jagint start, jagint end, jagint birthhc, jagint nullbox )
{
		if ( start < end ) {
			return ( birthhc <= nullbox ) ? true : false;
		} else {
			if ( 0 <= birthhc  && birthhc <= end ) {
				birthhc += _arrlen;
			}

			if ( 0 <= nullbox  && nullbox <= end ) {
				nullbox += _arrlen;
			}

			return ( birthhc <= nullbox ) ? true : false;
		}
}

void JagLocalDiskHash::rehashCluster( jagint hc )
{
		jagint start, end;
		jagint nullbox = hc;

		findCluster( hc, &start, &end );
		if ( start < 0 ) return;

		jagint  birthhc;
		bool b;
		ssize_t n;

		raysafepwrite( _fdHash, (char *)_NullKeyValBuf, KVLEN, hc*KVLEN );

		char *kvbuf = (char*)jagmalloc(KVLEN+1);
		memset( kvbuf, 0, KVLEN+1 );
		while ( 1 ) {
			hc = nextHC( hc, _arrlen );
			n = raysafepread( _fdHash, (char *)kvbuf, KVLEN, hc*KVLEN );
			if ( n <= 0 ) break;
			if ( *kvbuf == '\0' ) { break; }
			JagDBPair t_JagDBPair(kvbuf, KEYLEN );
			birthhc = hashKey( t_JagDBPair, _arrlen );
			if ( birthhc == hc ) {
				continue; 
			}

			b = aboveq( start, end, birthhc, nullbox );
			if ( b ) {
				raysafepwrite( _fdHash, (char *)kvbuf, KVLEN, nullbox*KVLEN );

				raysafepwrite( _fdHash, (char *)_NullKeyValBuf, KVLEN, hc*KVLEN );
				nullbox = hc;
			} else {
			}
		}

		free( kvbuf );
}


char* JagLocalDiskHash::makeKeyValueBuffer( const JagDBPair &jagDBPair )
{
	char *mem = (char*) jagmalloc( KVLEN );
	memset( mem, 0, KVLEN );
	memcpy( mem, jagDBPair.key.addr(), jagDBPair.key.size() );
	memcpy( mem+KEYLEN, jagDBPair.value.addr(), jagDBPair.value.size() );
	return mem;
}


jagint JagLocalDiskHash::countCells()
{
	struct stat     statbuf;
	if (  fstat( _fdHash, &statbuf) < 0 ) {
		return 0;
	}

	_arrlen = statbuf.st_size/KVLEN;
	_elements = 0;
   	char *kvbuf = (char*)jagmalloc( KVLEN+1 );
	memset( kvbuf, 0,  KVLEN + 1 );
	JagSingleBuffReader navig( _fdHash, _arrlen, KEYLEN, VALLEN );
	while ( navig.getNext( kvbuf ) ) {
		++_elements;
	}
	free( kvbuf );
	jagmalloc_trim( 0 );

	return _arrlen;
}

void JagLocalDiskHash::print()
{
	jagint i;
	char *kvbuf = (char*)jagmalloc(KVLEN+1);
	memset( kvbuf, 0, KVLEN+1 );

	JagSingleBuffReader navig( _fdHash, _arrlen, KEYLEN, VALLEN );
	int num = 0;
	printf("JagLocalDiskHash::print() _elements=%lld _fdHash:\n", _elements );
	while ( navig.getNext( kvbuf, KVLEN, i ) ) {
		JagDBPair pair( kvbuf, KEYLEN, kvbuf+KEYLEN, VALLEN );
		printf("%03d %08lld [%s] --> [%s]\n", num, i, pair.key.addr(), pair.value.addr() );
		++num;
	}

	free( kvbuf );
}

void JagLocalDiskHash::printnew()
{
	jagint i;
	char *kvbuf = (char*)jagmalloc(KVLEN+1);
	memset( kvbuf, 0, KVLEN+1 );

	JagSingleBuffReader navig( _fdHash2, _newarrlen, KEYLEN, VALLEN );
	int num = 0;
	printf("JagLocalDiskHash::printnew() _fdHash2:\n");
	while ( navig.getNext( kvbuf, KVLEN, i ) ) {
		JagDBPair pair( kvbuf, KEYLEN, kvbuf+KEYLEN, VALLEN );
		printf("%03d %08lld [%s] --> [%s]\n", num, i, pair.key.addr(), pair.value.addr() );
		++num;
	}

	free( kvbuf );
}

bool JagLocalDiskHash::_insertAt( int fdHash, const JagDBPair &jagDBPair, jagint hloc )
{
	char *newkvbuf = makeKeyValueBuffer( jagDBPair );
	raysafepwrite(fdHash, (char*)newkvbuf, KVLEN, hloc*KVLEN );
	free( newkvbuf );
	return 1;
}

Jstr JagLocalDiskHash::getListKeys()
{
    Jstr res;
	JagFixString keys[_arrlen+1];

    char *buf = (char*)jagmalloc(KVLEN+1);
    memset(buf, 0, KVLEN+1);
    JagSingleBuffReader ntr( _fdHash, _arrlen, KEYLEN, VALLEN, 0, 0, 1 );
	int num = 0;
    while ( ntr.getNext( buf ) ) {
        JagFixString key ( buf, KEYLEN );
		keys[num++] = key;
    }
    free( buf );
	inlineQuickSort<JagFixString>( keys, num );
	for ( int k =0; k < num ; ++k ) {
        res += Jstr( keys[k].c_str() ) + "\n";
	}
    return res;
}

jagint JagLocalDiskHash::removeMatchKey( const char *kstr, int klen )
{
    JagFixString fixs(kstr, klen);
    JagDBPair pair(fixs);
    Jstr keys =  getListKeys();

    JagStrSplit sp(keys, '\n', true );
    if ( sp.length() < 1 ) return 0;
    jagint cnt = 0;
    JagDBPair newpair;
    for ( jagint i = 0; i < sp.length(); ++i ) {
        if ( 0 == jagstrncasecmp( sp[i].c_str(), pair.key.c_str(), pair.key.size() ) ) {
            newpair.point( sp[i].c_str(), sp[i].size() );
            if ( remove( newpair ) ) {
                ++cnt;
            }
        }
    }

    return cnt;
}

jagint  JagLocalDiskHash::hashKey( const JagDBPair &key, jagint arrlen ) 
{ 
	return key.hashCode() % arrlen; 
}

