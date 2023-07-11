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

#undef JAG_CLIENT_SIDE
#define JAG_SERVER_SIDE 1

#include <JagDef.h>
#include <JagDBServer.h>
#include <JagUtil.h>
#include <JagTable.h>
#include <JagHashLock.h>
#include <JagMutex.h>
#include <JagPass.h>
#include <JagUserRole.h>
#include <JagTableSchema.h>
#include <JagIndexSchema.h>
#include <JagSession.h>
#include <JagTable.h>
#include <JagIndex.h>
#include <JagServerObjectLock.h>
#include <JagParser.h>
#include <JagUserID.h>
#include <JagMD5lib.h>

int JagDBServer::importTable( JagRequest &req, const Jstr &dbname,
							  JagParseParam *parseParam, Jstr &reterr )
{
	int rc = 0;
	if ( req.isPrepare() ) {
		rc = importTablePrepare( req, dbname, parseParam);	
		if ( 0 == rc ) {
			sendDataEnd( req, "OK");
			req.session->spCommandReject = 0;
		} else {
			sendDataEnd( req, "NG");
			req.session->spCommandReject = 0;
		}
	} else {
		rc = importTableCommit( req, dbname, parseParam, reterr);	
	}

	d("s303344 importTable done rc=%d req.sqlhdr[2]=%c ...\n", rc, req.sqlhdr[2]);

	if ( 0 == rc ) return 1;
	else return 0;
}

// 0: good;  <0 bad
int JagDBServer::importTablePrepare( JagRequest &req, const Jstr &dbname,
							  JagParseParam *parseParam )
{
	return 0;
}

// 0: good;  <0 bad
int JagDBServer::importTableCommit( JagRequest &req, const Jstr &dbname,
							  JagParseParam *parseParam, Jstr &reterr )
{
	JagTable *ptab = NULL;

	Jstr dbtab = parseParam->objectVec[0].dbName + "." + parseParam->objectVec[0].tableName;
	//Jstr scdbobj = dbtab + "." + intToStr( req.session->replicType );

	Jstr dirpath = jaguarHome() + "/export/" + dbtab;
	if ( parseParam->impComplete ) {
		JagFileMgr::rmdir( dirpath );
		//schemaChangeCommandSyncRemove( scdbobj );
		//return 1;
		req.session->spCommandReject = 0;
		return 0;
	}

	int lockrc;
	ptab = _objectLock->readLockTable( parseParam->opcode, parseParam->objectVec[0].dbName, 
								       parseParam->objectVec[0].tableName, req.session->replicType, 0, lockrc );
	if ( ! ptab ) {
		jd(JAG_LOG_LOW, "E3008 ptab is NULL\n");
		req.session->spCommandReject = 0;
		return 0;
	}

	Jstr host = "localhost", objname = dbtab + ".sql";
	if ( _listenIP.size() > 0 ) { host = _listenIP; }
	JaguarCPPClient pcli;
	Jstr unixSocket = Jstr("/TOKEN=") + _servToken;
	while ( !pcli.connect( host.c_str(), _port, "admin", "anon", "test", unixSocket.c_str(), 0 ) ) {
		jd(JAG_LOG_LOW, "s4022 Connect (%s:%s) (%s:%d) error [%s], retry ...\n", 
				  "admin", "anon", host.c_str(), _port, pcli.error() );
		jagsleep(5, JAG_SEC);
	}

	if ( ptab ) {	
		_objectLock->readUnlockTable( parseParam->opcode, parseParam->objectVec[0].dbName, 
									   parseParam->objectVec[0].tableName, req.session->replicType, 0 );
	}
	//schemaChangeCommandSyncRemove( scdbobj );

	Jstr fpath = JagFileMgr::getFileFamily( JAG_TABLE, dirpath, objname );
	int rc = pcli.importLocalFileFamily( fpath );
	pcli.close();
	if ( rc < 0 ) {
		reterr = "Import file not found on server";
		return 0;
	}

	req.session->spCommandReject = 0;
	return 0;
}

void JagDBServer::createUser( JagRequest &req, JagParseParam &parseParam, jagint threadQueryTime )
{
	int rc = 0;
	if ( req.isPrepare() ) {
		rc = createUserPrepare( req, parseParam, threadQueryTime );	
		if ( 0 == rc ) {
			sendDataEnd( req, "OK");
			req.session->spCommandReject = 0;
		} else {
			sendDataEnd( req, "NG");
			req.session->spCommandReject = 0;
		}
	} else {
		rc = createUserCommit( req, parseParam, threadQueryTime );	
	}

	d("s303344 createUser done rc=%d req.sqlhdr[2]=%c ...\n", rc, req.sqlhdr[2]);
}

// 0: good; < 0: bad
int JagDBServer::createUserPrepare( JagRequest &req, JagParseParam &parseParam, jagint threadQueryTime )
{
	Jstr uid = parseParam.uid;
	Jstr pass = parseParam.passwd;

	if ( threadQueryTime > 0 && threadQueryTime < g_lastHostTime ) {
		return -10;
	}

	JagUserID *uiddb = NULL;
	if ( req.session->replicType == 0 ) uiddb = _userDB;
	else if ( req.session->replicType == 1 ) uiddb = _prevuserDB;
	else if ( req.session->replicType == 2 ) uiddb = _nextuserDB;

	//Jstr scdbobj = uid + "." + intToStr( req.session->replicType );

	int rc = 0;
	_objectLock->writeLockSchema( req.session->replicType );
	AbaxString dbpass = uiddb->getValue(uid, JAG_PASS );
	if ( dbpass.size() < 1 ) {
		rc = 0;
		req.session->spCommandReject = 0;
	} else {
		rc = -20;
		req.session->spCommandReject = 1;
	}

	_objectLock->writeUnlockSchema( req.session->replicType );
	req.session->spCommandReject = 0;
	return rc;
}

// 0: OK
int JagDBServer::createUserCommit( JagRequest &req, JagParseParam &parseParam, jagint threadQueryTime )
{
	req.session->spCommandReject = 0;
	Jstr uid = parseParam.uid;
	Jstr pass = parseParam.passwd;

	JagUserID *uiddb = NULL;

	if ( req.session->replicType == 0 ) uiddb = _userDB;
	else if ( req.session->replicType == 1 ) uiddb = _prevuserDB;
	else if ( req.session->replicType == 2 ) uiddb = _nextuserDB;

	//Jstr scdbobj = uid + "." + intToStr( req.session->replicType );

	_objectLock->writeLockSchema( req.session->replicType );

	char *md5 = MDString( pass.c_str() );
	Jstr mdpass = md5;
	if ( md5 ) free( md5 );

	JAG_BLURT jaguar_mutex_lock ( &g_dbschemamutex ); JAG_OVER
	if ( uiddb ) {
		uiddb->addUser(uid, mdpass, JAG_USER, JAG_WRITE );
	}
	jaguar_mutex_unlock ( &g_dbschemamutex );

	_objectLock->writeUnlockSchema( req.session->replicType );
	//schemaChangeCommandSyncRemove( scdbobj ); 
	req.session->spCommandReject = 0;
	return 0;
}


// dropuser uid
void JagDBServer::dropUser( JagRequest &req, JagParseParam &parseParam, jagint threadQueryTime )
{
	int rc = 0;
	if ( req.isPrepare() ) {
		rc = dropUserPrepare( req, parseParam, threadQueryTime );	
		if ( 0 == rc ) {
			sendDataEnd( req, "OK");
			req.session->spCommandReject = 0;
		} else {
			sendDataEnd( req, "NG");
			req.session->spCommandReject = 0;
		}
	} else {
		rc = dropUserCommit( req, parseParam, threadQueryTime );	
	}

	d("s303344 dropUser done rc=%d req.sqlhdr[2]=%c ...\n", rc, req.sqlhdr[2]);
}

// dropuser uid
// 0: good; <0 bad
int JagDBServer::dropUserPrepare( JagRequest &req, JagParseParam &parseParam, jagint threadQueryTime )
{
	d("s322065 dropUserPrepare threadQueryTime=%ld g_lastHostTime=%d\n", threadQueryTime, g_lastHostTime );
   	if ( threadQueryTime > 0 && threadQueryTime < g_lastHostTime ) {
		return -10;
	} else {
		return 0;
	}
}

// dropuser uid
// 0: good; <0 bad
int JagDBServer::dropUserCommit( JagRequest &req, JagParseParam &parseParam, jagint threadQueryTime )
{
	Jstr uid = parseParam.uid;
	JagUserID *uiddb = NULL;
	if ( req.session->replicType == 0 ) uiddb = _userDB;
	else if ( req.session->replicType == 1 ) uiddb = _prevuserDB;
	else if ( req.session->replicType == 2 ) uiddb = _nextuserDB;
	//Jstr scdbobj = uid + "." + intToStr( req.session->replicType );

	_objectLock->writeLockSchema( req.session->replicType );

	AbaxString dbpass = uiddb->getValue( uid, JAG_PASS );
	
	req.session->spCommandReject = 0;

	JAG_BLURT jaguar_mutex_lock ( &g_dbschemamutex ); JAG_OVER
	if ( uiddb ) {
		uiddb->dropUser( uid );
	}
	jaguar_mutex_unlock ( &g_dbschemamutex );

	_objectLock->writeUnlockSchema( req.session->replicType );

	return 0;
}

// changepass uid password
void JagDBServer::changePass( JagRequest &req, JagParseParam &parseParam, jagint threadQueryTime )
{
	int rc = 0;
	if ( req.isPrepare() ) {
		rc = changePassPrepare( req, parseParam, threadQueryTime );	
		if ( 0 == rc ) {
			sendDataEnd( req, "OK");
			req.session->spCommandReject = 0;
		} else {
			sendDataEnd( req, "NG");
			req.session->spCommandReject = 0;
		}
	} else {
		rc = changePassCommit( req, parseParam, threadQueryTime );	
	}

	d("s303344 changePass done rc=%d req.sqlhdr[2]=%c ...\n", rc, req.sqlhdr[2]);
}

// 0: good; <0 bad
int JagDBServer::changePassPrepare( JagRequest &req, JagParseParam &parseParam, jagint threadQueryTime )
{
	Jstr uid = parseParam.uid;
	Jstr pass = parseParam.passwd;

	JagUserID *uiddb = NULL;

	if ( threadQueryTime > 0 && threadQueryTime < g_lastHostTime ) {
		return -10;
	}

	if ( req.session->replicType == 0 ) uiddb = _userDB;
	else if ( req.session->replicType == 1 ) uiddb = _prevuserDB;
	else if ( req.session->replicType == 2 ) uiddb = _nextuserDB;

	//Jstr scdbobj = uid + "." + intToStr( req.session->replicType );

	_objectLock->writeLockSchema( req.session->replicType );

	AbaxString dbpass = uiddb->getValue( uid, JAG_PASS );
	if ( dbpass.size() < 1 ) {
		_objectLock->writeUnlockSchema( req.session->replicType );
		return -20;
	}
	
	_objectLock->writeUnlockSchema( req.session->replicType );

	return 0;
}

// 0: good; <0 bad
int JagDBServer::changePassCommit( JagRequest &req, JagParseParam &parseParam, jagint threadQueryTime )
{
	Jstr uid = parseParam.uid;
	Jstr pass = parseParam.passwd;
	JagUserID *uiddb = NULL;

	if ( req.session->replicType == 0 ) uiddb = _userDB;
	else if ( req.session->replicType == 1 ) uiddb = _prevuserDB;
	else if ( req.session->replicType == 2 ) uiddb = _nextuserDB;

	//Jstr scdbobj = uid + "." + intToStr( req.session->replicType );

	_objectLock->writeLockSchema( req.session->replicType );

	req.session->spCommandReject = 0;
	char *md5 = MDString( pass.c_str() );
	Jstr mdpass = md5;
	if ( md5 ) free( md5 );

	JAG_BLURT jaguar_mutex_lock ( &g_dbschemamutex ); JAG_OVER
	if ( uiddb ) {
		uiddb->setValue( uid, JAG_PASS, mdpass );
	}
	jaguar_mutex_unlock ( &g_dbschemamutex );
	_objectLock->writeUnlockSchema( req.session->replicType );

	jd(JAG_LOG_LOW, "user [%s] changepass uid=[%s]\n", req.session->uid.c_str(),  parseParam.uid.c_str() );

	return 0;
}

// method to change dfdb ( use db command input by user, not via connection )
// changedb dbname
void JagDBServer::changeDB( JagRequest &req, JagParseParam &parseParam, jagint threadQueryTime )
{
	int rc = 0;
	if ( req.isPrepare() ) {
		rc = changeDBPrepare( req, parseParam, threadQueryTime );	
		if ( 0 == rc ) {
			sendDataEnd( req, "OK");
			req.session->spCommandReject = 0;
		} else {
			sendDataEnd( req, "NG");
			req.session->spCommandReject = 0;
		}
	} else {
		rc = changeDBCommit( req, parseParam, threadQueryTime );	
	}

	d("s303344 changeDB done rc=%d req.sqlhdr[2]=%c ...\n", rc, req.sqlhdr[2]);
}

// 0: good; <0 bad
int JagDBServer::changeDBPrepare( JagRequest &req, JagParseParam &parseParam, jagint threadQueryTime )
{
	if ( threadQueryTime > 0 && threadQueryTime < g_lastHostTime ) {
		return -10;
	}

	jagint lockrc;

	req.session->spCommandReject = 0;
	Jstr jagdatahome = _cfg->getJDBDataHOME( req.session->replicType );
    Jstr sysdir = jagdatahome + "/" + parseParam.dbName;
	//Jstr scdbobj = sysdir + "." + intToStr( req.session->replicType );

	int rc = -20;

	lockrc = _objectLock->readLockDatabase( parseParam.opcode, parseParam.dbName, req.session->replicType );
	if ( 0 == jagstrcmp( parseParam.dbName.c_str(), "test" ) || 0 == jagaccess( sysdir.c_str(), X_OK ) ) {
		if ( lockrc ) {
			req.session->spCommandReject = 0;
			rc = 0;
		}
	}

	if ( lockrc )  {
		_objectLock->readUnlockDatabase( parseParam.opcode, parseParam.dbName, req.session->replicType );
	}

	return rc;
}

int JagDBServer::changeDBCommit( JagRequest &req, JagParseParam &parseParam, jagint threadQueryTime )
{
	jagint lockrc;
	Jstr jagdatahome = _cfg->getJDBDataHOME( req.session->replicType );
    Jstr sysdir = jagdatahome + "/" + parseParam.dbName;
	//Jstr scdbobj = sysdir + "." + intToStr( req.session->replicType );

	lockrc = _objectLock->readLockDatabase( parseParam.opcode, parseParam.dbName, req.session->replicType );

	req.session->dbname = parseParam.dbName;
	if ( lockrc )  {
		_objectLock->readUnlockDatabase( parseParam.opcode, parseParam.dbName, req.session->replicType );
	}

	return 0;
}

// createdb dbname
void JagDBServer::createDB( JagRequest &req, JagParseParam &parseParam, jagint threadQueryTime )
{
	int rc = 0;
	if ( req.isPrepare() ) {
		rc = createDBPrepare( req, parseParam, threadQueryTime );	
		if ( 0 == rc ) {
			sendDataEnd( req, "OK");
			req.session->spCommandReject = 0;
		} else {
			sendDataEnd( req, "NG");
			req.session->spCommandReject = 0;
		}
	} else {
		rc = createDBCommit( req, parseParam, threadQueryTime );	
	}

	d("s303344 createDB done rc=%d req.sqlhdr[2]=%c ...\n", rc, req.sqlhdr[2]);
}

int JagDBServer::createDBPrepare( JagRequest &req, JagParseParam &parseParam, jagint threadQueryTime )
{
	Jstr jagdatahome = _cfg->getJDBDataHOME( req.session->replicType );
    Jstr sysdir = jagdatahome + "/" + parseParam.dbName;

	if ( threadQueryTime > 0 && threadQueryTime < g_lastHostTime ) {
		return -10;
	}

	int rc = 0;
	if ( JagFileMgr::isDir( sysdir ) ) {
		rc = -20;
	}

	return rc;
}

// 0: OK; <0 bad
int JagDBServer::createDBCommit( JagRequest &req, JagParseParam &parseParam, jagint threadQueryTime )
{
	jagint lockrc;

	Jstr jagdatahome = _cfg->getJDBDataHOME( req.session->replicType );
    Jstr sysdir = jagdatahome + "/" + parseParam.dbName;
	//Jstr scdbobj = sysdir + "." + intToStr( req.session->replicType );

	lockrc = _objectLock->writeLockDatabase( parseParam.opcode, parseParam.dbName, req.session->replicType );

	req.session->spCommandReject = 0;

    jagmkdir( sysdir.c_str(), 0700 );

	if ( lockrc ) {
		_objectLock->writeUnlockDatabase( parseParam.opcode, parseParam.dbName, req.session->replicType );
	}

	jd(JAG_LOG_LOW, "user [%s] create database [%s]\n", req.session->uid.c_str(),  parseParam.dbName.c_str() );
	return 0;
}

// dropdb dbname
void JagDBServer::dropDB( JagRequest &req, JagParseParam &parseParam, jagint threadQueryTime )
{
	int rc = 0;
	if ( req.isPrepare() ) {
		rc = dropDBPrepare( req, parseParam, threadQueryTime );	
		if ( 0 == rc ) {
			sendDataEnd( req, "OK");
			req.session->spCommandReject = 0;
		} else {
			sendDataEnd( req, "NG");
			req.session->spCommandReject = 0;
		}
	} else {
		rc = dropDBCommit( req, parseParam, threadQueryTime );	
	}

	d("s303344 dropUser done rc=%d req.sqlhdr[2]=%c ...\n", rc, req.sqlhdr[2]);

}

// 0: OK; <0 bad
int JagDBServer::dropDBPrepare( JagRequest &req, JagParseParam &parseParam, jagint threadQueryTime )
{
	if ( threadQueryTime > 0 && threadQueryTime < g_lastHostTime ) {
		return -10;
	}

	jagint lockrc;
	Jstr jagdatahome = _cfg->getJDBDataHOME( req.session->replicType );
    Jstr sysdir = jagdatahome + "/" + parseParam.dbName;
	//Jstr scdbobj = sysdir + "." + intToStr( req.session->replicType );

	int rc = -20;
	lockrc = _objectLock->writeLockDatabase( parseParam.opcode, parseParam.dbName, req.session->replicType );
	if ( JagFileMgr::isDir( sysdir ) > 0 && lockrc ) {
		rc = 0;
	} 
	

	if ( parseParam.hasForce ) {
		rc = 0;
	}

	// when doing dropDB, createIndexLock && tableUsingLock needs to be locked to protect tables
	if ( lockrc ) {
		_objectLock->writeUnlockDatabase( parseParam.opcode, parseParam.dbName, req.session->replicType );
	}

	return rc;
}

int JagDBServer::dropDBCommit( JagRequest &req, JagParseParam &parseParam, jagint threadQueryTime )
{
	jagint lockrc;
	Jstr jagdatahome = _cfg->getJDBDataHOME( req.session->replicType );
    Jstr sysdir = jagdatahome + "/" + parseParam.dbName;
	//Jstr scdbobj = sysdir + "." + intToStr( req.session->replicType );

	lockrc = _objectLock->writeLockDatabase( parseParam.opcode, parseParam.dbName, req.session->replicType );
	
	// when doing dropDB, createIndexLock && tableUsingLock needs to be locked to protect tables
	req.session->spCommandReject = 0;
	JagTableSchema *tableschema = getTableSchema( req.session->replicType );

	// drop all tables and indexes under this database
	dropAllTablesAndIndexUnderDatabase( req, tableschema, parseParam.dbName );

    JagFileMgr::rmdir( sysdir );
	refreshSchemaInfo( req.session->replicType, g_lastSchemaTime );

	if ( lockrc ) {
		_objectLock->writeUnlockDatabase( parseParam.opcode, parseParam.dbName, req.session->replicType );
	}

	if ( parseParam.hasForce ) {
		jd(JAG_LOG_LOW, "user [%s] force dropped database [%s]\n", req.session->uid.c_str(),  parseParam.dbName.c_str() );
	} else {
		jd(JAG_LOG_LOW, "user [%s] dropped database [%s]\n", req.session->uid.c_str(),  parseParam.dbName.c_str() );
	}

	return 0;
}

// create table schema
// return 1: OK   0: error
int JagDBServer::createTable( JagRequest &req, const Jstr &dbname, 
							  JagParseParam *parseParam, Jstr &reterr, jagint threadQueryTime )
{
	int rc = 0;
	if ( req.isPrepare() ) {
		dn("s300218 createTablePrepare...");
		rc = createTablePrepare( req, dbname, parseParam, threadQueryTime );	
		jagint crc;
		if ( 0 == rc ) {
			crc = sendDataEnd( req, "OK");
			dn("s30309001 sendMessageLength OK crc=%d", crc );
			req.session->spCommandReject = 0;
		} else {
			crc = sendDataEnd( req, "NG");
			dn("s30309001 sendMessageLength NG crc=%d", crc );
			req.session->spCommandReject = 0;
		}
	} else {
		dn("s300218 createTableCommit...");
		rc = createTableCommit( req, dbname, parseParam, reterr, threadQueryTime );	
	}
	d("s303344 createTable prepare/commit done rc=%d req.sqlhdr[2]=%c ...\n", rc, req.sqlhdr[2]);

	if ( 0 == rc ) return 1;
	else return 0;
}

// 0: good; <0: bad
int JagDBServer::createTablePrepare( JagRequest &req, const Jstr &dbname, 
							  		 JagParseParam *parseParam, jagint threadQueryTime )
{
   	if ( threadQueryTime > 0 && threadQueryTime < g_lastHostTime ) {
		return -10;
   	}

	jagint lockrc;
	req.session->spCommandReject = 0;
	Jstr dbtable = dbname + "." + parseParam->objectVec[0].tableName;
	//Jstr scdbobj = dbtable + "." + intToStr( req.session->replicType );

	// for createtable, write lock db first, insert schema then lock table

	JagTableSchema *tableschema;
	JagIndexSchema *indexschema;
	getTableIndexSchema( req.session->replicType, tableschema, indexschema );

	lockrc = _objectLock->writeLockDatabase( parseParam->opcode, dbname, req.session->replicType );

   	bool found1 = false, found2 = false;
	int rc = 0;

	while ( true ) {
    	found1 = indexschema->tableExist( dbname, parseParam );
    	if ( found1 ) {
			rc = -20;
			break;
    	}

    	found2 = tableschema->existAttr( dbtable );
    	if ( found2 ) {
			rc = -30;
			break;
    	}

		break;
	}

	if ( lockrc ) {
		_objectLock->writeUnlockDatabase( parseParam->opcode, dbname, req.session->replicType );
	}

    if ( 0 == rc ) {
	    jd(JAG_LOG_LOW, "user [%s] create table prepare [%s] OK\n", req.session->uid.c_str(), dbtable.c_str() );		
    } else {
	    jd(JAG_LOG_LOW, "user [%s] create table prepare [%s] error rc=%d\n", req.session->uid.c_str(), dbtable.c_str(), rc );		
    }

	return rc;
}

// 0: good; <0 bad
int JagDBServer::createTableCommit( JagRequest &req, const Jstr &dbname, 
							        JagParseParam *parseParam, Jstr &reterr, jagint threadQueryTime )
{
	int repType =  req.session->replicType;
	JagTableSchema *tableschema;
	JagIndexSchema *indexschema;
	getTableIndexSchema(  req.session->replicType, tableschema, indexschema );
	if ( JAG_CREATECHAIN_OP == parseParam->opcode ) {
		parseParam->isChainTable = 1;
	}  else {
		parseParam->isChainTable = 0;
	}

	jagint lockrc;
	Jstr table = parseParam->objectVec[0].tableName;
	Jstr dbtable = dbname + "." + table;
	//Jstr scdbobj = dbtable + "." + intToStr( repType );

	// for createtable, write lock db first, insert schema then lock table
	lockrc = _objectLock->writeLockDatabase( parseParam->opcode, dbname, repType );
	
	req.session->spCommandReject = 0;

	JAG_BLURT jaguar_mutex_lock ( &g_dbschemamutex ); JAG_OVER
	tableschema->insert( parseParam );
	refreshSchemaInfo( repType, g_lastSchemaTime );
	jaguar_mutex_unlock ( &g_dbschemamutex );

	// create table object 
	bool ok = 0;
	int trc;
	if ( _objectLock->writeLockTable( parseParam->opcode, dbname, table, tableschema, repType, 1, trc ) ) {

		dn("s600218 writeLockTable() OK ptab %s created", dbtable.s() );
		ok = true;
		_objectLock->writeUnlockTable( parseParam->opcode, dbname, table, repType, 1 );
	} else {
		dn("s600219 writeLockTable() error ptab not created, lockrc=%d", trc);
	}

	if ( lockrc ) {
		_objectLock->writeUnlockDatabase( parseParam->opcode, dbname, repType );
	}

	// schemaChangeCommandSyncRemove( scdbobj );
	if ( parseParam->isChainTable ) {
		jd(JAG_LOG_LOW, "user [%s] create chain [%s] reptype=%d ok=%d\n", 
				req.session->uid.c_str(), dbtable.c_str(), repType, ok );
	} else {
		jd(JAG_LOG_LOW, "user [%s] create table [%s] reptype=%d ok=%d\n", 
				req.session->uid.c_str(), dbtable.c_str(), repType, ok );
	}

	return 0;
}

// create memtable schema
// 1: OK 0: bad
int JagDBServer::createMemTable( JagRequest &req, const Jstr &dbname, 
							     JagParseParam *parseParam, Jstr &reterr, jagint threadQueryTime )
{
	int rc = 0;
	if ( req.isPrepare() ) {
		rc = createMemTablePrepare( req, dbname, parseParam, threadQueryTime );	
		if ( 0 == rc ) {
			sendDataEnd( req, "OK");
			req.session->spCommandReject = 0;
		} else {
			sendDataEnd( req, "NG");
			req.session->spCommandReject = 0;
		}
	} else {
		rc = createMemTableCommit( req, dbname, parseParam, reterr, threadQueryTime );	
	}
	d("s303344 createTable done rc=%d req.sqlhdr[2]=%c ...\n", rc, req.sqlhdr[2]);

	if ( 0 == rc ) return 1;
	else return 0;

}

// 0: ok, <0 bad
int JagDBServer::createMemTablePrepare( JagRequest &req, const Jstr &dbname, 
							     JagParseParam *parseParam, jagint threadQueryTime )
{
	if ( threadQueryTime > 0 && threadQueryTime < g_lastHostTime ) {
		return -10;
	}

	JagTableSchema *tableschema;
	JagIndexSchema *indexschema;
	getTableIndexSchema(  req.session->replicType, tableschema, indexschema );
	jagint lockrc;

	int repType = req.session->replicType;
	Jstr dbtable = dbname + "." + parseParam->objectVec[0].tableName;
	//Jstr scdbobj = dbtable + "." + intToStr( repType );

	parseParam->isMemTable = 1;
	lockrc = _objectLock->writeLockDatabase( parseParam->opcode, dbname, repType );
	if ( ! lockrc ) {
		return -20;
	}

	int rc = -30;

	bool found = indexschema->tableExist( dbname, parseParam );
	bool found2 = tableschema->existAttr( dbtable );
	if ( !found && !found2 && lockrc ) {
		rc = 0;
	}

	if ( lockrc ) {
		_objectLock->writeUnlockDatabase( parseParam->opcode, dbname, repType );
	}

	return rc;
}

// 0: ok
// not supported
int JagDBServer::createMemTableCommit( JagRequest &req, const Jstr &dbname, 
							     JagParseParam *parseParam, Jstr &reterr, jagint threadQueryTime )
{
	JagTableSchema *tableschema;
	JagIndexSchema *indexschema;
	getTableIndexSchema(  req.session->replicType, tableschema, indexschema );

	jagint lockrc;
	int repType = req.session->replicType;
	Jstr dbtable = dbname + "." + parseParam->objectVec[0].tableName;
	//Jstr scdbobj = dbtable + "." + intToStr( req.session->replicType );

	parseParam->isMemTable = 1;
	lockrc = _objectLock->writeLockDatabase( parseParam->opcode, dbname, repType );

	req.session->spCommandReject = 0;

	JAG_BLURT jaguar_mutex_lock ( &g_dbschemamutex ); JAG_OVER
	tableschema->insert( parseParam );
	refreshSchemaInfo( repType, g_lastSchemaTime );
	jaguar_mutex_unlock ( &g_dbschemamutex );

	if ( lockrc ) {
		_objectLock->writeUnlockDatabase( parseParam->opcode, dbname, repType );
	}

	jd(JAG_LOG_LOW, "user [%s] create memtable [%s]\n", req.session->uid.c_str(), dbtable.c_str() );
	return 0;
}

//  return 0: error with reterr;  1: success
int JagDBServer::createIndex( JagRequest &req, const Jstr &dbname, JagParseParam *parseParam,
							  JagTable *&ptab, JagIndex *&pindex, Jstr &reterr, jagint threadQueryTime )
{
	int rc = 0;
	if ( req.isPrepare() ) {
		rc = createIndexPrepare( req, dbname, parseParam, threadQueryTime );	
		d("s333001 createIndexPrepare rc=%d sendMessageLength\n", rc);
		if ( 0 == rc ) {
			sendDataEnd( req, "OK");
			req.session->spCommandReject = 0;
		} else {
			sendDataEnd( req, "NG");
			req.session->spCommandReject = 0;
		}
		d("s333001 createIndexPrepare rc=%d sendMessageLength done\n", rc);
	} else {
		d("s0333880 createIndexCommit() ...\n");
		rc = createIndexCommit( req, dbname, parseParam, ptab, pindex, reterr, threadQueryTime );	
		d("s0333880 createIndexCommit() done\n");
	}
	d("s303344 createIndex done rc=%d req.sqlhdr[2]=%c ...\n", rc, req.sqlhdr[2]);

	if ( 0 == rc ) return 1;
	else return 0;
}

// 0: ok ; <0 bad
int JagDBServer::createIndexPrepare( JagRequest &req, const Jstr &dbname, JagParseParam *parseParam,
							  	     jagint threadQueryTime )
{
	if ( threadQueryTime > 0 && threadQueryTime < g_lastHostTime ) {
		return -10;
	}

	JagIndexSchema *indexschema; 
	JagTableSchema *tableschema;
	getTableIndexSchema( req.session->replicType, tableschema, indexschema );

	int lockrc;

	// the table has same index ?
	Jstr tgttab = indexschema->getTableNameScan( dbname, parseParam->objectVec[1].indexName );
	d("s838394 createIndexPrepare tgttab=[%s]\n", tgttab.c_str() );
	if ( tgttab.size() > 0 ) {
		return -20;
	}

	dn("s8330100 p.opcode=%d p.dbName=[%s] tableName=[%s] replicType=%d", 
		parseParam->opcode, parseParam->objectVec[0].dbName.s(), parseParam->objectVec[0].tableName.s(), req.session->replicType );

	JagTable *ptab;
	ptab = _objectLock->writeLockTable( JAG_NOOP, parseParam->objectVec[0].dbName, 
									    parseParam->objectVec[0].tableName, tableschema, req.session->replicType, 0, lockrc );

	if ( ! ptab ) {
		return -30;
	}

	_objectLock->writeUnlockTable( JAG_NOOP, parseParam->objectVec[0].dbName, 
								   parseParam->objectVec[0].tableName, req.session->replicType, 0 );

	return 0;
}

// 0: ok; <0 bad
int JagDBServer::createIndexCommit( JagRequest &req, const Jstr &dbname, JagParseParam *parseParam,
							  JagTable *&ptab, JagIndex *&pindex, Jstr &reterr, jagint threadQueryTime )
{
	JagIndexSchema *indexschema; 
	JagTableSchema *tableschema;
	getTableIndexSchema(  req.session->replicType, tableschema, indexschema );
	Jstr dbindex = dbname + "." + parseParam->objectVec[0].tableName + 
							 "." + parseParam->objectVec[1].indexName;

	int rc = -1;
	int lockrct;
	int lockrci;

	Jstr dbobj = parseParam->objectVec[0].dbName + "." + parseParam->objectVec[0].tableName;
	Jstr scdbobj = dbobj + "." + intToStr( req.session->replicType );

	d("s003838 createIndexCommit writeLockTable...\n");
	ptab = _objectLock->writeLockTable( parseParam->opcode, parseParam->objectVec[0].dbName, 
										 parseParam->objectVec[0].tableName, tableschema, req.session->replicType, 0, lockrct );
	d("s003838 createIndexCommit writeLockTable done lockrct=%d\n", lockrct);

	if ( ptab ) {
		d("s003838 createIndexCommit createIndexSchema ...\n");
		createIndexSchema( req, dbname, parseParam, reterr, true );
		d("s003838 createIndexCommit createIndexSchema done\n");
	} else {
		d("s038238 createIndexCommit writeLockTable failed, lockrct=%d Tabled not found\n", lockrct);
		pindex = NULL;
		return -1;
	}

	req.session->spCommandReject = 0;

	d("s222309 g_dbschemamutex lock...\n");
	JAG_BLURT jaguar_mutex_lock ( &g_dbschemamutex ); JAG_OVER
	d("s222309 g_dbschemamutex lock done refreshSchemaInfo() ...\n");
	refreshSchemaInfo( req.session->replicType, g_lastSchemaTime );
	jaguar_mutex_unlock ( &g_dbschemamutex );
	d("s222309 g_dbschemamutex unlock done...\n");

	pindex = _objectLock->writeLockIndex( parseParam->opcode, parseParam->objectVec[1].dbName,
										   parseParam->objectVec[0].tableName, parseParam->objectVec[1].indexName,
										   tableschema, indexschema, req.session->replicType, 1, lockrci );

	d("s222309 writeLockIndex done. ptab=%p pindex=%p\n", ptab, pindex);

	if ( pindex ) {
		// if successfully create index, begin process table's data
		doCreateIndex( ptab, pindex );
		rc = 0;
	} else {
		JAG_BLURT jaguar_mutex_lock ( &g_dbschemamutex ); JAG_OVER
	    indexschema->remove( dbindex );
		jaguar_mutex_unlock ( &g_dbschemamutex );
		req.session->spCommandReject = 0;
		rc = -10;
	}
	d("s222339 doCreateIndex done\n");

	if ( ptab ) {
        // must be true
		_objectLock->writeUnlockTable( parseParam->opcode, parseParam->objectVec[0].dbName, 
									   parseParam->objectVec[0].tableName, req.session->replicType, 0 );
	}

	if ( pindex ) {
		_objectLock->writeUnlockIndex( parseParam->opcode, parseParam->objectVec[1].dbName,
										parseParam->objectVec[0].tableName, parseParam->objectVec[1].indexName,
										req.session->replicType, 1 );		
	}

	d("s222339 writeUnlockTable writeUnlockIndex done\n");
	jd(JAG_LOG_LOW, "user [%s] create index [%s][%d]\n", req.session->uid.c_str(), scdbobj.c_str(), rc );
	return rc;
}

// 1: OK   0: error
int JagDBServer
::alterTable( const JagParseAttribute &jpa, JagRequest &req, const Jstr &dbname,
			   const JagParseParam *parseParam, Jstr &reterr, jagint threadQueryTime, 
			   jagint &threadSchemaTime )
{

	int rc = 0;
	if ( req.isPrepare() ) {
        dn("s90115 alterTablePrepare ...");
		rc = alterTablePrepare( jpa, req, dbname, parseParam, threadQueryTime );	
		d("s333001 alterTablePrepare rc=%d sendMessageLength\n", rc);
		if ( 0 == rc ) {
			sendDataEnd( req, "OK");
			req.session->spCommandReject = 0;
		} else {
			sendDataEnd( req, "NG");
			req.session->spCommandReject = 0;
		}
		d("s333001 alterTablePrepare rc=%d sendMessageLength done\n", rc);
	} else {
		d("s0333880 alterTableCommit() ...\n");
		rc = alterTableCommit( jpa, req, dbname, parseParam, reterr, threadQueryTime, threadSchemaTime );	
		d("s0333880 alterTableCommit() done\n");
	}
	d("s303344 alterTable done rc=%d req.sqlhdr[2]=%c ...\n", rc, req.sqlhdr[2]);

	if ( 0 == rc ) return 1;
	else return 0;
}

// 0: ok; <0 error
int JagDBServer
::alterTablePrepare( const JagParseAttribute &jpa, JagRequest &req, const Jstr &dbname,
			   const JagParseParam *parseParam, jagint threadQueryTime ) 
{
	if ( threadQueryTime > 0 && threadQueryTime < g_lastHostTime ) {
		return -10;
	}

	Jstr dbName = parseParam->objectVec[0].dbName;
	Jstr tableName = parseParam->objectVec[0].tableName;
	int  replicType = req.session->replicType;
	int  opcode =  parseParam->opcode;

	JagTableSchema *tableschema = getTableSchema( replicType );
	JagTable *ptab = NULL;
	req.session->spCommandReject = 0;
	Jstr dbtable = dbName + "." + tableName;
	Jstr scdbobj = dbtable + "." + intToStr( replicType );

	int lockrc;
	ptab = _objectLock->writeLockTable( opcode, dbName, tableName, tableschema, replicType, 0, lockrc );
	if ( !ptab ) {
		return -20;
	}

	if ( ! tableschema->existAttr( dbtable ) ) {
		_objectLock->writeUnlockTable( opcode, dbName, tableName, replicType, 0 );
		return -30;
	}
	
	// for add column command, if new column's length is larger than spare_ remains length, reject
	if ( parseParam->createAttrVec.size() > 0 ) {
		if ( !tableschema->checkSpareRemains( dbtable, parseParam ) ) {
			_objectLock->writeUnlockTable( opcode, dbName, tableName, replicType, 0 );
			return -40;
		}
	}

	_objectLock->writeUnlockTable( opcode, dbName, tableName, replicType, 0 );
	req.session->spCommandReject = 0;
	return 0;
}

// 0: OK; <0 error
int JagDBServer
::alterTableCommit( const JagParseAttribute &jpa, JagRequest &req, const Jstr &dbname,
			   const JagParseParam *parseParam, Jstr &reterr, jagint threadQueryTime, 
			   jagint &threadSchemaTime )
{
    dn("s761229760 alterTableCommit() ");

	Jstr dbName = parseParam->objectVec[0].dbName;
	Jstr tableName = parseParam->objectVec[0].tableName;
	int replicType = req.session->replicType;
	int opcode =  parseParam->opcode;
	bool brc;

	JagTableSchema *tableschema = getTableSchema( replicType );
	JagTable *ptab = NULL;
	req.session->spCommandReject = 0;
	Jstr dbtable = dbName + "." + tableName;
	Jstr scdbobj = dbtable + "." + intToStr( replicType );

	int  lockrc;
	ptab = _objectLock->writeLockTable( opcode, dbName, tableName, tableschema, replicType, 0, lockrc );
	if ( ! ptab ) {
		d("s55550081 ptab NULL return 0\n");
		req.session->spCommandReject = 1;
		return 0;
	}
	
	JAG_BLURT jaguar_mutex_lock ( &g_dbschemamutex ); JAG_OVER	

	bool hasChange = false;
	Jstr sql, normalizedTser;

	if ( ptab ) {
		if ( parseParam->cmd == JAG_SCHEMA_ADD_COLUMN || parseParam->cmd == JAG_SCHEMA_RENAME_COLUMN ) {
			brc = tableschema->addOrRenameColumn( dbtable, parseParam );
			if ( brc ) {
				hasChange = true;
			} else {
				reterr = "E12302 error add or rename table column"; 
			}
		} else if ( parseParam->cmd == JAG_SCHEMA_SET ) {
			brc = tableschema->setColumn( dbtable, parseParam );
			if ( brc ) {
				hasChange = true;
			} else {
				reterr = "E12303 error setting table column property"; 
			}
		} else if ( parseParam->cmd == JAG_SCHEMA_ADD_TICK && ptab->hasTimeSeries() ) {

			normalizedTser = JagSchemaRecord::translateTimeSeries( parseParam->value );
			normalizedTser = JagSchemaRecord::makeTickPair( normalizedTser );
			sql = describeTable( JAG_TABLE_TYPE, req, _tableschema, dbtable, false, true, true, normalizedTser );
			JagParser parser((void*)this);
			JagParseParam pparam2( &parser );
			if ( parser.parseCommand( jpa, sql, &pparam2, reterr ) ) {

				bool crc = createSimpleTable( req, dbname, &pparam2 );

				if ( crc ) {
					bool arc = tableschema->addTick( dbtable, normalizedTser );
					if ( arc ) {
						hasChange = true;
					} else {
						jd(JAG_LOG_LOW, "E20221 Error: [%s] addTick[%s]\n", sql.s(), normalizedTser.s() );
						reterr = "E13214 error adding tick";
					}
				} else {
					reterr = "E13215 error adding tick table";
				}
			} else {
				jd(JAG_LOG_LOW, "E20121 Error: parse [%s] [%s]\n", sql.s(), reterr.s() );
				reterr = Jstr("E13216 error parsing command ") + sql + " " + reterr;
			}
		} else if ( parseParam->cmd == JAG_SCHEMA_DROP_TICK && ptab->hasTimeSeries() ) {
            dn("s601128 JAG_SCHEMA_DROP_TICK ...");
			normalizedTser = JagSchemaRecord::translateTimeSeries( parseParam->value );
			JagStrSplit ss(normalizedTser, '_');
			Jstr rollTab = ss[0];
			Jstr sql = Jstr("drop table ") + dbtable + "@" + rollTab;
			JagParser parser((void*)this);
			JagParseParam pparam2( &parser );
            
            dn("s706161 sql=[%s] parse ... ", sql.s() );

			if ( parser.parseCommand( jpa, sql, &pparam2, reterr ) ) {
                dn("s716161 sql=[%s] parse OK", sql.s() );

				bool crc = dropSimpleTable( req, &pparam2, reterr, false );
				if ( crc ) {
					bool arc = tableschema->dropTick( dbtable, normalizedTser );
					if ( arc ) {
						hasChange = true;
					} else {
						jd(JAG_LOG_LOW, "E20133 Error: [%s] dropTick(%s)\n", sql.s(), normalizedTser );
						reterr = "E13217 error dropping tick";
					}
				} else {
					reterr = "E13218 error dropping tick table";
				}
			} else {
				jd(JAG_LOG_LOW, "E20123 Error: parse [%s] [%s]\n", sql.s(), reterr.s() );
				reterr = "E13219 error parsing command";
			}

            dn("s1180929 JAG_SCHEMA_DROP_TICK done reterr=[%s] hasChange=%d", reterr.s(), hasChange );

		} else if ( parseParam->cmd == JAG_SCHEMA_CHANGE_RETENTION ) {
			bool arc = tableschema->changeRetention( dbtable, parseParam->value );
			if ( arc ) {
				hasChange = true;

				if ( strchr( dbtable.s(), '@' ) ) {
					JagStrSplit sp( dbtable, '@');
					Jstr parentTable = sp[0];
					Jstr tick =  sp[1];
					tableschema->changeTickRetention( parentTable, tick, parseParam->value );
				}

			} else {
				jd(JAG_LOG_LOW, "E20153 Error: change retention(%s)\n", parseParam->value.s() );
				reterr = "E13220 error change retention";
			}
		}

		if ( hasChange ) {
			ptab->refreshSchema();  // including tableRecord is changed
		} 
	}

	if ( parseParam->createAttrVec.size() < 1 && hasChange ) {
		if ( ptab ) {
			if ( parseParam->cmd == JAG_SCHEMA_ADD_COLUMN || parseParam->cmd == JAG_SCHEMA_RENAME_COLUMN ) {
				ptab->renameIndexColumn( parseParam, reterr );
			} else if (  parseParam->cmd == JAG_SCHEMA_SET ) {
				ptab->setIndexColumn( parseParam, reterr );
			} else if ( parseParam->cmd == JAG_SCHEMA_ADD_TICK && ptab->hasTimeSeries() ) {
				JagTable *newPtab; JagIndex *newPindex;
				const JagVector<Jstr> &indexVec = ptab->getIndexes();
				Jstr  sql, indexName;
				for ( int i = 0; i < indexVec.size(); ++i ) {
					indexName = indexVec[i];
               		sql = describeIndex( false, req, _indexschema, dbname, indexName, reterr, true, true, normalizedTser );
               		sql.replace('\n', ' ');
               		JagParser parser((void*)this);
               		JagParseParam pparam2( &parser );
               		if ( parser.parseCommand( jpa, sql, &pparam2, reterr ) ) {
               			bool crc = createSimpleIndex( req, &pparam2, newPtab, newPindex, reterr );
               			if ( ! crc ) {
               				jd(JAG_LOG_LOW, "E13271 Error: creating timeseries index [%s]\n", sql.s() );
               			} else {
               				jd(JAG_LOG_LOW, "OK13213 OK: creating timeseries index [%s]\n", sql.s() );
               			}
               		} else {
               			jd(JAG_LOG_LOW, "E12211 Error: parse [%s] [%s]\n", sql.s(), reterr.s() );
               		}
        
        		}
			} else if ( parseParam->cmd == JAG_SCHEMA_DROP_TICK && ptab->hasTimeSeries() ) {
				JagStrSplit ss(normalizedTser, '_');
				Jstr rollTab = ss[0];
				Jstr parentTableName = tableName;
				const JagVector<Jstr> &indexVec = ptab->getIndexes();
				Jstr  sql, parentIndexName;
				for ( int i = 0; i < indexVec.size(); ++i ) {
					parentIndexName = indexVec[i];
					sql = Jstr("drop index ") + parentIndexName + "@" + rollTab + " on " + parentTableName + "@" + rollTab;

               		sql.replace('\n', ' ');
               		JagParser parser((void*)this);
               		JagParseParam pparam2( &parser );
               		if ( parser.parseCommand( jpa, sql, &pparam2, reterr ) ) {
						bool crc = dropSimpleIndex( req, &pparam2, reterr, false );
               			if ( ! crc ) {
               				// jd(JAG_LOG_LOW, "E13281 Error: dropSimpleIndex [%s]\n", sql.s() );
               			} else {
               				jd(JAG_LOG_LOW, "OK13413 OK: dropSimpleIndex [%s]\n", sql.s() );
               			}
               		} else {
               			jd(JAG_LOG_LOW, "E12241 Error: parse [%s] [%s]\n", sql.s(), reterr.s() );
               		}
        
        		}
			}
		}
	}

	if ( hasChange ) {
		refreshSchemaInfo( replicType, g_lastSchemaTime );
	}

	jaguar_mutex_unlock ( &g_dbschemamutex );

	if ( ptab ) {
		_objectLock->writeUnlockTable( opcode, dbName, tableName, replicType, 0 );
	}

	if ( hasChange ) {
        /***
		if ( !req.session->origserv && !_restartRecover ) {
			dn("sc342209 broadcastSchemaToClients() ...");
			//broadcastSchemaToClients();
		}
        ***/

		threadSchemaTime = g_lastSchemaTime;
	} 

	if ( hasChange ) {
		return 0;
	} else {
		jd(JAG_LOG_LOW, "user [%s] alter table [%s] nochange\n", 
				  req.session->uid.c_str(), dbtable.c_str() );		
		return 0;
	}
}

// return 1: OK  0: error
int JagDBServer::dropTable( JagRequest &req, JagParseParam *parseParam, Jstr &reterr, jagint threadQueryTime, Jstr &timeSeries )
{
	int rc;
	if ( req.isPrepare() ) {
		rc = dropTablePrepare( req, parseParam, threadQueryTime );	
		if ( 0 == rc ) {
			sendDataEnd( req, "OK");
			req.session->spCommandReject = 0;
		} else {
			sendDataEnd( req, "NG");
			req.session->spCommandReject = 0;
		}
	} else {
		rc = dropTableCommit( req, parseParam, reterr, threadQueryTime, timeSeries );	
	}

	d("s303344 dropTable done rc=%d req.sqlhdr[2]=%c ...\n", rc, req.sqlhdr[2]);
	if ( 0 == rc ) return 1;
	else return 0;
}

// return 0: OK  <0: error
int JagDBServer::dropTablePrepare( JagRequest &req, JagParseParam *parseParam, jagint threadQueryTime )
{
   	if ( threadQueryTime > 0 && threadQueryTime < g_lastHostTime ) {
		return -10;
   	}

	Jstr dbname =  parseParam->objectVec[0].dbName;
	req.session->spCommandReject = 0;
	Jstr dbtable = dbname + "." + parseParam->objectVec[0].tableName;
	Jstr scdbobj = dbtable + "." + intToStr( req.session->replicType );
	Jstr tabname =  parseParam->objectVec[0].tableName;

	JagTableSchema *tableschema;
	JagIndexSchema *indexschema;
	getTableIndexSchema( req.session->replicType, tableschema, indexschema );

	jagint lockrc = _objectLock->writeLockDatabase( parseParam->opcode, dbname, req.session->replicType );
	if ( ! lockrc ) {
		req.session->spCommandReject = 0;
		return -20;
	}

	int rc = 0;
   	bool found = tableschema->existAttr( dbtable );
   	if ( ! found ) {
		req.session->spCommandReject = 1;
		rc = -30;
   	}

	if ( lockrc ) {
		_objectLock->writeUnlockDatabase( parseParam->opcode, dbname, req.session->replicType );
	}

	jd(JAG_LOG_LOW, "user [%s] drop table prepare [%s] rc=%d\n", req.session->uid.c_str(), dbtable.c_str(), rc );		
	return rc;
}

// return 0: OK;  <0: error
int JagDBServer::dropTableCommit( JagRequest &req, JagParseParam *parseParam, Jstr &reterr, jagint threadQueryTime, Jstr &timeSeries )
{
	Jstr dbname =  parseParam->objectVec[0].dbName;
	Jstr tabname =  parseParam->objectVec[0].tableName;
	Jstr dbobj = dbname + "." + tabname;
	Jstr scdbobj = dbobj + "." + intToStr( req.session->replicType );

	JagTableSchema *tableschema;
	JagIndexSchema *indexschema;
	getTableIndexSchema( req.session->replicType, tableschema, indexschema );

	req.session->spCommandReject = 0;
	int lockrc;

	JagTable *ptab = _objectLock->writeLockTable( parseParam->opcode, dbname, tabname, tableschema, req.session->replicType, 0, lockrc );	
	if ( ! ptab ) {
		jd(JAG_LOG_LOW, "user [%s] drop table commit [%s], table not found\n", 
				  req.session->uid.c_str(), dbobj.c_str() );		
		return -10;
	}

	req.session->spCommandReject = 0;
	if ( ptab ) {
		ptab->hasTimeSeries( timeSeries );
		ptab->drop( reterr ); 
	}

	JAG_BLURT jaguar_mutex_lock ( &g_dbschemamutex ); JAG_OVER
	tableschema->remove( dbobj );
	refreshSchemaInfo( req.session->replicType, g_lastSchemaTime );
	jaguar_mutex_unlock ( &g_dbschemamutex );
	
	if ( ptab ) {
		_objectLock->writeUnlockTable( parseParam->opcode, dbname, tabname, req.session->replicType, 0 );
		delete ptab; 
	}

	JAG_BLURT jaguar_mutex_lock ( &g_wallogmutex ); JAG_OVER
	Jstr fpath = _cfg->getWalLogHOME() + "/" + dbname + "." + tabname + ".wallog";
	jagunlink( fpath.s() );
	JAG_BLURT jaguar_mutex_unlock ( &g_wallogmutex ); 

	if ( parseParam->hasForce ) {
		jd(JAG_LOG_LOW, "user [%s] force drop table [%s]\n", req.session->uid.c_str(), dbobj.c_str() );		
	} else {
		jd(JAG_LOG_LOW, "user [%s] drop table [%s]\n", req.session->uid.c_str(), dbobj.c_str() );		
	}
	req.session->spCommandReject = 0;
	return 0;
}

// return 1: OK  0: error
int JagDBServer::dropIndex( JagRequest &req, const Jstr &dbname, 
							JagParseParam *parseParam, Jstr &reterr, jagint threadQueryTime, Jstr &timeSer )
{
	int rc;
	if ( req.isPrepare() ) {
		rc = dropIndexPrepare( req, dbname, parseParam, threadQueryTime );	
		if ( 0 == rc ) {
			sendDataEnd( req, "OK");
			req.session->spCommandReject = 0;
		} else {
			sendDataEnd( req, "NG");
			req.session->spCommandReject = 0;
		}
	} else {
		rc = dropIndexCommit( req, dbname, parseParam, reterr, threadQueryTime, timeSer );	
	}

	d("s303344 dropIndex done rc=%d req.sqlhdr[2]=%c ...\n", rc, req.sqlhdr[2]);
	if ( 0 == rc ) return 1;
	else return 0;
}

// 0: OK, <0; error
int JagDBServer::dropIndexPrepare( JagRequest &req, const Jstr &dbname, 
							JagParseParam *parseParam, jagint threadQueryTime )
{
	dn("s1229 dropIndexPrepare() ...");
	if ( threadQueryTime > 0 && threadQueryTime < g_lastHostTime ) {
		dn("sc0371 timing return -10");
		return -10;
	}

	int replicType = req.session->replicType;

	Jstr dbName = parseParam->objectVec[0].dbName;

	Jstr tableName = parseParam->objectVec[0].tableName;
	Jstr indexName = parseParam->objectVec[0].indexName;
	Jstr indexName1 = parseParam->objectVec[1].indexName;
	if ( indexName.size() < 1) indexName = indexName1;

	dn("sc03726 dropindex prepare dbName=[%s] tableName=[%s] indexName=[%s] indexName1=[%s]",
		dbName.s(), tableName.s(), indexName.s(), indexName1.s() );

	JagTable *ptab = NULL;
	JagIndex *pindex = NULL;
	JagTableSchema *tableschema;
	JagIndexSchema *indexschema;

	getTableIndexSchema( replicType, tableschema, indexschema );

	int  lockrc;
	ptab = _objectLock->writeLockTable( JAG_NOOP, dbName, tableName, tableschema, replicType, 0, lockrc );	
	if ( ! ptab ) {
		dn("sc01019 ptab==NULL return -20");
		return -20;
	}

	pindex = _objectLock->writeLockIndex( JAG_NOOP, dbName, tableName, indexName,
										tableschema, indexschema, replicType, 1, lockrc );

	if ( ! pindex ) {
		_objectLock->writeUnlockTable( JAG_NOOP, dbName, tableName, replicType, 0 );
		dn("sc01329 pindex==NULL return -30  lockrc=%d, index not found", lockrc);
		return -30;
	}

	int rci = _objectLock->writeUnlockIndex( JAG_NOOP, dbName, tableName, indexName, replicType, 1 );
	int rct = _objectLock->writeUnlockTable( JAG_NOOP, dbName, tableName, replicType, 0 );

	dn("sc01040 return 0 rctab=%d rcidx=%d", rct, rci);
	return 0;
}

// 0: OK; <0 error
int JagDBServer::dropIndexCommit( JagRequest &req, const Jstr &dbname, 
							JagParseParam *parseParam, Jstr &reterr, jagint threadQueryTime, Jstr &timeSer )
{
	dn("s1228 dropIndexCommit() ...");

	if ( threadQueryTime > 0 && threadQueryTime < g_lastHostTime ) {
		dn("sc033391 time not ok");
		return -10;
	}

	Jstr dbobj = parseParam->objectVec[0].dbName + "." + parseParam->objectVec[0].tableName;

	JagTable *ptab = NULL;
	JagIndex *pindex = NULL;
	JagTableSchema *tableschema;
	JagIndexSchema *indexschema;
	int replicType = req.session->replicType;

	getTableIndexSchema( replicType, tableschema, indexschema );

	Jstr dbName = parseParam->objectVec[0].dbName;
	Jstr tableName = parseParam->objectVec[0].tableName;
	Jstr indexName = parseParam->objectVec[0].indexName;
	Jstr indexName1 = parseParam->objectVec[1].indexName;
	if ( indexName.size() < 1) indexName = indexName1;

	dn("sc03766 dropindex prepare dbName=[%s] tableName=[%s] indexName=[%s] indexName1=[%s]",
		dbName.s(), tableName.s(), indexName.s(), indexName1.s() );

	int  lockrc;
	ptab = _objectLock->writeLockTable( parseParam->opcode, dbName, tableName, tableschema, replicType, 0, lockrc );
    if ( ! ptab ) {
		dn("sc03331 ptab==NULL not ok lockrc=%d table not found", lockrc );
 	    return -20;
	}

	pindex = _objectLock->writeLockIndex( parseParam->opcode, dbName, tableName, indexName,
										tableschema, indexschema, replicType, 1, lockrc );

	if ( ! pindex ) {
		_objectLock->writeUnlockTable( parseParam->opcode, dbName, tableName, replicType, 0 );
		dn("sc03531 pindex==NULL not ok, return 0 lockrc=%d index not found", lockrc);
		return -30;
	}

	ptab->hasTimeSeries( timeSer );

	req.session->spCommandReject = 0;
	Jstr dbtabidx;

	pindex->drop();

	dbtabidx = dbname + "." + tableName + "." + indexName;

	JAG_BLURT jaguar_mutex_lock ( &g_dbschemamutex ); JAG_OVER
	
	indexschema->remove( dbtabidx );
	ptab->dropFromIndexList( indexName );	
	refreshSchemaInfo( replicType, g_lastSchemaTime );

	jaguar_mutex_unlock ( &g_dbschemamutex );
	
	// drop one index
	delete pindex;

	_objectLock->writeUnlockIndex( parseParam->opcode, dbName, tableName, indexName, replicType, 1 );

	_objectLock->writeUnlockTable( parseParam->opcode,  dbName, tableName, replicType, 0 );

	jd(JAG_LOG_LOW, "user [%s] drop index [%s]\n", req.session->uid.c_str(), dbtabidx.c_str() );
	return 0;
}

int JagDBServer::truncateTable( JagRequest &req, JagParseParam *parseParam, Jstr &reterr, jagint threadQueryTime )
{
	int rc;
	if ( req.isPrepare() ) {
		rc = truncateTablePrepare( req, parseParam, threadQueryTime );	
		if ( 0 == rc ) {
			sendDataEnd( req, "OK");
			req.session->spCommandReject = 0;
		} else {
			sendDataEnd( req, "NG");
			req.session->spCommandReject = 1;
		}
	} else {
		rc = truncateTableCommit( req, parseParam, reterr, threadQueryTime);	
	}

	d("s303344 truncateTable done rc=%d req.sqlhdr[2]=%c ...\n", rc, req.sqlhdr[2]);
	if ( 0 == rc ) return 1;
	else return 0;
}

// 0: OK  <0: error
int JagDBServer::truncateTablePrepare( JagRequest &req, JagParseParam *parseParam, jagint threadQueryTime )
{
	if ( threadQueryTime > 0 && threadQueryTime < g_lastHostTime ) {
		return -10;
	}

	Jstr dbname = parseParam->objectVec[0].dbName;
	Jstr tabname = parseParam->objectVec[0].tableName;
	Jstr dbobj = dbname + "." + tabname;
	Jstr scdbobj = dbobj + "." + intToStr( req.session->replicType );
	Jstr indexNames;

	JagTable *ptab = NULL;
	JagTableSchema *tableschema;
	JagIndexSchema *indexschema;
	getTableIndexSchema( req.session->replicType, tableschema, indexschema );

	int  lockrc;
	ptab = _objectLock->writeLockTable( JAG_NOOP, dbname, tabname, tableschema, req.session->replicType, 0, lockrc ); 
	if ( ! ptab ) {
		dn("sc938337 writeLockTable NULL lockrc=%d", lockrc );
		return -20;
	}

	_objectLock->writeUnlockTable( JAG_NOOP, dbname, tabname, req.session->replicType, 0 );

	return 0;

}

// 0: ok
int JagDBServer::truncateTableCommit( JagRequest &req, JagParseParam *parseParam, Jstr &reterr, jagint threadQueryTime )
{
	if ( threadQueryTime > 0 && threadQueryTime < g_lastHostTime ) {
		return 0;
	}

	Jstr dbname = parseParam->objectVec[0].dbName;
	Jstr tabname = parseParam->objectVec[0].tableName;
	Jstr dbobj = dbname + "." + tabname;
	Jstr scdbobj = dbobj + "." + intToStr( req.session->replicType );
	Jstr indexNames;

	JagTable *ptab = NULL;
	JagIndex *pindex = NULL;
	JagTableSchema *tableschema;
	JagIndexSchema *indexschema;
	getTableIndexSchema( req.session->replicType, tableschema, indexschema );

	int lockrc;
	ptab = _objectLock->writeLockTable( parseParam->opcode, dbname, tabname, tableschema, req.session->replicType, 0, lockrc ); 
	if ( ! ptab ) {
		return 0;
	}

	req.session->spCommandReject = 0;

	indexNames = ptab->drop( reterr, true );

	refreshSchemaInfo( req.session->replicType, g_lastSchemaTime );
	
	delete ptab;

	// rebuild ptab, and possible related indexes
	ptab = _objectLock->writeTruncateTable( parseParam->opcode, dbname, tabname, tableschema, req.session->replicType, 0 ); 
	if ( ptab ) {
		JagStrSplit sp( indexNames, '|', true );
		for ( int i = 0; i < sp.length(); ++i ) {
			pindex = _objectLock->writeLockIndex( JAG_CREATEINDEX_OP, dbname, tabname, sp[i],
													tableschema, indexschema, req.session->replicType, 1, lockrc );
			if ( pindex ) {
				ptab->_indexlist.append( pindex->getIndexName() );
			    _objectLock->writeUnlockIndex( JAG_CREATEINDEX_OP, dbname, tabname, sp[i], req.session->replicType, 1 );
			}
		}
		_objectLock->writeUnlockTable( parseParam->opcode, dbname, tabname, req.session->replicType, 0 );
	}

	jd(JAG_LOG_LOW, "user [%s] truncate table [%s]\n", req.session->uid.c_str(), dbobj.c_str() );

	// remove wallog
	JAG_BLURT jaguar_mutex_lock ( &g_wallogmutex ); JAG_OVER
	Jstr fpath = _cfg->getWalLogHOME() + "/" + dbname + "." + tabname + ".wallog";
	jagunlink( fpath.s() );
	JAG_BLURT jaguar_mutex_unlock ( &g_wallogmutex ); 

	return 0;
}

// methods will affect userrole or schema
void JagDBServer::grantPerm( JagRequest &req, const JagParseParam &parseParam, jagint threadQueryTime )
{
	int rc;
	if ( req.isPrepare() ) {
		rc = grantPermPrepare( req, parseParam, threadQueryTime );	
		if ( 0 == rc ) {
			sendDataEnd( req, "OK");
			req.session->spCommandReject = 0;
		} else {
			sendDataEnd( req, "NG");
			req.session->spCommandReject = 0;
		}
	} else {
		rc = grantPermCommit( req, parseParam, threadQueryTime);	
	}

	d("s303344 truncateTable done rc=%d req.sqlhdr[2]=%c ...\n", rc, req.sqlhdr[2]);
}

// 0: OK  <0 error
int JagDBServer::grantPermPrepare( JagRequest &req, const JagParseParam &parseParam, jagint threadQueryTime )
{
	if ( threadQueryTime > 0 && threadQueryTime < g_lastHostTime ) {
		return -10;
	}

	req.session->spCommandReject = 0;
	Jstr uid  = parseParam.grantUser;
	JagStrSplit sp( parseParam.grantObj, '.' );
	JagUserRole *uidrole = NULL;

	if ( req.session->replicType == 0 ) uidrole = _userRole;
	else if ( req.session->replicType == 1 ) uidrole = _prevuserRole;
	else if ( req.session->replicType == 2 ) uidrole = _nextuserRole;

	if ( ! uidrole ) {
		return -15;
	}

	Jstr scdbobj = uid + "." + intToStr( req.session->replicType );

	int rc = 0;
	_objectLock->writeLockSchema( req.session->replicType );

	if ( sp.length() == 3 ) {
		if ( sp[0] != "*" ) {
			if ( ! dbExist( sp[0], req.session->replicType ) ) {
				rc = -20;
			} else if (  sp[1] != "*" ) {
				if ( ! objExist( sp[0], sp[1], req.session->replicType ) ) {
					rc = -30;
				}
			}
		}
	} else {
		rc = -35;
	}

	if ( rc < 0 ) {
		_objectLock->writeUnlockSchema( req.session->replicType );
		return rc;
	}

	JagUserID *uiddb = NULL;
	if ( req.session->replicType == 0 ) uiddb = _userDB;
	else if ( req.session->replicType == 1 ) uiddb = _prevuserDB;
	else if ( req.session->replicType == 2 ) uiddb = _nextuserDB;

	// check if uid exists
	rc = 0;
	if ( uiddb ) {
		if ( ! uiddb->exist( uid ) ) {
			rc = -40;
		} 
	} else {
		rc = -50;
	}

	_objectLock->writeUnlockSchema( req.session->replicType );
	return rc;
}

// 0: OK
int JagDBServer::grantPermCommit( JagRequest &req, const JagParseParam &parseParam, jagint threadQueryTime )
{
	if ( threadQueryTime > 0 && threadQueryTime < g_lastHostTime ) {
		return 0;
	}

	Jstr uid  = parseParam.grantUser;
	JagStrSplit sp( parseParam.grantObj, '.' );
	JagUserRole *uidrole = NULL;

	if ( req.session->replicType == 0 ) uidrole = _userRole;
	else if ( req.session->replicType == 1 ) uidrole = _prevuserRole;
	else if ( req.session->replicType == 2 ) uidrole = _nextuserRole;

	if ( ! uidrole ) {
		return 0;
	}

	Jstr scdbobj = uid + "." + intToStr( req.session->replicType );

	_objectLock->writeLockSchema( req.session->replicType );

	int rc = 0;
	if ( sp.length() == 3 ) {
		if ( sp[0] != "*" ) {
			// db.tab.col  check DB
			if ( ! dbExist( sp[0], req.session->replicType ) ) {
				rc = -20;
			} else if (  sp[1] != "*" ) {
				if ( ! objExist( sp[0], sp[1], req.session->replicType ) ) {
					rc = -30;
				}
			}
		}
	} else {
		rc = -35;
	}

	if ( rc  < 0 ) {
		_objectLock->writeUnlockSchema( req.session->replicType );
		return 0;
	}

	JagUserID *uiddb = NULL;
	if ( req.session->replicType == 0 ) uiddb = _userDB;
	else if ( req.session->replicType == 1 ) uiddb = _prevuserDB;
	else if ( req.session->replicType == 2 ) uiddb = _nextuserDB;

	// check if uid exists
	rc = 0;
	if ( uiddb ) {
		if ( ! uiddb->exist( uid ) ) {
			rc = -40;
		} 
	} else {
		rc = -50;
	}

	if ( rc  < 0 ) {
		_objectLock->writeUnlockSchema( req.session->replicType );
		return 0;
	}

	JAG_BLURT jaguar_mutex_lock ( &g_dbschemamutex ); JAG_OVER
	if ( uidrole ) {
		uidrole->addRole( uid, sp[0], sp[1], sp[2], parseParam.grantPerm, parseParam.grantWhere );
	} 
	jaguar_mutex_unlock ( &g_dbschemamutex );

	_objectLock->writeUnlockSchema( req.session->replicType );
	return 0;
}

void JagDBServer::revokePerm( JagRequest &req, const JagParseParam &parseParam, jagint threadQueryTime )
{
	int rc;
	if ( req.isPrepare() ) {
		rc = revokePermPrepare( req, parseParam, threadQueryTime );	
		if ( 0 == rc ) {
			sendDataEnd( req, "OK");
			req.session->spCommandReject = 0;
		} else {
			sendDataEnd( req, "NG");
			req.session->spCommandReject = 1;
		}
	} else {
		rc = revokePermCommit( req, parseParam, threadQueryTime);	
	}

	d("s303344 revokePerm done rc=%d req.sqlhdr[2]=%c ...\n", rc, req.sqlhdr[2]);
}

// 0: OK;  <0 error
int JagDBServer::revokePermPrepare( JagRequest &req, const JagParseParam &parseParam, jagint threadQueryTime )
{
	if ( threadQueryTime > 0 && threadQueryTime < g_lastHostTime ) {
		return -10;
	}

	Jstr uid = parseParam.grantUser;
	JagStrSplit sp( parseParam.grantObj, '.' );

	if ( sp.length() != 3 ) {
		return -20;
	}

	JagUserRole *uidrole = NULL;
	if ( req.session->replicType == 0 ) uidrole = _userRole;
	else if ( req.session->replicType == 1 ) uidrole = _prevuserRole;
	else if ( req.session->replicType == 2 ) uidrole = _nextuserRole;

	if ( ! uidrole ) {
		return -25;
	}

	Jstr scdbobj = uid + "." + intToStr( req.session->replicType );

	_objectLock->writeLockSchema( req.session->replicType );


	JagUserID *uiddb = NULL;
	if ( req.session->replicType == 0 ) uiddb = _userDB;
	else if ( req.session->replicType == 1 ) uiddb = _prevuserDB;
	else if ( req.session->replicType == 2 ) uiddb = _nextuserDB;

	JAG_BLURT jaguar_mutex_lock ( &g_dbschemamutex ); JAG_OVER
	bool rc = false;
	if ( uiddb ) {
		rc = uiddb->exist( uid );
	}
	jaguar_mutex_unlock ( &g_dbschemamutex );

	_objectLock->writeUnlockSchema( req.session->replicType );

	if ( ! rc ) {
		return -30;
	}

	return 0;
}

// 0: ok
int JagDBServer::revokePermCommit( JagRequest &req, const JagParseParam &parseParam, jagint threadQueryTime )
{
	if ( threadQueryTime > 0 && threadQueryTime < g_lastHostTime ) {
		return 0;
	}

	Jstr uid = parseParam.grantUser;
	JagStrSplit sp( parseParam.grantObj, '.' );

	if ( sp.length() != 3 ) {
		return 0;
	}

	JagUserRole *uidrole = NULL;

	if ( req.session->replicType == 0 ) uidrole = _userRole;
	else if ( req.session->replicType == 1 ) uidrole = _prevuserRole;
	else if ( req.session->replicType == 2 ) uidrole = _nextuserRole;

	if ( ! uidrole ) {
		return 0;
	}

	Jstr scdbobj = uid + "." + intToStr( req.session->replicType );

	_objectLock->writeLockSchema( req.session->replicType );


	JagUserID *uiddb = NULL;
	if ( req.session->replicType == 0 ) uiddb = _userDB;
	else if ( req.session->replicType == 1 ) uiddb = _prevuserDB;
	else if ( req.session->replicType == 2 ) uiddb = _nextuserDB;

	if ( ! uiddb ) return 0;

	JAG_BLURT jaguar_mutex_lock ( &g_dbschemamutex ); JAG_OVER
	bool rc = false;
	if ( uiddb ) {
		rc = uiddb->exist( uid );
	}
	jaguar_mutex_unlock ( &g_dbschemamutex );

	if ( ! rc ) {
		_objectLock->writeUnlockSchema( req.session->replicType );
		return 0;
	}

	JAG_BLURT jaguar_mutex_lock ( &g_dbschemamutex ); JAG_OVER
	rc = false;
	if ( uidrole ) {
		rc = uidrole->dropRole( uid, sp[0], sp[1], sp[2], parseParam.grantPerm );
	}
	jaguar_mutex_unlock ( &g_dbschemamutex );

	_objectLock->writeUnlockSchema( req.session->replicType );
	return 0;
}
