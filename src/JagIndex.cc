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

#include <JagDBServer.h>
#include <JagIndex.h>
#include <JaguarCPPClient.h>
#include <JagHashLock.h>
#include <JagDataAggregate.h>
#include <JagDiskArrayClient.h>
#include <JagDBConnector.h>
#include <JagParser.h>

JagIndex::JagIndex( int replicType, const JagDBServer *servobj, const Jstr &wholePathName, 
					const JagSchemaRecord &trecord, 
				    const JagSchemaRecord &irecord, bool buildInitIndex ) 
  : _tableRecord(trecord), _indexRecord(irecord), _servobj( servobj )
{
	_cfg = _servobj->_cfg;

    dn("s455008 JagIndex ctor wholePathName=[%s]", wholePathName.s() );
	JagStrSplit oneSplit( wholePathName, '.' );
	_dbname = oneSplit[0];
	_tableName = oneSplit[1];
	_indexName = oneSplit[2];

	_dbobj = _dbname + "." + _indexName;
    dn("s45009 _dbobj=[%s]", _dbobj.s() );

	_darrFamily = NULL;
	_indexmap = NULL;
	_indtotabOffset = NULL;
	_schAttr = NULL;
	_tableschema = NULL;
	_indexschema = NULL;
	_numKeys = 0;
	_numCols = 0;
	_replicType = replicType;
	_KEYLEN = 0;
	_VALLEN = 0;
	_KEYVALLEN = 0;

	init( buildInitIndex );
}

JagIndex::~JagIndex ()
{
	/***
	if ( _marr ) {
		delete _marr;
	}
	_marr = NULL;
	***/

	if ( _darrFamily ) {
		delete _darrFamily;
	}
	_darrFamily = NULL;
	
	if ( _indexmap ) {
		delete _indexmap;
	}	
	_indexmap = NULL;
	
	if ( _indtotabOffset ) {
		free ( _indtotabOffset );
	}
	_indtotabOffset = NULL;
	
	if ( _schAttr ) {
		delete [] _schAttr;
	}
	_schAttr = NULL;

	pthread_mutex_destroy( &_parseParamParentMutex );
}

void JagIndex::init( bool buildInitIndex ) 
{
	if ( NULL != _darrFamily ) { 
        dn("s3560081 JagIndex::init _darrFamily != NULL  return");
        return; 
    }
	
	Jstr fpath, dbcolumn;
	Jstr dbindex;
	Jstr jagdatahome = _cfg->getJDBDataHOME( _replicType );
	fpath = jagdatahome + "/" + _dbname + "/" + _tableName + "/" + _tableName + "." + _indexName;

    /**
     Compared to table path for JagDiskArrayFamily: table path is:
     fpath = jagdatahome + "/" + _dbname + "/" + _tableName + "/" + _tableName;
    **/

    dn("s500377 JagIndex::init fpath=[%s]", fpath.s() );

	_darrFamily = new JagDiskArrayFamily( JAG_INDEX, _servobj, fpath, &_indexRecord, 0, buildInitIndex );

	_KEYLEN = _indexRecord.keyLength;
	_VALLEN = _indexRecord.valueLength;
	_KEYVALLEN = _KEYLEN + _VALLEN;
	_TABKEYLEN = _tableRecord.keyLength;
	_TABVALLEN = _tableRecord.valueLength;

    dn("s272801  JagIndex _KEYLEN=%d _VALLEN=%d  _KEYVALLEN=%d    _TABKEYLEN=%d _TABVALLEN=%d",
            _KEYLEN, _VALLEN, _KEYVALLEN, _TABKEYLEN, _TABVALLEN );

	_numCols = _indexRecord.columnVector->size();

	_schAttr = new JagSchemaAttribute[_numCols];
	_schAttr[0].record = _tableRecord;
	_indtotabOffset = (int*)calloc(_numCols, sizeof(int));
	_indexmap = newObject<JagHashStrInt>();
	
	dbindex = _dbname + "." + _indexName;

    const JagVector<JagColumn> &tcv = *(_tableRecord.columnVector);
    const JagVector<JagColumn> &icv = *(_indexRecord.columnVector);

    dn("s360034 _indexRecord.columnVector->size=_numCols=%d", _numCols );

    // look at each column in index
	for ( int i = 0; i < _numCols; ++i ) {
		dbcolumn = dbindex + "." + icv[i].name.c_str();
        dn("s520887 index column=[%s]", dbcolumn.s() );
		_indexmap->addKeyValue(dbcolumn, i);

        for ( int j = 0; j < tcv.size(); ++j ) {
			if ( icv[i].name == tcv[j].name ) {
				_indtotabOffset[i] = tcv[j].offset;
				dn("s7532 i=%d colname=[%s] _indtotabOffset[i]=%d", i, tcv[j].name.c_str(), _indtotabOffset[i] ); 
				break;
			}
		}
		_schAttr[i].dbcol = dbcolumn;
		_schAttr[i].objcol = _indexName + "." + icv[i].name.c_str();
		_schAttr[i].colname = icv[i].name.c_str();
		_schAttr[i].isKey = icv[i].iskey;
		_schAttr[i].offset = icv[i].offset;
		_schAttr[i].length = icv[i].length;
		_schAttr[i].sig = icv[i].sig;
		_schAttr[i].type = icv[i].type;
		_schAttr[i].srid = icv[i].srid;
		_schAttr[i].metrics = icv[i].metrics;

        dn("s33830 index i=%d _schAttr[i].offset=%d  _schAttr[i].length=%d", i, _schAttr[i].offset, _schAttr[i].length );

		if ( _numKeys == 0 && !_schAttr[i].isKey ) {
			_numKeys = i;
		}
	}

	if ( _numKeys == 0 ) {
		_numKeys = _numCols;
	}

	if ( 0 == _replicType ) {
		_tableschema = _servobj->_tableschema;
		_indexschema = _servobj->_indexschema;
	} else if ( 1 == _replicType ) {
		_tableschema = _servobj->_prevtableschema;
		_indexschema = _servobj->_previndexschema;
	} else if ( 2 == _replicType ) {
		_tableschema = _servobj->_nexttableschema;
		_indexschema = _servobj->_nextindexschema;
	} else {
        dn("s220238");
        abort();
    }

	pthread_mutex_init( &_parseParamParentMutex, NULL );
}


bool JagIndex::getPair( JagDBPair &pair ) 
{ 
	if ( _darrFamily->get( pair ) ) return true;
	else return false;
}

void JagIndex::refreshSchema()
{
	Jstr dbpath = _dbname + "." + _tableName;
	Jstr dbtable, dbcolumn;
	AbaxString ischemaStr;
	const JagSchemaRecord *onerecord  = _tableschema->getAttr( dbpath );
	if ( ! onerecord ) return;
	_tableRecord = *onerecord;
	
	dbpath = _dbname + "." + _tableName + "." + _indexName;
	onerecord = _indexschema->getAttr( dbpath );
	if ( ! onerecord ) return;
	_indexRecord = *onerecord;

    if ( _indexmap ) {
        delete _indexmap;
    }
    _indexmap = newObject<JagHashStrInt>();

	dbtable = _dbname + "." + _tableName;
    for ( int i = 0; i < _numCols; ++i ) {
		dbcolumn = dbtable + "." + (*(_indexRecord.columnVector))[i].name.c_str();
        _indexmap->addKeyValue(dbcolumn, i);
    }

}

bool JagIndex::bufChangeT2I( char *indexbuf, char *tablebuf ) 
{
    dn("s220080 bufChangeT2I(): index _numCols=%d", _numCols );

    dn("s222208 tablebuf=NOTNULL indexbuf=NULL  tablebuf ==> indexbuf  _numCols=%d", _numCols );
	for ( int i = 0; i < _numCols; ++i ) {
        dn("s50022402 i=%d index.colname=[%s] _schAttr[i].offset=%d  _indtotabOffset[i]=%d _schAttr[i].length=%d", 
            i, _schAttr[i].colname.s(), _schAttr[i].offset, _indtotabOffset[i], _schAttr[i].length );

        if ( _schAttr[i].length > 0 ) {
            if ( i == 0 ) {
                if ( '\0' == *(tablebuf+_indtotabOffset[i]) ) {
                    dn("tablebuf.column first byte is NULL, skip adding to index");
                    continue;
                }
            }

		    memcpy(indexbuf+_schAttr[i].offset, tablebuf+_indtotabOffset[i], _schAttr[i].length);		

            d("s322028 dumpmem %s tablebuf column: ", _schAttr[i].colname.s() );
            //dumpmem(tablebuf+_indtotabOffset[i], _schAttr[i].length);

            d("s322029 dumpmem %s indexbuf column: ", _schAttr[i].colname.s() );
            //dumpmem(indexbuf+_schAttr[i].offset, _schAttr[i].length);
        }
	}

    dn("s345029 tablebuf=[%s]", tablebuf );
    dn("s345029 indexbuf=[%s]", indexbuf );

	return 1;
}

bool JagIndex::bufChangeI2T( char *indexbuf, char *tablebuf ) 
{
    dn("s220080 bufChangeI2T():");

    dn("s222202 indexbuf=NOTNULL tablebuf=NULL  indexbuf ==> tablebuf");
	for ( int i = 0; i < _numCols; ++i ) {
		memcpy(tablebuf+_indtotabOffset[i], indexbuf+_schAttr[i].offset, _schAttr[i].length);		
	}		

	return 1;
}

int JagIndex::insertIndexFromTable( const char *tablebuf, bool tabHasFlushed )
{
	char *indexbuf = (char*)jagmalloc(_KEYVALLEN+1);
	memset(indexbuf, 0, _KEYVALLEN+1);

	int rc = bufChangeT2I( indexbuf, (char*)tablebuf );
    dn("s22220736 bufChangeT2I rc=%d", rc );

    dn("s9873010 in insertIndexFromTable(): after bufChangeT2I rc=%d indexbuf=[%s]", rc, indexbuf );
    /**
    dn("i2220 insertIndexFromTable() dump _tableName=[%s]  _indexName=[%s]  indexbuf:", _tableName.s(), _indexName.s() );
    dumpmem( indexbuf, _KEYLEN );
    dumpmem( indexbuf, _KEYVALLEN );
    **/

	if ( !rc || *indexbuf == '\0' ) {
		if ( indexbuf ) free( indexbuf );
        dn("s22120766 bufChangeT2I rc=%d or indexbuf=NULL return 0", rc );
		return 0;
	}

    dn("s98730107650 goon ...");

	dbNaturalFormatExchange( indexbuf, _numKeys, _schAttr,0,0, " " ); // natural format -> db format
    //dn("ss828282 !!!!!!!!!!!!! dumpmem in formatIndexCmdFromTable:");
    //dumpmem( indexbuf, KEYVALLEN);

	JagDBPair pair( indexbuf, _KEYLEN, indexbuf+_KEYLEN, _VALLEN );

    // debug
    //dn("s22061  insertIndexFromTable _KEYLEN=%ld _VALLEN=%ld", _KEYLEN, _VALLEN);
    //pair.printkv(true);

	rc = insertPair( pair, tabHasFlushed);

	if ( indexbuf ) free( indexbuf );
	dn("s5083 JagIndex::formatIndexCmdFromTable rc=%d", rc );
	return rc;
}

int JagIndex::insertPair( JagDBPair &pair, bool tabHasFlushed )
{
	d("s40931 JagIndex::insertPair...\n");

    bool hasFlushed;
	int irc = _darrFamily->insert( pair, hasFlushed );
	d("s40931 JagIndex::insertPair irc=%d\n", irc);

    if ( tabHasFlushed ) {
        bool arrFlushed;
        dn("s627388 tabHasFlushed flushInsertBuffer ...");
	    jagint rcflush = _darrFamily->flushInsertBuffer( arrFlushed );
        dn("s020283 JagIndex::insertPair tabHasFlushed");
        dn("s020283 _darrFamily->flushInsertBuffer arrFlushed=%d rcflush=%ld", arrFlushed, rcflush );
    }

	return irc;
}

int JagIndex::removePair( const JagDBPair &pair )
{
	return _darrFamily->remove( pair );
}

int JagIndex::updateFromTable( const char *tableoldbuf, const char *tablenewbuf )
{
	insertIndexFromTable( tablenewbuf, false );
	return 1;
}

int JagIndex::removeFromTable( const char *tablebuf )
{
    char *indexbuf = (char*)jagmalloc(_KEYVALLEN+1);
    memset(indexbuf, 0, _KEYVALLEN+1);

    int rc = bufChangeT2I( indexbuf, (char*)tablebuf );
    if ( !rc || *indexbuf == '\0' ) {
        if ( indexbuf ) free( indexbuf );
        return 0;
    }

    dbNaturalFormatExchange( indexbuf, _numKeys, _schAttr,0,0, " " ); // natural format -> db format

    JagDBPair pair( indexbuf, _KEYLEN, indexbuf+_KEYLEN, _VALLEN );

    rc = removePair( pair );

    if ( indexbuf ) free( indexbuf );
    // d("s5083 JagIndex::formatIndexCmdFromTable rc=%d\n", rc );
    return rc;

}

bool JagIndex::needUpdate( const Jstr &colName ) const
{
    Jstr dbcolumn = _dbname + "." + _indexName + "." + colName;
    int getpos;
    if ( _indexmap->getValue(dbcolumn, getpos) ) return true;
    else return false;
}

jagint JagIndex::getCount( const char *cmd, const JagRequest& req, JagParseParam *parseParam, Jstr &errmsg )
{
	if ( parseParam->hasWhere ) {
		JagDataAggregate *jda = NULL;
		jagint cnt = select( jda, cmd, req, parseParam, errmsg, false );
		if ( jda ) delete jda;
		return cnt;
	}
	else {
		return _darrFamily->getCount( );
	}
}

jagint JagIndex::select( JagDataAggregate *&jda, const char *cmd, const JagRequest& req, JagParseParam *parseParam, 
						Jstr &errmsg, bool nowherecnt, bool isInsertSelect )
{
	struct timeval now;
	gettimeofday( &now, NULL ); 
	jagint bsec = now.tv_sec;

	bool            timeoutFlag = 0;
	JagFixString    treestr;
	Jstr            treetype = " ";
	int             rc, typeMode = 0, tabnum = 0, treelength = 0;
	bool            uniqueAndHasValueCol = 0, needInit = 1;
	jagint          nm = parseParam->limit;
	std::atomic<jagint> cnt;

	cnt = 0;

    // debugging
    /**
    dn("s00182 select index:");
    _darrFamily->debugPrintBuffer();
    **/


	if ( parseParam->hasLimit && nm == 0 && nowherecnt ) return 0;
	if ( parseParam->exportType == JAG_EXPORT ) return 0;
	if ( JAG_INSERTSELECT_OP == parseParam->opcode && ! parseParam->hasTimeout ) {
        parseParam->timeout = -1;
    }

	int     keylen[1];
	int     numKeys[1];
	const   JagHashStrInt *maps[1];
	const   JagSchemaAttribute *attrs[1];

	keylen[0] = _KEYLEN;
	numKeys[0] = _numKeys;

	maps[0] = _indexmap;
	attrs[0] = _schAttr;
	JagMinMax minmax[1];
	minmax[0].setbuflen( keylen[0] );

	Jstr        newhdr, gbvheader;
	jagint      finalsendlen = 0;
	jagint      gbvsendlen = 0;
	JagSchemaRecord     nrec;
	ExprElementNode     *root = NULL;
	JagFixString        strres;
	
	if ( nowherecnt ) {
		JagVector<SetHdrAttr> hspa;
		SetHdrAttr honespa;
		AbaxString getstr;
		Jstr fullname = _dbname + "." + _tableName + "." + _indexName;

		_indexschema->getAttr( fullname, getstr );
		honespa.setattr( _numKeys, false, _dbobj, &_indexRecord, getstr.c_str() );
		hspa.append( honespa );

		rc = rearrangeHdr( 1, maps, attrs, parseParam, hspa, newhdr, gbvheader, finalsendlen, gbvsendlen );
		if ( !rc ) {
			errmsg = "E0833 Error header for select";
			return -1;			
		}
		nrec.parseRecord( newhdr.c_str() );
	}

	if ( parseParam->hasWhere ) {
		root = parseParam->whereVec[0].tree->getRoot();

		rc = root->setWhereRange( maps, attrs, keylen, numKeys, 1, uniqueAndHasValueCol, minmax, treestr, typeMode, tabnum );

		if ( 0 == rc ) {
			memset( minmax[0].minbuf, 0, keylen[0]+1 );
			memset( minmax[0].maxbuf, 255, keylen[0] );
			(minmax[0].maxbuf)[keylen[0]] = '\0';
		} else if ( rc < 0 ) {
			errmsg = "E0843 Error header for select";
			return -1;
		}
	}
	
	// finalbuf, hasColumn len or KEYVALLEN if !hasColumn
	// gbvbuf, if has group by
	char *finalbuf = (char*)jagmalloc(finalsendlen+1);
	memset(finalbuf, 0, finalsendlen+1);

    /**
	char *gbvbuf = (char*)jagmalloc(gbvsendlen+1);
	memset(gbvbuf, 0, gbvsendlen+1);
    **/

	JagMemDiskSortArray *gmdarr = NULL;

	if ( gbvsendlen > 0 ) {
		gmdarr = newObject<JagMemDiskSortArray>();
		gmdarr->init( atoi((_cfg->getValue("GROUPBY_SORT_SIZE_MB", "1024")).c_str()), gbvheader.c_str(), "GroupByValue" );
		gmdarr->beginWrite();
	}

	// if insert into ... select syntax, create cpp client object to send insert cmd to corresponding server
	JaguarCPPClient *pcli = NULL;
	if ( JAG_INSERTSELECT_OP == parseParam->opcode ) {
		//pcli = new JaguarCPPClient();
		pcli = newObject<JaguarCPPClient>();
		Jstr host = "localhost", unixSocket = Jstr("/TOKEN=") + _servobj->_servToken;
		if ( _servobj->_listenIP.size() > 0 ) { host = _servobj->_listenIP; }
		if ( !pcli->connect( host.c_str(), _servobj->_port, "admin", "jaguar", "test", unixSocket.c_str(), 0 ) ) {
			jd(JAG_LOG_LOW, "s4055 Connect (%s:%s) (%s:%d) error [%s], retry ...\n",
					  "admin", "jaguar", host.c_str(), _servobj->_port, pcli->error() );
			pcli->close();
			if ( pcli ) delete pcli;
			if ( gmdarr ) delete gmdarr;
			if ( finalbuf ) free(finalbuf);
			//if ( gbvbuf ) free(gbvbuf);
			errmsg = "E0844 Error connect";
			return -1;
		}
	}
	
	// point query, one record
	if ( memcmp(minmax[0].minbuf, minmax[0].maxbuf, _KEYLEN) == 0 ) {
		JagDBPair pair( minmax[0].minbuf, _KEYLEN );
		if ( _darrFamily->get( pair ) ) {
			const char *buffers[1];
			char *buf = (char*)jagmalloc(_KEYVALLEN+1);
			memset( buf, 0, _KEYVALLEN+1 );	
			memcpy(buf, pair.key.c_str(), _KEYLEN);
			memcpy(buf+_KEYLEN, pair.value.c_str(), _VALLEN);
			buffers[0] = buf;
			Jstr iscmd;
			Jstr hdir;

			if ( JAG_GETFILE_OP == parseParam->opcode ) { 
				char *tablebuf = (char*)jagmalloc(_TABKEYLEN+_TABVALLEN+1);
				memset( tablebuf, 0, _TABKEYLEN+_TABVALLEN+1 );
				bufChangeT2I( buf, tablebuf );
				JagFixString kstr( tablebuf, _TABKEYLEN, _TABKEYLEN );
				// d("s30094 print:\n" );
				// fstr.dump();
				//hdir = fileHashDir( fstr );
                hdir = getFileHashDir( _tableRecord, kstr );

				jagfree( tablebuf );
			}

			if ( !uniqueAndHasValueCol ) {
				// key only
				MultiDbNaturalFormatExchange( (char**)buffers, 1, numKeys, attrs ); // db format -> natural format

				if ( parseParam->opcode == JAG_GETFILE_OP ) { setGetFileAttributes( hdir, parseParam, (char**)buffers ); }
				JagTable::nonAggregateFinalbuf(NULL, maps, attrs, &req, buffers, parseParam, finalbuf, finalsendlen, 
												jda, _dbobj, cnt, nowherecnt, NULL, true );

				if ( parseParam->opcode == JAG_GETFILE_OP && parseParam->getFileActualData ) {
					Jstr  ddcol, inpath; 
                    int   getpos; 
                    char  fname[JAG_FILE_FIELD_LEN+1];
                    int   actualSent = 0;

					for ( int i = 0; i < parseParam->selColVec.size(); ++i ) {
						ddcol = _dbobj + "." + parseParam->selColVec[i].getfileCol.c_str();
						if ( _indexmap->getValue(ddcol, getpos) ) {
							memcpy( fname, buffers[0]+_schAttr[getpos].offset, _schAttr[getpos].length );
							inpath = _darrFamily->_sfilepath + "/" + hdir + "/" + fname;
							req.session->active = false;

							oneFileSender( req.session->sock, inpath, parseParam->objectVec[0].dbName,
                                            parseParam->objectVec[0].indexName, hdir, actualSent );
                            //  int oneFileSender( JAGSOCK sock, const Jstr &inpath, const Jstr &jagHome, bool tryPNdata, int &actualSent )

							req.session->active = true;
						}
					}
				} else {
					if ( JAG_INSERTSELECT_OP == parseParam->opcode ) {
						if ( formatInsertSelectCmdHeader( parseParam, iscmd ) ) {
							JagTable::formatInsertFromSelect( parseParam, attrs[0], finalbuf, buffers[0], 
															  finalsendlen, _numCols, pcli, iscmd );
						}
					}
				}
			} else if ( root ) {
				// has value column; change to natural data before apply checkFuncValid 
				MultiDbNaturalFormatExchange( (char**)buffers, 1, numKeys, attrs ); // db format -> natural format

				if ( root->checkFuncValid( NULL, maps, attrs, buffers, strres, typeMode, treetype, treelength, needInit, 0, 0 ) > 0 ) {
					if ( parseParam->opcode == JAG_GETFILE_OP ) { setGetFileAttributes( hdir, parseParam, (char**)buffers ); }
					JagTable::nonAggregateFinalbuf( NULL, maps, attrs, &req, buffers, parseParam, finalbuf, finalsendlen, jda, 
												    _dbobj, cnt, nowherecnt, NULL, true );

					if ( parseParam->opcode == JAG_GETFILE_OP && parseParam->getFileActualData ) {
						Jstr ddcol, inpath; 
                        int  getpos; 
                        char fname[JAG_FILE_FIELD_LEN+1];
                        int  actualSent = 0;

						for ( int i = 0; i < parseParam->selColVec.size(); ++i ) {
							ddcol = _dbobj + "." + parseParam->selColVec[i].getfileCol.c_str();
							if ( _indexmap->getValue(ddcol, getpos) ) {
								memcpy( fname, buffers[0]+_schAttr[getpos].offset, _schAttr[getpos].length );
								inpath = _darrFamily->_sfilepath + "/" + hdir + "/" + fname;
								req.session->active = false;

								//oneFileSender( req.session->sock, inpath, actualSent );
								oneFileSender( req.session->sock, inpath, parseParam->objectVec[0].dbName,
                                               parseParam->objectVec[0].indexName, hdir, actualSent );

								req.session->active = true;
							}
						}
					} else {
						if ( JAG_INSERTSELECT_OP == parseParam->opcode ) {
							if ( formatInsertSelectCmdHeader( parseParam, iscmd ) ) {
								JagTable::formatInsertFromSelect( parseParam, attrs[0], finalbuf, buffers[0], 
																  finalsendlen, _numCols, pcli, iscmd );
							}
						}
					}
				}
			}
			free( buf );
		}
	} else { // range query
        dn("s266301 select from index range query or select all");
		jagint callCounts = -1, lastBytes = 0;
		if ( JAG_INSERTSELECT_OP != parseParam->opcode ) {
			if ( !jda ) jda = newObject<JagDataAggregate>();

            jda->_keylen = _KEYLEN;
            jda->_vallen = _VALLEN;
            jda->_datalen = _KEYVALLEN;

			jda->setwrite( _dbobj, _dbobj, false );
			jda->setMemoryLimit( _darrFamily->getElements( )*_KEYVALLEN*2 );
		}

		// get num of threads
        jagint numthrds = 1;
        /***
		bool lcpu = false; 
        jagint numthrds = _darrFamily->_darrlist.size()/_servobj->_numCPUs;
		if ( numthrds < 1 ) {
			numthrds = _darrFamily->_darrlist.size();
			lcpu = true;
			if ( numthrds < 1 ) { numthrds = 1; }
		}
        ***/
		
        dn("s727288 numthrds=%lld", numthrds );

		JagParseParam *pparam[numthrds];

		JagParseAttribute jpa( _servobj, req.session->timediff, _servobj->servtimediff, req.session->dbname, _servobj->_cfg );

		if ( parseParam->hasGroup ) {
            // qwer123 4/13/2023
	        char *gbvbuf = (char*)jagmalloc(gbvsendlen+1);
	        memset(gbvbuf, 0, gbvsendlen+1);

			// group by, no insert into ... select ... syntax allowed
			JagMemDiskSortArray *lgmdarr[numthrds];
			JagParser parser((void*)_servobj);
			for ( jagint i = 0; i < numthrds; ++i ) {
				pparam[i] = new JagParseParam( &parser );
				d("s52958 parser.parseCommand\n");
				parser.parseCommand( jpa, cmd, pparam[i], errmsg );

				lgmdarr[i] = newObject<JagMemDiskSortArray>();
				lgmdarr[i]->init( 40, gbvheader.c_str(), "GroupByValue" );
				lgmdarr[i]->beginWrite();
			}

			jagint memlim = availableMemory( callCounts, lastBytes )/8/numthrds/1024/1024;
			if ( memlim <= 0 ) memlim = 1;
			
			ParallelCmdPass psp[numthrds];
			for ( jagint i = 0; i < numthrds; ++i ) {
				#if 1
				psp[i].ptab = NULL;
				psp[i].pindex = this;
				psp[i].pos = i;
				psp[i].sendlen = gbvsendlen;
				psp[i].parseParam = pparam[i];
				psp[i].gmdarr = lgmdarr[i];
				psp[i].req = (JagRequest*) &req;
				psp[i].jda = jda;
				psp[i].writeName = _dbobj;
				psp[i].recordcnt = &cnt;
				psp[i].actlimit = nm;
				psp[i].nowherecnt = nowherecnt;
				psp[i].nrec = &nrec;
				psp[i].memlimit = memlim;
				psp[i].minbuf = minmax[0].minbuf;
				psp[i].maxbuf = minmax[0].maxbuf;
				psp[i].starttime = bsec;
				psp[i].kvlen = _KEYVALLEN;

                /***
				if ( lcpu ) {
					psp[i].spos = i; psp[i].epos = i;
				} else {
					psp[i].spos = i*_servobj->_numCPUs;
					psp[i].epos = psp[i].spos+_servobj->_numCPUs-1;
					if ( i == numthrds-1 ) psp[i].epos = _darrFamily->_darrlist.size()-1;
				}
                ***/
				psp[i].spos = 0; 
                psp[i].epos = _darrFamily->_darrlist.size()-1;


				#else
                JagTable::fillCmdParse( JAG_INDEX, (void *)this, i, gbvsendlen, pparam,
                             lgmdarr[i], req, jda, _dbobj,  &cnt, nm, nowherecnt, &nrec, memlim, minmax,
                             bsec, KEYVALLEN, _servobj, numthrds, _darrFamily, lcpu, psp );
			    #endif

				if ( JAG_INSERTSELECT_OP == parseParam->opcode && ! parseParam->hasTimeout ) {
                    pparam[i]->timeout = -1;
                }

                if ( 0 == i ) {
                    psp[i].useInsertBuffer = true;
                } else {
                    psp[i].useInsertBuffer = false;
                }

                // has group by
				JagTable::parallelSelectStatic( (void*)&psp[i] );
			}
			
			for ( jagint i = 0; i < numthrds; ++i ) {
				if ( psp[i].timeoutFlag ) timeoutFlag = 1;
				lgmdarr[i]->endWrite();
				lgmdarr[i]->beginRead();

				while ( true ) {
					rc = lgmdarr[i]->get( gbvbuf );
					if ( !rc ) break;
					JagDBPair pair(gbvbuf, gmdarr->_keylen, gbvbuf+gmdarr->_keylen, gmdarr->_vallen, true );
					rc = gmdarr->groupByUpdate( pair );
				}

				lgmdarr[i]->endRead();
				delete lgmdarr[i];
				delete pparam[i];
			}
			
			JagTable::groupByFinalCalculation( gbvbuf, nowherecnt, finalsendlen, cnt, nm, 
										       _dbobj, parseParam, jda, gmdarr, &nrec );

            free(gbvbuf );

		} else {
            //has no group by
            dn("s8880123 has no group by");
			// check if has aggregate
			bool hAggregate = false;
			if ( parseParam->hasColumn ) {
				for ( int i = 0; i < parseParam->selColVec.size(); ++i ) {
					if ( parseParam->selColVec[i].isAggregate ) {
						hAggregate = true;
						break;
					}
				}
			}
			
			JagParser parser((void*)_servobj);
			for ( jagint i = 0; i < numthrds; ++i ) {
				pparam[i] = new JagParseParam( &parser );
				d("s52938 parser.parseCommand\n");
				parser.parseCommand( jpa, cmd, pparam[i], errmsg );
			}

			jagint memlim = availableMemory( callCounts, lastBytes )/8/numthrds/1024/1024;
			if ( memlim <= 0 ) memlim = 1;
			
			ParallelCmdPass  psp[numthrds];

			for ( jagint i = 0; i < numthrds; ++i ) {
				psp[i].cli = pcli;
				#if 1
				psp[i].ptab = NULL;
				psp[i].pindex = this;
				psp[i].pos = i;
				psp[i].sendlen = finalsendlen;
				psp[i].parseParam = pparam[i];
				psp[i].gmdarr = NULL;
				psp[i].req = (JagRequest*)&req;
				psp[i].jda = jda;
				psp[i].writeName = _dbobj;
				psp[i].recordcnt = &cnt;
				psp[i].actlimit = nm;
				psp[i].nowherecnt = nowherecnt;
				psp[i].nrec = &nrec;
				psp[i].memlimit = memlim;
				psp[i].minbuf = minmax[0].minbuf;
				psp[i].maxbuf = minmax[0].maxbuf;
				psp[i].starttime = bsec;
				psp[i].kvlen = _KEYVALLEN;

                /***
				if ( lcpu ) {
					psp[i].spos = i; psp[i].epos = i;
				} else {
					psp[i].spos = i*_servobj->_numCPUs;
					psp[i].epos = psp[i].spos+_servobj->_numCPUs-1;
					if ( i == numthrds-1 ) psp[i].epos = _darrFamily->_darrlist.size()-1;
				}
                ***/
				psp[i].spos = 0; 
                psp[i].epos = _darrFamily->_darrlist.size()-1;


				#else
                JagTable::fillCmdParse( JAG_INDEX, (void *)this, i, gbvsendlen, pparam,
                             NULL, req, jda, _dbobj,  &cnt, nm, nowherecnt, &nrec, memlim, minmax,
                             bsec, KEYVALLEN, _servobj, numthrds, _darrFamily, lcpu, psp );
				#endif


				if ( JAG_INSERTSELECT_OP == parseParam->opcode && ! parseParam->hasTimeout ) {
                    pparam[i]->timeout = -1;
                }

                if ( 0 == i ) {
                    psp[i].useInsertBuffer = true;
                } else {
                    psp[i].useInsertBuffer = false;
                }

                dn("s99373700 index select calls JagTable::parallelSelectStatic() useInsertBuffer=%d ...", psp[i].useInsertBuffer);
				JagTable::parallelSelectStatic( (void*)&psp[i] );
			}

			for ( jagint i = 0; i < numthrds; ++i ) {
				if ( psp[i].timeoutFlag ) timeoutFlag = 1;
			}

			if ( hAggregate ) {
				JagTable::aggregateFinalbuf( &req, newhdr, numthrds, pparam, finalbuf, finalsendlen, jda, 
											_dbobj, cnt, nowherecnt, &nrec );
			}
		
			for ( jagint i = 0; i < numthrds; ++i ) {
				delete pparam[i];
			}			

            dn("s627280 no group by");
		}

		if ( jda ) {
			jda->flushwrite();
		}

	}
	
	if ( timeoutFlag ) {
		Jstr timeoutStr = "Index select command has timed out. Results have been truncated;";
		sendER( req, timeoutStr);
	}

	if ( pcli ) {
		pcli->close();
		delete pcli;
	}

	if ( gmdarr ) delete gmdarr;
	if ( finalbuf ) free ( finalbuf );
	//if ( gbvbuf ) free ( gbvbuf );

    dn("s66272 index::select cnt=%ld", (jagint)cnt);
	return (jagint)cnt;
}

int JagIndex::drop()
{
	if ( _darrFamily ) {
		_darrFamily->drop();
		delete _darrFamily;
	}
	_darrFamily = NULL;

    // rmdir
    Jstr datahome = _cfg->getJDBDataHOME( _replicType );
    Jstr dbpath = datahome + "/" + _dbname;
    Jstr tabidx = _tableName + "." + _indexName;

    Jstr dbtabpath = dbpath + "/" + _tableName + "/" + tabidx + "." + intToStr(_replicType) + ".jdb";

    JagFileMgr::rmdir( dbtabpath, true );
    d("s4222939 indexDrop() rmdir(%s)\n", dbtabpath.s() );

	return 1;
}

void JagIndex::flushBlockIndexToDisk() 
{ 
	if ( _darrFamily ) { _darrFamily->flushBlockIndexToDisk(); } 
}

void JagIndex::setGetFileAttributes( const Jstr &hdir, JagParseParam *parseParam, char *buffers[] )
{
	Jstr ddcol, inpath, instr, outstr; int getpos; char fname[JAG_FILE_FIELD_LEN+1]; struct stat sbuf;
	for ( int i = 0; i < parseParam->selColVec.size(); ++i ) {
		ddcol = _dbobj + "." + parseParam->selColVec[i].getfileCol.c_str();
		if ( _indexmap->getValue(ddcol, getpos) ) {
			memcpy( fname, buffers[0]+_schAttr[getpos].offset, _schAttr[getpos].length );
			inpath = _darrFamily->_sfilepath + "/" + hdir + "/" + fname;
			if ( stat(inpath.c_str(), &sbuf) == 0 ) {
				if ( parseParam->selColVec[i].getfileType == JAG_GETFILE_SIZE ) {
					outstr = longToStr( sbuf.st_size );
				} else if ( parseParam->selColVec[i].getfileType == JAG_GETFILE_SIZEGB ) {
					outstr = longToStr( sbuf.st_size/ONE_GIGA_BYTES);
				} else if ( parseParam->selColVec[i].getfileType == JAG_GETFILE_SIZEMB ) {
					outstr = longToStr( sbuf.st_size/ONE_MEGA_BYTES);
				} else if ( parseParam->selColVec[i].getfileType == JAG_GETFILE_TIME ) {
					char octime[JAG_CTIME_LEN]; memset(octime, 0, JAG_CTIME_LEN);
 					jag_ctime_r(&sbuf.st_mtime, octime);
					octime[JAG_CTIME_LEN-1] = '\0';
					outstr = octime;
				} else if ( parseParam->selColVec[i].getfileType == JAG_GETFILE_MD5SUM ) {
					if ( lastChar( inpath ) != '/' ) {
						instr = Jstr("md5sum ") + inpath;
						outstr = Jstr(psystem( instr.c_str() ).c_str(), 32);
					} else {
						outstr = "";
					}
				} else if ( parseParam->selColVec[i].getfileType == JAG_GETFILE_FPATH ) {
					outstr = inpath;
				} else if ( parseParam->selColVec[i].getfileType == JAG_GETFILE_HOST ) {
					outstr = _servobj->_localInternalIP;
				} else if ( parseParam->selColVec[i].getfileType == JAG_GETFILE_HOSTFPATH ) {
					outstr = _servobj->_localInternalIP + ":" + inpath;
				} else if ( parseParam->selColVec[i].getfileType == JAG_GETFILE_TYPE ) {
					JagStrSplit sp(fname, '.');
					if ( sp.length() >= 2 ) {
						outstr = sp[sp.length()-1];
					} else {
						outstr = "?"; 
					}
				} else {
					outstr = fname;
				}
				parseParam->selColVec[i].strResult = outstr;
			} else {
				parseParam->selColVec[i].strResult = "error file status";
			}
		} else {
			parseParam->selColVec[i].strResult = "error file column";
		}
	}
}

jagint JagIndex::memoryBufferSize()
{
    if ( ! _darrFamily ) {
        return 0;
    }

    return _darrFamily->memoryBufferSize();
}

