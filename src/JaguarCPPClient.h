/*
 * Copyright JaguarDB www.jaguardb.com
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
#ifndef _jaguar_cpp_client_h_
#define _jaguar_cpp_client_h_

#include <atomic>
#include <vector>
#include <string>
#include "JagNet.h"

#include <abax.h>
#include <jaghashtable.h>
#include <JagKeyValProp.h>
#include <JagTableOrIndexAttrs.h>
#include <JagHashStrStr.h>
#include <spsc_queue.h>

class JagCfg;
class JagRow;
class CliPass;
class JagRecord;
class JagDBPair;
class JagBlockLock;
class HostMinMaxKey;
class JagParseParam;
class JagIndexString;
class JagDataAggregate;
class JagReadWriteLock;
class JagSchemaAttribute;
class JagReplicateBackup;
class ValueAttribute;
class JagParser;

template <class Pair> class JagVector;
template <class K, class V> class JagHashMap;
template <class Pair> class JagArray;

class JagUUID;
class JagSchemaRecord;
class JagBuffReader;
class JagBuffBackReader;
class JagMemDiskSortArray;
class JagLineFile;
class JagDBMap;

struct JagNameIndex
{
	int i;
	Jstr name;
	JagNameIndex();
	JagNameIndex( const JagNameIndex &o);
	JagNameIndex& operator=( const JagNameIndex& o );
	bool operator==(const JagNameIndex& o );
	bool operator<=(const JagNameIndex& o );
	bool operator<(const JagNameIndex& o );
	bool operator>(const JagNameIndex& o );
	bool operator>=(const JagNameIndex& o );
};

class JaguarCPPClient 
{
  public:
    //////////// Business API calls, such as from JDBC or C++, Java client
  	JaguarCPPClient( );
	~JaguarCPPClient();
	
	int connect( const char *ipaddress, unsigned short port, const char *username, const char *passwd,
		const char *dbname, const char *connectOpt, jaguint clientFlag, const char *token=NULL);
	int execute( const char *query );
    int query( const char *query, bool reply=true ); 
    int apiquery( const char *query ); 

    int reply( bool headerOnly=false, bool unlock=true );
    int replyAll( bool headerOnly, bool unlock );

    int queryDirect( int qmode, int runmode, const char *query, int querylen, bool reply=true, bool batchReply=false, 
					 bool dirConn=false, bool forceConnection=false ); 

	int freeResult() { return freeRow(); } // alias of freeRow
	void close();

    int getInt(  const char *name, int *value );  
    int getLong(  const char *name, jagint *value );
    int getFloat(  const char *name, float *value );
    int getDouble(  const char *name, double *value );
	
	// helper methods
	int 	printRow();
	bool 	printAll();
	char   *getAll();
	char   *getAllByName( const char *name );
	const char   *getAllByIndex( int i ); // 1---->len
	char    *getRow();
	int 	hasError( );
	int 	freeRow( int type=0 );
	const 	char *error();
	const 	char *status();
	int   	errorCode();
    const 	char *getMessage();
    const 	char *message();
	const 	char *jsonString();
	const 	char *getLastUuid();

	char 	*getValue( const char *name );
	const char 	*getNthValue( int nth );
	int  	getColumnCount();

	char 	*getCatalogName( int col );
	char 	*getColumnClassName( int col );	
	int  	getColumnDisplaySize( int col );
	char 	*getColumnLabel( int col );
	char 	*getColumnName( int col );	
	int  	getColumnType( int col );
	char 	*getColumnTypeName( int col );		
	int  	getScale( int col ); 
	char 	*getSchemaName( int col );
	char 	*getTableName( int col );
	bool 	isAutoIncrement( int col );
	bool 	isCaseSensitive( int col );
	bool 	isCurrency( int col );
	bool 	isDefinitelyWritable( int col );	
	int  	isNullable( int col );
	bool 	isReadOnly( int col );
	bool 	isSearchable( int col );
	bool 	isSigned( int col );


	///////// Below are public, called by Jaguar programs, but not public APIs
	static bool getSQLCommand( Jstr &sqlcmd, int echo, FILE *fp, int saveNewline=0 );
	static int doAggregateCalculation( const JagSchemaRecord &aggrec, const JagVector<int> &selectPartsOpcode,
						const JagVector<JagFixString> &oneSetData, char *aggbuf, int datalen, int countBegin );

	int concurrentDirectInsert( const char *querys, int qlen );
	JAGSOCK     getSocket() const;
	const char *getSession();
	const char *getDatabase();
	Jstr        getHost() const;
	jagint      getMessageLen();
	char        getMessageType();
	void        destroy();
	void        flush();
	
	// update schema and/or host list
	void clearSchemaMap();
	void updateSchemaMap( const Jstr &schstr );

	void setWalLog( bool flag = true );
	void getReplicateHostList( JagVector<Jstr> &hostlist );
	void getSchemaMapInfo( Jstr &schemaInfo );
	int oneCmdInsertPool( JagParseAttribute &jpa, JagParser &parser, JagHashMap<AbaxString, 
						  AbaxPair<AbaxString,AbaxBuffer> > &qmap, const Jstr &cmd );
	jagint flushQMap( JagHashMap<AbaxString, AbaxPair<AbaxString,AbaxBuffer> > &qmap );
	void updateDBName( const Jstr &dbname );
	void updatePassword( const Jstr &password );
	void setFullConnection( bool flag );
	void setDebug( bool flag );
    int  sendDeltaLog( const Jstr &jagHome,  const Jstr &inpath, jagint &readRows, jagint &sentRows, JagVector<Jstr> &doneFiles );

	int importLocalFileFamily( const Jstr &inpath, const char *spstr=NULL );

	jagint sendDirectToSockAll( const char *mesg, jagint len, bool nohdr=false );
	jagint recvDirectFromSockAll( char *&buf, char *hdr );
	jagint doSendDirectToSockAll( const char *mesg, jagint len, bool nohdr=false );
	jagint doRecvDirectFromSockAll( char *&buf, char *hdr );

	jagint recvRawDirectFromSockAll( char *&buf, jagint len );
	jagint doRecvRawDirectFromSockAll( char *&buf, jagint len );
	void appendJSData( const Jstr &line );
	bool _isInsert;
	Jstr  getLastUuidData();
	Jstr  getUuid();
	int   getHostCluster();
	int   getCurrentCluster();
	int   getParseInfo( const Jstr &cmd, JagParseParam &param, Jstr &errmsg );
	int   getParseInfo( const JagParseAttribute &jpa, JagParser &parser, const Jstr &cmd, JagParseParam &param, Jstr &errmsg );
	int   sendMultiServerCommand( const char *qstr, JagParseParam &pParam, Jstr &errmsg );
	void  printProto(const char *id);
	static void peekSocket( int sock, Jstr &schema, Jstr &hostStr );

	int   getInsertFilesWithoutNames(int tdiff, const JagParseParam &param, JagVector<Jstr> &fvec, 
                                     JagVector<Jstr> &dvec, Jstr &errmsg );

    int   getKeyWithoutNames( int tdiff, const JagTableOrIndexAttrs *objAttr, const JagParseParam &parseParam, 
                              JagFixString &kstr, Jstr &errmsg );

    int   pingFileHost( const Jstr &fpath, Jstr &errmsg );

    // public data members
	Jstr 		_version;	
	short 		_queryCode;
	int 		_deltaRecoverConnection;
	int 		_servCurrentCluster;
	int 		_tdiff;
	int 		_sleepTime;
	jag_hash_t _connMap;
	int		   _allSocketsBad;
	int      _datcSrcType;  
	int      _datcDestType;
	int      _isToGate;  // connecting to GATE
	int      _hostClusterNumber; // connected server's cluster number
	int      _totalClusterNumber; // total number of clusters in whole system
	Jstr 	 _allHostsString;
	jagint   _seq;
	FILE 	 *_outf;
	JAGSOCK	 _redirectSock;
	JagHashMap<AbaxString, JagTableOrIndexAttrs> *_schemaMap;
	JagLineFile   *_lineFile;
	JagBlockLock  *_schemaLock;
	Jstr    _destHost;
	unsigned short _destPort;
	Jstr 	_username;
	Jstr 	_password;
	Jstr 	_connectOpt;
	Jstr 	_dbname;
    jaguint _clientFlag;
	Jstr    _token;
	int 	_multiReplica;


  protected:
	int checkSpecialCommandValidation( const char *querys, Jstr &newquery, 
									   const JagParseParam &param, Jstr &errmsg, AbaxString &hdbobj, 
									   const JagTableOrIndexAttrs *objAttr );
	int formatReturnMessageForSpecialCommand( const JagParseParam &param, Jstr &retmsg );
	int checkCmdTableIndexExist( const JagParseParam &pParam, AbaxString &hdbobj, 
								 const JagTableOrIndexAttrs *&objAttr, Jstr &errmsg );
	int processInsertCommands( JagVector<JagDBPair> &cmdhosts, JagParseParam &pParam, Jstr &errmsg, 
							   JagVector<Jstr> *filevec=NULL );
	int processInsertCommandsWithNames( JagVector<JagDBPair> &cmdhosts, JagParseParam &pParam, Jstr &errmsg, 
							   JagVector<Jstr> *filevec=NULL );
	int processInsertCommandsWithoutNames( JagVector<JagDBPair> &cmdhosts, JagParseParam &pParam, Jstr &errmsg, 
							   JagVector<Jstr> *filevec=NULL );
	int reformOriginalAggQuery( const JagSchemaRecord &aggrec, const JagVector<int> &selectPartsOpcode, 
							const JagVector<int> &selColSetAggParts, const JagHashMap<AbaxInt, AbaxInt> &selColToselParts, 
							const char *aggbuf, int aggbuflen, char *finalbuf, JagParseParam &pParam );
	int processDataSortArray( const JagParseParam &pParam, const Jstr &selcnt, const Jstr &selhdr, 
								const JagFixString &aggstr );
	int processOrderByQuery( const JagParseParam &pParam );
	int formatSendQuery( JagParseParam &Param, JagParseParam &pParam2, const JagHashStrInt *maps[], 
		const JagSchemaAttribute *attr[], JagVector<int> &selectPartsOpcode, JagVector<int> &selColSetAggParts, 
		JagHashMap<AbaxInt, AbaxInt> &selColToselParts, int &nqHasLimit, bool &hasAggregate, 
		jagint &limitNum, int &grouplen, int numCols[], int num, Jstr &newquery, Jstr &errmsg );
	int getUsingHosts( JagVector<Jstr> &hosts, const JagFixString &kstr, int isWrite, int cluster );

	int  sendFilesToServer( const JagVector<Jstr> &files );
	int  recvFilesFromServer( const JagParseParam *pParam );
	void getSchemaFromServer( const JagParseParam *ppram, bool more =false );
	void checkSchemaUpdate();
	int  processInsertFile( int qmode, JagParseParam &newParseParam, bool noQueryButReply, Jstr &retmsg, int &setEnd );
	int  checkRegularCommands( const char *querys, int &submode );
	int  getnewClusterString( const char *querys, Jstr &newquery, Jstr &queryerrmsg );
	Jstr getServerToken();
	void setConnectionBrokenTime();
	int  checkConnectionRetry();
	int  sendMaintenanceCommand( int qmode, const char *querys );
	bool buildConnMap( const char *token, Jstr &errmsg );
	jagint loadFile( const char *loadCommand );
	int  initRow();
	void cleanUpSchemaMap( bool dolock );
	void rebuildHostsConnections( const char *msg, jagint len );
	jagint redoLog( const Jstr &fpath );
  	void _init();
	void resetLog();
	int  flushInsertBuffer();
	int  flushInsertCmd( const Jstr &cmd);
	void _printSelectedCols();
	void appendToLog( const Jstr &querys );
	void checkPoint( const Jstr &infpath );
	bool processSpool( char *querys );
	bool _getKeyOrValue( const char *key, Jstr & strValue, Jstr &type );
	int  setOneCmdProp( int pos );
	int  _parseSchema( const char *schema );
	int  findAllMetaKeyValueProperty( JagRecord &rrec );
	int  findAllNameValueProperty( JagRecord &rrec );
	int  _printRow( FILE *outf, int nth, bool retRow, Jstr &rowStr, int forExport, const char *dbobj );
    Jstr _getField( const char * rowstr, char fieldToken );
    
    // methods with do at beginning
    int doquery( const JagParseParam *pParam, int qmode, int runmode, const char *querys, int qlen, bool reply=true, bool compress=true, 
				 bool batchReply=false, bool dirConn=false, 
				 int setEnd=0, const char *msg="", bool forceConnection=false ); 
    int doreply( bool headerOnly );
    int doPrintRow( bool retRow, Jstr &rowStr );
    int doPrintAll( bool retRow, Jstr &rowStr );
    void doFlush();
    int doHasError();
    int doFreeRow( int type=0 );
	const char *doError();
    const char *doGetMessage();
	const char *doJsonString();
	jagint doGetMessageLen();
	char doGetMessageType();
    char *doGetValue( const char *name );    
	const char *doGetNthValue( int nth );
	int doGetColumnCount();	
	char *doGetCatalogName( int col );
	char *doGetColumnClassName( int col );	
	int doGetColumnDisplaySize( int col );
	char *doGetColumnLabel( int col );
	char *doGetColumnName( int col );	
	int doGetColumnType( int col );
	char *doGetColumnTypeName( int col );		
	int doGetScale( int col ); 
	char *doGetSchemaName( int col );
	char *doGetTableName( int col );
	bool doIsAutoIncrement( int col );
	bool doIsCaseSensitive( int col );
	bool doIsCurrency( int col );
	bool doIsDefinitelyWritable( int col );	
	int doIsNullable( int col );
	bool doIsReadOnly( int col );
	bool doIsSearchable( int col );
	bool doIsSigned( int col );
	char 	*_getValue( const char *name );
	const char 	*_getNthValue( int nth );
	void	clearError();
	void	printError( const char *id);
	void 	setupHostCluster();
    bool    isOrderByKeys( const JagParseParam &param, const JagTableOrIndexAttrs *objAttr );

    
	// static methods
	static void *recoverDeltaLogStatic( void *ptr );
	static void *batchInsertStatic( void * ptr );    
	static void *pingServer( void *ptr );
	static void *broadcastAllRegularStatic( void *ptr );

	static void *broadcastAllRejectFailureStaticPrepare( void *ptr );
	static void *broadcastAllRejectFailureStaticCommit( void *ptr );
	static void *broadcastAllSelectStatic( void *ptr );
	static void *broadcastAllSelectStatic2( void *ptr );
	static void *broadcastAllSelectStatic3( void *ptr );
	static void *broadcastAllSelectStatic4( void *ptr );
	static void *broadcastAllSelectStatic5( void *ptr );
	static void *broadcastAllSelectStatic6( void *ptr );
	
	// other methods
	int recvTwoBytes( char *condition );

	int getRebalanceString( Jstr &newquery, Jstr &queryerrmsg );
	int doShutDown( const char *querys );

	int checkOrderByValidation( JagParseParam &pParam, const JagSchemaAttribute *attr[], int numCols[], int num );
	int checkInsertSelectColumnValidation( const JagParseParam &pParam );
	bool  hasEnoughDiskSpace( jagint numServers, jagint totbytes, int &requiredGB, int &availableGB );

	void cleanForNewQuery();
	void formReplyDataFromTempFile( const char *str, jagint len, char type );
	void setPossibleMetaStr();
	void commit(); // flush insert buffer
	int  doDebugCheck( const char *querys, int len, JagParseParam &pParam );
	int  doRepairCheck( const char *querys, int len );
	int  doRepairObject( const char *querys, int len );
	void getHostKeyStr( const char *kbuf, const JagTableOrIndexAttrs *objAttr, JagFixString &hostKeyStr );
	Jstr getSquareCoordStr( const Jstr &shape, const JagParseParam &pParam, int pos );
	Jstr getCoordStr( const Jstr &shape, const JagParseParam &pParam, 
								int pos, bool hasX, bool hasY, bool hasZ, bool hasWidth, 
								bool hasDepth=false, bool hasHeight=false ); 
	Jstr getLineCoordStr( const Jstr &shape, const JagParseParam &pParam, 
								int pos, bool hasX1, bool hasY1, bool hasZ1, 
								         bool hasX2, bool hasY2, bool hasZ2 );
	Jstr getTriangleCoordStr( const Jstr &shape, const JagParseParam &pParam, 
								int pos, bool hasX1, bool hasY1, bool hasZ1, 
								         bool hasX2, bool hasY2, bool hasZ2,
										 bool hasX3, bool hasY3, bool hasZ3 );

	Jstr get3DPlaneCoordStr( const Jstr &shape, const JagParseParam &pParam,  int pos,
								       bool hasX,  bool hasY,  bool hasZ, 
								       bool hasW,  bool hasD,  bool hasH,
									   bool hasNX, bool hasNY, bool hasNZ );
	Jstr convertToJson(const char *buf);
	void  getNowTimeBuf( char spare4, char *timebuf );
	void getLocalNowBuf( const Jstr &colType, char *timebuf );
	void addQueryToCmdhosts( const JagParseParam &pParam, const JagFixString &hostkstr,
	                         const JagTableOrIndexAttrs *objAttr,
		                     JagVector<Jstr> &hosts, JagVector<JagDBPair> &cmdhosts, 
							 Jstr &newquery );
	int  getClusterFromUUid( const JagParseParam &ppram, const JagTableOrIndexAttrs *objAttr);
	int  reqAuth( const char *querys );
	void setHasReply( bool flag );

	static void *searchKeyOneServer( void *arg );
	bool searchKeyInAllClusters( const JagParseParam &pParam, const JagFixString &kstr );
	void queryLock( const char *note );
	void queryUnLock( const char *note, bool unlock );
	void processSelectConstData( const JagParseParam *pParam );
    int  query_can_throw( const char *query, bool reply=true ); 

    void aggregateRollups( const JagSchemaRecord &ssrec );
    void doRollUp( JagDBMap &map,  const JagSchemaRecord &ssrec, const JagDBPair &inspair,
                   int counterOffset, int counterLength, const char *dbBuf, char *newbuf );

    bool rollupType( const Jstr &name, const Jstr &colType, int offset, int length, int sig,
                     double dbCounter, double insCounter, const char *dbBuf, const char *insBuf, char *resbuf,
                     double &dbAvg, double &insAvg );

    bool getGeoStrFromPPram( const JagParseParam &pram, int insType, bool colInOther, const Jstr &type, int pos, Jstr &colData);
    Jstr getRightFilePath( const JagParseParam &param, const Jstr &hashDir, const Jstr &colData );



	// protected data members
	JagCfg      *_cfg;
	JagHashMap<AbaxString, jagint> *_hostIdxMap;
	JagHashMap<AbaxString, jagint> *_clusterIdxMap;
	spsc_queue<Jstr> *_insertBuffer;
	std::atomic<jaguint> _insertBufferSize;
	JagVector<jagint> *_selectCountBuffer;
	int         _spCommandErrorCnt;
	std::atomic<jagint>	_lastConnectionBrokenTime;
	Jstr 	    _datauuid;
	int 	    _end;
	jagint      _qMutexThreadID;
	std::atomic<bool> _threadend;
	std::atomic<bool> _hasReply;
	bool 	    _fromShell;
	int 	    _forExport;
	Jstr 	    _exportObj;
	jagint 	    _nthHost;
	jagint      _maxQueryNum;
	JagRow 	    *_row;
	pthread_t   _threadmo;
	pthread_t   _threadflush;	
	Jstr 	    _newdbname;
	Jstr 	    _queryerrmsg;
	Jstr 	    _queryStatMsg;
	JaguarCPPClient *_parentCli;
	short	    _lastOpCode;
	bool	    _lastHasGroupBy;
	bool	    _lastHasOrderBy;
	int 	    _fullConnectionArg;
	int 	    _fullConnectionOK;
	int 	    _fromServ;
	int 	    _connInit;
	int 	    _connMapDone;
	int 	    _makeSingleReconnect;
	JagDataAggregate 	*_jda;
	JagFixString 		_aggregateData;
	JagFixString 		_pointQueryString;
	Jstr 				_dataFileHeader;
	Jstr 				_dataSelectCount;
	Jstr 				_lastQuery;
	Jstr 				_randfilepath;
	Jstr 				_selectTruncateMsg;

	// for host update string and schema update string
	Jstr 	        _hostUpdateString;
	Jstr 	        _schemaUpdateString;
	JagMemDiskSortArray *_dataSortArr;
	int             _oneConnect;
	int 		    _orderByReadFrom;
	jagint 		    _orderByKEYLEN;
	jagint 		    _orderByVALLEN;
	bool 		    _orderByIsAsc;
	jagint 		    _orderByReadPos;
	jagint 		    _orderByWritePos;
	jagint 		    _orderByLimit;
	jagint 		    _orderByLimitCnt;
	jagint 		    _orderByLimitStart;
	JagArray<JagDBPair> *_orderByMemArr;
	JagSchemaRecord 	*_orderByRecord;
	JagBuffReader 		*_orderByBuffReader;
	JagBuffBackReader 	*_orderByBuffBackReader;
	JagReplicateBackup 	*_jpb;
	int 				_dtimeout;
	int 				_connRetryLimit;
	int 				_cliservSameProcess;
	int 				_lastQueryConnError;

	pthread_rwlock_t    *_qrLock;
	pthread_mutex_t     _lineFileMutex;
    int                 _numLocks;
    int                 _numUnLocks;
	
	JAGSOCK 	    _sock;
	int 	        _numCPU;
	int 	        _faultToleranceCopy;
	bool 	        _isCluster;
	bool 	        _shareSock;
	bool 	        _isparent;
	bool            _destroyed;
	bool 	        _connectOK;
	bool 	        _walLog;
	bool 	        _lastHasSemicolon;
	bool 	        _isExclusive;
	FILE 	        *_insertLog;

	Jstr 	        _lefthost;
	Jstr 	        _righthost;
	int             _tcode;
	CliPass         *_passmo;
	CliPass         *_passflush;
	int             _debug;
	Jstr           _localIP;
	unsigned short _localPort;

	JagVector<Jstr> 			*_allHosts;
	JagVector<JagVector<Jstr> > *_allHostsByCluster;
	Jstr 						_replyerrmsg;
	Jstr 						_session;
	Jstr 						_logFPath;
	Jstr 						_jsonString;
	Jstr 						_lastUuidString;
	Jstr 						_nthValue;
	JagUUID    					*_jagUUID;
	bool 					    _isSelectConst;
};


#define UPDATE_SCHEMA_INFO \
     for ( int i = 0; i < num; ++i ) { \
        if ( parseParam.objectVec[i+beginnum].indexName.length() > 0 ) { \
            dbobjs[i] = parseParam.objectVec[i+beginnum].dbName + "." + parseParam.objectVec[i+beginnum].indexName; \
        } else  { \
            dbobjs[i] = parseParam.objectVec[i+beginnum].dbName + "." + parseParam.objectVec[i+beginnum].tableName; \
        } \
        objAttr = _schemaMap->getValue( AbaxString(dbobjs[i]) ); \
        if ( ! objAttr ) { continue; } \
        keylen[i] = objAttr->keylen; \
        numKeys[i] = objAttr->numKeys; \
        numCols[i] = objAttr->numCols; \
        records[i] = objAttr->schemaRecord; \
        schStrings[i] = objAttr->schemaString; \
        maps[i] = &(objAttr->schmap); \
        attrs[i] = objAttr->schAttr; \
        minmax[i].setbuflen( keylen[i] ); \
    }

#endif

