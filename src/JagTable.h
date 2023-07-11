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
#ifndef _jag_table_h_
#define _jag_table_h_

#include <stdio.h>
#include <string.h>
#include <atomic>
#include <JagDiskArrayServer.h>

#include <JagSchemaRecord.h>
#include <JagTableSchema.h>
#include <JagIndexSchema.h>
#include <JagDBPair.h>
#include <JagUtil.h>
#include <JagDBServer.h>
#include <JagColumn.h>
#include <JagVector.h>
#include <JagParseExpr.h>
#include <JagTableUtil.h>
#include <JagMergeReader.h>
#include <JagMergeBackReader.h>
#include <JagMemDiskSortArray.h>
#include <JagDiskArrayFamily.h>

class JagDataAggregate;
class JagIndexString;
class JagIndex;

struct JagPolyPass
{
	double xmin, ymin, zmin, xmax, ymax, zmax;
	int tzdiff, srvtmdiff;
	int getxmin, getymin, getzmin, getxmax, getymax, getzmax;
	int getid, getcol, getm, getn, geti, getx, gety, getz; 
	int  col, m, n, i;
	bool is3D;
	Jstr dbtab, colname, lsuuid;
};

/***
struct JagMinMaxXYZ
{
    JagMinMaxXYZ()
    {
        xmin = xmax= ymin= ymax= zmin= zmax = -1.0;
    }
    double xmin, xmax, ymin, ymax, zmin, zmax;
};
***/

class JagTable
{

  public:
  	JagTable( int replicType, const JagDBServer *servobj, const Jstr &dbname, const Jstr &tableName, 
				const JagSchemaRecord &record, bool buildInitIndex=true );
  	~JagTable();
	const JagVector<Jstr> &getIndexes() const { return _indexlist; }
	int getNumIndexes() const { return _indexlist.size(); }
	int getnumCols() const { return _numCols; }
	int getnumKeys() const { return _numKeys; }	
	Jstr getdbName() const { return _dbname; }
	Jstr getTableName() const { return _tableName; }
	JagHashStrInt *getTableMap() { return _tablemap; }
	JagSchemaAttribute *getSchemaAttributes() { return _schAttr; }
	const 	JagSchemaRecord *getRecord() { return &_tableRecord; }
	bool 	getPair( JagDBPair &pair );
	void 	getlimitStart( jagint &startlen, jagint limitstart, jagint& soffset, jagint &foffset ); 

	int 	insert( const JagRequest &req, JagParseParam *parseParam, Jstr &errmsg );
	int 	finsert( const JagRequest &req, JagParseParam *parseParam, Jstr &errmsg );

	int 	parsePair( int tzdiff, JagParseParam *parseParam, JagVector<JagDBPair> &pairVec, Jstr &errmsg ) const;
	int 	insertPair( JagDBPair &pair, bool doIndexLock ); 
	Jstr 	drop( Jstr &errmsg, bool isTruncate=false );
	int 	renameIndexColumn ( const JagParseParam *parseParam, Jstr &errmsg );
	int 	setIndexColumn ( const JagParseParam *parseParam, Jstr &errmsg );
	void 	dropFromIndexList( const Jstr &indexName );
	void 	buildInitIndexlist();
	void 	setGetFileAttributes( const Jstr &hdir, JagParseParam *parseParam, const char *buffers[] );
	int 	rollupPair( const JagRequest &req, JagDBPair &inpair, const JagVector<ValueAttribute> &rollupVec );
	void 	doRollUp( const JagDBPair &inspair, const char *dbBuf, char *newbuf );
	bool 	convertTimeToWindow( int timediff, const Jstr &twindow, char *kbuf, const Jstr &colName );
	jagint  segmentTime( int tzdiff, jagint tval, const Jstr &twindow ); // seconds

	jagint update( const JagRequest &req, const JagParseParam *parseParam, bool upsert, Jstr &errmsg );
	jagint remove( const JagRequest &req, JagParseParam *parseParam, Jstr &errmsg );	
	jagint getCount( const char *cmd, const JagRequest &req, JagParseParam *parseParam, Jstr &errmsg );
	jagint getElements( const char *cmd, const JagRequest &req, JagParseParam *parseParam, Jstr &errmsg );
	jagint select( JagDataAggregate *&jda, const char *cmd, const JagRequest &req, JagParseParam *parseParam, 
				   Jstr &errmsg, bool nowherecnt=true, bool isInsertSelect=false );
	
	bool   chkkey( const Jstr &keystr );
	static void *parallelSelectStatic( void * ptr );
	static int buildDiskArrayForGroupBy( JagMergeReaderBase *ntr, const JagHashStrInt *maps[], const JagSchemaAttribute *attrs[], 
										 const JagRequest *req, const char *buffers[], 
										JagParseParam *parseParam, JagMemDiskSortArray *gmdarr, char *gbvbuf );
	static void groupByFinalCalculation( char *gbvbuf, bool nowherecnt, jagint finalsendlen, std::atomic<jagint> &cnt, jagint actlimit, 
										const Jstr &writeName, JagParseParam *parseParam, 
										JagDataAggregate *jda, JagMemDiskSortArray *gmdarr, const JagSchemaRecord *nrec );
	static void nonAggregateFinalbuf( JagMergeReaderBase *ntr, const JagHashStrInt *maps[], const JagSchemaAttribute *attrs[], 
										const JagRequest *req, const char *buffers[], JagParseParam *parseParam, char *finalbuf, 
										jagint finalsendlen, JagDataAggregate *jda, const Jstr &writeName, 
										std::atomic<jagint> &cnt, bool nowherecnt, 
										const JagSchemaRecord *nrec, bool oneLine );
	static void aggregateDataFormat( JagMergeReaderBase *ntr,  const JagHashStrInt *maps[], const JagSchemaAttribute *attrs[], 
									 const JagRequest *req, const char *buffers[], JagParseParam *parseParam, bool init );
	static void aggregateFinalbuf( const JagRequest *req, const Jstr &sendhdr, jagint len, JagParseParam *parseParam[], 
				char *finalbuf, jagint finalsendlen, JagDataAggregate *jda, const Jstr &writeName, 
				std::atomic<jagint> &cnt, bool nowherecnt, const JagSchemaRecord *nrec );
	static void doWriteIt( JagDataAggregate *jda, const JagParseParam *parseParam, 
				const Jstr &host, const char *buf, jagint buflen, const JagSchemaRecord *nrec=NULL );
	static void formatInsertFromSelect( const JagParseParam *parseParam, const JagSchemaAttribute *attrs, 
				const char *finalbuf, const char *buffers, jagint finalsendlen, jagint numCols,
				JaguarCPPClient *pcli, const Jstr &iscmd );

	int insertIndex( JagDBPair &pair, bool doIndexLock, bool hasFlushed );
	int formatCreateIndex( JagIndex *pindex );
	static void *parallelCreateIndexStatic( void * ptr );

	void   	flushBlockIndexToDisk();
	Jstr 	getIndexNameList();
	int 	refreshSchema();
	void 	setupSchemaMapAttr( int numCols );
	bool 	hasSpareColumn();
	void 	appendOther( JagVector<ValueAttribute> &otherVec,  int n, bool isSub=true) const;
	void 	formatMetricCols( int tzdiff, int srvtmdiff, const Jstr &dbtab, const Jstr &colname, 
						      int nmetrics, const JagVector<Jstr> &metrics, char *tablekvbuf ) const;
	bool 	hasTimeSeries( Jstr &tser ) const ;
	bool 	hasTimeSeries() const ;
	Jstr    timeSeriesRentention() const;
	bool 	hasRollupColumn() const ;
	jagint  cleanupOldRecords( time_t secs );
	void    refreshTableRecord();

	static const jagint keySchemaLen = JAG_SCHEMA_KEYLEN;
    static const jagint valSchemaLen = JAG_SCHEMA_VALLEN;
	std::atomic<bool>			_isExporting;
	Jstr 						_dbtable;
	jagint 						_KEYLEN;
	jagint 						_VALLEN;
	jagint 						_KEYVALLEN;
	int							_replicType;
	JagSchemaRecord 			_tableRecord;
	JagDiskArrayFamily			*_darrFamily;
	int	   						_numCols;
	int 						_numKeys;
	JagHashStrInt 			 	*_tablemap;
	JagSchemaAttribute			*_schAttr;
	JagVector<int>				_defvallist;
	JagVector<Jstr>				_indexlist;
	const JagDBServer			*_servobj;
	JagTableSchema				*_tableschema;
	JagIndexSchema				*_indexschema;
	
  protected:
	pthread_mutex_t              _parseParamParentMutex;
	const JagCfg  				*_cfg;
	//JagServerObjectLock 		*_objectLock;
	Jstr  						_dbname;
	Jstr 						_tableName;
	int  						_objectType;
    int                         _counterOffset;
    int                         _counterLength;

	void 	init( bool buildInitIndex );
	void 	destroy();
	int 	refreshIndexList();
	int 	_removeIndexRecords( const char *buf );
	int  	removeColFiles(const char *kvbuf );
	bool 	isFileColumn( const Jstr &colname );

	void 	formatPointsInLineString( int nmetrics, const JagLineString &line, char *tablekvbuf, const JagPolyPass &pass, 
								      JagVector<JagDBPair> &retpair, Jstr &errmg ) const;
	void 	formatPointsInVector( int nmetrics, const JagVectorString &line, char *tablekvbuf, const JagPolyPass &pass, 
								      JagVector<JagDBPair> &retpair, Jstr &errmg ) const;

	void 	getColumnIndex( const Jstr &colType, const Jstr &dbtab, const Jstr &colname, bool is3D,
                            int &getx, int &gety, int &getz, int &getxmin, int &getymin, int &getzmin,
                            int &getxmax, int &getymax, int &getzmax,
                            int &getid, int &getcol, int &getm, int &getn, int &geti ) const;

	int     findPairRollupOrInsert( JagDBPair &inspair, JagDBPair &getDBPair,
                                    char *tableoldbuf, char *tablenewbuf, int setindexnum, JagIndex *lpindex[] );

	void 	initStarPositions( JagVector<int> &pointer, int K );
	void 	fillStarsAndRollup( const JagVector<int> &pointer, JagDBPair &inspair, JagDBPair &getDBpair,
	                            char *tableoldbuf, char *tablenewbuf, int setindexnum, JagIndex *lpindex[] );
	void 	starCombinations( int N, int K, JagDBPair &inspair, JagDBPair &getDBpair,
	                  		  char *tableoldbuf, char *tablenewbuf, int setindexnum, JagIndex *lpindex[] );

	bool 	movePointerPositions( JagVector<int> &pointer );
	bool 	findNextPositions( JagVector<int> &pointer, int k );
	int 	findNextFreePosition( int startPosition );
	void 	fillStars( const JagVector<int> &pointer, JagDBPair &starDBPair );
	jagint 	cleanupOldRecordsByOrderOrScan( int colidx, time_t ttime, bool byOrder );
	bool    rollupType( const Jstr &name, const Jstr &colType, double inv, double dbCounter, 
					    const char *dbBuf, char *newbuf, double &avgVal );
    jagint  memoryBufferSize();
    jagint  getAllIndexBufferSize();

};


/********** Still hit invaid fastbin entry bug
#define fillCmdParse( objType, objPtr, i, gbvsendlen, pparam, notnull, lgmdarr, req, jda, dbobj, cnt, nm, nowherecnt, rec, memlim, minmaxbuf, bsec, KEYVALLEN, servobj, numthrds, darrFamily, lcpu )   \
	if ( JAG_TABLE == objType ) { \
		psp[i].ptab = (JagTable*)objPtr; \
		psp[i].pindex = NULL; \
	} else { \
		psp[i].ptab = NULL; \
		psp[i].pindex = (JagIndex*)objPtr; \
	} \
	psp[i].pos = i; \
	psp[i].sendlen = gbvsendlen; \
	psp[i].parseParam = pparam[i]; \
	if ( notnull ) psp[i].gmdarr = lgmdarr[i]; else psp[i].gmdarr = NULL; \
	psp[i].req = req; \
	psp[i].jda = jda; \
	psp[i].writeName = dbobj; \
	psp[i].cnt = &cnt; \
	psp[i].nrec = &rec; \
	psp[i].actlimit = nm; \
	psp[i].nowherecnt = nowherecnt; \
	psp[i].memlimit = memlim; \
	psp[i].minbuf = minmaxbuf[0].minbuf; \
	psp[i].maxbuf = minmaxbuf[0].maxbuf; \
	psp[i].starttime = bsec; \
	psp[i].kvlen = KEYVALLEN; \
	if ( lcpu ) { \
		psp[i].spos = i; \
		psp[i].epos = i; \
	} else { \
		psp[i].spos = i*servobj->_numCPUs; \
		psp[i].epos = psp[i].spos+servobj->_numCPUs-1; \
		if ( i == numthrds-1 ) psp[i].epos = darrFamily->_darrlistlen-1; \
	} 

***************/

#endif
