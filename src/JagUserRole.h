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
#ifndef _jag_userrole_h_
#define _jag_userrole_h_

#include <JagRecord.h>
#include <JagMutex.h>
#include <JagFixKV.h>

class JagDBServer;

//////// userid|db|tab|col|perm --> [filter where]
class JagUserRole : public JagFixKV
{
  public:

    JagUserRole( int replicType=0 );
	virtual ~JagUserRole();
	Jstr  getListRoles();
	bool 	addRole( const AbaxString &userid, const AbaxString& db, const AbaxString& tab, const AbaxString& col, 
					 const AbaxString& role, const AbaxString &rowfilter );
	bool 	dropRole( const AbaxString &userid, const AbaxString& db, const AbaxString& tab, const AbaxString& col, const AbaxString& op ); 
	bool 	checkUserCommandPermission( const JagSchemaRecord *srec, const JagRequest &req, 
										const JagParseParam &parseParam, int i, Jstr &rowFilter, Jstr& errmsg );
	bool    isAuthed( const Jstr &op, const Jstr &userid, const Jstr &db, const Jstr &tab,
                 	  const Jstr &col, Jstr &rowFilter );

	Jstr showRole( const AbaxString &userid );

	protected:
	  void getDbTabCol( const JagParseParam &parseParam, int i, const JagStrSplit &sp2,
	  					Jstr &db, Jstr &tab, Jstr &col );
	  Jstr getError( const Jstr &code, const Jstr &action, 
	  				 const Jstr &db, const Jstr &tab, const Jstr &col, const Jstr &uid );



};

#endif
