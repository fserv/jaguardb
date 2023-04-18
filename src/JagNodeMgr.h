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
#ifndef _jag_node_mgr_h_
#define _jag_node_mgr_h_

#include <abax.h>
#include <JagVector.h>

class JagNodeMgr
{
  public:
  	JagNodeMgr( const Jstr& listenIP ) { _listenIP = listenIP; init(); }
  	~JagNodeMgr() { destroy(); }
	void refreshClusterFile( Jstr &hostlist );
	bool validNode( const Jstr& node );
    Jstr getClusterHosts();


	Jstr                  	_listenIP;
	bool					_isHost0OfCluster0;
	Jstr					_selfIP;
	Jstr					_sendAllNodes; // format of cluster & nodes to other servers
	Jstr					_allNodes;
	Jstr					_hostClusterNodes;
	jagint					_hostClusterNumber;
	jagint					_totalClusterNumber;
	int                     _numCurNodes;
	int                     _numAllNodes;

  protected:
  	void init();
	void destroy() {}
};

#endif
