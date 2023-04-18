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
#ifndef _jag_fixindex_class_h_
#define _jag_fixindex_class_h_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <abax.h>
#include <JagDef.h>
#include <JagFixGapVector.h>
#include <JagMutex.h>
#include <JagUtil.h>
#include <JagDBPair.h>


///////////////////////////// array index class ////////////////////////////////
// JagDBPair has key and value from tables
// but jagFixGapVector takes only key part
class JagFixBlock
{
	public:

		JagFixBlock( int klen,  int initLevel = 15 );
		~JagFixBlock();
		void destroy();

		void setNull( const JagDBPair &pair, jagint loweri );
		void cleanPartIndex( jagint pos, bool dolock=true );
		void deleteIndex( const JagDBPair &dpair, const JagDBPair &npair, jagint pos, bool isClean, bool dolock=true );
		bool updateIndex( const JagDBPair &pair, jagint loweri, bool force=false, bool dolock=true );
		void updateCounter( jagint loweri, int val, bool isSet, bool dolock=true );
		void updateMinKey( const JagDBPair &pair,  bool dolock=true );
		void updateMaxKey( const JagDBPair &pair,  bool dolock=true );
		jagint findRealLast();
		bool findFirstLast( const JagDBPair &newpair, jagint *first, jagint *last );
		bool findLimitStart( jagint &startlen, jagint limitstart, jagint &soffset ); 
		jagint getPartElements( jagint pos ) { return _vec[0].getPartElements( pos ); }
		JagFixString getMinKey();
		JagFixString getMaxKey();
		void  flushBottomLevel( const Jstr &outPath, jagint elemts, jagint arln, jagint minindx, jagint maxindx );
		jagint getBottomCapacity() const { return _vec[0].capacity(); }

		// debug purpose
		const JagFixGapVector  *getVec() const { return _vec; }
		int   getTopLevel() const { return _topLevel; }
		bool  setNull();
		void  print();

		jaguint ops;
		static const bool  debug = 1;
		static const int  BLOCK = JAG_BLOCK_SIZE;
		pthread_rwlock_t *_lock;
		JagDBPair		_maxKey;
		JagDBPair		_minKey;

	protected:

		JagFixGapVector  *_vec;
		int			_topLevel; // current top level
		bool        isPrimary( int level, jagint i, jagint *primei );
		int         klen;
		int         kvlen;
		bool        binSearchPred( const JagDBPair &key, jagint *index, const char *arr,
		                       jagint arrlen, jagint first, jagint last );


};

#endif
