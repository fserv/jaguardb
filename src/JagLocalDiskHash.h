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
#ifndef _jaglocal_disk_hash_h_
#define _jaglocal_disk_hash_h_

#include <JagDBPair.h>
#include <JagSingleBuffReader.h>
#include <JagSingleBuffWriter.h>
#include <JagSchemaRecord.h>


class JagLocalDiskHash
{
	public:

		JagLocalDiskHash( const Jstr &filepath, int keylength=16, int vallength=16, int arrlength=32 );
		// JagLocalDiskHash( const Jstr &filepath, JagSchemaRecord *record, int arrlength=32 );
		~JagLocalDiskHash();

		inline bool	insert( const JagDBPair &pair ) { return _insertHash( pair, 1); }
		bool get( JagDBPair &pair ); 
		bool set( const JagDBPair &pair );
		bool remove( const JagDBPair &pair ); 
		bool setforce( const JagDBPair &pair );
		inline bool exist( const JagDBPair &pair ) { jagint hc; return _exist( 1, pair, &hc ); }

		void    setConcurrent( bool flag );
		void 	drop();

		void destroy();
		jagint size() const { return _arrlen; }
		int getFD() const { return _fdHash; }
		jagint keyLength() const { return KEYLEN; }
		jagint valueLength() const { return VALLEN; }
		jagint keyValueLength() const { return KVLEN; }
		// Jstr getName() consrt { return _hashname; }
		Jstr getFilePath() const { return _filePath; }

		jagint elements() { return _elements; }
		void print();
		void printnew();
		jagint getLength() const { return _arrlen; }
		Jstr getListKeys();
		jagint removeMatchKey( const char *str, int strlen );

		
	protected:
		void    init( const Jstr &fileName, int arrlength );
		void 	reAllocDistribute();
		void 	reAllocShrink();
		// bool 	updateHash( const JagDBPair &pair );
		void 	rehashCluster( jagint hc );
		jagint 	countCells( );
		bool 	_insertAt(int fdHash, const JagDBPair& pair, jagint hloc);

		char  	*makeKeyValueBuffer( const JagDBPair &pair );
		bool  	_exist( int current, const JagDBPair &pair, jagint *hc );
		bool  	_insertHash( const JagDBPair &pair, int current );

		jagint 	hashKey( const JagDBPair &key, jagint arrlen );

    	jagint 	probeLocation( jagint hc, const int fdHash, jagint arrlen );
    	jagint 	findProbedLocation( int fdHash, jagint arrlen, const JagDBPair &search, jagint hc ) ;
    	void 	findCluster( jagint hc, jagint *start, jagint *end );
    	jagint 	prevHC ( jagint hc, jagint arrlen );
    	jagint 	nextHC( jagint hc, jagint arrlen );
		bool   	aboveq( jagint start, jagint end, jagint birthhc, jagint nullbox );

		jagint 	_arrlen;
		jagint 	_newarrlen;

		jagint 	_elements;
		Jstr 	_filePath;
		Jstr 	_newhashname;
		
		jagint KEYLEN; 
		jagint VALLEN;
		jagint KVLEN;

		char		*_NullKeyValBuf;
		int 		_fdHash;
		int			_fdHash2;
		JagSchemaRecord  *_schemaRecord;

		static const int _GEO  = 2;	 // fixed

		// RayReadWriteLock    *_lock;
		bool				_doLock;

};

#endif
