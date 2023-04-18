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
#ifndef _jag_replicate_backup_h_
#define _jag_replicate_backup_h_

#include <atomic>
#include <abax.h>
#include <JagDef.h>
#include <JagNet.h>
#include <JaguarCPPClient.h>

class JagReplicateConnAttr
{
  public:
	JagReplicateConnAttr() { init(); }
	~JagReplicateConnAttr() { destroy(); }
	
	void init() 
    {
		_sock = INVALID_SOCKET; 
		_port = 0; _clientFlag = 0;
		_fromServ = 0;
		_cliservSameProcess = 0;
		_bulkSend = _hasReply = true;
		_result = 0; _len = 0;
		memset(_hdr, 0, JAG_SOCK_TOTAL_HDR_LEN+1);
		memset(_qbuf, 0, 2048);
		_dbuf = NULL;
	}

	void    destroy() { if ( !socket_bad(_sock) ) rayclose( _sock ); }
	
	bool    _bulkSend;
	bool    _hasReply;
	bool    _fromServ;
	int     _result;
    JAGSOCK _sock;
	int     _port;
	int     _cliservSameProcess;
	jagint _len;
	Jstr    _host;
	Jstr    _username;
	Jstr    _password;
	Jstr    _dbName;
	Jstr    _tableName;
	Jstr    _hashDir;
	Jstr    _connectOpt;
	Jstr    _session;
	Jstr    _query;
	Jstr    _token;
	jaguint _clientFlag;
	char    _qbuf[2048];
	char    _hdr[JAG_SOCK_TOTAL_HDR_LEN+1];
	char    *_dbuf;
};

class JagReplicateBackup
{
  public:
	JagReplicateBackup( int timeout=2, int retrylimit=30 );
	~JagReplicateBackup();
	
	bool connectReplicaHosts( int replicateCopy, int deltaRecoverConnection,
		                      unsigned int port, jaguint clientFlag, bool fromServ,
		                      const JagVector<Jstr> &hostlist, const Jstr &username, const Jstr &passwd,
		                      const Jstr &dbname, const Jstr &connectOpt, const Jstr &token );

	void updateDBName( Jstr &dbname );
	void updatePassword( Jstr &password );

	int makeConnection( int i );

	// query method
	jagint simpleQuery( int i, const char *querys, bool checkConnection );
	jagint sendQuery( const char *querys, jagint len, bool hasReply, 
				      bool isWrite, bool bulkSend, bool forceConnection, Jstr &schema, Jstr &hoststr );
	static void *simpleSendStatic( void *conn );

	jagint simpleSendRaw( int i, const char *querys, jagint len );
	static void *simpleSendStaticRaw( void *conn );

	jagint  sendDirectToSockAll( const char *mesg, jagint len, bool nohdr=false );
	jagint  recvDirectFromSockAll( char *&buf, char *hdr );
	jagint  recvRawDirectFromSockAll( char *&buf, jagint len );
    bool    sendFileToRepServers( const Jstr &inpath, int &actutalSent );
    int     sendFileToOneRepServer( int i, const Jstr &db, const Jstr &tab, const Jstr &inpath, const Jstr &hashDir );

	static void *sendFileToServerStatic( void *conn );
	bool    recvFilesFromServer( const Jstr &outpath );

	jagint  simpleReply( int i, char *hdr, char *&buf, bool &eom );
	jagint  recvReply( char *hdr, char *&buf, bool &eom );

	int     recvTwoBytes( char *condition );

	void    setConnectionBrokenTime();
	int     checkConnectionRetry();
	void    setHasReply( bool flag );


	int     _tdiff;
	int     _replicateCopy;
	int     _deltaRecoverConnection;
	int	    _replypos; // only used by read commands
	int     _isWrite;
	int     _tcode;
	JagReplicateConnAttr _conn[JAG_REPLICATE_MAX];
	char    _signal3[4];

	int     _dtimeout;
	int     _connRetryLimit;
	std::atomic<jagint>	_lastConnectionBrokenTime;
	int     _connectTimeout;
	int     _allSocketsBad;
	int     _debug;
	unsigned long _seq;
};

#endif
