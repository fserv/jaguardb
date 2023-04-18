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

#include <JagNodeMgr.h>
#include <JagStrSplit.h>
#include <JagCfg.h>
#include <JagNet.h>
#include <JagUtil.h>
#include <JagLog.h>

// protected method
void JagNodeMgr::init()
{
    dn("snm010102 JagNodeMgr::init() ...");

	_isHost0OfCluster0 = 0;
	_selfIP = "";
	_sendAllNodes = "";
	_allNodes = "";
	_hostClusterNodes = ""; // cluster of this host
	_hostClusterNumber = 0; // cluster # of this host
	_totalClusterNumber = 0;

	Jstr vfpath;
	vfpath = jaguarHome() + "/conf/cluster.conf";
	FILE *fv = jagfopen( vfpath.c_str(), "r" );
	if ( ! fv ) { 
		i("s5082 Error open %s exit\n", vfpath.c_str() );
		exit( 11 );
	}

	// get all ips in cluster.conf
	char line[2048];
	int len, first = true, cnt = 0, isPound = false;
	jagint cnum = 0, cpos = -1;
	Jstr sline;
	memset( line, 0, 2048 );

	JagHashMap<AbaxString, jagint> checkMap;
	JagHashMap<AbaxString, jagint> clusterNumMap;

	JagVector<Jstr> checkVec;
	JagVector<Jstr> nicvec;
	JagNet::getLocalIPs( nicvec );
	Jstr  curHost = JagNet::getLocalHost();
	Jstr curHostIP = JagNet::getIPFromHostName( curHost );
	Jstr ip;
	_numAllNodes = 0;

	// d("s1800 nicvec:\n" );
	// nicvec.printString();
	while ( NULL != (fgets( line, 2048, fv ) ) ) {
        if ( strstr( line, "###" ) ) continue;
		if ( strchr( line, '@' ) ) {
			// next cluster
			checkVec.append( "@" );
			if ( first ) first = false;
			continue;
		}

		len = strlen( line );
		if ( len < 2 ) { continue; }
		sline = trimTailLF( line );
		JagStrSplit split( sline, ' ', true );
		if ( split[0].size() < 1 ) continue;

		if ( first ) {
			checkVec.append( "@" );
			first = false;
		}

		if ( split[0] == "localhost" || split[0] == "127.0.0.1" ) {
			i("E3930 localhost or 127.0.0.1 cannot exist in conf/cluster.conf, please fix and restart exit 33\n" );
			exit(33);
		} 
		
		ip = JagNet::getIPFromHostName(  split[0] );
		if ( ip.length() < 2 ) {
			i("E9293 cannot resolve IP address of host %s exit 21 ...\n", split[0].c_str() );
			i("Please fix conf/cluster.conf and restart\n" );
			exit(21);
		}

		if ( ! checkVec.exist(ip) ) { 
            checkVec.append( ip ); 
        }
		memset( line, 0, 2048 );
	}
   	jagfclose( fv );

	// checkVec has all IP addresses now, and '#' between clusters

	// get self host ip
	Jstr up1, up2;
	cnt = 0;
	for ( int i = 0; i < checkVec.size(); ++i ) {
		if ( nicvec.exist( checkVec[i] ) || checkVec[i] == curHostIP ) {
			if ( _selfIP.size() < 1 ) _selfIP = checkVec[i];
			++cnt;
		}
	}

	if ( cnt > 1 ) { 
		i("Too many current host in conf/cluster.conf Please fix and restart exit 18\n");
		exit( 18 );
	} else if ( cnt < 1 ) {
		if ( checkVec.size() > 0 || _listenIP.size() < 1 ) {
			i("No current host is provided in cluster.conf or server.conf, exit 19\n");
			exit( 19 );
		} else {
			checkVec.append( "@" );
			checkVec.append( _listenIP );
			_selfIP = _listenIP;
		}
	} else {
		if ( _listenIP.size() > 0 && _listenIP != _selfIP ) {
			i("listen ip [%s] is not the same as self ip [%s] in conf/cluster.conf  exit 16\n", _listenIP.c_str(), _selfIP.c_str());
			exit( 16 );
		}
	}

	// set isHost0Cluster0 flag
	for ( int i = 0; i < checkVec.size(); ++i ) {
		if ( ! strchr(checkVec[i].c_str(), '@' ) ) {
			if ( checkVec[i] == _selfIP ) {
				_isHost0OfCluster0 = 1;
			} else {
				_isHost0OfCluster0 = 0;
			}
			break;
		}
	}
	
	// format _sendAllNodes  "#ip1|ip2|ip3#ip4|ip5"
	// and _allNodes "ip1|ip2|ip3|ip4|ip5"
	first = true;
	Jstr tok; // IP or #
	for ( int i = 0; i < checkVec.size(); ++i ) {
		tok = checkVec[i];
		if ( strchr(tok.c_str(), '@' ) ) {
			_sendAllNodes += "#"; 
			isPound = true;
			cnum = i;
			++cpos;
		} else {
			if ( isPound ) {
				_sendAllNodes += tok;
				isPound = false;
			} else {
				_sendAllNodes += Jstr("|") + tok;
			}

			if ( first ) {
				_allNodes += tok;
				first = false;
			} else {
				_allNodes += Jstr("|") + tok;
			}
			++ _numAllNodes;

			checkMap.addKeyValue( AbaxString(tok), cnum );
			clusterNumMap.addKeyValue( AbaxString(tok), cpos );
		}
	}

	// checkVec has all IP addresses
	// last, format current cluster node string
	// _hostClusterNodes: "ip4|ip5|ip6"
	checkMap.getValue( AbaxString(_selfIP), cnum );
	clusterNumMap.getValue( AbaxString(_selfIP), _hostClusterNumber );
	first = true;
	_numCurNodes = 0;
	for ( int i = cnum+1; i < checkVec.size(); ++i ) {
		if ( strchr(checkVec[i].c_str(), '@' ) ) {
			// next cluster, break
			break;
		} else {
			if ( first ) {
				_hostClusterNodes = checkVec[i];
				first = false;
			} else {
				_hostClusterNodes += Jstr("|") + checkVec[i];
			}
			++ _numCurNodes;
		}
	}

    i("Nodemgr::init()  I am host=[%s]\n", _selfIP.s() );

	// count totalClusterNumber  below is 2 clusters
	// # iddid
	// @ kdkd
	//  ip1
	//  ip2
	// @ idid
	//  ip3
	//  ip4
	// # dd
	_totalClusterNumber = 0;
	int inComment = 0;
	for ( int i = 0; i < checkVec.size(); ++i ) {
		if (  strchr( checkVec[i].c_str(), '@' ) ) {
			inComment = 1;
		} else {
			if ( inComment ) {
				++ _totalClusterNumber;
			} 
			inComment = 0;
		}
	}

}

// public methods
// rename original file to .backup, write newhost list to file, then redo init
// str has the form of: #\nip1\nip2\n#\nip3\nip4\n...
void JagNodeMgr::refreshClusterFile( Jstr &hoststr )
{
	Jstr vfpath = jaguarHome() + "/conf/cluster.conf";
	Jstr bvfpath = vfpath + ".backup";
	jagrename( vfpath.c_str(), bvfpath.c_str() );
	FILE *fv = jagfopen( vfpath.c_str(), "w" );
	if ( ! fv ) { 
		i("s5083 Error open %s exit\n", vfpath.c_str() );
		exit( 21 );
	}
	fprintf(fv, "%s", hoststr.c_str());
	jagfclose( fv );

	// if cluster.conf changes, require do sshsetup in install script
	Jstr setup = Jstr(getenv("HOME")) + "/.jagsetupssh";
	jagunlink( setup.c_str() );

	init();
}

bool JagNodeMgr::validNode( const Jstr& node )
{
	JagStrSplit sp( _allNodes, '|' );
	for ( int i = 0; i < sp.length(); ++i ) {
		if ( sp[i] == node ) {
			return true;
		}
	}

	return false;
}

Jstr JagNodeMgr::getClusterHosts()
{
	Jstr vfpath;
	vfpath = jaguarHome() + "/conf/cluster.conf";
	FILE *fv = jagfopen( vfpath.c_str(), "r" );
	if ( ! fv ) { 
        return "";
	}

	// get all ips in cluster.conf
	char line[256];
	memset( line, 0, 256 );

    int clusterNum = -1;
    JagVector< JagVector<Jstr> >  clusterVec;

    JagVector<Jstr> cvec;
    clusterVec.push_back( cvec );

	while ( NULL != (fgets( line, 256, fv ) ) ) {
        if ( strstr( line, "###" ) ) continue;
        if ( strlen(line) < 4 ) continue;
		if ( strchr( line, '@' ) ) {
            ++ clusterNum;
            if ( clusterNum >= 1 ) {
                JagVector<Jstr> cvec;
                clusterVec.push_back( cvec );
            }
			continue;
		}

        if ( clusterNum < 0 ) {
            clusterNum = 0;
        }

        clusterVec[clusterNum].push_back( line );
        // newline at end
		memset( line, 0, 256 );
	}
   	jagfclose( fv );

    Jstr res;
    res += Jstr("JaguarDB currently has ") + intToStr(clusterVec.size()) + " cluster(s)\n";
    res += Jstr("----------------------------------------------------------\n");

    for ( int c = 0; c < clusterVec.size(); ++ c ) {
        res += Jstr("Cluster ") + intToStr(c+1) + ": \n";
        for ( int j = 0; j < clusterVec[c].size(); ++j ) {
            res += Jstr("    ") + clusterVec[c][j];
        }
        res += Jstr("\n");
    }

    res += Jstr("----------------------------------------------------------\n");
    res += Jstr("Client is now connected to server ") + _selfIP + "\n";

    return res;
}
