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
#include <JagGlobalDef.h>

#include <sys/types.h>

#ifndef _WINDOWS64_
#include <ifaddrs.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/netdevice.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#else
#include<winsock2.h>
#include<mswsock.h>
#include<ws2tcpip.h>
#include<ws2def.h>
#include<mstcpip.h>
#include<IPIfCons.h>
#include<windows.h>
#pragma comment(lib,"ws2_32.lib")
#include <iphlpapi.h>
#include <stdio.h>
#include <stdlib.h>
#pragma comment(lib, "IPHLPAPI.lib")
#endif

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <JagNet.h>
#include <JagStrSplit.h>
#include <JagUtil.h>
#include <JagLog.h>

int JagNet::_socketHasBeenSetup = 0;

Jstr JagNet::getLocalHost()
{
	char buf[128];
	memset( buf, 0, 128 );
	gethostname( buf, 127 );
	return buf;
}


//////// rayconnect
JAGSOCK rayconnect( const char *host, unsigned int port, int timeoutsecs, bool errPrint )
{
	//int  rc, len;
	//struct  hostent *serv;
	//struct sockaddr_in serv_addr;
	if ( 0 == strcmp( host, "localhost" ) ) {
		host = "127.0.0.1";
	}

	JAGSOCK sockfd = JagNet::connectToHost( host, port, timeoutsecs, errPrint );
	return sockfd;
}

#ifdef _WINDOWS64_
void rayclose ( JAGSOCK winsock ) 
{
	int rlen = ::closesocket( winsock );
	while ( rlen < 0 && WSAGetLastError() == WSAEWOULDBLOCK ) {
		rlen = ::closesocket( winsock );
	} 
}
#else
void rayclose ( JAGSOCK sock ) 
{
	int rlen = ::close( sock );
	while ( rlen < 0 && errno == EWOULDBLOCK ) {
		rlen = ::close( sock );
	} 
}
#endif


#ifdef _WINDOWS64_
void JagNet::getLocalIPs( JagVector<Jstr> &vec )
{
    char ac[180];
    if (gethostname(ac, sizeof(ac)) == SOCKET_ERROR) { return; }
    struct hostent *phe = gethostbyname(ac);
    if (phe == 0) { return; }
    for (int i = 0; phe->h_addr_list[i] != 0; ++i) {
        struct in_addr addr;
        memcpy(&addr, phe->h_addr_list[i], sizeof(struct in_addr));
		vec.append( Jstr(inet_ntoa(addr)) );
    }
}

JAGSOCK JagNet::connectToHost( const char *host, unsigned short port, int timeoutsecs, bool errPrint )
{
	struct sockaddr_in server;
	socketStartup();
	SOCKET s;

	char    ip[64];
	memset( ip, 0, 64 );
	_getIPFromHostName( host, ip );
	if ( strlen( ip ) < 1 ) {
		return INVALID_SOCKET;
	}

	if((s = socket(AF_INET , SOCK_STREAM , 0 )) == INVALID_SOCKET) {
        printf("Could not create socket : %d" , WSAGetLastError());
		return INVALID_SOCKET;
    }

    // server.sin_addr.s_addr = inet_addr(host);
    server.sin_addr.s_addr = inet_addr( ip );
    server.sin_family = AF_INET;
    server.sin_port = htons( port );
    dn("n202910 connectWithTimeout ...");
	int rc = connectWithTimeout(s, (const struct sockaddr *)&server, sizeof(sockaddr_in), timeoutsecs*1000); 
    dn("n202910 connectWithTimeout rc=%d", rc);
	if ( rc < 0 ) {
		return INVALID_SOCKET;
	}

	return s;
}

#else
// Linux
void JagNet::getLocalIPs( JagVector<Jstr> &vec )
{
    struct ifconf ifconf;
    struct ifreq ifr[50];
    int s, i, ifs;
  
    int domain = AF_INET;
    s = socket( domain, SOCK_STREAM, 0);
    if (s < 0) {
        perror("socket");
        return;
    }
  
    ifconf.ifc_buf = (char *) ifr;
    ifconf.ifc_len = sizeof ifr;
  
    if (ioctl(s, SIOCGIFCONF, &ifconf) == -1) {
        perror("ioctl");
        return;
    }
  
    ifs = ifconf.ifc_len / sizeof(ifr[0]);
    // printf("interfaces = %d:\n", ifs);
    for (i = 0; i < ifs; i++) {
        char ip[INET_ADDRSTRLEN+1];
		memset( ip, 0, INET_ADDRSTRLEN+1);
        struct sockaddr_in *s_in = (struct sockaddr_in *) &ifr[i].ifr_addr;
        if (!inet_ntop(domain, &s_in->sin_addr, ip, sizeof(ip))) {
            perror("inet_ntop");
            return;
        }
        // printf("%s - %s\n", ifr[i].ifr_name, ip);
		if ( strlen(ip) > 0 ) {
  	    	vec.append( Jstr(ip) );
		}
    }
  
    close(s);
}

// new
JAGSOCK JagNet::connectToHost( const char *host, unsigned short port, int timeoutsecs, bool errPrint )
{
    JAGSOCK 	sockfd;
    struct addrinfo hints, *servinfo, *p;
    int     rv;
	char    ports[12];
	char    ip[64];

	memset( ip, 0, 64 );
	_getIPFromHostName( host, ip );
	d("s3092 host=[%s] ip=[%s]\n", host, ip );
	if ( strlen( ip ) < 1 ) {
		return -1;
	}

	sprintf( ports, "%d", port );
	socketStartup();
    memset(&hints, 0, sizeof(hints) );
    // hints.ai_family = AF_UNSPEC; // use AF_INET6 to force IPv6
    hints.ai_family = AF_INET; // use AF_INET6 to force IPv6
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;

    if ((rv = getaddrinfo( ip, ports, &hints, &servinfo)) != 0) {
        if ( errPrint ) { 
			printf( "Error s7201: getaddrinfo(%s/%s:%d) %s\n", host, ip, port, gai_strerror(rv)); 
			fflush( stdout ); 
		}
        return ( -1 );
    }

    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            continue;
        }

        // if ( ::connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        if ( connectWithTimeout(sockfd, p->ai_addr, p->ai_addrlen, timeoutsecs*1000 ) < 0 ) {
            close(sockfd);
            sockfd = INVALID_SOCKET;
            continue;
        }
		// if ( ports[0] != '7' || ports[1] != '7' || ports[2] != '7' || ports[3] != '7' ) abort();
        break; // if we get here, we must have connected successfully
    }
    
    freeaddrinfo(servinfo); // all done with this structure
    if (p == NULL) {
		/***
		if ( errPrint ) { printf("Unable to connect to [%s:%d], try connecting again ...\n", host, port ); fflush( stdout ); }
		return simpleConnect( host, port, errPrint );
		***/
		if ( errPrint ) { printf("E30887 Unable to connect to [%s:%d]\n", host, port ); fflush( stdout ); }
		return INVALID_SOCKET;
    }

	int yes = 1;
	setsockopt( sockfd, IPPROTO_TCP, TCP_NODELAY, (CHARPTR)&yes, sizeof(yes));
	setsockopt( sockfd, SOL_SOCKET, SO_KEEPALIVE, (CHARPTR)&yes, sizeof(yes));

	return sockfd;
}
#endif


#ifndef _WINDOWS64_
// Linux
Jstr JagNet::getMacAddress()
{
    struct ifreq ifr;
    struct ifconf ifc;
    char buf[1024];
    int success = 0;

    JAGSOCK sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock == INVALID_SOCKET) { /* handle error*/ };

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (ioctl(sock, SIOCGIFCONF, &ifc) == -1) { /* handle error */ }

    struct ifreq* it = ifc.ifc_req;
    const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));

    for (; it != end; ++it) {
        strcpy(ifr.ifr_name, it->ifr_name);
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
            if (! (ifr.ifr_flags & IFF_LOOPBACK)) { // don't count loopback
                if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
                    success = 1;
                    break;
                }
            }
        }
        else { /* handle error */ }
    }

    unsigned char mac_address[7];
	memset( mac_address, 0, 7 );
    if (success) memcpy(mac_address, ifr.ifr_hwaddr.sa_data, 6);
	return Jstr( (char*)mac_address );
}
void beginBulkSend( JAGSOCK sock ) 
{ 
	int on = 1;
	setsockopt( sock, SOL_TCP, TCP_CORK, (CHARPTR)&on, sizeof(on) );
}

void endBulkSend( JAGSOCK sock ) 
{ 
	int on = 0;
	setsockopt( sock, SOL_TCP, TCP_CORK, (CHARPTR)&on, sizeof(on) );
}
#else
// windows
Jstr JagNet::getMacAddress()
{
	Jstr firstMac;
	char buf[64];

    IP_ADAPTER_INFO AdapterInfo[16];       // Allocate information 
    // for up to 16 NICs
    DWORD dwBufLen = sizeof(AdapterInfo);  // Save memory size of buffer

    DWORD dwStatus = GetAdaptersInfo(      // Call GetAdapterInfo
        AdapterInfo,                 // [out] buffer to receive data
        &dwBufLen);                  // [in] size of receive data buffer

    PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo; // Contains pointer to
    // current adapter info
    while( pAdapterInfo ) {
        // PrintMACaddress(pAdapterInfo->Address); // Print MAC address
    	sprintf( buf, "%02X%02X%02X%02X%02X%02X", 
    			pAdapterInfo->Address[0], pAdapterInfo->Address[1], 
				pAdapterInfo->Address[2], pAdapterInfo->Address[3], 
				pAdapterInfo->Address[4], pAdapterInfo->Address[5] );
		firstMac = buf;
		break;
        pAdapterInfo = pAdapterInfo->Next;    // Progress through 
    }

    return firstMac;
}
void beginBulkSend( JAGSOCK sock ) 
{ 
	/***
	int on = 1;
	setsockopt( sock, SOL_TCP, TCP_CORK, (CHARPTR)&on, sizeof(on) );
	***/
}

void endBulkSend( JAGSOCK sock ) 
{ 
	/***
	int on = 0;
	setsockopt( sock, SOL_TCP, TCP_CORK, (CHARPTR)&on, sizeof(on) );
	***/
}
#endif


JAGSOCK JagNet::createIPV4Socket( const char *ip, short port )
{
    struct sockaddr_in servaddr;
	int yes = 1;

    JAGSOCK sock=socket(AF_INET,SOCK_STREAM, 0);
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
	if ( strlen(ip) < 1 ) {
    	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	} else {
    	servaddr.sin_addr.s_addr= inet_addr( ip );
	}
    servaddr.sin_port=htons( port );

	setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, (CHARPTR)&yes, sizeof(yes) );
	setsockopt( sock, IPPROTO_TCP, TCP_NODELAY, (CHARPTR)&yes, sizeof(yes));
    if ( bind(sock,(struct sockaddr *)&servaddr,sizeof(servaddr)) < 0 ) {
		printf("s4004 %s port=%d\n", strerror( errno ), port );
		fflush( stdout );
		return -1;
	}
	return sock;
}

#ifndef _WINDOWS64_
// Linux
int connectWithTimeout(JAGSOCK sockfd, const struct sockaddr *addr, socklen_t addrlen, int timeoutmillisec ) 
{
	d("n8872939 connectWithTimeout timeoutmillisec=%d\n", timeoutmillisec );

	//jagint arg;
	int res, valopt, lon=sizeof(int);

	JagNet::socketNonBlocking( sockfd );
	res = ::connect(sockfd, addr, addrlen); 

	if (res < 0) { 
		if (errno == EINPROGRESS) {
            dn("n501928 EINPROGRESS ..");
			struct pollfd pfds;
			pfds.fd = sockfd;
			pfds.events = POLLOUT;

            dn("n2009 poll ...");
			res = poll(&pfds, 1, timeoutmillisec);
            dn("n2009 poll done res=%d", res);

			if ( res < 0 ) {
				d("Error connecting with poll 1 %d - %s\n", errno, strerror(errno)); fflush( stdout );
				return -1;
			} else if ( 0 == res ) {
				d("Connection timeout 2 \n"); fflush( stdout );
				return -1;
			} else {
				if ( pfds.revents == POLLERR || pfds.revents == POLLHUP || pfds.revents == POLLNVAL ) {
					d("Poll connection return with error 3 %d\n", pfds.revents); fflush( stdout );
					return -1;
				}

				getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), (socklen_t*)&lon);
				if (valopt) {
					d( "Error in connection() 4 %d - %s\n", valopt, strerror(valopt)); fflush( stdout );
					return -1;
				}
			}
		} else {
			d("Error connecting with poll 5 %d - %s\n", errno, strerror(errno)); fflush( stdout );
			return -1;
		}
	}
	
	// Set to blocking mode again... 
	JagNet::socketBlocking( sockfd );
	return 1;
}
#else
// _WINDOWS64_
#define ERR(e) printf("%s:%s failed: %d [%s@%lld]\n",__FUNCTION__,e,WSAGetLastError(),__FILE__,__LINE__)
#define CLOSESOCK(s) if(INVALID_SOCKET != s) {closesocket(s); s = INVALID_SOCKET;}
int connectWithTimeout(JAGSOCK sockfd, const struct sockaddr *addr, socklen_t addrlen, int timeoutmillisec ) 
{
	INT ret;

	// Set non-blocking 
	JagNet::socketNonBlocking( sockfd );

	// res = ::connect(sockfd, addr, addrlen); 
	if ( SOCKET_ERROR == ::connect(sockfd, addr, addrlen) ) {
		if (WSAEWOULDBLOCK != WSAGetLastError()) {
			return -1;
		}
	}

	WSAEVENT hEvent = WSACreateEvent();
	if ( hEvent == WSA_INVALID_EVENT ) {
		return -2;
	}

	// Ask event notification when socket will be connected
	int nRet = WSAEventSelect(sockfd, hEvent, FD_CONNECT);             
	if (nRet == SOCKET_ERROR) {
		return -3;
	}

    // wait for connection
    DWORD dwRet = WSAWaitForMultipleEvents(1, &hEvent , FALSE, timeoutmillisec, FALSE);
    if (dwRet == WSA_WAIT_TIMEOUT) {
    	return -4; // timedout
    }
    
    WSANETWORKEVENTS events;
    nRet = WSAEnumNetworkEvents(sockfd, hEvent, &events);
    if (nRet == SOCKET_ERROR ) {
    	return -5;
    }
    
    if ( events.lNetworkEvents & FD_CONNECT ) {
        if ( events.iErrorCode[FD_CONNECT_BIT] != 0 ) {
    		return -6;
        }
    
		// after doing EventSelect, need to turn it off in order to set back to blocking mode for socket later
		WSAEventSelect(sockfd, hEvent, 0);
		JagNet::socketBlocking( sockfd );
		return 1; // connection OK !!!
    } else {
   		return -7;
	}

	// after doing EventSelect, need to turn it off in order to set back to blocking mode for socket later
	WSAEventSelect(sockfd, hEvent, 0);
	JagNet::socketBlocking( sockfd );
	return 0;
}
// get network reads and writes
int JagNet::getNetStat( jaguint & reads, jaguint &writes )
{
    reads = writes = 0;
    DWORD dwRetval;
    MIB_IPSTATS *pStats;
    pStats = (MIB_IPSTATS *) malloc(sizeof (MIB_IPSTATS));
    if (pStats == NULL) {
        return 0;
    }

    dwRetval = GetIpStatistics(pStats);
    if (dwRetval != NO_ERROR) {
    	if (pStats) free(pStats);
		return 0;
    } else {
		reads =  pStats->dwInReceives;
		writes =  pStats->dwOutRequests;
    }

    if (pStats) free(pStats);
    return 1;
}
#endif



#ifndef _WINDOWS64_
// Linux
// get network reads and writes
int JagNet::getNetStat( jaguint & reads, jaguint &writes )
{
    reads = writes = 0;
    FILE *fp = fopen( "/proc/net/dev", "rb" );
    if ( ! fp ) return 0;
    Jstr line;

    char buf[1024];
    while ( NULL != ( fgets(buf, 1024, fp ) ) ) {
        if ( ! strchr( buf, ':' ) ) continue;
        line = buf;
        JagStrSplit sp( line, ' ', true );
        if ( sp.length() < 10 ) continue;
        reads += jagatoll( sp[1].c_str() );
        writes += jagatoll( sp[9].c_str() );
    }

    return 1;
}

void JagNet::socketNonBlocking( JAGSOCK sockfd )
{
	jagint arg;
	// Set to non-blocking 
	arg = fcntl(sockfd, F_GETFL, NULL); 
	arg |= O_NONBLOCK; 
	fcntl(sockfd, F_SETFL, arg); 
}

void JagNet::socketBlocking( JAGSOCK sockfd )
{
	jagint arg;
	// Set to blocking 
	arg = fcntl(sockfd, F_GETFL, NULL); 
	arg ^= O_NONBLOCK; 
	fcntl(sockfd, F_SETFL, arg); 
}

int JagNet::socketStartup()
{
	return 1;
}

int JagNet::socketCleanup()
{
	return 1;
}

int JagNet::getNumTCPConnections()
{
    FILE *fp = fopen("/proc/net/tcp", "rb" );
	if ( ! fp ) return 0;
    int cnt = 0;
	char line[256];
    while ( NULL != fgets( line, 250, fp ) ) { ++cnt; }
    fclose( fp);
	return cnt;
}

#else
// windows
void JagNet::socketNonBlocking( JAGSOCK sockfd )
{
	u_long non_blocking = 1;
	ioctlsocket(sockfd, FIONBIO, &non_blocking);
}

void JagNet::socketBlocking( JAGSOCK sockfd )
{
	u_long non_blocking = 0;
	ioctlsocket(sockfd, FIONBIO, &non_blocking);
}

int JagNet::socketStartup()
{
	if ( _socketHasBeenSetup ) {
		return 1;
	}

	WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0) {
        return 0;
    }

	_socketHasBeenSetup = 1;
	return 1;
}

int JagNet::socketCleanup()
{
	WSACleanup();
	_socketHasBeenSetup = 0;
	return 1;
}

int JagNet::getNumTCPConnections()
{
    FILE *fp = popen("netstat -n -p tcp | findstr ESTABLISHED", "r" );
	if ( ! fp ) return 0;
    char  buf[256];

	int cnt = 0;
    while ( NULL != fgets( buf, 256, fp ) ) {
		++cnt;
    }
    pclose( fp );
    return cnt;
}
#endif

#ifdef _WINDOWS64_
void  JagNet::setRecvSendTimeOut( JAGSOCK sock, int dtimeout, int cliservSameProcess  )
{
	DWORD tv;
    struct linger so_linger;

	tv = dtimeout*1000;
    if ( cliservSameProcess > 0 ) tv = 0;
    setsockopt( sock, SOL_SOCKET, SO_SNDTIMEO, (CHARPTR)&tv, sizeof(DWORD));
    setsockopt( sock, SOL_SOCKET, SO_RCVTIMEO, (CHARPTR)&tv, sizeof(DWORD));

    so_linger.l_onoff = 1;
    so_linger.l_linger = dtimeout;
    setsockopt( sock, SOL_SOCKET, SO_LINGER, (CHARPTR)&so_linger, sizeof(so_linger) );
}
#else
void  JagNet::setRecvSendTimeOut( JAGSOCK sock, int dtimeout, int cliservSameProcess  )
{
    struct timeval tv;
    struct linger so_linger;

    tv.tv_usec = 0;
    tv.tv_sec = dtimeout;
    if ( cliservSameProcess > 0 ) tv.tv_sec = 0;
    setsockopt( sock, SOL_SOCKET, SO_SNDTIMEO, (CHARPTR)&tv, sizeof(tv));
    setsockopt( sock, SOL_SOCKET, SO_RCVTIMEO, (CHARPTR)&tv, sizeof(tv));

    so_linger.l_onoff = 1;
    so_linger.l_linger = dtimeout;
    setsockopt( sock, SOL_SOCKET, SO_LINGER, (CHARPTR)&so_linger, sizeof(so_linger) );
}
#endif

#if 0
void JagNet::_getIPFromHostName( const char *hostname, char *ip )
{
   	strcpy( ip, hostname );
	if ( atoi( hostname ) > 0 ) {
		// hostname is already ip address
		return;
	}

	// read /etc/hosts files to get IP for hostname
	getIPFromEtcFile( hostname, ip );
	if ( strlen(ip) > 4 ) {
		// d("s2239 got host from getIPFromEtcFile hostname=%s ip=%s\n", hostname, ip );
		return;
	}
	
    struct hostent *he;
    struct in_addr **addr_list;
    if ( (he = gethostbyname( hostname ) ) == NULL)  {
            printf("gethostbyname NULL %s", hostname );
            return;
    }

    addr_list = (struct in_addr **) he->h_addr_list;
    for ( int i = 0; addr_list[i] != NULL; i++)  {
        //Return the first one;
        strcpy(ip , inet_ntoa(*addr_list[i]) );
        return;
    }
}
#endif

// force hostname to be IP
void JagNet::_getIPFromHostName( const char *hostname, char *ip )
{
    strcpy(ip, hostname);
}



void JagNet::getIPFromEtcFile( const char *hostname, char *ip )
{
	const char *fn = "/etc/hosts";
	FILE *fp = fopen(fn, "r");
	if ( ! fp ) {
		*ip = '\0';
		return;
	}

	*ip = '\0';
	char buf[1024];
	Jstr line;
	Jstr hostnameUpper = makeUpperString( hostname );
	Jstr upper;
	int i;
	while ( NULL != fgets( buf, 1024, fp ) ) {
		if ( strlen( buf ) < 4 ) continue;
		if ( strchr( buf, '#' ) ) continue;
		line = trimTailChar(buf);
		JagStrSplit sp(line, ' ', true );
		if ( sp.length() < 2 ) continue;
		for ( i = 1; i < sp.length(); ++i ) {
			upper = makeUpperString( sp[i] );
			if ( upper == hostnameUpper ) {
				strcpy( ip, sp[0].c_str() );
				fclose( fp );
				return;
			}
		}
	}

	fclose( fp );
	// ip would be empty
}

Jstr JagNet::getIPFromHostName( const Jstr &hostname )
{
	if ( isIPAddress( hostname ) ) {
		return hostname;
	}

	char buf[64];
	memset( buf, 0, 64 );
	_getIPFromHostName( hostname.c_str(), buf );
	return buf;
}

/*************
#ifndef _WINDOWS64_
// Linux
// int  JagNet::acceptTimeOut( JAGSOCK sock, (struct sockaddr *)&cliaddr, ( socklen_t* )&clilen);
// return -9999 for timeout; else rc from accept
JAGSOCK JagNet::acceptTimeout( JAGSOCK sock, struct sockaddr *pcliaddr, socklen_t* pclilen, jagint thrdgrp, int timeOutseconds )
{
	int connfd = 0;
	if ( 0 == thrdgrp ) {
		d("s2266 POSIX accept ... \n" );
		connfd =  accept( sock, pcliaddr, pclilen);
		d("s2267 acceptTimeout returned accept connfd=%d \n", connfd );
		return connfd;
	}

	// set non-blocking
	JagNet::socketNonBlocking( sock );
	int cnt = 0;
	while ( 1 ) {
		connfd = accept( sock, pcliaddr, pclilen);
		if ( -1 == connfd ) {
			if ( EAGAIN == errno || EWOULDBLOCK == errno ) {
				jagsleep( 1000, JAG_MSEC );
				if ( cnt > timeOutseconds ) {
					return -9999;
				}
				d("s4801  accept none, try again ...\n" );
				++cnt;
			} else {
				break;
			}
		} else {
			break;
		}
	}

	// set blocking
	JagNet::socketBlocking( sock );
	return connfd;
}
#else
// Windows
JAGSOCK JagNet::acceptTimeout( JAGSOCK sock, struct sockaddr *pcliaddr, socklen_t* pclilen, jagint thrdGroup, int timeOutseconds )
{
	return accept( sock, pcliaddr, pclilen);
}
#endif
**********/


// if "eedd:03c3:2b6f:2d90::338c"  >= 4 ':', it is IP V6 address
// else
//	if contains "d.d.d.d" it is IPV4   d is [0-9]
//  else
//	    false
bool JagNet::isIPAddress( const Jstr &hoststr )
{
	if ( hoststr.size() < 1 ) return false;
	const char *p;
	bool isIPV6 = true;
	int numSeps = 0;
	for ( p = hoststr.s(); *p != '\0'; ++p ) {
		if ( *p == ':' || ( '0' <= *p && *p <= '9' ) || ( 'a' <= *p && *p <= 'f') ) {
			if ( *p == ':' ) ++numSeps;
		} else {
			isIPV6 = false;
			break;
		}
	}

	if ( isIPV6 && ( numSeps >= 4  ) ) return true;

	// check if it is IPV4
	bool isIPV4 = true;
	numSeps = 0;
	for ( p = hoststr.s(); *p != '\0'; ++p ) {
		if ( *p == '.' || ( '0' <= *p && *p <= '9' ) ) {
			if ( *p == '.' ) ++numSeps;
		} else {
			isIPV4 = false;
			break;
		}
	}

	if ( isIPV4 && ( 3 == numSeps ) ) return true;

	return false;
}
