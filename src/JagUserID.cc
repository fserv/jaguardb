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

#include <JagFixKV.h>
#include <JagUserID.h>
#include <JagDBServer.h>

// ctor
JagUserID::JagUserID( int replicType ) :JagFixKV( "system", "UserID", replicType )
{
}

// dtor
JagUserID::~JagUserID()
{
	// this->destroy();
}


// PASS: password; PERM: role:  READ/WRITE  READ: read-only    WRITE: read and write
// PERM:  ADMIN or USER
bool JagUserID::addUser( const AbaxString &userid, const AbaxString& passwd, 
	  const AbaxString& role, const AbaxString &perm )
{
	JagReadWriteMutex mutex( _lock, JagReadWriteMutex::WRITE_LOCK );

	JagRecord  record;
	record.addNameValue( JAG_PASS, passwd.c_str() );
	record.addNameValue( JAG_ROLE, role.c_str() );
	record.addNameValue( JAG_PERM, perm.c_str() );
	_hashmap->addKeyValue( userid, record.getSource() ); 

    // char kv[ KVLEN + 1];
    char *kv = (char*)jagmalloc(KVLEN+1);
    memset(kv, 0, KVLEN + 1 );
    JagDBPair pair;
    pair.point( kv, KLEN, kv+KLEN, VLEN );
	strcpy( kv, userid.c_str() );
    strcpy( kv+KLEN, record.getSource() );
    // int rc = _darr->insert( pair, insertCode, false, true, retpair );
    _darr->insert( pair );
	// printf("s4821 addUser _darr->insert rc=%d\n", rc );
	free( kv );
	return 1;
}

bool JagUserID::isAuth( char op,  const Jstr &dbname, 
						const Jstr &tabname, const Jstr &uid )
{

	AbaxString role = this->getValue( uid, JAG_ROLE );
	AbaxString perm = this->getValue( uid, JAG_PERM );

	// admin user
	if ( role == JAG_ADMIN ) {
		if ( op == 'W' &&  perm != JAG_WRITE ) {
			return false;
		}
		return true;
	}

	// regular user
	if ( dbname == "system" && role != JAG_ADMIN ) {
		return false;
	}

	if ( op == 'W' &&  perm != JAG_WRITE ) {
		return false;
	}

	return true;
}

bool JagUserID::dropUser( const AbaxString &userid )
{
	return dropKey( userid );
}

Jstr JagUserID::getListUsers()
{
	return this->getListKeys();
}

bool JagUserID::exist( const AbaxString &userid )
{
	return _hashmap->keyExist( userid );
}

