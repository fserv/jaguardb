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
#ifndef _jag_index_class_h_
#define _jag_index_class_h_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <abax.h>
#include <JagDef.h>
#include <JagGapVector.h>
#include <JagVector.h>
#include <JagMutex.h>
#include <JagUtil.h>

///////////////////////////// array index class ////////////////////////////////
template <class Pair>
class JagBlock
{
	public:

		JagBlock( int initLevel = 15 );
		~JagBlock();
		void destroy();

		void setNull( const Pair &pair, jagint loweri );

        void deleteIndex( const Pair &dpair, const Pair &npair, jagint pos, bool dolock=false );
		bool updateIndex( const Pair &pair, jagint loweri, bool force=false, bool dolock=false );

		void updateCounter( jagint loweri, int val, bool isSet, bool dolock=false );

		void updateMinKey( const Pair &pair,  bool dolock=false );
		void updateMaxKey( const Pair &pair,  bool dolock=false );

		jagint findRealLast();
		bool findFirstLast( const Pair &newpair, jagint *first, jagint *last );
		bool findLimitStart( jagint &startlen, jagint limitstart, jagint &soffset ); 
		jagint getPartElements( jagint pos ) { return _vec[0].getPartElements( pos ); }
		void cleanPartBlockIndex( jagint pos, bool dolock=false );
		JagFixString getMinKey();
		JagFixString getMaxKey();
		void  flushBottomLevel( const Jstr &outPath, jagint elemts, jagint arln, jagint minindx, jagint maxindx );
		jagint getBottomCapacity() const { return _vec[0].capacity(); }

		// debug purpose
		const JagGapVector<Pair>  *getVec() const { return _vec; }
		int   getTopLevel() const { return _topLevel; }
		bool setNull();
		void print();

		jaguint ops;
		static const bool  debug = 1;
		static const int  BLOCK = JAG_BLOCK_SIZE;
		pthread_rwlock_t *_lock;
		Pair		_maxKey;
		Pair		_minKey;

	protected:
		JagGapVector<Pair>  *_vec;
		int			_topLevel; // current top level
		bool        isPrimary( int level, jagint i, jagint *primei );

};

template <class Pair> 
JagBlock<Pair>::JagBlock( int levels )
{
	_vec = new JagGapVector<Pair>[levels];
	_topLevel = 0;
	ops = 0;
	_lock = NULL;
}

template <class Pair> 
JagBlock<Pair>::~JagBlock()
{
	destroy();
}

template <class Pair> 
void JagBlock<Pair>::destroy( )
{
	if ( _vec ) {
		delete [] _vec;
	}
	_vec = NULL;

	if ( _lock ) {
		deleteJagReadWriteLock( _lock );
		_lock = NULL;
	}
}

template <class Pair>
JagFixString JagBlock<Pair>::getMinKey()
{
	JagReadWriteMutex mutex( _lock, JagReadWriteMutex::READ_LOCK );
	return _minKey.key;
}

template <class Pair>
JagFixString JagBlock<Pair>::getMaxKey()
{
	JagReadWriteMutex mutex( _lock, JagReadWriteMutex::READ_LOCK );
	return _maxKey.key;
}

template <class Pair> 
void JagBlock<Pair>::updateMinKey( const Pair &inpair, bool dolock )
{
	JagReadWriteMutex mutex( _lock );
	if ( dolock ) { mutex.writeLock(); }
	
	if ( _minKey.key.size() < 1 || inpair.key < _minKey.key ) {
		_minKey.key = inpair.key;
	}
	if ( dolock ) { mutex.writeUnlock(); }
}

template <class Pair> 
void JagBlock<Pair>::updateMaxKey( const Pair &inpair, bool dolock )
{
	JagReadWriteMutex mutex( _lock );
	if ( dolock ) { mutex.writeLock(); }

	if ( inpair.key > _maxKey.key ) {
		_maxKey.key = inpair.key;
	}
	if ( dolock ) { mutex.writeUnlock(); }
}

// pos is level 0 actual position, not jdb file position
template <class Pair> 
void JagBlock<Pair>::cleanPartBlockIndex( jagint pos, bool dolock )
{
	JagReadWriteMutex mutex( _lock );
	if ( dolock ) { mutex.writeLock(); }
			
	int rc = _vec[0].cleanPartPair( pos );
	if ( !rc ) {
		if ( dolock ) { mutex.writeUnlock(); }
		return;
	}
		
	jagint start;
	jagint usepos;
	bool noUpdate = false;
	Pair pair;

	// loop to check and update upper levels of block indexs if this block key is removed
	for ( int level = 1; level <= _topLevel; ++level ) {
		usepos = pos/BLOCK;
		noUpdate = false;
		start = pos/BLOCK*BLOCK;
		for ( jagint i = start; i < pos; ++i ) {
			if ( i >= _vec[level-1].capacity() || _vec[level-1][i] != Pair::NULLVALUE ) {
				noUpdate = true;
				break;
			}
		}
		if ( noUpdate ) break;
		// else, the previous pos for blocks are all empty, check to this blocks last pos ( relative pos 32 ) 
		// to make sure this new block is totally empty or has new pair keys to be set to the upper level
		pair = Pair::NULLVALUE;
		for ( jagint i = pos; i < start+BLOCK; ++i ) {
			if ( i < _vec[level-1].capacity() && _vec[level-1][i] != Pair::NULLVALUE ) {
				pair = _vec[level-1][i];
				break;
			}
		}
		_vec[level].insertForce( pair, usepos );
		pos = pos/BLOCK;
	}
	if ( dolock ) { mutex.writeUnlock(); }
}

// pos is level 0 actual position, not jdb file position
template <class Pair> 
void JagBlock<Pair>::deleteIndex( const Pair &dpair, const Pair &npair, jagint pos, bool dolock )
{
    JagReadWriteMutex mutex( _lock );
    if ( dolock ) { mutex.writeLock(); }

	// if delete pair is larger than current pair, ignore
	if ( !_vec[0].deleteUpdateNeeded( dpair, npair, pos ) ) {
		if ( dolock ) { mutex.writeUnlock(); }
		return;
	}

    // then, loop to check and update upper levels of block indexs
    jagint start, usepos; bool noUpdate;
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

        // if start-pos blocks are all empty, need to check from pos to last to update new pair keys
        // or null block for the upper level
        Pair pk;
        for ( jagint i = pos; i < start+BLOCK; ++i ) {
            if ( i < _vec[level-1].capacity() && ! _vec[level-1].isNull(i) ) {
                pk = _vec[level-1][i];
                break;
            }
        }
        
		_vec[level].insertForce( pk, usepos );
        pos = pos/BLOCK;
    }
    if ( dolock ) { mutex.writeUnlock(); }
}

template <class Pair> 
bool JagBlock<Pair>::updateIndex( const Pair &inpair, jagint loweri, bool force, bool dolock )
{
	JagReadWriteMutex mutex( _lock );
	if ( dolock ) { mutex.writeLock(); }

	bool rc = false;
	jagint i = loweri;
	int   level = 0;
	bool goup = 1;
	Pair  pair = Pair(inpair.key);
	jagint  primei;

	while ( goup ) {
		i = i / BLOCK;

		while ( i >= _vec[level].capacity() ) {
			_vec[level].reAlloc();
			++ops;
		}

		// printf("c4431 updateIndex Less()  level=%d i=%d :\n", level, i );
		if ( force ) {
			_vec[level].insertForce( pair, i );
		} else {
			_vec[level].insertLess( pair, i );
		}
	
		
		// fill the hole if exists
		if ( level >0 && _vec[level][0] == Pair::NULLVALUE && _vec[level-1][0] != Pair::NULLVALUE ) {
			_vec[level][0] = _vec[level-1][0];
		}

		if ( _vec[level].last() < 1 ) {
			// fix: no update above level
			// _vec[level+1][0] = pair;
			break;
		} 

		goup  = isPrimary(level, i, &primei );

		if ( ! goup ) {
			break;
		}
		pair = _vec[level][primei];
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

	// printf("done234\n");
	if ( dolock ) { mutex.writeUnlock(); }
	return rc;

}

// is i-th postion the start-position or the first non-zero element in level-vec?
template <class Pair> 
bool JagBlock<Pair>::isPrimary( int level, jagint i, jagint *primei )
{
	if ( (i % BLOCK) == 0 ) {
		*primei = i;
		//*primei = 0;
		return true;
	}
	
	jagint j = (i/BLOCK) * BLOCK;
	jagint k = j + BLOCK;
	while ( _vec[level][j] == Pair::NULLVALUE ) {
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

template <class Pair>
void JagBlock<Pair>::updateCounter( jagint loweri, int val, bool isSet, bool dolock )
{
	JagReadWriteMutex mutex( _lock );
	if ( dolock ) { mutex.writeLock(); }
	jagint i = loweri / BLOCK;
	_vec[0].setValue( val, isSet, i );
	if ( dolock ) { mutex.writeUnlock(); }
}

template <class Pair> 
void JagBlock<Pair>::setNull( const Pair &pair, jagint loweri )
{
	jagint i = loweri;
	int   level = 0;

	while ( 1 ) {
		i = i / BLOCK;
		_vec[level].setNull( pair, i );
		if ( _vec[level].last() < 1 ) break;
		++ level;
	}
}

template <class Pair> 
jagint JagBlock<Pair>::findRealLast()
{
	return _vec[0].last();
}

template <class Pair> 
bool JagBlock<Pair>::findFirstLast( const Pair &pair, jagint *retfirst, jagint *retlast )
{
	JagReadWriteMutex mutex( _lock, JagReadWriteMutex::READ_LOCK );

	// _topLevel
	// ..
	// 0  level
	jagint first, last, index;
	int level;
	int realtop;

	if ( _topLevel == 0 &&  _vec[0].size() < 1 ) {
		*retfirst = 0;
		return 0;
	}

	if ( _vec[_topLevel].size() < 2 ) {
		realtop = _topLevel -1;
		if ( realtop < 0 ) realtop = 0;
	} else {
		realtop = _topLevel;
	}

	first = 0;
	// last = _vec[_topLevel].last(); 
	last = _vec[realtop].last(); 
	if ( last < 0 ) {
		// printf("s333930 xxxxx here realtop=%d  _topLevel=%d\n", realtop, _topLevel );
		*retfirst = 0;
		return 0;
	}

	for ( level = realtop; level >=0; --level )
	{
		if ( last > _vec[level].capacity()-1 ) {
			last =  _vec[level].capacity()-1; 
		}

		// make sure first and last is not exceed _vec[level]._last
		if ( first > _vec[level].last() ) {
			first = _vec[level].last() / BLOCK * BLOCK;
			last = first + BLOCK - 1;
		}

		if ( last > _vec[level].last() ) {
			last = _vec[level].last();
		}

		//printf("GET INDEX ARRAY\n");
		binSearchPred( pair, &index, _vec[level].array(), _vec[level].capacity(), first, last );
		//printf("END INDEX ARRAY\n");
		if ( index < 0 ) {
			index = first;
		}

		// go down
		first = index * BLOCK;
		last = first + BLOCK -1;
	}

	*retfirst = first;  *retlast = last;
	return 1;
}

// write bottom level of block index to a file
template <class Pair> 
void JagBlock<Pair>::flushBottomLevel( const Jstr &outFPath, jagint elements, jagint arrlen, jagint minindex, jagint maxindex )
{
	if ( _topLevel == 0 &&  _vec[0].size() < 1 ) {
		return;
	}

	// JagGapVector<Pair>  *_vec;
	int fd = jagopen( outFPath.c_str(), O_CREAT|O_RDWR|JAG_NOATIME, S_IRWXU);
	if ( fd < 0 ) {
		printf("s3804 error open [%s] for write\n", outFPath.c_str() );
		return;
	}
	// d("s2203 flushBottomLevel outFPath=[%s]\n", outFPath.c_str() );

	const Pair *arr = _vec[0].array();
	// get pair's key length
	int klen = 0;
	for ( jagint i = 0; i <= _vec[0].last(); ++i ) {
		if ( _vec[0].isNull( i ) ) { continue; }
		const Pair &pair = arr[i];
		klen = pair.key.size();
		break;
	}

	if ( 0 == klen ) {
		// printf("s5812 klen is 0 [%s]\n", outFPath.c_str() );
		jagclose(fd);
		return;
	}

	// write header  elementtsarrlen first  32 bytes
	jagint pos = 0;
	int minlen, maxlen;
	minlen = _minKey.key.size();
	maxlen = _maxKey.key.size();
	// char buf[33+minlen+maxlen+1];
	char *buf = (char*)jagmalloc(1+JAG_BID_FILE_HEADER_BYTES+minlen+maxlen+1);
	memset( buf, 0, 1+JAG_BID_FILE_HEADER_BYTES+minlen+maxlen+1 );
	buf[0] = '1';
	sprintf(buf+1, "%016ld%016ld%016ld%016ld", elements, _vec[0].last()+1, minindex, maxindex );
	memcpy(buf+1+JAG_BID_FILE_HEADER_BYTES, _minKey.key.c_str(), minlen );
	memcpy(buf+1+JAG_BID_FILE_HEADER_BYTES+minlen, _maxKey.key.c_str(), maxlen );
	// printf("s4776 _maxKey=[%s] buf=[%s] minlen=%d maxlen=%d\n", _maxKey.key.c_str(), buf, minlen, maxlen );

	raysafepwrite( fd, buf, 1+JAG_BID_FILE_HEADER_BYTES+minlen+maxlen, pos ); 
	pos += 1+JAG_BID_FILE_HEADER_BYTES+minlen+maxlen;
	// char nullbuf[klen+2];
	char *nullbuf = (char*)jagmalloc(klen+2);
	memset( nullbuf, 0, klen+1 );

	for ( jagint i = 0; i <= _vec[0].last(); ++i ) {
		if ( _vec[0].isNull( i ) ) {
			raysafepwrite( fd, nullbuf, klen+1, pos ); 
			//printf("s5801 write NULL at pos=%lld len=%lld\n", pos, 1 );
		} else {
			const Pair &pair = arr[i];
			raysafepwrite( fd, pair.key.c_str(), pair.key.size(), pos );
			raysafepwrite( fd, pair.value.c_str(), 1, pos+klen );
			//printf("s5801 write [%s] at pos=%lld len=%lld\n", pair.key.c_str(), pos, pair.key.size() );
		}
		pos += klen+1;
	}
	buf[0] = '0';
	raysafepwrite( fd, buf, 1, 0 );
	// printf("s5711 done write [%s]\n", outFPath.c_str() );
	jagclose( fd );
	if ( buf ) free ( buf );
	if ( nullbuf ) free ( nullbuf );
}

template <class Pair> 
bool JagBlock<Pair>::findLimitStart( jagint &startlen, jagint limitstart, jagint &soffset ) 
{
	bool rc = _vec[0].findLimitStart( startlen, limitstart, soffset ); 
	if ( rc ) {
		soffset = soffset * BLOCK;
	}
 	return rc;
}


template <class Pair> 
bool JagBlock<Pair>::setNull() 
{
	for ( int level = _topLevel; level >=0; --level ) {
		 _vec[level].setNull();
	}
	_topLevel = 0;
	return true;
}

template <class Pair> 
void JagBlock<Pair>::print()
{
	printf("Index:\n");
	for ( int level = _topLevel; level >=0; --level )
	{
		printf("Level: %d\n", level );
		_vec[level].print();
		printf("\n"); 
	}
}

#endif
