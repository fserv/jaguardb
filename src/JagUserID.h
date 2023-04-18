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
#ifndef _jag_userid_h_
#define _jag_userid_h_

#include <JagRecord.h>
#include <JagMutex.h>
#include <JagFixKV.h>

#define JAG_ADMIN  "ADMIN"
#define JAG_USER  "USER"
#define JAG_READ  "READ"
#define JAG_WRITE  "WRITE"
#define JAG_PASS  "PASS"
#define JAG_PERM  "PERM"
#define JAG_ROLE  "ROLE"

class JagDBServer;

//////// userid -->  [JagRecord:   PASS for password ROLE: ADMIN/USER   PERM: role READ/WRITE ]
class JagUserID : public JagFixKV
{
  public:

    JagUserID( int replicType=0 );
	virtual ~JagUserID();
	bool 	addUser( const AbaxString &userid, const AbaxString& passwd, 
				const AbaxString &role, const AbaxString& permission );
	bool    isAuth( char  op, const Jstr &dbname, 
					const Jstr &tabname, const Jstr &uid );
	bool 	dropUser( const AbaxString &userid ); 
	bool 	exist( const AbaxString &userid ); 
	Jstr  	getListUsers();

};

#endif
