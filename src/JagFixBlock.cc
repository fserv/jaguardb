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
#include <JagFixBlock.h>

JagFixBlock::JagFixBlock(int inklen, int levels )
{
	klen = inklen;
	kvlen = klen + 1;
	_vec = new JagFixGapVector[levels];
	for ( int i = 0; i < levels; ++i ) {
		_vec[i].initWithKlen( klen );
	}

	_topLevel = 0;
	ops = 0;
	_lock = NULL;
}

JagFixBlock::~JagFixBlock()
{
	destroy();
}

void JagFixBlock::destroy( )
{
	if ( _vec ) {
		delete [] _vec;
	}
	_vec = NULL;

	if ( _lock ) {
		/**
		delete _lock;
		_lock = NULL;
		**/
		deleteJagReadWriteLock( _lock );
	}
}

JagFixString JagFixBlock::getMinKey()
{
	JagReadWriteMutex mutex( _lock, JagReadWriteMutex::READ_LOCK );
	return _minKey.key;
}

JagFixString JagFixBlock::getMaxKey()
{
	JagReadWriteMutex mutex( _lock, JagReadWriteMutex::READ_LOCK );
	return _maxKey.key;
}

void JagFixBlock::updateMinKey( const JagDBPair &inpair, bool dolock )
{
	JagReadWriteMutex mutex( _lock );
	if ( dolock ) { mutex.writeLock(); }
	
	if ( _minKey.key.size() < 1 || inpair.key < _minKey.key ) {
		_minKey.key = inpair.key;
	}
	if ( dolock ) { mutex.writeUnlock(); }
}

void JagFixBlock::updateMaxKey( const JagDBPair &inpair, bool dolock )
{
	JagReadWriteMutex mutex( _lock );
	if ( dolock ) { mutex.writeLock(); }

	if ( inpair.key > _maxKey.key ) {
		_maxKey.key = inpair.key;
	}
	if ( dolock ) { mutex.writeUnlock(); }
}

void JagFixBlock::cleanPartIndex( jagint pos, bool dolock )
{
	JagDBPair dpair, npair;
	deleteIndex( dpair, npair, pos, true, dolock );
}

void JagFixBlock::deleteIndex( const JagDBPair &dpair, const JagDBPair &npair, jagint pos, bool isClean, bool dolock )
{
	JagReadWriteMutex mutex( _lock );
	if ( dolock ) { mutex.writeLock(); }
	
	if ( isClean ) {
		if ( !_vec[0].cleanPartPair( pos ) ) {
			if ( dolock ) { mutex.writeUnlock(); }
			return;
		}
	} else {
		if ( !_vec[0].deleteUpdateNeeded( dpair.key.c_str(), npair.key.c_str(), pos ) ) {
			if ( dolock ) { mutex.writeUnlock(); }
			return;
		}
	}

	jagint start, usepos; bool noUpdate; const char *pk;
	for ( int level = 1; level <= _topLevel; ++level ) {
		usepos = pos/BLOCK;
		noUpdate = 0;
		start = pos/BLOCK*BLOCK;
		for ( jagint i = start; i < pos; ++i ) {
			if ( i >= _vec[level-1].capacity() || ! _vec[level-1].isNull(i) ) {
				noUpdate = 1; break;
			}
		}
		if ( noUpdate ) break;
		
		pk = NULL;
		for ( jagint i = pos; i < start+BLOCK; ++i ) {
			if ( i < _vec[level-1].capacity() && ! _vec[level-1].isNull(i) ) {
				pk = _vec[level-1][i];
				break;
			}
		}

		if ( pk ) {
			_vec[level].insertForce( pk, usepos );
		} else {
			char nullbuf[klen];
			memset( nullbuf, 0, klen );
			_vec[level].insertForce( nullbuf, usepos );
		}
		pos = pos/BLOCK;
	}
	if ( dolock ) { mutex.writeUnlock(); }
}

bool JagFixBlock::updateIndex( const JagDBPair &inpair, jagint loweri, bool force, bool dolock )
{
	JagReadWriteMutex mutex( _lock );
	if ( dolock ) { mutex.writeLock(); }

	bool rc = false;
	jagint i = loweri;
	int   level = 0;
	bool goup = 1;
	JagDBPair  pair(inpair.key);
	jagint  primei;

	while ( goup ) {
		i = i / BLOCK;

		while ( i >= _vec[level].capacity() ) {
			_vec[level].reAlloc();
			++ops;
		}

		if ( force ) {
			_vec[level].insertForce( pair.key.c_str(), i );
		} else {
			_vec[level].insertLess( pair.key.c_str(), i );
		}
	
		
		if ( level >0 && _vec[level].isNull(0) && ! _vec[level-1].isNull(0) ) {
			memcpy( (void*)_vec[level][0], (const void*)_vec[level-1][0], klen ); 
		}

		if ( _vec[level].last() < 1 ) {
			break;
		} 

		goup  = isPrimary(level, i, &primei );

		if ( ! goup ) {
			break;
		}

		pair.key = JagFixString( _vec[level][primei], klen, klen);

		++ level;
		if ( level > _topLevel ) {
			_topLevel = level ;
		}

		++ops;
	}

	if ( inpair.key > _maxKey.key ) {
		_maxKey.key = inpair.key;
		rc = true;
	}

	if ( _minKey.key.size() < 1 || inpair.key < _minKey.key ) {
		_minKey.key = inpair.key;
		rc = true;
	}

	if ( dolock ) { mutex.writeUnlock(); }
	return rc;

}

bool JagFixBlock::isPrimary( int level, jagint i, jagint *primei )
{
	if ( (i % BLOCK) == 0 ) {
		*primei = i;
		return true;
	}
	
	jagint j = (i/BLOCK) * BLOCK;
	jagint k = j + BLOCK;
	while ( _vec[level].isNull(j) ) {
		++j;
		if ( j == k ) {
			break;
		}
	}

	if ( j == i || j == k ) {
		*primei = i;
		return true;
	}

	*primei = j;
	return false;
}

void JagFixBlock::updateCounter( jagint loweri, int val, bool isSet, bool dolock )
{
	JagReadWriteMutex mutex( _lock );
	if ( dolock ) { mutex.writeLock(); }
	jagint i = loweri / BLOCK;
	_vec[0].setValue( val, isSet, i );
	if ( dolock ) { mutex.writeUnlock(); }
}

void JagFixBlock::setNull( const JagDBPair &pair, jagint loweri )
{
	jagint i = loweri;
	int   level = 0;

	while ( 1 ) {
		i = i / BLOCK;
		_vec[level].setNull( pair.key.c_str(), i );
		if ( _vec[level].last() < 1 ) break;
		++ level;
	}
}

jagint JagFixBlock::findRealLast()
{
	return _vec[0].last();
}

bool JagFixBlock::findFirstLast( const JagDBPair &pair, jagint *retfirst, jagint *retlast )
{
	JagReadWriteMutex mutex( _lock, JagReadWriteMutex::READ_LOCK );

	jagint first, last, index;
	int level;
	int realtop;

	if ( _topLevel == 0 &&  _vec[0].size() < 1 ) {
		*retfirst = 0;
		return false;
	}

	if ( _vec[_topLevel].size() < 2 ) {
		realtop = _topLevel -1;
		if ( realtop < 0 ) realtop = 0;
	} else {
		realtop = _topLevel;
	}

	first = 0;
	last = _vec[realtop].last(); 
	if ( last < 0 ) {
		*retfirst = 0;
		return false;
	}

	for ( level = realtop; level >=0; --level )
	{
		if ( last > _vec[level].capacity()-1 ) {
			last =  _vec[level].capacity()-1; 
		}

		if ( first > _vec[level].last() ) {
			first = (_vec[level].last() / BLOCK) * BLOCK;
			last = first + BLOCK - 1;
		}

		if ( last > _vec[level].last() ) {
			last = _vec[level].last();
		}

		binSearchPred( pair, &index, _vec[level].array(), _vec[level].capacity(), first, last );
		if ( index < 0 ) {
			index = first;
		}

		first = index * BLOCK;
		last = first + BLOCK -1;
	}

	*retfirst = first;  
	*retlast = last;
	return true;
}

void JagFixBlock::flushBottomLevel( const Jstr &outFPath, jagint elements, jagint arrlen, jagint minindex, jagint maxindex )
{
	if ( _topLevel == 0 &&  _vec[0].size() < 1 ) {
		return;
	}

	int fd = jagopen( outFPath.c_str(), O_CREAT|O_RDWR|JAG_NOATIME, S_IRWXU);
	if ( fd < 0 ) {
		return;
	}

	jagint pos = 0;
	int minlen, maxlen;
	minlen = _minKey.key.size();
	maxlen = _maxKey.key.size();

	char *buf = (char*)jagmalloc(1+JAG_BID_FILE_HEADER_BYTES+minlen+maxlen+1);
	memset( buf, 0, 1+JAG_BID_FILE_HEADER_BYTES+minlen+maxlen+1 );

	buf[0] = '1';
	sprintf(buf+1, "%016lld%016lld%016lld%016lld", elements, _vec[0].last()+1, minindex, maxindex );
	memcpy(buf+1+JAG_BID_FILE_HEADER_BYTES, _minKey.key.c_str(), minlen );
	memcpy(buf+1+JAG_BID_FILE_HEADER_BYTES+minlen, _maxKey.key.c_str(), maxlen );

	raysafepwrite( fd, buf, 1+JAG_BID_FILE_HEADER_BYTES+minlen+maxlen, pos ); 
	pos += 1+JAG_BID_FILE_HEADER_BYTES+minlen+maxlen;
	char *nullbuf = (char*)jagmalloc(klen+2);
	memset( nullbuf, 0, klen+1 );

	const char *pk;
	for ( jagint i = 0; i <= _vec[0].last(); ++i ) {
		if ( _vec[0].isNull( i ) ) {
			raysafepwrite( fd, nullbuf, klen+1, pos ); 
		} else {
			/**
			const JagDBPair &pair = arr[i];
			raysafepwrite( fd, pair.key.c_str(), pair.key.size(), pos );
			raysafepwrite( fd, pair.value.c_str(), 1, pos+klen );
			**/
			pk = _vec[0][i];
			raysafepwrite( fd, pk, klen, pos );
			raysafepwrite( fd, pk+klen, 1, pos+klen );
		}
		pos += klen+1;
	}

	buf[0] = '0';
	raysafepwrite( fd, buf, 1, 0 );
	jagclose( fd );
	if ( buf ) free ( buf );
	if ( nullbuf ) free ( nullbuf );
}

bool JagFixBlock::findLimitStart( jagint &startlen, jagint limitstart, jagint &soffset ) 
{
	bool rc = _vec[0].findLimitStart( startlen, limitstart, soffset ); 
	if ( rc ) {
		soffset = soffset * BLOCK;
	}
 	return rc;
}


bool JagFixBlock::setNull() 
{
	for ( int level = _topLevel; level >=0; --level ) {
		 _vec[level].setNull();
	}
	_topLevel = 0;
	return true;
}

void JagFixBlock::print()
{
	printf("JagFixBlock Index:\n");
	for ( int level = _topLevel; level >=0; --level )
	{
		printf("Level: %d\n", level );
		_vec[level].print();
		printf("\n"); 
	}
}

bool JagFixBlock::binSearchPred( const JagDBPair &desired, jagint *index, const char *arr, 
				    jagint arrlen, jagint first, jagint last )
{
    register jagint mid; 
	register int  rc1;
	bool found = 0;
	int  cmp;

	*index = -1;

	while ( arr[last*kvlen] == NBT && last > first ) --last; 
	while ( arr[first*kvlen] == NBT && first < last ) ++first;
	mid = (first + last) / 2;
	while ( arr[mid*kvlen] == NBT && mid > first ) --mid;

	if ( memcmp( desired.key.c_str(), arr+first*kvlen, klen ) < 0 ) {
		*index = first-1;
		return false;
	}

    while( first <= last ) {
		if ( memcmp( arr+last*kvlen, desired.key.c_str(), klen ) <0 ) {
			*index = last;
			break;
		}

		cmp = memcmp( desired.key.c_str(), arr+mid*kvlen, klen ); 
		if ( 0 == cmp ) { 
			rc1 = 0; 
			found = 1;
			*index = mid;
			break;
		}
		else if (  cmp < 0 ) { rc1 = -1; } 
		else { rc1 = 1; }

		if (  rc1 < 0 ) {
			last = mid - 1;
			while ( last >=0 && arr[last*kvlen] == NBT ) --last; 

		} else {
			if ( last == mid +1 ) {
				if ( memcmp( desired.key.c_str(), arr+last*kvlen, klen ) < 0 ) {
					*index = mid;
					break;
				}
			}

			first = mid + 1;
			while ( first < arrlen && arr[first*kvlen] == NBT ) ++first;
			if ( first == arrlen ) continue;

			if ( memcmp( arr+first*kvlen, desired.key.c_str(), klen ) > 0 ) {
				*index = mid;
				break;
			}
		}
		
    	mid = (first + last) / 2;
		while ( mid >= first &&  arr[mid*kvlen] == NBT ) --mid;

    }    

	return found;
}
