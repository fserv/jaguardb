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
#ifndef _jag_disk_array_family_h_
#define _jag_disk_array_family_h_

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <atomic>

#include <abax.h>
#include <JagUtil.h>
#include <JagDef.h>
#include <JagDBPair.h>
#include <JagVector.h>
#include <JagRequest.h>
#include <JagParseExpr.h>
#include <JagParseParam.h>
#include <JagSchemaRecord.h>
#include <JagMergeReader.h>
#include <JagMergeBackReader.h>
#include <JagFamilyKeyChecker.h>
#include <JagDBMap.h>


class JagDiskArrayFamily
{
  public:
	JagDiskArrayFamily( int objType, const JagDBServer *servobj, const Jstr &fpathname, const JagSchemaRecord *record, 
						jagint length, bool buildInitIndex  );
	~JagDiskArrayFamily();

	int     insert( const JagDBPair &pair, bool &hasFlushed );
	int     insertTable( const JagDBPair &pair, bool &hasFlushed );
	int     insertIndex( const JagDBPair &pair );
	jagint  getCount( );
	jagint  getElements();
	bool 	isFlushing();
	bool    remove( const JagDBPair &pair ); // set all darrs
	bool    exist( JagDBPair &pair ); // check from oldest darr to newest darr
	bool    get( JagDBPair &pair ); // get from oldest darr to newest darr
	bool    set( const JagDBPair &pair ); // set all darrs
	bool    getWithRange( JagDBPair &pair, jagint &index ); // set all darrs

	bool    setWithRange( const JagRequest &req, JagDBPair &pair, const char *buffers[], bool uniqueAndHasValueCol, 
						ExprElementNode *root, const JagParseParam *parseParam, int numKeys, const JagSchemaAttribute *schAttr, 
						jagint setposlist[], JagDBPair &retpair, const JagVector<JagValInt> &vec ); // set all darrs

	void    drop();
	void    flushBlockIndexToDisk();
	void    copyAndInsertBufferAndClean();
	void    cleanupInsertBuffer();
	jagint  setFamilyRead( JagMergeReader *&nts, bool userInsertBuffer, const char *minbuf=NULL, const char *maxbuf=NULL ); 

	jagint  setFamilyReadPartial( JagMergeReader *&nts, bool userInsertBuffer, const char *minbuf=NULL, const char *maxbuf=NULL, 
								jagint spos=0, jagint epos=0, jagint mmax=128 ); 

	jagint  setFamilyReadBackPartial( JagMergeBackReader *&nts, bool userInsertBuffer, const char *minbuf=NULL, const char *maxbuf=NULL, 
								  jagint spos=0, jagint epos=0, jagint mmax=128 ); 

	void    debugJDBFile( int flag, jagint limit, jagint hold, jagint instart, jagint inend );
	static  void *separateDACompressStatic( void *ptr );
	int 	buildInitKeyCheckerFromSigFile();
	jagint 	flushKeyChecker();
	jagint  addKeyCheckerFromJDB( JagDiskArrayServer *darr, int activepos );
	jagint  flushInsertBuffer( bool &hasFlushed);
	JagDiskArrayServer* flushBufferToNewFile( );
	int  	findMinCostFile( JagVector<JagMergeSeg> &vec, bool forceFlush, int &mtype );
	jagint  memoryBufferSize(); // size of _insertBufferMap
	void    removeAndReopenWalLog();
	jagint  addKeyCheckerFromInsertBuffer( int darrNum );
    jagint  bufferSize() const;

    // debugging methods
    void   debugPrintBuffer();

	
	JagDBMap    					*_insertBufferMap;
	int         					_KLEN;
	int         					_VLEN;
	jagint     						_KVLEN;
	Jstr 							_tablepath;
	Jstr 							_pathname;
	Jstr 							_sfilepath;
	Jstr 							_objname;
	Jstr 							_taboridxname;
	Jstr 							_dbname;
	const JagSchemaRecord 			*_schemaRecord;
	JagVector<JagDiskArrayServer*>  _darrlist;
	JagFamilyKeyChecker				*_keyChecker;
	JagDBServer						*_servobj;
	std::atomic<jagint> 			_insdelcnt;

	pthread_t           			_threadmo;
	std::atomic<int>    			_monitordone;
	std::atomic<int>    			_sessionactive;
	jagint  						_dupwrites;
	jagint  						_writes;
	jagint  						_reads;
	jagint  						_upserts;
	jagint  						_insdircnt;
	std::atomic<int>  				_isFlushing;
	std::atomic<bool>				_doForceFlush;
    int                             _objType;

};

#endif
