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
#ifndef _jag_index_h_
#define _jag_index_h_

#include <JagTable.h>
#include <JagTableUtil.h>
#include <JagMergeReader.h>
#include <JagBuffReader.h>

class JagDataAggregate;

class JagIndex
{
  public:
  	JagIndex( int replicType, const JagDBServer *servobj, const Jstr &wholePathName, 
				const JagSchemaRecord &tabrecord, const JagSchemaRecord &idxrecord, 
				bool buildInitIndex=true );	
  	~JagIndex();

	inline int getnumCols() { return _numCols; }
	inline int getnumKeys() { return _numKeys; }
	bool    needUpdate( const Jstr &colName ) const;
	void    getlimitStart( jagint &startlen, jagint limitstart, jagint& soffset, jagint &foffset );
	inline Jstr getdbName() { return _dbname; }
	inline Jstr getTableName() { return _tableName; }	
	inline Jstr getIndexName() { return _indexName; }
	inline const JagHashStrInt *getIndexMap() { return _indexmap; }
	inline const JagSchemaAttribute *getSchemaAttributes() { return _schAttr; }
	inline const JagSchemaRecord* getTableRecord() { return &_tableRecord; }
	inline const JagSchemaRecord* getIndexRecord() { return &_indexRecord; }

	bool    getPair( JagDBPair &pair );
	int     insertPair( JagDBPair &pair, bool tabHasFlushed );
	int     removePair( const JagDBPair &pair );
	int     updateFromTable( const char *tableoldbuf, const char *tablenewbuf );
	int     removeFromTable( const char *tablebuf );
	int     drop();	
	jagint  select( JagDataAggregate *&jda, const char *cmd, const JagRequest& req, JagParseParam *parseParam, Jstr &errmsg, 
				 bool nowherecnt=true, bool isInsertSelect=false );
	jagint  getCount( const char *cmd, const JagRequest& req, JagParseParam *parseParam, Jstr &errmsg );
	
	void 	refreshSchema();
	void   	flushBlockIndexToDisk();
	void   	copyAndInsertBufferAndClean();
	int 	insertIndexFromTable( const char *tablebuf, bool hasFlushed );
    jagint  memoryBufferSize();


    // data
	JagDiskArrayFamily *_darrFamily;
	Jstr 			_dbobj;
    /**
  	jagint 			KEYLEN;
	jagint 			VALLEN;
	jagint 			KEYVALLEN;
	jagint 			TABKEYLEN;
	jagint 			TABVALLEN;
    **/
  	jagint 			_KEYLEN;
	jagint 			_VALLEN;
	jagint 			_KEYVALLEN;
	jagint 			_TABKEYLEN;
	jagint 			_TABVALLEN;

	int 			_replicType;
	JagSchemaRecord _tableRecord;
	JagSchemaRecord _indexRecord;
	int 			_numKeys;
	int 			_numCols;
	JagHashStrInt 	*_indexmap;
	JagSchemaAttribute *_schAttr;
	const JagDBServer *_servobj;
	JagTableSchema	*_tableschema;
	JagIndexSchema	*_indexschema;
	pthread_mutex_t _parseParamParentMutex;

  protected:
	bool 	bufChangeT2I( char *indexbuf, char *tablebuf );
	bool 	bufChangeI2T( char *indexbuf, char *tablebuf );
	void 	setGetFileAttributes( const Jstr &hdir, JagParseParam *parseParam, char *buffers[] );

	int 			*_indtotabOffset;
	const JagCfg 	*_cfg;
	Jstr 			_dbname;
	Jstr 			_tableName;
	Jstr 			_indexName;

	void 			init( bool buildInitIndex );
	
};

#endif
