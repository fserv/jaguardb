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
#ifndef _jag_disk_array_base_h_
#define _jag_disk_array_base_h_

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <atomic>
#include <vector>

#include <abax.h>
#include <JagCfg.h>
#include <JagUtil.h>
#include <JagMutex.h>
#include <JagGapVector.h>
#include <JagFixBlock.h>
#include <JagArray.h>
#include <JagDBPair.h>
#include <JagFileMgr.h>
#include <JagStrSplit.h>
#include <JagParseExpr.h>
#include <JagTableUtil.h>
#include <JagBuffReader.h>
#include <JagBuffWriter.h>
#include <JagParseParam.h>
#include <JagIndexString.h>
#include <JagSchemaRecord.h>
#include <JagVector.h>
#include <JagDBServer.h>
#include <JDFS.h>
#include <JagRequest.h>
#include <JagDBMap.h>

////////////////////////////////////////// disk array class ///////////////////////////////////
class JagDiskArrayBase;
class JaguarCPPClient;
class JagBuffReader;
class JagBuffBackReader;
class JagDiskArrayFamily;
class JagCompFile;

typedef JagArray<JagDBPair>  PairArray;

class JagDiskArrayBase
{
	public:
		JagDiskArrayBase( const JagDBServer *servobj, JagDiskArrayFamily *fam, const Jstr &fpathname, 
							const JagSchemaRecord *record, int index );
		JagDiskArrayBase( const Jstr &fpathname, const JagSchemaRecord *record  ); // for client class
		virtual ~JagDiskArrayBase();
		
		const   JagSchemaRecord *getSchemaRecord() const { return _schemaRecord; }
		Jstr    getFilePathName() const { return _pathname; }
		Jstr    getFilePath() const { return _filePath; }
		Jstr    getDBName() const { return _dbname; }
		Jstr    getObjName() const { return _objname; }
		Jstr    getDBObject() const { return _dbobj; }
		JagCompFile *getCompf() const { return _jdfs->getCompf(); }
		int insert( JagDBPair &pair ) {
    		int insrc; JagDBPair retpair;
    		return insertData(pair, insrc, true, retpair );
		}
		int     insertData( JagDBPair &pair, int &insertCode, bool doFirstRedist, JagDBPair &retpair );
		jagint  size() const; // bytes
		jagint  mergeBufferToFile(const JagDBMap *pairmap, const JagVector<JagMergeSeg> &vec );
		bool    getFirstLast( const JagDBPair &pair, jagint &first, jagint &last );
		virtual void drop() {}
		virtual void buildInitIndex( bool force=false )  {}
		virtual void init( jagint length, bool buildBlockIndex ) {}
		static  Jstr jdbPath( const Jstr &jdbhome, const Jstr &db, const Jstr &tab );
		Jstr    jdbPathName( const Jstr &jdbhome, const Jstr &db, const Jstr &tab );
		void    debugJDBFile( int flag, jagint limit, jagint hold, jagint instart, jagint inend );
		jagint  getRegionElements( jagint first, jagint length );
		void    insertMergeUpdateBlockIndex( char *kvbuf, jagint ipos, jagint &lastBlock );
		jagint  flushBufferToNewFile( const JagDBMap *pairmap );
		static bool checkSetPairCondition( const JagDBServer *servobj, const JagRequest &req, const JagDBPair &pair, char *buffers[], 
									bool uniqueAndHasValueCol, ExprElementNode *root, 
									const JagParseParam *parseParam, int numKeys, const JagSchemaAttribute *schAttr, 
									jagint  KLEN, jagint VLEN,
									jagint setposlist[], JagDBPair &retpair, const JagVector<JagValInt> &vec );

		
		int         _GEO;
		jagint      _KLEN; 
		jagint      _VLEN;
		jagint      _KVLEN;
		int         _keyMode;
		bool        _doneIndex;
		jagint      _arrlen; // arrlen for local server
		std::atomic<jagint> _elements; // elements for local server		
		std::atomic<jagint> _minindex; // min index
		std::atomic<jagint> _maxindex; // max index
		JDFS 		*_jdfs;
		const JagDBServer *_servobj;
		const JagSchemaRecord *_schemaRecord;
		Jstr 	    _dirPath;
		Jstr 	    _filePath;
		Jstr 	    _pdbobj;
		Jstr 	    _dbobj;
		time_t		_lastSyncTime;
		Jstr 		_dbname;
		Jstr 		_objname;
		jagint 				_newarrlen;		
		std::atomic<int>  	_isFlushing;
		int 				_nthserv;
		int 				_numservs;
		jagint				_lastSyncOneTime;
		JagCompFile			*_compf;
		JagDiskArrayFamily  *_family; // back pointer to family
		int      			_index;
		char				*_maxKey;
		
	protected:

		// data member for protected class use only
		Jstr _pathname;
		Jstr _tmpFilePath;

		void destroy();
		void _getPair( char buffer[], int keylength, int vallength, JagDBPair &pair, bool keyonly ) const;
		int insertToRange( JagDBPair &pair, int &insertCode, bool doresizePartial=true );
		int insertToAll( JagDBPair &pair, int &insertCode );
		void cleanRegionElements( jagint first, jagint length );
		
		jagint getRealLast();
		bool findPred( const JagDBPair &pair, jagint *index, jagint first, jagint last, JagDBPair &retpair, char *diskbuf );
		static void logInfo( jagint t1, jagint t2, jagint cnt, const JagDiskArrayBase *jda );
		pthread_t  			_threadmo;
		std::atomic<int>  	_sessionactive;

		// debug params
		jagint  _fulls;
		jagint  _partials;

		jagint  _insmrgcnt;
		jagint  _insdircnt;
		jagint  _reads;
		jagint  _writes;
		jagint  _dupwrites;
		jagint  _upserts;
		int		 _isClient;
};

#endif
