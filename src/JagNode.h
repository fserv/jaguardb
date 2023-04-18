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
#ifndef _jag_node_h_
#define _jag_node_h_

#include <JagRecord.h>
#include <JagDBServer.h>
#include <JagFixKV.h>

#define JAG_USED  "U"
#define JAG_FREE  "F"
#define JAG_REG   "R"
#define JAG_STAT  "S"

class JagDBServer;

//////// nodeid -->  [JagRecord:   PASS for password ROLE: ADMIN/USER   PERM: role READ/WRITE ]
class JagNode : public JagFixKV
{
  public:

    // JagNode( JagDBServer *servobj=NULL );
    JagNode();
	bool 	addNode( const AbaxString &nodeid, const AbaxString& usedGB, const AbaxString& freeGB );
	bool 	dropNode( const AbaxString &nodeid ); 
	Jstr  getListNodes();

	~JagNode();

  protected:
};

#endif
