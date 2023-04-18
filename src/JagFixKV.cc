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
#include <JagDBServer.h>
#include <JDFS.h>
#include <JagDBConnector.h>
#include <JagUtil.h>

// ctor
JagFixKV::JagFixKV( const Jstr &dbname, const Jstr & tabname, int replicType )
{
	_darr = NULL;
	_hashmap = NULL;
	_dbname = dbname;
	_tabname = tabname;
	//_servobj = servobj;
	_replicType = replicType;

    KLEN = _onerecord.keyLength = 256;
    VLEN = _onerecord.valueLength = 4096;
	KVLEN = KLEN + VLEN;
    _lock = newJagReadWriteLock();

	init();
}

// dtor
JagFixKV::~JagFixKV()
{
	destroy();
}

void JagFixKV::init( )
{
    Jstr fpath;
    Jstr jagdatahome = JagCfg::getJDBDataHOME( _replicType );
    fpath = jagdatahome + "/" + _dbname + "/" + _tabname;

    _darr = new JagLocalDiskHash( fpath, KLEN, VLEN );
	int fd = _darr->getFD();

	_hashmap = new JagHashMap<AbaxString, AbaxString>();
	bool rc;
	char *k = (char*) jagmalloc ( KLEN+1 );
	char *v = (char*) jagmalloc ( VLEN+1 );
	char *kv = (char*) jagmalloc ( KLEN+VLEN+1 );
	memset( k, 0, KLEN + 1 );
	memset( v, 0, VLEN + 1 );
	memset( kv, 0, KLEN + VLEN + 1 );
	// jagint length = _darr->_garrlen;
	jagint length = _darr->getLength();
	// printf("s3030 fpath=[%s] length = %lld\n", fpath.c_str(), length ); 
	// JagBuffReader nti( _darr->_jdfs, length, KLEN, VLEN, 0, 0 );
	//JagBuffReader nti( _darr, length, KLEN, VLEN, 0, 0, 1 );
	JagSingleBuffReader nti( fd, length, KLEN, VLEN, 0, 0, 1 );
    while ( true ) {
		memset( k, 0, KLEN );
		memset( v, 0, VLEN );
		rc = nti.getNext(kv);
		if ( !rc ) { break; }
		memcpy( k, kv, KLEN );
		memcpy( v, kv+KLEN, VLEN );
		AbaxString key( k );
		AbaxString val( v );
		_hashmap->addKeyValue( key, val );
		// printf("s3031 addkey=[%s] val=[%s]\n", key.c_str(), val.c_str());
	}
	free( kv );
	free( v );
	free( k );

}


void JagFixKV::destroy( bool removelock )
{
	if ( _darr ) {
		delete _darr;
	}
	_darr = NULL;

	if ( _hashmap ) {
		delete _hashmap;
	}
	_hashmap = NULL;

	if ( _lock && removelock ) {
		//delete _lock;
		deleteJagReadWriteLock( _lock );
		_lock = NULL;
	}

}

// get short name from long name , fwd mappping
void JagFixKV::refresh( )
{
	JAG_BLURT JagReadWriteMutex mutex( _lock, JagReadWriteMutex::WRITE_LOCK ); JAG_OVER;
	destroy( false );
	init( );
}

// get short name from long name , fwd mappping
AbaxString JagFixKV::getValue( const AbaxString &userid, const AbaxString& type ) const
{
	// JagReadWriteMutex mutex( _lock, JagReadWriteMutex::READ_LOCK );
	JagReadWriteMutex mutex( _lock, JagReadWriteMutex::WRITE_LOCK );
	AbaxString rec;

	bool rc = _hashmap->getValue( userid, rec );
	// printf("s7738 userid _hashmap->getValue( userid=[%s] rc=%d\n", userid.c_str(), rc );
	if ( ! rc ) {
		return "";
	}

	JagRecord  record;
	record.setSource( rec.c_str() );
	char *p = record.getValue( type.c_str() );
	if ( ! p ) {
		return "";
	}
	AbaxString value = p;
	if ( p ) free( p );
	p = NULL;

	return value;
}

bool JagFixKV::setValue( const AbaxString &userid, const AbaxString &name, const AbaxString& value )
{
	AbaxString rec;
	JagReadWriteMutex mutex( _lock, JagReadWriteMutex::WRITE_LOCK );
    JagRecord  record;

    // char kv[ KVLEN + 1];
    char *kv = (char*) jagmalloc ( KVLEN+1 );
    memset(kv, 0, KVLEN + 1 );
    strcpy( kv, userid.c_str() );
    JagDBPair pair;
    pair.point( kv, KLEN, kv+KLEN, VLEN );
    JagDBPair getPair(kv, KLEN, kv+KLEN, VLEN );
    bool rc = _darr->get( getPair );
	//int insertCode;
    if ( ! rc ) {
        record.addNameValue( name.c_str(), value.c_str() );
        strcpy( kv+KLEN, record.getSource() );
        // _darr->insert( pair, insertCode, false, true, retpair );
        // _darr->insertData( pair, insertCode, false, retpair );
        _darr->insert( pair );
    } else {
        record.setSource( pair.value.c_str() );
        record.setValue( name.c_str(),  value.c_str() );
        strcpy( kv+KLEN, record.getSource() );
        _darr->set( pair );
	}

	_hashmap->setValue( userid, record.getSource(), true ); 
	free ( kv );

	return 1;
}

bool JagFixKV::dropKey( const AbaxString &key, bool doLock )
{
	JagReadWriteMutex *mutex = NULL;
	if ( doLock ) {
		mutex = new JagReadWriteMutex( _lock, JagReadWriteMutex::WRITE_LOCK );
	}
    // char k[ KLEN + 1];
	bool rc = false;
    char *k = (char*) jagmalloc ( KLEN+1 );
    memset(k, 0, KLEN + 1 );
    strcpy( k, key.c_str() );
    JagDBPair pair( k, KLEN );
    _darr->remove( pair );

	rc = _hashmap->removeKey( key ); 
	free ( k );

	if ( doLock ) {
		delete mutex;
	}
	return rc;
}

Jstr JagFixKV::getListKeys()
{
	JagReadWriteMutex mutex( _lock, JagReadWriteMutex::READ_LOCK );
	return _darr->getListKeys();
}

