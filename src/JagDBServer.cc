/*
 * Copyright JaguarDB  www.jaguardb.com
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <strings.h>
#include <string.h>
#include <signal.h>
#include <iostream>
#include <pthread.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
//#include <malloc.h>

#undef JAG_CLIENT_SIDE
#define JAG_SERVER_SIDE 1

#include <JagDef.h>
#include <JagUtil.h>
#include <JagUserID.h>
#include <JagUserRole.h>
#include <JagPass.h>
#include <JagTableSchema.h>
#include <JagIndexSchema.h>
#include <JagSchemaRecord.h>
#include <JagDBServer.h>
#include <JagNet.h>
#include <JagMD5lib.h>
#include <JagStrSplitWithQuote.h>
#include <JagParseExpr.h>
#include <JagSession.h>
#include <JagRequest.h>
#include <JagMutex.h>
#include <JagSystem.h>
#include <JagFSMgr.h>
#include <JagFastCompress.h>
#include <JagTime.h>
#include <JagHashLock.h>
#include <JagServerObjectLock.h>
#include <JagDataAggregate.h>
#include <JagUUID.h>
#include <JagTableUtil.h>
#include <JagProduct.h>
#include <JagTable.h>
#include <JagIndex.h>
#include <JagBoundFile.h>
#include <JagDBConnector.h>
#include <JagDiskArrayBase.h>
#include <JagIPACL.h>
#include <JagSingleMergeReader.h>
#include <JagDBLogger.h>
#include <JaguarAPI.h>
#include <JagParser.h>
#include <JagLineFile.h>
#include <JagCrypt.h>
#include <JagBlockLock.h>
#include <JagFamilyKeyChecker.h>
#include <base64.h>

int JagDBServer::g_receivedSignal = 0;
jagint JagDBServer::g_lastSchemaTime = 0;
jagint JagDBServer::g_lastHostTime = 0;

pthread_mutex_t JagDBServer::g_dbschemamutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t JagDBServer::g_flagmutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t JagDBServer::g_wallogmutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t JagDBServer::g_dlogmutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t mallocMutex;

JagDBServer::JagDBServer() 
{
	Jstr fpath = jaguarHome() + "/conf/host.conf";
	Jstr fpathnew = jaguarHome() + "/conf/cluster.conf";
	if ( JagFileMgr::exist( fpath ) && ( ! JagFileMgr::exist( fpathnew ) )  ) {
		jagrename( fpath.c_str(), fpathnew.c_str() );
	}

	_allowIPList = _blockIPList = NULL;
	_serverReady = false;

	g_receivedSignal = g_lastSchemaTime = g_lastHostTime = 0;
	_taskID = _numPrimaryServers = 1;
	_nthServer = 0;
	_xferInsert = 0;
	_isSSD = 0;
	_memoryMode = JAG_MEM_HIGH;
	_shutDownInProgress = 0;
	numInInsertBuffers = numInserts = numSelects = numUpdates = numDeletes = 0;
	_connections = 0;
	
	_hashRebalanceLen = 0;
	_hashRebalanceFD = -1;
	
	jdfsMgr = newObject<JagFSMgr>();
	servtimediff = JagTime::getTimeZoneDiff();

	_cfg = newObject<JagCfg>();
	_userDB = NULL;
	_prevuserDB = NULL;
	_nextuserDB = NULL;
	_userRole = NULL;
	_prevuserRole = NULL;
	_nextuserRole = NULL;
	_tableschema = NULL;
	_indexschema = NULL;
	_prevtableschema = NULL;
	_previndexschema = NULL;
	_nexttableschema = NULL;
	_nextindexschema = NULL;

	_delPrevOriCommandFile = NULL;
	_delPrevRepCommandFile = NULL;
	_delPrevOriRepCommandFile = NULL;
	_delNextOriCommandFile = NULL;
	_delNextRepCommandFile = NULL;
	_delNextOriRepCommandFile = NULL;
	_recoveryRegCommandFile = NULL;
	_recoverySpCommandFile = NULL;

	_objectLock = new JagServerObjectLock( this );
	_jagUUID = new JagUUID();
	pthread_rwlock_init(&_aclrwlock, NULL);

	_internalHostNum = new JagHashMap<AbaxString, jagint>();

	Jstr dblist, dbpath;
	dbpath = _cfg->getJDBDataHOME( JAG_MAIN );
	dblist = JagFileMgr::listObjects( dbpath );
	_objectLock->setInitDatabases( dblist, JAG_MAIN );

	dbpath = _cfg->getJDBDataHOME( JAG_PREV );
	dblist = JagFileMgr::listObjects( dbpath );
	_objectLock->setInitDatabases( dblist, JAG_PREV );

	dbpath = _cfg->getJDBDataHOME( JAG_NEXT );
	dblist = JagFileMgr::listObjects( dbpath );
	_objectLock->setInitDatabases( dblist, JAG_NEXT );

	_beginAddServer = 0;
	_exclusiveAdmins = 0;
	_doingRemoteBackup = 0;
	_doingRestoreRemote = 0;
	_restartRecover = 0;
	_addClusterFlag = 0;
	_newdcTrasmittingFin = 0;

	init( _cfg );

	_dbConnector = newObject<JagDBConnector>( );

	_clusterMode = true;
	_faultToleranceCopy = _cfg->getIntValue("REPLICATION", 1);
	if ( _faultToleranceCopy < 1 ) _faultToleranceCopy = 1;
	else if ( _faultToleranceCopy > 3 ) _faultToleranceCopy = 3;

	loadACL(); 

	_jdbMonitorTimedoutPeriod = _cfg->getLongValue("JDB_MONITOR_TIMEDOUT", 600);
	jd(JAG_LOG_LOW, "JdbMonitorTimedoutPeriod = [%d]\n", _jdbMonitorTimedoutPeriod );

	_localInternalIP = _dbConnector->_nodeMgr->_selfIP;
	if ( _localInternalIP.size() > 0 ) {
		jd(JAG_LOG_LOW, "Host IP = [%s]\n", _localInternalIP.c_str() );
		jd(JAG_LOG_LOW, "Servers [%s]\n", _dbConnector->_nodeMgr->_allNodes.c_str() );
	}

	jd(JAG_LOG_LOW, "Clusters [%d]\n",  _dbConnector->_nodeMgr->_totalClusterNumber );
	jd(JAG_LOG_LOW, "Cluster  [%d]\n",  _dbConnector->_nodeMgr->_hostClusterNumber );

	_servToken = _cfg->getValue("SERVER_TOKEN", "wvcYrfYdVagqXQ4s3eTFKyvNFxV");

	JagStrSplit sp( _dbConnector->_nodeMgr->_hostClusterNodes, '|' );
	_numPrimaryServers = sp.length();
	for ( int i=0; i < sp.length(); ++i ) {
		if ( sp[i] == _localInternalIP ) {
			_nthServer = i;
			break;
		}
	}

	if ( _numPrimaryServers == 1 ) {
		if ( _faultToleranceCopy >= 2 ) {
			_faultToleranceCopy = 1;
		}
	} else if ( _numPrimaryServers == 2 ) {
		if ( _faultToleranceCopy >= 3 ) {
			_faultToleranceCopy = 2;
		}
	}

	for ( int i=0; i < sp.length(); ++i ) {
		_internalHostNum->addKeyValue(AbaxString(sp[i]), i);
	}

	jd(JAG_LOG_LOW, "This is %d-th server in the cluster\n", _nthServer );
	jd(JAG_LOG_LOW, "Fault tolerance copy is %d\n", _faultToleranceCopy );

	_perfFile = jaguarHome() + "/log/perflog.txt";
	JagBoundFile bf( _perfFile.c_str(), 96 );
	bf.openAppend();
	bf.close();

	Jstr dologmsg = makeLowerString(_cfg->getValue("DO_DBLOG_MSG", "no"));
	Jstr dologerr = makeLowerString(_cfg->getValue("DO_DBLOG_ERR", "yes"));
	int logdays = _cfg->getIntValue("DBLOG_DAYS", 3);
	int logmsg=0, logerr=0;
	if ( dologmsg == "yes" ) { logmsg = 1; jd(JAG_LOG_LOW, "DB logging message is enabled.\n" ); } 
	if ( dologerr == "yes" ) { logerr = 1; jd(JAG_LOG_LOW, "DB logging error logger is enabled.\n" ); } 
	if ( logmsg || logerr ) {
		jd(JAG_LOG_LOW, "DB log %d days\n", logdays );
	}
	_dbLogger = new JagDBLogger( logmsg, logerr, logdays );

	_numCPUs = _jagSystem.getNumCPUs();
	jd(JAG_LOG_LOW, "Number of cores %d\n", _numCPUs );

	pthread_mutex_init( &g_dbconnectormutex, NULL );

	pthread_mutex_init( &_delPrevOriMutex, NULL );
	pthread_mutex_init( &_delPrevRepMutex, NULL );
	pthread_mutex_init( &_delPrevOriRepMutex, NULL );
	pthread_mutex_init( &_delNextOriMutex, NULL );
	pthread_mutex_init( &_delNextRepMutex, NULL );
	pthread_mutex_init( &_delNextOriRepMutex, NULL );
}

JagDBServer::~JagDBServer()
{
    destroy();
	jagclose( _dumfd );
}

void JagDBServer::destroy()
{
	if ( _cfg ) {
		delete _cfg;
		_cfg = NULL;
	}

	if ( _userDB ) {
		delete _userDB;
		_userDB = NULL;
	}

	if ( _prevuserDB ) {
		delete _prevuserDB;
		_prevuserDB = NULL;
	}

	if ( _nextuserDB ) {
		delete _nextuserDB;
		_nextuserDB = NULL;
	}

	if ( _userRole ) {
		delete _userRole;
		_userRole = NULL;
	}

	if ( _prevuserRole ) {
		delete _prevuserRole;
		_prevuserRole = NULL;
	}

	if ( _nextuserRole ) {
		delete _nextuserRole;
		_nextuserRole = NULL;
	}

	if ( _tableschema ) {
		delete _tableschema;
		_tableschema = NULL;
	}

	if ( _indexschema ) {
		delete _indexschema;
		_indexschema = NULL;
	}

	if ( _prevtableschema ) {
		delete _prevtableschema;
		_prevtableschema = NULL;
	}

	if ( _previndexschema ) {
		delete _previndexschema;
		_previndexschema = NULL;
	}

	if ( _nexttableschema ) {
		delete _nexttableschema;
		_nexttableschema = NULL;
	}

	if ( _nextindexschema ) {
		delete _nextindexschema;
		_nextindexschema = NULL;
	}

	if ( _dbConnector ) {
		delete _dbConnector;
		_dbConnector = NULL;
	}

	if ( _taskMap ) {
		delete _taskMap;
		_taskMap = NULL;
	}
	
	/***
	if ( _joinMap ) {
		delete _joinMap;
		_joinMap = NULL;
	}
	***/

	if ( _scMap ) {
		delete _scMap;
		_scMap = NULL;
	}

	if ( _objectLock ) {
		delete _objectLock;
		_objectLock = NULL;
	}

	if ( _jagUUID ) {
		delete _jagUUID;
		_jagUUID = NULL;
	}

	if ( _internalHostNum ) {
		delete _internalHostNum;
		_internalHostNum = NULL;
	}

	if ( _delPrevOriCommandFile ) {
		jagfclose( _delPrevOriCommandFile );
		_delPrevOriCommandFile = NULL;
	}

	if ( _delPrevRepCommandFile ) {
		jagfclose( _delPrevRepCommandFile );
		_delPrevRepCommandFile = NULL;
	}

	if ( _delPrevOriRepCommandFile ) {
		jagfclose( _delPrevOriRepCommandFile );
		_delPrevOriRepCommandFile = NULL;
	}

	if ( _delNextOriCommandFile ) {
		jagfclose( _delNextOriCommandFile );
		_delNextOriCommandFile = NULL;
	}

	if ( _delNextRepCommandFile ) {
		jagfclose( _delNextRepCommandFile );
		_delNextRepCommandFile = NULL;
	}

	if ( _delNextOriRepCommandFile ) {
		jagfclose( _delNextOriRepCommandFile );
		_delNextOriRepCommandFile = NULL;
	}

	if ( _recoveryRegCommandFile ) {
		jagfclose( _recoveryRegCommandFile );
		_recoveryRegCommandFile = NULL;
	}

	if ( _recoverySpCommandFile ) {
		jagfclose( _recoverySpCommandFile );
		_recoverySpCommandFile = NULL;
	}


	if ( _blockIPList )  delete _blockIPList;
	if ( _allowIPList )  delete _allowIPList;
	pthread_rwlock_destroy(&_aclrwlock);

	delete _dbLogger;

	pthread_mutex_destroy( &g_dbschemamutex );
	pthread_mutex_destroy( &g_flagmutex );
	pthread_mutex_destroy( &g_wallogmutex );
	pthread_mutex_destroy( &g_dlogmutex );

	pthread_mutex_destroy( &g_dbconnectormutex );

	pthread_mutex_destroy( &_delPrevOriMutex );
	pthread_mutex_destroy( &_delPrevRepMutex );
	pthread_mutex_destroy( &_delPrevOriRepMutex );

	pthread_mutex_destroy( &_delNextOriMutex );
	pthread_mutex_destroy( &_delNextRepMutex );
	pthread_mutex_destroy( &_delNextOriRepMutex );

	pthread_mutex_destroy( &mallocMutex );

}

int JagDBServer::main(int argc, char*argv[])
{
	pthread_mutex_init( &mallocMutex, NULL );

	jagint callCounts = -1, lastBytes = 0;
	jd(JAG_LOG_LOW, "s1101 server::main() availmem=%ld M\n", availableMemory( callCounts, lastBytes)/ONE_MEGA_BYTES );

	JagNet::socketStartup();

	umask(077);
	initDirs();

	initConfigs();
	jd(JAG_LOG_LOW, "Initialized config data\n" );

	_jagSystem.initLoad(); // for load stats

	pthread_mutex_init( &g_dbschemamutex, NULL );
	pthread_mutex_init( &g_flagmutex, NULL );
	pthread_mutex_init( &g_wallogmutex, NULL );
	pthread_mutex_init( &g_dlogmutex, NULL );

	createSocket( argc, argv );

	jd(JAG_LOG_LOW, "Initialized listening socket\n" );
	_activeThreadGroups = 0;
	_activeClients = 0;
	_threadGroupSeq = 0;

	makeThreadGroups( _threadGroups + _initExtraThreads, _threadGroupSeq++, 0 );

	jd(JAG_LOG_LOW, "Created socket thread groups\n" );
	jd(JAG_LOG_LOW, "s1105 availmem=%ld M\n", availableMemory( callCounts, lastBytes)/ONE_MEGA_BYTES );

	initObjects();
	jd(JAG_LOG_LOW, "s1102 availmem=%ld M\n", availableMemory( callCounts, lastBytes)/ONE_MEGA_BYTES );
	createAdmin();

	jd(JAG_LOG_LOW, "makeTableObjects ...\n");
	makeTableObjects( );
	jd(JAG_LOG_LOW, "makeTableObjects is done\n");

	jd(JAG_LOG_LOW, "s1103 availmem=%ld M\n", availableMemory( callCounts, lastBytes)/ONE_MEGA_BYTES );

	JAG_BLURT jaguar_mutex_lock ( &g_flagmutex ); JAG_OVER
	_restartRecover = 1;
	jaguar_mutex_unlock ( &g_flagmutex );

	resetDeltaLog();
	// resetRegSpLog();  3/31/2023
	jd(JAG_LOG_LOW, "s1104 availmem=%ld M\n", availableMemory( callCounts, lastBytes)/ONE_MEGA_BYTES );

	mainInit();
	jd(JAG_LOG_LOW, "s1106 availmem=%ld M\n", availableMemory( callCounts, lastBytes)/ONE_MEGA_BYTES );

	makeSysTables( 0 );
	makeSysTables( 1 );
	makeSysTables( 2 );

	printResources();
	dn("s88092 _serverReady is true");
	_serverReady = true;

	//openDataCenterConnection();
	jd(JAG_LOG_LOW, "JaguarDB server ready\n" );

	jaguint seq = 1;
	
	while ( true ) {
		sleep(60);

		doBackup( seq );
		writeLoad( seq );

		//rotateDinsertLog();
		//refreshDataCenterConnections( seq );

		if ( 0 == (seq%15) ) {
			refreshACL( 1 );
		}

		//jaguar_mutex_lock ( &g_dlogmutex );
		if ( checkDeltaFileStatus() && !_restartRecover ) {
            // send delta log to other servers
			onlineRecoverDeltaLog();
		}
		//jaguar_mutex_unlock ( &g_dlogmutex );

		++seq;
		if ( seq >= ULLONG_MAX-1 ) { seq = 0; }

		jagmalloc_trim(0);

		// dynamically increase thread groups
		checkAndCreateThreadGroups();

		// refresh user uid and password
		refreshUserDB( seq );
		refreshUserRole( seq );

	}

	mainClose( );
	JagNet::socketCleanup();
}

// static
void JagDBServer::addTask(  jaguint taskID, JagSession *session, const char *mesg )
{
	char buf[256];
	const int LEN=64;
	char sbuf[LEN+1];
	memset( sbuf, 0, LEN+1 );
	strncpy( sbuf, mesg, LEN );

	// "threadID|userid|dbname|timestamp|query"
	sprintf( buf, "%s|%s|%s|%lu|%s", 
			ulongToString( pthread_self() ).c_str(), session->uid.c_str(), session->dbname.c_str(), time(NULL), sbuf );

	session->servobj->_taskMap->addKeyValue( taskID, AbaxString(buf) );
}

// Process commands in one thread
// return 1; return 0 if in shutting down
int JagDBServer::processMultiSingleCmd( JagRequest &req, const char *mesg, jagint msglen, 
										jagint &threadSchemaTime, jagint &threadHostTime, jagint threadQueryTime, 
										bool redoOnly, int isReadOrWriteCommand, const Jstr &lastMsg )
{
    dn("s2457103 processMultiSingleCmd objectLock=%p", _objectLock );

	if ( _shutDownInProgress > 0 ) {
		return 0;
	}	

	if ( _faultToleranceCopy > 1 ) {
        bool isThreeByte = isServerStatBytes( mesg, msglen );

        if ( isThreeByte && lastMsg.size() > 0 ) {
            int lastCmdType = checkReadOrWriteCommand( lastMsg.s() );
            dn("s4600198 got ServerStatusBytes=[%s] lastCmdType=%d", mesg, lastCmdType );
            if ( lastCmdType != JAG_WRITE_SQL ) {
                dn("s34086002 lastmsg was not write, ignore");
                return 1;
            }

            char    rephdr[4];
			int     rsmode;

			memcpy( rephdr, mesg, 3 );
			rsmode = getReplicateStatusMode( rephdr );
            dn("s1098203 getReplicateStatusMode rsmode=%d", rsmode );
			if ( rsmode >0 ) {
                dn("s1000876 deltalogCommand mesg=[%s] req.batchReply=%d", lastMsg.s(), req.batchReply );
				deltalogCommand( rsmode, req.session, lastMsg.s(), req.batchReply );
			} 

            return 1;

		} else {
            dn("s3800712 not isThreeByte or short lastMsg=[%s]", lastMsg.s() );
            // continue down
        }
    }

	int rc;
	Jstr reterr, rowFilter;

	//JAG_BLURT jaguar_mutex_lock ( &g_flagmutex ); JAG_OVER
	    JagParseAttribute jpa( this, req.session->timediff, servtimediff, req.session->dbname, _cfg );
	//jaguar_mutex_unlock ( &g_flagmutex );

	JagParser parser((void*)this);
	JagParseParam pparam( &parser ); 
	//jaguar_mutex_unlock ( &g_flagmutex );
	
    dn("s533001 req.batchReply=%d req.hasReply=%d req.session->replicType=%d", req.batchReply, req.hasReply, req.session->replicType  );
	if ( req.batchReply ) {
		// inserts

		bool brc = parser.parseCommand( jpa, mesg, &pparam, reterr );
    	if ( brc ) {
			// before do insert, need to check permission of this user for insert
			rc = checkUserCommandPermission( NULL, req, pparam, 0, rowFilter, reterr );
			if ( rc ) {
				if ( ! redoOnly && req.batchReply ) {
					logCommand( &pparam, req.session, mesg, msglen, 2 );
				}
				
                dn("s100001 doInsert ...");
				rc = doInsert( req, pparam, reterr, mesg );
                dn("s100001 doInsert rc=%d", rc);
			}  else {
                dn("s1726003 checkUserCommandPermission false");
            }
    	} else {
            dn("s165001 parser.parseCommand [%s] error brc=%d", mesg, brc );
        }

		// check timestamp to see if need to update host list for client HL
		if ( !req.session->origserv && threadHostTime < g_lastHostTime ) {
            i("s7002203 sendHostInfo to client [%s] ...\n", req.session->ip.s());
			sendHostInfo( "_chost", req, false );
            i("s7002203 sendHostInfo to client done [%s]\n", req.session->ip.s());
			threadHostTime = g_lastHostTime;
		}

		if ( req.hasReply && !redoOnly ) {
			// Only send if hasReply is true, else, not send
            dn("s4009977 req.hasReply  true");
            // remove mesg in below line
			//Jstr e = Jstr("s0008 ") + mesg;
			Jstr e = Jstr("s0008");
			sendEOM( req, e.s() ); 
		} 
	} else {
		// single cmd (not batch insert)
		//JagParser parser( (void*)this );
		//JagParseParam pparam( &parser );

		dn("s333938  single cmd mesg=[%s] pparam=%p", mesg, &pparam );

		if ( parser.parseCommand( jpa, mesg, &pparam, reterr ) ) {
			if ( JAG_SHOWSVER_OP == pparam.opcode ) {
				char hellobuf[64];
				sprintf( hellobuf, "JaguarDB Server Version: %s", _version.c_str() );
				sendDataEnd( req, hellobuf );
				return 1;
			} 

        	if ( !redoOnly && isReadOrWriteCommand == JAG_WRITE_SQL ) { // write related commands, store in wallog
				if ( JAG_INSERT_OP == pparam.opcode ) {
        			logCommand(&pparam, req.session, mesg, msglen, 2 );
				} else if ( JAG_UPDATE_OP == pparam.opcode || JAG_DELETE_OP == pparam.opcode ) {
        			logCommand(&pparam, req.session, mesg, msglen, 1 );
				} else if ( JAG_FINSERT_OP == pparam.opcode ) {
        			logCommand(&pparam, req.session, mesg, msglen, 2 );
				} else {
					// other commands (e.g. scheme changes) have no logs
				}
        	}

			dn("s02028 processCmd mesg=[%s] ...", mesg );
			rc = processCmd( jpa, req, mesg, pparam, reterr, threadQueryTime, threadSchemaTime );
			dn("s02028 processCmd mesg=[%s] rc=%d done", mesg, rc );
			
			// send endmsg: -111 is X1, when select sent one record data, no more endmsg
			// send endmsg if not X1 ( one line select )
			dn("s3306613 rc=%d req.hasReply=%d redoOnly=%d", rc, req.hasReply, redoOnly );

			if ( -111 != rc && req.hasReply && !redoOnly ) {
				// check timestamp to see if need to update schema for client SC
				dn("s555502 req.session->origserv=%d threadSchemaTime=%ld g_lastSchemaTime=%ld", 
						req.session->origserv, threadSchemaTime, g_lastSchemaTime );

				dn("s560023 pparam.optype=[%c] pparam.opcode=%d", pparam.optype, pparam.opcode );

				// check timestamp to see if need to update host list for client HL
				if ( !req.session->origserv && threadHostTime < g_lastHostTime ) {
                    i("s000236 sendHostInfo to client [%s] ...\n", req.session->ip.s());
					sendHostInfo( "_chost", req, false );
                    i("s000236 sendHostInfo to client [%s] done\n", req.session->ip.s() );
					threadHostTime = g_lastHostTime;
				}
				
				if (  pparam.optype != 'C' ) {
					dn("s30102 not type 'C' optype=%c", pparam.optype);

					if ( reterr.length() > 0 ) {
                        dn("s102029 sendER [%s]", reterr.s() );
						sendER( req, reterr);
					} else {
						if ( pparam.optype == 'R' && JAG_COUNT_OP != pparam.opcode ) {
							// select & getfile  
							dn("s09228 READ op send EOM");
							// Jstr e = Jstr("s3060261 ") + mesg;
							Jstr e = Jstr("s3060261");
							sendEOM( req, e.s() );
						} else {
							dn("s09328 not READ op, no send EOM");
							if ( pparam.opcode == JAG_UPDATE_OP || pparam.opcode == JAG_DELETE_OP ) {
								dn("s332278 JAG_UPDATE_OP JAG_DELETE_OP senEOM");
								sendEOM( req, "s308"); // 1/12/2023 added
							}

							if (  pparam.opcode == JAG_INSERTSELECT_OP ) {
								dn("s332278 JAG_INSERTSELECT_OP JAG_DELETE_OP senEOM");
								sendEOM( req, "s309"); // 1/12/2023 added
							}
						}
					}
				} else if ( pparam.optype == 'C' && req.isCommit() ) {
					dn("s30112 type 'C', commit");
					if ( reterr.length() > 0 ) {
                        dn("s1120087 sendER reterr=%s", reterr.s() );
						sendER( req, reterr);
					} else {
                        // 03/30/2023 added 
                        dn("s112587 sendEOM s35308");
						sendEOM( req, "s35308");
					}
				} else {
					dn("s93002887 here no send to cli");
				}

			} else {
				dn("s3444028 rc=%d -111 ?", rc );
				dn("s3444028 NO sendSchemaMap _cschema nosend to cli");
			}

			dn("s020248 processCmd send response to cli done  mesg=[%s] rc=%d done", mesg, rc );

		} else {
			// parsing got error
			if ( req.hasReply && !redoOnly )  {
				dn("s0022 parsing error %s", reterr.s() );
				sendER( req, reterr);

			} else {
				dn("s566988 here no sendMessageLength no send cli");
			}
		}
	}

        // check timestamp to see if need to update schema for client SC
		/***
        if ( !req.session->origserv && threadSchemaTime < g_lastSchemaTime ) {
                dn("s333309 sendSchemaMap _cschema ...");
                sendSchemaMap( "_cschema", req, false );
                threadSchemaTime = g_lastSchemaTime;
                dn("s333309 sendSchemaMap _cschema done");
        } else {
            dn("s501612 no sendSchemaMap");
        }
		**/


	
	return 1;
}

// Process commands in one thread
jagint JagDBServer::processCmd(  const JagParseAttribute &jpa, JagRequest &req, const char *cmd, 
								 JagParseParam &parseParam, Jstr &reterr, jagint threadQueryTime, 
								 jagint &threadSchemaTime )
{

	reterr = "";
	if ( JAG_SELECT_OP != parseParam.opcode ) {
		_dbLogger->logmsg( req, "DML", cmd );
	}

	bool        rc;	
	int         lockrc;
	jagint      cnt = 0;
	JagCfg      *cfg = _cfg;
	JagTableSchema *tableschema;
	JagIndexSchema *indexschema;

	int         replicType = req.session->replicType;

	getTableIndexSchema( replicType, tableschema, indexschema );

	Jstr    errmsg, dbtable, rowFilter; 
	Jstr    dbName = makeLowerString(req.session->dbname);
	JagIndex *pindex = NULL;
	JagTable *ptab = NULL;

	// change default dbname for single cmd
	dn("s202213 session->dbname=[%s]", dbName.s() );
	if ( parseParam.objectVec.size() == 1 ) { 
		dbName = parseParam.objectVec[0].dbName; 
		dn("s202213 objvec.dbname=[%s]", dbName.s() );
	}

    Jstr  tableName = parseParam.objectVec[0].tableName; // 4/3/2023
    Jstr  indexName = parseParam.objectVec[0].indexName; // 4/3/2023

	req.opcode = parseParam.opcode;

	// methods of frequent use
	if ( JAG_SELECT_OP == parseParam.opcode || JAG_INSERTSELECT_OP == parseParam.opcode 
		 || JAG_GETFILE_OP == parseParam.opcode ) {

		if (  parseParam.isServSelectConst() ) {
            dn("s394001 parseParam.isSelectConst processSelectConstData ...");
			processSelectConstData( req, &parseParam );
			return -111;;
		}

		JagDataAggregate *jda = NULL; 
		int pos = 0;

		dn("s5021601 JAG_SELECT_OP or JAG_GETFILE_OP select starts parseParam=%p ...", &parseParam );

		if ( JAG_INSERTSELECT_OP == parseParam.opcode && parseParam.objectVec.size() > 1 ) {
			// insert into ... select ... from syntax, select part as objectVec[1]
			pos = 1;
		}

		dbName = parseParam.objectVec[pos].dbName; 
        tableName = parseParam.objectVec[pos].tableName; // 4/3/2023

		if ( indexName.length() > 0 ) {
			// known index must be:  "select * from d.table.index where ..."
			dn("s22026 indexName=[%s] dbidx=[%s] readLockIndex...", indexName.s(), dbName.s() );
			pindex = _objectLock->readLockIndex( parseParam.opcode, dbName, tableName, indexName, replicType, false, lockrc );
			dn("s22026 indexName=[%s] dbname=[%s] readLockIndex done pindex=%p lockrc=%d", indexName.s(), dbName.s(), pindex, lockrc );
		} else {
			// table object or index object
			dn("s40012 table object or index object ..");
			dn("s202951 objvec.dbName=[%s] tableName=[%s] readLockTable() pos=%d ...", dbName.s(), tableName.s(), pos );

			ptab = _objectLock->readLockTable( parseParam.opcode, dbName, tableName, replicType, false, lockrc );

			dn("s202951 objvec.dbName=[%s] tableName=[%s] readLockTable() done ptab=%p lockrc=%d", dbName.s(), tableName.s(), ptab, lockrc );

		}
			
		if ( !ptab && !pindex ) {
			if ( parseParam.objectVec[pos].colName.length() > 0 ) {
				reterr = Jstr("E11200 Error: Unable to select ") + 
							parseParam.objectVec[pos].colName + "." + tableName +
							" session.dbname=[" + req.session->dbname + "] table and index object not found "; 
			} else {
				reterr = Jstr("E11201 Error: Unable to select from ") + dbName + "." + tableName;
			}

			if ( parseParam.objectVec[pos].indexName.size() > 0 ) {
				reterr += Jstr(".") + parseParam.objectVec[pos].indexName;
			}

			dn("s5021601 JAG_SELECT_OP select error stop");
			return 0;
		}
		
		if ( ptab ) {
			// table to select ...
			rc = checkUserCommandPermission( &ptab->_tableRecord, req, parseParam, 0, rowFilter, errmsg );
			if ( rc ) {
				if ( rowFilter.size() > 0 ) {
					parseParam.resetSelectWhere( rowFilter );
					parseParam.setSelectWhere();
					Jstr newcmd = parseParam.formSelectSQL();
					cmd = newcmd.c_str();
				} 

				if ( parseParam.exportType == JAG_EXPORT ) {
					Jstr dbtab = dbName + "." + tableName;
					Jstr dirpath = jaguarHome() + "/export/" + dbtab;
					JagFileMgr::rmdir( dirpath );
					jd(JAG_LOG_LOW, "s22028 rmdir %s\n", dirpath.s() );
				}

				// select data
				dn("s333810 ptab->select() and getfile ...");
				cnt = ptab->select( jda, cmd, req, &parseParam, errmsg, true, pos );
				// export is processed in select
                dn("s538113 ptab->select( cmd=%s) cnt=%d", cmd, cnt );

			} else {
				cnt = -1;
			}

            dn("s7020301 after table select readUnlockTable pos=%d replicType=%d", pos, replicType );
			_objectLock->readUnlockTable( parseParam.opcode, dbName, tableName, replicType, false );

		} else if ( pindex ) {
			rc = checkUserCommandPermission( &pindex->_indexRecord, req, parseParam, 0, rowFilter, errmsg );

			if ( rc ) {
				cnt = pindex->select( jda, cmd, req, &parseParam, errmsg, true, pos );
			} else {
				cnt = -1;
			}

			_objectLock->readUnlockIndex( parseParam.opcode, dbName, tableName, indexName, replicType, 0 );
		}

		if ( cnt == -1 ) {
			reterr = errmsg;
		}

		if ( -111 == cnt || cnt > 0 ) {
			++ numSelects;
		}

		//jd(JAG_LOG_LOW, "s2239 cnt=%d req.session->sessionBroken=%d\n", cnt, (int)req.session->sessionBroken );
		jagint  dcnt = 0;
		if (  cnt > 0 && !req.session->sessionBroken ) {
			if ( jda ) {
				dn("s501234 jda->sendDataToClient() cnt=%lld _datalen=%ld _keylen=%d _vallen=%d", cnt, jda->_datalen, jda->_keylen, jda->_vallen);
				dcnt = jda->sendDataToClient( cnt, req );
				dn("s501234 jda->sendDataToClient() done dcnt=%lld/%lld", dcnt, cnt);
			} else {
				dn("s78012 cnt=%lld but no jda object", cnt);
			}
		} else {
			dn("s541028 trying to sendDataToClient but cnt=%lld or session is broken", cnt );
		}

        dn("s33093005 parseParam._lineFile=%p", parseParam._lineFile ); 

		if ( !req.session->sessionBroken && parseParam._lineFile && parseParam._lineFile->size() > 0 ) {

            dn("s58003002 sendValueData ...");
			sendValueData( parseParam, req  );

		}  else {
            dn("s56003118 no sendValueData");
        }

		// if ( parseParam.exportType == JAG_EXPORT ) req.syncDataCenter = true;
		if ( jda ) { 
			delete jda; 
			jda = NULL; 
		}
		
		dn("s5021605 JAG_SELECT_OP select is done record_cnt=%lld send_cnt=%lld", cnt, dcnt);

	} else if ( JAG_UPDATE_OP == parseParam.opcode ) {
		rc = checkUserCommandPermission( NULL, req, parseParam, 0, rowFilter, reterr );
		if ( rc ) {
            dn("s35607701 writeLockTable table=%s replicType=%d ...", tableName.s(), replicType );
			ptab = _objectLock->writeLockTable( parseParam.opcode, dbName, tableName,
												tableschema, replicType, false, lockrc );
			if ( !ptab ) {
				reterr = "E42831 Error: Update can only be applied to tables";
			} else {
				Jstr dbobj = dbName + "." + tableName;
				cnt = ptab->update( req, &parseParam, false, errmsg );

				_objectLock->writeUnlockTable( parseParam.opcode, dbName, tableName, replicType, false );
			}
	
   			if ( cnt < 0 ) {
   				reterr = Jstr("E13209 Error: server update error: ") + errmsg;
   				_dbLogger->logerr( req, reterr, cmd );
   			} else {
   				reterr= "";
   				++ numUpdates;
   			}

			cnt = 100;  // temp fix
			//req.syncDataCenter = true;
		} else {
			// no permission for update
		}
	} else if ( JAG_DELETE_OP == parseParam.opcode ) {
		rc = checkUserCommandPermission( NULL, req, parseParam, 0, rowFilter, reterr );
		if ( rc ) {
			ptab = _objectLock->writeLockTable( parseParam.opcode, dbName, tableName,
												tableschema, replicType, 0, lockrc );
			if ( ptab ) {
                Jstr dbobj = dbName + "." + tableName;

                cnt = ptab->remove( req, &parseParam, errmsg );

				jagint trimLimit = JAG_WALLOG_TRIM_RATIO * (JAG_SIMPFILE_LIMIT_BYTES/ptab->_KEYVALLEN );
				if ( cnt > trimLimit ) {
					dn("s20091 trimWalLogFile tableName=[%s] ...", tableName.s() );
					trimWalLogFile( ptab, dbName, tableName, 
								    ptab->_darrFamily->_insertBufferMap, ptab->_darrFamily->_keyChecker );
					dn("s20091 trimWalLogFile tableName=[%s] done", tableName.s() );
				}

                _objectLock->writeUnlockTable( parseParam.opcode, dbName, tableName, replicType, 0 );
				dn("s20191 writeUnlockTable done");
			} else {
    			reterr = "E10238 Error: Delete can only be applied to tables";
				dn("s22365 JAG_DELETE_OP writeLockTable failed lockrc=%d", lockrc );
			}

    		if ( cnt < 0 ) {
    			reterr = Jstr("E37703 Error: server delete error: ") +  errmsg;
    			_dbLogger->logerr( req, reterr, cmd );
    		} else {
    			reterr= "";
    			++ numDeletes;
    		}
			cnt = 100;  // temp fix
			//req.syncDataCenter = true;
		} else {
			// no permission for delete
			cnt = 100;  // temp fix
		}
	} else if ( JAG_COUNT_OP == parseParam.opcode ) {
        dn("s2008131 JAG_COUNT_OP");
		if ( indexName.length() > 0 ) {
			// known index
			pindex = _objectLock->readLockIndex( parseParam.opcode, dbName, tableName, indexName, replicType, false, lockrc );
		} else {
			// table object or index object
            dn("s20287029 JAG_COUNT_OP dbname=[%s] tableName=[%s] replicType=%d opcode=%d", dbName.s(), tableName.s(), replicType, parseParam.opcode );
			ptab = _objectLock->readLockTable( parseParam.opcode, dbName, tableName, replicType, false, lockrc );

            dn("s20287029 JAG_COUNT_OP dbname=[%s] tableName=[%s] replicType=%d opcode=%d retrieved ptab=%p", 
               dbName.s(), tableName.s(), replicType, parseParam.opcode, ptab );
		}
			
		if ( !ptab && !pindex ) {
				if ( parseParam.objectVec[0].colName.length() > 0 ) {
					reterr = Jstr("E1023 Error: Unable to select count(*) for ") + 
							 parseParam.objectVec[0].colName + "." + tableName;
				} else {
					reterr = Jstr("E1024 Error: Unable to select count(*) for ") + dbName + "." + tableName;
				}

				if ( indexName.size() > 0 ) {
					reterr += Jstr(".") + indexName;
				}
				return 0;
		}
		
		if ( ptab ) {
				rc = checkUserCommandPermission( &ptab->_tableRecord, req, parseParam, 0, rowFilter, errmsg );
				if ( rc ) {
                    dn("s30018 ptab->getCount() ptab=%p ", ptab);
					cnt = ptab->getCount( cmd, req, &parseParam, errmsg );
                    dn("s30018 ptab->getCount() ptab=%p cnt=%ld", cnt, ptab);
				} else {
					cnt = -1;
				}
				_objectLock->readUnlockTable( parseParam.opcode, dbName, tableName, replicType, 0 );
		} else if ( pindex ) {
				rc = checkUserCommandPermission( &pindex->_indexRecord, req, parseParam, 0, rowFilter, errmsg );
				if ( rc ) {
					cnt = pindex->getCount( cmd, req, &parseParam, errmsg );
				} else {
					cnt = -1;
				}
				_objectLock->readUnlockIndex( parseParam.opcode, dbName, tableName, indexName, replicType, false );
		}

		if ( cnt == -1 ) {
			reterr = errmsg;
		} else {
			char cntbuf[30];
			memset( cntbuf, 0, 30 );
			sprintf( cntbuf, "%lld", cnt );
			sendDataEnd( req, cntbuf );
		}
		
		if ( cnt >= 0 ) {
			dn("s222038 selectcount reptype=%d %s cnt=%lld ", replicType, parseParam.objectVec[0].tableName.c_str(), cnt);
		}
   	} else if ( JAG_IMPORT_OP == parseParam.opcode ) {
		importTable( req, dbName, &parseParam, reterr );
	} else if ( JAG_CREATEUSER_OP == parseParam.opcode ) {
		createUser( req, parseParam, threadQueryTime );
	} else if ( JAG_DROPUSER_OP == parseParam.opcode ) {
		dropUser( req, parseParam, threadQueryTime );
	} else if ( JAG_CHANGEPASS_OP == parseParam.opcode ) {
		changePass( req, parseParam, threadQueryTime );		
	} else if ( JAG_CHANGEDB_OP == parseParam.opcode ) {
		changeDB( req, parseParam, threadQueryTime );
	} else if ( JAG_CREATEDB_OP == parseParam.opcode ) {
		createDB( req, parseParam, threadQueryTime );
	} else if ( JAG_DROPDB_OP == parseParam.opcode ) {
		dropDB( req, parseParam, threadQueryTime );
	} else if ( JAG_CREATETABLE_OP == parseParam.opcode ) {
		d("s39393011 JAG_CREATETABLE_OP isPrepare=%d...\n", req.isPrepare() );
		rc = checkUserCommandPermission( NULL, req, parseParam, 0, rowFilter, reterr );
		if ( rc ) {
			rc = createTable( req, dbName, &parseParam, reterr, threadQueryTime );
			if ( rc && parseParam.timeSeries.size() > 0 && req.isCommit() ) {
				Jstr dbtable = dbName + "." + tableName;
				createTimeSeriesTables( req, parseParam.timeSeries, dbName, dbtable, jpa, reterr );
			}
		} else {
			dn("s408820 checkUserCommandPermission failed NG");
			noGood( req, parseParam );
		}
	} else if ( JAG_CREATECHAIN_OP == parseParam.opcode ) {
		rc = checkUserCommandPermission( NULL, req, parseParam, 0, rowFilter, reterr );
		if ( rc ) {
			rc = createTable( req, dbName, &parseParam, reterr, threadQueryTime );
			if ( rc && parseParam.timeSeries.size() > 0 ) {
				Jstr dbtable = dbName + "." + parseParam.objectVec[0].tableName;
				createTimeSeriesTables( req, parseParam.timeSeries, dbName, dbtable, jpa, reterr );
			}
		} else {
			noGood( req, parseParam );
		}
	} else if ( JAG_CREATEMEMTABLE_OP == parseParam.opcode ) {
		rc = checkUserCommandPermission( NULL, req, parseParam, 0, rowFilter, reterr );
		if ( rc ) {
			rc = createMemTable( req, dbName, &parseParam, reterr, threadQueryTime );
		} else {
			noGood( req, parseParam );
		}
	} else if ( JAG_CREATEINDEX_OP == parseParam.opcode ) {
		rc = checkUserCommandPermission( NULL, req, parseParam, 0, rowFilter, reterr );
		if ( rc ) { 
			dn("s5555088 createIndex() isprepare=%d ...", req.isPrepare() );
			rc = createIndex( req, dbName, &parseParam, ptab, pindex, reterr, threadQueryTime );
			dn("s5555089 createIndex() done isPrepare=%d rc=%d", req.isPrepare(), rc);
			Jstr tSer;
			if ( rc && req.isCommit() && ptab->hasTimeSeries( tSer ) && parseParam.value=="ticks" ) {
				d("s440242 createTimeSeriesIndexes");
				createTimeSeriesIndexes( jpa, req, parseParam, tSer, reterr );
			}
		} else {
			dn("s450420182 checkUserCommandPermission false, NG to client");
			noGood( req, parseParam ); 
		}
	} else if ( JAG_ALTER_OP == parseParam.opcode ) {
		dn("s4440001 JAG_ALTER_OP isprepare=%d", req.isPrepare() );
		rc = checkUserCommandPermission( NULL, req, parseParam, 0, rowFilter, reterr );
		if ( rc ) {
			dn("s4440001 alterTable isprepare=%d", req.isPrepare() );
			rc = alterTable( jpa, req, dbName, &parseParam, reterr, threadQueryTime, threadSchemaTime );	
			dn("s4440001 alterTable rc=%d isPrepare=%d", rc, req.isPrepare());
		} else {
			dn("s4440001 checkUserCommandPermission nogood");
			noGood( req, parseParam );
		}
   	} else if ( JAG_DROPTABLE_OP == parseParam.opcode ) {
		rc = checkUserCommandPermission( NULL, req, parseParam, 0, rowFilter, reterr );
		if ( rc ) {
			Jstr timeSeries;
			rc = dropTable( req, &parseParam, reterr, threadQueryTime, timeSeries );
			jd( JAG_LOG_LOW, "JAG_DROPTABLE req.isPrepare=%d dropTable.rc=%d timeSeries=[%s]\n",
				req.isPrepare(), rc, timeSeries.s() );

			if ( rc && timeSeries.size() > 0 && !req.isPrepare() ) {
				Jstr dbtable = dbName + "." + parseParam.objectVec[0].tableName;
				dropTimeSeriesTables( req, timeSeries, dbName, dbtable, jpa, reterr );
			}
		} else noGood( req, parseParam );
   	} else if ( JAG_DROPINDEX_OP == parseParam.opcode ) {
		rc = checkUserCommandPermission( NULL, req, parseParam, 0, rowFilter, reterr );
		if ( rc ) {
			Jstr timeSeries;
			jd( JAG_LOG_LOW, "JAG_DROPINDEX [%s] req.isPrepare=%d ...\n", 
				parseParam.objectVec[1].indexName.s(), req.isPrepare() );
			rc = dropIndex( req, dbName, &parseParam, reterr, threadQueryTime, timeSeries );
			jd( JAG_LOG_LOW, "JAG_DROPINDEX [%s] req.isPrepare=%d dropIndex.rc=%d done timeSeries=[%s]\n",
				parseParam.objectVec[1].indexName.s(), req.isPrepare(), rc, timeSeries.s() );

			if ( rc && timeSeries.size() > 0 && !req.isPrepare() ) {
				jd( JAG_LOG_LOW, "JAG_DROPINDEX req.isPrepare=%d dropTimeSeriesIndexes()...\n", req.isPrepare() );
				dropTimeSeriesIndexes( req, jpa, parseParam.objectVec[0].tableName, parseParam.objectVec[1].indexName, timeSeries );
			}
		} else {
			dn("s4440001 checkUserCommandPermission nogood");
			noGood( req, parseParam );
        }
   	} else if ( JAG_TRUNCATE_OP == parseParam.opcode ) {
		rc = checkUserCommandPermission( NULL, req, parseParam, 0, rowFilter, reterr );
		if ( rc ) rc = truncateTable( req, &parseParam, reterr, threadQueryTime );
		else noGood( req, parseParam );
   	} else if ( JAG_GRANT_OP == parseParam.opcode ) {
		grantPerm( req, parseParam, threadQueryTime );
   	} else if ( JAG_REVOKE_OP == parseParam.opcode ) {
		revokePerm( req, parseParam, threadQueryTime );
   	} else if ( JAG_SHOWGRANT_OP == parseParam.opcode ) {
		showPerm( req, parseParam, threadQueryTime );
	} else if ( JAG_DESCRIBE_OP == parseParam.opcode ) {
		if ( parseParam.objectVec[0].indexName.length() > 0 ) {
			Jstr res = describeIndex( parseParam.detail, req, indexschema, parseParam.objectVec[0].dbName, 
									  parseParam.objectVec[0].indexName, reterr, false, false, "" );
			if ( res.size() > 0 ) { 
				sendDataEnd( req, res);
			} 
		} else {
			Jstr dbtable = parseParam.objectVec[0].dbName + "." + parseParam.objectVec[0].tableName;
			Jstr res;
			if ( tableschema->existAttr ( dbtable ) ) {
				res = describeTable( JAG_ANY_TYPE, req, tableschema, dbtable, parseParam.detail, false, false, "" );
			} else {
				if ( parseParam.objectVec[0].colName.length() > 0 ) {
					res = describeIndex( parseParam.detail, req, indexschema, parseParam.objectVec[0].colName, 
							   			 parseParam.objectVec[0].tableName, reterr, false, false, "" );
				} else {
					res = describeIndex( parseParam.detail, req, indexschema, parseParam.objectVec[0].dbName, 
								   		 parseParam.objectVec[0].tableName, reterr, false, false, "" );
			    }
			}

			if ( res.size() > 0 ) { 
				dn("s304000 describeTable send [%s]", res.s());
				sendDataEnd( req, res );
			} else {
				dn("s304000 describeTable no send");
			}
		}
	} else if ( JAG_SHOW_CREATE_TABLE_OP == parseParam.opcode ) {
		Jstr dbtable = dbName + "." + parseParam.objectVec[0].tableName;
		if ( tableschema->existAttr ( dbtable ) ) {
			Jstr res = describeTable( JAG_TABLE_TYPE, req, tableschema, dbtable, parseParam.detail, true, false, "" );
			if ( res.size() > 0 ) { 
				sendDataEnd( req, res );
			} else { reterr = "E20141 Error: Table " + dbtable + " empty"; }
		} else {
			reterr = "E20011 Error: Table " + dbtable + " not found";
		}
	} else if ( JAG_SHOW_CREATE_CHAIN_OP == parseParam.opcode ) {
		Jstr dbtable = dbName + "." + parseParam.objectVec[0].tableName;
		if ( tableschema->existAttr ( dbtable ) ) {
			Jstr res = describeTable( JAG_CHAINTABLE_TYPE, req, tableschema, dbtable, parseParam.detail, true, false, "" );
			if ( res.size() > 0 ) { 
				sendDataEnd( req, res );
			} else { reterr = "Chain " + dbtable + " error "; }
		} else {
			reterr = "Chain " + dbtable + " not found";
		}
	} else if ( JAG_EXEC_DESC_OP == parseParam.opcode ) {
		Jstr dbtable = dbName + "." + parseParam.objectVec[0].tableName;
		if ( tableschema->existAttr ( dbtable ) ) {
			_describeTable( req, dbtable, 0 );
		} else {
			reterr = "E20104 Error: Table " + dbtable + " not found";
		}
	} else if ( JAG_SHOWUSER_OP == parseParam.opcode ) {
		showUsers( req );
	} else if ( JAG_SHOWDB_OP == parseParam.opcode ) {
		showDatabases( cfg, req );
	} else if ( JAG_CURRENTDB_OP == parseParam.opcode ) {
		showCurrentDatabase( cfg, req, dbName );
	} else if ( JAG_SHOWSTATUS_OP == parseParam.opcode ) {
		showClusterStatus( req );
	/**
	} else if ( JAG_SHOWDATACENTER_OP == parseParam.opcode ) {
		showDatacenter( req );
	***/
	} else if ( JAG_SHOWTOOLS_OP == parseParam.opcode ) {
		showTools( req );
	} else if ( JAG_CURRENTUSER_OP == parseParam.opcode ) {
		showCurrentUser( cfg, req );
	} else if ( JAG_SHOWTABLE_OP == parseParam.opcode ) {
		showTables( req, parseParam, tableschema, dbName, JAG_TABLE_TYPE );
	} else if ( JAG_SHOWCHAIN_OP == parseParam.opcode ) {
		showTables( req, parseParam, tableschema, dbName, JAG_CHAINTABLE_TYPE );
	} else if ( JAG_SHOWINDEX_OP == parseParam.opcode ) {
		if ( parseParam.objectVec.size() < 1 ) {
			showAllIndexes( req, parseParam, indexschema, dbName );
		} else {
			Jstr dbtable = dbName + "." + parseParam.objectVec[0].tableName;
			showIndexes( req, parseParam, indexschema, dbtable );
		}
	} else if ( JAG_SHOWTASK_OP == parseParam.opcode ) {
		showTask( req );
	} else if ( JAG_EXEC_SHOWDB_OP == parseParam.opcode ) {
		_showDatabases( cfg, req );
	} else if ( JAG_EXEC_SHOWTABLE_OP == parseParam.opcode ) {
		_showTables( req, tableschema, dbName );
	} else if ( JAG_SHOWCLUSTER_OP == parseParam.opcode ) {
		showCluster( cfg, req );
	} else if ( JAG_EXEC_SHOWINDEX_OP == parseParam.opcode ) {
		Jstr dbtable = dbName + "." + parseParam.objectVec[0].tableName;
		_showIndexes( req, indexschema, dbtable );
	}

	dn("s344008 parseParam.clearRowHash...");
	parseParam.clearRowHash();
	dn("s55550 processCmd done  cnt=%d", cnt );
	return cnt;
}


#ifndef SIGHUP
#define SIGHUP 1
#endif
void JagDBServer::sig_hup(int sig)
{
	// printf("sig_hup called, should reread cfg...\n");
	// jd(JAG_LOG_LOW, "Server received SIGHUP, cfg and userdb refresh ...\n");
	// JAG_BLURT jaguar_mutex_lock ( &g_dbschemamutex ); JAG_OVER
	g_receivedSignal = SIGHUP;
	/***
	if ( _cfg ) {
		_cfg->refresh();
	}

	if ( _userDB ) {
		_userDB->refresh();
	}
	if ( _prevuserDB ) {
		_prevuserDB->refresh();
	}
	if ( _nextuserDB ) {
		_nextuserDB->refresh();
	}
	***/
	// jd(JAG_LOG_LOW, "Server received SIGHUP, cfg/userDB refresh done\n");
	jd(JAG_LOG_LOW, "Server received SIGHUP, ignored.\n");
	// jaguar_mutex_unlock ( &g_dbschemamutex );
}

// method to check non standard command is valid or not
// may append more commands later
// return 0: for error
int JagDBServer::isValidInternalCommand( const char *mesg )
{	
	if ( 0 == strncmp( mesg, "_noop", 5 ) ) return JAG_SCMD_NOOP;
	else if ( 0 == strncmp( mesg, "_cschema_more", 13 ) ) return JAG_SCMD_CSCHEMA_MORE;
	else if ( 0 == strncmp( mesg, "_cschema", 8 ) ) return JAG_SCMD_CSCHEMA;
	//else if ( 0 == strncmp( mesg, "_cdefval", 8 ) ) return JAG_SCMD_CDEFVAL;
	else if ( 0 == strncmp( mesg, "_chost", 6 ) ) return JAG_SCMD_CHOST;
	else if ( 0 == strncmp( mesg, "_serv_crecover", 14 ) ) return JAG_SCMD_CRECOVER;
	else if ( 0 == strncmp( mesg, "_serv_checkdelta", 16 ) ) return JAG_SCMD_CHECKDELTA;
	else if ( 0 == strncmp( mesg, "_serv_beginfxfer", 16) ) return JAG_SCMD_BFILETRANSFER;
	else if ( 0 == strncmp( mesg, "_serv_addbeginfxfer", 19) ) return JAG_SCMD_ABFILETRANSFER;
	else if ( 0 == strncmp( mesg, "_serv_opinfo", 12 ) ) return JAG_SCMD_OPINFO;
	else if ( 0 == strncmp( mesg, "_serv_copydata", 14 ) ) return JAG_SCMD_COPYDATA;
	else if ( 0 == strncmp( mesg, "_serv_dolocalbackup", 19 ) ) return JAG_SCMD_DOLOCALBACKUP;
	else if ( 0 == strncmp( mesg, "_serv_doremotebackup", 20 ) ) return JAG_SCMD_DOREMOTEBACKUP;
	else if ( 0 == strncmp( mesg, "_serv_dorestoreremote", 21 ) ) return JAG_SCMD_DORESTOREREMOTE;
	else if ( 0 == strncmp( mesg, "_serv_refreshacl", 14 ) ) return JAG_SCMD_REFRESHACL;
	//else if ( 0 == strncmp( mesg, "_serv_reqschemafromdc", 21 ) ) return JAG_SCMD_REQSCHEMAFROMDC;
	else if ( 0 == strncmp( mesg, "_serv_unpackschinfo", 19 ) ) return JAG_SCMD_UNPACKSCHINFO;
	else if ( 0 == strncmp( mesg, "_serv_askdatafromdc", 19 ) ) return JAG_SCMD_ASKDATAFROMDC;
	else if ( 0 == strncmp( mesg, "_serv_preparedatafromdc", 23 ) ) return JAG_SCMD_PREPAREDATAFROMDC;
	else if ( 0 == strncmp( mesg, "_mon_dbtab", 10 ) ) return JAG_SCMD_MONDBTAB;
	else if ( 0 == strncmp( mesg, "_mon_info", 9 ) ) return JAG_SCMD_MONINFO;
	else if ( 0 == strncmp( mesg, "_mon_rsinfo", 11 ) ) return JAG_SCMD_MONRSINFO;
	else if ( 0 == strncmp( mesg, "_mon_clusteropinfo", 18 ) ) return JAG_SCMD_MONCLUSTERINFO;
	else if ( 0 == strncmp( mesg, "_mon_hosts", 10 ) ) return JAG_SCMD_MONHOSTS;
	else if ( 0 == strncmp( mesg, "_mon_remote_backuphosts", 23 ) ) return JAG_SCMD_MONBACKUPHOSTS;
	else if ( 0 == strncmp( mesg, "_mon_local_stat6", 16 ) ) return JAG_SCMD_MONLOCALSTAT;
	else if ( 0 == strncmp( mesg, "_mon_cluster_stat6", 18 ) ) return JAG_SCMD_MONCLUSTERSTAT;
	else if ( 0 == strncmp( mesg, "_ex_proclocalbackup", 19 ) ) return JAG_SCMD_EXPROCLOCALBACKUP;
	else if ( 0 == strncmp( mesg, "_ex_procremotebackup", 20 ) ) return JAG_SCMD_EXPROCREMOTEBACKUP;
	else if ( 0 == strncmp( mesg, "_ex_restorefromremote", 21 ) ) return JAG_SCMD_EXRESTOREFROMREMOTE;
	else if ( 0 == strncmp( mesg, "_ex_addclust_migrate", 20 ) ) return JAG_SCMD_EXADDCLUSTER_MIGRATE;
	else if ( 0 == strncmp( mesg, "_ex_addclust_migrcontinue", 25 ) ) return JAG_SCMD_EXADDCLUSTER_MIGRATE_CONTINUE;
	else if ( 0 == strncmp( mesg, "_ex_addclustr_mig_complete", 26 ) ) return JAG_SCMD_EXADDCLUSTER_MIGRATE_COMPLETE;
	else if ( 0 == strncmp( mesg, "_ex_addcluster", 14 ) ) return JAG_SCMD_EXADDCLUSTER;
	else if ( 0 == strncmp( mesg, "_ex_importtable", 15 ) ) return JAG_SCMD_IMPORTTABLE;
	else if ( 0 == strncmp( mesg, "_ex_truncatetable", 17 ) ) return JAG_SCMD_TRUNCATETABLE;
	else if ( 0 == strncmp( mesg, "_exe_shutdown", 13 ) ) return JAG_SCMD_EXSHUTDOWN;
	else if ( 0 == strncmp( mesg, "_getpubkey", 10 ) ) return JAG_SCMD_GETPUBKEY;
	else if ( 0 == strncmp( mesg, "_chkkey", 7 ) ) return JAG_SCMD_CHKKEY;
	else if ( 0 == strncmp( mesg, "_onefile", 8 ) ) return JAG_SCMD_RECVFILE;
	else return 0;
}

// method to check is simple command or not
// may append more commands later
// returns 0 for false (not simple commands)
int JagDBServer::isSimpleCommand( const char *mesg )
{	
	if ( 0 == strncmp( mesg, "help", 4 )  ) return JAG_RCMD_HELP;
	else if ( 0 == strncmp( mesg, "hello", 5 ) ) return JAG_RCMD_HELLO;
	else if ( 0 == strncmp( mesg, "use ", 4 ) ) return JAG_RCMD_USE;
	else if ( 0 == strncmp( mesg, "auth", 4 ) ) return JAG_RCMD_AUTH;
	else if ( 0 == strncmp( mesg, "quit", 4 ) ) return JAG_RCMD_QUIT;
	else return 0;
}

// static Task for a client/thread
void *JagDBServer::oneClientThreadTask( void *passptr )
{
	pthread_detach ( pthread_self() );
 	JagPass *ptr = (JagPass*)passptr;
 	JAGSOCK sock = ptr->sock;

 	int 	rc, simplerc;
 	char  	*pmesg;
	jagint 	threadSchemaTime = 0, threadHostTime = g_lastHostTime, threadQueryTime = 0;
	jagint  len, cnt = 1;
 	int     authed = 0;
    Jstr    lastMsg;
 
	JagSession session;
	session.sock = sock;
	session.servobj = (JagDBServer*)ptr->servobj;
	session.ip = ptr->ip;
	session.serverIP = session.servobj->_localInternalIP;
	session.port = ptr->port;
	session.active = 0;

	jd(JAG_LOG_HIGH, "Client IP:Port %s:%u replicType=%d\n", session.ip.c_str(), session.port, session.replicType );

	JagDBServer  *servobj = (JagDBServer*)ptr->servobj;
	++ servobj->_activeClients; 

	char   *sbuf = (char*)jagmalloc(SERVER_SOCKET_BUFFER_BYTES+1);
	char 	rephdr[4]; rephdr[3] = '\0';
	int 	hdrsz = JAG_SOCK_TOTAL_HDR_LEN;
 	char 	hdr[hdrsz+1];
	char 	*newbuf = NULL;
	char 	sqlhdr[JAG_SOCK_SQL_HDR_LEN+1];
	char 	h1, h2;
	struct  timeval now;

    for(;;)
    {
		if ( ( cnt % 100000 ) == 0 ) { jagmalloc_trim( 0 ); }

		JagRequest req;
		req.session = &session;

		session.active = 0;
		sbuf[0] = '\0';
		dn(" ");
		dn(" ");
		dn("s303388 thrd loop: recvMessageInBuf client is [%s:%u] thrd=%lu session.replicType=%d ...", session.ip.c_str(), session.port, THID, session.replicType );
		len = recvMessageInBuf( sock, hdr, newbuf, sbuf, SERVER_SOCKET_BUFFER_BYTES );
        if ( newbuf ) {
		    dn("s303388 recvMessageInBuf got newbuf=[%s] len=%d thrd=%lu ...", newbuf, len, THID);
        } else {
		    dn("s303388 recvMessageInBuf got sbuf=[%s] len=%d thrd=%lu ...", sbuf, len, THID);
        }
		dn("s303388 recvMessageInBuf session.origserv=%d session.replicType=%d", session.origserv, session.replicType );
		session.active = 1;

		if ( len <= 0 ) {
			if ( session.uid == "admin" ) {
				if ( session.exclusiveLogin ) {
					servobj->_exclusiveAdmins = 0;
					jd(JAG_LOG_LOW, "Exclusive admin disconnected from %s\n", session.ip.c_str() );
				}
			}

			// disconnecting ...
			if ( newbuf ) { free( newbuf ); }
			//if ( newbuf2 ) { free( newbuf2 ); }
			-- servobj->_connections;

			if ( ! session.origserv ) {
				servobj->_clientSocketMap.removeKey( sock );
			}

			dn("s562208 len=%d <= 0 break client connection", len );
			break;
		}

		++cnt;
		getXmitSQLHdr( hdr, sqlhdr );
		dn("s33383 getXmitSQLHdr hdr=[%s]", hdr );
		dn("202838 getXmitSQLHdr sqlhdr=[%s]", sqlhdr );
		req.sqlhdr = sqlhdr;

		gettimeofday( &now, NULL );
		threadQueryTime = now.tv_sec * (jagint)1000000 + now.tv_usec;

		h1 = hdr[hdrsz-3];
		h2 = hdr[hdrsz-2];

		// if recv heartbeat from client, ignore
		if ( h1  == 'H' && h2 == 'B' ) {
			continue;
		}

		if ( h1 == 'N' ) {
			req.hasReply = false;
			req.batchReply = false;
			dn("s4440481 h1 == N  req.hasReply = false");
		} else if ( h1 == 'B'  ) {
			req.hasReply = true;
			req.batchReply = true;
			dn("s4440481 h1=B req.hasReply = true", h1);
		} else if ( h1 == 'C' ) {
			req.hasReply = true;
			req.batchReply = false;
			dn("s4440481 h1=C req.hasReply = true", h1);
		} else {
			req.hasReply = true;
			req.batchReply = false;
			dn("s4440481 h1 != B/C/N h1=[%c]  req.hasReply = true", h1);
		}

		if (req.isPrepare() ) {
			req.hasReply = true;
			req.batchReply = false;
		} 

		if ( h2 == 'Z' ) {
			req.doCompress = true;
		} else {
			req.doCompress = false;
		}

		req.dorep = false;
		if ( newbuf ) { 
			pmesg = newbuf; 
		} else { 
			pmesg = sbuf; 
		}

		Jstr us;
		if ( req.doCompress ) {
			if ( newbuf ) {
				JagFastCompress::uncompress( newbuf, len, us );
			} else {
				JagFastCompress::uncompress( sbuf, len, us );
			}
			pmesg = (char*)us.c_str();
			len = us.length();
		}
	
		while ( *pmesg == ' ' || *pmesg == '\t' ) ++pmesg;

		dn("s3033822 recved pmesg=[%s] authed=%d", pmesg, authed );
		
		if ( *pmesg == '_' && 0 != strncmp( pmesg, "_show", 5 ) 
			  && 0 != strncmp(pmesg, "_desc", 5)
			  && 0 != strncmp(pmesg, "_send", 5) 
			  && 0 != strncmp(pmesg, "_disc", 5) ) {

			rc = isValidInternalCommand( pmesg ); // "_???" commands

			if ( !rc ) {
				Jstr errmsg = Jstr("E200238 Error non standard server command]") + "[" + pmesg + "]";
				sendER( req, errmsg);
			} else {
				if ( ! authed && JAG_SCMD_GETPUBKEY != rc ) {
					sendER( req, "E400280 Not authed before query");
					break;
				}
				servobj->processInternalCommands( rc, req, pmesg );
			}

			continue;
		}

		// help help auth use etc
		simplerc = isSimpleCommand( pmesg );
		dn("s457083 isSimpleCommand simplerc=%d req.session->dbname=[%s]", simplerc, req.session->dbname.s() );
		if ( simplerc > 0 ) {
			int prc = servobj->processSimpleCommand( simplerc, req, pmesg, authed );
			if ( prc < 0 ) {
				dn("s3330380 processSimpleCommand prc < 0 break");
				break;
			} else {
				dn("s60211 OK after processSimpleCommand prc=%d thrd=%lu  continue, recv next message", prc, THID );
				continue;
			}
		}

		if ( 0 == strcmp(pmesg, "YYY") || 0 == strcmp(sqlhdr, "SIG") ) {
			dn("s5781038 YYY or SIG skip");
			continue;
		}
		
		if ( req.session->dbname.size() < 1 ) {
			bool ok = false;
			if ( 0 == strncasecmp( pmesg, "show", 4 ) ) {
				JagStrSplit sp(pmesg, ' ', true );
				if ( sp.size() >=2 && sp[1].caseEqual("database") ) {
					ok = true;
				}
			}
			
			if ( ! ok ) {
				dn("s330401 No database selected");
				sendER( req, "E21001 No database selected");
				break;
			}
		}

		if ( 1 == session.drecoverConn ) {
			rePositionMessage( req, pmesg, len );
		}
		
		int isReadOrWriteCommand = checkReadOrWriteCommand( pmesg );
        /*** 3/26/2023
		if ( isReadOrWriteCommand == JAG_WRITE_SQL ) {
            dn("s34008 _restartRecover=%d", servobj->_restartRecover );
			if ( servobj->_restartRecover ) {
				jaguar_mutex_lock ( &g_flagmutex ); JAG_OVER

                dn("s300201 handleRestartRecover pmesg=[%s] ...", pmesg);
				int recovrc = servobj->handleRestartRecover( req, pmesg, len, hdr2, newbuf, newbuf2 );

				if ( 0 == recovrc ) {
					dn("s1112080  0 == recovrc skip");
					continue;
				} else {
					dn("s1112082  0 != recovrc break");
					break;
				}
			} 
		}
        ***/

		dn("s422380 isReadOrWriteCommand=%d", isReadOrWriteCommand);

		// add tasks	
		jaguint taskID;
		++ ( servobj->_taskID );
		taskID =  servobj->_taskID;
		addTask( taskID, req.session, pmesg );

		dn("s3330012 processMultiSingleCmd()...");

		try {
			// handles message from clients and may send reply back to client
			servobj->processMultiSingleCmd( req, pmesg, len, threadSchemaTime, threadHostTime, 
								   		   threadQueryTime, false, isReadOrWriteCommand, lastMsg );
		} catch ( const char *e ) {
			jd(JAG_LOG_LOW, "processMultiSingleCmd [%s] caught exception [%s]\n", pmesg, e );
		} catch ( ... ) {
			jd(JAG_LOG_LOW, "processMultiSingleCmd [%s] caught unknown exception\n", pmesg );
		}

		servobj->_taskMap->removeKey( taskID );
        lastMsg = pmesg;

        // after processMultiSingleCmd
        /***
		if ( servobj->_faultToleranceCopy > 1 
		     && isReadOrWriteCommand == JAG_WRITE_SQL 
			 && session.drecoverConn == 0 ) {

			rephdr[0] = rephdr[1] = rephdr[2] = 'N';
			int rsmode = 0;

			// peek sock, if there is node down message
			// receive it, and do below.
			// if there is no node down message, continue
			int pklen = ::recv( sock, hdr2, JAG_SOCK_TOTAL_HDR_LEN, MSG_PEEK | MSG_DONTWAIT );
			if ( pklen  < JAG_SOCK_TOTAL_HDR_LEN ) {
				dn("s56712209 peek pklen=%d < JAG_SOCK_TOTAL_HDR_LEN=%d, skip", pklen, JAG_SOCK_TOTAL_HDR_LEN);
				continue;
			}

			if ( hdr2[0] != JAG_SOME_NODE_DOWN ) {
				dn("s31129  peeked hdr2[0]=%c  != JAG_SOME_NODE_DOWN, continue", hdr2[0] );
				continue;
			}

			dn("s033371 servobj->_faultToleranceCopy > 1 JAG_SOME_NODE_DOWN. recvMessage() ...");
			int rcr = recvMessage( sock, hdr2, newbuf2 );
			dn("s033371 servobj->_faultToleranceCopy > 1 recvMessage() done hdr2=[%s] newbuf2=[%s] rcr=%d", hdr2, newbuf2, rcr);

			if ( rcr < 0 ) {
				rephdr[session.replicType] = 'Y';
				rsmode = servobj->getReplicateStatusMode( rephdr, session.replicType );

				if ( !session.spCommandReject && rsmode > 0 ) {
					servobj->deltalogCommand( rsmode, &session, pmesg, req.batchReply );
				}

				if ( session.uid == "admin" ) {
					if ( session.exclusiveLogin ) {
						servobj->_exclusiveAdmins = 0;
						jd(JAG_LOG_LOW, "Exclusive admin disconnected from %s\n", session.ip.c_str() );
					}
				}

				if ( newbuf ) { free( newbuf ); }
				if ( newbuf2 ) { free( newbuf2 ); }
				-- servobj->_connections;
				break;
			}

			memcpy( rephdr, newbuf2, 3 );
			rsmode = servobj->getReplicateStatusMode( rephdr );
			if ( !session.spCommandReject && rsmode >0 ) {
                dn("s1000876 deltalogCommand pmesg=[%s] req.batchReply=%d", pmesg, req.batchReply );
				servobj->deltalogCommand( rsmode, &session, pmesg, req.batchReply );
			} 
		}
        ***/

    } // for ( ; ; )

	session.active = 0;
	-- servobj->_activeClients; 

   	jagclose(sock);
	delete ptr;
	jagmalloc_trim(0);
	free(sbuf);
   	return NULL;
}

int JagDBServer::getReplicateStatusMode( char *rephdr, int replicType )
{
	if ( replicType >= 0 && _faultToleranceCopy > 2 ) {
		// if client is down, ask servers status using _noop only for 3 replications
		int pos1, pos2, rc1, rc2;
		JagStrSplit sp( _dbConnector->_nodeMgr->_hostClusterNodes, '|' );
		if ( 0 == replicType ) {
			// use left one and right one
			if ( _nthServer == 0 ) {
				pos1 = _nthServer+1;
				pos2 = sp.length()-1;
			} else if ( _nthServer == sp.length()-1 ) {
				pos1 = 0;
				pos2 = _nthServer-1;
			} else {
				pos1 = _nthServer+1;
				pos2 = _nthServer-1;
			}
		} else if ( 1 == replicType ) {
			// use left two hosts;
			if ( _nthServer == 0 ) {
				pos1 = sp.length()-1;
				pos2 = sp.length()-2;
			} else if ( _nthServer == 1 ) {
				pos1 = 0;
				pos2 = sp.length()-1;
			} else {
				pos1 = _nthServer-1;
				pos2 = _nthServer-2;
			}
		} else if ( 2 == replicType ) {
			// use right two hosts
			if ( _nthServer == sp.length()-1 ) {
				pos1 = 0;
				pos2 = 1;
			} else if ( _nthServer == sp.length()-2 ) {
				pos1 = sp.length()-1;
				pos2 = 0;
			} else {
				pos1 = _nthServer+1;
				pos2 = _nthServer+2;
			}
		}

		// ask each server of pos1 and pos2 to see if them is up
		dn("cr8373790 broadcastSignal noop ...");
		rc1 = _dbConnector->broadcastSignal( "_noop", sp[pos1] );
		dn("cr8373790 broadcastSignal noop done");

		rc2 = _dbConnector->broadcastSignal( "_noop", sp[pos2] );
		dn("cr8373794 broadcastSignal noop done");

		// set correct bytes for rephdr
		// "NNN" "YYY" "NYN"
		if ( 0 == replicType ) {
			if ( !rc1 ) *(rephdr+1) = 'N';
			else *(rephdr+1) = 'Y';
			if ( !rc2 ) *(rephdr+2) = 'N';
			else *(rephdr+2) = 'Y';
		} else if ( 1 == replicType ) {
			if ( !rc1 ) *(rephdr) = 'N';
			else *(rephdr) = 'Y';
			if ( !rc2 ) *(rephdr+2) = 'N';
			else *(rephdr+2) = 'Y';
		} else if ( 2 == replicType ) {
			if ( !rc1 ) *(rephdr) = 'N';
			else *(rephdr) = 'Y';
			if ( !rc2 ) *(rephdr+1) = 'N';
			else *(rephdr+1) = 'Y';
		}
	}

	if ( 0 == strncmp( rephdr, "YYN", 3 ) ) return 1;
	else if ( 0 == strncmp( rephdr, "YNY", 3 ) ) return 2;
	else if ( 0 == strncmp( rephdr, "YNN", 3 ) ) return 3;
	else if ( 0 == strncmp( rephdr, "NYY", 3 ) ) return 4;
	else if ( 0 == strncmp( rephdr, "NYN", 3 ) ) return 5;
	else if ( 0 == strncmp( rephdr, "NNY", 3 ) ) return 6;
	return 0;

}

int JagDBServer::createIndexSchema( const JagRequest &req, 
									const Jstr &dbname, JagParseParam *parseParam, Jstr &reterr, bool lockSchema )
{	
	JagTableSchema *tableschema;
	JagIndexSchema *indexschema;
	getTableIndexSchema( req.session->replicType, tableschema, indexschema );

	bool found = false;
	Jstr dbtable = dbname + "." + parseParam->objectVec[0].tableName;
	Jstr dbindex = dbtable + "." + parseParam->objectVec[1].indexName;
	//parseParam->isMemTable = tableschema->isMemTable( dbtable );
	found = indexschema->indexExist( dbname, parseParam );
	if ( found ) {
		reterr = "E32016 Error: Index already exists";
		return 0;
	}

	found = indexschema->existAttr( dbindex );
	if ( found ) { 
		reterr = "E32017 Error: Index already exists in schama";
		return 0; 
	}

	const JagSchemaRecord *trecord = tableschema->getAttr( dbtable );
	if ( ! trecord ) {
		reterr = "E32018 Error: Cannot find table of the index";
		return 0;
	}

	jagint getpos;
	Jstr errmsg;
	Jstr dbcol;
	Jstr defvalStr;
	int keylen = 0, vallen = 0;
	CreateAttribute createTemp;
	JagHashStrInt checkmap;
	JagHashStrInt schmap;

	createTemp.init();
	createTemp.objName.dbName = parseParam->objectVec[1].dbName;
	createTemp.objName.tableName = "";
	createTemp.objName.indexName = parseParam->objectVec[1].indexName;
    
	// map for each column
	const JagVector<JagColumn> &cv = *(trecord->columnVector);

	for ( int i = 0; i < trecord->columnVector->size(); ++i ) {
		schmap.addKeyValue( cv[i].name.c_str(), i);
        dn("s3900222 schmap add [%s] ==> %d", cv[i].name.c_str(), i );
	}
	
	// get key part of create index
	createTemp.spare[0] = JAG_C_COL_KEY;
	createTemp.spare[2] = JAG_ASC;
	bool hrc;

    int polyDim = 0;

    // if first column in index is polyDim >= 1 , then prepend geo:id geo:cl ,... geo:id columns
    int i = 0;
	getpos = schmap.getValue(parseParam->valueVec[i].objName.colName.c_str(), hrc );
	if ( hrc ) {
        polyDim = getPolyDimension( cv[getpos].type );
        dn("s220239 polyDim=%d", polyDim );

        if ( polyDim >= 1 ) {
            //int isMute = 1;
            int offset = 0;
		    createTemp.offset = 0;

            createTemp.objName.colName = "geo:id";
            parseParam->fillStringSubData( createTemp, offset, 1, JAG_GEOID_FIELD_LEN, 1, 0, 0 );
            // offset updated
		    keylen += JAG_GEOID_FIELD_LEN;
            dn("s345018 geo:id createTemp.length=%d  keylen=%d", createTemp.length, keylen );

            createTemp.objName.colName = "geo:col";
            parseParam->fillSmallIntSubData( createTemp, offset, 1, 1, 0, 0 );  // must be part of key and mute
		    keylen += JAG_DSMALLINT_FIELD_LEN ;
            dn("s345018 geo:col createTemp.length=%d  keylen=%d", createTemp.length, keylen );
        
            if ( polyDim > 1 ) {
                createTemp.objName.colName = "geo:m";  // m-th polygon in multipolygon  starts from 1
                parseParam->fillIntSubData( createTemp, offset, 1, 1, 0, 0 );  // must be part of key and mute
		        keylen += JAG_DINT_FIELD_LEN;
                dn("s345018 geo:m createTemp.length=%d  keylen=%d", createTemp.length, keylen );
        
                createTemp.objName.colName = "geo:n";  // n-th ring in a polygon or n-th linestring in multilinestring
                parseParam->fillIntSubData( createTemp, offset, 1, 1, 0, 0 );  // must be part of key and mute
		        keylen += JAG_DINT_FIELD_LEN;
                dn("s345018 geo:n createTemp.length=%d  keylen=%d", createTemp.length, keylen );
            }
        
            createTemp.objName.colName = "geo:i";  // i-th point in a linestring
            parseParam->fillIntSubData( createTemp, offset, 1, 1, 0, 0 );  // must be part of key and mute
		    keylen += JAG_DINT_FIELD_LEN;
            dn("s345018 geo:i createTemp.length=%d  keylen=%d", createTemp.length, keylen );

            dn("s12208 added geo fields");

        } 
	}

	for ( int i = 0; i < parseParam->limit; ++i ) {
		// save for checking duplicate keys
		checkmap.addKeyValue(parseParam->valueVec[i].objName.colName.c_str(), i);

		getpos = schmap.getValue(parseParam->valueVec[i].objName.colName.c_str(), hrc );
		if ( ! hrc ) {
            dn("s298611001 skip i=%d [%s]", i, parseParam->valueVec[i].objName.colName.c_str() );
			continue;
		}

		createTemp.objName.colName = cv[getpos].name.c_str();

		createTemp.type = cv[getpos].type;
		createTemp.offset = keylen;
		createTemp.length = cv[getpos].length;
		createTemp.sig = cv[getpos].sig;
		createTemp.srid = cv[getpos].srid;
		createTemp.metrics = cv[getpos].metrics;

        // user specified key in creating an index
        dn("s2009281 createTemp.objName.colName=[%s] offset=%d len=%d", createTemp.objName.colName.s(), createTemp.offset, createTemp.length );
        // if linestring, add geo:id geo:col geo:i
		
        createTemp.spare[1] = cv[getpos].spare[1];
		createTemp.spare[4] = cv[getpos].spare[4]; // default datetime value patterns
		createTemp.spare[5] = cv[getpos].spare[5]; // mute
		createTemp.spare[6] = cv[getpos].spare[6]; // subcol
		createTemp.spare[7] = cv[getpos].spare[7]; // rollup
		createTemp.spare[8] = cv[getpos].spare[8]; // rollup-type
		createTemp.spare[9] = cv[getpos].spare[9]; // given length of col

		dbcol = dbtable + "." + createTemp.objName.colName;
		defvalStr = "";
		tableschema->getAttrDefVal( dbcol, defvalStr );
		createTemp.defValues = defvalStr.c_str();
		keylen += createTemp.length;
		parseParam->createAttrVec.append( createTemp );
	}

	// add existing keys in table, following specified index-keys in above
    /***
	for (int i = 0; i < cv.size(); i++) {
		if ( cv[i].iskey && !checkmap.keyExist(cv[i].name.c_str() ) ) {
			createTemp.objName.colName = cv[i].name.c_str();
			createTemp.type = cv[i].type;
			createTemp.offset = keylen;
			createTemp.length = cv[i].length;
			createTemp.sig = cv[i].sig;
			createTemp.srid = cv[i].srid;
			createTemp.metrics = cv[i].metrics;

			createTemp.spare[1] = cv[i].spare[1];
			createTemp.spare[4] = cv[i].spare[4];
			createTemp.spare[5] = cv[i].spare[5];
			createTemp.spare[6] = cv[i].spare[6];
			createTemp.spare[7] = cv[i].spare[7];
			createTemp.spare[8] = cv[i].spare[8];
			createTemp.spare[9] = cv[i].spare[9];

			dbcol = dbtable + "." + createTemp.objName.colName;
			defvalStr = "";
			tableschema->getAttrDefVal( dbcol, defvalStr );
			createTemp.defValues = defvalStr.c_str();
			keylen += createTemp.length;
			parseParam->createAttrVec.append( createTemp );

            dn("s4500019 createTemp.objName.colName=[%s]", createTemp.objName.colName.s() );
		}
	}
    ***/

	parseParam->keyLength = keylen;
	
	// get value part of create index. Index can attach value columns.  "on tab1(key: v1, v4, k3, value: v8, v10, v12 )"
	createTemp.spare[0] = JAG_C_COL_VALUE;
	for ( int i = parseParam->limit; i < parseParam->valueVec.size(); ++i ) {
		getpos = schmap.getValue(parseParam->valueVec[i].objName.colName.c_str(), hrc);
		if ( ! hrc ) {
            dn("s3450011 i=%d skip [%s]", i, parseParam->valueVec[i].objName.colName.c_str() );
			continue;
		}

		createTemp.objName.colName = cv[getpos].name.c_str();
		createTemp.type = cv[getpos].type;
		createTemp.offset = keylen;
		createTemp.length = cv[getpos].length;
		createTemp.sig = cv[getpos].sig;
		createTemp.srid = cv[getpos].srid;
		createTemp.metrics = cv[getpos].metrics;

		createTemp.spare[1] = cv[getpos].spare[1];
		createTemp.spare[4] = cv[getpos].spare[4];
		createTemp.spare[5] = cv[getpos].spare[5];
		createTemp.spare[6] = cv[getpos].spare[6];
		createTemp.spare[7] = cv[getpos].spare[7];
		createTemp.spare[8] = cv[getpos].spare[8];
		createTemp.spare[9] = cv[getpos].spare[9];

		dbcol = dbtable + "." + createTemp.objName.colName;
		defvalStr = "";
		tableschema->getAttrDefVal( dbcol, defvalStr );
		createTemp.defValues = defvalStr.c_str();
		keylen += createTemp.length;
		vallen += createTemp.length;
		parseParam->createAttrVec.append( createTemp );

        dn("s5911220 parseParam->createAttrVec.append colname=[%s]", createTemp.objName.colName.s() );
	}

	parseParam->valueLength = vallen;
	if ( lockSchema ) {
		JAG_BLURT jaguar_mutex_lock ( &g_dbschemamutex ); JAG_OVER
	}

    dn("s366001 indexschema->insert( parseParam )");
    int rc = indexschema->insert( parseParam, false );

	if ( lockSchema ) {
		jaguar_mutex_unlock ( &g_dbschemamutex );
	}

    if ( rc ) jd(JAG_LOG_LOW, "user [%s] create index [%s]\n", req.session->uid.c_str(), dbindex.c_str() );
	return rc;
}

void JagDBServer::init( JagCfg *cfg )
{
	Jstr jagdatahome = cfg->getJDBDataHOME( JAG_MAIN );

   	jagmkdir( jagdatahome.c_str(), 0700 );

    Jstr sysdir = jagdatahome + "/system";
   	jagmkdir( sysdir.c_str(), 0700 );

    sysdir = jagdatahome + "/test";
   	jagmkdir( sysdir.c_str(), 0700 );
	
	jagdatahome = cfg->getJDBDataHOME( JAG_PREV );
   	jagmkdir( jagdatahome.c_str(), 0700 );

	sysdir = jagdatahome + "/system";
    jagmkdir( sysdir.c_str(), 0700 );

    sysdir = jagdatahome + "/test";
    jagmkdir( sysdir.c_str(), 0700 );
	
	jagdatahome = cfg->getJDBDataHOME( JAG_NEXT );
   	jagmkdir( jagdatahome.c_str(), 0700 );

	sysdir = jagdatahome + "/system";
    jagmkdir( sysdir.c_str(), 0700 );

    sysdir = jagdatahome + "/test";
    jagmkdir( sysdir.c_str(), 0700 );

    sysdir = jaguarHome() + "/backup";
    jagmkdir( sysdir.c_str(), 0700 );

    Jstr newdir;
    newdir = sysdir + "/15min";
    jagmkdir( newdir.c_str(), 0700 );

    newdir = sysdir + "/hourly";
    jagmkdir( newdir.c_str(), 0700 );

    newdir = sysdir + "/daily";
    jagmkdir( newdir.c_str(), 0700 );

    newdir = sysdir + "/weekly";
    jagmkdir( newdir.c_str(), 0700 );

    newdir = sysdir + "/monthly";
    jagmkdir( newdir.c_str(), 0700 );

	Jstr cs;
	jd(JAG_LOG_LOW, "Jaguar start\n" );
	cs = cfg->getValue("MEMORY_MODE", "high");
	if ( cs == "high" || cs == "HIGH" ) {
		_memoryMode = JAG_MEM_HIGH;
		jd(JAG_LOG_LOW, "MEMORY_MODE is high\n" );
	} else {
		_memoryMode = JAG_MEM_LOW;
		jd(JAG_LOG_LOW, "MEMORY_MODE is low\n" );
	}

}

void JagDBServer::helpPrintTopic( const JagRequest &req )
{
	Jstr str;
	str += "You can enter the following commands:\n\n";

	str += "help use;           (how to use databases)\n";
	str += "help desc;          (how to describe tables)\n";
	str += "help show;          (how to show tables)\n";
	str += "help create;        (how to create tables)\n";
	str += "help insert;        (how to insert data)\n";
	str += "help load;          (how to load data from client host)\n";
	str += "help select;        (how to select data)\n";
	str += "help getfile;       (how to get file data)\n";
	str += "help update;        (how to update data)\n";
	str += "help delete;        (how to delete data)\n";
	str += "help drop;          (how to drop a table completely)\n";
	// str += "help rename        (how to rename a table)\n";
	str += "help alter;         (how to alter a table and rename a key column)\n";
	str += "help truncate;      (how to truncate a table)\n";
	str += "help func;          (how to call functions in select)\n";
	str += "help spool;         (how to write output data to a file)\n";
	str += "help password;      (how to change the password of current user)\n";
	str += "help source;        (how to exeucte commands from a file)\n";
	str += "help shell;         (how to exeucte shell commands)\n";
	str += "\n";
	if ( req.session->uid == "admin" ) {
		str += "help admin;         (how to execute commands for admin account)\n";
		str += "help shutdown;      (how to shutdown servers)\n";
		str += "help createdb;      (how to create a new database)\n";
		str += "help dropdb;        (how to drop a database)\n";
		str += "help createuser;    (how to create a new user account)\n";
		str += "help dropuser;      (how to drop a user account)\n";
		str += "help showusers;     (how to view all user accounts)\n";
		str += "help addcluster;    (how to add a new cluster)\n";
		str += "help grant;         (how to grant permission to a user)\n";
		str += "help revoke;        (how to revoke permission to a user)\n";
	}
	str += "\n";
	str += "\n";

	sendDataEnd( req, str);
}

void JagDBServer::helpTopic( const JagRequest &req, const char *cmd )
{
	Jstr str;
	str += "\n";

	if ( 0 == strncasecmp( cmd, "use", 3 ) ) {
		str += "use DATABASE;\n";
		str += "\n";
		str += "Example:\n";
		str += "use userdb;\n";
	} else if ( 0 == strncasecmp( cmd, "admin", 5 ) ) {
    		str += "createdb DBNAME;\n"; 
    		str += "create database DBNAME;\n"; 
    		str += "dropdb [force] DBNAME;\n"; 
    		str += "createuser UID;\n"; 
    		str += "create user UID;\n"; 
    		str += "createuser UID:PASSWORD;\n"; 
    		str += "create user UID:PASSWORD;\n"; 
    		str += "dropuser UID;\n"; 
    		str += "drop user UID;\n"; 
    		str += "showusers;\n"; 
    		str += "show users;\n"; 
    		str += "grant PERM1, PERM2, ... PERM on DB.TAB.COL to USER [where ...];\n"; 
    		str += "revoke PERM1, PERM2, ... PERM on DB.TAB.COL from USER;\n"; 
    		str += "addcluster;  (add a new cluster of servers)\n"; 
    		str += "\n"; 
    		str += "Example:\n";
    		str += "createdb mydb;\n";
    		str += "create database mydb2;\n";
    		str += "dropdb mydb;\n";
    		str += "dropdb force mydb123;\n";
    		str += "createuser test123;\n"; 
    		str += "  New user password: ******\n"; 
    		str += "  New user password again: ******\n"; 
    		str += "createuser test123:mypassword888000;\n"; 
    		str += "create user test123:mypassword888888;\n"; 
    		str += "(Maximum length of user ID and password is 32 characters.)\n";
    		str += "dropuser test123;\n"; 
    		str += "drop user test123;\n"; 
    		str += "grant all on all to test123;\n";
    		str += "grant select, update, delete on all to user123;\n";
    		str += "grant select on mydb.tab123.col3 to user123;\n";
    		str += "grant select on mydb.tab123.* to user123 where tab123.col4 >= 100 or tab123.col4 <= 800;\n";
    		str += "grant update, delete on mydb.tab123 to user123;\n";
    		str += "revoke update on mydb.tab123 from user123;\n";
   	} else if ( 0 == strncasecmp( cmd, "createdb", 8 ) ) {
    		str += "jaguar> createdb DBNAME;\n"; 
    		str += "\n";
    		str += "Only the admin account can issue this command to create a new database.\n"; 
    		str += "\n";
    		str += "Example:\n";
    		str += "jaguar> createdb mydb123;\n";
   	} else if (  0 == strncasecmp( cmd, "shutdown", 8 ) ) {
    		str += "Shutdown jaguar server process\n";
    		str += "jaguar> shutdown SERVER_IP;\n"; 
    		str += "jaguar> shutdown all;\n"; 
    		str += "\n";
    		str += "Only the admin account logging with exclusive mode can issue this command.\n"; 
    		str += "\n";
    		str += "Example:\n";
    		str += "$ jag -u admin -p -h localhost:8888 -x yes\n";
    		str += "jaguar> shutdown 192.168.7.201;\n";
    		str += "jaguar> shutdown all;\n";
   	} else if ( 0 == strncasecmp( cmd, "dropdb", 6 ) ) {
    		str += "jaguar> dropdb DBNAME;\n"; 
    		str += "\n";
    		str += "Only the admin account can issue this command to drop a new database.\n"; 
    		str += "\n";
    		str += "Example:\n";
    		str += "jaguar> dropdb mydb123;\n";
   	} else if ( 0 == strncasecmp( cmd, "createuser", 9 ) ) {
    		str += "jaguar> createuser UID;\n"; 
    		str += "\n";
    		str += "Only the admin account can issue this command to create a new user account.\n"; 
    		str += "\n";
    		str += "Example:\n";
    		str += "jaguar> createuser johndoe;\n";
   	} else if ( 0 == strncasecmp( cmd, "dropuser", 8 ) ) {
    		str += "jaguar> dropuser UID;\n"; 
    		str += "\n";
    		str += "Only the admin account can issue this command to delete a user account.\n"; 
    		str += "\n";
    		str += "Example:\n";
    		str += "jaguar> dropuser johndoe;\n";
   	} else if (  0 == strncasecmp( cmd, "showusers", 8 ) ) {
    		str += "jaguar> showusers;\n"; 
    		str += "\n";
    		str += "Only the admin account can issue this command to display a list of current user accounts.\n"; 
    		str += "\n";
    		str += "Example:\n";
    		str += "jaguar> showusers;\n";
   	} else if ( 0 == strncasecmp( cmd, "addcluster", 10 ) ) {
    		str += "jaguar> addcluster;\n"; 
    		str += "\n";
    		str += "Only the admin account can issue this command to add new servers.\n"; 
    		str += "The admin user must login to an existing host, connect to jaguardb with exclusive mode, i.e., \n";
    		str += " using \"-x yes\" option in the jag command.\n"; 
    		str += "Before starting this operation, on the host that the admin user is on, there must exist a file \n";
    		str += "$JAGUAR_HOME/conf/newcluster.conf that contains the IP addresses of new hosts on each line\n";
    		str += "in the file. DO NOT inclue existing hosts in the file newcluster.conf. Once this file is ready and correct, \n";
    		str += " execute the addcluster command which will include and activate the new servers.\n";
    		str += "\n";
    		str += "Example:\n";
    		str += "\n";
    		str += "Suppose you have 192.168.1.10, 192.168.1.11, 192.168.1.12, 192.168.1.13 on your current cluster.\n";
    		str += "You want to add a new cluster with new hosts: 192.168.1.14, 192.168.1.15, 192.168.1.16, 192.168.1.17 to the system.\n";
    		str += "The following steps demonstrate the process to add the new cluster into the system:\n";
    		str += "\n";
    		str += "1. Provision the new hosts 192.168.1.14, 192.168.1.15, 192.168.1.16, 192.168.1.17 and install jaguardb on these hosts\n";
    		str += "2. Configure the new cluster with new hosts 192.168.1.14, 192.168.1.15, 192.168.1.16, 192.168.1.17\n";
    		str += "3. The new cluster is a working but blank cluster, without any table data.\n";
    		str += "   The new cluster is itself an independent database system with all JaguarDB software installed with the SAME version.\n";
    		str += "   But DO NOT create any tables, indexes, or insert any data in the new cluster.\n";
    		str += "   The new cluster and existing clusters should have the same password for admin.\n";
    		str += "4. Admin user should log in (or ssh) to a host in your EXISTING cluster, e.g., 192.168.1.10\n";
    		str += "5. Prepare the file newcluster.conf, with the IP addresses of new hosts on each line separately\n";
    		str += "   In newcluster.conf file:\n";
    		str += "   192.168.1.14\n";
    		str += "   192.168.1.15\n";
    		str += "   192.168.1.16\n";
    		str += "   192.168.1.17\n";
    		str += "\n";
    		str += "6. Save the newcluster.conf file in $JAGUAR_HOME/conf/ directory\n";
    		str += "7. Connect to local jaguardb server from the host that contains the newcluster.conf file\n";
    		str += "       $JAGUAR_HOME/bin/jag -u admin -p <adminpassword> -h 192.168.1.10:8888 -x yes\n";
    		str += "8. While connected to the jagaurdb, run the addcluster command:\n";
    		str += "       jaguardb> addcluster; \n";
    		str += "\n";
    		str += "   The above command should finish quickly and the new cluster will be acceped.\n";
    		str += "Note: all above tasks should be performed by the Linux user account who owns the JaguarDB server and\n";
            str += "      the $JAGUAR_HOME directory\n";
    		str += "\n";
    		str += "Note:\n";
    		str += "\n";
    		str += "1. Never directly add any new hosts in the file $JAGUAR_HOME/conf/cluster.conf manually\n";
    		str += "2. Any plan to add a new cluster of hosts must strictly follow the addcluster process described here\n";
    		str += "3. Execute addcluster in the existing cluster, NOT in the new cluster\n";
    		str += "4. It is recommended that existing clusters and new cluster contain large number of hosts. (hundreds or thousands)\n";
    		str += "   For example, the existing cluster can have 100 hosts, and the new cluster can have 200 hosts.\n";
    		str += "5. Make sure JaguarDB is installed on all the hosts of the new cluster, and connectivity is good among all the hosts.\n";
    		str += "6. The server and client must have the same version, on ALL the hosts of existing clusters and the new cluster\n";
    		str += "7. If the new cluster is setup with a new version of JaguarDB, the old clusters most likely will need an upgrade\n";
    		str += "8. After adding a new cluster, all hosts will have the same cluster.conf file\n";
    		str += "9. Make sure REPLICATION factor is the same in the file conf/server.conf on each host\n";
   	} else if (  0 == strncasecmp( cmd, "grant", 5 ) ) {
    		str += "jaguar> grant all on all to user;\n"; 
    		str += "jaguar> grant PERM1, PERM2, ... PERM on DB.TAB.COL to user;\n"; 
    		str += "jaguar> grant PERM on DB.TAB.* to user;\n"; 
    		str += "jaguar> grant PERM on DB.TAB  to user;\n"; 
    		str += "jaguar> grant PERM on DB  to user;\n"; 
    		str += "jaguar> grant PERM on all  to user;\n"; 
    		str += "jaguar> grant select on DB.TAB.COL to user [where TAB.COL1 > NNN and TAB.COL2 < MMM;\n"; 
    		str += "\n";
    		str += "Only the admin account can issue this command.\n"; 
    		str += "PERM is one of: all, create, insert, select, update, delete, alter, truncate\n"; 
    		str += "All means all permissions.\n";
    		str += "The where statement, if provided, will be used to filter rows in select.\n";
    		str += "\n";
    		str += "Example:\n";
    		str += "jaguar> grant all on all to user123;\n";
    		str += "jaguar> grant all on mydb.tab123 to user123;\n";
    		str += "jaguar> grant select on mydb.tab123.* to user123;\n";
    		str += "jaguar> grant select on mydb.tab123.col2 to user3 where tab123.col4>100;\n";
    		str += "jaguar> grant delete, update on mydb.tab123.col4 to user1;\n";
   	} else if (  0 == strncasecmp( cmd, "revoke", 6 ) ) {
    		str += "jaguar> revoke all on all from user;\n"; 
    		str += "jaguar> revoke PERM1, PERM2, ... PERM on DB.TAB.COL from user;\n"; 
    		str += "jaguar> revoke PERM on DB.TAB.* from user;\n"; 
    		str += "jaguar> revoke PERM on DB.TAB  from user;\n"; 
    		str += "jaguar> revoke PERM on DB  from user;\n"; 
    		str += "jaguar> revoke PERM on all  from user;\n"; 
    		str += "\n";
    		str += "Only the admin account can issue this command.\n"; 
    		str += "PERM is one of: all/create/insert/select/update/delete/alter/truncate \n"; 
    		str += "All means all permissions. The permission to be revoked must exist already.\n";
    		str += "\n";
    		str += "Example:\n";
    		str += "jaguar> revoke all on all from user123;\n";
    		str += "jaguar> revoke all on mydb.tab123 from user123;\n";
    		str += "jaguar> revoke select on mydb.tab123.* from user123;\n";
    		str += "jaguar> revoke select, update on mydb.tab123.col2 from user3;\n";
    		str += "jaguar> revoke update, delete on mydb.tab123.col4 from user1;\n";
	} else if ( 0 == strncasecmp( cmd, "desc", 4 ) ) {
		str += "desc TABLE [detail];\n"; 
		str += "desc INDEX [detail];\n"; 
		str += "(If detail is provided, it will display internal fields for complex columns)\n";
		str += "\n";
		str += "Example:\n";
		str += "desc usertab;\n";
		str += "desc addr_index;\n";
		str += "desc geotab detail;\n";
	} else if ( 0 == strncasecmp( cmd, "password", 8 ) ) {
		str += "changepass;\n"; 
		str += "changepass NEWPASSWORD;\n"; 
		str += "changepass uid:NEWPASSWORD; -- for admin only\n"; 
		str += "\n";
		str += "Example:\n";
		str += "changepass;\n"; 
		str += "   New password: ******\n"; 
		str += "   New password again: ******\n"; 
		str += "changepass mynewpassword888;\n"; 
	} else if ( 0 == strncasecmp( cmd, "show", 3 ) ) {
		str += "show cluster              (display servers in clusters)\n";
		str += "show databases            (display all databases in the system)\n"; 
		str += "show tables [LIKE PAT]    (display all tables in current database. PAT: '%pat' or '%pat%' or 'pat%' )\n"; 
		str += "show create TABLE         (display statement of creating a table)\n";
		str += "show indexes              (display all indexes in current database)\n"; 
		str += "show currentdb            (display current database being used)\n"; 
		str += "show task                 (display all active tasks)\n"; 
		str += "show tasks                (display all active tasks)\n"; 
		str += "show indexes from TABLE   (display all indexes of a table)\n"; 
		str += "show indexes in TABLE     (display all indexes of a table)\n"; 
		str += "show indexes on TABLE     (display all indexes of a table)\n"; 
		str += "show indexes [LIKE PAT]   (display all indexes matching a pattern. PAT: '%pat' or '%pat%' or 'pat%' )\n"; 
		str += "show server version       (display Jaguar server version)\n"; 
		str += "show client version       (display Jaguar client version)\n"; 
		str += "show user                 (display username of current session)\n"; 
		str += "show status               (display statistics of server cluster)\n";
		str += "show tools                (display bin/tools command scripts)\n";
		str += "show grants [for UID]     (display grants for user. Default is current user)\n";
		str += "\n";
		str += "Example:\n";
		str += "show databases;\n"; 
		str += "show tables;\n"; 
		str += "show tables like '%mytab%';\n"; 
		str += "show tables like 'mytab%';\n"; 
		str += "show indexes from mytable;\n"; 
		str += "show indexes in mytable;\n"; 
		str += "show indexes on mytable;\n"; 
		str += "show indexes like 'jb%';\n"; 
		str += "show indexes;\n"; 
		str += "show status;\n";
		str += "show task;\n";
	} else if ( 0 == strncasecmp( cmd, "create", 3 ) ) {
		str += "create table [if not exists] TABLE ( key: KEY TYPE(size), ..., value: VALUE TYPE(size), ...  );\n";
		str += "create table [if not exists] TABLE ( key: TS TIMESTAMP, KEY TYPE(size), ..., value: VALUE ROLLUP(TYPE) TYPE, ...  );\n";
		str += "create table [if not exists] TABLE ( key: KEY TYPE(size), ..., value: VALUE TYPE(srid:ID,metrics:M), ...  );\n";
		str += "create index [if not exists] INDEXNAEME [ticks] on TABLE(COL1, COL2, ...[, value: COL,COL]);\n";
		str += "create index [if not exists] INDEXNAEME [ticks] on TABLE(key: COL1, COL2, ...[, value: COL,COL]);\n";
		str += "create user <username:password>;\n";
		str += "\n";
		str += "Example:\n";
		str += "create table dept ( key: name char(32),  value: age int, b bit default b'0' );\n";
		str += "create table dept ( key: name char(32),  value: age int default '10', b bit default b'0' );\n";
		str += "create table log ( key: id bigint,  value: tm timestamp default current_timestamp);\n";
		str += "create table log ( key: id bigint,  value: tm timestamp default current_timestamp on update current_timestamap );\n";
		str += "create table log ( key: id bigint,  value: tn timestampnano default current_timestamp );\n";
		str += "create table log ( key: id bigint,  value: tn timestampnano default current_timestamp on update current_timestamp );\n";
		str += "create table timeseries(5m,30m) ts2 ( key: k1 int, ts timestamp, value: v1 rollup int, v2 int);\n";
		str += "create index ts2idx1 on ts2(v1, k1);\n";
		str += "create index ts2idx2 ticks on ts2(v1);\n";

		str += "create table timeseries(15m:3d,1h:24h,1d:30d,1y:10y|3M) log1 (key: ts timestamp, id bigint, value: a rollup(sum) bigint, b int);\n";
		str += "create table timeseries(15s:1m,5m:1d|3d) log2 (key: ts timestamp, id bigint, value: a rollup(sum) bigint, b int);\n";
		str += "create table timeseries(15s:3m,5m:3d) log3 (key: ts timestamp, id bigint, value: a rollup(sum) bigint, b int);\n";
		str += "create table user ( key: name char(32),  value: age int, address char(128), rdate date );\n";
		str += "create table sp ( key: name varchar(32), stime datetime, value: bus varchar(32), id uuid );\n";
		str += "create table tr ( key: name varchar(32), value: tp char(2) enum('A2', 'B3'), driver varchar(32), id uuid );\n";
		str += "create table mt ( key: name varchar(32), value: tp char(2) enum('A2', 'B3') default 'A2', zip varchar(32) );\n";
		str += "create table message ( key: name varchar(32), value: msg text );\n";
		str += "create table media ( key: name varchar(32), value: img1 file, img2 file );\n";
		str += "create table if not exists sales ( key: id bigint, stime datetime, value: member char(32) );\n";
		str += "create table geo ( key: id bigint, value: ls linestring, sq square );\n";
		str += "create table ai ( key: id bigint, value: v vector, int vol );\n";
		str += "create table park ( key: id bigint, value: lake polygon(srid:wgs84), farm rectangle(srid:wgs84) );\n";
		str += "create table tm ( key: id bigint, value: name char(32), range(datetime) );\n";
		str += "create table pt ( key: id int, value: s linestring(srid:4326,metrics:5) );\n";
		str += "create index if not exists addr_index on user( address );\n";
		str += "create index addr_index on user( address, value: zipcode );\n";
		str += "create index addr_index on user( key: address, value: zipcode, city );\n";
		str += "create user  iamjon:somelong!pass888888888888888;\n";
		str += "createuser  iamjon:somelong!pass888888888888888;\n";
	} else if ( 0 == strncasecmp( cmd, "insert", 3 ) ) {
		str += "insert into TABLE (col1, col2, col3, ...) values ( 'va1', 'val2', intval, ... );\n"; 
		str += "insert into TABLE values ( 'k1', 'k2', 'va1', 'val2', intval, ... );\n"; 
		str += "insert into TABLE values ( 'k1', 'k2', 'va1', square(0 0 100), ... );\n"; 
		str += "insert into TABLE values ( 'k1', 'k2', 'va1', json({...}), ... );\n"; 
		str += "insert into TAB1 select TAB2.col1, TAB2.col2, ... from TAB2 [WHERE] [LIMIT];\n";
		str += "insert into TAB1 (TAB1.col1, TAB1.col2, ...) select TAB2.col1, TAB2.col2, ... from TAB2 [WHERE] [LIMIT];\n";
		//str += "insert into TAB1 (TAB1.col1, TAB1.col2, ...) values ( 'k1', 'k2', load_file(/path/to/file), 'v4' );\n";
		str += "insert into TAB1 (TAB1.col1, TAB1.col2, ...) values ( 'k1', now(), 'v3', 'v4' );\n";
		str += "insert into TAB1 (TAB1.col1, TAB1.col2, ...) values ( 'k1', time(), 'v3', 'v4' );\n";
		str += "\n";
		str += "Example:\n";
		str += "insert into user values ( 'John S.', 'Doe' );\n";
		str += "insert into user ( fname, lname ) values ( 'John S.', 'Doe' );\n";
		str += "insert into user ( fname, lname, age ) values ( 'David', 'Doe', 30 );\n";
		str += "insert into user ( fname, lname, age, addr ) values ( 'Larry', 'Lee', 40, '123 North Ave., CA' );\n";
		str += "insert into member ( name, datecol ) values ( 'LarryK', '2015-03-21' );\n";
		str += "insert into member ( name, timecol ) values ( 'DennyC', '2015-12-23 12:32:30.022012 +08:30' );\n";
		str += "insert into geo values ( 'DennyC', linestring(1 2, 2 3, 3 4, 4 5), '123' );\n";
		str += "insert into ai values ( 'DennyC', vector(1, 3, 4, 5, 10, 21), '123' );\n";
		str += "insert into geo2 values ( '123', json({\"type\":\"Point\", \"coordinates\": [2,3] }) );\n";
		str += "insert into ev values ( '123', range(10, 20) );\n";
		str += "insert into tm values ( '123', range('2018-09-01 11:12:10', '2019-09-01 11:12:10' ) );\n";
		str += "For datetime, datetimenano, timestamp columns, if no time zone is provided (as in +HH:MM),\n";
		str += "then the provided time is taken as the client's local time.\n";
		str += "insert into t1 select * from t2 where t2.key1=1000;\n";
		str += "insert into t1 (t1.k1, t1.k2, t1.c2) select t2.k1, t2.c2, t2.c4 from t2 where t2.k1=1000;\n";
		str += "The columns in t1 and selected columns from t2 must be compatible (same type)\n";
		//str += "insert into t1 values ( 'k1', 'k2', load_file($HOME/path/to/girl.jpg), 'v4' );\n";
		str += "The data of $HOME/path/to/girl.jpg will be first base64-encoded and then loaded into table t1.\n";
		str += "now() inserts datetime string \"YYYY-MM-DD HH:MM:SS.dddddd\" into a table.\n";
		str += "time() inserts the number of seconds since the Epoch.\n";
	} else if ( 0 == strncasecmp( cmd, "getfile", 7 ) ) {
		str += "getfile FILECOL1 into LOCALFILE, FILECOL2 into LOCALFILE2, ... from TABLE where ...;\n";
		str += "    (The above command downloads data of column(s) of type file and saves to client side files)\n";
		str += "getfile FILECOL into stdout from TABLE where ...;\n";
		str += "    (The above command reads the data of a file column and writes the data to stdout)\n";
		str += "getfile FILECOL1 time, FILECOL2 size, FILECOL3 md5 from TABLE where ...;\n";
		str += "getfile FILECOL1 time, FILECOL1 size, FILECOL1 md5 from TABLE where ...;\n";
		str += "    (The above command displays modification time, size (in bytes), and md5sum of file column)\n";
		str += "getfile FILECOL1 sizemb from TABLE where ...;\n";
		str += "    (The above command displays size (in megabytes) of file column)\n";
		str += "getfile FILECOL1 sizegb from TABLE where ...;\n";
		str += "    (The above command displays size (in gigabytes) of file column)\n";
		str += "getfile FILECOL1 fpath from TABLE where ...;\n";
		str += "    (The above command displays the fullpath name of a file column)\n";
		str += "getfile FILECOL1 host from TABLE where ...;\n";
		str += "    (The above command displays the host where the file is stored)\n";
		str += "getfile FILECOL1 hostfpath from TABLE where ...;\n";
		str += "    (The above command displays the host and full path of the file)\n";
		str += "\n";
		str += "Example:\n";
		str += "getfile img1 into myimg1.jpg, img2 into myimg2.jog from media where uid='100';\n"; 
		str += "    (assume img1 and img2 are two file type columns in table media)\n";
		str += "getfile img1 into stdout from media where uid='100';\n"; 
		str += "getfile img time, img size, img md5 from TABLE where ...;\n";
		str += "getfile img fpath from TABLE where ...;\n";
		str += "getfile img hostfpath from TABLE where ...;\n";
		str += "getfile img fpath from INDEX where ...;\n";
	} else if ( 0 == strncasecmp( cmd, "select", 3 ) ) {
		str += "(SELECT CLAUSE) from TABLE [WHERE CLAUSE] [GROUP BY CLAUSE] [ORDER BY] [LIMIT CLAUSE] [exportsql] [TIMEOUT N];\n";
		str += "(SELECT CLAUSE) from TABLE [WHERE CLAUSE] [GROUP BY CLAUSE] [ORDER BY] [LIMIT CLAUSE] [exportcsv] [TIMEOUT N];\n";
		str += "(SELECT CLAUSE) from TABLE [WHERE CLAUSE] [GROUP BY CLAUSE] [ORDER BY] [LIMIT CLAUSE] [export] [TIMEOUT N];\n";
		str += "(SELECT CLAUSE) from INDEX [WHERE CLAUSE] [GROUP BY CLAUSE] [ORDER BY] [LIMIT CLAUSE] [TIMEOUT N];\n";
		str += "select * from TABLE;\n"; 
		str += "select * from TABLE limit N;\n"; 
		str += "select * from TABLE limit S,N;\n"; 
		str += "select COL1, COL2, ... from TABLE;\n"; 
		str += "select COL1, COL2, ... from TABLE limit N;\n"; 
		str += "select COL1, COL2, ... from TABLE limit N;\n"; 
		str += "select COL1, COL2, ... from TABLE where KEY='...' or KEY='...' and ( ... ) ;\n"; 
		str += "select COL1, COL2, ... from TABLE where KEY <='...' and KEY >='...' or ( ... and ... );\n"; 
		str += "select COL1, COL2, ... from TABLE where KEY between 'abc' and 'bcd' and KEY2 between e and f;\n";
		str += "select COL1, COL2, ... from TABLE where KEY between 'abc' and 'bcd' and VAL1 between m and n;\n";
		str += "select COL1 as col1label, COL2 col2label, ... from TABLE;\n";
		str += "select count(*) from TABLE;\n"; 
		str += "select min(COL1), max(COL2) max2, avg(COL3) as avg3, sum(COL4) sum4, count(1) from TABLE;\n"; 
		str += "select FUNC(COL1), FUNC(COL2) as x from TABLE;\n"; 
		str += "\n";
		str += "Example:\n";
		str += "select * from user;\n"; 
		str += "select * from user export;\n"; 
		str += "select * from user exportcsv;\n"; 
		str += "select * from user exportsql timeout 500;\n"; 
		str += "select * from user limit 100;\n"; 
		str += "select * from user limit 1000,100;\n"; 
		str += "select fname, lname, address from user;\n"; 
		str += "select fname, lname, address, age  from user limit 10;\n"; 
		str += "select fname, lname, address  from user where fname='Sam' and lname='Walter';\n"; 
		str += "select * from user where fname='Sam' and lname='Walter';\n"; 
		str += "select * from user where fname='Sam' or ( fname='Ted' and lname like 'Ben%');\n"; 
		str += "select * from user where fname='Sam' and match 'Ben.*s');\n"; 
		str += "select * from user where fname >= 'Sam';\n"; 
		str += "select * from user where fname >= 'Sam' and fname < 'Zack';\n"; 
		str += "select * from user where fname >= 'Sam' and fname < 'Zack' and ( zipcode = 94506 or zipcode = 94507);\n"; 
		str += "select * from user where fname match 'Sam.*' and fname < 'Zack' and zipcode in ( 94506, 94582 );\n"; 
		str += "select * from t1_index  where uid='frank380' or uid='davidz';\n";
		str += "select * from sales  where stime between '2014-12-01 00:00:00 -08:00' and '2014-12-31 23:59:59 -08:00';\n";
		str += "select avg(amt) as amt_avg from sales;\n";
		str += "select sum(amt) amt_sum from sales where ...;\n";
		str += "select sum(amt) amt_sum from sales group by key1, key2 limit 10;\n";
		str += "select k1, sum(amt) amt_sum from sales group by val1, key1 limit 10;\n";
		str += "select sum(amt+fee) as amt_sum from sales where ...;\n";
		str += "select c1, c4, count(1) as cnt from tab123 where k1 like 'abc%' group by v1, k2 order by cnt desc limit 100; \n";
		str += "select c1, c4, count(1) as cnt from tab123 group by v1, k2 order by cnt desc limit 1000 timeout 180; \n";
	} else if ( 0 == strncasecmp( cmd, "func", 4 ) ) {
		str += "SELECT FUNC( EXPR(COL) ) from TABLE [WHERE CLAUSE] [GROUP BY CLAUSE] [LIMIT CLAUSE];\n";
		str += "  EXPR(COL): \n";
		str += "    Numeric columns:  columns with arithmetic operation\n";
		str += "                      +  addition\n";
		str += "                      -  subtraction)\n";
		str += "                      *  multiplication)\n";
		str += "                      /  division\n";
		str += "                      %  modulo\n";
		str += "                      ^  power\n";
		str += "    String columns: Concatenation of columns or  string constants\n";
		str += "                      string column + string column\n";
		str += "                      string column + string constant\n";
		str += "                      string constant + string column\n";
		str += "                      string constant + string constant\n";
		str += "                      string constant:  'some string'\n";
		str += "  FUNC( EXPR(COL) ): \n";

		str += "    min( EXPR(COL) )    -- minimum value of column expression\n";
		str += "    max( EXPR(COL) )    -- maximum value of column expression\n";
		str += "    avg( EXPR(COL) )    -- average value of column expression\n";
		str += "    sum( EXPR(COL) )    -- sum of column expression\n";
		str += "    stddev( EXPR(COL) ) -- standard deviation of column expression\n";
		str += "    first( EXPR(COL) )  -- first value of column expression\n";
		str += "    last( EXPR(COL) )   -- last value of column expression\n";
		str += "    abs( EXPR(COL) )    -- absolute value of column expression\n";
		str += "    acos( EXPR(COL) )   -- arc cosine function of column expression\n";
		str += "    asin( EXPR(COL) )   -- arc sine function of column expression\n";
		str += "    ceil( EXPR(COL) )   -- smallest integral value not less than column expression\n";
		str += "    cos( EXPR(COL) )    -- cosine value of column expression\n";
		str += "    cot( EXPR(COL) )    -- inverse of tangent value of column expression\n";
		str += "    floor( EXPR(COL) )  -- largest integral value not greater than column expression\n";
		str += "    log2( EXPR(COL) )   -- base-2 logarithmic function of column expression\n";
		str += "    log10( EXPR(COL) )  -- base-10 logarithmic function of column expression\n";
		str += "    log( EXPR(COL) )    -- natural logarithmic function of column expression\n";
		str += "    ln( EXPR(COL) )     -- natural logarithmic function of column expression\n";
		str += "    mod( EXPR(COL), EXPR(COL) )  -- modulo value of first over second column expression\n";
		str += "    pow( EXPR(COL), EXPR(COL) )  -- power function of first to second column expression\n";
		str += "    radians( EXPR(COL) )   -- convert degrees to radian\n";
		str += "    degrees( EXPR(COL) )   -- convert radians to degrees\n";
		str += "    sin( EXPR(COL) )    -- sine function of column expression\n";
		str += "    sqrt( EXPR(COL) )   -- square root function of column expression\n";
		str += "    tan( EXPR(COL) )    -- tangent function of column expression\n";
		str += "    substr( EXPR(COL), start, length )  -- sub string of column expression\n";
		str += "    substr( EXPR(COL), start, length, ENCODING )  -- sub string of international language\n";
		str += "                                      ENCODING: 'UTF8', 'GBK', 'GB2312', 'GB10830'\n";
		str += "    substring( EXPR(COL), start, length)             -- same as substr() above\n";
		str += "    substring( EXPR(COL), start, length, ENCODING )  -- same as substr() with encoding\n";
		str += "    strdiff( EXPR(COL), EXPR(COL) )  -- Levenshtein (or edit) distance between two strings\n";
		str += "    upper( EXPR(COL) )  -- upper case string of column expression\n";
		str += "    lower( EXPR(COL) )  -- lower case string of column expression\n";
		str += "    ltrim( EXPR(COL) )  -- remove leading white spaces of string column expression\n";
		str += "    rtrim( EXPR(COL) )  -- remove trailing white spaces of string column expression\n";
		str += "    trim( EXPR(COL) )   -- remove leading and trailing white spaces of string column expression\n";
		str += "    length( EXPR(COL) ) -- length of string column expression\n";
		str += "\n";

		str += "    second( TIMECOL )   -- value of second in a time column, 0-59\n";
		str += "    minute( TIMECOL )   -- value of minute in a time column, 0-59\n";
		str += "    hour( TIMECOL )     -- value of hour in a time column, 0-23\n";
		str += "    day( TIMECOL )      -- value of day in a time column, 1-31\n";
		str += "    month( TIMECOL )    -- value of month in a time column, 1-12\n";
		str += "    year( TIMECOL )     -- value of year in a time column\n";
		str += "    date( TIMECOL )     -- date in YYYY-MM-DD in a time column\n";
		str += "    datediff(type, BEGIN_TIMECOL, END_TIMECOL )  -- difference of two time columns\n";
		str += "             type: second  (difference in seconds)\n";
		str += "             type: minute  (difference in minutes)\n";
		str += "             type: hour    (difference in hours)\n";
		str += "             type: day     (difference in days)\n";
		str += "             type: month   (difference in months)\n";
		str += "             type: year    (difference in years)\n";
		str += "             The result is the value of (END_TIMECOL - BEGIN_TIMECOL) \n";
		str += "    dayofmonth( TIMECOL )  -- the day of the month in a time column (1-31)\n";
		str += "    dayofweek( TIMECOL )   -- the day of the week in a time column (0-6)\n";
		str += "    dayofyear( TIMECOL )   -- day of the year in a time column (1-366)\n";
		str += "    curdate()              -- current date (yyyy-mm-dd) in client time zone\n";
		str += "    curtime()              -- current time (hh:mm:ss) in client time zone\n";
		str += "    now()                  -- current date and time (yyyy-dd-dd hh:mm:ss) in client time zone\n";
		str += "    time()                 -- current time in seconds since the Epoch (1970-01-01 00:00:00)\n";
		str += "    tosecond('PATTERN')    -- convert PATTERN to seconds. PATTERN: xM, xH, xD, xW. x is a number.\n";
		str += "    tomicrosecond('PATTERN')  -- convert PATTERN to microseconds. PATTERN: xS, xM, xH, xD, xW. x is a number.\n";
		str += "    window(PATTERN, TIMECOL)  -- devide timecol by windows. PATTERN: xs, xm, xh, xd, xw, xM, xy, xD. x is a number.\n";
		str += "\n";

		str += "    metertomile(METERS)            -- convert meters to miles\n";
		str += "    miletometer(MILES)             -- convert miles to meters\n";
		str += "    kilometertomile(KILOMETERS)    -- convert kilometers to miles\n";
		str += "    miletokilometer(MILES     )    -- convert miles to kilometers\n";
		str += "    within(geom1, geom2)           -- check if shape geom1 is within another shape geom2\n";
		str += "    nearby(geom1, geom2, radius)   -- check if shape geom1 is close to another shape geom2 by distance radius\n";
		str += "    intersect(geom1, geom2 )       -- check if shape geom1 intersects another shape geom2\n";
		str += "    coveredby(geom1, geom2 )       -- check if shape geom1 is covered by another shape geom2\n";
		str += "    cover(geom1, geom2 )           -- check if shape geom1 covers another shape geom2\n";
		str += "    contain(geom1, geom2 )         -- check if shape geom1 contains another shape geom2\n";
		str += "    disjoint(geom1, geom2 )        -- check if shape geom1 and geom2 are disjoint\n";
		str += "    distance(geom1, geom2, 'TYPE') -- compute distance between shape geom1 and geom2. TYPE: min, max, center\n";
		str += "    distance(geom1, geom2 )        -- compute distance between shape geom1 and geom2. TYPE: center\n";
		str += "    area(geom)                     -- compute area or surface area of 2D or 3D objects\n";
		str += "    all(geom)                      -- get GeoJSON data of 2D or 3D objects\n";
		str += "    dimension(geom)                -- get dimension as integer of a shape geom \n";
		str += "    geotype(geom)                  -- get type as string of a shape geom\n";
		str += "    pointn(geom,n)                 -- get n-th point (1-based) of a shape geom. (x y [z])\n";
		str += "    extent(geom)                   -- get bounding box of a shape geom, resulting rectangle or box\n";
		str += "    startpoint(geom)               -- get start point of a line string geom. (x y [z])\n";
		str += "    endpoint(geom)                 -- get end point of a line string geom. (x y [z])\n";
		str += "    isclosed(geom)                 -- check if points of a line string geom is closed. (0 or 1)\n";
		str += "    numpoints(geom)                -- get total number of points of a line string or polygon\n";
		str += "    numsegments(geom)              -- get total number of line segments of linestring or polygon\n";
		str += "    numrings(geom)                 -- get total number of rings of a polygon or multipolygon\n";
		str += "    numpolygons(geom)              -- get total number of polygons of a multipolygon\n";
		str += "    srid(geom)                     -- get SRID of a shape geom\n";
		str += "    summary(geom)                  -- get a text summary of a shape geom\n";
		str += "    similarity(v1, v2, 'TYPE')     -- compute distance between shape geom1 and geom2. TYPE: min, max, center\n";
		str += "    xmin(geom)                     -- get the minimum x-coordinate of a shape with raster data\n";
		str += "    ymin(geom)                     -- get the minimum y-coordinate of a shape with raster data\n";
		str += "    zmin(geom)                     -- get the minimum z-coordinate of a shape with raster data\n";
		str += "    xmax(geom)                     -- get the maximum x-coordinate of a shape with raster data\n";
		str += "    ymax(geom)                     -- get the maximum y-coordinate of a shape with raster data\n";
		str += "    zmax(geom)                     -- get the maximum z-coordinate of a shape with raster data\n";
		str += "    xminpoint(geom)                -- get the coordinates of minimum x-coordinate of a shape\n";
		str += "    yminpoint(geom)                -- get the coordinates of minimum y-coordinate of a shape\n";
		str += "    zminpoint(geom)                -- get the coordinates of minimum z-coordinate of a 3D shape\n";
		str += "    xmaxpoint(geom)                -- get the coordinates of maximum x-coordinate of a shape\n";
		str += "    ymaxpoint(geom)                -- get the coordinates of maximum y-coordinate of a shape\n";
		str += "    zmaxpoint(geom)                -- get the coordinates of maximum z-coordinate of a 3D shape\n";
		str += "    convexhull(geom)               -- get the convex hull of a shape with raster data\n";
		str += "    centroid(geom)                 -- get the centroid coordinates of a vector or raster shape\n";
		str += "    volume(geom)                   -- get the volume of a 3D shape\n";
		str += "    closestpoint(point(x y), geom) -- get the closest point on geom from point(x y)\n";
		str += "    angle(line(x y), geom)         -- get the angle in degrees between two lines\n";
		str += "    buffer(geom, 'STRATEGY')       -- get polygon buffer of a shape. The STRATEGY is:\n";
		str += "                 distance=symmetric/asymmetric:RADIUS,join=round/miter:N,end=round/flat,point=circle/square:N\n";
		str += "    linelength(geom)               -- get length of line/3d, linestring/3d, multilinestring/3d\n";
		str += "    perimeter(geom)                -- get perimeter length of a closed shape (vector or raster)\n";
		str += "    equal(geom1,geom2)             -- check if shape geom1 is exactly the same as shape geom2\n";
		str += "    issimple(geom)                 -- check if shape geom has no self-intersecting or tangent points\n";
		str += "    isvalid(geom)                  -- check if multipoint, linestring, polygon, multilinestring, multipolygon is valid\n";
		str += "    isring(geom)                   -- check if linestring is a ring\n";
		str += "    ispolygonccw(geom)             -- check if the outer ring is counter-clock-wise, inner rings clock-wise\n";
		str += "    ispolygoncw(geom)              -- check if the outer ring is clock-wise, inner rings couter-clock-wise\n";
		str += "    outerring(polygon)             -- the outer ring as linestring of a polygon\n";
		str += "    outerrings(mpolygon)           -- the outer rings as multilinestring of a multipolygon\n";
		str += "    innerrings(polygon)            -- the inner rings as multilinestring of a polygon or multipolygon\n";
		str += "    ringn(polygon,n)               -- the n-th ring as linestring of a polygon. n is 1-based\n";
		str += "    innerringn(polygon,n)          -- the n-th inner ring as linestring of a polygon. n is 1-based\n";
		str += "    polygonn(multipgon,n)          -- the n-th polygon of a multipolygon. n is 1-based\n";
		str += "    unique(geom)                   -- geom with consecutive duplicate points removed\n";
		str += "    union(geom1,geom2)             -- union of two geoms. Polygon outer ring should be counter-clock-wise\n";
		str += "    collect(geom1,geom2)           -- collection of two geoms\n";
		str += "    topolygon(geom)                -- converting square, rectangle, circle, ellipse, triangle to polygon\n";
		str += "    topolygon(geom,N)              -- converting circle, ellipse to polygon with N points\n";
		str += "    text(geom)                     -- text string of a geometry shape\n";
		str += "    difference(g1,g2)              -- g1 minus the common part of g1 and g2\n";
		str += "    symdifference(g1,g2)           -- g1+g2 minus the common part of g1 and g2\n";
		str += "    isconvex(pgon)                 -- check if the outer ring of a polygon is convex\n";
		str += "    interpolate(lstr,frac)         -- the point on linestring where linelength is at a fraction (0.0-1.0)\n";
		str += "    linesubstring(lstr,startfrac,endfrac)  -- substring of linestring between start fraction and end fraction\n";
		str += "    locatepoint(lstr,point)        -- fraction where on linestring a point is closest to a given point\n";
		str += "    addpoint(lstr,point)           -- add a point to end of a linestring\n";
		str += "    addpoint(lstr,point,position)  -- add a point to the position of a linestring\n";
		str += "    setpoint(lstr,point,position)  -- change a point at position of a linestring\n";
		str += "    removepoint(lstr,position)     -- remove a point at position of a linestring\n";
		str += "    reverse(geom)                  -- reverse the order of points on a line, linestring, polygon, and multipolygon\n";
		str += "    scale(geom,factor)             -- scale the coordinates of geom by a factor\n";
		str += "    scale(geom,xfac,yfac)          -- scale the x-y coordinates of geom by factors\n";
		str += "    scale(geom,xfac,yfac,zfac)     -- scale the x-y-z coordinates of geom by factors\n";
		str += "    scaleat(geom,point,factor)     -- scale the coordinates of geom relative to a point by a factor\n";
		str += "    scaleat(geom,point,xfac,yfac)  -- scale the coordinates of geom relative to a point by a factor\n";
		str += "    scaleat(geom,point,xfac,yfac,zfac) -- scale the coordinates of geom relative to a point by a factor\n";
		str += "    scalesize(geom,factor)         -- scale the size of vector shapes by a factor\n";
		str += "    scalesize(geom,xfac,yfac)      -- scale the size of vector shapes by diffeent factors\n";
		str += "    scalesize(geom,xfac,yfac,zfac) -- scale the size of vector shapes by diffeent factors\n";
		str += "    translate(geom,dx,dy)          -- translate the location of 2D geom by dx,dy\n";
		str += "    translate(geom,dx,dy,dz)       -- translate the location of 3D geom by dx,dy,dz\n";
		str += "    transscale(geom,dx,dy,fac)     -- translate and then scale 2D geom\n";
		str += "    transscale(geom,dx,dy,xfac,yfac)  -- translate and then scale 2D geom with xfac, yfac factors\n";
		str += "    transscale(geom,dx,dy,dz,xfac,yfac,zfac)  -- translate and then scale 3D geom\n";
		str += "    rotate(geom,N)                 -- rotate 2D geom by N degrees counter-clock-wise with respect to point(0,0)\n";
		str += "    rotate(geom,N,'radian')        -- rotate 2D geom by N radians counter-clock-wise with respect to point(0,0)\n";
		str += "    rotate(geom,N,'degree')        -- rotate 2D geom by N degrees counter-clock-wise with respect to point(0,0)\n";
		str += "    rotateself(geom,N)             -- rotate 2D geom by N degrees counter-clock-wise with respect to self-center\n";
		str += "    rotateself(geom,N,'radian')    -- rotate 2D geom by N radians counter-clock-wise with respect to self-center\n";
		str += "    rotateself(geom,N,'degree')    -- rotate 2D geom by N degrees counter-clock-wise with respect to self-center\n";
		str += "    rotateat(geom,N,'radian',x,y)  -- rotate 2D geom by N radians counter-clock-wise with respect to point(x,y)\n";
		str += "    rotateat(geom,N,'degree',x,y)  -- rotate 2D geom by N degrees counter-clock-wise with respect to point(y,y)\n";
		str += "    affine(geom,a,b,d,e,dx,dy)     -- affine transformation on 2D geom\n";
		str += "    affine(geom,a,b,c,d,e,f,g,h,i,dx,dy,dz) -- affine transformation on 3D geom\n";
		str += "    voronoipolygons(mpoint)                 -- find Voronoi polygons from multipoints\n";
		str += "    voronoipolygons(mpoint,tolerance)       -- find Voronoi polygons from multipoints with tolerance\n";
		str += "    voronoipolygons(mpoint,tolerance,bbox)  -- find Voronoi polygons from multipoints with tolerance and bounding box\n";
		str += "    voronoilines(mpoint)                    -- find Voronoi lines from multipoints\n";
		str += "    voronoilines(mpoint,tolerance)          -- find Voronoi lines from multipoints with tolerance\n";
		str += "    voronoilines(mpoint,tolerance,bbox)     -- find Voronoi lines from multipoints with tolerance and bounding box\n";
		str += "    delaunaytriangles(mpoint)               -- find Delaunay triangles from multipoints\n";
		str += "    delaunaytriangles(mpoint,tolerance)     -- find Delaunay triangles from multipoints with tolerance\n";
		str += "    geojson(geom)                  -- GeoJSON string of geom\n";
		str += "    geojson(geom,N)                -- GeoJSON string of geom, receiving maximum of N points (default 3000)\n";
		str += "    geojson(geom,N,n)              -- GeoJSON string, receiving maximum of N points, n samples on 2D vector shapes\n";
		str += "    tomultipoint(geom)             -- converting geom to multipoint\n";
		str += "    tomultipoint(geom,N)           -- converting geom to multipoint. N is number of points for vector shapes\n";
		str += "    wkt(geom)                      -- Well Known Text of geom\n";
		str += "    minimumboundingcircle(geom)    -- minimum bounding circle of 2D geom\n";
		str += "    minimumboundingsphere(geom)    -- minimum bounding sphere of 3D geom\n";
		str += "    isonleft(geom1,geom2)          -- check if geom1 is on the left of geom2 (point and linear objects)\n";
		str += "    isonright(geom1,geom2)         -- check if geom1 is on the right of geom2 (point and linear objects)\n";
		str += "    leftratio(geom1,geom2)         -- ratio of geom1 on the left of geom2 (point and linear objects)\n";
		str += "    rightratio(geom1,geom2)        -- ratio of geom1 on the right of geom2 (point and linear objects)\n";
		str += "    knn(geom,point,K)              -- K-nearest neighbors in geom of point\n";
		str += "    knn(geom,point,K,min,max)      -- K-nearest neighbors in geom of point within maximum and mininum distance\n";
		str += "    metricn(geom)                  -- metrics of vector shapes\n";
		str += "    metricn(geom,N)                -- metric of N-th point. If vector shape, N-th metric\n";
		str += "    metricn(geom,N,m)              -- metric of N-th point, m-th metric. 1-based\n";
		str += "\n";
		str += "Example:\n";
		str += "select sum(amt) as amt_sum from sales limit 3;\n";
		str += "select cos(lat), sin(lon) from map limit 3;\n";
		str += "select tan(lat+sin(lon)), cot(lat^2+lon^2) from map limit 3;\n";
		str += "select uid, uid+addr, length(uid+addr)  from user limit 3;\n";
		str += "select price/2.0 + 1.25 as newprice, lead*1.25 - 0.3 as newlead from plan limit 3;\n";
		str += "select * from tm where dt < time() - tomicrosecond('1D');\n";
		str += "select angle(c1,c2) from g3 where id < 100;\n";
		str += "select buffer(col2, 'distance=symmetric:20,join=round:20,end=round') as buf from g2;\n";
		str += "select perimeter(squares) as perim from myshape;\n";
		str += "select ringn(poly, 2) as ring2 from myshape;\n";
		str += "select interpolate(lstr, 0.5) as midpoint from myline;\n";
		str += "select linesubstring(lstr, 0.2, 0.8) as midseg from myline where a=100;\n";
		str += "select rotate(lstr, 30, 'degree' ) as rot from myline where a=100;\n";
	} else if ( 0 == strncasecmp( cmd, "update", 6 ) ) {
		str += "update TABLE set VALUE='...', VALUE='...', ... where KEY1='...' and KEY2='...', ... ;\n";
		str += "update TABLE set VALUE='...', VALUE='...', ... where KEY1>='...' and KEY2>='...', ...;\n";
		str += "update TABLE set VALUE='...', ... where KEY='...' and VALUE='...', ...;\n";
		str += "update TABLE set VALUE=VALUE+N where KEY='...';\n";
		str += "\n";
		str += "Example:\n";
		str += "update user set address='200 Main St., SR, CA 94506' where fname='Sam' and lname='Walter';\n";
		str += "update user set fname='Tim', address='201 Main St., SR, CA 94506' where fname='Sam' and lname='Walter';\n";
	/***
	} else if ( 0 == strncasecmp( cmd, "upsert", 6 ) ) {
		str += "upsert is same as insert except that when the key exists in the table,\n";
		str += "the old record is replaced with the new record.\n";
	***/
	} else if ( 0 == strncasecmp( cmd, "delete", 3 ) ) {
		str += "delete from TABLE;\n";
		str += "delete from TABLE where KEY='...' and KEY='...' and ... ;\n";
		str += "delete from TABLE where KEY>='...' and KEY<='...' and ... ;\n";
		str += "\n";
		str += "Example:\n";
		str += "delete from junktable;\n";
		str += "delete from user where fname='Sam' and lname='Walter';\n";
	} else if ( 0 == strncasecmp( cmd, "drop", 4 ) ) {
		str += "drop table [if exists] [force] TABLENAME;\n"; 
		str += "drop index [if exists] INDEXNAME on TABLENAME;\n"; 
		str += "drop index [if exists] INDEXNAME in TABLENAME;\n"; 
		str += "drop index [if exists] INDEXNAME from TABLENAME;\n"; 
		str += "dropuser <username>;\n"; 
		str += "drop user <username>;\n"; 
		str += "\n";
		str += "Example:\n";
		str += "drop table user;\n";
		str += "drop table if exists user;\n";
		str += "drop table force user123;\n";
		str += "drop index user_idx9 on user;\n";
		str += "drop index if exists user_idx9 on user;\n";
		str += "dropuser iamjon;\n";
		str += "drop user iamjon;\n";
	} else if ( 0 == strncasecmp( cmd, "truncate", 3 ) ) {
		str += "truncate table TABLE;\n"; 
		str += "\n";
		str += "Data in table will be deleted, but the table tableschema still exists.\n";
		str += "\n";
		str += "Example:\n";
		str += "truncate table user; \n";
	/***
	} else if ( 0 == strncasecmp( cmd, "rename", 3 ) ) {
		str += "rename table OLDTABLE to NEWTABLE; \n"; 
		str += "\n";
		str += "Rename old table OLDTABLE to new table NEWTABLE.\n";
		str += "\n";
		str += "Example:\n";
		str += "rename table myoldtable to newtable;\n";
	***/
	} else if ( 0 == strncasecmp( cmd, "alter", 3 ) ) {
		str += "alter table TABLE add COLNAME TYPE;\n";
		str += "alter table TABLE rename OLDKEY to NEWKEY;\n";
		str += "alter table TABLE rename OLDCOL to NEWCOL;\n";
		str += "alter table TABLE set COL:ATTR to VALUE;\n";
		str += "alter table TABLE add tick(TICK:RERENTION);\n";
		str += "alter table TABLE drop tick(TICK);\n";
		str += "alter table TABLE retention RERENTION;\n";
		str += "\n";
		str += "Add a new column in table TABLE.\n";
		str += "Rename a key column name in table TABLE.\n";
		str += "Rename a value column name in table TABLE.\n";
		str += "Set an attribute ATTR of column COL to value VALUE.\n";
		str += "Add a new tick to a timeseries table.\n";
		str += "Drop a tick from a timeseries table.\n";
		str += "Change the retention of a timeseries table.\n";
		str += "\n";
		str += "Example:\n";
		str += "alter table mytable add zipcode char(6);\n";
		str += "alter table mytable rename mykey to userid;\n";
		str += "alter table mytable rename col2 to col3;\n";
		str += "alter table mytable set sq:srid to 4326;\n";
		str += "alter table traffic add tick(3M:24M);\n";
		str += "alter table traffic drop tick(3M);\n";
		str += "alter table traffic retention 12M;\n";
		str += "alter table traffic retention 0;\n";
		str += "alter table traffic@1h retention 24h;\n";
		str += "alter table traffic@1M retention 24M;\n";
	} else if ( 0 == strcasecmp( cmd, "load" ) ) {
		str += "load /path/input.txt into TABLE [columns terminated by C] [lines terminated by N] [quote terminated by Q];\n"; 
		str += "(Instructions inside [ ] are optional. /path/input.txt is located on client host.)\n";
		str += "Default values: \n";
		str += "   columns terminated by: ','\n";
		str += "   lines terminated by: '\\n'\n";
		str += "   columns quoted by singe quote (') character.\n";
		str += "\n";
		str += "Example:\n";
		str += "load /tmp/input.txt into user columns terminated by ',';\n"; 
		str += "load /tmp/input.txt into user quote terminated by '\\'';\n"; 
		str += "load /tmp/input.txt into user quote terminated by '\"';\n"; 
		str += "\n";
	/***
	} else if ( 0 == strncasecmp( cmd, "copy", 4 ) ) {
		str += "copy /path/input.txt into TABLE [columns terminated by C] [lines terminated by N] [quote terminated by Q];\n"; 
		str += "(Instructions inside [ ] are optional. /path/input.txt is located on single server host.)\n";
		str += "Default values: \n";
		str += "   columns terminated by: ','\n";
		str += "   lines terminated by: '\\n'\n";
		str += "   columns can be quoted by singe quote (') character.\n";
		str += "\n";
		str += "Example:\n";
		str += "copy /tmp/input.txt into user columns terminated by ',';\n"; 
		str += "\n";
	***/
	} else if ( 0 == strncasecmp( cmd, "spool", 5 ) ) {
		str += "spool LOCALFILE;\n";
		str += "spool off;\n";
		str += "\n";
		str += "Example:\n";
		str += "spool /tmp/myout.txt;\n";
		str += "(The above command will make the output data to be written to file /tmp/myout.txt)\n";
		str += "\n";
		str += "spool off;\n";
		str += "(The above command will stop writing output data to any file)\n";
		str += "\n";
	} else if ( 0 == strncasecmp( cmd, "source", 6 ) ) {
		str += "source SQLFILE;\n";
		str += "@SQLFILE;\n";
		str += "Execute commands from file SQLFILE";
		str += "\n";
		str += "Example:\n";
		str += "source /tmp/myout.sql;\n";
		str += "@/tmp/myout.sql;\n";
		str += "(The above commands will execute the commands  in /tmp/myout.txt)\n";
		str += "\n";
	} else if ( 0 == strncasecmp( cmd, "shell", 5 ) ) {
		str += "!command;\n";
		str += "shell command;\n";
		str += "Execute shell command";
		str += "\n";
		str += "Example:\n";
		str += "!cat my.sql;\n";
		str += "shell ls -l /tmp;\n";
		str += "(The above shell commands are executed.)\n";
		str += "\n";
	} else {
		str += Jstr("Command not supported [") + cmd + "]\n";
	}

	// str += "\n";

	//sendMessageLength( req, str.c_str(), str.size(), "I_" );
	dn("s3030491 helpTopic() sendDataEnd str=[%s]", str.s() );
	sendDataEnd( req, str);
}

void JagDBServer::showCurrentDatabase( JagCfg *cfg, const JagRequest &req, const Jstr &dbname )
{
	Jstr res = dbname;
	sendDataEnd( req, res);
}

void JagDBServer::showCluster( JagCfg *cfg, const JagRequest &req )
{
	Jstr res;
    res = _dbConnector->_nodeMgr->getClusterHosts();
	sendDataEnd( req, res);
}

void JagDBServer::showDatabases( JagCfg *cfg, const JagRequest &req )
{
	Jstr res;
	res = JagSchema::getDatabases( cfg, req.session->replicType );
	sendDataEnd( req, res);
}

void JagDBServer::showCurrentUser( JagCfg *cfg, const JagRequest &req )
{
	Jstr res = req.session->uid;
	sendDataEnd( req, res);
}

void JagDBServer::showClusterStatus( const JagRequest &req )
{
	// client expects: "numservs|numDBs|numTables|selects|inserts|updates|deletes|usersessions"
	char buf[1024];
	Jstr res = getClusterOpInfo( req );
	JagStrSplit sp( res, '|' );
	res = "Servers   Databases   Tables  Connections      Selects      Inserts      Updates      Deletes\n";
	res += "---------------------------------------------------------------------------------------------\n";
	sprintf( buf, "%7lld %6lld %8lld %12lld %12lld %12lld %12lld %12lld\n", 
				jagatoll( sp[0].c_str() ),
				jagatoll( sp[1].c_str() ),
				jagatoll( sp[2].c_str() ),
				jagatoll( sp[7].c_str() ),
				jagatoll( sp[3].c_str() ),
				jagatoll( sp[4].c_str() ),
				jagatoll( sp[5].c_str() ),
				jagatoll( sp[6].c_str() )  );

	res += buf;
	sendDataEnd( req, res);
}

void JagDBServer::showTools( const JagRequest &req )
{
	Jstr res;
	res += "Scripts in $JAGUAR_HOME/bin/tools/: \n";
	res += "\n";
	res += "check_servers_on_all_hosts.sh               -- check if server.conf is same on all hosts\n";
	res += "check_binaries_on_all_hosts.sh              -- check jaguar server program on all hosts\n";
	res += "check_config_on_all_hosts.sh                -- check each config parameter in server.conf on all hosts\n";
	//res += "check_datacenter_on_all_hosts.sh  -- check if datacenter.conf is same on all hosts\n";
	res += "check_log_on_all_hosts.sh [N]               -- display the last N (default 10) lines of jaguar.log on all hosts\n";
	res += "change_config_on_all_hosts.sh NAME VALUE    -- change the value of config paramter NAME  to VALUE on all hosts\n";
	res += "delete_config_on_all_hosts.sh NAME          -- delete the config paramter NAME on all hosts\n";
	res += "remove_config_on_all_hosts.sh NAME          -- comment out the config paramter NAME on all hosts\n";
	res += "restore_config_on_all_hosts.sh NAME         -- uncomment the config paramter NAME on all hosts\n";
	res += "list_configs_on_all_hosts.sh                -- list all config parameters in server.conf on all hosts\n";
	//res += "disable_datacenter_on_all_hosts.sh -- disable datacenter on all hosts\n";
	//res += "enable_datacenter_on_all_hosts.sh  -- enable datacenter on all hosts\n";
	res += "sshall \"command\"                          -- execute command on all hosts\n";
	res += "scpall LOCALFILE DESTDIR                    -- copy LOCALFILE to destination directory on all hosts\n";
	res += "setupsshkeys                                -- setup account public keys on all hosts\n";
	res += "createKeyPair.[bin|exe]                     -- generate server public and private keys on current host\n";
	res += "\n";

	sendDataEnd( req, res);
}

void JagDBServer::_showDatabases( JagCfg *cfg, const JagRequest &req )
{
	Jstr res;
	res = JagSchema::getDatabases( cfg, req.session->replicType );
	JagStrSplit sp(res, '\n', true);
	if ( sp.length() < 1 ) {
		//sendMessageLength( req, " ", 1, "KV" );
		sendEOM( req, "_showdberr");
		return;
	}

	for ( int i = 0; i < sp.length(); ++i ) {
		JagRecord rec;
		rec.addNameValue("TABLE_CAT", sp[i].c_str() );
		//sendMessageLength( req, rec.getSource(), rec.getLength(), "KV" );
		sendMessageLength( req, rec.getSource(), rec.getLength(), JAG_MSG_KV, JAG_MSG_NEXT_MORE);
    }
	sendEOM( req, "_showdbdone");
}

void JagDBServer::showTables( const JagRequest &req, const JagParseParam &pparam, 
							  const JagTableSchema *tableschema, 
							  const Jstr &dbname, int objType )
{
	JagVector<AbaxString> *vec = tableschema->getAllTablesOrIndexesLabel( objType, dbname, pparam.like );

	if ( vec ) {
		dn("s40298 showTables vec not null, vec->size=%d", vec->size() );
	}

	int i;
	AbaxString res;
	for ( i = 0; i < vec->size(); ++i ) {
		res += (*vec)[i] + "\n";
	}
	if ( vec ) delete vec;

	dn("s52230 showTables res=[%s]", res.c_str() );

	sendDataEnd( req, res.s());
}

void JagDBServer::_showTables( const JagRequest &req, const JagTableSchema *tableschema, const Jstr &dbname )
{
	/***
	TABLE_CAT String => table catalog (may be null)
	TABLE_SCHEM String => table schema (may be null)
	TABLE_NAME String => table name
	TABLE_TYPE String => table type. 
         Typical types are "TABLE", "VIEW", "SYSTEM TABLE", "GLOBAL TEMPORARY", "LOCAL TEMPORARY", "ALIAS", "SYNONYM".
	***/
	JagVector<AbaxString> *vec = tableschema->getAllTablesOrIndexes( dbname, "" );
	int i, len;
	//AbaxString res;

	JagRecord rrec;
	rrec.addNameValue("ISMETA", "Y");
	rrec.addNameValue("CMD", "SHOWTABLES");
	rrec.addNameValue("COLUMNCOUNT", "4");
	JagRecord colrec;
	colrec.addNameValue("TABLE_CAT", "0");
	colrec.addNameValue("TABLE_SCHEM", "0");
	colrec.addNameValue("TABLE_NAME", "0");
	colrec.addNameValue("TABLE_TYPE", "0");
	rrec.addNameValue("schema.table", colrec.getSource() );

	len = rrec.getLength();
	char buf[ 5+len];
	memcpy( buf, "m   |", 5 ); // meta data
	memcpy( buf+5, rrec.getSource(), rrec.getLength() );

	//sendMessageLength( req, buf, 5+len, "FH" );
	dn("s511008 sending JAG_MSG_HDR m JAG_MSG_NEXT_MORE ...");
	sendMessageLength( req, buf, 5+len, JAG_MSG_HDR, JAG_MSG_NEXT_MORE );

	for ( i = 0; i < vec->size(); ++i ) {
		const AbaxString &res = (*vec)[i];
		JagStrSplit split( res.c_str(), '.' );
		if ( split.size() < 3 ) continue;
		JagRecord rec;
		rec.addNameValue( "TABLE_CAT", split[0].c_str() );
		rec.addNameValue( "TABLE_SCHEM", split[0].c_str() );
		rec.addNameValue( "TABLE_NAME", split[1].c_str() );
		rec.addNameValue( "TABLE_TYPE", "TABLE" );

		//sendMessageLength( req, rec.getSource(), rec.getLength(), "KV" );
		sendMessageLength( req, rec.getSource(), rec.getLength(), JAG_MSG_KV, JAG_MSG_NEXT_MORE);
		// KV records
	}

	sendEOM(req, "_shtabs");
	if ( vec ) delete vec;
	vec = NULL;

}

void JagDBServer::showIndexes( const JagRequest &req, const JagParseParam &pparam, 
							   const JagIndexSchema *indexschema, const Jstr &dbtable )
{
	JagVector<AbaxString> *vec = indexschema->getAllTablesOrIndexes( dbtable, pparam.like );
	int i;
	AbaxString res;
	for ( i = 0; i < vec->size(); ++i ) {
		res += (*vec)[i] + "\n";
	}
	if ( vec ) delete vec;
	vec = NULL;

	sendDataEnd( req, res.s());
}

void JagDBServer::showAllIndexes( const JagRequest &req,  const JagParseParam &pparam, const JagIndexSchema *indexschema, 
								const Jstr &dbname )
{
	JagVector<AbaxString> *vec = indexschema->getAllIndexes( dbname, pparam.like );
	int i;
	AbaxString res;
	for ( i = 0; i < vec->size(); ++i ) {
		res += (*vec)[i] + "\n";
	}
	if ( vec ) delete vec;
	vec = NULL;

	sendDataEnd( req, res.s());
}

void JagDBServer::_showIndexes( const JagRequest &req, const JagIndexSchema *indexschema, const Jstr &dbtable )
{
	//sendMessageLength( req, " ", 1, "KV" );
	sendEOM(req, "_shindxs");
}

void JagDBServer::showTask( const JagRequest &req )
{
    int running;
	Jstr str = getTasks(req, running);
	//sendMessageLength( req, str.c_str(), str.size(), "I_" );
	sendDataEnd( req, str);
}

Jstr JagDBServer::getTasks( const JagRequest &req, int &running )
{
	Jstr str;
	char buf[1024];
	const AbaxPair<AbaxLong,AbaxString> *arr = _taskMap->array();
	jagint len = _taskMap->arrayLength();
	sprintf( buf, "%14s  %20s  %16s  %16s  %16s %s\n", "TaskID", "ThreadID", "User", "Database", "StartTime", "Command");
	str += Jstr( buf );
	sprintf( buf, "------------------------------------------------------------------------------------------------------------\n");
	str += Jstr( buf );

    running = 0;
	for ( jagint i = 0; i < len; ++i ) {
		if ( ! _taskMap->isNull( i ) ) {
			const AbaxPair<AbaxLong,AbaxString> &pair = arr[i];
			JagStrSplit sp( pair.value.c_str(), '|' );
			// "threadID|userid|dbname|timestamp|query"
			sprintf( buf, "%14lld  %20s  %16s  %16s  %16s %s\n", 
				pair.key.value(), sp[0].c_str(), sp[1].c_str(), sp[2].c_str(), sp[3].c_str(), sp[4].c_str() );
			str += Jstr( buf );
            ++running;
		}
	}

	return str;
}

Jstr JagDBServer::describeTable( int inObjType, const JagRequest &req, 
								const JagTableSchema *tableschema, const Jstr &dbtable, 
								bool showDetail, bool showCreate, bool forRollup, const Jstr &tserRetain )
{
	const JagSchemaRecord *record = tableschema->getAttr( dbtable );
	if ( ! record ) return "";

	Jstr tname;
	Jstr retain;
	if ( forRollup ) {
		if ( tserRetain.containsChar('_') ) {
			JagStrSplit sp2( tserRetain, '_');
			tname = sp2[0];
			retain = sp2[1];
		} else {
			tname = tserRetain;
			retain = "0";
		}
	}

	Jstr res;
	Jstr type, dbcol, defvalStr;
	char buf[16];
	char spare4;
	int  offset, len, deflen;
	bool enteredValue = false;

	int objtype = tableschema->objectType ( dbtable );
	if ( JAG_TABLE_TYPE == inObjType ) {
		if ( objtype != JAG_TABLE_TYPE && objtype != JAG_MEMTABLE_TYPE ) {
			return "";
		}
	} else if ( JAG_CHAINTABLE_TYPE == inObjType ) {
		if ( objtype != JAG_CHAINTABLE_TYPE ) {
			return "";
		}
	} 

	if ( showCreate ) { res = Jstr("create "); } 
	if ( JAG_MEMTABLE_TYPE == objtype ) {
		res += Jstr("memtable "); 
	} else if ( JAG_CHAINTABLE_TYPE == objtype ) {
		res += Jstr("chain "); 
	} else {
		res += Jstr("table "); 
	}

	Jstr systser;
	if ( record->hasTimeSeries( systser ) ) {
		if ( ! forRollup ) {
			Jstr inputTser = record->translateTimeSeriesBack( systser );
			Jstr retention = record->timeSeriesRentention();
			if ( retention == "0" ) {
				res += Jstr("timeseries(") + inputTser + ") " + dbtable+"\n";
			} else {
				res += Jstr("timeseries(") + inputTser + "|" + retention + ") " + dbtable+"\n";
			}
		} else {
			res += Jstr("timeseries(0|") + retain + ") " + dbtable + "@" + tname + "\n";
		}
	} else {
		if ( strchr( dbtable.s(), '@' ) ) {
			Jstr retention = record->timeSeriesRentention();
			if ( retention == "0" ) {
				res += dbtable + "\n";
			} else {
				res += dbtable + "|" + retention + "\n";
			}
		} else {
			res += dbtable + "\n";
		}
	}

	res += Jstr("(\n");
	res += Jstr("  key:\n");

	bool isRollupCol;
	bool doConvertSec = false;
	if ( forRollup ) { doConvertSec = true; } // first time do conversion
	bool  doneConvertSec;
	Jstr colStr;
	Jstr colName;
	Jstr colType;

    const JagVector<JagColumn> &rcv = *(record->columnVector);
	int sz = rcv.size();

	// look at each column
	for (int i = 0; i < sz; i++) {
		colName = rcv[i].name.c_str();
		colType = rcv[i].type;

		colStr = "";
		if ( ! showDetail ) {
			if ( rcv[i].spare[6] == JAG_SUB_COL ) {
				continue;
			}

			if ( 0==jagstrncmp( rcv[i].name.c_str(), "geo:", 4 ) ) {
				// geo:*** ommit
				continue;
			}

		} else {
			if ( rcv[i].spare[6] == JAG_SUB_COL ) {
				colStr += Jstr("  ");
			}
		}

		if ( showCreate && rcv[i].name == "spare_" ) {
			continue;
		}

		if ( rcv[i].spare[7] == JAG_ROLL_UP ) {
			isRollupCol = true;
		} else {
			isRollupCol = false;
		}


		if ( !enteredValue && rcv[i].spare[0] == JAG_C_COL_VALUE ) {
			enteredValue = true;
			res += Jstr("  value:\n");
		}

		// if forRollup, skip the non-rollup columns
		if ( enteredValue && forRollup && ! isRollupCol ) {
			continue;
		}

		colStr += Jstr("    ");
		colStr += colName + " ";

		offset = rcv[i].offset;
		len = rcv[i].length;
		deflen = rcv[i].spare[9];

		dbcol = dbtable + "." + rcv[i].name.c_str();

		if ( rcv[i].spare[5] == JAG_KEY_MUTE ) {
			colStr += Jstr("mute ");
		}

		if ( isRollupCol && ! dbtable.containsChar('@') ) {
			colStr += Jstr("rollup ");
		}

		colStr += fillDescBuf( tableschema, rcv[i], dbtable, doConvertSec, doneConvertSec );
		spare4 =  rcv[i].spare[4];

		if ( JAG_CREATE_DEFDATE == spare4 || JAG_CREATE_DEFDATETIMESEC == spare4 ||
			 JAG_CREATE_DEFDATETIMEMILL == spare4 ||
		     JAG_CREATE_DEFDATETIME == spare4 || JAG_CREATE_DEFDATETIMENANO == spare4 ) {
			colStr += " DEFAULT CURRENT_TIMESTAMP";
		} else if ( JAG_CREATE_DEFUPDATE_DATE == spare4 || JAG_CREATE_DEFUPDATE_DATETIMESEC == spare4 || 
					JAG_CREATE_DEFUPDATE_DATETIMEMILL == spare4 ||
				    JAG_CREATE_DEFUPDATE_DATETIME == spare4 || JAG_CREATE_DEFUPDATE_DATETIMENANO == spare4 ) {
			colStr += " DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP";
		} else if ( JAG_CREATE_UPDATE_DATE == spare4 || JAG_CREATE_UPDATE_DATETIMESEC == spare4 || 
					JAG_CREATE_UPDATE_DATETIMEMILL == spare4 ||
		            JAG_CREATE_UPDATE_DATETIME == spare4 || JAG_CREATE_UPDATE_DATETIMENANO == spare4  ) {
			colStr += " ON UPDATE CURRENT_TIMESTAMP";
		} else if ( JAG_CREATE_DEFINSERTVALUE == spare4 ) {
			defvalStr = "";
			tableschema->getAttrDefVal( dbcol, defvalStr );
			if ( type == JAG_C_COL_TYPE_DBIT ) {
				colStr += Jstr(" DEFAULT b") + defvalStr.c_str();
			} else {
				if ( rcv[i].spare[1] == JAG_C_COL_TYPE_ENUM[0] ) {
					JagStrSplitWithQuote sp( defvalStr.c_str(), '|' );
					colStr += Jstr(" DEFAULT ") + sp[sp.length()-1].c_str();
				} else {
					colStr += Jstr(" DEFAULT ") + defvalStr.c_str();
				}
			}
		}

		if ( showDetail ) {
            /***
            if ( deflen > 0 ) {
			    colStr += Jstr(" ") + intToStr(offset) + ":" + intToStr(deflen) + "-" + intToStr(len);
            } else {
			   colStr += Jstr(" ") + intToStr(offset) + ":" + intToStr(len);
            }
            ***/
			colStr += Jstr(" ") + intToStr(offset) + ":" + intToStr(len);

			#if 1
			sprintf(buf, "  %03d", i+1 );
			colStr += Jstr(buf);
			#endif

            dn("s761008 colName=%s defen=%d len=%d ", colName.s(), deflen, len );
		}

		colStr += Jstr(",\n");
		res += colStr;

		// append stat cols
		/////////////////////////////////////////////////////
    	if ( forRollup && ! dbtable.containsChar('@') ) {
			if ( rcv[i].iskey ) {
				continue;
			}

    		if ( ! isInteger( colType ) && ! isFloat(colType ) ) {
				continue;
			}

			colStr = "";
    		if ( isInteger( colType ) ) {
    			colStr += Jstr("    ") + colName + "::sum bigint,\n";
			} else {
    			colStr += Jstr("    ") + colName + "::sum longdouble(24.3),\n";
                dn("s202093 non-integer field double 24.3");
                // rollup column length
			}
			Jstr ct = getTypeStr( colType ); 
    		colStr += Jstr("    ") + colName + "::min " + ct + ",\n";
    		colStr += Jstr("    ") + colName + "::max " + ct + ",\n";
    		colStr += Jstr("    ") + colName + "::avg " + ct + ",\n";
    		colStr += Jstr("    ") + colName + "::var " + ct + ",\n";

			res += colStr;
    	}
	} // end of record->columnVector

	if ( forRollup ) {
		res += "    counter bigint default '1' ";
	}

	res += Jstr(");\n");
	return res;
}

void JagDBServer::sendValueData( const JagParseParam &pparm, const JagRequest &req )
{
	if ( ! pparm._lineFile ) {
        dn("s3520004 no lineFile return");
		return;
	}

	Jstr line;
	pparm._lineFile->startRead();
    jagint cnt = 0;

	while ( pparm._lineFile->getLine( line ) ) {
		//sendMessageLength( req, line.c_str(), line.size(), "JS" );
        dn("s12098208 sendMessageLength JAG_MSG_JS ...");
		sendMessageLength( req, line.c_str(), line.size(), JAG_MSG_JS, JAG_MSG_NEXT_MORE );
        ++ cnt;
	}
    dn("s352110 sendValueData is done cnt=%ld", cnt );
}

void JagDBServer::_describeTable( const JagRequest &req, const Jstr &dbtable, int keyOnly )
{
	JagTableSchema *tableschema =  getTableSchema( req.session->replicType );
	Jstr cols = "TABLE_CAT|TABLE_SCHEM|TABLE_NAME|COLUMN_NAME|COLUMN_NAME|DATA_TYPE|TYPE_NAME|COLUMN_SIZE|DECIMAL_DIGITS|NULLABLE|KEY_SEQ";
	JagStrSplit sp( cols, '|' );

	JagRecord rrec;
	rrec.addNameValue("ISMETA", "Y");
	rrec.addNameValue("CMD", "DESCRIBETABLE");
	rrec.addNameValue("COLUMNCOUNT", intToString(sp.length()).c_str() );
	JagRecord colrec;
	for ( int i = 0; i < sp.length(); ++i ) {
		colrec.addNameValue( sp[i].c_str(), "0");
	}
	rrec.addNameValue("schema.table", colrec.getSource() );

	int len = rrec.getLength();
	char buf5[ 5+len];
	memcpy( buf5, "m   |", 5 ); // meta data
	memcpy( buf5+5, rrec.getSource(), rrec.getLength() );
	//sendMessageLength( req, buf5, 5+len, "FH" );
	dn("s70012 sending JAG_MSG_HDR m ... JAG_MSG_NEXT_MORE");
	sendMessageLength( req, buf5, 5+len, JAG_MSG_HDR, JAG_MSG_NEXT_MORE );


	const JagSchemaRecord *record = tableschema->getAttr( dbtable );
	if ( ! record ) return;

	char buf[128];
	Jstr  type;
	JagStrSplit split( dbtable, '.' );
	Jstr dbname = split[0];
	Jstr tabname = split[1];
	int seq = 1;
	int iskey;
	char sig;

	for (int i = 0; i < record->columnVector->size(); i++) {
		JagRecord rec;
		iskey = (*(record->columnVector))[i].iskey;

		if ( iskey && (*(record->columnVector))[i].name == "_id" && 0 == i ) {
			continue;
		}

		if ( keyOnly && ! iskey ) {
			continue;
		}

		rec.addNameValue("TABLE_CAT", dbname.c_str() );
		rec.addNameValue("TABLE_SCHEM", dbname.c_str() );
		rec.addNameValue("TABLE_NAME", tabname.c_str() );
		rec.addNameValue("COLUMN_NAME", (*(record->columnVector))[i].name.c_str() );

		type = (*(record->columnVector))[i].type;
		len = (*(record->columnVector))[i].length;
		sig = (*(record->columnVector))[i].sig;
		iskey = (*(record->columnVector))[i].iskey;
		//printf("sig=[%c] %d\n", sig, sig );

		if ( type == JAG_C_COL_TYPE_STR ) {
			rec.addNameValue("DATA_TYPE", "1"); // char
			rec.addNameValue("TYPE_NAME", "CHAR"); // char
		} else if ( type == JAG_C_COL_TYPE_FLOAT ) {
			rec.addNameValue("DATA_TYPE", "6"); // float
			rec.addNameValue("TYPE_NAME", "FLOAT");
		} else if ( type == JAG_C_COL_TYPE_DOUBLE ) {
			rec.addNameValue("DATA_TYPE", "8"); // double
			rec.addNameValue("TYPE_NAME", "DOUBLE");
		} else if ( type == JAG_C_COL_TYPE_LONGDOUBLE ) {
			rec.addNameValue("DATA_TYPE", "9"); // longdouble
			rec.addNameValue("TYPE_NAME", "LONGDOUBLE");
		} else if ( type == JAG_C_COL_TYPE_TIMEMICRO ) {
			rec.addNameValue("DATA_TYPE", "92"); // time
			rec.addNameValue("TYPE_NAME", "TIME");
		} else if ( type == JAG_C_COL_TYPE_DATETIMESEC ||  type == JAG_C_COL_TYPE_DATETIMEMICRO
		            || type == JAG_C_COL_TYPE_DATETIMEMILLI || type == JAG_C_COL_TYPE_DATETIMENANO ) {
			rec.addNameValue("DATA_TYPE", "93"); // time
			rec.addNameValue("TYPE_NAME", "DATETIME");
		} else if ( type == JAG_C_COL_TYPE_TIMESTAMPMICRO || type == JAG_C_COL_TYPE_TIMESTAMPSEC
		            || type == JAG_C_COL_TYPE_TIMESTAMPMILLI || type == JAG_C_COL_TYPE_TIMESTAMPNANO ) {
			rec.addNameValue("DATA_TYPE", "94"); // time
			rec.addNameValue("TYPE_NAME", "TIMESTAMP");
		} else if ( type == JAG_C_COL_TYPE_DINT ) {
			rec.addNameValue("DATA_TYPE", "4"); 
			rec.addNameValue("TYPE_NAME", "INTEGER");
		} else if ( type == JAG_C_COL_TYPE_DBOOLEAN ) {
			rec.addNameValue("DATA_TYPE", "-2"); 
			rec.addNameValue("TYPE_NAME", "BINARY");
		} else if ( type == JAG_C_COL_TYPE_DBIT) {
			rec.addNameValue("DATA_TYPE", "-3"); 
			rec.addNameValue("TYPE_NAME", "BIT");
		} else if ( type == JAG_C_COL_TYPE_DTINYINT ) {
			rec.addNameValue("DATA_TYPE", "-6"); 
			rec.addNameValue("TYPE_NAME", "TINYINT");
		} else if ( type == JAG_C_COL_TYPE_DSMALLINT ) {
			rec.addNameValue("DATA_TYPE", "5"); 
			rec.addNameValue("TYPE_NAME", "TINYINT");
		} else if ( type == JAG_C_COL_TYPE_DBIGINT ) {
			rec.addNameValue("DATA_TYPE", "5"); 
			rec.addNameValue("TYPE_NAME", "BIGINT");
		} else if ( type == JAG_C_COL_TYPE_DMEDINT ) {
			rec.addNameValue("DATA_TYPE", "4"); 
			rec.addNameValue("TYPE_NAME", "INTEGER");
		} else if ( type == JAG_C_COL_TYPE_DATETIMENANO ) {
			rec.addNameValue("DATA_TYPE", "98"); // time
			rec.addNameValue("TYPE_NAME", "DATETIMENANO");
		} else if ( type == JAG_C_COL_TYPE_TIMESTAMPNANO ) {
			rec.addNameValue("DATA_TYPE", "99"); // time
			rec.addNameValue("TYPE_NAME", "TIMESTAMPNANO");
		} else {
			rec.addNameValue("DATA_TYPE", "12"); // varchar
			rec.addNameValue("TYPE_NAME", "VARCHAR");
		}

		memset(buf, 0, 128 );
		sprintf( buf, "%d", len );
		rec.addNameValue("COLUMN_SIZE", buf ); // bytes

		sprintf( buf, "%d", sig );
		rec.addNameValue("DECIMAL_DIGITS", buf ); 
		rec.addNameValue("NULLABLE", "0" ); // 

		if ( iskey ) {
			sprintf( buf, "%d", seq++ );
			rec.addNameValue("KEY_SEQ", buf ); // 
		}

		//sendMessageLength( req, rec.getSource(), rec.getLength(), "KV" );
		sendMessageLength( req, rec.getSource(), rec.getLength(), JAG_MSG_KV, JAG_MSG_NEXT_MORE );
	}

	sendEOM( req, "_descend");
}

Jstr JagDBServer::describeIndex( bool showDetail, const JagRequest &req, 
							     const JagIndexSchema *indexschema, 
								 const Jstr &dbname, const Jstr &indexName, Jstr &reterr,
								 bool showCreate, bool forRollup, const Jstr &tserRetain )
{
	Jstr tab = indexschema->getTableName( dbname, indexName );
	if ( tab.size() < 1 ) {
		reterr = Jstr("E12380 Error: table or index [") + indexName + "] not found";
		return "";
	}

	Jstr tname;
	Jstr retain;
	if ( forRollup ) {
		JagStrSplit sp2( tserRetain, '_');
		tname = sp2[0];
		retain = sp2[1];
	}

	Jstr dbtabidx = dbname + "." + tab + "." + indexName, dbcol;
	Jstr schemaText = indexschema->readSchemaText( dbtabidx );
	Jstr defvalStr;

	if ( schemaText.size() < 1 ) {
		reterr = Jstr("E12080 Error: Index ") + indexName + " not found";
		return "";
	}

	JagSchemaRecord record;
	record.parseRecord( schemaText.c_str() );

	Jstr res;
	Jstr type;
	int len, offset;
	bool enteredValue = false;
	bool doneConvertSec;

	if  ( showCreate ) {
		res = "create ";
	}

	int isMemIndex = indexschema->isMemTable( dbtabidx );
	if ( isMemIndex ) {
		res += Jstr("memindex ") + dbtabidx + "\n";
	} else {
		if ( forRollup ) {
			res += Jstr("index ") + dbtabidx + "@" + tname + "\n";
		} else {
			res += Jstr("index " ) + dbtabidx + "\n";
		}
	}

	if ( forRollup ) {
		res += Jstr(" on ") + tab + "@" + tname + " ";
	}

	res += Jstr("(\n");
	res += Jstr("  key:\n");
	int sz = record.columnVector->size();
	char spare4, spare7;
	Jstr colName, colType, colStr;

	const JagVector<JagColumn> &cv = *(record.columnVector);

	for (int i = 0; i < sz; i++) {
		colStr = "";
        colName = cv[i].name.c_str();
        colType = cv[i].type;

		if ( ! showDetail ) {
			if ( cv[i].spare[6] == JAG_SUB_COL ) {
				continue;
			}
		} else {
			if ( cv[i].spare[6] == JAG_SUB_COL ) {
				colStr += Jstr("  ");
			}
		}

		if ( !enteredValue && cv[i].spare[0] == JAG_C_COL_VALUE ) {
			enteredValue = true;
			colStr += Jstr("  value:\n");
		}


		spare7 = cv[i].spare[7];
		if ( ( ! cv[i].iskey ) && forRollup && ( JAG_ROLL_UP != spare7 ) ) {
			continue;
		}

		colStr += Jstr("    ");
		colStr += colName + " ";
		offset = cv[i].offset;
		len = cv[i].length;

		if ( forRollup ) {
			colStr += fillDescBuf( indexschema, cv[i], dbtabidx, true, doneConvertSec );
		} else {
			Jstr descbuf = fillDescBuf( indexschema, cv[i], dbtabidx, false, doneConvertSec );
			colStr += descbuf;
		}


		dbcol = dbtabidx + "." + colName;
		spare4 = cv[i].spare[4];
		if ( spare4 == JAG_CREATE_DEFDATETIME || spare4 == JAG_CREATE_DEFDATE ) {
			colStr += " DEFAULT CURRENT_TIMESTAMP";
		} else if ( spare4 == JAG_CREATE_DEFDATETIMESEC || spare4 == JAG_CREATE_DEFDATETIMENANO || spare4 == JAG_CREATE_DEFDATETIMEMILL ) {
			colStr += " DEFAULT CURRENT_TIMESTAMP";
		} else if ( spare4 == JAG_CREATE_DEFUPDATE_DATETIME || spare4 == JAG_CREATE_DEFUPDATE_DATE ) {
			colStr += " DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP";
		} else if ( spare4 == JAG_CREATE_DEFUPDATE_DATETIMENANO || spare4 == JAG_CREATE_DEFUPDATE_DATETIMESEC 
		            || spare4 == JAG_CREATE_DEFUPDATE_DATETIMEMILL ) {
			colStr += " DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP";
		} else if ( spare4 == JAG_CREATE_UPDATE_DATETIME || spare4 == JAG_CREATE_UPDATE_DATE ) {
			colStr += " ON UPDATE CURRENT_TIMESTAMP";
		} else if ( spare4 == JAG_CREATE_UPDATE_DATETIMESEC || spare4 == JAG_CREATE_UPDATE_DATETIMENANO 
					|| spare4 == JAG_CREATE_UPDATE_DATETIMEMILL ) {
			colStr += " ON UPDATE CURRENT_TIMESTAMP";
		} else if ( spare4 == JAG_CREATE_DEFINSERTVALUE ) {
			defvalStr = "";
			indexschema->getAttrDefVal( dbcol, defvalStr );
			if ( type == JAG_C_COL_TYPE_DBIT ) {
				colStr += Jstr(" DEFAULT b") + defvalStr.c_str();
			} else {
				if ( cv[i].spare[1] == JAG_C_COL_TYPE_ENUM[0]  ) {
					JagStrSplitWithQuote sp( defvalStr.c_str(), '|' );
					colStr += Jstr(" DEFAULT ") + sp[sp.length()-1].c_str();
				} else {
					colStr += Jstr(" DEFAULT ") + defvalStr.c_str();
				}
			}
		}

		if ( showDetail ) {
			colStr += Jstr(" ") + intToStr(offset) + ":" + intToStr(len);
		}
		
		//if ( i < sz-1 ) { colStr += ",\n"; } else { colStr += "\n"; }
		colStr += ",\n";
		res += colStr;

		if ( forRollup ) {
			if ( cv[i].iskey ) {
				continue;
			}
            if ( ! isInteger( colType ) && ! isFloat( colType ) ) {
                continue;
            }

            colStr = "";
			if ( isInteger( colType ) ) {
            	colStr += Jstr("    ") + colName + "::sum bigint,\n";
			} else {
            	colStr += Jstr("    ") + colName + "::sum longdouble(24.3),\n";
                // rollup column length, will be redced to b254 length
			}

			Jstr ct = getTypeStr( colType ); 
            colStr += Jstr("    ") + colName + "::min " + ct + ",\n";
            colStr += Jstr("    ") + colName + "::max " + ct + ",\n";
            colStr += Jstr("    ") + colName + "::avg " + ct + ",\n";
            colStr += Jstr("    ") + colName + "::var " + ct + ",\n";
            res += colStr;
		}
	}

	res += Jstr(");\n");
	return res;
}

// sequential
void JagDBServer::makeTableObjects( bool doRestoreInserBufferMap )
{
	AbaxString dbtab;
	JagRequest req;
	JagSession session;
	session.servobj = this;
	session.serverIP = _localInternalIP;

	req.session = &session;

	time_t t1 = time(NULL);
	int  lockrc;

	jd(JAG_LOG_HIGH, "Initialing all tables ...\n");

	JagParseParam pparam;
	ObjectNameAttribute objNameTemp;
	objNameTemp.init();
	pparam.objectVec.append(objNameTemp);
	pparam.optype = 'C';
	pparam.opcode = JAG_CREATETABLE_OP;
	
	JagTable *ptab;
	JagTableSchema *tableschema;
	tableschema = _tableschema;
	JagVector<AbaxString> *vec = _tableschema->getAllTablesOrIndexes( "", "" );

    if ( ! vec ) {
	    jd(JAG_LOG_LOW, "makeTableObjects: No objects found\n");
        return;
    }

    /////////////////  0
	req.session->replicType = 0;
	for ( int i = 0; i < vec->size(); ++i ) {
		dbtab = (*vec)[i];
	    jd(JAG_LOG_LOW, "Init object 0:[%s] ...\n", dbtab.s() );
		JagStrSplit split(  dbtab.c_str(), '.' );
		pparam.objectVec[0].dbName = split[0];
		pparam.objectVec[0].tableName = split[1];

        dn("s0001229603 dbName=[%s] tableName=[%s] writeLockTable JAG_CREATETABLE_OP", 
            pparam.objectVec[0].dbName.s(), pparam.objectVec[0].tableName.s() );

		ptab = _objectLock->writeLockTable( JAG_CREATETABLE_OP, pparam.objectVec[0].dbName, 
											 pparam.objectVec[0].tableName, tableschema, req.session->replicType, 0, lockrc );
		if ( ptab ) {
	        jd(JAG_LOG_LOW, "Object 0:[%s] buildInitIndexlist ...\n", dbtab.s() );
			ptab->buildInitIndexlist();
		}

		if ( ptab ) _objectLock->writeUnlockTable( JAG_CREATETABLE_OP, pparam.objectVec[0].dbName, 
												   pparam.objectVec[0].tableName, req.session->replicType, 0 );
	}
	if ( vec ) delete vec;

	///////////////  1
	req.session->replicType = 1;
	tableschema = _prevtableschema;
	vec = _prevtableschema->getAllTablesOrIndexes( "", "" );
	for ( int i = 0; i < vec->size(); ++i ) {
		dbtab = (*vec)[i];
	    jd(JAG_LOG_LOW, "Init object 1:[%s] ...\n", dbtab.s() );
		JagStrSplit split(  dbtab.c_str(), '.' );
		pparam.objectVec[0].dbName = split[0];
		pparam.objectVec[0].tableName = split[1];
		ptab = _objectLock->writeLockTable( JAG_CREATETABLE_OP, pparam.objectVec[0].dbName, 
											 pparam.objectVec[0].tableName, tableschema, req.session->replicType, 0, lockrc );
		if ( ptab ) {
	        jd(JAG_LOG_LOW, "Object 1:[%s] buildInitIndexlist ...\n", dbtab.s() );
			ptab->buildInitIndexlist();
		}
		if ( ptab ) _objectLock->writeUnlockTable( JAG_CREATETABLE_OP, pparam.objectVec[0].dbName, 
												   pparam.objectVec[0].tableName, req.session->replicType, 0 );
	}
	delete vec;

	///////////////   2
	req.session->replicType = 2;
	tableschema = _nexttableschema;
	vec = _nexttableschema->getAllTablesOrIndexes( "", "" );
	for ( int i = 0; i < vec->size(); ++i ) {
		dbtab = (*vec)[i];
	    jd(JAG_LOG_LOW, "Init object 2:[%s] ...\n", dbtab.s() );
		JagStrSplit split(  dbtab.c_str(), '.' );
		pparam.objectVec[0].dbName = split[0];
		pparam.objectVec[0].tableName = split[1];
		ptab = _objectLock->writeLockTable( JAG_CREATETABLE_OP, pparam.objectVec[0].dbName, 
													 pparam.objectVec[0].tableName, tableschema, req.session->replicType, 0, lockrc );
		if ( ptab ) {
	        jd(JAG_LOG_LOW, "Object 2:[%s] buildInitIndexlist ...\n", dbtab.s() );
			ptab->buildInitIndexlist();
		}

		if ( ptab ) _objectLock->writeUnlockTable( JAG_CREATETABLE_OP, pparam.objectVec[0].dbName, 
												   pparam.objectVec[0].tableName, req.session->replicType, 0 );
	}

	time_t  t2 = time(NULL);
	int min = (t2-t1)/60;
	int sec = (t2-t1) % 60;
	jd(JAG_LOG_LOW, "Initialized all %d tables in %d minutes %d seconds\n", vec->size(), min, sec );
	delete vec;

}

void JagDBServer::makeSysTables( int replicType)
{
    JagSession session;
    session.sock = 0;
    session.servobj = this;
    session.ip = "0";
    session.port = 0;
    session.active = 0;
	session.replicType = replicType;

	JagRequest req;
	req.session = &session;

	JagTableSchema *tableschema;
	JagIndexSchema *indexschema;
	getTableIndexSchema( req.session->replicType, tableschema, indexschema );

	Jstr dbname = "system";
	Jstr tableName = "_SYS_";
	Jstr reterr;

	Jstr dbtable = dbname + "." + tableName;
	bool src = tableschema->existAttr( dbtable );
	if ( src ) {
		dn("s560287 makeSysTables _SYS_ %d already exists, return", replicType);
		return;
	}

	JagParseAttribute jpa( this, 0, 0, dbname, _cfg );

	Jstr sql = "create table system._SYS_ ( key: a char(1) )";
	JagParser parser((void*)this);
	JagParseParam pparam( &parser );
	if ( parser.parseCommand( jpa, sql, &pparam, reterr ) ) {
		int rc = createTableCommit( req, dbname, &pparam, reterr, 0 );
		if ( 0 == rc ) {
			dn("s939384 makeSysTables() createTableCommit good system._SYS_ ");
		} else {
			dn("s939385 makeSysTables() createTableCommit error rc=%d system._SYS_ ", rc);
		}
	} else {
		dn("s937085 makeSysTables() parser.parseCommand(%s) error", sql.s() );
	}

	int  rc = 0;
	sql = "insert into system._SYS_ values ('1')";
	bool brc = parser.parseCommand( jpa, sql.s(), &pparam, reterr );
   	if ( brc ) {
		rc = doInsert( req, pparam, reterr, sql.s() );
	} else {
		dn("s39330 parser.parseCommand [%s] error", sql.s() ); 
	}

	dn("s391130 command [%s]  rc=%d 0=error 1=ok", sql.s(), rc );
}

// static
void *JagDBServer::monitorRemoteBackup( void *ptr )
{
	JagPass *jp = (JagPass*)ptr;
	int period;
	Jstr bcastCmd;
	JagFixString data;
	JagRequest req;

	while ( 1 ) {
		period = jp->servobj->_cfg->getIntValue("REMOTE_BACKUP_INTERVAL", 30);
		jagsleep(period, JAG_SEC);
		jp->servobj->processRemoteBackup( "_ex_procremotebackup", req );
	}

	delete jp;
   	return NULL;
}

// static
void *JagDBServer::monitorTimeSeries( void *ptr )
{
	JagPass *jp = (JagPass*)ptr;
	int period;

	while ( 1 ) {
		period = jp->servobj->_cfg->getIntValue("TIMESERIES_CLEANUP_INTERVAL", 127);
		jagsleep(period, JAG_SEC);

		jp->servobj->trimTimeSeries();
	}

	delete jp;
   	return NULL;
}

// thread for local doRemoteBackup on host0
// static
void * JagDBServer::threadRemoteBackup( void *ptr )
{
	JagPass *jp = (JagPass*)ptr;
	Jstr rmsg = Jstr("_serv_doremotebackup|") + jp->ip + "|" + jp->passwd;
	JagRequest req;
	jp->servobj->doRemoteBackup( rmsg.c_str(), req );
	delete jp;
	return NULL;
}

// Get localhost IP address. In case there are N IP for this host (many ether cards)
// it matches the right one in the primary host list
Jstr JagDBServer::getLocalHostIP( const Jstr &hostips )
{
	JagVector<Jstr> vec;
	JagNet::getLocalIPs( vec );
	
	jd(JAG_LOG_HIGH, "Local Interface IPs [%s]\n", vec.asString().c_str() );

	JagStrSplit sp( hostips, '|' );
	for ( int i = 0; i < sp.length(); ++i ) {
		if ( _listenIP.size() > 0 ) {
			if ( _listenIP == sp[i] ) {
				jd(JAG_LOG_HIGH, "Local Listen IP [%s]\n", sp[i].c_str() );
				return sp[i];
			}
		} else {
			if ( vec.exist( sp[i] ) ) {
				jd(JAG_LOG_HIGH, "Local IP [%s]\n", sp[i].c_str() );
				return sp[i];
			}
		}
	}

	return "";
}

Jstr JagDBServer::getBroadcastRecoverHosts( int replicateCopy )
{
	Jstr hosts;
	int pos1, pos2, pos3, pos4;
	JagStrSplit sp( _dbConnector->_nodeMgr->_hostClusterNodes, '|' );
	if ( _nthServer == 0 ) {
		pos1 = _nthServer+1;
		pos2 = sp.length()-1;
	} else if ( _nthServer == sp.length()-1 ) {
		pos1 = 0;
		pos2 = _nthServer-1;
	} else {
		pos1 = _nthServer+1;
		pos2 = _nthServer-1;
	}
	if ( pos1 == sp.length()-1 ) {
		pos3 = 0;
	} else {
		pos3 = pos1+1;
	}
	if ( pos2 == 0 ) {
		pos4 = sp.length()-1;
	} else {
		pos4 = pos2-1;
	}

	// check for repeat pos, set to -1
	if ( pos1 == _nthServer ) pos1 = -1;
	if ( pos2 == _nthServer || pos2 == pos1 ) pos2 = -1;
	if ( pos3 == _nthServer || pos3 == pos1 || pos3 == pos2 ) pos3 = -1;
	if ( pos4 == _nthServer || pos4 == pos1 || pos4 == pos2 || pos4 == pos3 ) pos4 = -1;

	hosts = sp[_nthServer];
	if ( 2 == replicateCopy ) { // one replicate copy, broadcast to self and right one server
		if ( pos1 >= 0 ) hosts += Jstr("|") + sp[pos1];
	} else if ( 3 == replicateCopy ) { 
		if ( pos1 >= 0 ) hosts += Jstr("|") + sp[pos1];
		if ( pos2 >= 0 ) hosts += Jstr("|") + sp[pos2];
		if ( pos3 >= 0 ) hosts += Jstr("|") + sp[pos3];
		if ( pos4 >= 0 ) hosts += Jstr("|") + sp[pos4];
	}

	jd(JAG_LOG_LOW, "broadcastRecoverHosts [%s]\n", hosts.c_str() );
	return hosts;
}	

// static
void  JagDBServer::noLinger( const JagRequest &req )
{
	JAGSOCK sock = req.session->sock;
	struct linger so_linger;
	so_linger.l_onoff = 1;
	so_linger.l_linger = 0;
	setsockopt( sock , SOL_SOCKET, SO_LINGER, (CHARPTR)&so_linger, sizeof(so_linger) );
}


// object method: do initialization for server
int JagDBServer::mainInit()
{
    //int rc;
    //int len;
	AbaxString  ports;
	AbaxString  cs;

	jd(JAG_LOG_LOW, "Initializing main ...\n");
	
	// make dbconnector connections first
	jd(JAG_LOG_LOW, "Make init connection from %s ...\n", _localInternalIP.c_str() ); 
	_dbConnector->makeInitConnection( _debugClient );
	jd(JAG_LOG_LOW, "Done init connection from %s\n", _localInternalIP.c_str() );
	jd(JAG_LOG_LOW, "Debug dbConnector %d\n", _debugClient );

	JagRequest req;
	JagSession session;
	session.replicType = 0;
	session.servobj = this;
	session.serverIP = _localInternalIP;
	req.session = &session;

	refreshSchemaInfo( session.replicType, g_lastSchemaTime );

	// set recover flag, check to see if need to do drecover ( delta log recovery, if hard disk is ok )
	// or do crecover ( tar copy file, if hard disk is new )
	Jstr recoverCmd;
    Jstr bcasthosts = getBroadcastRecoverHosts( _faultToleranceCopy );

	_crecoverFpath = "";
	_prevcrecoverFpath = "";
	_nextcrecoverFpath = "";
	_walrecoverFpath = "";
	_prevwalrecoverFpath = "";
	_nextwalrecoverFpath = "";

	// first broadcast crecover to get package
    /**** 3/31/2023 **
	if ( 1 ) {
		// ask to do crecover
        dn("s0200644 broadcastSignal _serv_crecover ...");
		_dbConnector->broadcastSignal( "_serv_crecover", bcasthosts );
        dn("s0200644 broadcastSignal _serv_crecover done");

        dn("s0200644 recoveryFromTransferredFile ...");
		recoveryFromTransferredFile();
        dn("s0200644 recoveryFromTransferredFile done");

		// wait for this socket to be ready to recv data from client
		// then request delta data from neighbor
		jd(JAG_LOG_LOW, "end broadcast\n" );
	}
    ***/

	// recoverRegSpLog(); // regular data and metadata redo  3/31/2023
    _restartRecover = 0;

	_newdcTrasmittingFin = 1;

	if ( 1 ) {
		jd(JAG_LOG_LOW, "begin recover wallog ...\n");
		jagint total = recoverWalLog();
		jd(JAG_LOG_LOW, "done recover wallog total=%ld records\n", total);
	}


	// create thread to monitor logged incoming insert/update/delete commands
	// also create and open delta logs for replicate
	JagFileMgr::makeLocalLogDir("cmd");
	JagFileMgr::makeLocalLogDir("delta");

	Jstr cs1 = _cfg->getValue("REMOTE_BACKUP_SERVER", "" );
	Jstr cs2 = _cfg->getValue("REMOTE_BACKUP_INTERVAL", "0" );
	int interval = jagatoi( cs2.c_str() );
	if ( interval > 0 && interval < 10 ) interval = 10;
	int ishost0 = _dbConnector->_nodeMgr->_isHost0OfCluster0;

	if ( ishost0 && cs1.length()>0 && interval > 0 ) {
    	pthread_t  threadmo;
		JagPass *jp = new JagPass();
		jp->servobj = this;
		jd(JAG_LOG_LOW, "Initializing thread for remote backup\n");
    	jagpthread_create( &threadmo, NULL, monitorRemoteBackup, (void*)jp );
    	pthread_detach( threadmo );
	}

    bool monTS = true;
	if ( monTS ) {
    	pthread_t  threadmo;
		JagPass *jp = new JagPass();
		jp->servobj = this;
		jd(JAG_LOG_LOW, "Initializing thread for timeseries\n");
    	jagpthread_create( &threadmo, NULL, monitorTimeSeries, (void*)jp );
    	pthread_detach( threadmo );
	}

	return 1;
}  // end mainInit

int JagDBServer::mainClose()
{
	pthread_mutex_destroy( &g_dbschemamutex );
	pthread_mutex_destroy( &g_flagmutex );
	pthread_mutex_destroy( &g_wallogmutex );
	pthread_mutex_destroy( &g_dlogmutex );
	return 1;
}

// object method: make thread groups
int JagDBServer::makeThreadGroups( int threadGroups, int threadGroupNum, int interval )
{
	this->_threadGroupNum = threadGroupNum;
	pthread_t thr[threadGroups];

	jd(JAG_LOG_LOW, "makeThreadGroups threadGroups=%d threadGroupNum=%d ...\n", threadGroups, threadGroupNum);

	for ( int i = 0; i < threadGroups; ++i ) {
		this->_groupSeq = i;
    	jagpthread_create( &thr[i], NULL, oneThreadGroupTask, (void*)this );
	    jd(JAG_LOG_LOW, "i=%d oneThreadGroupTask is created\n", i );
        if ( interval ) {
            sleep(interval);
        }
    	pthread_detach( thr[i] );
	}

	return 1;
}

// static method
void* JagDBServer::oneThreadGroupTask( void *servptr )
{
	JagDBServer *servobj = (JagDBServer*)servptr;
	jagint 		threadGroupNum = (jagint)servobj->_threadGroupNum;
    struct 		sockaddr_in cliaddr;
    socklen_t 	clilen;
	JAGSOCK 	connfd;
	int     	lasterrno = -1;
	jagint     	toterrs = 0;
	
	++ servobj->_activeThreadGroups;

    listen( servobj->_sock, 300);

    for(;;) {
		servobj->_dumfd = dup2( jagopen("/dev/null", O_RDONLY ), 0 );
        clilen = sizeof(cliaddr);

		memset(&cliaddr, 0, clilen );
        connfd = accept( servobj->_sock, (struct sockaddr *)&cliaddr, ( socklen_t* )&clilen);

		if ( connfd < 0 ) {
			if ( errno == lasterrno ) {
				if ( toterrs < 10 ) {
					jd(JAG_LOG_LOW, "E3234 accept error connfd=%d errno=%d (%s)\n", connfd, errno, strerror(errno) );
				} else if ( toterrs > 1000000 ) {
					toterrs = 0;
					jd(JAG_LOG_LOW, "E3258 accept error connfd=%d errno=%d (%s)\n", connfd, errno, strerror(errno) );
				}
			} else {
				jd(JAG_LOG_LOW, "E3334 accept error connfd=%d errno=%d (%s)\n", connfd, errno, strerror(errno) );
				toterrs = 0;
			}

			lasterrno = errno;
			++ toterrs;
			jagsleep(3, JAG_SEC);
			continue;
		}

		if ( 0 == connfd ) {
			jagclose( connfd );
			jagsleep(1, JAG_SEC);
			continue;
		}

		++ servobj->_connections;

		JagPass *passmo = newObject<JagPass>();
		passmo->sock = connfd;
		passmo->servobj = servobj;
		passmo->ip = inet_ntoa( cliaddr.sin_addr );
		passmo->port = ntohs( cliaddr.sin_port );

		// Loop taking commands from clients
		oneClientThreadTask( (void*)passmo );

		if ( threadGroupNum > 0 && jagint(servobj->_activeClients) * 100 <= servobj->_activeThreadGroups * 40  ) {
			jd(JAG_LOG_LOW, "c9499 threadGroup=%ld idle quit\n", threadGroupNum );
			break;
		}
	}
	
	-- servobj->_activeThreadGroups;
   	jd(JAG_LOG_LOW, "s2894 thread group=%ld done, activeThreadGroups=%ld\n", 
				threadGroupNum, (jagint)servobj->_activeThreadGroups );

	return NULL;
}


// auth user
// 0: error
// 1: OK
bool JagDBServer::doAuth( JagRequest &req, char *pmesg )
{
	int ipOK = 0;

	d("c303032 pthread_rwlock_lock ...\n");
	pthread_rwlock_rdlock( & _aclrwlock );
	d("c303032 pthread_rwlock_lock done\n");

	if ( _allowIPList->size() < 1 && _blockIPList->size() < 1 ) {
		// no witelist and blocklist setup, pass
		ipOK = 1;
	} else {
		if ( _allowIPList->match( req.session->ip ) ) {
			if ( ! _blockIPList->match( req.session->ip ) ) {
				ipOK = 1;
			}
		}
	}

	d("c303039 pthread_rwlock_unlock\n");
	pthread_rwlock_unlock( &_aclrwlock );

	if ( ! ipOK ) {
    	sendER( req, "E99579 Error client IP address");
		jd(JAG_LOG_HIGH, "client IP (%s) is blocked\n", req.session->ip.c_str() );
		return false;
	}

	JagStrSplit sp(pmesg, '|' );
   	Jstr uid;

	// token auth
	if ( 7 == sp.length()) {
		d("s610023 token auth\n");
		Jstr token = sp[1];

		if ( token != _servToken ) {
    		sendER( req, "E21321 Error auth token");
			jd(JAG_LOG_HIGH, "token (%s) is not authed\n", token.c_str() );
			return false;
		}

		req.session->cliPID = sp[2];
		Jstr thid = sp[3];
		Jstr seq = sp[4];
		req.session->dbname = sp[5];
        Jstr connectOpt = sp[6];

        req.session->connectOpt = sp[6];

        JagHashMap<AbaxString, AbaxString> hashmap;

        convertToHashMap( req.session->connectOpt, '/', hashmap);

        req.session->exclusiveLogin = 0;
        AbaxString exclusive;

        if ( hashmap.getValue("exclusive", exclusive ) ) {
            if ( exclusive == "yes" || exclusive == "true" || exclusive == "1" ) {
                req.session->exclusiveLogin = 1;
            }
        }

        AbaxString replicTS;
        if ( hashmap.getValue("replicType", replicTS ) ) {
            req.session->replicType = jagatoi( replicTS.s() );
            dn("s2083003 connectOpt has replicType=%d", req.session->replicType ); 
        }


		uid = "admin";
		d("s60123 token OK, goto isauthed client: PID=%s  TID=%s SEQ=%s req.session->dbname=[%s]...\n", 
		  req.session->cliPID.s(), thid.s(), seq.s(), req.session->dbname.s() );

    	if ( req.session->cliPID.toInt() == getpid() ) {
			req.session->samePID = 1;
		} else {
			req.session->samePID = 0;
		}

		goto isauthed;
	}

	{
    	// user/password auth
    	if ( sp.length() < 9 ) {
    		sendER( req, "E214 Error Auth");
    		return false;
    	}
    
		// "auth|uid|encpass|timediff|replciateType|drecoverConn|connectOpt|clientPID"
    	req.session->uid = sp[1];
    	Jstr encpass = sp[2];
    	req.session->timediff = jagatoi( sp[3].c_str() );
    	req.session->replicType = jagatoi( sp[4].c_str() );

        // disable client drecoverConn
    	//req.session->drecoverConn = atoi( sp[5].c_str() );
    	req.session->drecoverConn = 0;

		jd(JAG_LOG_LOW, "req.session drecoverConn=%d\n", req.session->drecoverConn );

    	req.session->connectOpt = sp[6];
    	req.session->cliPID = sp[7];
    	req.session->dbname = sp[8];

    	Jstr pass = JagDecryptStr( _privateKey, encpass );
    	req.session->passwd = pass;
    	req.session->origserv = 0;
    
		uid = sp[1];
    	bool isGood = false;
    
    	d("s4408023 _serverReady=%d req.session->origserv=%d req.session->ip=[%s] cliPID=%s mypid=%d\n", 
    	      _serverReady, req.session->origserv, req.session->ip.c_str(), req.session->cliPID.c_str(), getpid() );
    
    	d("s4408024 _serverReady=%d req.session->origserv=%d req.session->ip=[%s]\n", 
    	      _serverReady, req.session->origserv, req.session->ip.c_str() );
    
    	if ( ! _serverReady && ! req.session->origserv ) {
    		d("s400421 doAuth() sending to client: Server not ready, please try later client=%s:%u\n", 
				 req.session->ip.c_str(), req.session->port ); 
    		sendER( req, "E21500 Server not ready, please try later");
    		return false;
    	}
    
		d("s38091301  req.session->cliPID=%d mupid=%d\n", req.session->cliPID.s(), getpid() );
    	if ( req.session->cliPID.toInt() == getpid() ) {
			req.session->samePID = 1;
		} else {
			req.session->samePID = 0;
		}
    	
    	JagHashMap<AbaxString, AbaxString> hashmap;
    	convertToHashMap( req.session->connectOpt, '/', hashmap);
    	AbaxString exclusive, clear, checkip;
    	req.session->exclusiveLogin = 0;
    
    	if ( uid == "admin" ) {
        	if ( hashmap.getValue("clearex", clear ) ) {
        		if ( clear == "yes" || clear == "true" || clear == "1" ) {
            		_exclusiveAdmins = 0;
            		jd(JAG_LOG_LOW, "admin cleared exclusive from %s\n", req.session->ip.c_str() );
    			}
    		}
    
    		req.session->datacenter = false;
    
    		req.session->dcfrom = 0;
        	if ( hashmap.getValue("exclusive", exclusive ) ) {
        		if ( exclusive == "yes" || exclusive == "true" || exclusive == "1" ) {
    				req.session->exclusiveLogin = 1;
    				if ( req.session->replicType == 0 ) {
    					// main connection accept exclusive login, other backup connection ignore it
    					if ( _exclusiveAdmins > 0 ) {
    						sendER( req, "E90008 Failed to login in exclusive mode");
    						return false;
    					}
    				}
            	}
        	}
    	}
    
     	JagUserID *userDB;
    	if ( req.session->replicType == 0 ) userDB = _userDB;
    	else if ( req.session->replicType == 1 ) userDB = _prevuserDB;
    	else if ( req.session->replicType == 2 ) userDB = _nextuserDB;
    	// check password first
    
    	int passOK = 0;
    	if ( userDB ) {
    		AbaxString dbpass = userDB->getValue( uid, JAG_PASS );
    		char *md5 = MDString( pass.c_str() );
    		AbaxString md5pass = md5;
    		if ( md5 ) free( md5 );
    		if ( dbpass == md5pass && dbpass.size() > 0 ) {
    			isGood = true;
    			passOK = 1;
    		} else {
    			passOK = -1;
           		//jd(JAG_LOG_HIGH, "dbpass=[%s] inmd5pass=[%s]\n", dbpass.c_str(), md5pass.c_str() );
    		}
    	} else {
    		passOK = -2;
    	}
    	
    	int tokenOK = 0;
    	if ( ! isGood ) {
    		AbaxString servToken;
        	if ( hashmap.getValue("TOKEN", servToken ) ) {
    			if ( servToken == _servToken ) {
    				isGood = true;
    				tokenOK = 1;
    			} else {
    				tokenOK = -1;
           			jd(JAG_LOG_HIGH, "inservToken=[%s] _servToken=[%s]\n", servToken.c_str(), _servToken.c_str() );
    			}
    		}  else {
    			tokenOK = -2;
    		}
    	}
    
    	if ( ! isGood ) {
           	sendER( req, "E9018 Error password or token");
           	jd(JAG_LOG_LOW, "Connection from %s, Error password or TOKEN\n", req.session->ip.c_str() );
           	jd(JAG_LOG_LOW, "%s passOK=%d tokenOK=%d\n", req.session->ip.c_str(), passOK, tokenOK );
           	return false;
    	}
    
    	if ( uid == "admin" ) {
    		if ( req.session->exclusiveLogin ) {
            	jd(JAG_LOG_LOW, "Exclusive admin connected from %s\n", req.session->ip.c_str() );
            	_exclusiveAdmins = 1;
    		}
    	}
    	
    	if ( ! req.session->origserv && 0 == req.session->replicType ) {
        	jd(JAG_LOG_LOW, "user %s connected from %s\n", uid.c_str(), req.session->ip.c_str() );
    	}
    
    	d("s949440 good, send OK back to client %s\n", req.session->ip.c_str());
	}


isauthed:
	d("s600231 after isauthed label, sending OK back to client=%s:%u ...\n", req.session->ip.c_str(), req.session->port);
	jagint len;

	Jstr okmsg = "OK^";  // idx: 0
	okmsg += longToStr(  pthread_self() );  // idx: 1
	okmsg += Jstr("^") + _dbConnector->_nodeMgr->_sendAllNodes; // idx: 2 
	okmsg += Jstr("^") + longToStr( _nthServer ); // idx: 3 
	okmsg += Jstr("^") + longToStr( _faultToleranceCopy ); // idx: 4

	if ( req.session->samePID ) okmsg += Jstr("^1^");  // idx: 5
	else okmsg += Jstr("^0^");  // idx: 5

	okmsg += longToStr( req.session->exclusiveLogin );  // idx: 6
	//okmsg += Jstr("^") + _cfg->getValue("HASHJOIN_TABLE_MAX_RECORDS", "100000"); // idx: 7
	okmsg += Jstr("^") + intToStr( _dbConnector->_nodeMgr->_totalClusterNumber ); // idx:  7
	okmsg += Jstr("^") + intToStr( _isGate );                                   // idx:  8
	okmsg += Jstr("^") + intToStr( _dbConnector->_nodeMgr->_hostClusterNumber ); // idx:  9

    dn("s87020023  _totalClusterNumber=%d", _dbConnector->_nodeMgr->_totalClusterNumber );
    dn("s87020023  _hostClusterNumber=%d",  _dbConnector->_nodeMgr->_hostClusterNumber );

	d("s440440 before sendMessageLength okmsg=[%s]...\n", okmsg.s() );
	len = sendDataEnd( req, okmsg);
	d("s440440 after sendMessageLength auth OK msg client=%s:%u sent_len=%d\n", req.session->ip.c_str(), req.session->port, len);

	req.session->uid = uid;

    /** 4/1/2023
	// set timer for session if recover connection is not 2 ( clean recover )
	if ( req.session->drecoverConn != 2 && !req.session->samePID ) {
		d("s44404 createTimer...\n");
		req.session->createTimer();
		d("s44404 createTimer done\n");
	}
    **/

	d("s404491 return true from doAuth() clientIP=%s:%u thrd=%lu\n", 
		 req.session->ip.c_str(), req.session->port, THID );
	return true;
}

// method for make connection use only
bool JagDBServer::useDB( JagRequest &req, char *pmesg  )
{
	jd(JAG_LOG_HIGH, "useDB %s\n", pmesg );

	char *tok;
	char dbname[JAG_MAX_DBNAME+1];
	char *saveptr;

	tok = strtok_r( pmesg, " ", &saveptr );  // pmesg is modified!!
	tok = strtok_r( NULL, " ;", &saveptr );
	if ( ! tok ) {
		sendER( req, "E80012 Database empty");
	} else {
		if ( strlen( tok ) > JAG_MAX_DBNAME ) {
			sendER( req, "E80013 Database name is too long ");
		} else {
    		strcpy( dbname, tok );
			tok = strtok_r( NULL, " ;", &saveptr );
			if ( tok ) {
				sendER( req, "E80014 Use database syntax error");
			} else {
				if ( 0 == strcmp( dbname, "test" ) ) {
					req.session->dbname = dbname;
					sendDataEnd( req, "Database changed" );
					return true;
				}

    			Jstr jagdatahome = _cfg->getJDBDataHOME( req.session->replicType );
    			Jstr fpath = jagdatahome + "/" + dbname;
    			if ( 0 == jagaccess( fpath.c_str(), X_OK )  ) {
    				req.session->dbname = dbname;
					sendDataEnd( req, "Database changed" );
    			} else {
    				sendER( req, "E91087 error Database not found");
    			}
			}
		}
	}

	return true;
}

// method to refresh schemaMap and schematime for dbConnector
void JagDBServer::refreshSchemaInfo( int replicType, jagint &schtime )
{
	Jstr schemaInfo;

	JagTableSchema *tableschema;
	JagIndexSchema *indexschema;

	getTableIndexSchema( replicType, tableschema, indexschema );

	Jstr obj;
	AbaxString recstr;

	// tables
	JagVector<AbaxString> *vec = tableschema->getAllTablesOrIndexes( "", "");
	if ( vec ) {
    	for ( int i = 0; i < vec->size(); ++i ) {
    		obj = (*vec)[i].c_str();
    		if ( tableschema->getAttr( obj, recstr ) ) {
    			schemaInfo += Jstr( obj.c_str() ) + ":" + recstr.c_str() + "\n";
    		} 
    	}
    	if ( vec ) delete vec;
    	vec = NULL;
	}

	// indexes
	vec = indexschema->getAllTablesOrIndexes( "", "" );
	if ( vec ) {
    	for ( int i = 0; i < vec->size(); ++i ) {
    		obj = (*vec)[i].c_str();
    		if ( indexschema->getAttr( obj, recstr ) ) {
    			schemaInfo += Jstr( obj.c_str() ) + ":" + recstr.c_str() + "\n"; 
    		}
    	}
    	if ( vec ) delete vec;
    	vec = NULL;
	}

    /***
	_dbConnector->_parentCliNonRecover->_schemaLock->writeLock( -1 );
	_dbConnector->_parentCliNonRecover->rebuildSchemaMap();
	_dbConnector->_parentCliNonRecover->updateSchemaMap( schemaInfo.c_str() );
	_dbConnector->_parentCliNonRecover->_schemaLock->writeUnlock( -1 );
    ***/

	_dbConnector->_parentCli->_schemaLock->writeLock( -1 );
	_dbConnector->_parentCli->clearSchemaMap();
	_dbConnector->_parentCli->updateSchemaMap( schemaInfo.c_str() );
	_dbConnector->_parentCli->_schemaLock->writeUnlock( -1 );

	_dbConnector->_broadcastCli->_schemaLock->writeLock( -1 );
	_dbConnector->_broadcastCli->clearSchemaMap();
	_dbConnector->_broadcastCli->updateSchemaMap( schemaInfo.c_str() );
	_dbConnector->_broadcastCli->_schemaLock->writeUnlock( -1 );

	struct timeval now;
	gettimeofday( &now, NULL );
	schtime = now.tv_sec * (jagint)1000000 + now.tv_usec;
}

// Reload ACL list
void JagDBServer::refreshACL( int bcast )
{
	if ( ! _dbConnector->_nodeMgr->_isHost0OfCluster0 ) {
		return;
	}

	loadACL();

	if ( bcast && ( _allowIPList->_data.size() >0 || _blockIPList->_data.size() > 0 ) ) {
		Jstr bcastCmd = "_serv_refreshacl|" + _allowIPList->_data+ "|" + _blockIPList->_data;
		_dbConnector->broadcastSignal( bcastCmd, "" );
	}
}

// Load ACL list
void JagDBServer::loadACL()
{
	Jstr fpath = jaguarHome() + "/conf/allowlist.conf";
	pthread_rwlock_wrlock(&_aclrwlock);
	if ( _allowIPList ) delete _allowIPList;
	_allowIPList = new JagIPACL( fpath );

	fpath = jaguarHome() + "/conf/blocklist.conf";
	if ( _blockIPList ) delete _blockIPList;
	_blockIPList = new JagIPACL( fpath );
	pthread_rwlock_unlock(&_aclrwlock);

}

// back up schema and other meta data
// seq should be every minute
void JagDBServer::doBackup( jaguint seq )
{
	// use _cfg
	Jstr cs = _cfg->getValue("LOCAL_BACKUP_PLAN", "" );
	if ( cs.length() < 1 ) {
		return;
	}

	if ( ! _dbConnector->_nodeMgr->_isHost0OfCluster0 ) {
		return;
	}

	if ( _restartRecover ) {
		return;
	}

	JagStrSplit sp( cs, '|' );
	Jstr  rec;
	Jstr  bcastCmd, bcasthosts;

	// copy meta data to 15min directory
	if ( ( seq%15 ) == 0 && sp.contains( "15MIN", rec ) ) {
		copyData( rec, false );
		bcastCmd = "_serv_copydata|" + rec + "|0";
		_dbConnector->broadcastSignal( bcastCmd, bcasthosts ); 
	}

	// copy meta data to hourly directory
	if ( ( seq % 60 ) == 0 && sp.contains( "HOURLY", rec ) ) {
		copyData( rec, false );
		bcastCmd = "_serv_copydata|" + rec + "|0";
		_dbConnector->broadcastSignal( bcastCmd, bcasthosts ); 
	}

	// copy meta data to daily directory
	if ( ( seq % 1440 ) == 0 && sp.contains( "DAILY", rec ) ) {
		copyData( rec );
		bcastCmd = "_serv_copydata|" + rec + "|1";
		_dbConnector->broadcastSignal( bcastCmd, bcasthosts ); 
	}

	// copy meta data to weekly directory
	if ( ( seq % 10080 ) == 0 && sp.contains( "WEEKLY", rec ) ) {
		copyData( rec );
		bcastCmd = "_serv_copydata|" + rec + "|1";
		_dbConnector->broadcastSignal( bcastCmd, bcasthosts ); 
	}

	// copy meta data to monthly directory
	if ( ( seq % 43200 ) == 0 && sp.contains( "MONTHLY", rec ) ) {
		copyData( rec );
		bcastCmd = "_serv_copydata|" + rec + "|1";
		_dbConnector->broadcastSignal( bcastCmd, bcasthosts ); 
	}
}

// back up schema and other meta data
#ifndef _WINDOWS64_
#include <sys/sysinfo.h>
void JagDBServer::writeLoad( jaguint seq )
{
	if ( ( seq%15) != 0 ) {
		return;
	}

	struct sysinfo info;
    sysinfo( &info );

	jagint usercpu, syscpu, idle;
	_jagSystem.getCPUStat( usercpu, syscpu, idle );

	Jstr userCPU = ulongToString( usercpu );
	Jstr sysCPU = ulongToString( syscpu );

	Jstr totrams = ulongToString( info.totalram / ONE_GIGA_BYTES );
	Jstr freerams = ulongToString( info.freeram / ONE_GIGA_BYTES );
	Jstr totswaps = ulongToString( info.totalswap / ONE_GIGA_BYTES );
	Jstr freeswaps = ulongToString( info.freeswap / ONE_GIGA_BYTES );
	Jstr procs = intToString( info.procs );

	jaguint diskread, diskwrite, netread, netwrite;
	JagNet::getNetStat( netread, netwrite );
	JagFileMgr::getIOStat( diskread, diskwrite );
	Jstr netReads = ulongToString( netread /ONE_GIGA_BYTES );
	Jstr netWrites = ulongToString( netwrite /ONE_GIGA_BYTES );
	Jstr diskReads = ulongToString( diskread  );
	Jstr diskWrites = ulongToString( diskwrite  );

	Jstr nselects = ulongToString( numSelects );
	Jstr ninserts = ulongToString( numInserts );
	Jstr nupdates = ulongToString( numUpdates );
	Jstr ndeletes = ulongToString( numDeletes );

	// printf("s3104 diskread=%lu diskwrite=%lu netread=%lu netwrite=%lu\n", diskread, diskwrite, netread, netwrite );

	Jstr rec = ulongToString( time(NULL) ) + "|" + Jstr(userCPU) + "|" + sysCPU;
	rec += Jstr("|") + totrams + "|" + freerams + "|" + totswaps + "|" + freeswaps + "|" + procs; 
	rec += Jstr("|") + diskReads + "|" + diskWrites + "|" + netReads + "|" + netWrites;
	rec += Jstr("|") + nselects + "|" + ninserts + "|" + nupdates + "|" + ndeletes;
	JagBoundFile bf( _perfFile.c_str(), 96 ); // 24 hours
	bf.openAppend();
	bf.appendLine( rec.c_str() );
	bf.close();
}
#else
// Windows
void JagDBServer::writeLoad( jaguint seq )
{
	if ( ( seq%15) != 0 ) {
		return;
	}

	jagint usercpu, syscpu, idlecpu;
	_jagSystem.getCPUStat( usercpu, syscpu, idlecpu );
	Jstr userCPU = ulongToString( usercpu );
	Jstr sysCPU = ulongToString( syscpu );

	jagint totm, freem, used; //GB
	JagSystem::getMemInfo( totm, freem, used );
	Jstr totrams = ulongToString( totm );
	Jstr freerams = ulongToString( freem );

	MEMORYSTATUSEX statex;
	Jstr totswaps = intToString( statex.ullTotalPageFile/ ONE_GIGA_BYTES );
	Jstr freeswaps = intToString( statex.ullAvailPageFile / ONE_GIGA_BYTES );
	Jstr procs = intToString( _jagSystem.getNumProcs() );

	Jstr nselects = ulongToString( numSelects );
	Jstr ninserts = ulongToString( numInserts );
	Jstr nupdates = ulongToString( numUpdates );
	Jstr ndeletes = ulongToString( numDeletes );

	jaguint diskread, diskwrite, netread, netwrite;
	JagNet::getNetStat( netread, netwrite );
	JagFileMgr::getIOStat( diskread, diskwrite );
	Jstr netReads = ulongToString( netread / ONE_GIGA_BYTES );
	Jstr netWrites = ulongToString( netwrite / ONE_GIGA_BYTES );
	Jstr diskReads = ulongToString( diskread  );
	Jstr diskWrites = ulongToString( diskwrite  );

	Jstr rec = ulongToString( time(NULL) ) + "|" + Jstr(userCPU) + "|" + sysCPU;
	rec += Jstr("|") + totrams + "|" + freerams + "|" + totswaps + "|" + freeswaps + "|" + procs; 
	rec += Jstr("|") + diskReads + "|" + diskWrites + "|" + netReads + "|" + netWrites;
	rec += Jstr("|") + nselects + "|" + ninserts + "|" + nupdates + "|" + ndeletes;
	JagBoundFile bf( _perfFile.c_str(), 96 ); // 24 hours
	bf.openAppend();
	bf.appendLine( rec.c_str() );
	bf.close();

}

#endif

// rec:  15MIN:OVERWRITE
void JagDBServer::copyData( const Jstr &rec, bool show )
{
	if ( rec.length() < 1 ) {
		return;
	}
	
	JagStrSplit sp( rec, ':' );
	if ( sp.length() < 1 ) {
		return;
	}

	Jstr dirname = makeLowerString( sp[0] );
	Jstr policy;
	policy = sp[1];

	Jstr tmstr = JagTime::YYYYMMDDHHMM();
	copyLocalData( dirname, policy, tmstr, show );
}

// < 0: error
// 0: OK
// dirname: 15min/hourly/daily/weekly/monthly or last   policy: "OVERWRITE" or "SNAPSHOT"
int JagDBServer::copyLocalData( const Jstr &dirname, const Jstr &policy, const Jstr &tmstr, bool show )
{
	if ( policy != "OVERWRITE" && policy != "SNAPSHOT" ) {
		return -1;
	}

    Jstr srcdir = jaguarHome() + "/data";
	if ( JagFileMgr::dirEmpty( srcdir ) ) {
		return -2;
	}

	char hname[80];
	gethostname( hname, 80 );

    // Jstr fileName = JagTime::YYYYMMDDHHMM() + "-" + hname;
    Jstr fileName = tmstr + "-" + hname;
    Jstr destdir = jaguarHome() + "/backup";
	destdir += Jstr("/") + dirname + "/" + fileName;

	JagFileMgr::makedirPath( destdir, 0700 );
	
	char cmd[2048];
	sprintf( cmd, "/bin/cp -rf %s/*  %s", srcdir.c_str(), destdir.c_str() );
	system( cmd );
	if ( show ) {
		jd(JAG_LOG_LOW, "Backup data from %s to %s\n", srcdir.c_str(), destdir.c_str() );
	}

	Jstr lastPath = jaguarHome() + "/backup/" + dirname + "/.lastFileName.txt";
	if ( policy == "OVERWRITE" ) {
		Jstr lastFileName;
		JagFileMgr::readTextFile( lastPath, lastFileName );
		if (  lastFileName.length() > 0 ) {
    		Jstr destdir = jaguarHome() + "/backup";
			destdir += Jstr("/") + dirname + "/" + lastFileName;
			sprintf( cmd, "/bin/rm -rf %s", destdir.c_str() );
			system( cmd );
		}
	}

	JagFileMgr::writeTextFile( lastPath, fileName );
	return 0;
}

void JagDBServer::doCreateIndex( JagTable *ptab, JagIndex *pindex )
{
	ptab->_indexlist.append( pindex->getIndexName() );
	if ( ptab->_darrFamily->getCount( ) < 1 ) {
		return;
	}

	ptab->formatCreateIndex( pindex );
}

// method to drop all tables and indexs under database "dbname"
void JagDBServer::dropAllTablesAndIndexUnderDatabase( const JagRequest &req, JagTableSchema *schema, const Jstr &dbname )
{	
	JagTable *ptab;
	Jstr dbobj, errmsg;
	JagVector<AbaxString> *vec = schema->getAllTablesOrIndexes( dbname, "" );
	if ( NULL == vec ) return;

	int lockrc;

    	for ( int i = 0; i < vec->size(); ++i ) {
			ptab = NULL;
    		dbobj = (*vec)[i].c_str();
			JagStrSplit sp( dbobj, '.' );
			if ( sp.length() != 2 ) continue;

			JagParseParam pparam;
			ObjectNameAttribute objNameTemp;
			objNameTemp.init();
			objNameTemp.dbName = sp[0];
			objNameTemp.tableName = sp[1];
			pparam.objectVec.append(objNameTemp);
			pparam.optype = 'C';
			pparam.opcode = JAG_DROPTABLE_OP;
			
			ptab = _objectLock->writeLockTable( pparam.opcode, pparam.objectVec[0].dbName,
												 pparam.objectVec[0].tableName, schema, req.session->replicType, 1, lockrc );
			if ( ptab ) {
				//flushOneTableAndRelatedIndexsInsertBuffer( dbobj, req.session->replicType, 1, ptab, NULL );
        		ptab->drop( errmsg );  //_darr is gone, raystat has cleaned up this darr
    			schema->remove( dbobj );
        		delete ptab; ptab = NULL;
				_objectLock->writeUnlockTable( pparam.opcode, pparam.objectVec[0].dbName,
												pparam.objectVec[0].tableName, req.session->replicType, 1 );
			}
    	}

		delete vec;
		vec = NULL;
}

// method to drop all tables and indexs under all databases
void JagDBServer::dropAllTablesAndIndex( const JagRequest &req, JagTableSchema *schema )
{	
	JagTable *ptab;
	Jstr dbobj, errmsg;
	JagVector<AbaxString> *vec = schema->getAllTablesOrIndexes( "", "" );
	int lockrc;

	if ( vec ) {
    	for ( int i = 0; i < vec->size(); ++i ) {
			ptab = NULL;
    		dbobj = (*vec)[i].c_str();
			JagStrSplit sp( dbobj, '.' );
			if ( sp.length() != 2 ) continue;

			JagParseParam pparam;
			ObjectNameAttribute objNameTemp;
			objNameTemp.init();
			objNameTemp.dbName = sp[0];
			objNameTemp.tableName = sp[1];
			pparam.objectVec.append(objNameTemp);
			pparam.optype = 'C';
			pparam.opcode = JAG_DROPTABLE_OP;
			
			ptab = _objectLock->writeLockTable( pparam.opcode, pparam.objectVec[0].dbName,
												 pparam.objectVec[0].tableName, schema, req.session->replicType, 1, lockrc );
			if ( ptab ) {
        		ptab->drop( errmsg );  //_darr is gone, raystat has cleaned up this darr
    			schema->remove( dbobj );
        		delete ptab; ptab = NULL;
				_objectLock->writeUnlockTable( pparam.opcode, pparam.objectVec[0].dbName,
												pparam.objectVec[0].tableName, req.session->replicType, 1 );
			}
    	}

		delete vec;
		vec = NULL;
	}
}

// showusers
void JagDBServer::showUsers( const JagRequest &req )
{
	if ( req.session->uid != "admin" ) {
		//sendMessage( req, "Only admin can get a list of user accounts", "OK" );
		sendER( req, "Only admin can get a list of user accounts");
		return;
	}

	JagUserID *uiddb = NULL;
	if ( req.session->replicType == 0 ) uiddb = _userDB;
	else if ( req.session->replicType == 1 ) uiddb = _prevuserDB;
	else if ( req.session->replicType == 2 ) uiddb = _nextuserDB;
	Jstr users;
	if ( uiddb ) {
		users = uiddb->getListUsers();
	}
	//sendMessageLength( req, users.c_str(), users.length(), "OK" );
	sendOKEnd( req, users );
}

// ensure admin account is created
void JagDBServer::createAdmin()
{
	_dbConnector->_passwd = "anon";
	JagUserID *uiddb = _userDB;
	makeAdmin( uiddb );
	uiddb = _prevuserDB;
	makeAdmin( uiddb );
	uiddb = _nextuserDB;
	makeAdmin( uiddb );
}

void JagDBServer::makeAdmin( JagUserID *uiddb )
{
	// default admin password
	Jstr uid = "admin";
	Jstr pass = "jaguarjaguarjaguar";
	AbaxString dbpass = uiddb->getValue(uid, JAG_PASS );
	if ( dbpass.size() > 0 ) {
		return;
	}

	char *md5 = MDString( pass.c_str() );
	Jstr mdpass = md5;
	if ( md5 ) free( md5 );
	md5 = NULL;

	uiddb->addUser(uid, mdpass, JAG_USER, JAG_WRITE );
	// printf("s8339 createAdmin uid=[%s] mdpass=[%s] rc=%d\n", uid.c_str(), mdpass.c_str(), rc );
}

// client expects: "numservs|numDBs|numTables|selects|inserts|updates|deletes|usersessions"
Jstr JagDBServer::getClusterOpInfo( const JagRequest &req )
{
	dn("s511128  getClusterOpInfo ...");
	JagStrSplit srv( _dbConnector->_nodeMgr->_allNodes, '|' );
	int nsrv = srv.length(); 
	int dbs, tabs;
	numDBTables( dbs, tabs );

	// get opinfo on this server
	Jstr res;
	char buf[1024];
	sprintf( buf, "%d|%d|%d|%lld|%lld|%lld|%lld|%lld", nsrv, dbs, tabs, 
			(jagint)numSelects, (jagint)numInserts, 
			(jagint)numUpdates, (jagint)numDeletes, 
			_taskMap->size() );
	res = buf;

	// broadcast other server request info
	Jstr resp, bcasthosts;
	resp = _dbConnector->broadcastGet( "_serv_opinfo", bcasthosts ); 
	resp += res + "\n";
	//printf("s4820 broadcastGet(_serv_opinfo)  resp=[%s]\n", resp.c_str() );
	//fflush( stdout );

	JagStrSplit sp( resp, '\n', true );
	jagint sel, ins, upd, del, sess;
	sel = ins = upd = del = sess =  0;

	for ( int i = 0 ; i < sp.length(); ++i ) {
		JagStrSplit ss( sp[i], '|' );
		sel += jagatoll( ss[3].c_str() );
		ins += jagatoll( ss[4].c_str() );
		upd += jagatoll( ss[5].c_str() );
		del += jagatoll( ss[6].c_str() );
		sess += jagatoll( ss[7].c_str() );
	}

	memset(buf, 0, 1024);
	sprintf( buf, "%d|%d|%d|%lld|%lld|%lld|%lld|%lld", nsrv, dbs, tabs, sel, ins, upd, del, sess ); 
	return buf;
}

// object method
// get number of databases, and tables
void JagDBServer::numDBTables( int &databases, int &tables )
{
	databases = tables = 0;
	Jstr dbs = JagSchema::getDatabases( _cfg, 0 );
	JagStrSplit sp(dbs, '\n', true );
	databases = sp.length();
	for ( int i = 0; i < sp.length(); ++i ) {
		JagVector<AbaxString> *vec = _tableschema->getAllTablesOrIndexes( sp[i], "" );
		tables += vec->size();
		if ( vec ) delete vec;
		vec = NULL;
	}
}

// object method
int JagDBServer::initObjects()
{
	jd(JAG_LOG_LOW, "begin initObjects ...\n" );
	
	_tableschema = new JagTableSchema( this, 0 );
	_prevtableschema = new JagTableSchema( this, 1 );
	_nexttableschema = new JagTableSchema( this, 2 );

	_indexschema = new JagIndexSchema( this, 0 );
	_previndexschema = new JagIndexSchema( this, 1 );
	_nextindexschema = new JagIndexSchema( this, 2 );
	
	_userDB = new JagUserID( 0 );
	_prevuserDB = new JagUserID( 1 );
	_nextuserDB = new JagUserID( 2 );

	_userRole = new JagUserRole( 0 );
	_prevuserRole = new JagUserRole( 1 );
	_nextuserRole = new JagUserRole( 2 );

	jd(JAG_LOG_LOW, "end initObjects\n" );
	jd(JAG_LOG_HIGH, "Initialized schema objects\n" );
	return 1;
}

int JagDBServer::createSocket( int argc, char *argv[] )
{
	AbaxString ports;

    if ( argc > 1 ) {
   	   _port = jagatoi( argv[1] );
    } else {
		ports = _cfg->getValue("PORT", "8888");
		_port = jagatoi( ports.c_str() );
		_listenIP = _cfg->getValue("LISTEN_IP", "");
	}

	_sock = JagNet::createIPV4Socket( _listenIP.c_str(), _port );
	if ( _sock < 0 ) {
		jd(JAG_LOG_LOW, "Failed to create server socket, exit\n" );
		exit( 51 );
	}

	if ( _listenIP.size() > 1 ) {
		jd(JAG_LOG_LOW, "Listen connections at %s:%d (socket=%d)\n", _listenIP.c_str(), _port, _sock );
	} else {
		jd(JAG_LOG_LOW, "Listen connections at port %d (socket=%d)\n", _port, _sock );
	}
	return 1;
}

int JagDBServer::initConfigs()
{
	Jstr cs;
	_isGate = 0;

	int threads = _numCPUs*_cfg->getIntValue("CPU_SELECT_FACTOR", 4);
	// _selectPool = new JaguarThreadPool( threads );
	jd(JAG_LOG_LOW, "Select thread pool %d\n", threads );

	threads = _numCPUs*_cfg->getIntValue("CPU_PARSE_FACTOR", 2);
	// _parsePool = new JaguarThreadPool( threads );
	jd(JAG_LOG_LOW, "Parser thread pool %d\n", threads );

	cs = _cfg->getValue("JAG_LOG_LEVEL", "0");
	JAG_LOG_LEVEL = jagatoi( cs.c_str() );
	if ( JAG_LOG_LEVEL <= 0 ) JAG_LOG_LEVEL = 1;
	jd(JAG_LOG_LOW, "JAG_LOG_LEVEL=%d\n", JAG_LOG_LEVEL );

	_version = Jstr(JAG_VERSION);
	jd(JAG_LOG_LOW, "Server version is %s\n", _version.c_str() );
	jd(JAG_LOG_LOW, "Server license is %s\n", PRODUCT_VERSION );

	_threadGroups = _faultToleranceCopy * _dbConnector->_nodeMgr->_numAllNodes * 15;
	/***
	if ( _isGate ) {
		cs = _cfg->getValue("INIT_EXTRA_THREADS", "100" );
	} else {
		cs = _cfg->getValue("INIT_EXTRA_THREADS", "50" );
	}
	***/
	cs = _cfg->getValue("INIT_EXTRA_THREADS", "50" );
	_initExtraThreads = jagatoi( cs.c_str() );


	jd(JAG_LOG_LOW, "Thread Groups = %d\n", _threadGroups );
	jd(JAG_LOG_LOW, "Init Extra Threads = %d\n", _initExtraThreads );

	// write process ID
	Jstr logpath = jaguarHome() + "/log/jaguar.pid";
	FILE *pidf = loopOpen( logpath.c_str(), "wb" );
	fprintf( pidf, "%d\n", getpid() );
	fflush( pidf ); jagfclose( pidf );

	jd(JAG_LOG_LOW, "Process ID %d in %s\n", getpid(), logpath.c_str() );

	_walLog = 0;
	cs = _cfg->getValue("WAL_LOG", "yes");
	if ( startWith( cs, 'y' ) ) {
		_walLog =  1;
		jd(JAG_LOG_LOW, "WAL Log %s\n", cs.c_str() );
	}

	cs = _cfg->getValue("FLUSH_WAIT", "1");
	_flushWait = jagatoi( cs.c_str() );

	cs = _cfg->getValue("DEBUG_CLIENT", "no");
    _debugClient = 0;
	if ( startWith( cs, 'y' ) ) {
		_debugClient =  1;
	} 

	jd(JAG_LOG_LOW, "DEBUG_CLIENT %s\n", cs.c_str() );

	_taskMap = new JagHashMap<AbaxLong,AbaxString>();
	_scMap = new JagHashMap<AbaxString, AbaxInt>();

	// read public and private keys
	int keyExists = 0;
	Jstr keyFile = jaguarHome() + "/conf/public.key";
	while ( ! keyExists ) {
		JagFileMgr::readTextFile( keyFile, _publicKey );
		if ( _publicKey.size() < 1 ) {
			jd(JAG_LOG_LOW, "Key file %s not found, wait ...\n", keyFile.c_str() );
			jd(JAG_LOG_LOW, "Please execute createKeyPair program to generate it.\n");
			jd(JAG_LOG_LOW, "Once conf/public.key and conf/private.key are generated, server will use them.\n");
			jagsleep(10, JAG_SEC);
		} else {
			keyExists = 1;
		}
	}

	keyFile = jaguarHome() + "/conf/private.key";
	keyExists = 0;
	while ( ! keyExists ) {
		JagFileMgr::readTextFile( keyFile, _privateKey );
		if ( _privateKey.size() < 1 ) {
			jd(JAG_LOG_LOW, "Key file %s not found, wait ...\n", keyFile.c_str() );
			jd(JAG_LOG_LOW, "Please execute createKeyPair program to generate it.\n");
			jd(JAG_LOG_LOW, "Once conf/public.key and conf/private.key are generated, server will use them.\n");
			jagsleep(10, JAG_SEC);
		} else {
			keyExists = 1;
		}
	}

	return 1;
}

// log command to the command wallog file
// spMode = 0 : special cmds, create/drop etc. 
// spMode = 1 : single regular cmds, update/delete etc.
// spMode = 2 : batch regular cmds, insert/finsert 
void JagDBServer::logCommand( const JagParseParam *pparam, JagSession *session, const char *mesg, jagint msglen, int spMode )
{
	Jstr db = pparam->objectVec[0].dbName;
	Jstr tab = pparam->objectVec[0].tableName;
	if ( db.size() < 1 || tab.size() <1 ) { return; }

	JAG_BLURT jaguar_mutex_lock ( &g_wallogmutex ); JAG_OVER

	Jstr fpath = _cfg->getWalLogHOME() + "/" + db + "." + tab + ".wallog";

	FILE *walFile = _walLogMap.ensureFile( fpath );
	if ( NULL == walFile ) {
		jd(JAG_LOG_LOW, "error open wallog %s\n", fpath.c_str() );
	} else {
		int isInsert;
		if ( 2==spMode ) isInsert = 1; else isInsert = 0;
		fprintf( walFile, "%d;%d;%d;%010lld%s",
		         session->replicType, session->timediff, isInsert, msglen, mesg );
	    fflush( walFile );
	}
	JAG_BLURT jaguar_mutex_unlock ( &g_wallogmutex ); 
}

// log command to the recovery log file
// spMode = 0 : special cmds, create/drop etc. spMode = 1 : single regular cmds, update/delete etc.
// spMode = 2 : batch regular cmds, insert etc. 
void JagDBServer::regSplogCommand( JagSession *session, const char *mesg, jagint len, int spMode )
{
    dn("s3600201 regSplogCommand mesg=[%s] spMMode=%d", mesg, spMode );
	// logformat
	// JAG_REDO_MSGLEN is 10, %010ld--> prepend 0, max 10 long digits 
	JAG_BLURT jaguar_mutex_lock ( &g_dlogmutex ); JAG_OVER
	if ( 0 == spMode && session->servobj->_recoverySpCommandFile ) {
		fprintf( session->servobj->_recoverySpCommandFile, "%d;%d;0;%010lld%s",
			     session->replicType, session->timediff, len, mesg );
        dn("s109228 log to _recoverySpCommandFile, datasync"); 
		jagfdatasync( fileno(session->servobj->_recoverySpCommandFile ) );
	} else if ( 0 != spMode && session->servobj->_recoveryRegCommandFile ) {
		fprintf( session->servobj->_recoverySpCommandFile, "%d;%d;%d;%010lld%s",
			     session->replicType, session->timediff, 2==spMode, len, mesg );
		if ( 2 == spMode ) {
            dn("s109208 log to _recoverySpCommandFile, datasync"); 
			jagfdatasync( fileno(session->servobj->_recoveryRegCommandFile ) );
		}
	}
	jaguar_mutex_unlock ( &g_dlogmutex );
}

// replicType 0/1/2;  mode:  0: YYY; 1: YYN; 2: YNY; 3: YNN; 4: NYY; 5: NYN; 6: NNY;
void JagDBServer::deltalogCommand( int mode, JagSession *session, const char *mesg, bool isBatch )
{	
	//JAG_BLURT jaguar_mutex_lock ( &g_dlogmutex ); JAG_OVER

	int needSync = 0;
	Jstr cmd;
	FILE *f1 = NULL;
	FILE *f2 = NULL;

    dn("s44983112 deltalogCommand mesg=[%s] isBatch=%d mode=%d", mesg, isBatch, mode );
	
	if ( !isBatch ) { // not batch command, parse and rebuild cmd with dbname inside
        dn("s2005301 not batch");
        /***
		Jstr reterr;
		JagParseAttribute jpa( session->servobj, session->timediff, session->servobj->servtimediff, 
							   session->dbname, session->servobj->_cfg );
		JagParser parser( (void*)this );
		JagParseParam parseParam(&parser);

		rc = parser.parseCommand( jpa, mesg, &parseParam, reterr );

		if ( rc ) {
			cmd = parseParam.dbNameCmd;
            dn("s326110 parser.parseCommand OK rc=%d  cmd=[%s]", rc, cmd.s() );
		} else {
            dn("s326110 parser.parseCommand error rc=%d  reterr=[%s] cmd=[]", rc, reterr.s() );
        }

		if ( parseParam.opcode != JAG_UPDATE_OP && parseParam.opcode != JAG_DELETE_OP ) {
			needSync = 1;
		}
        ***/
        cmd = mesg;
		needSync = 1;
	} else { // batch command, insert
        dn("s2005304 yes batch");
		cmd = mesg;
		needSync = 1;
	}
	
	if ( session->servobj->_faultToleranceCopy == 2 ) {
		if ( 1 == mode ) mode = 0;
		else if ( 3 == mode ) mode = 2;
		else if ( 5 == mode ) mode = 4;
	}

	// store to correct delta files
	if ( 1 == mode ) {
		if ( 0 == session->replicType ) {
            JAG_BLURT jaguar_mutex_lock ( &_delPrevRepMutex ); JAG_OVER
			f1 = session->servobj->_delPrevRepCommandFile;
            if ( f1 ) {
		        fprintf( f1, "%d;%s\n", session->timediff, cmd.c_str() );
                fflush( f1 );
            }
            jaguar_mutex_unlock( &_delPrevRepMutex );
		} else if ( 1 == session->replicType ) {
            JAG_BLURT jaguar_mutex_lock ( &_delPrevOriRepMutex ); JAG_OVER
			f1 = session->servobj->_delPrevOriRepCommandFile;
            if ( f1 ) {
		        fprintf( f1, "%d;%s\n", session->timediff, cmd.c_str() );
                fflush( f1 );
            }
            jaguar_mutex_unlock( &_delPrevOriRepMutex );
		} 
	} else if ( 2 == mode ) {
		if ( 0 == session->replicType ) {
            JAG_BLURT jaguar_mutex_lock ( &_delNextRepMutex ); JAG_OVER
			f1 = session->servobj->_delNextRepCommandFile;
            if ( f1 ) {
		        fprintf( f1, "%d;%s\n", session->timediff, cmd.c_str() );
                fflush( f1 );
            }
            jaguar_mutex_unlock( &_delNextRepMutex );
		} else if ( 2 == session->replicType ) {
            JAG_BLURT jaguar_mutex_lock ( &_delNextOriRepMutex ); JAG_OVER
			f1 = session->servobj->_delNextOriRepCommandFile;
            if ( f1 ) {
		        fprintf( f1, "%d;%s\n", session->timediff, cmd.c_str() );
                fflush( f1 );
            }
            jaguar_mutex_unlock( &_delNextOriRepMutex );
		} 
	} else if ( 3 == mode ) {
		if ( 0 == session->replicType ) {
            JAG_BLURT jaguar_mutex_lock( &_delPrevRepMutex ); JAG_OVER
			f1 = session->servobj->_delPrevRepCommandFile;
            if ( f1 ) {
		        fprintf( f1, "%d;%s\n", session->timediff, cmd.c_str() );
                fflush( f1 );
            }
            jaguar_mutex_unlock( &_delPrevRepMutex );


            JAG_BLURT jaguar_mutex_lock( &_delNextRepMutex ); JAG_OVER
			f2 = session->servobj->_delNextRepCommandFile;
            if ( f2 ) {
		        fprintf( f2, "%d;%s\n", session->timediff, cmd.c_str() );
                fflush( f2 );
            }
            jaguar_mutex_unlock( &_delNextRepMutex );
		} 
	} else if ( 4 == mode ) {
		if ( 1 == session->replicType ) {
            JAG_BLURT jaguar_mutex_lock( &_delPrevOriMutex ); JAG_OVER
			f1 = session->servobj->_delPrevOriCommandFile;
            if ( f1 ) {
		        fprintf( f1, "%d;%s\n", session->timediff, cmd.c_str() );
                fflush( f1 );
            }
            jaguar_mutex_unlock( &_delPrevOriMutex );

		} else if ( 2 == session->replicType ) {
            JAG_BLURT jaguar_mutex_lock( &_delNextOriMutex ); JAG_OVER
			f1 = session->servobj->_delNextOriCommandFile;		
            if ( f1 ) {
		        fprintf( f1, "%d;%s\n", session->timediff, cmd.c_str() );
                fflush( f1 );
            }
            jaguar_mutex_unlock( &_delNextOriMutex );
		} 
	} else if ( 5 == mode ) {
		if ( 1 == session->replicType ) {
            JAG_BLURT jaguar_mutex_lock( &_delPrevOriMutex ); JAG_OVER
			f1 = session->servobj->_delPrevOriCommandFile;
            if ( f1 ) {
		        fprintf( f1, "%d;%s\n", session->timediff, cmd.c_str() );
                fflush( f1 );
            }
            jaguar_mutex_unlock( &_delPrevOriMutex );


            JAG_BLURT jaguar_mutex_lock( &_delPrevOriRepMutex ); JAG_OVER
			f2 = session->servobj->_delPrevOriRepCommandFile;
            if ( f2 ) {
		        fprintf( f2, "%d;%s\n", session->timediff, cmd.c_str() );
                fflush( f2 );
            }
            jaguar_mutex_unlock( &_delPrevOriRepMutex );
		}
	} else if ( 6 == mode ) {
		if ( 2 == session->replicType ) {
            JAG_BLURT jaguar_mutex_lock( &_delNextOriMutex ); JAG_OVER
			f1 = session->servobj->_delNextOriCommandFile;
            if ( f1 ) {
		        fprintf( f1, "%d;%s\n", session->timediff, cmd.c_str() );
                fflush( f1 );
            }
            jaguar_mutex_unlock( &_delNextOriMutex );

            JAG_BLURT jaguar_mutex_lock( &_delNextOriRepMutex ); JAG_OVER
			f2 = session->servobj->_delNextOriRepCommandFile;
            if ( f2 ) {
		        fprintf( f2, "%d;%s\n", session->timediff, cmd.c_str() );
                fflush( f2 );
            }
            jaguar_mutex_unlock( &_delNextOriRepMutex );
		}
	}
	

    /***
	if ( needSync ) {
		if ( f1 ) {
            dn("s56002 jagfdatasync f1");
			jagfdatasync( fileno( f1 ) );
		}

		if ( f2 ) {
            dn("s56004 jagfdatasync f2");
			jagfdatasync( fileno( f2 ) );
		}

		if ( 0 == session->replicType && ! session->origserv ) {
			// if ( f1 || f2 ) JagStrSplitWithQuote split( mesg, ';' );
		}
	}
    ***/

	//jaguar_mutex_unlock ( &g_dlogmutex );
}

// method to recover delta log soon after delta log written, if connection has already been recovered
void JagDBServer::onlineRecoverDeltaLog()
{
	if ( _faultToleranceCopy <= 1 ) return; // no replicate

    dn("s7600123 onlineRecoverDeltaLog()");

	// use client to recover delta log
	if ( _delPrevOriCommandFile && JagFileMgr::fileSize(_actdelPOpath) > 0 ) {

        processDeltaLog( _actdelPOpath, _delPrevOriCommandFile, &_delPrevOriMutex );
	}

	if ( _delPrevRepCommandFile && JagFileMgr::fileSize(_actdelPRpath) > 0 ) { 

        processDeltaLog( _actdelPRpath, _delPrevRepCommandFile, &_delPrevRepMutex );
	}

	if ( _delPrevOriRepCommandFile && JagFileMgr::fileSize(_actdelPORpath) > 0 ) {

        processDeltaLog( _actdelPORpath, _delPrevOriRepCommandFile, &_delPrevOriRepMutex );
	}

	if ( _delNextOriCommandFile && JagFileMgr::fileSize(_actdelNOpath) > 0 ) {

        processDeltaLog( _actdelNOpath, _delNextOriCommandFile, &_delNextOriMutex );
	}

	if ( _delNextRepCommandFile && JagFileMgr::fileSize(_actdelNRpath) > 0 ) {

        processDeltaLog( _actdelNRpath, _delNextRepCommandFile, &_delNextRepMutex );

	}

	if ( _delNextOriRepCommandFile && JagFileMgr::fileSize(_actdelNORpath) > 0 ) {

        processDeltaLog( _actdelNORpath, _delNextOriRepCommandFile, &_delNextOriRepMutex );
	}

}

// return rc <0 for error; 0 for OK
int JagDBServer::recoverOneDeltaLog( const Jstr &fpath)
{
	jd(JAG_LOG_LOW, "begin online recoverDeltaLog %s ...\n", fpath.c_str() );
    jagint readRows = 0;
    jagint sentRows = 0;
    int   rc;

    JagVector<Jstr>  doneFile;

	int src = _dbConnector->_parentCli->sendDeltaLog( jaguarHome(), fpath, readRows, sentRows, doneFile );

	jd(JAG_LOG_LOW, "end online sending recoverDeltaLog src=%d rows read: %ld  row sent: %ld doneFile: %ld\n", 
                    src, readRows, sentRows, doneFile.size() );

    if( ( readRows == sentRows ) && (sentRows > 0) && (doneFile.size() > 0) ) {
        Jstr dirname, fname, histpath, tstr, newPath;
        Jstr doneF, cmd;

        for ( int i=0; i < doneFile.size(); ++i ) {
            doneF = doneFile[i];

            cmd = Jstr("gzip -f ") + doneF;
            system( cmd.s() );

            in("s8348999 compressed [%s] to [%s.gz]", doneF.s(), newPath.s() );
        }

        rc = 0;
    } else {
        in("s2002932 doneFile.size=%ld not sent to other nodes", doneFile.size() );
        rc = -1;
    }

    return rc;
}

void JagDBServer::moveToHistory( const Jstr &fpath, Jstr &histFile )
{
    Jstr dirname = JagFileMgr::dirName( fpath );
    Jstr fname = JagFileMgr::baseName( fpath );
    dn("s33330812 move file [%s] dirname=[%s] fname=[%s]", fpath.s(), dirname.s(), fname.s() );

    Jstr histpath = dirname + "/history";
    JagFileMgr::makedirPath( histpath );
    dn("s333401 makedirPath [%s]", histpath.s() );
    Jstr tstr = JagTime::YYYYMMDDHHMMSS();
    histFile = histpath + "/" + fname + "_" + tstr;

    jagrename( fpath.s(),  histFile.s() );
    in("s400392 move %s --> %s",  fpath.s(),  histFile.s() );
}


// close and reopen delta log file, for replicate use only
void JagDBServer::resetDeltaLog()
{
	if ( _faultToleranceCopy <= 1 ) return; // no replicate

	jd(JAG_LOG_LOW, "begin resetDeltaLog ...\n");

	Jstr  deltahome = JagFileMgr::getLocalLogDir("delta");
	Jstr  fpath, host0, host1, host2, host3, host4;
	int   pos1, pos2, pos3, pos4;

	JagStrSplit sp( _dbConnector->_nodeMgr->_hostClusterNodes, '|' );
	// set pos1: _nthServer+1; pos2: _nthServer-1
	if ( _nthServer == 0 ) {
		pos1 = _nthServer+1;
		pos2 = sp.length()-1;
	} else if ( _nthServer == sp.length()-1 ) {
		pos1 = 0;
		pos2 = _nthServer-1;
	} else {
		pos1 = _nthServer+1;
		pos2 = _nthServer-1;
	}
	// set pos3: pos1+1; pos4: pos2-1;
	if ( pos1 == sp.length()-1 ) {
		pos3 = 0;
	} else {
		pos3 = pos1+1;
	}
	if ( pos2 == 0 ) {
		pos4 = sp.length()-1;
	} else {
		pos4 = pos2-1;
	}
	// set host0, host1, host2, host3, host4
	host0 = sp[_nthServer];
	host1 = sp[pos1];
	host2 = sp[pos2];
	host3 = sp[pos3];
	host4 = sp[pos4];

    dn("s400298 host0=%s", host0.s() );
    dn("s400298 host1=%s", host1.s() );
    dn("s400298 host2=%s", host2.s() );
    dn("s400298 host3=%s", host3.s() );
    dn("s400298 host4=%s", host4.s() );
	
	if ( _faultToleranceCopy == 2 ) {
		if ( _delPrevOriCommandFile ) {
			jagfclose( _delPrevOriCommandFile );
		}
		if ( _delNextRepCommandFile ) {
			jagfclose( _delNextRepCommandFile );
		}
		
		fpath = deltahome + "/" + host2 + "_0";
		_delPrevOriCommandFile = loopOpen( fpath.c_str(), "ab" );
		_actdelPOpath = fpath;
		_actdelPOhost = host2;
		
		fpath = deltahome + "/" + host0 + "_1";	
		_delNextRepCommandFile = loopOpen( fpath.c_str(), "ab" );
		_actdelNRpath = fpath;
		_actdelNRhost = host1;

        dn("s34008 _actdelPOpath=%s _actdelPOhost=%s", _actdelPOpath.s(), _actdelPOhost.s() );
        dn("s34008 _actdelNRpath=%s _actdelNRhost=%s", _actdelNRpath.s(), _actdelNRhost.s() );

	} else if ( _faultToleranceCopy == 3 ) {
		if ( _delPrevOriCommandFile ) {
			jagfclose( _delPrevOriCommandFile );
		}
		if ( _delPrevRepCommandFile ) {
			jagfclose( _delPrevRepCommandFile );
		}
		if ( _delPrevOriRepCommandFile ) {
			jagfclose( _delPrevOriRepCommandFile );
		}
		if ( _delNextOriCommandFile ) {
			jagfclose( _delNextOriCommandFile );
		}
		if ( _delNextRepCommandFile ) {
			jagfclose( _delNextRepCommandFile );
		}
		if ( _delNextOriRepCommandFile ) {
			jagfclose( _delNextOriRepCommandFile );
		}
		
		fpath = deltahome + "/" + host2 + "_0";
		_delPrevOriCommandFile = loopOpen( fpath.c_str(), "ab" );
		_actdelPOpath = fpath;
		_actdelPOhost = host2;

		fpath = deltahome + "/" + host0 + "_2";
		_delPrevRepCommandFile = loopOpen( fpath.c_str(), "ab" );
		_actdelPRpath = fpath;
		_actdelPRhost = host2;

		fpath = deltahome + "/" + host2 + "_2";
		_delPrevOriRepCommandFile = loopOpen( fpath.c_str(), "ab" );
		_actdelPORpath = fpath;
		_actdelPORhost = host4; 

		fpath = deltahome + "/" + host1 + "_0";
		_delNextOriCommandFile = loopOpen( fpath.c_str(), "ab" );
		_actdelNOpath = fpath;
		_actdelNOhost = host1;

		fpath = deltahome + "/" + host0 + "_1";
		_delNextRepCommandFile = loopOpen( fpath.c_str(), "ab" );
		_actdelNRpath = fpath;
		_actdelNRhost = host1;

		fpath = deltahome + "/" + host1 + "_1";
		_delNextOriRepCommandFile = loopOpen( fpath.c_str(), "ab" );
		_actdelNORpath = fpath;
		_actdelNORhost = host3;
	}

	jd(JAG_LOG_LOW, "end resetDeltaLog\n");
}

int JagDBServer::checkDeltaFileStatus()
{
    jagint sz1 = JagFileMgr::fileSize(_actdelPOpath);
    dn("s400021 _actdelPOpath=[%s] size=%ld", _actdelPOpath.s(), sz1 );

    jagint sz2 = JagFileMgr::fileSize(_actdelPRpath);
    dn("s400021 _actdelPRpath=[%s] size=%ld", _actdelPRpath.s(), sz2 );

    jagint sz3 = JagFileMgr::fileSize(_actdelPORpath);
    dn("s400021 _actdelPORpath=[%s] size=%ld", _actdelPORpath.s(), sz3 );

    jagint sz4 = JagFileMgr::fileSize(_actdelNOpath);
    dn("s400021 _actdelNOpath=[%s] size=%ld", _actdelNOpath.s(), sz4 );

    jagint sz5 = JagFileMgr::fileSize(_actdelNRpath);
    dn("s400021 _actdelNRpath=[%s] size=%ld", _actdelNRpath.s(), sz5 );

    jagint sz6 = JagFileMgr::fileSize(_actdelNORpath);
    dn("s400021 _actdelNORpath=[%s] size=%ld", _actdelNORpath.s(), sz6 );

	if ( sz1 > 0 || sz2 > 0 || sz3 > 0 || sz4 > 0 || sz5 > 0 || sz6 > 0 ) {
		// has delta content
		return 1;
	} else {
	    return 0;
	}
}

// orginize and tar dir to be transmitted later
// mode 0: data; mode 1: pdata; mode 2: ndata
void JagDBServer::organizeCompressDir( int mode, Jstr &fpath )
{
	Jstr spath = _cfg->getJDBDataHOME( mode );
	fpath = _cfg->getTEMPDataHOME( mode ) + "/" + longToStr(THID) + ".tar.gz";
	Jstr cmd = Jstr("cd ") + spath + "; " + "tar -zcf " + fpath + " *";
	system(cmd.c_str());
	jd(JAG_LOG_LOW, "s6107 [%s]\n", cmd.c_str() );
}

// file open a.tar.gz and send to another server
// mode=0: main data; mode=1: pdata; mode=2: ndata  
// mode=10: transmit wallog file
// return 0: error; 1: OK
int JagDBServer::fileTransmit( const Jstr &host, unsigned int port, const Jstr &passwd, 
								const Jstr &connectOpt, int mode, const Jstr &fpath, int isAddCluster )
{
	if ( fpath.size() < 1 ) {
		return 0; // no fpath, no need to transfer
	}

	jd(JAG_LOG_LOW, "begin fileTransmit [%s]\n", fpath.c_str() );
	int pkgsuccess = false;
	
	int hdrsz = JAG_SOCK_TOTAL_HDR_LEN;
	jagint rc;
	ssize_t rlen;
	struct stat sbuf;
	if ( 0 != stat(fpath.c_str(), &sbuf) || sbuf.st_size <= 0 ) {
		jd(JAG_LOG_LOW, "E12026 error stat [%s], return\n", fpath.c_str() );
		return 0;
	}

	//char tothdr[ hdrsz +1 ];
	char sqlhdr[ 8 ];

	while ( !pkgsuccess ) {
		JaguarCPPClient pcli;
		pcli._deltaRecoverConnection = 2;
		if ( !pcli.connect( host.c_str(), port, "admin", passwd.c_str(), "test", connectOpt.c_str(), JAG_CONNECT_ONE, _servToken.c_str() ) ) {
			jd(JAG_LOG_LOW, "s4058 recover failure, unable to connect %s:%d ...\n", host.c_str(), _port );
			if ( !isAddCluster ) jagunlink( fpath.c_str() );
			return 0;
		}

		Jstr cmd;
		int fd = jagopen( fpath.c_str(), O_RDONLY, S_IRWXU);
		if ( fd < 0 ) {
			if ( !isAddCluster ) jagunlink( fpath.c_str() );
			pcli.close();
			jd(JAG_LOG_LOW, "E0838 end fileTransmit [%s] open error\n", fpath.c_str() );
			return 0;
		}

		if ( isAddCluster ) {
			cmd = "_serv_addbeginfxfer|";
		} else {
			cmd = "_serv_beginfxfer|";
		}

		cmd += intToStr( mode ) + "|" + longToStr(sbuf.st_size) + "|" + longToStr(THID);
		char cmdbuf[JAG_SOCK_TOTAL_HDR_LEN+cmd.size()+1];
		makeSQLHeader( sqlhdr );
		putXmitHdrAndData( cmdbuf, sqlhdr, cmd.c_str(), cmd.size(), "ANCC" );

		rc = sendRawData( pcli.getSocket(), cmdbuf, hdrsz+cmd.size() ); // xxx00000000168ANCCmessage client query mode
		if ( rc < 0 ) {
			jagclose( fd );
			pcli.close();
			jd(JAG_LOG_LOW, "E0831 retry fileTransmit [%s] sendrawdata error\n", fpath.c_str() );
			continue;
		}

        dn("s202029001 sendRawData of hdr is done rc=%d OK. Now do jagsendfile ...", rc );
			
        if ( socket_bad(pcli.getSocket() ) ) {
            rlen = -1;
        } else {
		    //beginBulkSend( pcli.getSocket() );
		    rlen = jagsendfile( pcli.getSocket(), fd, sbuf.st_size );
		    //endBulkSend( pcli.getSocket() );
        }

		if ( rlen < sbuf.st_size ) {
			jagclose( fd );
			pcli.close();
			jd(JAG_LOG_LOW, "E8371 retry fileTransmit [%s] sendfile error sendfile_len=%ld sbuf.st_size=%ld\n", fpath.c_str(), rlen, sbuf.st_size );
			sleep(10);
			continue;
		}

		jd(JAG_LOG_LOW, "fileTransmit [%s] sendfile %ld bytes to host=[%s] port=%u\n", fpath.c_str(), rlen, host.c_str(), port );

		jagclose( fd );
		if ( !isAddCluster ) jagunlink( fpath.c_str() );
		pkgsuccess = true;
		pcli.close();
		jd(JAG_LOG_LOW, "end fileTransmit [%s] OK\n", fpath.c_str() );
	}

	return 1;
}

// method to unzip tar.gz file and rebuild necessary memory objects
void JagDBServer::recoveryFromTransferredFile()
{
	if ( _faultToleranceCopy <= 1 ) return; // no replicate

	jd(JAG_LOG_LOW, "begin redo xfer data ...\n" );

	Jstr cmd, datapath, tmppath, fpath;
	bool isdo = false;

	// first, need to drop all current tables and indexs if untar needed
	JagRequest req;
	JagSession session;
	session.servobj = this;
	session.serverIP = _localInternalIP;

	req.session = &session;

	// process original data first
	tmppath = _cfg->getTEMPDataHOME( JAG_MAIN );
	datapath = _cfg->getJDBDataHOME( JAG_MAIN );

	JagStrSplit sp( _crecoverFpath.c_str(), '|', true );

	if ( sp.length() > 0 ) {
		jd(JAG_LOG_LOW, "cleanredo file [%s]\n", _crecoverFpath.c_str() );
		isdo = true;
		// if more than one file has received, need to consider which file to be used
		// since all packages sent are non-recover server, use first package is enough
		
		session.replicType = 0;
		dropAllTablesAndIndex( req, _tableschema );

		JagFileMgr::rmdir( datapath, false );

        applyMultiTars( tmppath, _crecoverFpath, datapath );
	}

	if ( _faultToleranceCopy >= 2 ) {
		// process prev data dir
		tmppath = _cfg->getTEMPDataHOME( JAG_PREV );
		datapath = _cfg->getJDBDataHOME( JAG_PREV );
		sp.init( _prevcrecoverFpath.c_str(), -1, '|', true );
		if ( sp.length() > 0 ) {
			jd(JAG_LOG_LOW, "prevcleanredo file [%s]\n", _prevcrecoverFpath.c_str() );
			isdo = true;
			// if more than one file has received, need to consider which file to be used
			// since all packages sent are non-recover server, use first package is enough
	
			session.replicType = 1;
			dropAllTablesAndIndex( req, _prevtableschema );
			JagFileMgr::rmdir( datapath, false );
			fpath = tmppath + "/" + sp[0];
			cmd = Jstr("tar -zxf ") + fpath + " --keep-newer-files --directory=" + datapath;
			system( cmd.c_str() );
			jd(JAG_LOG_LOW, "s6102 crecover system cmd[%s]\n", cmd.c_str() );

			// remove tar.gz files
			for ( int i = 0; i < sp.length(); ++i ) {
				Jstr dfp = tmppath+"/"+sp[i];
				jagunlink(dfp.c_str());
				jd(JAG_LOG_LOW, "delete [%s]\n", dfp.c_str() );
			}
		}
	}

	if ( _faultToleranceCopy >= 3 ) {
		// process next data dir
		tmppath = _cfg->getTEMPDataHOME( JAG_NEXT );
		datapath = _cfg->getJDBDataHOME( JAG_NEXT );
		sp.init( _nextcrecoverFpath.c_str(), -1, '|', true );
		if ( sp.length() > 0 ) {
			jd(JAG_LOG_LOW, "nextcleanredo file [%s]\n", _nextcrecoverFpath.c_str() );
			isdo = true;
			// if more than one file has received, need to consider which file to be used
			// since all packages sent are non-recover server, use first package is enough

			session.replicType = 2;
			dropAllTablesAndIndex( req, _nexttableschema );
			JagFileMgr::rmdir( datapath, false );
			fpath = tmppath + "/" + sp[0];
			cmd = Jstr("tar -zxf ") + fpath + " --keep-newer-files --directory=" + datapath;
			system( cmd.c_str() );
			jd(JAG_LOG_LOW, "s6104 crecover system cmd[%s]\n", cmd.c_str() );

			// remove tar.gz files
			for ( int i = 0; i < sp.length(); ++i ) {
				jagunlink((tmppath+"/"+sp[i]).c_str());
				jd(JAG_LOG_LOW, "delete %s\n", sp[i].c_str() );
			}
		}		
	}

	jd(JAG_LOG_LOW, "end redo xfer data\n" );
	
	// after finish copying files, refresh some related memory objects
	if ( isdo ) crecoverRefreshSchema( JAG_MAKE_OBJECTS_CONNECTIONS );
	_crecoverFpath = "";
	_prevcrecoverFpath = "";
	_nextcrecoverFpath = "";
}

// mode JAG_MAKE_OBJECTS_CONNECTIONS: rebuild all objects and remake connections
// mode JAG_MAKE_OBJECTS_ONLY: rebuild all objects only
// mode JAG_MAKE_CONNECTIONS_ONLY: remake connections only
void JagDBServer::crecoverRefreshSchema( int mode, bool doRestoreInsertBufferMap )
{
	// destory schema related objects and rebuild them
	jd(JAG_LOG_LOW, "begin redo schema mode=%d ...\n", mode );

	if ( JAG_MAKE_OBJECTS_CONNECTIONS == mode || JAG_MAKE_OBJECTS_ONLY == mode ) {
		if ( _userDB ) {
			delete _userDB;
			_userDB = NULL;
		}

		if ( _prevuserDB ) {
			delete _prevuserDB;
			_prevuserDB = NULL;
		}

		if ( _nextuserDB ) {
			delete _nextuserDB;
			_nextuserDB = NULL;
		}

		if ( _userRole ) {
			delete _userRole;
			_userRole = NULL;
		}

		if ( _prevuserRole ) {
			delete _prevuserRole;
			_prevuserRole = NULL;
		}

		if ( _nextuserRole ) {
			delete _nextuserRole;
			_nextuserRole = NULL;
		}

		if ( _tableschema ) {
			delete _tableschema;
			_tableschema = NULL;
		}

		if ( _indexschema ) {
			delete _indexschema;
			_indexschema = NULL;
		}

		if ( _prevtableschema ) {
			delete _prevtableschema;
			_prevtableschema = NULL;
		}

		if ( _previndexschema ) {
			delete _previndexschema;
			_previndexschema = NULL;
		}
	
		if ( _nexttableschema ) {
			delete _nexttableschema;
			_nexttableschema = NULL;
		}

		if ( _nextindexschema ) {
			delete _nextindexschema;
			_nextindexschema = NULL;
		}

        dn("s88091003  _objectLock->rebuildObjects() ");
		_objectLock->rebuildObjects();

		Jstr dblist, dbpath;
		dbpath = _cfg->getJDBDataHOME( JAG_MAIN );
		dblist = JagFileMgr::listObjects( dbpath );
		_objectLock->setInitDatabases( dblist, JAG_MAIN );

		dbpath = _cfg->getJDBDataHOME( JAG_PREV );
		dblist = JagFileMgr::listObjects( dbpath );
		_objectLock->setInitDatabases( dblist, JAG_PREV );

		dbpath = _cfg->getJDBDataHOME( JAG_NEXT );
		dblist = JagFileMgr::listObjects( dbpath );
		_objectLock->setInitDatabases( dblist, JAG_NEXT );

		initObjects();

		makeTableObjects( doRestoreInsertBufferMap );
	}

	if ( JAG_MAKE_OBJECTS_CONNECTIONS == mode || JAG_MAKE_CONNECTIONS_ONLY == mode ) {
		_dbConnector->makeInitConnection( _debugClient );

		JagRequest req;
		JagSession session;
		session.servobj = this;
        session.serverIP = _localInternalIP;
		session.replicType = 0;
		req.session = &session;

		refreshSchemaInfo( session.replicType, g_lastSchemaTime );
	}

	jd(JAG_LOG_LOW, "end redo schema\n" );
}

// deprecated
// close and reopen temp log file to store temporary write related commands while recovery
void JagDBServer::resetRegSpLog()
{
	jd(JAG_LOG_LOW, "begin reset data/schema log\n" );
	
    Jstr deltahome = JagFileMgr::getLocalLogDir("delta");
    Jstr fpath = deltahome + "/redodata.log";
    if ( _recoveryRegCommandFile ) {
        jagfclose( _recoveryRegCommandFile );
    }
    if ( _recoverySpCommandFile ) {
        jagfclose( _recoverySpCommandFile );
    }
	
	_recoveryRegCmdPath = fpath;
    _recoveryRegCommandFile = loopOpen( fpath.c_str(), "ab" );
	
	fpath = deltahome + "/redometa.log";
	_recoverySpCmdPath = fpath;
    _recoverySpCommandFile = loopOpen( fpath.c_str(), "ab" );
	jd(JAG_LOG_LOW, "end reset data/schema log\n" );
}

void JagDBServer::recoverRegSpLog()
{
	jd(JAG_LOG_LOW, "s11210 begin redo data and schema ...\n" );
	jagint cnt;

	// global lock
	JAG_BLURT jaguar_mutex_lock ( &g_flagmutex ); JAG_OVER

	// check sp command first
	if ( _recoverySpCommandFile && JagFileMgr::fileSize(_recoverySpCmdPath) > 0 ) {
		jagfclose( _recoverySpCommandFile );
		_recoverySpCommandFile = NULL;
		jd(JAG_LOG_LOW, "begin redo metadata [%s] ...\n", _recoverySpCmdPath.c_str() );
		cnt = redoWalLog( _recoverySpCmdPath );
		jd(JAG_LOG_LOW, "end redo metadata [%s] count=%ld\n", _recoverySpCmdPath.c_str(), cnt );
		jagunlink( _recoverySpCmdPath.c_str() );
		_recoverySpCommandFile = loopOpen( _recoverySpCmdPath.c_str(), "ab" );
	}

	// unlock global lock
	_restartRecover = 0;
	jaguar_mutex_unlock ( &g_flagmutex );

	// check reg command to redo log
	if ( _recoveryRegCommandFile && JagFileMgr::fileSize(_recoveryRegCmdPath) > 0 ) {
		jagfclose( _recoveryRegCommandFile );
		_recoveryRegCommandFile = NULL;
		jd(JAG_LOG_LOW, "begin redo data [%s] ...\n", _recoveryRegCmdPath.c_str() );
		cnt = redoWalLog( _recoveryRegCmdPath );
		jd(JAG_LOG_LOW, "end redo data [%s] count=%ld\n", _recoveryRegCmdPath.c_str(), cnt );
		jagunlink( _recoveryRegCmdPath.c_str() );
		_recoveryRegCommandFile = loopOpen( _recoveryRegCmdPath.c_str(), "ab" );
	}

	jd(JAG_LOG_LOW, "end redo data and schema\n" );
}

// read uncommited logs in cmd/ dir and execute them
jagint JagDBServer::recoverWalLog( )
{
    dn("s100023 recoverWalLog ...");

	if ( ! _walLog ) {
        dn("s350287 recoverWalLog _walLog==NULL return");
        return 0;
    }
	
	Jstr walpath = _cfg->getWalLogHOME();
	Jstr fpath;
	Jstr fileNames = JagFileMgr::listObjects( walpath, ".wallog" );
    dn("s81635 wallog files=[%s}", fileNames.s() );

	JagStrSplit sp( fileNames, '|');

    jagint total = 0;
	for ( int i=0; i < sp.size(); ++i ) {
		fpath = walpath + "/" + sp[i];
		jd(JAG_LOG_LOW, "begin redoWalLog %s ...\n",  fpath.c_str() );

		jagint cnt = redoWalLog( fpath );
        total += cnt;

		jd(JAG_LOG_LOW, "end redoWalLog %s cnt=%ld\n",  fpath.c_str(), cnt );
	}

    return total;
}

// execute commands in fpath one by one
// replicType;client_timediff;isInsert;ddddddddddddddddqstzreplicType;client_timediff;isBatch;ddddddddddddddddqstr
jagint JagDBServer::redoWalLog( const Jstr &fpath )
{
	if ( JagFileMgr::fileSize( fpath ) <= 0 ) {
        dn("s01252 fpath=[%s] size <= 0 skip", fpath.s() );
		return 0;
	}

	int i, fd = jagopen( fpath.c_str(), O_RDONLY|JAG_NOATIME );
	if ( fd < 0 ) {
        dn("s91252 fpath=[%s] open error", fpath.s() );
        return 0;
    }

	char buf16[17];
	jagint len;
	char c;
	char *buf = NULL;

	JagRequest req;
	JagSession session;

	req.hasReply = false;
	session.servobj = this;
	session.dbname = "test";
	session.uid = "admin";
	session.origserv = 1;
	session.drecoverConn = 3;
    session.serverIP = _localInternalIP;
	req.session = &session;

	jagint cnt0 = 0;
	jagint proccnt = 0;
	int  prc;

	while ( 1 ) {
		i = 0;
		memset( buf16, 0, 4 );
		while( 1==read(fd, &c, 1) ) {
			buf16[i] = c;
			if ( c == ';' ) {
				buf16[i] =  '\0';
				break;
			} else {
				if ( i > 0 ) {
					jagclose( fd );
					return cnt0;
				}
			}
			++i;
		}

		if ( buf16[0] == '\0' ) {
			break;
		}

		session.replicType = jagatoi( buf16 );
		if ( buf16[0] != '0' && buf16[0] != '1' && buf16[0] != '2' ) {
			jd(JAG_LOG_LOW, "Error in wal file [%s]. Please fix it and then restart jaguardb. cnt=%lld\n", fpath.c_str(), cnt0 );
			break;
		}

        dn("s2920301 redo session.replicType=%d", session.replicType );

		// get time zone diff
		i = 0;
		memset( buf16, 0, 17 );
		while( 1==read(fd, &c, 1) ) {
			buf16[i] = c;
			if ( c == ';' ) {
				buf16[i] =  '\0';
				break;
			} else {
				if ( i > 15 ) {
					jagclose( fd );
					return cnt0;
				}
			}
			++i;
		}

		if ( buf16[0] == '\0' ) {
			printf("s3398 end timediff 0 is 0\n");
			break;
		}
		session.timediff = jagatoi( buf16 );

		// get isBatch
		i = 0;
		memset( buf16, 0, 4 );
		while( 1==read(fd, &c, 1) ) {
			buf16[i] = c;
			if ( c == ';' ) {
				buf16[i] =  '\0';
				break;
			} else {
				if ( i > 0 ) {
					jagclose( fd );
					return cnt0;
				}
			}
			++i;
		}

		if ( buf16[0] == '\0' ) {
			printf("s3398 end isBatch 0 is 0\n");
			break;
		}

		req.batchReply = jagatoi( buf16 );
        dn("s3029132 req.batchReply=%d", req.batchReply );

		// get mesg len
		memset( buf16, 0, JAG_REDO_MSGLEN+1 );
		raysaferead( fd, buf16, JAG_REDO_MSGLEN );
		len = jagatoll( buf16 );

		buf = (char*)jagmalloc(len+1);
		memset(buf, 0, len+1);
		raysaferead( fd, buf, len );

		prc = 0;
		try {
			// in redoWalLog
            dn("s922112 redo sql buf=[%s]", buf );
			prc = processMultiSingleCmd( req, buf, len, g_lastSchemaTime, g_lastHostTime, 0, true, 1, "" );
			//jd(JAG_LOG_LOW, "redolog rep: %d [%s] prc=%d\n", session.replicType, buf, prc );
		} catch ( const char *e ) {
			jd(JAG_LOG_LOW, "redolog processMultiSingleCmd [%s] caught exception [%s]\n", buf, e );
		} catch ( ... ) {
			jd(JAG_LOG_LOW, "redolog processMultiSingleCmd [%s] caught unknown exception\n", buf );
		}

		free( buf );
		buf = NULL;
		if ( req.session->replicType == 0 ) ++cnt0;

		proccnt += prc;

		if ( (cnt0%50000) == 0 ) {
			jd(JAG_LOG_LOW, "redo rep0_count=%ld  allrep_proccnt=%ld\n", cnt0, proccnt );
		}
	}

	jagclose( fd );
	jd(JAG_LOG_LOW, "redoWalLog done  count0=%ld proccnt=%ld\n", cnt0, proccnt );
	return cnt0;
}

// object method
// goto to each table/index, write the bottom level to a disk file
void JagDBServer::flushAllBlockIndexToDisk()
{
	JagTable *ptab; 
	Jstr str;
	int lockrc;

	for ( int i = 0; i < _faultToleranceCopy; ++i ) {
		jd(JAG_LOG_LOW, "Copy %d/%d:\n", i, _faultToleranceCopy );
		str = _objectLock->getAllTableNames( i );
		JagStrSplit sp( str, '|', true );
		for ( int j = 0; j < sp.length(); ++j ) {
			JagStrSplit sp2( sp[j], '.', true );
			jd(JAG_LOG_LOW, "  Table %s\n", sp[j].c_str() );
			ptab = _objectLock->readLockTable( JAG_INSERT_OP, sp2[0], sp2[1], i, true, lockrc );
			if ( ptab ) {
				ptab->flushBlockIndexToDisk();
				_objectLock->readUnlockTable( JAG_INSERT_OP, sp2[0], sp2[1], i, true );
			}
		}
	}
}

// object method
// goto to each table/index, remove the bottom level idx file
void JagDBServer::removeAllBlockIndexInDisk()
{
	removeAllBlockIndexInDiskAll( _tableschema, "/data/" );
	removeAllBlockIndexInDiskAll( _prevtableschema, "/pdata/" );
	removeAllBlockIndexInDiskAll( _nexttableschema, "/ndata/" );
}

void JagDBServer::removeAllBlockIndexInDiskAll( const JagTableSchema *tableschema, const char *datapath )
{
	if ( ! tableschema ) return;
	Jstr dbtab, db, tab, idx, fpath;
	JagVector<AbaxString> *vec = tableschema->getAllTablesOrIndexes( "", "" );
	for ( int i = 0; i < vec->size(); ++i ) {
		dbtab = (*vec)[i].c_str();
		//printf("s4490 dbtab=[%s]\n", dbtab.c_str() );
		JagStrSplit sp(dbtab, '.');
		if ( sp.length() >= 2 ) {
			db = sp[0];
			tab = sp[1];
			fpath = jaguarHome() + datapath + db + "/" + tab  + ".bid";
			jagunlink( fpath.c_str() );

			fpath = jaguarHome() + datapath + db + "/" + tab +"/" + tab + ".bid";
			jagunlink( fpath.c_str() );
			//printf("s4401 jagunlink [%s]\n", fpath.c_str() );
		}
	}
	if ( vec ) delete vec;
}

// obj method
// look at _taskMap and get all TaskIDs with the same threadID
Jstr JagDBServer::getTaskIDsByThreadID( jagint threadID )
{
	Jstr taskIDs;
	const AbaxPair<AbaxLong,AbaxString> *arr = _taskMap->array();
	jagint len = _taskMap->arrayLength();
	// sprintf( buf, "%14s  %20s  %16s  %16s  %16s %s\n", "TaskID", "ThreadID", "User", "Database", "StartTime", "Command");
	for ( jagint i = 0; i < len; ++i ) {
		if ( ! _taskMap->isNull( i ) ) {
			const AbaxPair<AbaxLong,AbaxString> &pair = arr[i];
			if ( pair.key != AbaxLong(threadID) ) { continue; }
			JagStrSplit sp( pair.value.c_str(), '|' );
			// "threadID|userid|dbname|timestamp|query"
			if ( taskIDs.size() < 1 ) {
				taskIDs = sp[0];
			} else {
				taskIDs += Jstr("|") + sp[0];
			}
		}
	}
	return taskIDs;
}

#if 0
int JagDBServer::schemaChangeCommandSyncRemove( const Jstr & dbobj )
{
	_scMap->removeKey( dbobj );
	return 1;
}
#endif

// only flush one table ( and related indexs ) insert buffer or one index insert buffer
// dbobj must be db.tab or db.idx
void JagDBServer::flushOneTableAndRelatedIndexsInsertBuffer( const Jstr &dbobj, int replicType, int isTable, 
	JagTable *iptab, JagIndex *ipindex )
{
	return;  // new method of compf

}

// insert one record
// return 0: error   1: OK
int JagDBServer::doInsert( JagRequest &req, JagParseParam &parseParam, 
						   Jstr &reterr, const Jstr &oricmd )
{
	jagint      cnt = 0;
	Jstr        dbobj;
	JagTable    *ptab = NULL;
	Jstr        dbName = parseParam.objectVec[0].dbName;
	Jstr        tableName = parseParam.objectVec[0].tableName;

	JagTableSchema *tableschema = getTableSchema( req.session->replicType );

	int lockrc;

	if ( parseParam.objectVec.size() > 0 ) {
        dn("s538002 req.session->replicType=%d dbName=%s tableName=%s", req.session->replicType, dbName.s(), tableName.s() );
		ptab = _objectLock->writeLockTable( parseParam.opcode, dbName, tableName, 
											tableschema, req.session->replicType, 0, lockrc );
        dn("s538002 req.session->replicType=%d dbName=%s tableName=%s ptab=%p", req.session->replicType, dbName.s(), tableName.s(), ptab );
	}

	if ( ! ptab ) {
		dbobj = dbName + "." + tableName;
		jd(JAG_LOG_HIGH, "s4304 table %s not found\n", dbobj.c_str() );
		reterr = "E20040 Error: Table " + dbobj + " not found";
		return 0;
	}

	if ( JAG_INSERT_OP == parseParam.opcode ) {

        dn("s4003001 ptab->insert() ...");
		cnt = ptab->insert( req, &parseParam, reterr );

		if ( 1 == cnt ) {
			_dbLogger->logmsg( req, "INS", oricmd );
			Jstr tser;
			if ( ptab->hasTimeSeries( tser ) ) {
				insertToTimeSeries( ptab->_tableRecord, req, parseParam, tser, dbName, tableName, tableschema, 
                                    req.session->replicType, oricmd );
			}
		} else {
			_dbLogger->logerr( req, reterr, oricmd );
		}
		++ numInserts;

	} else if ( JAG_FINSERT_OP == parseParam.opcode ) {

        dn("s40028501 JAG_FINSERT_OP");
		cnt = ptab->finsert( req, &parseParam, reterr );
        dn("s40028501 JAG_FINSERT_OP finsert done");

		++ numInserts;
		if ( 1 == cnt ) {
			_dbLogger->logmsg( req, "INS", oricmd );
		} else {
			_dbLogger->logerr( req, reterr, oricmd );
		}
	} 

	_objectLock->writeUnlockTable( parseParam.opcode, dbName, tableName, req.session->replicType, 0 );
	
	return 1;
}

// handle signals
#ifndef _WINDOWS64_
int JagDBServer::processSignal( int sig )
{
	if ( sig == SIGHUP ) {
		jd(JAG_LOG_LOW, "Processing SIGHUP ...\n" );
		sig_hup( sig );
	} else if ( sig == SIGINT || sig == SIGTERM ) {
		JagRequest req;
		JagSession session;
		session.uid = "admin";
		session.exclusiveLogin = 1;
		session.servobj = this;
        session.serverIP = _localInternalIP;

		req.session = &session;
		jd(JAG_LOG_LOW, "Processing [%s(%d)] ...\n", strsignal(sig), sig );
		shutDown( "_exe_shutdown", req );
	} else if ( sig == SIGSYS ) {
		jd(JAG_LOG_LOW, "SIGSYS signal [%d] ignored.\n", sig );
	} else if ( sig == SIGSEGV ) {
		jd(JAG_LOG_LOW, "SIGSEGV signal [%d] ignored.\n", sig );
	} else {
		if ( sig != SIGCHLD && sig != SIGWINCH ) {
			jd(JAG_LOG_LOW, "Unknown signal [%s(%d)] ignored.\n", strsignal(sig), sig );
		}
	}
	return 1;
}
#else
// WINDOWS
int JagDBServer::processSignal( int sig )
{
	if ( sig == JAG_CTRL_HUP ) {
		jd(JAG_LOG_LOW, "Processing SIGHUP ...\n" );
		sig_hup( sig );
	} else if ( sig == JAG_CTRL_CLOSE ) {
		JagRequest req;
		JagSession session;
		session.uid = "admin";
		session.exclusiveLogin = 1;
		session.servobj = this;
		req.session = &session;
		jd(JAG_LOG_LOW, "Processing [%d] ...\n", sig );
		shutDown( "_exe_shutdown", req );
	} else {
		jd(JAG_LOG_LOW, "Unknown signal [%d] ignored.\n", sig );
	}
	return 1;
}

#endif


bool JagDBServer::isInteralIP( const Jstr &ip )
{
	bool rc = false;
	JagStrSplit spp( _dbConnector->_nodeMgr->_hostClusterNodes, '|' );
	for ( int i = 0; i < spp.length(); ++i ) {
		if ( spp[i] == ip ) {
			rc = true;
			break;
		}
	}
	return rc;
}

bool JagDBServer::existInAllHosts( const Jstr &ip )
{
	return isInteralIP( ip );
}

void JagDBServer::initDirs()
{
    Jstr jagHome = jaguarHome();

	Jstr fpath = jagHome + "/data/system/schema";
	JagFileMgr::makedirPath( fpath, 0700 );

	fpath = jagHome + "/pdata/system/schema";
	JagFileMgr::makedirPath( fpath, 0700 );

	fpath = jagHome + "/ndata/system/schema";
	JagFileMgr::makedirPath( fpath, 0700 );

	fpath = jagHome + "/log/cmd";
	JagFileMgr::makedirPath( fpath, 0700 );

	fpath = jagHome + "/log/delta";
	JagFileMgr::makedirPath( fpath, 0700 );

	fpath = jagHome + "/export";
	JagFileMgr::makedirPath( fpath, 0700 );

	fpath = jagHome + "/tmp/*";
	Jstr cmd = Jstr("/bin/rm -rf ") + fpath;
	system( cmd.c_str() );

	fpath = jagHome + "/tmp/data";
	JagFileMgr::makedirPath( fpath );

	fpath = jagHome + "/tmp/pdata";
	JagFileMgr::makedirPath( fpath );

	fpath = jagHome + "/tmp/ndata";
	JagFileMgr::makedirPath( fpath );

	fpath = jagHome + "/tmp/join";
	JagFileMgr::makedirPath( fpath );

	fpath = jagHome + "/log/delta";
	JagFileMgr::makedirPath( fpath );

}

// negative reply to client
void JagDBServer::noGood( JagRequest &req, JagParseParam &parseParam )
{
	//sendMessageLength( req, "NG", 2, "SS" );
	sendDataEnd( req, "NG");
	return;
}

int JagDBServer::createSimpleTable( const JagRequest &req, const Jstr &dbname, const JagParseParam *parseParam )
{
	Jstr tableName = parseParam->objectVec[0].tableName;
	Jstr dbtable = dbname + "." + tableName;
	int  replicType = req.session->replicType;
	int opcode = parseParam->opcode;

	bool found1 = false, found2 = false;
	int repType =  req.session->replicType;
	JagTableSchema *tableschema;
	JagIndexSchema *indexschema;
	getTableIndexSchema( replicType, tableschema, indexschema );

	found1 = indexschema->tableExist( dbname, parseParam );
	found2 = tableschema->existAttr( dbtable );

	if ( found1 || found2 ) {
        dn("s36002 exists already return 0");
		return 0;
	}
	
    tableschema->insert( parseParam );
    refreshSchemaInfo( repType, g_lastSchemaTime );
    _objectLock->getTable( opcode, dbname, tableName, tableschema, repType );

	if ( parseParam->isChainTable ) {
		jd(JAG_LOG_LOW, "user [%s] create simple chain [%s] reptype=%d\n", 
				 req.session->uid.c_str(), dbtable.c_str(), repType );
	} else {
		jd(JAG_LOG_LOW, "user [%s] create simple table [%s] reptype=%d\n", 
				 req.session->uid.c_str(), dbtable.c_str(), repType );
	}

	return 1;
}

// return 1: OK  0: error
int JagDBServer::dropSimpleTable( const JagRequest &req, const JagParseParam *parseParam, Jstr &reterr, bool lockSchema )
{
	Jstr dbname = parseParam->objectVec[0].dbName;
	Jstr tabname = parseParam->objectVec[0].tableName;

	Jstr dbobj = dbname + "." + tabname;

	JagTable *ptab = NULL;
	JagTableSchema *tableschema;
	JagIndexSchema *indexschema;
	getTableIndexSchema( req.session->replicType, tableschema, indexschema );
	int lockrc;

	ptab = _objectLock->writeLockTable( parseParam->opcode, dbname, tabname, tableschema, req.session->replicType, 0, lockrc );

	if ( ptab ) {
		ptab->drop( reterr ); 
	}

	if ( lockSchema ) {
		JAG_BLURT jaguar_mutex_lock ( &g_dbschemamutex ); JAG_OVER
	}
	tableschema->remove( dbobj );
	refreshSchemaInfo( req.session->replicType, g_lastSchemaTime );

	if ( lockSchema ) {
		jaguar_mutex_unlock ( &g_dbschemamutex );
	}

	// remove wallog
	JAG_BLURT jaguar_mutex_lock ( &g_wallogmutex ); JAG_OVER
	Jstr fpath = _cfg->getWalLogHOME() + "/" + dbname + "." + tabname + ".wallog";
	jagunlink( fpath.s() );
	JAG_BLURT jaguar_mutex_unlock ( &g_wallogmutex ); 
	
	// drop table and related indexs
	if ( ptab ) {
		_objectLock->writeUnlockTable( parseParam->opcode, dbname, tabname, req.session->replicType, 0 ); 
		delete ptab; 
		return 1;
	} else {
        if ( ! dbobj.containsChar('@') ) {
		    reterr = Jstr("E33301 Error: cannot get ") + dbobj;
        }
		return 0;
	}
}

void JagDBServer::sendSchemaMap( const char *mesg, const JagRequest &req, bool end )
{		
	dn("s340183 enter sendSchemaMap req.session->origserv=%d _restartRecover=%d end=%d", req.session->origserv, (int)_restartRecover, end );

	if ( !req.session->origserv && !_restartRecover ) {	
		dn("s3394400 enter sendSchemaMap() !req.session->origserv && !_restartRecover end=%d", end);
		Jstr schemaInfo;
		_dbConnector->_broadcastCli->getSchemaMapInfo( schemaInfo );
        // dn("s2020299 _broadcastCli->getSchemaMapInfo schemaInfo=[%s]", schemaInfo.s() );

		if ( schemaInfo.size() > 0 ) {
			//sendMessageLength( req, schemaInfo.c_str(), schemaInfo.size(), "SC" );
			//sendMessageLength( req, schemaInfo.c_str(), schemaInfo.size(), "ED" );
			if ( end ) {
				sendMessageLength( req, schemaInfo.c_str(), schemaInfo.size(), JAG_MSG_SCHEMA, JAG_MSG_NEXT_END);
				dn("w333309 sendMessageLength end done");
			} else {
				sendMessageLength( req, schemaInfo.c_str(), schemaInfo.size(), JAG_MSG_SCHEMA, JAG_MSG_NEXT_MORE);
				dn("w333309 sendMessageLength more done");
			}
		} else {
			if ( end ) {
				sendEOM( req, "sndschnull");
			}
		}
	} else {
		if ( end ) {
			sendEOM( req, "sndschzero");
		}
	}

	return;
}

// end: true if requested by client; end: false if server sends it to client
void JagDBServer::sendHostInfo( const char *mesg, const JagRequest &req, bool end )
{	
	if ( !req.session->origserv && !_restartRecover ) {	
		jaguar_mutex_lock ( &g_dbconnectormutex );
		Jstr snodes = _dbConnector->_nodeMgr->_sendAllNodes;
		jaguar_mutex_unlock ( &g_dbconnectormutex );
		//sendMessageLength( req, snodes.c_str(), snodes.size(), "HL" );
		if ( end ) {
			sendMessageLength( req, snodes.c_str(), snodes.size(), JAG_MSG_HOST, JAG_MSG_NEXT_END);
		} else {
			sendMessageLength( req, snodes.c_str(), snodes.size(), JAG_MSG_HOST, JAG_MSG_NEXT_MORE);
		}
	} else {
		if ( end ) {
			sendEOM( req, "sndhostzero");
		}
	}
}

void JagDBServer::chkkey( const char *mesg, const JagRequest &req )
{	
	// "_chkkey|db|table|....key_base64_..."
    JagStrSplit sp(mesg, '|');
	if ( sp.size() < 4 ) {
		sendMessageLength( req, "0", 1, JAG_MSG_OK, JAG_MSG_NEXT_END);
		return;
	}

	Jstr dbname = sp[1];
	Jstr tabname = sp[2];
	Jstr keystr = abaxDecodeBase64(sp[3]);

	JagTable *ptab;
	int  lockrc;
	ptab = _objectLock->readLockTable( JAG_SELECT_OP, dbname, tabname, req.session->replicType, false, lockrc ); 

	if ( ! ptab ) {
		dn("s6277220 readLockTable got NULL lockrc=%d", lockrc );
		sendMessageLength( req, "0", 1, JAG_MSG_OK, JAG_MSG_NEXT_END);
		return;
	}
	
	bool rc = ptab->chkkey( keystr );
	_objectLock->readUnlockTable( JAG_SELECT_OP, dbname, tabname, req.session->replicType, false ); 

	if ( rc )  {
		sendMessageLength( req, "1", 1, JAG_MSG_OK, JAG_MSG_NEXT_END);
	} else {
		sendMessageLength( req, "0", 1, JAG_MSG_OK, JAG_MSG_NEXT_END);
	}
}

void JagDBServer::checkDeltaFiles( const char *mesg, const JagRequest &req )
{
    dn("s12071220 checkDeltaFiles mesg=[%s]", mesg );

	Jstr str;
	if ( _actdelPOhost == req.session->ip && JagFileMgr::fileSize(_actdelPOpath) > 0 ) {
		str = _actdelPOpath + " not empty";
        dn("s3003 str=%s", str.s() );
		sendDataMore( req, str);
	}

	if ( _actdelPRhost == req.session->ip && JagFileMgr::fileSize(_actdelPRpath) > 0 ) {
		str = _actdelPRpath + " not empty";
        dn("s3005 str=%s", str.s() );
		sendDataMore( req, str);
	}

	if ( _actdelPORhost == req.session->ip && JagFileMgr::fileSize(_actdelPORpath) > 0 ) {
		str = _actdelPORpath + " not empty";
        dn("s3007 str=%s", str.s() );
		sendDataMore( req, str);
	}

	if ( _actdelNOhost == req.session->ip && JagFileMgr::fileSize(_actdelNOpath) > 0 ) {
		str = _actdelNOpath + " not empty";
        dn("s3009 str=%s", str.s() );
		sendDataMore( req, str);
	}

	if ( _actdelNRhost == req.session->ip && JagFileMgr::fileSize(_actdelNRpath) > 0 ) {
		str = _actdelNRpath + " not empty";
        dn("s3013 str=%s", str.s() );
		sendDataMore( req, str);
	}

	if ( _actdelNORhost == req.session->ip && JagFileMgr::fileSize(_actdelNORpath) > 0 ) { 
		str = _actdelNORpath + " not empty";
        dn("s3015 str=%s", str.s() );
		sendDataMore( req, str);
	}
}

// crecover, clean recover server(s)
// pmesg: "_serv_crecover"
void JagDBServer::cleanRecovery( const char *mesg, const JagRequest &req )
{
    dn("s3330394 cleanRecovery mesg=[%s]", mesg );

	if ( req.session->servobj->_restartRecover ) return;					
	if ( _faultToleranceCopy <= 1 ) return; // no replicate

	// first, ask other servers to see if current server has delta recover file; if yes, return ( not up-to-date files )
	Jstr connectOpt = Jstr("/TOKEN=") + _servToken;
	JaguarCPPClient reqcli;
	if ( ! reqcli.connect( _dbConnector->_nodeMgr->_selfIP.c_str(), _port, "admin", "anon", "test", 
                           connectOpt.c_str(), JAG_SERV_PARENT, _servToken.c_str() ) ) {
		jd(JAG_LOG_LOW, "s4058 crecover check failure, unable to make connection ...\n" );
		reqcli.close();
		return;
	}

	Jstr bcasthosts = getBroadcastRecoverHosts( _faultToleranceCopy );

    dn("s340031 cleanRecovery broadcastGet _serv_checkdelta ...");
	Jstr resp = _dbConnector->broadcastGet( "_serv_checkdelta", bcasthosts, &reqcli );

    dn("s3490305 broadcastGet _serv_checkdelta got resp=[%s]", resp.s() );

	JagStrSplit checksp( resp, '\n', true );
	if ( checksp.length() > 1 || checksp[0].length() > 0 ) {
		reqcli.close();
        dn("s3000812 return");
		return;
	}
	reqcli.close();

	JagStrSplit sp( _dbConnector->_nodeMgr->_hostClusterNodes, '|' );
	jd(JAG_LOG_LOW, "begin proc request of clean redo from %s ...\n", req.session->ip.c_str() );
	jagsync();

	jagint reqservi;
	req.session->servobj->_internalHostNum->getValue(req.session->ip, reqservi);

	jd(JAG_LOG_LOW, "begin cleanRecovery reqservi=%ld ...\n", reqservi);

	int pos1, pos2, pos3, pos4, rc;
	Jstr filePath, passwd = "anon";
	unsigned int uport = _port;

	if ( _faultToleranceCopy == 2 ) {
		if ( reqservi == 0 ) {
			pos1 = reqservi+1;
			pos2 = req.session->servobj->_numPrimaryServers-1;
		} else if ( reqservi == req.session->servobj->_numPrimaryServers-1 ) {
			pos1 = 0;
			pos2 = reqservi-1;
		} else {
			pos1 = reqservi+1;
			pos2 = reqservi-1;
		}

		jd(JAG_LOG_LOW, "_faultToleranceCopy is 2: pos1=%d pos2=%d\n", pos1, pos2);

		if ( pos1 == _nthServer ) {
			// perpare to copy prev dir to original data dir
			if ( _objectLock->getnumObjects( 1, 1 ) > 0 ) {	
				organizeCompressDir( 1, filePath );
				rc = fileTransmit( req.session->ip, uport, passwd, connectOpt, 0, filePath, 0 );
			}
		} 

		if ( pos2 == _nthServer ) {
			// prepare to copy original data dir to prev dir
			if ( _objectLock->getnumObjects( 1, 0 ) > 0 ) {	
				organizeCompressDir( 0, filePath );
				rc = fileTransmit( req.session->ip, uport, passwd, connectOpt, 1, filePath, 0 );
			}
		}
		// else ignore
	} else if ( _faultToleranceCopy == 3 ) {
		if ( reqservi == 0 ) {
			pos1 = reqservi+1;
			pos2 = req.session->servobj->_numPrimaryServers-1;
		} else if ( reqservi == req.session->servobj->_numPrimaryServers-1 ) {
			pos1 = 0;
			pos2 = reqservi-1;
		} else {
			pos1 = reqservi+1;
			pos2 = reqservi-1;
		}

		if ( pos1 == sp.length()-1 ) {
			pos3 = 0;
		} else {
			pos3 = pos1+1;
		}

		if ( pos2 == 0 ) {
			pos4 = sp.length()-1;
		} else {
			pos4 = pos2-1;
		}

		jd(JAG_LOG_LOW, "_faultToleranceCopy is 3: pos1=%d pos2=%d pos3=%d pos4=%d\n", pos1, pos2, pos3, pos4);

		if ( pos1 == _nthServer ) {
			// perpare to copy prev dir to original data dir
			if ( _objectLock->getnumObjects( 1, 1 ) > 0 ) {	
				organizeCompressDir( 1, filePath );
				rc = fileTransmit( req.session->ip, uport, passwd, connectOpt, 0, filePath, 0 );
			}

			// prepare to copy original data dir to next dir
			if ( _objectLock->getnumObjects( 1, 0 ) > 0 ) {	
				organizeCompressDir( 0, filePath );
				rc = fileTransmit( req.session->ip, uport, passwd, connectOpt, 2, filePath, 0 );
			}
		}

		if ( pos2 == _nthServer ) {
			// perpare to copy next dir to original data dir
			if ( _objectLock->getnumObjects( 1, 2 ) > 0 ) {	
				organizeCompressDir( 2, filePath );
				rc = fileTransmit( req.session->ip, uport, passwd, connectOpt, 0, filePath, 0 );
			}

			// prepare to copy original data dir to prev dir
			if ( _objectLock->getnumObjects( 1, 0 ) > 0 ) {	
				organizeCompressDir( 0, filePath );
				rc = fileTransmit( req.session->ip, uport, passwd, connectOpt, 1, filePath, 0 );
			}
		}

		if ( pos3 == _nthServer ) {
			// perpare to copy prev dir to next dir
			if ( _objectLock->getnumObjects( 1, 1 ) > 0 ) {	
				organizeCompressDir( 1, filePath );
				rc = fileTransmit( req.session->ip, uport, passwd, connectOpt, 2, filePath, 0 );
			}
		}

		if ( pos4 == _nthServer ) {
			// perpare to copy next dir to prev dir
			if ( _objectLock->getnumObjects( 1, 2 ) > 0 ) {	
				organizeCompressDir( 2, filePath );
				rc = fileTransmit( req.session->ip, uport, passwd, connectOpt, 1, filePath, 0 );
			}
		}
	}

	jd(JAG_LOG_LOW, "end cleanRecovery\n");
	jd(JAG_LOG_LOW, "end proc request of clean redo from %s\n", req.session->ip.c_str() );
}

// method to receive tar.gz recovery file
// pmesg: "_serv_beginfxfer|0|123456|sender_tid" or "_serv_addbeginfxfer|0|123456|sender_tid"
void JagDBServer::recoveryFileReceiver( const char *mesg, const JagRequest &req )
{
	/**
	if ( req.session->drecoverConn != 2 ) {
		jd(JAG_LOG_LOW, "in recoveryFileReceiver() req.session->drecoverConn=%d != 2 return\n", 
           req.session->drecoverConn );
		return;
	}
	**/

	jd(JAG_LOG_LOW, "begin proc request of xfer from %s ...\n", req.session->ip.c_str() );
	jd(JAG_LOG_LOW, "begin recoveryFileReceiver ...\n" );
	
	JagStrSplit sp( mesg, '|', true );
	if ( sp.length() < 4 ) return;

	int fpos = jagatoi(sp[1].c_str());
	size_t rlen;
	jagint fsize = jagatoll(sp[2].c_str());
	jagint memsize = 128*1024*1024;
	jagint totlen = 0;
	jagint recvlen = 0;
	Jstr fname = sp[3];
	Jstr ip_fname_tar_gz = req.session->ip + "_" + fname + ".tar.gz";

	Jstr recvpath = req.session->servobj->_cfg->getTEMPDataHOME( fpos ) + "/" + ip_fname_tar_gz;
	int fd = jagopen( recvpath.c_str(), O_CREAT|O_RDWR|O_TRUNC, S_IRWXU);
	jd(JAG_LOG_LOW, "s6207 open recvpath=[%s] for recoveryFile\n", recvpath.c_str() );
	if ( fd < 0 ) {
		jd(JAG_LOG_LOW, "s6208 error open recvpath=[%s]\n", recvpath.c_str() );
		jd(JAG_LOG_LOW, "end recoveryFileReceiver error open\n" );
		//return;
	}


	jd(JAG_LOG_LOW, "expect to recv %ld bytes \n", fsize );
	char *buf =(char*)jagmalloc(memsize);
	
	while( 1 ) {
		if ( totlen >= fsize ) break;
		if ( fsize-totlen < memsize ) {
			recvlen = fsize-totlen;
		} else {
			recvlen = memsize;
		}

		// even if fd < 0; still recv data
		rlen = recvRawData( req.session->sock, buf, recvlen );
		if ( rlen < recvlen ) {
			if ( buf ) free ( buf );
			jagclose( fd );
			jagunlink( recvpath.c_str() );
			jd(JAG_LOG_LOW, "end recoveryFileReceiver error recvrawdata\n" );
			return;
		}

		if ( fd > 0 ) {
			rlen = raysafewrite( fd, buf, recvlen );
			if ( rlen < recvlen ) {
				if ( buf ) free ( buf );
				jagclose( fd );
				jagunlink( recvpath.c_str() );
				jd(JAG_LOG_LOW, "end recoveryFileReceiver error savedata\n" );
				return;
			}
		}

		totlen += rlen;
	}

	if ( fd > 0 ) {
		jagfdatasync( fd );
		jagclose( fd );
		jd(JAG_LOG_LOW, "saved %ld bytes\n", totlen );
	}
	jd(JAG_LOG_LOW, "recved %ld bytes\n", totlen );
	
	// check number of bytes
	struct stat sbuf;
	if ( 0 != stat(recvpath.c_str(), &sbuf) || sbuf.st_size != fsize || totlen != fsize ) {
		// incorrect number of bytes of file, remove file
		jagunlink( recvpath.c_str() );
	} else {
		JAG_BLURT jaguar_mutex_lock ( &g_dlogmutex ); JAG_OVER
		if ( 0 == fpos ) {
			if ( _crecoverFpath.size() < 1 ) {
				_crecoverFpath = ip_fname_tar_gz;
			} else {
				_crecoverFpath += Jstr("|") + ip_fname_tar_gz;
			}
		} else if ( 1 == fpos ) {
			if ( _prevcrecoverFpath.size() < 1 ) {
				_prevcrecoverFpath = ip_fname_tar_gz;
			} else {
				_prevcrecoverFpath += Jstr("|") + ip_fname_tar_gz;
			}
		} else if ( 2 == fpos ) {
			if ( _nextcrecoverFpath.size() < 1 ) {
				_nextcrecoverFpath = ip_fname_tar_gz;
			} else {
				_nextcrecoverFpath += Jstr("|") + ip_fname_tar_gz;
			}
		}
		jaguar_mutex_unlock ( &g_dlogmutex );
		jd(JAG_LOG_LOW, "end recoveryFileReceiver OK\n" );
	}

	if ( buf ) free ( buf );
	jd(JAG_LOG_LOW, "end proc request of xfer from %s\n", req.session->ip.c_str() );
}


// method to receive tar.gz recovery file
// pmesg: "_serv_beginfxfer|10|123456|sender_tid" 
void JagDBServer::walFileReceiver( const char *mesg, const JagRequest &req )
{
}

// client expects: "numservs|numDBs|numTables|selects|inserts|updates|deletes|usersessions"
// pmesg: "_serv_opinfo"
void JagDBServer::sendOpInfo( const char *mesg, const JagRequest &req )
{
	JagStrSplit sp( _dbConnector->_nodeMgr->_hostClusterNodes, '|' );
	int nsrv = sp.length(); 

	int dbs, tabs;
	numDBTables( dbs, tabs );

	Jstr res;
	char buf[1024];
	sprintf( buf, "%d|%d|%d|%lld|%lld|%lld|%lld|%lld", nsrv, dbs, tabs, 
			(jagint)numSelects, (jagint)numInserts, 
			(jagint)numUpdates, (jagint)numDeletes, 
			(jagint)_connections );

	res = buf;
	// printf("s4910 sendOpInfo [%s]\n", res.c_str() );
	//sendMessageLength( req, res.c_str(), res.size(), "OK" );
	sendOKEnd( req, res);
}

// do local data backup
// pmesg: "_serv_copydata|15MIN:OVERWRITE|0"
void JagDBServer::doCopyData( const char *mesg, const JagRequest &req )
{
	JagStrSplit sp( mesg, '|', true );
	if ( sp.length() < 3 ) {
		jd(JAG_LOG_LOW, "s8035 doCopyData error [%s]\n", mesg );
		return;
	}
	Jstr rec = sp[1];
	int show = jagatoi(sp[2].c_str());
	copyData( rec, show );
}

// do local backup method
// pmesg: "_serv_dolocalbackup"
void JagDBServer::doLocalBackup( const char *mesg, const JagRequest &req )
{
	jd(JAG_LOG_LOW, "dolocalbackup ...\n" );
	JagStrSplit sp( mesg, '|', true );
	copyLocalData("last", "OVERWRITE", sp[1], true );
	jd(JAG_LOG_LOW, "dolocalbackup done\n" );
}

// do remote backup method
// pmesg: "_serv_doremotebackup"
void JagDBServer::doRemoteBackup( const char *mesg, const JagRequest &req )
{
	if ( _doingRemoteBackup ) {
		// do not comment out
		d("s1192 _doingRemoteBackup active, doRemoteBackup skip\n");
		return;
	}

	JagStrSplit sp( mesg, '|', true ); // sp[1] is rserv, sp[2] is passwd
	if ( sp[1].length() < 1 || sp[2].length() < 1 ) {
		// do not comment out
		d("s1193 rserv or passwd empty, skip doRemoteBackup\n" );
		return;
	}
	
	jd(JAG_LOG_LOW, "doremotebackup ...\n" );	
	_doingRemoteBackup = 1;

	Jstr passwdfile = _cfg->getConfHOME() + "/tmpsyncpass.txt";
	JagFileMgr::writeTextFile( passwdfile, sp[2] );
	Jstr cmd;
	chmod( passwdfile.c_str(), 0600 );

	Jstr intip = _localInternalIP;
	Jstr jagdatahome = _cfg->getJDBDataHOME( JAG_MAIN ); // /home/jaguar/data
	Jstr backupdir = jaguarHome() + "/tmp/remotebackup";
	JagFileMgr::rmdir( backupdir );
	JagFileMgr::makedirPath( backupdir );
	char buf[2048];
	sprintf( buf, "rsync -r %s/ %s", jagdatahome.c_str(), backupdir.c_str() );
	system( buf );  // first local copy /home/jaguar/data/* to /home/jaguar/tmp/remotebackup

	cmd = "rsync -q --contimeout=10 --password-file=" + passwdfile + " -az " + backupdir + "/ " + sp[1] + "::jaguardata/" + intip;
	// then copy from /home/jaguar/data/remotebackup/* to remotehost::jaguardata/192.183.2.120
	// do not comment out
	d("s1829 [%s]\n", cmd.c_str() );
	Jstr res = psystem( cmd.c_str() );
	jd(JAG_LOG_LOW, "doRemoteBackup done %s\n", res.c_str() );
	_doingRemoteBackup = 0;
	JagFileMgr::rmdir( backupdir, false );
	jd(JAG_LOG_LOW, "doremotebackup done\n" );
}

// do restore remote backup method
// pmesg: "_serv_dorestoreremote"
void JagDBServer::doRestoreRemote( const char *mesg, const JagRequest &req )
{
	if ( _doingRestoreRemote || _doingRemoteBackup ) {
		// do not comment out
		d("s1192 _doingRestoreRemote or _doingRemoteBackup active, doRestoreRemote skip\n");
		return;
	}

	JagStrSplit sp( mesg, '|', true );	// sp[1] is rserv, sp[2] is passwd
	if ( sp[1].length() < 1 ) {
		// do not comment out
		d("s1193 rserv empty, skip doRestoreRemote\n" );
		return;
	}
	
	jd(JAG_LOG_LOW, "dorestoreremote ...\n" );
	_doingRestoreRemote = 1;

	char buf[2048];
    Jstr cmd = jaguarHome() + "/bin/restorefromremote.sh";
	Jstr logf = jaguarHome() + "/log/restorefromremote.log";
	sprintf( buf, "%s %s %s > %s 2>&1", cmd.c_str(), sp[1].c_str(), sp[2].c_str(), logf.c_str() );

	Jstr res = psystem( buf ); 
	jd(JAG_LOG_LOW, "doRestoreRemote %s\n", res.c_str() );
	_doingRestoreRemote = 0;
	jd(JAG_LOG_LOW, "exit. Please restart jaguar after restore completes\n", res.c_str() );
	jd(JAG_LOG_LOW, "dorestoreremote done\n" );
	exit(39);
}

// refresh local server allowlist and blocklist
// pmesg: "_serv_refreshacl|allowlistIPS|blocklistIPS"
void JagDBServer::doRefreshACL( const char *mesg, const JagRequest &req )
{
	JagStrSplit sp( mesg, '|' );
	if ( sp.length() < 3 ) {
		jd(JAG_LOG_LOW, "s8315 doRefreshACL error [%s]\n", mesg );
		return;
	}

	Jstr allowlist = sp[1];  // ip1\nip2\nip3\n
	Jstr blocklist = sp[2];  // ip4\nip5\ip6\n
	pthread_rwlock_wrlock( &_aclrwlock);
	_allowIPList->refresh( allowlist );
	_blockIPList->refresh( blocklist );
	pthread_rwlock_unlock( &_aclrwlock );

}

// dbtabInfo 
// pmesg: "_mon_dbtab" 
void JagDBServer::dbtabInfo( const char *mesg, const JagRequest &req )
{
	// "db1:t1:t2|db2:t1:t2|db3:t1:t3"
	JagTableSchema *tableschema = getTableSchema( req.session->replicType );
	Jstr res;
	Jstr dbs = JagSchema::getDatabases( _cfg, req.session->replicType );
	JagStrSplit sp(dbs, '\n', true );
	for ( int i = 0; i < sp.length(); ++i ) {
		JagVector<AbaxString> *vec = tableschema->getAllTablesOrIndexes( sp[i], "" );
		res += sp[i];
		for ( int j =0; j < vec->size(); ++j ) {
			res += Jstr(":") + (*vec)[j].c_str();
		}
		if ( vec ) delete vec;
		vec = NULL;
		res += Jstr("|");
	}
	//sendMessageLength( req, res.c_str(), res.size(), "OK" );
	sendOKEnd( req, res);
}

// send info 
// pmesg: "_mon_info"
void JagDBServer::sendInfo( const char *mesg, const JagRequest &req )
{
	Jstr res;
	JagVector<Jstr> vec;
	JagBoundFile bf( _perfFile.c_str(), 96 );
	bf.openRead();
	bf.readLines( 96, vec );
	bf.close();
	int len = vec.length();
	for ( int i = 0; i < len; ++i ) {
		res += vec[i]  + "\n";
	}

	if ( res.length() < 1 ) {
		res = "0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0";
	}
	//sendMessageLength( req, res.c_str(), res.size(), "OK" );
	sendOKEnd( req, res);
}

// client expects: "disk:123:322|mem:234:123|cpu:23:12";  in GB  used:free
// pmesg: "_mon_rsinfo"
void JagDBServer::sendResourceInfo( const char *mesg, const JagRequest &req )
{
	int rc = 0;
	Jstr res;
	jagint usedDisk, freeDisk;
	jagint usercpu, syscpu, idle;
	_jagSystem.getCPUStat( usercpu, syscpu, idle );
	jagint totm, freem, used; //GB
	rc = _jagSystem.getMemInfo( totm, freem, used );

	Jstr jaghome= jaguarHome();
	JagFileMgr::getPathUsage( jaghome.c_str(), usedDisk, freeDisk );

	char buf[1024];
	sprintf( buf, "disk:%lld:%lld|mem:%lld:%lld|cpu:%lld:%lld", 
			 usedDisk, freeDisk, totm-freem, freem, usercpu+syscpu, 100-usercpu-syscpu );
	res = buf;
	//sendMessageLength( req, res.c_str(), res.size(), "OK" );
	sendOKEnd( req, res);
}

// client expects: "numservs|numDBs|numTables|selects|inserts|updates|deletes|usersessions"
// pmesg: "_mon_clusteropinfo"
void JagDBServer::sendClusterOpInfo( const char *mesg, const JagRequest &req )
{
	dn("s522224 sendClusterOpInfo _mon_clusteropinfo");
	Jstr res = getClusterOpInfo( req );
	//sendMessageLength( req, res.c_str(), res.size(), "OK" );
	sendOKEnd( req, res);
}

// client expects: "disk:123:322|mem:234:123|cpu:23:12";  in GB  used:free
// pmesg: "_mon_hosts"
void JagDBServer::sendHostsList( const char *mesg, const JagRequest &req )
{
	Jstr res;
	res = _dbConnector->_nodeMgr->_allNodes;
	sendDataEnd( req, res);
}

// client expects: "ip1|ip2|ip3";  in GB  used:free
// pmesg: "_mon_remote_backuphosts"
void JagDBServer::sendRemoteHostsInfo( const char *mesg, const JagRequest &req )
{
	Jstr res = _cfg->getValue("REMOTE_BACKUP_SERVER", "0" );
	sendDataEnd( req, res);
}

// client expects: "%lld|%lld|%lld|%lld|%.2f|%lld", totalDiskGB, usedDiskGB, freeDiskGB, nproc, loadvg, tcp
// pmesg: "_mon_local_stat6"
void JagDBServer::sendLocalStat6( const char *mesg, const JagRequest &req )
{
	jagint totalDiskGB, usedDiskGB, freeDiskGB, nproc, tcp;
	float loadvg;
	_jagSystem.getStat6( totalDiskGB, usedDiskGB, freeDiskGB, nproc, loadvg, tcp );
	char line[256];
	sprintf(line, "%lld|%lld|%lld|%lld|%.2f|%lld", totalDiskGB, usedDiskGB, freeDiskGB, nproc, loadvg, tcp );
	//sendMessageLength( req, line, strlen(line), "OK" );
	sendOKEnd( req, line );
}

// client expects: "%lld|%lld|%lld|%lld|%.2f|%lld", totalDiskGB, usedDiskGB, freeDiskGB, nproc, loadvg, tcp
// loadvg is avg, others are accumulative from all nodes
// pmesg: "_mon_cluster_stat6"
void JagDBServer::sendClusterStat6( const char *mesg, const JagRequest &req )
{
	jagint totalDiskGB=0, usedDiskGB=0, freeDiskGB=0, nproc=0, tcp=0;
	float loadvg;

	_jagSystem.getStat6( totalDiskGB, usedDiskGB, freeDiskGB, nproc, loadvg, tcp );
	char line[256];
	sprintf(line, "%lld|%lld|%lld|%lld|%.2f|%lld", totalDiskGB, usedDiskGB, freeDiskGB, nproc, loadvg, tcp );
	Jstr self = line;

	Jstr resp, bcasthosts;
	resp = _dbConnector->broadcastGet( "_mon_local_stat6", bcasthosts ); 
	// \n separated data from all nodes

	resp += self + "\n";

	JagStrSplit sp( resp, '\n', true );

	totalDiskGB = usedDiskGB = freeDiskGB = nproc = tcp = 0;
	loadvg = 0.0;
	int splen = sp.length();
	for ( int i = 0 ; i < splen; ++i ) {
		JagStrSplit ds( sp[i], '|' );
		totalDiskGB += jagatoll( ds[0].c_str() ); 
		usedDiskGB += jagatoll( ds[1].c_str() ); 
		freeDiskGB += jagatoll( ds[2].c_str() ); 
		nproc += jagatoll( ds[3].c_str() ); 
		loadvg += jagatof( ds[4].c_str() ); 
		tcp += jagatoll( ds[5].c_str() ); 
	}

	loadvg = loadvg/(float)splen;

	sprintf(line, "%lld|%lld|%lld|%lld|%.2f|%lld", totalDiskGB, usedDiskGB, freeDiskGB, nproc, loadvg, tcp );
	//sendMessageLength( req, line, strlen(line), "OK" );
	sendOKEnd( req, line);
}

// 0: not done; 1: done
// pmesg: "_ex_proclocalbackup"
void JagDBServer::processLocalBackup( const char *mesg, const JagRequest &req )
{
	if ( req.session->uid!="admin" || !req.session->exclusiveLogin ) {
		jd(JAG_LOG_LOW, "localbackup rejected. admin exclusive login is required\n" );
		//sendMessage( req, "_END_[T=922|E=Command Failed. admin exclusive login is required]", "ER" );
		sendER( req, "E401220 Command Failed. admin exclusive login is required");
		return;
	}
	
	if ( ! _dbConnector->_nodeMgr->_isHost0OfCluster0 ) {
		jd(JAG_LOG_LOW, "localbackup not processed\n" );
		sendER( req, "localbackup is not setup and not processed");
		//sendMessage( req, "_END_[T=924|E=]", "ED" );
		return;
	}
	
	jd(JAG_LOG_LOW, "localbackup started\n" );
	sendDataMore( req, "localbackup started...");
	
	// copy local data
	Jstr tmstr = JagTime::YYYYMMDDHHMM();
	copyLocalData("last", "OVERWRITE", tmstr, true );

	// bcast to other servers
	Jstr bcastCmd, bcasthosts;
	bcastCmd = Jstr("_serv_dolocalbackup|") + tmstr;
	jd(JAG_LOG_LOW, "broadcast localbackup to all servers ...\n" ); 
	_dbConnector->broadcastSignal( bcastCmd, bcasthosts );
	jd(JAG_LOG_LOW, "localbackup finished\n" );
	/**
	//sendMessage( req, "localbackup finished", "OK" );
	//sendMessage( req, "_END_[T=240|E=]", "ED" );
	**/
	sendOKEnd( req, "localbackup finished");
}

// 0: not done; 1: done
// pmesg: "_ex_procremotebackup"
void JagDBServer::processRemoteBackup( const char *mesg, const JagRequest &req )
{
	// if _END_ leads, then it is final message to client.
	// "_END_[]", "ED"  means end of message, no error
	// "_END_[]", "ER"  means end of message, with error
	// "msg", "OK"  just an message
	
	if ( req.session && ( req.session->uid!="admin" || !req.session->exclusiveLogin ) ) {
		jd(JAG_LOG_LOW, "remotebackup rejected. admin exclusive login is required\n" );
		//if ( req.session ) sendMessage( req, "_END_[T=930|E=Command Failed. admin exclusive login is required]", "ER" );
		if ( req.session ) sendER( req, "E2300 Command Failed. admin exclusive login is required");
		return;
	}
	
	// if i am host0, do remotebackup
	if ( ! _dbConnector->_nodeMgr->_isHost0OfCluster0 ) {
		d("s2930 processRemoteBackup not _isHost0OfCluster0 skip\n");
		jd(JAG_LOG_LOW, "remotebackup not processed\n" );
		//if ( req.session ) sendMessage( req, "remotebackup is not setup and not processed", "ER" );
		if ( req.session ) sendER( req, "E33220 remotebackup is not setup and not processed");
		//if ( req.session ) sendMessage( req, "_END_[T=248|E=]", "ED" );
		return;
	}

	if ( _restartRecover ) {
		d("s2931 in recovery, skip processRemoteBackup\n");
		jd(JAG_LOG_LOW, "remotebackup not processed\n" );
		//if ( req.session ) sendMessage( req, "remotebackup is not setup and not processed", "ER" );
		if ( req.session ) sendER( req, "E22108 remotebackup is not processed");
		//if ( req.session ) sendMessage( req, "_END_[T=250|E=]", "ED" );
		return;
	}

	Jstr rs = _cfg->getValue("REMOTE_BACKUP_SERVER", "" );
	Jstr passwdfile = _cfg->getConfHOME() + "/syncpass.txt";
	chmod( passwdfile.c_str(), 0600 );
	Jstr passwd;
	JagFileMgr::readTextFile( passwdfile, passwd );
	passwd = trimChar( passwd, '\n');
	if ( rs.size() < 1 || passwd.size() < 1 ) {
		// do not comment out
		jd(JAG_LOG_LOW, "REMOTE_BACKUP_SERVER/syncpass.txt empty, skip\n" ); 
		jd(JAG_LOG_LOW, "remotebackup not processed\n" );
		//if ( req.session ) sendMessage( req, "remotebackup is not setup and not processed", "ER" );
		if ( req.session ) sendER( req, "E25210 remotebackup is not processed"); 
		//if ( req.session ) sendMessage( req, "_END_[T=252|E=]", "ED" );
		return;
	}
	
	jd(JAG_LOG_LOW, "remotebackup started\n" );
	//if ( req.session ) sendMessage( req, "remotebackup started...", "OK" );
	if ( req.session ) sendOKMore( req, "remotebackup started...");
	
	JagStrSplit sp (rs, '|');
	Jstr remthost;
	for ( int i = 0; i < sp.length(); ++i ) {
		// multiple remote backup servers
		remthost = sp[i];
    	// self thread starts
       	pthread_t  threadmo;
    	//JagPass *jp = new JagPass();
    	JagPass *jp = newObject<JagPass>();
    	jp->servobj = this;
    	jp->ip = remthost;
    	jp->passwd = passwd;
       	jagpthread_create( &threadmo, NULL, threadRemoteBackup, (void*)jp );
       	pthread_detach( threadmo );
    
    	// bcast to other servers
    	Jstr bcastCmd, bcasthosts;
    	bcastCmd = Jstr("_serv_doremotebackup|") + remthost +"|" + passwd;
    	jd(JAG_LOG_LOW, "broadcast remotebackup to all servers ...\n" ); 
    	_dbConnector->broadcastSignal( bcastCmd, bcasthosts ); 

		jagsleep(10, JAG_SEC);
	}
	jd(JAG_LOG_LOW, "remotebackup finished\n" );
	/**
	//if ( req.session ) sendMessage( req, "remotebackup finished", "OK" );
	//if ( req.session ) sendMessage( req, "_END_[T=258|E=]", "ED" );
	**/
	if ( req.session ) sendOKEnd( req, "remotebackup finished");
}

// 0: not done; 1: done
// pmesg: "_ex_restorefromremote"
void JagDBServer::processRestoreRemote( const char *mesg, const JagRequest &req )
{
	// if _END_ leads, then it is final message to client.
	// "_END_[]", "ED"  means end of message, no error
	// "_END_[]", "ER"  means end of message, with error
	// "msg", "OK"  just an message
	if ( req.session->uid!="admin" || !req.session->exclusiveLogin ) {
		jd(JAG_LOG_LOW, "restorefromremote rejected. admin exclusive login is required\n" );
		sendER( req, "E330201 Command Failed. admin exclusive login is required");
		return;
	}
	
	// if i am host0, do remotebackup
	if ( ! _dbConnector->_nodeMgr->_isHost0OfCluster0 ) {
		d("s2930 processRestoreRemote not _isHost0OfCluster0 skip\n");
		jd(JAG_LOG_LOW, "restorefromremote not processed\n" );
		sendER( req, "E3008 restorefromremote is not setup and not processed");
		//sendMessage( req, "_END_[T=933|E=]", "ED" );
		return;
	}

	if ( _restartRecover ) {
		d("s2931 in recovery, skip processRestoreRemote\n");
		jd(JAG_LOG_LOW, "restorefromremote not processed\n" );
		sendER( req, "E20877 restorefromremote is not setup and not processed");
		//sendMessage( req, "_END_[T=934|E=]", "ED" );
		return;
	}

	JagStrSplit sp( mesg, '|' );
	if ( sp.length() < 3 ) {
		jd(JAG_LOG_LOW, "processRestoreRemote pmsg empty, skip\n" ); 
		jd(JAG_LOG_LOW, "restorefromremote not processed\n" );
		sendER( req, "E22220 restorefromremote is not setup and not processed");
		//sendMessage( req, "_END_[T=935|E=]", "ED" );
		return;
	}

	Jstr remthost  = sp[1];
	Jstr passwd  = sp[2];
	if ( remthost.size() < 1 ) {
		jd(JAG_LOG_LOW, "processRestoreRemote IP empty, skip\n" ); 
		jd(JAG_LOG_LOW, "restorefromremote not processed\n" );
		sendER( req, "E11220 restorefromremote is not setup and not processed");
		//sendMessage( req, "_END_[T=264|E=]", "ED" );
		return;
	}
	
	d("s6370 _restorefromremote mesg=[%s]\n", mesg );
	jd(JAG_LOG_LOW, "restorefromremote started\n" );
	sendDataMore( req, "restorefromremote started... please restart jaguar after completion");

    // bcast to other servers
    Jstr bcastCmd, bcasthosts;
    bcastCmd = Jstr("_serv_dorestoreremote|") + remthost +"|" + passwd;
    jd(JAG_LOG_LOW, "broadcast restoreremote to all servers ...\n" ); 
    _dbConnector->broadcastSignal( bcastCmd, bcasthosts ); 

	jagsleep(3, JAG_SEC);

	Jstr rmsg = Jstr("_serv_dorestoreremote|") + remthost + "|" + passwd;
	doRestoreRemote( rmsg.c_str(), req );
	// doRestoreRemote( remthost, passwd );
	jd(JAG_LOG_LOW, "restorefromremote finished\n" );
	sendOKEnd( req, "restorefromremote finished");
	//sendMessage( req, "restorefromremote finished", "OK" );
	//sendMessage( req, "_END_[T=270|E=]", "ED" );
}

void JagDBServer::addClusterMigrate( const char *mesg, const JagRequest &req )
{
    /***
	if ( req.session->uid !="admin" || !req.session->exclusiveLogin ) {
		jd(JAG_LOG_LOW, "adding cluster rejected. admin exclusive login is required\n" );
		sendER( req, "E400123 Command Failed. admin exclusive login is required");
		return;
	}
    ***/

    dn("s4780188 addClusterMigrate() mesg=[%s]", mesg );

	Jstr oldHosts = _dbConnector->_nodeMgr->_allNodes.s(); // existing hosts "ip1|ip2|ip3"
	this->_migrateOldHosts = oldHosts;
	JagHashMap<AbaxString, AbaxInt> ipmap;

	int elen = strlen("_ex_addclust_migrate") + 2; 
	
	const char *end = strchr( mesg+elen, '|' ); // skip _ex_addcluster_migrate|#
	if ( !end ) end = strchr( mesg+elen, '!' );
	if ( !end ) end = mesg+elen;

	Jstr hstr = mesg+elen-1;
    Jstr absfirst( mesg+elen, end-mesg-elen );

    dn("s870001 hstr=[%s] absfirst=[%s]", hstr.s(), absfirst.s() );

	// split to get original cluster(s) and new added cluster
	JagStrSplit sp( hstr, '!', true );
	JagStrSplit sp2( sp[1], '|', true ); // new nodes

	_objectLock->writeLockSchema( -1 );
	
	// form new cluster.conf string as the form of: #\nip1\nip2\n#\nip3\nip4...
	int clusternum = 1;
	Jstr nhstr, ip, err, clustname;
	JagStrSplit sp3( sp[0], '#', true );

	for ( int i = 0; i < sp3.length(); ++i ) {
        dn("s87005 i=%d sp3[i]=[%s]", i, sp3[i].s() );

		++ clusternum;
		JagStrSplit sp4( sp3[i], '|', true );

		for ( int j = 0; j < sp4.length(); ++j ) {
			ip = JagNet::getIPFromHostName( sp4[j] );
			if ( ip.length() < 2 ) {
				err = Jstr( "E91220 Command Failed. Unable to resolve IP address of " ) +  sp4[j] ;
				jd(JAG_LOG_LOW, "E1300 addcluster error %s \n", err.c_str() );
				sendER( req, err);
				_objectLock->writeUnlockSchema( -1 );
				return;
			}

			if ( ! ipmap.keyExist( ip.c_str() ) ) {
				ipmap.addKeyValue( ip.c_str(), 1 );
				nhstr += ip + "\n";
			}
		}
	}

	// nhstr += "# Do not delete this line\n";
	//clustname = Jstr("# Cluster ") + intToStr(clusternum) + " (New. Do not delete this line)";
	//nhstr += clustname + "\n";
	for ( int i = 0; i < sp2.length(); ++i ) { // new hosts
		// nhstr += sp2[i] + "\n";
		ip = JagNet::getIPFromHostName( sp2[i] );
		if ( ip.length() < 2 ) {
			err = Jstr( "E93637 Command Failed. Unable to resolve IP address of newhost " ) +  sp2[i] ;
			jd(JAG_LOG_LOW, "E1301 addcluster error %s \n", err.c_str() );
			sendER( req, err);
			_objectLock->writeUnlockSchema( -1 );
			return;
		}

		if ( ! ipmap.keyExist( ip.c_str() ) ) {
			ipmap.addKeyValue( ip.c_str(), 1 );
			nhstr += ip + "\n";
		}
	}
	// nhstr has all hosts now ( old + new )
	jd(JAG_LOG_LOW, "addcluster migrate updated set of hosts:\n%s\n", nhstr.c_str() );

	Jstr fpath, cmd, dirpath, tmppath, passwd = "anon";
    Jstr connectOpt = Jstr("/TOKEN=") + _servToken;
	unsigned int uport = _port;
	// first, let host0 of cluster0 send schema info to new server(s)
	bool isDirector = false;

	if ( _dbConnector->_nodeMgr->_selfIP == absfirst ) {
        dn("s87006 i am old main host");
		isDirector = true; // the main old host

		dirpath = _cfg->getJDBDataHOME( JAG_MAIN );
		tmppath = _cfg->getTEMPDataHOME( JAG_MAIN );
		// make schema package -- empty database dirs and full system dir
		cmd = Jstr("cd ") + dirpath + "; tar -zcf " + tmppath + "/a.tar.gz --no-recursion *; tar -zcf ";
		cmd += tmppath + "/b.tar.gz system";
		system(cmd.c_str());
		jd(JAG_LOG_LOW, "s6300 [%s]\n", cmd.c_str() );
		
		// untar the above two tar.gzs, remove them and remake a new tar.gz
		cmd = Jstr("cd ") + tmppath + "; tar -zxf a.tar.gz; tar -zxf b.tar.gz; rm -f a.tar.gz b.tar.gz; tar -zcf c.tar.gz *";
		system(cmd.c_str());
		jd(JAG_LOG_LOW, "s6302 [%s]\n", cmd.c_str() );

		fpath = tmppath + "/c.tar.gz";
		// make connection and transfer package to each server
		for ( int i = 0; i < sp2.length(); ++i ) {
            dn("s5403021 fileTransmit i=%d %s %s ...", i, sp2[i].s(), fpath.s() ); 
			fileTransmit( sp2[i], uport, passwd, connectOpt, 0, fpath, 1 );
            dn("s5403021 fileTransmit i=%d %s %s done", i, sp2[i].s(), fpath.s() ); 
		}

		jagunlink(fpath.c_str());
		// clean up tmp dir
		JagFileMgr::rmdir( tmppath, false );
	} else {
		d("s2767172 i am not host0 of cluster, no tar file and fileTransmit\n");
	}
	
	// for new added servers, wait for package receive, and then format schema
	bool amNewNode = false;

	for ( int i = 0; i < sp2.length(); ++i ) {
		if ( _dbConnector->_nodeMgr->_selfIP == sp2[i] ) {
			amNewNode = true;
			d("s307371 I am a new server %s , wait for schema files to arrive ...\n", sp2[i].s() );
			// is new server, waiting for package to receive
			while ( _addClusterFlag < 1 ) {
				jagsleep(1, JAG_SEC);
				d("s402981 sleep 1 sec ...\n");
			}

			d("s307371 i am a new server %s , _addClusterFlag=%d schema files arrived\n", sp2[i].s(), (int)_addClusterFlag );
			// received new schema package
			// 1. cp tar.gz to pdata and ndata
			// 2. drop old tables, untar packages, cp -rf of data to pdata and ndata and rebuild new table objects
			// 3. init clean other dirs ( /tmp etc. )
			JagRequest req;
			JagSession session;
			session.servobj = this;
            session.serverIP = _localInternalIP;
			req.session = &session;

			// for data dir
			dirpath = _cfg->getJDBDataHOME( JAG_MAIN );
			session.replicType = 0;

			dropAllTablesAndIndex( req, _tableschema );

			JagFileMgr::rmdir( dirpath, false );

            /**
			fpath = _cfg->getTEMPDataHOME( JAG_MAIN ) + "/" + _crecoverFpath;
			cmd = Jstr("tar -zxf ") + fpath + " --directory=" + dirpath;
			system( cmd.c_str() );
			jd(JAG_LOG_LOW, "s63204 [%s]\n", cmd.c_str() );
			jagunlink(fpath.c_str());
			jd(JAG_LOG_LOW, "delete %s\n", fpath.c_str() );
            **/
		    jd(JAG_LOG_LOW, "s34352 applyMultiTars [%s]\n", _crecoverFpath.s() );
            applyMultiTars( _cfg->getTEMPDataHOME( JAG_MAIN ), _crecoverFpath, dirpath );

			
			// for pdata dir
			if ( _faultToleranceCopy >= 2 ) {
				dirpath = _cfg->getJDBDataHOME( JAG_PREV );
				session.replicType = 1;
				dropAllTablesAndIndex( req, _prevtableschema );
				JagFileMgr::rmdir( dirpath, false );
				cmd = Jstr("/bin/cp -rf ") + _cfg->getJDBDataHOME( JAG_MAIN ) + "/* " + dirpath;
				system( cmd.c_str() );
				jd(JAG_LOG_LOW, "s6307 [%s]\n", cmd.c_str() );
			}

			// for ndata dir
			if ( _faultToleranceCopy >= 3 ) {
				dirpath = _cfg->getJDBDataHOME( JAG_NEXT );
				session.replicType = 2;
				dropAllTablesAndIndex( req, _nexttableschema );
				JagFileMgr::rmdir( dirpath, false );
				cmd = Jstr("/bin/cp -rf ") + _cfg->getJDBDataHOME( JAG_MAIN ) + "/* " + dirpath;
				system( cmd.c_str() );
				jd(JAG_LOG_LOW, "s6308 [%s]\n", cmd.c_str() );
			}

			_objectLock->writeUnlockSchema( -1 );
			_objectLock->writeLockSchema( -1 );
			_crecoverFpath = "";

			// 2.
            dn("s02029016 makeNeededDirectories ...");
			makeNeededDirectories();

			_addClusterFlag = 0;
			break;
		} else {
		}
	}

	// then, for all servers, refresh cluster.conf and remake connections
	jd(JAG_LOG_LOW, "s101281 existing allnodes: [%s]\n", _dbConnector->_nodeMgr->_allNodes.s() );

	if ( ! isDirector || amNewNode ) {
		this->_isDirectorNode = false;
		this->_migrateOldHosts = "";
		_dbConnector->_nodeMgr->refreshClusterFile( nhstr );

		struct timeval now;
		gettimeofday( &now, NULL );
		g_lastHostTime = now.tv_sec * (jagint)1000000 + now.tv_usec;
		_objectLock->writeUnlockSchema( -1 );

		jd(JAG_LOG_LOW, "new node or follower, existing allnodes: [%s]\n", _dbConnector->_nodeMgr->_allNodes.s() );
		jd(JAG_LOG_LOW, "_sendAllNodes: [%s]\n", _dbConnector->_nodeMgr->_sendAllNodes.s() );

		jaguar_mutex_lock ( &g_dbconnectormutex );
		if ( _dbConnector ) {
			delete _dbConnector;
			jd(JAG_LOG_LOW, "done delete _dbConnector, new JagDBConnector...\n" );
			_dbConnector = newObject<JagDBConnector>( );
			jd(JAG_LOG_LOW, "done new JagDBConnector\n" );
		}
		jaguar_mutex_unlock ( &g_dbconnectormutex );

		jd(JAG_LOG_LOW, "crecoverRefreshSchema(0)... \n" );
		if ( amNewNode ) {
			crecoverRefreshSchema( JAG_MAKE_OBJECTS_CONNECTIONS, true );
		} else {
			crecoverRefreshSchema( JAG_MAKE_CONNECTIONS_ONLY, true );
		}
		jd(JAG_LOG_LOW, "crecoverRefreshSchema(0) done \n" );


		//sendMessage( req, "_END_[T=270|E=]", "ED" );
		sendEOM(req, "tdonenewnode270");
		jd(JAG_LOG_LOW, "done new node or old non-main node, allnodes: [%s]\n", _dbConnector->_nodeMgr->_allNodes.s() );
		jd(JAG_LOG_LOW, "_sendAllNodes: [%s]\n", _dbConnector->_nodeMgr->_sendAllNodes.s() );

		return;

	}

	_objectLock->writeUnlockSchema( -1 );
	this->_isDirectorNode = true;


	struct timeval now;
	gettimeofday( &now, NULL );
	g_lastHostTime = now.tv_sec * (jagint)1000000 + now.tv_usec;

	jd(JAG_LOG_LOW, "s1927 existing allnodes: [%s]\n", _dbConnector->_nodeMgr->_allNodes.s() );

	_dbConnector->_nodeMgr->refreshClusterFile( nhstr );

	jaguar_mutex_lock ( &g_dbconnectormutex );
	if ( _dbConnector ) {
		delete _dbConnector;
		_dbConnector = newObject<JagDBConnector>( );
	}
	jaguar_mutex_unlock ( &g_dbconnectormutex );

	jd(JAG_LOG_LOW, "s13 new allnodes: [%s]\n", _dbConnector->_nodeMgr->_allNodes.s() );
	jd(JAG_LOG_LOW, "s132 _sendAllNodes: [%s]\n", _dbConnector->_nodeMgr->_sendAllNodes.s() );

	crecoverRefreshSchema( JAG_MAKE_CONNECTIONS_ONLY, true );
	jd(JAG_LOG_LOW, "s23 new allnodes: [%s]\n", _dbConnector->_nodeMgr->_allNodes.s() );

	sendEOM( req, "addclsmig");

	jd(JAG_LOG_LOW, "s14 new allnodes: [%s]\n", _dbConnector->_nodeMgr->_allNodes.s() );
	jd(JAG_LOG_LOW, "s30087 addclustermigrate() begin is done\n" );
}


// after  preparing hosts, files, etc. now do broadcast to other servers
// method to add new servers in a cluster, do full data migration
// pmesg: "_ex_addclust_migrcontinue"
void JagDBServer::addClusterMigrateContinue( const char *mesg, const JagRequest &req )
{
    /***
	if ( req.session->uid!="admin" || !req.session->exclusiveLogin ) {
		jd(JAG_LOG_LOW, "adding cluster rejected. admin exclusive login is required\n" );
		sendER( req, "E1390029 Command Failed. admin exclusive login is required");
		return;
	}
    **/
    dn("s8701004 addClusterMigrateContinue ...");

	Jstr oldHosts = this->_migrateOldHosts;

	if ( ! this->_isDirectorNode ) {
		sendEOM( req, "addclsmigcont");
        dn("s029289 i am not _isDirectorNode return");
		return;

	}

    dn("s8701004 addClusterMigrateContinue go on ...");

	JagVector<AbaxString> *vec = _tableschema->getAllTablesOrIndexesLabel( JAG_TABLE_OR_CHAIN_TYPE, "", "" );
	Jstr sql;
	int bad = 0;
	bool erc;
	Jstr dbtab, db, tab;

	for ( int j=0; j < vec->length(); ++j ) {
		dbtab = (*vec)[j].s();  // "db.tab123"
		sql = Jstr("select * from ") + dbtab + " export;";
		d("s2220833 dbtab=[%s] sql=[%s] broadcastSignal ...\n", dbtab.s(),  sql.s() );
		erc = _dbConnector->broadcastSignal( sql, oldHosts, NULL, true ); // all hosts including self
		d("s2220834 dbtab=[%s] sql=[%s] broadcastSignal done erc=%d\n", dbtab.s(),  sql.s(), erc );
		if ( erc ) {
			jd(JAG_LOG_LOW, "requested exporting %s\n", dbtab.s() );
		} else {
			jd(JAG_LOG_LOW, "requesting exporting %s has error\n", dbtab.s() );
			bad ++;
			break;
		}
	}

	Jstr resp;
	if ( bad > 0 ) {
		for ( int j=0; j < vec->length(); ++j ) {
			dbtab = (*vec)[j].s();
			JagStrSplit sp( dbtab, '.' );
			db = sp[0];
			tab = sp[1];
			sql= Jstr("_ex_importtable|") + db + "|" + tab + "|YES";
			_dbConnector->broadcastSignal( sql, oldHosts, NULL, true ); 
			jd(JAG_LOG_LOW, "s387 broadcastSignal %s to hosts=[%s]\n", sql.s(), oldHosts.s() );
		}
	} else {
		for ( int j=0; j < vec->length(); ++j ) {
			dbtab = (*vec)[j].s();
			JagStrSplit sp( dbtab, '.' );
			db = sp[0];
			tab = sp[1];

			for ( int r = 0; r < _faultToleranceCopy; ++r ) {
				sql= Jstr("_ex_truncatetable|") + intToStr(r) + "|" + db + "|" + tab;
				_dbConnector->broadcastSignal( sql, oldHosts, NULL, true ); 
			}

			jd(JAG_LOG_LOW, "s387 broadcastSignal %s to hosts=[%s]\n", sql.s(), oldHosts.s() );
		}
	}

	if ( bad == 0 ) {
		for ( int j=0; j < vec->length(); ++j ) {
			dbtab = (*vec)[j].s();
			JagStrSplit sp( dbtab, '.' );
			db = sp[0];
			tab = sp[1];
			sql= Jstr("_ex_importtable|") + db + "|" + tab + "|NO";
			_dbConnector->broadcastSignal( sql, oldHosts, NULL, true ); 
			jd(JAG_LOG_LOW, "s387 broadcastSignal %s to hosts=[%s]\n", sql.s(), oldHosts.s() );
		}

		//sendMessageLength( req, resp.s(), resp.size(), "OK");  
	}

	delete vec;
	sendEOM( req, "addclustermigratecontnue");

	jd(JAG_LOG_LOW, "s14 new allnodes: [%s]\n", _dbConnector->_nodeMgr->_allNodes.s() );
	jd(JAG_LOG_LOW, "s30087 addclustermigratecontnue() broadcast is done bad=%d\n", bad );
}


// method to add new cluster
// pmesg: "_ex_addcluster|#ip1|ip2#ip3|ip4!ip5|ip6"
void JagDBServer::addCluster( const char *mesg, const JagRequest &req )
{
    /***
	if ( req.session->uid!="admin" || !req.session->exclusiveLogin ) {
		jd(JAG_LOG_LOW, "adding cluster rejected. admin exclusive login is required\n" );
		sendER( req, "E130023 Command Failed. admin exclusive login is required");
		return;
	}
    ***/
    dn("s2009718 addCluster()  mesg=[%s]", mesg );
    // pmesg=[_ex_addcluster|#192.168.1.207#192.168.1.203!192.168.2.97]
    // pmesg=[_ex_addcluster|#192.168.1.207#192.168.1.203#192.168.2.97!192.168.1.11|192.18.1.16]
    // # separates old clusters, ! introdueces a new cluster ( | separated new nodes)
    // addCluster mesg=[_ex_addcluster|#192.168.1.17|192.168.1.13!192.168.1.51|192.168.1.36]

	JagHashMap<AbaxString, AbaxInt> ipmap;

	int elen = strlen("_ex_addcluster") + 2; 

	Jstr hstr = mesg+elen-1;  // #192.18.1.17#192.18.1.13!192.168.2.9

	JagStrSplit sp( hstr, '!', true );

	JagStrSplit sp3( sp[0], '#', true ); // old clusters
	JagStrSplit sp2( sp[1], '|', true ); // nodes in new cluster

	JagStrSplit sph( sp3[0], '|', true ); // first node in first cluster
    Jstr absfirst = sph[0];

    dn("s8710003 hstr=[%s]  absfirst=[%s]", hstr.s(), absfirst.s() );

	_objectLock->writeLockSchema( -1 );
	
	int clusternum = 1;
	Jstr nhstr, ip, err, clustname;

    nhstr =  Jstr("######## !!!!!!!! PLEASE DO NOT MANUALLY EDIT THIS FILE !!!!!!!! ########\n");
    nhstr += Jstr("######## !!!!!!!! IF IT IS MANUALLY EDITED, THE SYSTEM WILL STOP WORKING !!!!!!!! ########\n");

	for ( int i = 0; i < sp3.length(); ++i ) {
        dn("s32018 sp3[i=%d]=[%s]", i, sp3[i].s() );

		clustname = Jstr("@ Cluster ") + intToStr(clusternum);
		nhstr += clustname + "\n";
		++ clusternum;
		JagStrSplit sp4( sp3[i], '|', true );

		for ( int j = 0; j < sp4.length(); ++j ) {
			ip = JagNet::getIPFromHostName( sp4[j] );
			if ( ip.length() < 2 ) {
				err = Jstr( "E13003 Command Failed. Unable to resolve IP address of " ) +  sp4[j] ;
				jd(JAG_LOG_LOW, "E13003 addcluster error %s \n", err.c_str() );
				sendER( req, err);
				_objectLock->writeUnlockSchema( -1 );
				return;
			}

			if ( ! ipmap.keyExist( ip.c_str() ) ) {
				ipmap.addKeyValue( ip.c_str(), 1 );
				nhstr += ip + "\n";
			}
		}
	}

	clustname = Jstr("@ Cluster ") + intToStr(clusternum);
	nhstr += clustname + "\n";

	for ( int i = 0; i < sp2.length(); ++i ) {
		ip = JagNet::getIPFromHostName( sp2[i] );
		if ( ip.length() < 2 ) {
			err = Jstr( "E132207 E=Command Failed. Unable to resolve IP address of newhost " ) +  sp2[i] ;
			jd(JAG_LOG_LOW, "E132207 addcluster error %s \n", err.c_str() );
			sendER( req, err);
			_objectLock->writeUnlockSchema( -1 );
			return;
		}

		if ( ! ipmap.keyExist( ip.c_str() ) ) {
			ipmap.addKeyValue( ip.c_str(), 1 );
			nhstr += ip + "\n";
		}
	}
	jd(JAG_LOG_LOW, "addcluster newhosts:\n%s\n", nhstr.c_str() );

	Jstr fpath, cmd, dirpath, tmppath, passwd = "anon";
    Jstr connectOpt = Jstr("/TOKEN=") + _servToken;
	unsigned int uport = _port;

	// first, let host0 of cluster0 send schema info to new server(s)
    dn("s87105005 selfIP=[%s]  =?=  absfirst=[%s]", _dbConnector->_nodeMgr->_selfIP.s(),  absfirst.s() );

	if ( _dbConnector->_nodeMgr->_selfIP == absfirst ) {
        dn("s7101827 _selfIP == absfirst send schema files ");
		dirpath = _cfg->getJDBDataHOME( JAG_MAIN );
		tmppath = _cfg->getTEMPDataHOME( JAG_MAIN );
		// make schema package -- empty database dirs and full system dir
		cmd = Jstr("cd ") + dirpath + "; tar -zcf " + tmppath + "/a.tar.gz --no-recursion *; tar -zcf ";
		cmd += tmppath + "/b.tar.gz system";
		system(cmd.c_str());
		jd(JAG_LOG_LOW, "s6300 [%s]\n", cmd.c_str() );
		
		// untar the above two tar.gzs, remove them and remake a new tar.gz
		cmd = Jstr("cd ") + tmppath + "; tar -zxf a.tar.gz; tar -zxf b.tar.gz; rm -f a.tar.gz b.tar.gz; tar -zcf c.tar.gz *";
		system(cmd.c_str());
		jd(JAG_LOG_LOW, "s6302 [%s]\n", cmd.c_str() );
		fpath = tmppath + "/c.tar.gz";

		// make connection and transfer package to each server
		for ( int i = 0; i < sp2.length(); ++i ) {
            dn("s7800231 i=%d fileTransmit fpath=[%s] ...", i, fpath.s() );
			fileTransmit( sp2[i], uport, passwd, connectOpt, 0, fpath, 1 );
            dn("s7800231 i=%d fileTransmit fpath=[%s] done", i, fpath.s() );
		}

		jagunlink(fpath.c_str());
		// clean up tmp dir
		JagFileMgr::rmdir( tmppath, false );
	} else {
		d("s276717 i am not host0 of cluster, no tar file and fileTransmit\n");
	}
	
	// for new added servers, wait for package receive, and then format schema
	//bool amNewNode = false;
	for ( int i = 0; i < sp2.length(); ++i ) {
        dn("s600018 _selfIP=[%s]   sp2[i=%d]=[%s]", _dbConnector->_nodeMgr->_selfIP.s(), sp2[i].s() );
        dn("s20018287 _addClusterFlag=%d", (int)_addClusterFlag );

		if ( _dbConnector->_nodeMgr->_selfIP == sp2[i] ) {
			while ( _addClusterFlag < 1 ) {
				jagsleep(100, JAG_MSEC);
				d("s402981 sleep 100 msec ...\n");
			}

			// received new schema package
			// 1. cp tar.gz to pdata and ndata
			// 1. drop old tables, untar packages, cp -rf of data to pdata and ndata and rebuild new table objects
			// 3. init clean other dirs ( /tmp etc. )
			// 1.
			JagRequest req;
			JagSession session;
			session.servobj = this;
            session.serverIP = _localInternalIP;
			req.session = &session;

			// for data dir
			dirpath = _cfg->getJDBDataHOME( JAG_MAIN );
			session.replicType = 0;
			dropAllTablesAndIndex( req, _tableschema );
			JagFileMgr::rmdir( dirpath, false );

		    jd(JAG_LOG_LOW, "s34651 applyMultiTars [%s]\n", _crecoverFpath.s() );
            applyMultiTars( _cfg->getTEMPDataHOME(JAG_MAIN), _crecoverFpath, dirpath);
			
			// for pdata dir
			if ( _faultToleranceCopy >= 2 ) {
				dirpath = _cfg->getJDBDataHOME( JAG_PREV );
				session.replicType = 1;
				dropAllTablesAndIndex( req, _prevtableschema );
				JagFileMgr::rmdir( dirpath, false );
				cmd = Jstr("/bin/cp -rf ") + _cfg->getJDBDataHOME( JAG_MAIN ) + "/* " + dirpath;
				system( cmd.c_str() );
				jd(JAG_LOG_LOW, "s6307 [%s]\n", cmd.c_str() );
			}

			// for ndata dir
			if ( _faultToleranceCopy >= 3 ) {
				dirpath = _cfg->getJDBDataHOME( JAG_NEXT );
				session.replicType = 2;
				dropAllTablesAndIndex( req, _nexttableschema );
				JagFileMgr::rmdir( dirpath, false );
				cmd = Jstr("/bin/cp -rf ") + _cfg->getJDBDataHOME( JAG_MAIN ) + "/* " + dirpath;
				system( cmd.c_str() );
				jd(JAG_LOG_LOW, "s6308 [%s]\n", cmd.c_str() );
			}

            dn("s7750023 copied schema files");
			_objectLock->writeUnlockSchema( -1 );
			crecoverRefreshSchema( JAG_MAKE_OBJECTS_ONLY );
			_objectLock->writeLockSchema( -1 );
			_crecoverFpath = "";

			// 2.
            dn("s860023 makeNeededDirectories ...");
			makeNeededDirectories();

			_addClusterFlag = 0;
			break;
		}
	}

	// then, for all servers, refresh cluster.conf and remake connections
    dn("s650032 for all servers, refresh cluster.conf and remake connections ...");
	_dbConnector->_nodeMgr->refreshClusterFile( nhstr );
	crecoverRefreshSchema( JAG_MAKE_CONNECTIONS_ONLY );
	_localInternalIP = _dbConnector->_nodeMgr->_selfIP;

    dn("s640023 sendEOM to req.client addclusterdone");
	sendEOM( req, "addclusterdone");

	struct timeval now;
	gettimeofday( &now, NULL );
	g_lastHostTime = now.tv_sec * (jagint)1000000 + now.tv_usec;
	_objectLock->writeUnlockSchema( -1 );

	jd(JAG_LOG_LOW, "s30087 addcluster() is done\n" );
}



// method to cleanup saved sql files for import
// pmesg: "_ex_addclustr_mig_complete|#ip1|ip2#ip3|ip4!ip5|ip6"
void JagDBServer::addClusterMigrateComplete( const char *mesg, const JagRequest &req )
{
    dn("s8701003 addClusterMigrateComplete() ...");
    /***
	if ( req.session->uid!="admin" || !req.session->exclusiveLogin ) {
		jd(JAG_LOG_LOW, "adding cluster rejected. admin exclusive login is required\n" );
		sendER( req, "E130028 Command Failed. admin exclusive login is required");
		return;
	}
    ***/

	Jstr oldHosts = this->_migrateOldHosts;
	if ( oldHosts.size() < 1 ) {
		jd(JAG_LOG_LOW, "I am a new host or non-main host. Do not do broadcast\n" );
		sendEOM( req, "newh_non_main");
		return;
	}

	JagVector<AbaxString> *vec = _tableschema->getAllTablesOrIndexesLabel( JAG_TABLE_OR_CHAIN_TYPE, "", "" );
	Jstr sql;
	int bad = 0;
	Jstr dbtab, db, tab;

	for ( int j=0; j < vec->length(); ++j ) {
		dbtab = (*vec)[j].s();
		JagStrSplit sp( dbtab, '.' );
		db = sp[0];
		tab = sp[1];
		sql= Jstr("_ex_importtable|") + db + "|" + tab + "|YES";
		_dbConnector->broadcastSignal( sql, oldHosts, NULL, true ); 
		jd(JAG_LOG_LOW, "s387 broadcastSignal [%s] to hosts=[%s]\n", sql.s(), oldHosts.s() );
	}

	if ( vec ) delete vec;
	sendEOM( req, "addClusterMigrateComplete");

	jd(JAG_LOG_LOW, "s14 new allnodes: [%s]\n", _dbConnector->_nodeMgr->_allNodes.s() );
	jd(JAG_LOG_LOW, "s30087 addclustermigratecomplete() is done bad=%d\n", bad );

}

// method to unpack schema info after recv tar packages
// pmesg: "_serv_unpackschinfo"
void JagDBServer::unpackSchemaInfo( const char *mesg, const JagRequest &req )
{
	if ( _addClusterFlag < 1 ) {
		return;
	}
	_objectLock->writeLockSchema( -1 );
	// received new schema package
	// 1. cp tar.gz to pdata and ndata
	// 1. drop old tables, untar packages, cp -rf of data to pdata and ndata and rebuild new table objects
	// 3. init clean other dirs ( /tmp etc. )
	// 1.
	JagRequest req2;
	JagSession session;
	session.servobj = this;
    session.serverIP = _localInternalIP;
	req2.session = &session;

	// for data dir
	Jstr dirpath = _cfg->getJDBDataHOME( JAG_MAIN );
	session.replicType = 0;
	dropAllTablesAndIndex( req2, _tableschema );
	JagFileMgr::rmdir( dirpath, false );

	jd(JAG_LOG_LOW, "s62800 applyMultiTars [%s]\n", _crecoverFpath.s() );
    applyMultiTars( _cfg->getTEMPDataHOME( JAG_MAIN ), _crecoverFpath, dirpath);
	
	// for pdata dir
	if ( _faultToleranceCopy >= 2 ) {
		dirpath = _cfg->getJDBDataHOME( JAG_PREV );
		session.replicType = 1;
		dropAllTablesAndIndex( req2, _prevtableschema );
		JagFileMgr::rmdir( dirpath, false );
		Jstr cmd = Jstr("/bin/cp -rf ") + _cfg->getJDBDataHOME( JAG_MAIN ) + "/* " + dirpath;
		system( cmd.c_str() );
		jd(JAG_LOG_LOW, "s6307 [%s]\n", cmd.c_str() );
	}

	// for ndata dir
	if ( _faultToleranceCopy >= 3 ) {
		dirpath = _cfg->getJDBDataHOME( JAG_NEXT );
		session.replicType = 2;
		dropAllTablesAndIndex( req2, _nexttableschema );
		JagFileMgr::rmdir( dirpath, false );
		Jstr cmd = Jstr("/bin/cp -rf ") + _cfg->getJDBDataHOME( JAG_MAIN ) + "/* " + dirpath;
		system( cmd.c_str() );
		jd(JAG_LOG_LOW, "s6308 [%s]\n", cmd.c_str() );
	}
	_objectLock->writeUnlockSchema( -1 );
	crecoverRefreshSchema( JAG_MAKE_OBJECTS_ONLY );
	_objectLock->writeLockSchema( -1 );
	_crecoverFpath = "";

	// 2.
    dn("s280035 makeNeededDirectories ...");
	makeNeededDirectories();

	crecoverRefreshSchema( JAG_MAKE_CONNECTIONS_ONLY );

	_addClusterFlag = 0;
	_newdcTrasmittingFin = 1;
	_objectLock->writeUnlockSchema( -1 );
}

// method to shut down servers
// pmesg: "_exe_shutdown"
void JagDBServer::shutDown( const char *mesg, const JagRequest &req )
{
	if ( req.session && (req.session->uid!="admin" || !req.session->exclusiveLogin ) ) {
		jd(JAG_LOG_LOW, "shut down rejected. admin exclusive login is required\n" );
		sendER( req, "E9300 Command Failed. admin exclusive login is required");
		return;
	}
	
	if ( _shutDownInProgress ) {
		jd(JAG_LOG_LOW, "Shutdown is already in progress. return\n");
		return;
	}
	
	Jstr stopPath = jaguarHome() + "/log/shutdown.cmd";
	JagFileMgr::writeTextFile(stopPath, "WIP");
	_shutDownInProgress = 1;
	
	jd(JAG_LOG_LOW, "Trying to get lock on schema ...\n");
	_objectLock->writeLockSchema( -1 );
	jd(JAG_LOG_LOW, "Received the lock on schema. Wait for other tasks to finish:\n");

	int cnt = 0;
    int running;
	while ( _taskMap->size() > 0 ) {
		jagsleep(1, JAG_SEC);
		Jstr tasks = getTasks( req, running );
		jd(JAG_LOG_LOW, "Shutdown waits %d other tasks to finish ...\n",  _taskMap->size());
		jd(JAG_LOG_LOW, "Current running tasks:\n%s\n", tasks.s());
		++cnt;
		if ( running < 1 || cnt > 60 ) {
			break;
		}
	}
	jd(JAG_LOG_LOW, "Other tasks have finished.\n");

	JAG_BLURT jaguar_mutex_lock ( &g_dlogmutex ); JAG_OVER
	
	if ( _delPrevOriCommandFile ) jagfsync( fileno( _delPrevOriCommandFile ) );
	if ( _delPrevRepCommandFile ) jagfsync( fileno( _delPrevRepCommandFile ) );
	if ( _delPrevOriRepCommandFile ) jagfsync( fileno( _delPrevOriRepCommandFile ) );
	if ( _delNextOriCommandFile ) jagfsync( fileno( _delNextOriCommandFile ) );
	if ( _delNextRepCommandFile ) jagfsync( fileno( _delNextRepCommandFile ) );
	if ( _delNextOriRepCommandFile ) jagfsync( fileno( _delNextOriRepCommandFile ) );
	if ( _recoveryRegCommandFile ) jagfsync( fileno( _recoveryRegCommandFile ) );
	if ( _recoverySpCommandFile ) jagfsync( fileno( _recoverySpCommandFile ) );

	jaguar_mutex_unlock ( &g_dlogmutex );

	jd(JAG_LOG_LOW, "Shutdown: Flushing block index to disk ...\n");
	flushAllBlockIndexToDisk();
	jd(JAG_LOG_LOW, "Shutdown: Completed\n");

	JagFileMgr::writeTextFile(stopPath, "DONE");
	jagsync();
	_objectLock->writeUnlockSchema( -1 );
	exit(0);
}

#ifdef _WINDOWS64_
void JagDBServer::printResources() 
{
	jagint totm, freem, used; //GB
	JagSystem::getMemInfo( totm, freem, used );
	int maxopen = _getmaxstdio();
	_setmaxstdio(2048);
	int maxopennew = _getmaxstdio();
    jd(JAG_LOG_LOW, "totrams=%ld freerams=%ld usedram=%ld maxopenfiles=%d ==> %d\n",
				totm, freem, used, maxopen, maxopennew );
}
#else
#include <sys/resource.h>
void JagDBServer::printResources()
{
    struct rlimit rlim;
    getrlimit(RLIMIT_AS,&rlim);
    jd(JAG_LOG_LOW, "s1107 limit virtual memory soft=%ld hard=%ld\n", rlim.rlim_cur, rlim.rlim_max);
    getrlimit(RLIMIT_CORE,&rlim);
    jd(JAG_LOG_LOW, "s1108 limit core file soft=%ld hard=%ld\n", rlim.rlim_cur, rlim.rlim_max);
    getrlimit(RLIMIT_CPU,&rlim);
    jd(JAG_LOG_LOW, "s1109 limit cpu time soft=%ld hard=%ld\n", rlim.rlim_cur, rlim.rlim_max);
    getrlimit(RLIMIT_DATA,&rlim);
    jd(JAG_LOG_LOW, "s1110 limit data segment soft=%ld hard=%ld\n", rlim.rlim_cur, rlim.rlim_max);
    getrlimit(RLIMIT_FSIZE,&rlim);
    jd(JAG_LOG_LOW, "s1111 limit size of files soft=%ld hard=%ld\n", rlim.rlim_cur, rlim.rlim_max);
    getrlimit(RLIMIT_NOFILE,&rlim);
    jd(JAG_LOG_LOW, "s1112 limit number of fd soft=%ld hard=%ld\n", rlim.rlim_cur, rlim.rlim_max);
    getrlimit(RLIMIT_NPROC,&rlim);
    jd(JAG_LOG_LOW, "s1113 limit number of threads soft=%ld hard=%ld\n", rlim.rlim_cur, rlim.rlim_max);
    getrlimit(RLIMIT_RSS,&rlim);
    jd(JAG_LOG_LOW, "s1114 limit resident memory soft=%ld hard=%ld\n", rlim.rlim_cur, rlim.rlim_max);
    getrlimit(RLIMIT_STACK,&rlim);
    jd(JAG_LOG_LOW, "s1115 limit stack soft=%ld hard=%ld\n", rlim.rlim_cur, rlim.rlim_max);
}
#endif

// return table schema according to replicType
// return NULL for error
JagTableSchema *JagDBServer::getTableSchema( int replicType ) const
{
	JagTableSchema *tableschema = NULL;

	if ( replicType == 0 ) {
		tableschema = this->_tableschema;
	} else if ( replicType == 1 ) {
		tableschema = this->_prevtableschema;
	} else if ( replicType == 2 ) {
		tableschema = this->_nexttableschema;
	}

	return tableschema;
}

void JagDBServer::getTableIndexSchema( int replicType, JagTableSchema *& tableschema, JagIndexSchema *&indexschema )
{
	if ( replicType == 0 ) {
		tableschema = this->_tableschema;
		indexschema = this->_indexschema;
	} else if ( replicType == 1 ) {
		tableschema = this->_prevtableschema;
		indexschema = this->_previndexschema;
	} else if ( replicType == 2 ) {
		tableschema = this->_nexttableschema;
		indexschema = this->_nextindexschema;
	} else {
		tableschema = NULL;
		indexschema = NULL;
	}
}

// object method
void JagDBServer::checkAndCreateThreadGroups()
{
	dn("s5328 activeClients=%ld activeThreads=%ld\n", (jagint)_activeClients, (jagint)_activeThreadGroups );
	jagint percent = 70;
	if ( jagint(_activeClients) * 100 >= percent * (jagint)_activeThreadGroups ) {
		jd(JAG_LOG_LOW, "s3380 create new threads activeClients=%ld activeGrps=%ld makeThreadGroups(%l) ...\n",
				(jagint) _activeClients, (jagint)_activeThreadGroups, _threadGroupSeq );
		makeThreadGroups( _threadGroups, _threadGroupSeq++, 0 );
	}
}

// line: "ip:port:HOST|ip:host:GATE|ip:port|ip"
// return host, port, type parts
void JagDBServer::getDestHostPortType( const Jstr &inLine, 
			Jstr& host, Jstr& port, Jstr& destType )
{
	Jstr line = trimTailChar( inLine, '\r' );
	JagStrSplit sp(line, '|', true );
	int len = sp.length();
	int i = rand() % len;
	Jstr seg = sp[i];
	JagStrSplit sp2( seg, ':', true );
	if ( sp2.length() > 2 ) {
		host = sp2[0];
		port = sp2[1];
		destType = sp2[2];
	} else if ( sp2.length() > 1 ) {
		host = sp2[0];
		port = sp2[1];
		destType = "HOST";
	} else {
		host = sp2[0];
		port = "8888";
		destType = "HOST";
	}
}

void JagDBServer::refreshUserDB( jagint seq )
{
	if ( ( seq % 10 ) != 0 ) return;

	if ( _userDB ) {
		_userDB->refresh();
	}

	if ( _prevuserDB ) {
		_prevuserDB->refresh();
	}

	if ( _nextuserDB ) {
		_nextuserDB->refresh();
	}
}

void JagDBServer::refreshUserRole( jagint seq )
{
	if ( ( seq % 10 ) != 0 ) return;

	if ( _userRole ) {
		_userRole->refresh();
	}

	if ( _prevuserRole ) {
		_prevuserRole->refresh();
	}

	if ( _nextuserRole ) {
		_nextuserRole->refresh();
	}
}

// method read affect userrole or schema
void JagDBServer::showPerm( JagRequest &req, const JagParseParam &parseParam, jagint threadQueryTime )
{
	d("s3722 showPerm ...\n" );
	Jstr uid  = parseParam.grantUser;
	if ( uid.size() < 1 ) {
		uid = req.session->uid;
	}

	if ( req.session->uid != "admin" && uid != req.session->uid ) {
		Jstr err = Jstr("E5082 error show grants for user ") + uid;
		sendER( req, err);
		return;
	}

	d("s4423 showPerm uid=[%s]\n", uid.c_str() );

	JagUserRole *uidrole = NULL;
	if ( req.session->replicType == 0 ) uidrole = _userRole;
	else if ( req.session->replicType == 1 ) uidrole = _prevuserRole;
	else if ( req.session->replicType == 2 ) uidrole = _nextuserRole;


	// JAG_BLURT jaguar_mutex_lock ( &g_dbschemamutex ); JAG_OVER
	Jstr perms;
	if ( uidrole ) {
		perms = uidrole->showRole( uid );
	} else {
		perms = Jstr("E4024 error show grants for ") + uid;
	}

	d("s2473 perms=[%s] send to client\n", perms.c_str() );
	sendDataEnd( req, perms);
	// jaguar_mutex_unlock ( &g_dbschemamutex );
}

bool JagDBServer::checkUserCommandPermission( const JagSchemaRecord *srec, const JagRequest &req, 
	const JagParseParam &parseParam, int i, Jstr &rowFilter, Jstr &reterr )
{
	Jstr cs = _cfg->getValue("AUTH_USER_PERM", "no" );
	if ( cs == "no" ) {
		rowFilter = "";
		return true;
	}

	// For admin user, all true
	if ( req.session->uid == "admin" ) return true;
	JagUserRole *uidrole = NULL;

	if ( req.session->replicType == 0 ) uidrole = _userRole;
	else if ( req.session->replicType == 1 ) uidrole = _prevuserRole;
	else if ( req.session->replicType == 2 ) uidrole = _nextuserRole;

	return uidrole->checkUserCommandPermission( srec, req, parseParam, i, rowFilter, reterr );
}

// object method
// if dbExist  
bool JagDBServer::dbExist( const Jstr &dbName, int replicType )
{
	Jstr jagdatahome = _cfg->getJDBDataHOME( replicType );
    Jstr sysdir = jagdatahome + "/" + dbName;
	if ( JagFileMgr::isDir( sysdir ) > 0 ) {
		return true;
	} else {
		return false;
	}
}

// obj: table name or index name
bool JagDBServer::objExist( const Jstr &dbname, const Jstr &objName, int replicType )
{
	JagTableSchema *tableschema = NULL;
	JagIndexSchema *indexschema = NULL;
	bool rc;
	getTableIndexSchema( replicType, tableschema, indexschema );
	if (  tableschema ) {
		rc = tableschema->dbTableExist( dbname, objName );
		if ( rc ) return true;
	}

	if (  indexschema ) {
		rc = indexschema->indexExist( dbname, objName );
		if ( rc ) return true;
	}

	return false;

}

Jstr JagDBServer
::fillDescBuf( const JagSchema *schema, const JagColumn &column, const Jstr &dbtable, 
		       bool convertToSec, bool &doneConvert ) const
{
	char buf[512];
	Jstr type = column.type;
	int  len = column.length;
	int  sig = column.sig;
	int  srid = column.srid;
	int  metrics = column.metrics;
	Jstr dbcol = dbtable + "." + column.name.c_str();
	//d("s5041 fillDescBuf type=[%s]\n", type.c_str() );

	Jstr res;
	doneConvert = false;
	if ( type == JAG_C_COL_TYPE_DBOOLEAN ) {
		sprintf( buf, "boolean" );
		res += buf;
	} else if ( type == JAG_C_COL_TYPE_DBIT ) {
		sprintf( buf, "bit" );
		res += buf;
	} else if ( type == JAG_C_COL_TYPE_DINT ) {
		sprintf( buf, "int" );
		res += buf;
	} else if ( type == JAG_C_COL_TYPE_DTINYINT ) {
		sprintf( buf, "tinyint" );
		res += buf;
	} else if ( type == JAG_C_COL_TYPE_DSMALLINT ) {
		sprintf( buf, "smallint" );
		res += buf;
	} else if ( type == JAG_C_COL_TYPE_DMEDINT ) {
		sprintf( buf, "mediumint" );
		res += buf;
	} else if ( type == JAG_C_COL_TYPE_DBIGINT ) {
		sprintf( buf, "bigint" );
		res += buf;
	} else if ( type == JAG_C_COL_TYPE_FLOAT ) {
		sprintf( buf, "float(%d.%d)", len-2, sig );
		res += buf;
	} else if ( type == JAG_C_COL_TYPE_DOUBLE ) {
		sprintf( buf, "double(%d.%d)", len-2, sig );
		res += buf;
	} else if ( type == JAG_C_COL_TYPE_LONGDOUBLE ) {
		sprintf( buf, "longdouble(%d.%d)", len-2, sig );
		res += buf;
	} else if ( type == JAG_C_COL_TYPE_DATETIMEMICRO ) {
		if ( convertToSec ) {
			sprintf( buf, "datetimesec" );
			doneConvert = true;
		} else {
			sprintf( buf, "datetime" );
		}
		res += buf;
	} else if ( type == JAG_C_COL_TYPE_DATETIMESEC ) {
		sprintf( buf, "datetimesec" );
		res += buf;
	} else if ( type == JAG_C_COL_TYPE_TIMESTAMPSEC ) {
		sprintf( buf, "timestampsec" );
		res += buf;
	} else if ( type == JAG_C_COL_TYPE_TIMESTAMPMICRO ) {
		if ( convertToSec ) {
			sprintf( buf, "datetimesec" );
			doneConvert = true;
		} else {
			sprintf( buf, "timestamp" );
		}
		res += buf;
	} else if ( type == JAG_C_COL_TYPE_TIMEMICRO ) {
		sprintf( buf, "time" );
		res += buf;
	} else if ( type == JAG_C_COL_TYPE_DATETIMENANO ) {
		if ( convertToSec ) {
			sprintf( buf, "datetimesec" );
			doneConvert = true;
		} else {
			sprintf( buf, "datetimenano" );
		}
		res += buf;
	} else if ( type == JAG_C_COL_TYPE_TIMESTAMPNANO ) {
		if ( convertToSec ) {
			sprintf( buf, "datetimesec" );
			doneConvert = true;
		} else {
			sprintf( buf, "timestampnano" );
		}
		res += buf;
	} else if ( type == JAG_C_COL_TYPE_DATETIMEMILLI ) {
		if ( convertToSec ) {
			sprintf( buf, "datetimesec" );
			doneConvert = true;
		} else {
			sprintf( buf, "datetimemill" );
		}
		res += buf;
	} else if ( type == JAG_C_COL_TYPE_TIMESTAMPMILLI ) {
		if ( convertToSec ) {
			sprintf( buf, "datetimesec" );
			doneConvert = true;
		} else {
			sprintf( buf, "timestampmill" );
		}
		res += buf;
	} else if ( type == JAG_C_COL_TYPE_TIMENANO ) {
		sprintf( buf, "timenano" );
		res += buf;
	} else if ( type == JAG_C_COL_TYPE_DATE ) {
		sprintf( buf, "date" );
		res += buf;
	} else if ( type == JAG_C_COL_TYPE_POINT ) {
		res += columnProperty("point", srid, metrics );
	} else if ( type == JAG_C_COL_TYPE_POINT3D ) {
		res += columnProperty("point3d", srid, metrics );
	} else if ( type == JAG_C_COL_TYPE_CIRCLE ) {
		res += columnProperty("circle", srid, metrics );
	} else if ( type == JAG_C_COL_TYPE_CIRCLE3D ) {
		res += columnProperty("circle3d", srid, metrics );
	} else if ( type == JAG_C_COL_TYPE_TRIANGLE ) {
		res += columnProperty("triangle", srid, metrics );
	} else if ( type == JAG_C_COL_TYPE_TRIANGLE3D ) {
		res += columnProperty("triangle3d", srid, metrics );
	} else if ( type == JAG_C_COL_TYPE_SPHERE ) {
		res += columnProperty("sphere", srid, metrics );
	} else if ( type == JAG_C_COL_TYPE_SQUARE ) {
		res += columnProperty("square", srid, metrics );
	} else if ( type == JAG_C_COL_TYPE_SQUARE3D ) {
		res += columnProperty("square3d", srid, metrics );
	} else if ( type == JAG_C_COL_TYPE_CUBE ) {
		res += columnProperty("cube", srid, metrics );
	} else if ( type == JAG_C_COL_TYPE_RECTANGLE ) {
		res += columnProperty("rectangle", srid, metrics );
	} else if ( type == JAG_C_COL_TYPE_RECTANGLE3D ) {
		res += columnProperty("rectangle3d", srid, metrics );
	} else if ( type == JAG_C_COL_TYPE_BOX ) {
		res += columnProperty("box", srid, metrics );
	} else if ( type == JAG_C_COL_TYPE_CYLINDER ) {
		res += columnProperty("cylinder", srid, metrics );
	} else if ( type == JAG_C_COL_TYPE_CONE ) {
		res += columnProperty("cone", srid, metrics );
	} else if ( type == JAG_C_COL_TYPE_LINE ) {
		res += columnProperty("line", srid, metrics );
	} else if ( type == JAG_C_COL_TYPE_LINE3D ) {
		res += columnProperty("line3d", srid, metrics );
	} else if ( type == JAG_C_COL_TYPE_VECTOR ) {
		res += columnProperty("vector", srid, metrics );
	} else if ( type == JAG_C_COL_TYPE_LINESTRING ) {
		res += columnProperty("linestring", srid, metrics );
	} else if ( type == JAG_C_COL_TYPE_LINESTRING3D ) {
		res += columnProperty("linestring3d", srid, metrics );
	} else if ( type == JAG_C_COL_TYPE_MULTIPOINT ) {
		res += columnProperty("multipoint", srid, metrics );
	} else if ( type == JAG_C_COL_TYPE_MULTIPOINT3D ) {
		res += columnProperty("multipoint3d", srid, metrics );
	} else if ( type == JAG_C_COL_TYPE_MULTILINESTRING ) {
		res += columnProperty("multilinestring", srid, metrics );
	} else if ( type == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
		res += columnProperty("multilinestring3d", srid, metrics );
	} else if ( type == JAG_C_COL_TYPE_POLYGON ) {
		res += columnProperty("polygon", srid, metrics );
	} else if ( type == JAG_C_COL_TYPE_POLYGON3D ) {
		res += columnProperty("polygon3d", srid, metrics );
	} else if ( type == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		res += columnProperty("multipolygon", srid, metrics );
	} else if ( type == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
		res += columnProperty("multipolygon3d", srid, metrics );
	} else if ( type == JAG_C_COL_TYPE_ELLIPSE ) {
		res += columnProperty("ellipse", srid, metrics );
	} else if ( type == JAG_C_COL_TYPE_ELLIPSE3D ) {
		res += columnProperty("ellipse3d", srid, metrics );
	} else if ( type == JAG_C_COL_TYPE_ELLIPSOID ) {
		res += columnProperty("ellipsoid", srid, metrics );
	} else if ( type == JAG_C_COL_TYPE_RANGE ) {
		if ( srid > 0 ) {
			Jstr s = JagParser::getFieldTypeString( srid );
			sprintf( buf, "range(%s)", s.c_str() );
		} else {
			sprintf( buf, "range" );
		}
		res += buf;
	} else if ( column.spare[1] == JAG_C_COL_TYPE_UUID[0] ) {
		sprintf( buf, "uuid" );
		res += buf;
	} else if ( column.spare[1] == JAG_C_COL_TYPE_FILE[0] ) {
		sprintf( buf, "file" );
		res += buf;
	} else if ( column.spare[1] == JAG_C_COL_TYPE_ENUM[0] ) {
		Jstr defvalStr;
		Jstr enumstr =  "enum(";
		schema->getAttrDefVal( dbcol, defvalStr );
		JagStrSplitWithQuote sp( defvalStr.c_str(), '|' );
		for ( int k = 0; k < sp.length()-1; ++k ) {
			if ( 0 == k ) {
				enumstr += sp[k];
			} else {
				enumstr += Jstr(",") + sp[k];
			}
		}
		if ( *(column.spare+4) == JAG_CREATE_DEFINSERTVALUE ) {
			enumstr += ")";
		} else {
			enumstr += Jstr(",") + sp[sp.length()-1] + ")";;
		}
		res += enumstr;
	} else if ( type == JAG_C_COL_TYPE_STR ) {
		sprintf( buf, "char(%d)", len );
		res += buf;
	} else {
		sprintf( buf, "[%s] Error", type.c_str() );
		res += buf;
	}

	return res;
}

Jstr JagDBServer::columnProperty(const char *ctype, int srid, int metrics ) const
{
	char buf[64];
	if ( srid > 0 || metrics > 0 ) {
		sprintf( buf, "%s(srid:%d,metrics:%d)", ctype, srid, metrics );
	} else {
		sprintf( buf, "%s", ctype );
	}

	return buf;
}

int JagDBServer::processSelectConstData( const JagRequest &req, const JagParseParam *parseParam )
{
    JagRecord rec;
	Jstr asName;
    int  typeMode;
    Jstr type;
    int length;
    ExprElementNode *root; 
	int cnt = 0;

	dn("s1283 in processSelectConstData parseParam->selColVec.size()=%d", parseParam->selColVec.size() );

	for ( int i=0; i < parseParam->selColVec.size(); ++i ) {
        root = parseParam->selColVec[i].tree->getRoot();
    	JagFixString str;

    	root->checkFuncValidConstantCalc( str, typeMode, type, length );

    	d("s7372 checkFuncValidConstantOnly str=[%s] typeMode=%d type=[%s] length=%d name=%s asname=%s\n",
    		 str.c_str(), typeMode, type.c_str(), length, 
    		 parseParam->selColVec[i].name.c_str(), parseParam->selColVec[i].asName.c_str() );
		if ( str.size() > 0 ) {
    		asName = parseParam->selColVec[i].asName;
           	rec.addNameValue( asName.c_str(), str.c_str() );
			++cnt;
		}
	}

    dn("s108887 processSelectConstData cnt=%d", cnt );

	if ( cnt > 0 ) {
       	//int rc = sendMessageLength( req, rec.getSource(), rec.getLength(), "KV" );
       	//int rc = sendMessageLength( req, rec.getSource(), rec.getLength(), JAG_MSG_KV, JAG_MSG_NEXT_END);
		/***
       	int rc = sendMessageLength( req, rec.getSource(), rec.getLength(), JAG_MSG_KV, JAG_MSG_NEXT_MORE);
		//d("s1128 sendMessageLength msg=[%s] rc=%d\n", rec.getSource(), rc  );
		sendEOM(req, "sc02837" );
		return rc;
		***/
		//sendDataEnd( req, rec.getSource() );
       	int rc = sendMessageLength( req, rec.getSource(), rec.getLength(), JAG_MSG_KV, JAG_MSG_NEXT_END);
        dn("s5390021 sendMessageLength rec.getSource=[%s] JAG_MSG_KV", rec.getSource() );
		return rc;
	} else {
        dn("s42008801 processSelectConstData sendEOM sc037000");
		sendEOM(req, "sc037000" );
		return 0;
	}
}

// sendsmsg to client
void JagDBServer::processInternalCommands( int rc, const JagRequest &req, const char *pmesg )
{
	d("s50047 processInternalCommands pmesg=[%s]\n", pmesg );

	if ( JAG_SCMD_NOOP == rc ) {
		//sendMessage( req, "_END_[T=00|E=]", "ED" );
		sendEOM(req, "np" );
	} else if ( JAG_SCMD_CHKKEY == rc ) {
		d("s3330437 client requested _chkkey \n");
		chkkey( pmesg, req );
		dn("s2224038 chkkey done");
	} else if ( JAG_SCMD_CSCHEMA_MORE == rc ) {
		d("s3330937 client requested _cschema_more sending sendSchemaMap ... and sendMessage _END_\n");
		sendSchemaMap( pmesg, req, false );
		dn("s2229038 sendSchemaMap more done");
	} else if ( JAG_SCMD_CSCHEMA == rc ) {
		d("s3330907 client requested _cschema sending sendSchemaMap ... and sendMessage _END_\n");
		sendSchemaMap( pmesg, req, true );
		dn("s2229028 sendSchemaMap done");
	} else if ( JAG_SCMD_CHOST == rc ) {
		sendHostInfo( pmesg, req, true );
	} else if ( JAG_SCMD_CRECOVER == rc ) {
        dn("s176288 JAG_SCMD_CRECOVER cleanRecovery()...");
		cleanRecovery( pmesg, req );
		sendEOM( req, "crecov");
	} else if ( JAG_SCMD_CHECKDELTA == rc ) {
		checkDeltaFiles( pmesg, req );
		sendEOM( req, "chkdt" );
	} else if ( JAG_SCMD_BFILETRANSFER == rc ) {
		jd(JAG_LOG_LOW, "receive file [%s] ...\n", pmesg );
		JagStrSplit sp( pmesg, '|', true );
		if ( sp.length() >= 4 ) {
			int fpos = jagatoi(sp[1].c_str());
			if ( fpos < 10 ) {
				jd(JAG_LOG_LOW, "recovery receive file ...\n");
				recoveryFileReceiver( pmesg, req );
				jd(JAG_LOG_LOW, "recovery receive file done\n");
			} else if ( fpos >= 10 && fpos < 20 ) {
				// should not get this
				walFileReceiver( pmesg, req );
			}
		}
		// sendEOM( req, "begfxf" );
	} else if ( JAG_SCMD_ABFILETRANSFER == rc ) {
		recoveryFileReceiver( pmesg, req );
		_addClusterFlag = 1;
		sendEOM( req, "abegfxf" );
	} else if ( JAG_SCMD_OPINFO == rc ) {
		sendOpInfo( pmesg, req );
		sendEOM( req, "opi" );
	} else if ( JAG_SCMD_COPYDATA == rc ) {
		doCopyData( pmesg, req );
		sendEOM( req, "cpd" );
	} else if ( JAG_SCMD_REFRESHACL == rc ) {
		doRefreshACL( pmesg, req );
		sendEOM( req, "freshacl" );
	} else if ( JAG_SCMD_UNPACKSCHINFO == rc ) {
		unpackSchemaInfo( pmesg, req );
		sendEOM( req, "unpksch" );
	} else if ( JAG_SCMD_DOLOCALBACKUP == rc ) {
		doLocalBackup( pmesg, req );
		sendEOM( req, "lclbk" );
	} else if ( JAG_SCMD_DOREMOTEBACKUP == rc ) {
		doRemoteBackup( pmesg, req );
		sendEOM( req, "rmtbk" );
	} else if ( JAG_SCMD_DORESTOREREMOTE == rc ) {				
		doRestoreRemote( pmesg, req );
		sendEOM( req, "rmtrst" );
	} else if ( JAG_SCMD_MONDBTAB == rc ) {
		dbtabInfo( pmesg, req );
	} else if ( JAG_SCMD_MONINFO == rc ) {
		sendInfo( pmesg, req );
	} else if ( JAG_SCMD_MONRSINFO == rc ) {
		sendResourceInfo( pmesg, req );
	} else if ( JAG_SCMD_MONCLUSTERINFO == rc ) {
		sendClusterOpInfo( pmesg, req );
	} else if ( JAG_SCMD_MONHOSTS == rc ) {
		sendHostsList( pmesg, req );
	} else if ( JAG_SCMD_MONBACKUPHOSTS == rc ) {
		sendRemoteHostsInfo( pmesg, req );
	} else if ( JAG_SCMD_MONLOCALSTAT == rc ) {
		sendLocalStat6( pmesg, req );
	} else if ( JAG_SCMD_MONCLUSTERSTAT == rc ) {
		sendClusterStat6( pmesg, req );
	} else if ( JAG_SCMD_EXPROCLOCALBACKUP == rc ) {
		// no _END_ ED sent, already sent in method
		processLocalBackup( pmesg, req );
	} else if ( JAG_SCMD_EXPROCREMOTEBACKUP == rc ) {
		// no _END_ ED sent, already sent in method
		processRemoteBackup( pmesg, req );
	} else if ( JAG_SCMD_EXRESTOREFROMREMOTE == rc ) {
		// no _END_ ED sent, already sent in method
		processRestoreRemote( pmesg, req );
	} else if ( JAG_SCMD_EXADDCLUSTER == rc ) {
		// no _END_ ED sent, already sent in method
		addCluster( pmesg, req );
		if ( !req.session->origserv && !_restartRecover ) {
			broadcastHostsToClients();
		}
	} else if ( JAG_SCMD_EXADDCLUSTER_MIGRATE == rc ) {
		addClusterMigrate( pmesg, req );
	} else if ( JAG_SCMD_EXADDCLUSTER_MIGRATE_CONTINUE == rc ) {
		addClusterMigrateContinue( pmesg, req );
	} else if ( JAG_SCMD_EXADDCLUSTER_MIGRATE_COMPLETE == rc ) {
		addClusterMigrateComplete( pmesg, req );
	} else if ( JAG_SCMD_IMPORTTABLE == rc ) {
		importTableDirect( pmesg, req );
	} else if ( JAG_SCMD_TRUNCATETABLE == rc ) {
		truncateTableDirect( pmesg, req );
	} else if ( JAG_SCMD_EXSHUTDOWN == rc ) {
		// no _END_ ED sent, already sent in method
		shutDown( pmesg, req );
	} else if ( JAG_SCMD_GETPUBKEY == rc ) {
		// no _END_ ED sent, already sent in method
		d("s112923 received JAG_SCMD_GETPUBKEY send to client pubkey=[%s] client=%s ...\n", 
			_publicKey.c_str(), req.session->ip.c_str() );

		sendDataEnd( req, _publicKey);
		//sendMessage( req, "_END_[T=455|E=]", "ED" );

		d("s112923 for JAG_SCMD_GETPUBKEY sent to client pubkey=[%s] done client=%s\n", 
			_publicKey.c_str(), req.session->ip.c_str() );

	} else if ( JAG_SCMD_RECVFILE == rc ) {
		receiveFile( pmesg, req );
    }
}  // end processInternalCommands()

// return < 0 for break; else for continue
// ALl sendmessage to client
// help, use db, auth, quit
int JagDBServer::processSimpleCommand( int simplerc, JagRequest &req, char *pmesg, int &authed )
{
	d("s59881 processSimpleCommand pmesg=[%s]\n", pmesg );
	int rc = 0;

	// one time do statement, fake loop
	do {
    	if ( ! authed && JAG_RCMD_AUTH != simplerc ) {
    		sendER( req, "E202208 Not authed before requesting query");
			rc = -10;
    		break;
    	}

		char *tok;
		char *saveptr;
    	if ( JAG_RCMD_HELP == simplerc ) {
    		strtok_r( pmesg, " ;", &saveptr );
    		tok = strtok_r( NULL, " ;", &saveptr );
    		if ( tok ) { helpTopic( req, tok ); } 
    		else { helpPrintTopic( req ); }
    	} else if ( JAG_RCMD_USE == simplerc ) {
    		// no _END_ ED sent, already sent in method
    		//useDB( req, servobj, pmesg, saveptr );
    		useDB( req, pmesg );
    	} else if ( JAG_RCMD_AUTH == simplerc ) {
			d("s400080 JAG_RCMD_AUTH pmesg=[%s] doAuth() ...\n", pmesg );

    		if ( doAuth( req, pmesg ) ) {
    			// serv->serv still needs the insert pool when built init index from table
				d("s500132 doAuth is true\n");
    			authed = 1; 
    			struct timeval now;
    			gettimeofday( &now, NULL );
    			req.session->connectionTime = now.tv_sec * (jagint)1000000 + now.tv_usec;

				if ( !req.session->origserv && !_restartRecover ) {
					_clientSocketMap.addKeyValue( req.session->sock, 1 );
				}

    		} else {
    			req.session->active = 0;
				rc = -20;
				d("s588080 doAuth failed rc=-20 break\n");
    			break;
    		}		
    	} else if ( JAG_RCMD_QUIT == simplerc ) {
    		noLinger( req );
    		if ( req.session->exclusiveLogin ) {
    			_exclusiveAdmins = 0;
    		}
			rc = -30;
    		break;
    	} else if ( JAG_RCMD_HELLO == simplerc ) {
			char hellobuf[64];
    		sprintf( hellobuf, "JaguarDB Server Version: %s", _version.c_str() );
    		rc = sendDataEnd( req, hellobuf);  // SG: Server Greeting
    		req.session->fromShell = 1;
    	}

		rc = 0;

	} while (false);

	d("s500223 processSimpleCommand return rc=%d 0 is OK\n", rc );
	return rc;
}


// return < 0 : for error; 0 for OK
int JagDBServer::handleRestartRecover( const JagRequest &req, 
									   const char *pmesg, jagint len,
									   char *hdr2, char *&newbuf, char *&newbuf2 )
{
	d("s550283 handleRestartRecover ...\n");
	int rspec = 0;

	if ( req.batchReply ) {
		regSplogCommand( req.session, pmesg, len, 2 );
	} else if ( 0 == strncasecmp( pmesg, "update", 6 ) || 0 == strncasecmp( pmesg, "delete", 6 ) ) {
		//d("s43083 update or delete regSplogComman ...\n");
		regSplogCommand( req.session, pmesg, len, 1 );
	} else {
		regSplogCommand( req.session, pmesg, len, 0 );
		rspec = 1;
	}
	jaguar_mutex_unlock ( &g_flagmutex );

	int sprc = 1;
	char cond[3] = { 'O', 'K', '\0' };
	if ( 0 == req.session->drecoverConn && rspec == 1 ) {
        dn("s32901 sendDataEnd ...");
		if ( sendDataEnd( req, cond) < 0 ) {
			sprc = 0;
		} else {
            dn("1902103 recvMessage() ...");
			if ( recvMessage( req.session->sock, hdr2, newbuf2 ) < 0 ) {
				sprc = 0;
			}
		}
	}

	if ( 0 == sprc ) {
		jd(JAG_LOG_LOW, "E333309 error talking to client");
		return 0;
	}

	// receive three bytes of signal  3byte
	if ( sprc == 1 && _faultToleranceCopy > 1 && req.session->drecoverConn == 0 ) {
		// receive status bytes
		char rephdr[4];
		rephdr[3] = '\0';
		rephdr[0] = rephdr[1] = rephdr[2] = 'N';		

		int rsmode = 0;
		// "YYY" "NNN" "YNY" etc

        dn("s23094001 recvMessage() MMM ...");
		int rrc = recvMessage( req.session->sock, hdr2, newbuf2 );

		if ( rrc < 0 ) {
			rephdr[req.session->replicType] = 'Y';
			rsmode = getReplicateStatusMode( rephdr, req.session->replicType );
            dn("s5820031 getReplicateStatusMode rsmode=%d", rsmode );
			if ( !req.session->spCommandReject && rsmode >0 ) {
				deltalogCommand( rsmode, req.session, pmesg, req.batchReply );
			}

			if ( req.session->uid == "admin" ) {
				if ( req.session->exclusiveLogin ) {
					_exclusiveAdmins = 0;
					jd(JAG_LOG_LOW, "Exclusive admin disconnected from %s\n", req.session->ip.c_str() );
				}
			}

			if ( newbuf ) { free( newbuf ); }
			if ( newbuf2 ) { free( newbuf2 ); }
			-- _connections;

			return -1;
		}

		rephdr[0] = *newbuf2; 
        rephdr[1] = *(newbuf2+1); 
        rephdr[2] = *(newbuf2+2);

		rsmode = getReplicateStatusMode( rephdr );
		if ( !req.session->spCommandReject && rsmode >0 ) {
            dn("s553001 deltalogCommand [%s] req.batchReply=%d", pmesg, req.batchReply );
			deltalogCommand( rsmode, req.session, pmesg, req.batchReply );
		}

		sendEOM(req, "s54402");
	}

	return 0;
}

// shift pmesage from beginning special header bytes
void JagDBServer::rePositionMessage( JagRequest &req, char *&pmesg, jagint &len )
{
	char *p, *q;
	int tdlen = 0;

	p = q = pmesg;
	while ( *q != ';' && *q != '\0' ) ++q;
	if ( *q == '\0' ) {
		d("ERROR tdiff hdr for delta recover\n");
		abort();
	}

	tdlen = q-p;
	req.session->timediff = rayatoi( p, tdlen );

	pmesg = q+1; // ;pmesg
	len -= ( tdlen+1 );
	++q;
	p = q;
	while ( *q != ';' && *q != '\0' ) ++q;
	if ( *q == '\0' ) {
		d("ERROR isBatch hdr for delta recover\n");
		abort();
	}

	tdlen = q-p;
	req.batchReply = rayatoi( p, tdlen );

	pmesg = q+1;
	len -= ( tdlen+1 );
}

int JagDBServer::broadcastSchemaToClients()
{
	dn("s344093 broadcastSchemaToClients...");
	Jstr schemaInfo;
	_dbConnector->_broadcastCli->getSchemaMapInfo( schemaInfo );
	//d("bcast map info=[%s]\n", schemaInfo.c_str());
	if ( schemaInfo.size() < 1 ) {
		return 0;
	}

    char code4[5];
	char *buf = NULL;
    char sqlhdr[8]; makeSQLHeader( sqlhdr );

	jagint msglen = schemaInfo.size();
	const char *mesg = schemaInfo.c_str();

    if ( msglen >= JAG_SOCK_COMPRSS_MIN ) {
        Jstr comp;
        JagFastCompress::compress( mesg, msglen, comp );
        msglen = comp.size();
        buf = (char*)jagmalloc( JAG_SOCK_TOTAL_HDR_LEN+comp.size()+1+64 );

        sprintf( code4, "Z%c%cC", JAG_MSG_SCHEMA, JAG_MSG_NEXT_MORE );

        putXmitHdrAndData( buf, sqlhdr, comp.c_str(), msglen, code4 );
    } else {
        buf = (char*)jagmalloc( JAG_SOCK_TOTAL_HDR_LEN+msglen+1+64 );

        sprintf( code4, "C%c%cC", JAG_MSG_SCHEMA, JAG_MSG_NEXT_MORE );

        putXmitHdrAndData( buf, sqlhdr, mesg, msglen, code4 );
    }

	JagVector<int> vec = _clientSocketMap.getIntVector();
	int cnt = 0;
	int clientsock;
	jagint rc;
	for ( int i=0; i < vec.size(); ++i ) {
		clientsock = vec[i];
		dn("s2033970 i=%d sendRawData JAG_MSG_SCHEMA JAG_MSG_NEXT_END ...", i );
		rc = sendRawData( clientsock, buf, JAG_SOCK_TOTAL_HDR_LEN+msglen ); 
		if ( rc == JAG_SOCK_TOTAL_HDR_LEN+msglen ) {
			++ cnt;
		}
	}

	if ( buf ) free( buf );
	d("s39388 broadast schema to %d clients\n", cnt );
	return cnt;
}

int JagDBServer::broadcastHostsToClients()
{
    Jstr snodes = _dbConnector->_nodeMgr->_sendAllNodes;
	if ( snodes.size() < 1 ) {
		return 0;
	}
	d("s39388 broadast hosts to %d clients ...\n", snodes.size() );

    char code4[5];
	char *buf = NULL;
    char sqlhdr[8]; makeSQLHeader( sqlhdr );

	jagint msglen = snodes.size();
	const char *mesg = snodes.c_str();

    if ( msglen >= JAG_SOCK_COMPRSS_MIN ) {
        Jstr comp;
        JagFastCompress::compress( mesg, msglen, comp );
        msglen = comp.size();
        buf = (char*)jagmalloc( JAG_SOCK_TOTAL_HDR_LEN+comp.size()+1+64 );

        sprintf( code4, "Z%c%cC", JAG_MSG_HOST, JAG_MSG_NEXT_MORE );

        putXmitHdrAndData( buf, sqlhdr, comp.c_str(), msglen, code4 );
    } else {
        buf = (char*)jagmalloc( JAG_SOCK_TOTAL_HDR_LEN+msglen+1+64 );

        sprintf( code4, "C%c%cC", JAG_MSG_HOST, JAG_MSG_NEXT_MORE );

        putXmitHdrAndData( buf, sqlhdr, mesg, msglen, code4 );
    }

	JagVector<int> vec = _clientSocketMap.getIntVector();
	int cnt = 0;
	int clientsock;
	jagint rc;
	for ( int i=0; i < vec.size(); ++i ) {
		clientsock = vec[i];
		rc = sendRawData( clientsock, buf, JAG_SOCK_TOTAL_HDR_LEN+msglen ); 
		if ( rc == JAG_SOCK_TOTAL_HDR_LEN+msglen ) {
			++ cnt;
		}
	}

	if ( buf ) free( buf );
	d("s39388 broadast hosts to %d clients\n", cnt );
	return cnt;
}

void JagDBServer::makeNeededDirectories()
{
	Jstr fpath;

	fpath = jaguarHome() + "/tmp";
	JagFileMgr::rmdir( fpath );
	JagFileMgr::makedirPath( fpath );

	fpath = jaguarHome() + "/tmp/data";
	JagFileMgr::rmdir( fpath );
	JagFileMgr::makedirPath( fpath );

	fpath = jaguarHome() + "/tmp/pdata";
	JagFileMgr::rmdir( fpath );
	JagFileMgr::makedirPath( fpath );

	fpath = jaguarHome() + "/tmp/ndata";
	JagFileMgr::rmdir( fpath );
	JagFileMgr::makedirPath( fpath );

	fpath = jaguarHome() + "/tmp/join";
	JagFileMgr::rmdir( fpath );
	JagFileMgr::makedirPath( fpath );

	fpath = jaguarHome() + "/export";
	JagFileMgr::rmdir( fpath );
	JagFileMgr::makedirPath( fpath );

	fpath = jaguarHome() + "/backup";
	JagFileMgr::rmdir( fpath );

	JagFileMgr::makedirPath( fpath );
	JagFileMgr::makedirPath( fpath+"/15min" );
	JagFileMgr::makedirPath( fpath+"/hourly" );
	JagFileMgr::makedirPath( fpath+"/daily" );
	JagFileMgr::makedirPath( fpath+"/weekly" );
	JagFileMgr::makedirPath( fpath+"/monthly" );

}

// method to import local cached records and insert to a table
// pmesg: "_ex_importtable|db|table|finish(YES/NO)"
void JagDBServer::importTableDirect( const char *mesg, const JagRequest &req )
{
	if ( req.session->uid!="admin" ) {
		jd(JAG_LOG_LOW, "importTableDirect rejected. admin login is required\n" );
		sendER( req, "E30023 Command Failed. admin login is required");
		return;
	}

	JagStrSplit sp( mesg, '|');
	if ( sp.length() < 4 ) {
		jd(JAG_LOG_LOW, "importTableDirect rejected. wrong command [%s]\n", mesg );
		sendER( req, "E333300 Command Failed. importTableDirect rejected. wrong command");
		return;
	}

	Jstr db = sp[1];
	Jstr tab = sp[2];
	Jstr doFinishUp = sp[3]; // YES/NO

	Jstr dbtab = db + "." + tab;
	Jstr dirpath = jaguarHome() + "/export/" + dbtab;
	if ( doFinishUp == "YES" || doFinishUp == "Y" ) {
		JagFileMgr::rmdir( dirpath );
		d("s22029288 importTableDirect doFinishUp yes, rmdir %s\n", dirpath.s() );
		sendEOM( req, "impt100");
		return;
	}

	Jstr host = "localhost"; 
	Jstr objname = dbtab + ".sql";
	if ( this->_listenIP.size() > 0 ) { host = this->_listenIP; }
	Jstr connectOpt = Jstr("/TOKEN=") + this->_servToken;
	JaguarCPPClient pcli;
	// pcli.setDebug( true ); 
	while ( !pcli.connect( host.c_str(), this->_port, "admin", "anon", "test", connectOpt.c_str(), 0, _servToken.c_str() ) ) {
		jd(JAG_LOG_LOW, "s4022 Connect (%s:%s) (%s:%d) error [%s], retry ...\n", 
				  "admin", "anon", host.c_str(), this->_port, pcli.error() );
		jagsleep(5, JAG_SEC);
	}

	d("s344877 connect to localhost got pcli._allHostsString=[%s]\n", pcli._allHostsString.s() );

	Jstr fpath = JagFileMgr::getFileFamily( JAG_TABLE, dirpath, objname );
	d("s220291 getFileFamily dirpath=[%s] fpath=[%s]\n", dirpath.s(), fpath.s() );
	int rc = pcli.importLocalFileFamily( fpath );
	pcli.close();
	if ( rc < 0 ) {
		d("s4418 Import file not found on server %s\n", fpath.s() );
	}
	d("s1111028 importTable done rc=%d (<0 is error)\n", rc );

	sendEOM( req, "impt200");

	jd(JAG_LOG_LOW, "s300873 importTableDirect() is done\n" );
}

// method to truncate a table
// pmesg: "_ex_truncatetable|replicate_type(0/1/2)|db|table"
void JagDBServer::truncateTableDirect( const char *mesg, const JagRequest &req )
{
	if ( req.session->uid!="admin" ) {
		jd(JAG_LOG_LOW, "truncateTableDirect rejected. admin login is required\n" );
		sendER( req, "E9501 Command Failed. admin exclusive login is required");
		return;
	}

	JagStrSplit sp( mesg, '|');
	if ( sp.length() < 4 ) {
		jd(JAG_LOG_LOW, "truncateTableDirect rejected. wrong command [%s]\n", mesg );
		sendER( req, "E90511 Command Failed. truncateTableDirect rejected. wrong command");
		return;
	}

	Jstr reterr, indexNames;
	Jstr replicTypeStr = sp[1];
	int replicType = replicTypeStr.toInt();
	Jstr db = sp[2];
	Jstr tab = sp[3];

	Jstr dbobj = db + "." + tab;
	d("s22208 truncateTableDirect dbobj=%s replicType=%d\n", dbobj.s(), replicType );

	JagTable *ptab = NULL;
	JagIndex *pindex = NULL;
	JagTableSchema *tableschema;
	JagIndexSchema *indexschema;
	getTableIndexSchema( replicType, tableschema, indexschema );

	int lockrc;
	ptab = this->_objectLock->writeLockTable( JAG_TRUNCATE_OP, db, tab, tableschema, replicType, 0, lockrc );

	if ( ptab ) {
		indexNames = ptab->drop( reterr, true );
		d("s222029  ptab->drop() indexNames=%s\n", indexNames.s() );
	}

	refreshSchemaInfo( replicType, g_lastSchemaTime );
	
	if ( ptab ) {
		delete ptab; 
		// rebuild ptab, and possible related indexs

		ptab = _objectLock->writeTruncateTable( JAG_TRUNCATE_OP, db, tab, tableschema, replicType, 0 );
		if ( ptab ) {
			JagStrSplit sp( indexNames, '|', true );
			for ( int i = 0; i < sp.length(); ++i ) {
				pindex = _objectLock->writeLockIndex( JAG_CREATEINDEX_OP, db, tab, sp[i],
														tableschema, indexschema, replicType, true, lockrc );
				if ( pindex ) {
					ptab->_indexlist.append( pindex->getIndexName() );
				    _objectLock->writeUnlockIndex( JAG_CREATEINDEX_OP, db, tab, sp[i], replicType, true );
				}
			}
			this->_objectLock->writeUnlockTable( JAG_TRUNCATE_OP, db, tab, replicType, false );
		}
		jd(JAG_LOG_LOW, "user [%s] truncate table [%s]\n", req.session->uid.c_str(), dbobj.c_str() );
	} else {
		d("s2200112 no ptab\n");
	}

	d("s51128 truncateTableDirect() dbobj=%s done replicType=%d\n", dbobj.s(), replicType  );

	sendEOM( req, "trunctab");

	jd(JAG_LOG_LOW, "s300873 truncateTableDirect() replicType=%d is done\n", replicType );
}

void JagDBServer
::insertToTimeSeries( const JagSchemaRecord &parentSrec, const JagRequest &req, JagParseParam &parseParam, 
				  const Jstr &tser, const Jstr &dbName, const Jstr &tableName, 
				  const JagTableSchema *tableschema, int replicType, const Jstr &oricmd )
{
	if ( parseParam.insColMap ) {
		parseParam.insColMap->print();
	} 

	JagVector<ValueAttribute> rollupValueVec;
	for ( int i = 0; i < parentSrec.columnVector->size(); ++i ) {
		const JagColumn &jcol = (*parentSrec.columnVector)[i];
		if ( parseParam.valueVec[i].issubcol ) { 
			continue; 
		}

		if ( jcol.iskey ) {
			rollupValueVec.append( parseParam.valueVec[i] );
			continue;
		}

		if ( jcol.isrollup ) {
			rollupValueVec.append( parseParam.valueVec[i] );
			// 5 extra  col:sum col::min col::max  col::avg col::var
			rollupValueVec.append( parseParam.valueVec[i] );
			rollupValueVec.append( parseParam.valueVec[i] );
			rollupValueVec.append( parseParam.valueVec[i] );
			rollupValueVec.append( parseParam.valueVec[i] );
			parseParam.valueVec[i].valueData = "0";
			rollupValueVec.append( parseParam.valueVec[i] );
		}
	}

	ValueAttribute ca;
	ca.valueData = "1";
	rollupValueVec.append( ca );

	JagStrSplit sp( tser, ','); 
	Jstr tsTable, ts;
	int rc, lockrc;

	for ( int i =0; i < sp.length(); ++ i ) {
		ts = sp[i];
		tsTable = tableName + "@" + ts;
		JagTable *rtab = _objectLock->writeLockTable( parseParam.opcode, dbName, tsTable, tableschema, replicType, 0, lockrc ); 

		if ( rtab ) {
			Jstr errmsg;
			JagVector<JagDBPair> retpairVec;
			parseParam.objectVec[0].tableName = tsTable;
			parseParam.valueVec = rollupValueVec;

			rc = rtab->parsePair( req.session->timediff, &parseParam, retpairVec, errmsg  ); 

			JagDBPair &insPair = retpairVec[0];
			if ( rc ) {
				rc = rtab->rollupPair( req, insPair, rollupValueVec );
				if ( rc ) {
				} else {
					_dbLogger->logerr( req, errmsg, oricmd + "@" + ts );
				}
			} else {
				_dbLogger->logerr( req, errmsg, oricmd + "@" + ts );
			}

			_objectLock->writeUnlockTable( parseParam.opcode, dbName, tsTable, replicType, 0 ); 

		} else {
			jd(JAG_LOG_LOW, "E32038 Error: timeseries table [%s] not found\n", tsTable.s() );
		}
	}
}

int JagDBServer
::createTimeSeriesTables( const JagRequest &req, const Jstr &timeSeries, const Jstr &dbname, const Jstr &dbtable, 
					    const JagParseAttribute &jpa, Jstr &reterr )
{
	JagStrSplit sp( timeSeries, ':');
	int crc;
	Jstr sql;
	int  cnt = 0;

	for ( int i = 0; i < sp.length(); ++i ) {
		sql = describeTable( JAG_TABLE_TYPE, req, _tableschema, dbtable, false, true, true, sp[i] ); 
		if ( sql.size() < 4 ) continue;
		sql.replace('\n', ' ');
        sql = sql.condenseSpaces();

		JagParser parser((void*)this);
		JagParseParam pparam2( &parser );
		if ( parser.parseCommand( jpa, sql, &pparam2, reterr ) ) {
			crc = createSimpleTable( req, dbname, &pparam2 );
			if ( ! crc ) {
				jd(JAG_LOG_LOW, "E20320 Error: creating timeseries table [%s]\n", sql.s() );
			} else {
				jd(JAG_LOG_LOW, "OK: creating timeseries table [%s]\n", sql.s() );
				++ cnt;
			}
		} else {
			jd(JAG_LOG_LOW, "E20321 Error: parse [%s] [%s]\n", sql.s(), reterr.s() );
		}
	}

	return cnt;
}

void JagDBServer
::dropTimeSeriesTables( const JagRequest &req, const Jstr &timeSeries, const Jstr &dbname, const Jstr &dbtable, 
				    const JagParseAttribute &jpa, Jstr &reterr )
{
	JagStrSplit sp( timeSeries, ',');
	int crc;
	Jstr sql, rollTab;
	for ( int i = 0; i < sp.length(); ++i ) {
		JagStrSplit ss(sp[i], '_');
		rollTab = ss[0];

		sql = Jstr("drop table ") + dbtable + "@" + rollTab; 
		JagParser parser((void*)this);
		JagParseParam pparam2( &parser );
		if ( parser.parseCommand( jpa, sql, &pparam2, reterr ) ) {
			crc = dropSimpleTable( req, &pparam2, reterr, true );
			if ( ! crc ) {
				jd(JAG_LOG_LOW, "Error: drop timeseries table [%s][%s]\n", sql.s(), reterr.s() );
			} else {
				jd(JAG_LOG_LOW, "OK: drop timeseries rollup table [%s]\n", sql.s() );
			}
		} else {
			jd(JAG_LOG_LOW, "Error: parse [%s] [%s]\n", sql.s(), reterr.s() );
		}
	}
}

void JagDBServer
::createTimeSeriesIndexes( const JagParseAttribute &jpa, const JagRequest &req, 
					     const JagParseParam &parseParam, const Jstr &timeSeries, Jstr &reterr )
{
	Jstr dbname =  parseParam.objectVec[0].dbName;
	JagStrSplit sp( timeSeries, ',');
	int crc;
	Jstr sql;
	JagTable *newPtab;
	JagIndex *newPindex;
	JagRequest req2( req );

	for ( int i = 0; i < sp.length(); ++i ) {
		sql = describeIndex( false, req, _indexschema, dbname, 
							 parseParam.objectVec[1].indexName, reterr, true, true, sp[i] );
		if ( sql.size() < 4 ) continue;
		sql.replace('\n', ' ');
        sql = sql.condenseSpaces();
		JagParser parser((void*)this);
		JagParseParam pparam2( &parser );
		if ( parser.parseCommand( jpa, sql, &pparam2, reterr ) ) {
			crc = createSimpleIndex( req, &pparam2, newPtab, newPindex, reterr );
			if ( ! crc ) {
				jd(JAG_LOG_LOW, "Error: creating timeseries rollup index [%s]\n", sql.s() );
			} else {
				jd(JAG_LOG_LOW, "OK: creating timeseries rollup index [%s]\n", sql.s() );
			}
		} else {
			jd(JAG_LOG_LOW, "E21013 Error: parse [%s] [%s]\n", sql.s(), reterr.s() );
		}
	}
}

int JagDBServer::
createSimpleIndex( const JagRequest &req, JagParseParam *parseParam,
		        JagTable *&ptab, JagIndex *&pindex, Jstr &reterr )
{
	JagIndexSchema *indexschema; 
	JagTableSchema *tableschema;
	getTableIndexSchema(  req.session->replicType, tableschema, indexschema );
	Jstr dbname = parseParam->objectVec[0].dbName;
	int replicType = req.session->replicType;
	int opcode = parseParam->opcode;

	Jstr dbindex = dbname + "." + parseParam->objectVec[0].tableName + 
							 "." + parseParam->objectVec[1].indexName;

	Jstr tgttab = indexschema->getTableNameScan( dbname, parseParam->objectVec[1].indexName );
	if ( tgttab.size() > 0 ) {
		reterr = "E20373 Error: index already exists in database";
		return 0;
	}

	int rc = 0;
	int lockrc;

	Jstr dbobj = dbname + "." + parseParam->objectVec[0].tableName;
	Jstr scdbobj = dbobj + "." + intToStr( replicType );

	ptab = _objectLock->writeLockTable( opcode, parseParam->objectVec[0].dbName, 
										 parseParam->objectVec[0].tableName, tableschema, replicType, 0, lockrc );
	if ( ptab ) {
		rc = createIndexSchema( req, dbname, parseParam, reterr, false );
	} else {
		reterr = Jstr("E22208 Error: unable to find table ") + parseParam->objectVec[0].tableName ;
		return 0;
	}
	
	refreshSchemaInfo( replicType, g_lastSchemaTime );
	pindex = _objectLock->writeLockIndex( opcode, parseParam->objectVec[1].dbName,
										   parseParam->objectVec[0].tableName, parseParam->objectVec[1].indexName,
										   tableschema, indexschema, replicType, true, lockrc );
	if ( ptab && pindex ) {
		doCreateIndex( ptab, pindex );
		rc = 1;
	} else {
	    indexschema->remove( dbindex );
		rc = 0;
	}

	if ( pindex ) {
		_objectLock->writeUnlockIndex(  opcode, parseParam->objectVec[1].dbName,
										parseParam->objectVec[0].tableName, parseParam->objectVec[1].indexName,
										replicType, true );		
	}

	if ( ptab ) {
		_objectLock->writeUnlockTable( opcode, parseParam->objectVec[0].dbName, 
										parseParam->objectVec[0].tableName, replicType, false );
	}

	jd(JAG_LOG_LOW, "user [%s] create index [%s]\n", req.session->uid.c_str(), scdbobj.c_str() );
	return rc;
}

void JagDBServer::
dropTimeSeriesIndexes( const JagRequest &req, const JagParseAttribute &jpa, 
					 const Jstr &parentTableName,
				     const Jstr &parentIndexName, const Jstr &timeSeries )
{
	JagStrSplit sp( timeSeries, ',');
	int crc;
	Jstr sql, rollTab, reterr;
	for ( int i = 0; i < sp.length(); ++i ) {
		JagStrSplit ss(sp[i], '_');
		rollTab = ss[0];

		sql = Jstr("drop index ") + parentIndexName + "@" + rollTab + " on " + parentTableName + "@" + rollTab; 
		JagParser parser((void*)this);
		JagParseParam pparam2( &parser );
		if ( parser.parseCommand( jpa, sql, &pparam2, reterr ) ) {
			crc = dropSimpleIndex( req, &pparam2, reterr, true );
			if ( ! crc ) {
				jd(JAG_LOG_LOW, "Error: drop timeseries index [%s][%s]\n", sql.s(), reterr.s() );
			} else {
				jd(JAG_LOG_LOW, "OK: drop timeseries rollup index [%s]\n", sql.s() );
			}
		} else {
			jd(JAG_LOG_LOW, "Error: parse [%s] [%s]\n", sql.s(), reterr.s() );
		}
	}
}

int JagDBServer::dropSimpleIndex( const JagRequest &req, const JagParseParam *parseParam, Jstr &reterr, bool lockSchema )
{
	Jstr dbname = parseParam->objectVec[0].dbName;
	Jstr dbobj = parseParam->objectVec[0].dbName + "." + parseParam->objectVec[0].tableName;
	Jstr scdbobj = dbobj + "." + intToStr( req.session->replicType );
	JagTable *ptab = NULL;
	JagIndex *pindex = NULL;
	JagTableSchema *tableschema;
	JagIndexSchema *indexschema;
	int   lockrc1, lockrc2;

	getTableIndexSchema( req.session->replicType, tableschema, indexschema );

	ptab = _objectLock->writeLockTable( parseParam->opcode, parseParam->objectVec[0].dbName, 
										 parseParam->objectVec[0].tableName, tableschema, req.session->replicType, false, lockrc1 );	

	pindex = _objectLock->writeLockIndex( parseParam->opcode, parseParam->objectVec[1].dbName,
										  parseParam->objectVec[0].tableName, parseParam->objectVec[1].indexName,
										  tableschema, indexschema, req.session->replicType, true, lockrc2 );

	if ( ! pindex ) {
		if ( ptab ) {
			_objectLock->writeUnlockTable( parseParam->opcode, parseParam->objectVec[0].dbName, 
								   		   parseParam->objectVec[0].tableName, req.session->replicType, false );
		}
		return 0;
	}
	
	if ( ! ptab ) {
		_objectLock->writeUnlockIndex( parseParam->opcode, parseParam->objectVec[1].dbName,
								   parseParam->objectVec[0].tableName, parseParam->objectVec[1].indexName,
								   req.session->replicType, true );
		return 0;
	}

	Jstr dbtabidx;
	pindex->drop();
	dbtabidx = dbname + "." + parseParam->objectVec[0].tableName + "." + parseParam->objectVec[1].indexName;

	if ( lockSchema ) {
		JAG_BLURT jaguar_mutex_lock ( &g_dbschemamutex ); JAG_OVER
	}

	indexschema->remove( dbtabidx );
	ptab->dropFromIndexList( parseParam->objectVec[1].indexName );	
	refreshSchemaInfo( req.session->replicType, g_lastSchemaTime );

	if ( lockSchema ) {
		jaguar_mutex_unlock ( &g_dbschemamutex );
	}
	
	delete pindex; 
	_objectLock->writeUnlockIndex( parseParam->opcode, parseParam->objectVec[1].dbName,
								   parseParam->objectVec[0].tableName, parseParam->objectVec[1].indexName,
								   req.session->replicType, 1 );

	_objectLock->writeUnlockTable( parseParam->opcode, parseParam->objectVec[0].dbName, 
								   parseParam->objectVec[0].tableName, req.session->replicType, 0 );

	jd(JAG_LOG_LOW, "user [%s] drop simple index [%s]\n", req.session->uid.c_str(), dbtabidx.c_str() );
	return 1;
}

void JagDBServer::trimTimeSeries()
{
	const int replicType[] = { 0, 1, 2 };
	JagTable *ptab = NULL;
	JagTableSchema *tableschema;

	JagVector<AbaxString> *vec;
	Jstr    dbname, dbtab, tableName, allDBs;
	int     repType;
	bool    hasTser;
	jagint  cnt = 0;
	time_t  windowlen;
	int     lockrc;

	dn("s22209 trimTimeSeries...");

	for ( int i = 0; i < 3; ++i ) {
		dn("s102277 i=%d", i);

		repType = replicType[i];
		tableschema = this->getTableSchema( repType );
		allDBs = JagSchema::getDatabases( _cfg, repType );
		JagStrSplit sp(allDBs, '\n', true );

		for ( int j=0; j < sp.size(); ++j ) {
			dbname = sp[j];
			if ( dbname == "system" ) continue;
			vec = tableschema->getAllTablesOrIndexesLabel( JAG_TABLE_TYPE, dbname, "" );

			for ( int k = 0; k < vec->size(); ++k ) {
				dbtab = (*vec)[k].s();
				JagStrSplit ss(dbtab, '.');
				if ( ss.size() == 2 ) {
					tableName = ss[1];
				} else {
					tableName = dbtab;
				}

				//dn("s303380 writeLockTable tableName=%s ...", tableName.s() );
				ptab = _objectLock->writeLockTable( JAG_DELETE_OP, dbname, tableName, tableschema, repType, 0, lockrc );
				//dn("s303380 writeLockTable tableName=%s done", tableName.s() );
				if ( ! ptab ) { 
					continue; 
				}

				Jstr retention = ptab->timeSeriesRentention();
				Jstr timeSer;
				hasTser =  ptab->hasTimeSeries( timeSer );
				bool needProcess = false;
				if ( hasTser ) {
					if ( retention != "0" ) {
						needProcess = true;
					}
				} else {
					if ( ptab->hasRollupColumn() ) {
						if ( retention != "0" ) {
							needProcess = true;
						}
					} else {
					}

					if ( needProcess ) {
					} else {
						_objectLock->writeUnlockTable( JAG_DELETE_OP, dbname, tableName, repType, 0 );
						//dn("s30062 writeUnlockTable tableName=%s done continue", tableName.s() );
						continue; 
					}
				}

				windowlen = JagSchemaRecord::getRetentionSeconds( retention );
				cnt = -1;
				if ( windowlen > 0 ) {
					cnt = ptab->cleanupOldRecords( time(NULL) - windowlen );
                    if ( cnt > 0 ) {
                        in("Table %s cleanupOldRecords total = %ld", tableName.s(), cnt );
                    }

					cnt = trimWalLogFile( ptab, dbname, tableName, ptab->_darrFamily->_insertBufferMap, ptab->_darrFamily->_keyChecker );
                    if ( cnt > 0 ) {
                        in("Table %s trimWalLogFile total = %ld", tableName.s(), cnt );
                    }
				}

				_objectLock->writeUnlockTable( JAG_DELETE_OP, dbname, tableName, repType, 0 );
				dn("s30066 writeUnlockTable tableName=%s done", tableName.s() );
			}

			if ( vec ) delete vec;
		}
		dn("s3002737 done");
	}
	dn("s3002937 done");
}

jagint JagDBServer::trimWalLogFile( const JagTable *ptab, const Jstr &db, const Jstr &tab, 
									const JagDBMap *insertBufferMap, const JagFamilyKeyChecker *keyChecker )
{
	if ( db.size() < 1 || tab.size() <1 ) { return 0; }

	dn("s111027 inside trimWalLogFile()  jaguar_mutex_lock g_wallogmutex ...");
	JAG_BLURT jaguar_mutex_lock ( &g_wallogmutex ); JAG_OVER
	dn("s111027 inside trimWalLogFile()  jaguar_mutex_lock g_wallogmutex got lock.");

	Jstr fpath = _cfg->getWalLogHOME() + "/" + db + "." + tab + ".wallog";
	jagint cnt = doTrimWalLogFile( ptab, fpath, db, insertBufferMap, keyChecker );

	JAG_BLURT jaguar_mutex_unlock ( &g_wallogmutex ); 
	dn("s111028 inside trimWalLogFile()  jaguar_mutex_unlock g_wallogmutex done, OK.");

	return cnt;
}

jagint JagDBServer::doTrimWalLogFile( const JagTable *ptab, const Jstr &fpath, const Jstr &dbname, 
								      const JagDBMap *insertBufferMap, const JagFamilyKeyChecker *keyChecker )
{
	if ( JagFileMgr::fileSize( fpath ) <= 0 ) {
		return 0;
	}

	int i, fd = jagopen( fpath.c_str(), O_RDONLY|JAG_NOATIME );
	if ( fd < 0 ) return 0;

	Jstr newLogPath = fpath + ".trimmedXYZ";
	FILE *newLogFP = fopen( newLogPath.s(), "w" );
	if ( ! newLogFP ) {
		close(fd);
		jd(JAG_LOG_LOW, "E20281 Error open write [%s]\n", newLogPath.s() );
		return 0;
	}

	char    buf16[17];
	jagint  msglen;
	char    c;
	char    *msgbuf = NULL;
	int     replicType;
	int     timediff;
	int     batchReply;
	Jstr    reterr;

	jagint cntwrite = 0;
	jagint cntdel = 0;

	while ( 1 ) {
		i = 0;
		memset( buf16, 0, 4 );
		while( 1==read(fd, &c, 1) ) {
			buf16[i] = c;
			if ( c == ';' ) {
				buf16[i] =  '\0';
				break;
			} else {
				if ( i > 0 ) {
					jagclose( fd );
					return cntwrite;
				}
			}
			++i;
		}

		if ( buf16[0] == '\0' ) {
			break;
		}

		replicType = atoi( buf16 );

		i = 0;
		memset( buf16, 0, 17 );
		while( 1==read(fd, &c, 1) ) {
			buf16[i] = c;
			if ( c == ';' ) {
				buf16[i] =  '\0';
				break;
			} else {
				if ( i > 15 ) {
					jagclose( fd );
					return cntwrite;
				}
			}
			++i;
		}
		if ( buf16[0] == '\0' ) {
			break;
		}
		timediff = atoi( buf16 );

		i = 0;
		memset( buf16, 0, 4 );
		while( 1==read(fd, &c, 1) ) {
			buf16[i] = c;
			if ( c == ';' ) {
				buf16[i] =  '\0';
				break;
			} else {
				if ( i > 0 ) {
					jagclose( fd );
					return cntwrite;
				}
			}
			++i;
		}

		if ( buf16[0] == '\0' ) {
			printf("s3398 end isBatch 0 is 0\n");
			break;
		}

		batchReply = atoi( buf16 );

		memset( buf16, 0, JAG_REDO_MSGLEN+1 );
		raysaferead( fd, buf16, JAG_REDO_MSGLEN );
		msglen = jagatoll( buf16 );

		msgbuf = (char*)jagmalloc(msglen+1);
		memset(msgbuf, 0, msglen+1);
		raysaferead( fd, msgbuf, msglen );

		JagParseAttribute jpa( this, timediff, servtimediff, dbname, _cfg );
		JagParser parser((void*)this);
		JagParseParam pparam( &parser );
		JagVector<JagDBPair> pairVec;
		bool exist = false;
		bool brc = parser.parseCommand( jpa, msgbuf, &pparam, reterr );

		if ( brc && pparam.opcode == JAG_INSERT_OP ) {
			int prc = ptab->parsePair( timediff, &pparam, pairVec, reterr );
			if ( prc ) {
				if ( insertBufferMap->exist( pairVec[0] ) ) {
					exist = true;
				} else {
					char kbuf[ptab->_KEYLEN+1];
					memset( kbuf, 0, ptab->_KEYLEN+1);
					memcpy( kbuf, pairVec[0].key.c_str(), ptab->_KEYLEN );
					if ( keyChecker->exist( kbuf )  ) {
						exist = true;
					}
				}

				if ( exist ) {
					fprintf( newLogFP, "%d;%d;%d;%010lld%s", replicType, timediff, batchReply, msglen, msgbuf );
					++cntwrite;
				} else {
					++cntdel;
				}

			} else {
			}
		} else {
		}

		free( msgbuf );
		msgbuf = NULL;
	}

	jagclose( fd );
	fclose( newLogFP  );

	jagunlink( fpath.s() );
	jagrename( newLogPath.s(), fpath.s() );

	_walLogMap.removeKey( fpath );

	_walLogMap.ensureFile( fpath );

	jd(JAG_LOG_LOW, "trimWalLogFile %s write=%d deleted=%d\n", fpath.s(),  cntwrite, cntdel );
	return cntwrite;
}

int JagDBServer::getCurrentCluster() const
{
    return _dbConnector->_nodeMgr->_totalClusterNumber - 1;
}

int JagDBServer::getHostCluster() const
{
    return _dbConnector->_nodeMgr->_hostClusterNumber;
}

void JagDBServer::applyMultiTars( const Jstr &srcDir, const Jstr &tars, const Jstr &destDir )
{
    JagStrSplit sp(tars, '|', true);
    Jstr fpath, cmd;

    for ( int i = 0; i < sp.size(); ++i ) {
        fpath = srcDir + "/" + sp[i];
        cmd = Jstr("tar -zxf ") + fpath + " --keep-newer-files --directory=" + destDir;
	    jd(JAG_LOG_LOW, "%s\n", cmd.s() );
        system( cmd.c_str() );

		jd(JAG_LOG_LOW, "s63204 [%s]\n", cmd.c_str() );
		jagunlink(fpath.c_str());
		jd(JAG_LOG_LOW, "delete %s\n", fpath.c_str() );
    }
}

bool JagDBServer::isServerStatBytes( const char *msg, int msglen )
{
    if ( msg == NULL ) return false;

    if ( msglen != 3 ) {
        return false;
    }

    if ( ( msg[0] == 'Y' || msg[0] == 'N' )
           && ( msg[1] == 'Y' || msg[1] == 'N' )
           && ( msg[2] == 'Y' || msg[2] == 'N' ) ) {
       return true;
    }

    return false;
}


void JagDBServer::receiveFile( const char *mesg, const JagRequest &req )
{
    // _onefile|req.jpg|51857|50807936|db|tab|hashdir
    JagStrSplit sp(mesg, '|');

    Jstr  fname = sp[1];
    Jstr  fsize = sp[2];
    Jstr  db = sp[4];
    Jstr  tab = sp[5];
    Jstr  hashDir = sp[6];

  	Jstr jagdatahome = _cfg->getJDBDataHOME( req.session->replicType );

    Jstr fpath = jagdatahome + "/" + db + "/" + tab + "/files/" + hashDir;
    dn("s2930012 receiveFile fpath=[%s]", fpath.s() );

	JagFileMgr::makedirPath( fpath, 0700 );

    fpath += Jstr("/") + fname;
    dn("s2930013 receiveFile fpath=[%s]", fpath.s() );

	int sock = req.session->sock;

    jagint totlen = readSockAndSave( false, sock, fpath, fsize.tol() );
    dn("s03039001 readSockAndSave totlen=%ld fsize=%s", totlen, fsize.s() );

	sendEOM( req, "receiveFile" ); 
}

// return < 0 for error; else OK
int JagDBServer::processDeltaLog( const Jstr &fpath, FILE *& logf, pthread_mutex_t *mtx )
{
    in("s340288 processDeltaLog fpath=[%s] ...", fpath.s() );

    Jstr errmsg;
    int src = _dbConnector->_parentCli->pingFileHost( fpath, errmsg );
    if ( src < 0 ) {
        in("s345128 pingFileHost fpath=[%s] error=[%s] deltalog not sent", fpath.s(), errmsg.s() );
        return -888;
    }
    in("s35228 pingFileHost fpath=[%s] OK, continue ....", fpath.s() );

    Jstr histFile;

    JAG_BLURT jaguar_mutex_lock ( mtx ); JAG_OVER
	    jagfclose( logf );
	    logf = NULL;

        moveToHistory( fpath, histFile );
        in("s519711 move %s to %s", fpath.s(), histFile.s() );
	    logf = loopOpen( fpath.s(), "ab" );
        in("s1100083 reopen [%s]", fpath.s() );
    jaguar_mutex_unlock ( mtx );

    in("s1500818 recoverOneDeltaLog9%s) ...", histFile.s() );
    int rc = recoverOneDeltaLog( histFile );
    in("s1500818 recoverOneDeltaLog9%s) done. rc=%d", histFile.s(), rc );

    if ( rc < 0 ) {
        in("recoverOneDeltaLog [%s] error, restore [%s] ...", histFile.s(), fpath.s() );
        JAG_BLURT jaguar_mutex_lock ( mtx ); JAG_OVER
	        jagfclose( logf );
            Jstr tmpf = histFile + ".tmpf";
            char cmdbuf[1024];
            sprintf(cmdbuf, "cat %s %s > %s; mv -f %s %s", histFile.s(), fpath.s(), tmpf.s(), tmpf.s(),  fpath.s() );
            system( cmdbuf );
            in("s21018 %s", cmdbuf );

            jagunlink( histFile.s() );
            in("unlink %s", histFile.s() );

	        logf = loopOpen( fpath.s(), "ab" );
            in("s10287 opened %s to append", fpath.s() );
        jaguar_mutex_unlock ( mtx );
        in("s210762 restored [%s]", fpath.s() );
    } else {
        in("s291820 sent [%s]", histFile.s() );
    }

    in("s340298 processDeltaLog fpath=[%s] done", fpath.s() );
    return rc;
}
