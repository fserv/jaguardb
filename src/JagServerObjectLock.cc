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
#include <JagServerObjectLock.h>
#include <JagTableSchema.h>
#include <JagIndexSchema.h>
#include <JagTable.h>
#include <JagIndex.h>
#include <JagHashMap.h>
#include <JagHashLock.h>
#include <JagDBServer.h>

JagServerObjectLock::JagServerObjectLock( const JagDBServer *servobj )
: _servobj ( servobj )
{
	//_hashLock = newObject<JagHashLock>();
	_hashLock = new JagHashLock();
	initObjects();
}

JagServerObjectLock::~JagServerObjectLock()
{
	cleanupObjects();
	if ( _hashLock ) delete _hashLock;	
}

void JagServerObjectLock::initObjects()
{
	_databases = new JagHashMap<AbaxString, AbaxBuffer>();
	_prevdatabases = new JagHashMap<AbaxString, AbaxBuffer>();
	_nextdatabases = new JagHashMap<AbaxString, AbaxBuffer>();
	_tables = new JagHashMap<AbaxString, AbaxBuffer>();
	_prevtables = new JagHashMap<AbaxString, AbaxBuffer>();
	_nexttables = new JagHashMap<AbaxString, AbaxBuffer>();
	_indexs = new JagHashMap<AbaxString, AbaxBuffer>();
	_previndexs = new JagHashMap<AbaxString, AbaxBuffer>();
	_nextindexs = new JagHashMap<AbaxString, AbaxBuffer>();
	_idxtableNames = new JagHashMap<AbaxString, AbaxString>();

	AbaxBuffer bfr; 
	Jstr dbName = "system";
	Jstr dbName2 = "test";
	_databases->addKeyValue( dbName, bfr );  
	_databases->addKeyValue( dbName2, bfr );

	_prevdatabases->addKeyValue( dbName, bfr ); 
	_prevdatabases->addKeyValue( dbName2, bfr );

	_nextdatabases->addKeyValue( dbName, bfr ); 
	_nextdatabases->addKeyValue( dbName2, bfr );
}

void JagServerObjectLock::setInitDatabases( const Jstr &dblist, int replicType )
{
	AbaxBuffer bfr;
	JagHashMap<AbaxString, AbaxBuffer> *dbmap = NULL;
	if ( JAG_MAIN == replicType ) {
		dbmap = _databases;
	} else if ( JAG_PREV == replicType ) {
		dbmap = _prevdatabases;
	} else if ( JAG_NEXT == replicType ) {
		dbmap = _nextdatabases;
	} else {
		return;
	}

	JagStrSplit sp( dblist.c_str(), '|', true );
	for ( int i = 0; i < sp.length(); ++i ) {
		dbmap->addKeyValue( sp[i], bfr );
	}
}

void JagServerObjectLock::cleanupObjects()
{
	const AbaxPair<AbaxString, AbaxBuffer> *arr; 
    jagint len, i;
	
	arr = _tables->array(); 
    len = _tables->arrayLength();
	for ( i = 0; i < len; ++i ) {
		if ( arr[i].key.size() > 0 ) {
			JagTable *p = (JagTable*) arr[i].value.addr();
			if ( p ) delete p;
		}
	}

	arr = _prevtables->array(); 
    len = _prevtables->arrayLength();
	for ( i = 0; i < len; ++i ) {
		if ( arr[i].key.size() > 0 ) {
			JagTable *p = (JagTable*) arr[i].value.addr();
			if ( p ) delete p;
		}
	}
	
	arr = _nexttables->array(); 
    len = _nexttables->arrayLength();
	for ( i = 0; i < len; ++i ) {
		if ( arr[i].key.size() > 0 ) {
			JagTable *p = (JagTable*) arr[i].value.addr();
			if ( p ) delete p;
		}
	}
	
	arr = _indexs->array(); 
    len = _indexs->arrayLength();
	for ( i = 0; i < len; ++i ) {
		if ( arr[i].key.size() > 0 ) {
			JagIndex *p = (JagIndex*) arr[i].value.addr();
			if ( p ) delete p;
		}
	}
	
	arr = _previndexs->array(); 
    len = _previndexs->arrayLength();
	for ( i = 0; i < len; ++i ) {
		if ( arr[i].key.size() > 0 ) {
			JagIndex *p = (JagIndex*) arr[i].value.addr();
			if ( p ) delete p;
		}
	}
	
	arr = _nextindexs->array(); 
    len = _nextindexs->arrayLength();
	for ( i = 0; i < len; ++i ) {
		if ( arr[i].key.size() > 0 ) {
			JagIndex *p = (JagIndex*) arr[i].value.addr();
			if ( p ) delete p;
		}
	}

	if ( _idxtableNames ) delete _idxtableNames;
	if ( _indexs ) delete _indexs;
	if ( _previndexs ) delete _previndexs;
	if ( _nextindexs ) delete _nextindexs;	
	if ( _tables ) delete _tables;
	if ( _prevtables ) delete _prevtables;
	if ( _nexttables ) delete _nexttables;
	if ( _databases ) delete _databases;
	if ( _prevdatabases ) delete _prevdatabases;
	if ( _nextdatabases ) delete _nextdatabases;
}

void JagServerObjectLock::rebuildObjects()
{
    dnlock("lock35008 rebuildObjects() ...");
	cleanupObjects();
	initObjects();
}

jagint JagServerObjectLock::getnumObjects( int objType, int replicType )
{
	if ( 0 == objType ) {
		if ( 0 == replicType ) {
			return _databases->size();
		} else if ( 1 == replicType ) {
			return _prevdatabases->size();
		} else if ( 2 == replicType ) {
			return _nextdatabases->size();
		}
	} else if ( 1 == objType ) {
		if ( 0 == replicType ) {
			return _tables->size();
		} else if ( 1 == replicType ) {
			return _prevtables->size();
		} else if ( 2 == replicType ) {
			return _nexttables->size();
		}
	} else if ( 2 == objType ) {
		if ( 0 == replicType ) {
			return _indexs->size();
		} else if ( 1 == replicType ) {
			return _previndexs->size();
		} else if ( 2 == replicType ) {
			return _nextindexs->size();
		}
	}
	return 0;
}

int JagServerObjectLock::readLockSchema( int replicType )
{
	Jstr repstr;
	if ( replicType < 0 ) {
		repstr = "0";
 		JAG_BLURT _hashLock->readLock( repstr ); JAG_OVER
		repstr = "1";
 		JAG_BLURT _hashLock->readLock( repstr ); JAG_OVER
		repstr = "2";
 		JAG_BLURT _hashLock->readLock( repstr ); JAG_OVER
	} else {
		repstr = intToStr( replicType );
 		JAG_BLURT _hashLock->readLock( repstr ); JAG_OVER
	}
	return 1;
}
int JagServerObjectLock::readUnlockSchema( int replicType )
{
	Jstr repstr;
	if ( replicType < 0 ) {
		repstr = "0";
 		_hashLock->readUnlock( repstr );
		repstr = "1";
 		_hashLock->readUnlock( repstr );
		repstr = "2";
		_hashLock->readUnlock( repstr );
	} else {
		repstr = intToStr( replicType );
 		_hashLock->readUnlock( repstr );
	}
	return 1;
}

int JagServerObjectLock::writeLockSchema( int replicType )
{
	Jstr repstr;

	if ( replicType < 0 ) {
		repstr = "0";
        dnlock("lock028281 writeLockSchema  writeLock 0" );
 		JAG_BLURT _hashLock->writeLock( repstr ); JAG_OVER

		repstr = "1";
        dnlock("lock028281 writeLockSchema  writeLock 1" );
 		JAG_BLURT _hashLock->writeLock( repstr ); JAG_OVER

		repstr = "2";
        dnlock("lock028281 writeLockSchema  writeLock 2" );
 		JAG_BLURT _hashLock->writeLock( repstr ); JAG_OVER
	} else {
		repstr = intToStr( replicType );
        dnlock("lock028288 writeLockSchema  writeLock %d", replicType );
 		JAG_BLURT _hashLock->writeLock( repstr ); JAG_OVER
	}
	return 1;
}
int JagServerObjectLock::writeUnlockSchema( int replicType )
{
	Jstr repstr;
	if ( replicType < 0 ) {
		repstr = "0";
 		_hashLock->writeUnlock( repstr );
		repstr = "1";
 		_hashLock->writeUnlock( repstr );
		repstr = "2";
		_hashLock->writeUnlock( repstr );
	} else {
		repstr = intToStr( replicType );
 		_hashLock->writeUnlock( repstr );
	}
	return 1;
}

int JagServerObjectLock::readLockDatabase( jagint opcode, const Jstr &dbName, int replicType )
{
	Jstr repstr = intToStr( replicType );
	AbaxString lockdbrep = dbName + "." + repstr;
	JagHashMap<AbaxString, AbaxBuffer> *dbmap = NULL;

	if ( 0 == replicType ) {
		dbmap = _databases;
	} else if ( 1 == replicType ) {
		dbmap = _prevdatabases;
	} else if ( 2 == replicType ) {
		dbmap = _nextdatabases;
	} else {
		return -10;
	}
	
    /**
	if ( !dbmap->keyExist( dbName ) ) return 0; 
    **/

	JAG_BLURT _hashLock->readLock( repstr ); JAG_OVER
	JAG_BLURT _hashLock->readLock( lockdbrep ); JAG_OVER

	if ( !dbmap->keyExist( dbName ) ) {
		_hashLock->readUnlock( lockdbrep );
		_hashLock->readUnlock( repstr );
		return -20;
	}
	return 10;
}
int JagServerObjectLock::readUnlockDatabase( jagint opcode, const Jstr &dbName, int replicType )
{
	Jstr repstr = intToStr( replicType );
	AbaxString lockdbrep = dbName + "." + repstr;

	_hashLock->readUnlock( lockdbrep );
	_hashLock->readUnlock( repstr );
	return 1;
}

int JagServerObjectLock::writeLockDatabase( jagint opcode, const Jstr &dbName, int replicType )
{
	AbaxBuffer  bfr;
	Jstr        repstr = intToStr( replicType );
	AbaxString  lockdbrep = dbName + "." + repstr;
	JagHashMap<AbaxString, AbaxBuffer> *dbmap = NULL;

	if ( 0 == replicType ) {
		dbmap = _databases;
	} else if ( 1 == replicType ) {
		dbmap = _prevdatabases;
	} else if ( 2 == replicType ) {
		dbmap = _nextdatabases;
	} else {
		return -10;
	}
	
    /***
	if ( JAG_CREATEDB_OP == opcode ) {
		if ( dbmap->keyExist( dbName ) ) {
			return 10;
		}
	} else {
		if ( !dbmap->keyExist( dbName ) ) {
			return -20;
		}
	}
    ***/

	JAG_BLURT _hashLock->readLock( repstr ); JAG_OVER

    dnlock("lock0100129 writeLock %s ...", lockdbrep.s() );
	JAG_BLURT _hashLock->writeLock( lockdbrep ); JAG_OVER
    dnlock("lock0100129 writeLock %s done", lockdbrep.s() );

	if ( JAG_CREATEDB_OP == opcode ) {
		if ( dbmap->keyExist( dbName ) ) {
			_hashLock->writeUnlock( lockdbrep );
            dnlock("lock30012 writeUnlock %s", lockdbrep.s() );
			_hashLock->readUnlock( repstr );
			return 30;
		} else {
			if ( !dbmap->addKeyValue( dbName, bfr ) ) {
				_hashLock->writeUnlock( lockdbrep );
                dnlock("lock30112 writeUnlock %s", lockdbrep.s() );
				_hashLock->readUnlock( repstr );
				return -30;
			}
		}
	} else {
		if ( !dbmap->keyExist( dbName ) ) {
			_hashLock->writeUnlock( lockdbrep );
            dnlock("lock30115 writeUnlock %s", lockdbrep.s() );
			_hashLock->readUnlock( repstr );
			return -40;
		} else {
		}
	}

	return 100;
}

int JagServerObjectLock::writeUnlockDatabase( jagint opcode, const Jstr &dbName, int replicType )
{
	Jstr repstr = intToStr( replicType );
	AbaxString lockdbrep = dbName + "." + repstr;
	JagHashMap<AbaxString, AbaxBuffer> *dbmap = NULL;

	if ( 0 == replicType ) {
		dbmap = _databases;
	} else if ( 1 == replicType ) {
		dbmap = _prevdatabases;
	} else if ( 2 == replicType ) {
		dbmap = _nextdatabases;
	} else {
		return -1;
	}
	
	if ( JAG_DROPDB_OP == opcode ) {
		if ( !dbmap->removeKey( dbName ) ) {
			_hashLock->writeUnlock( lockdbrep );
            dnlock("lock30118 writeUnlock %s", lockdbrep.s() );
			_hashLock->readUnlock( repstr );
			return -10;
		} else {
			_hashLock->writeUnlock( lockdbrep );
            dnlock("lock30119 writeUnlock %s", lockdbrep.s() );
			_hashLock->readUnlock( repstr );	
			return 10;
		}
	} else {
		_hashLock->writeUnlock( lockdbrep );
        dnlock("lock30149 writeUnlock %s", lockdbrep.s() );
		_hashLock->readUnlock( repstr );
		return -100;
	}

	return 1;	
}

JagTable *JagServerObjectLock::readLockTable( jagint opcode, const Jstr &dbName, 
							   const Jstr &tableName, int replicType, 
							   bool lockSelfLevelOnly, int &rc )
{
	Jstr repstr = intToStr( replicType );
	AbaxString lockdbrep = dbName + "." + repstr;
	AbaxString dbtab = dbName + "." + tableName;
	AbaxString lockdbtabrep = dbtab + "." + repstr;

	JagHashMap<AbaxString, AbaxBuffer> *dbmap = NULL, *tabmap = NULL;

    dnlock("s445998 enter readLockTable lockSelfLevelOnly=%drepstr=[%s] lockdbrep=[%s] dbtab=[%s] lockdbtabrep=[%s]", 
        lockSelfLevelOnly, repstr.s(), lockdbrep.s(), dbtab.s(), lockdbtabrep.s() );

	if ( 0 == replicType ) {
		dbmap = _databases;
		tabmap = _tables;
	} else if ( 1 == replicType ) {
		dbmap = _prevdatabases;
		tabmap = _prevtables;
	} else if ( 2 == replicType ) {
		dbmap = _nextdatabases;
		tabmap = _nexttables;
	} else {
		rc = -1;
		return NULL;
	}

    dnlock("s24803 readLockTable replicType=%d tabmap=%p", replicType, tabmap );

    /***
	if ( !dbmap->keyExist( dbName ) ) {
		rc = -10;
		return NULL; 
	}

	if ( !tabmap->keyExist( lockdbtabrep ) ) {
		rc = -20;
		return NULL; 
	}
    ***/


	if ( !lockSelfLevelOnly ) {
        dnlock("lock10020 readLock repstr=%s ...", repstr.s() );
		JAG_BLURT _hashLock->readLock( repstr ); JAG_OVER
        dnlock("lock10020 readLock repstr=%s done", repstr.s() );

        dnlock("lock10023 readLock lockdbrep=%s ...", lockdbrep.s() );
		JAG_BLURT _hashLock->readLock( lockdbrep ); JAG_OVER
        dnlock("lock10023 readLock lockdbrep=%s done", lockdbrep.s() );
	}

    dnlock("lock30012 readLock lockdbtabrep=%s ...", lockdbtabrep.s() );
	JAG_BLURT _hashLock->readLock( lockdbtabrep ); JAG_OVER
    dnlock("lock30012 readLock lockdbtabrep=%s done", lockdbtabrep.s() );

	if ( !dbmap->keyExist( dbName ) || !tabmap->keyExist( lockdbtabrep ) ) {
		_hashLock->readUnlock( lockdbtabrep );
		if ( !lockSelfLevelOnly ) {
			_hashLock->readUnlock( lockdbrep );
			_hashLock->readUnlock( repstr );
		}
		rc = -30;
		return NULL;
	}

	AbaxBuffer bfr;
	if ( !tabmap->getValue( lockdbtabrep, bfr ) ) {
		_hashLock->readUnlock( lockdbtabrep );
		if ( !lockSelfLevelOnly ) {
			_hashLock->readUnlock( lockdbrep );
			_hashLock->readUnlock( repstr );
		}
		rc = -40;
		return NULL;
	}

	JagTable *ptab = (JagTable*) bfr.addr();
    dnlock("s35001812 readLockTable() retrieved ptab=%p for dbtab=[%s] replicType=%d", ptab, dbtab.s(), replicType );
	rc = 0;
	return ptab;
}

int JagServerObjectLock::readUnlockTable( jagint opcode, const Jstr &dbName, 
	const Jstr &tableName, int replicType, bool lockSelfLevelOnly )
{
	Jstr repstr = intToStr( replicType );
	AbaxString lockdbrep = dbName + "." + repstr;
	AbaxString dbtab = dbName + "." + tableName;
	AbaxString lockdbtabrep = dbtab + "." + repstr;

	_hashLock->readUnlock( lockdbtabrep );
    dnlock("lock312301 readUnlock lockdbtabrep=%s", lockdbtabrep.s() );

	if ( !lockSelfLevelOnly ) {
		_hashLock->readUnlock( lockdbrep );
        dnlock("lock31305 readUnlock lockdb=%s", lockdbrep.s() );

		_hashLock->readUnlock( repstr );
        dnlock("lock31306 readUnlock repstr=%s", repstr.s() );

	}

	return 1;	
}

JagTable *JagServerObjectLock::writeLockTable( jagint opcode, const Jstr &dbName, const Jstr &tableName, 
										       const JagTableSchema *tschema, int replicType, 
											   bool lockSelfLevelOnly, int &rc )
{
	Jstr repstr = intToStr( replicType );
	AbaxString lockdbrep = dbName + "." + repstr;
	AbaxString dbtab = dbName + "." + tableName;
	AbaxString lockdbtabrep = dbtab + "." + repstr;

	JagHashMap<AbaxString, AbaxBuffer> *dbmap = NULL, *tabmap = NULL;

    dnlock("s445996 enter writeLockTable lockSelfLevelOnly=%d repstr=[%s] lockdbrep=[%s] dbtab=[%s] lockdbtabrep=[%s]", 
        lockSelfLevelOnly, repstr.s(), lockdbrep.s(), dbtab.s(), lockdbtabrep.s() );

	if ( 0 == replicType ) {
		dbmap = _databases;
		tabmap = _tables;
	} else if ( 1 == replicType ) {
		dbmap = _prevdatabases;
		tabmap = _prevtables;
	} else if ( 2 == replicType ) {
		dbmap = _nextdatabases;
		tabmap = _nexttables;
	} else {
		dnlock("s4409039 writeLockTable() replicType=%d not known, return NULL", replicType);
		rc = -10;
		return NULL;
	}

    dnlock("s24801 writeLockTable replicType=%d tabmap=%p", replicType, tabmap );

	if ( !lockSelfLevelOnly ) {

        dnlock("s24801 writeLockTable readLock repstr=%s ...", repstr.s() );
		JAG_BLURT _hashLock->readLock( repstr ); JAG_OVER
        dnlock("s24801 writeLockTable readLock repstr=%s done ...", repstr.s() );

        dnlock("s24802 writeLockTable readLock lockdbrep=%s ...", lockdbrep.s() );
		JAG_BLURT _hashLock->readLock( lockdbrep ); JAG_OVER
        dnlock("s24802 writeLockTable readLock lockdbrep=%s done", lockdbrep.s() );
	}

    dnlock("lock011128 writeLock lockdbtabrep=%s", lockdbtabrep.s() );
	JAG_BLURT _hashLock->writeLock( lockdbtabrep ); JAG_OVER	
    dnlock("lock011128 writeLock lockdbtabrep=%s done", lockdbtabrep.s() );

	if ( JAG_CREATETABLE_OP == opcode || JAG_CREATECHAIN_OP == opcode || JAG_CREATEMEMTABLE_OP == opcode ) {
		if ( !dbmap->keyExist( dbName ) ) {

			_hashLock->writeUnlock( lockdbtabrep );
            dnlock("lock002218 writeUnlock %s", lockdbtabrep.s() );

			if ( !lockSelfLevelOnly ) {
				_hashLock->readUnlock( lockdbrep );
				_hashLock->readUnlock( repstr );
			}
			dnlock("s4508039 writeLockTable() replicType=%d, return NULL dbmap has no key [%s]", replicType, dbName.s() );
			rc = -20;
			return NULL;
		} else if ( dbmap->keyExist(dbName) && tabmap->keyExist( lockdbtabrep ) ) {

			_hashLock->writeUnlock( lockdbtabrep );
            dnlock("lock002418 writeUnlock %s", lockdbtabrep.s() );

			if ( !lockSelfLevelOnly ) {
				_hashLock->readUnlock( lockdbrep );
				_hashLock->readUnlock( repstr );
			}
			dnlock("s4528539 replicType=%d, return NULL  dbmap has dbName=[%s] and tabmap has [%s]", replicType, dbName.s(), lockdbtabrep.s() );
			rc = -22;
			return NULL;
		} else {
			const JagSchemaRecord *record = tschema->getAttr( dbtab.c_str() );
			if ( ! record ) {

				_hashLock->writeUnlock( lockdbtabrep );
                dnlock("lock002518 writeUnlock %s", lockdbtabrep.s() );

				if ( !lockSelfLevelOnly ) {
					_hashLock->readUnlock( lockdbrep );
					_hashLock->readUnlock( repstr );
				}
				dnlock("s4508434 replicType=%d, no schema dbtab=[%s], return NULL", replicType, dbtab.s() );
				rc = -30;
				return NULL;
			}

			JagTable *ptab = new JagTable( replicType, _servobj, dbName.c_str(), tableName.c_str(), *record );
            dnlock("lock02192 writeLockTable() replicType=%d dbName=[%s] tableName=[%s] new ptab=%p", replicType, dbName.c_str(), tableName.c_str(), ptab );
			AbaxBuffer bfr = (void*) ptab;
			if ( !tabmap->addKeyValue( lockdbtabrep, bfr ) ) {
				delete ptab;
				_hashLock->writeUnlock( lockdbtabrep );
                dnlock("lock003518 writeUnlock %s", lockdbtabrep.s() );

				if ( !lockSelfLevelOnly ) {
					_hashLock->readUnlock( lockdbrep );
					_hashLock->readUnlock( repstr );
				}
				dnlock("s4302434 replicType=%d, addKeyValue error, lockdbtabrep=[%s], return NULL", replicType, lockdbtabrep.s() );
				rc = -40;
				return NULL;
			}
            dnlock("s33340 writeLockTable() created ptab=%p and added to tabmap for lockdbtabrep=[%s]", ptab, lockdbtabrep.s() );
			return ptab;
		}
	} else {
		if ( !dbmap->keyExist( dbName ) || !tabmap->keyExist( lockdbtabrep ) ) {

			_hashLock->writeUnlock( lockdbtabrep );
            dnlock("lock013518 writeUnlock %s", lockdbtabrep.s() );

			if ( !lockSelfLevelOnly ) {
				_hashLock->readUnlock( lockdbrep );
				_hashLock->readUnlock( repstr );
			}
			dnlock("s4302480 replicType=%d, dbName=[%s] dbtab=[%s] keynotexist, return NULL", replicType, dbName.s(), dbtab.s() );
			rc = -50;
			return NULL;
		}

		AbaxBuffer bfr;
		if ( ! tabmap->getValue( lockdbtabrep, bfr ) ) {

			_hashLock->writeUnlock( lockdbtabrep );
            dnlock("lock043518 writeUnlock %s", lockdbtabrep.s() );

			if ( !lockSelfLevelOnly ) {
				_hashLock->readUnlock( lockdbrep );
				_hashLock->readUnlock( repstr );
			}
			rc = -60;
			return NULL;
		}		

		JagTable *ptab = (JagTable*) bfr.addr();
        dnlock("lock02195 writeLockTable() replicType=%d dbName=[%s] tableName=[%s] retrieved ptab=%p for lockdbtabrep=[%s]", 
            replicType, dbName.c_str(), tableName.c_str(), ptab, lockdbtabrep.s() );
		rc = 0;
		return ptab;
	}
}

int JagServerObjectLock::writeUnlockTable( jagint opcode, const Jstr &dbName, const Jstr &tableName, 
									       int replicType, bool lockSelfLevelOnly )
{
	Jstr repstr = intToStr( replicType );
	AbaxString lockdbrep = dbName + "." + repstr;
	AbaxString dbtab = dbName + "." + tableName;
	AbaxString lockdbtabrep = dbtab + "." + repstr;

	JagHashMap<AbaxString, AbaxBuffer> *dbmap = NULL, *tabmap = NULL;

	if ( 0 == replicType ) {
		dbmap = _databases;
		tabmap = _tables;
	} else if ( 1 == replicType ) {
		dbmap = _prevdatabases;
		tabmap = _prevtables;
	} else if ( 2 == replicType ) {
		dbmap = _nextdatabases;
		tabmap = _nexttables;
	} else {
        dnlock("s20073 return -1");
		return -1;
	}
	
	if ( JAG_DROPTABLE_OP == opcode ) {
		bool rc = tabmap->removeKey( lockdbtabrep );

		_hashLock->writeUnlock( lockdbtabrep );
        dnlock("lock112213 writeUnlock lockdbtabrep=%s", lockdbtabrep.s() );

		if ( !lockSelfLevelOnly ) {
			_hashLock->readUnlock( lockdbrep );
			_hashLock->readUnlock( repstr );
		}	

		if ( ! rc ) {
			return -10;
		} else {
			return 10;
		}
	} else {

		_hashLock->writeUnlock( lockdbtabrep );
        dnlock("lock112280 writeUnlock lockdbtabrep=%s", lockdbtabrep.s() );

		if ( !lockSelfLevelOnly ) {

            dnlock("lock112230 readUnlock lockdbrep=%s", lockdbrep.s() );
			_hashLock->readUnlock( lockdbrep );

            dnlock("lock112232 readUnlock repstr=%s", repstr.s() );
			_hashLock->readUnlock( repstr );
		}

		return 20;
	}

} // end of writeUnlockTable()


JagTable *JagServerObjectLock::getTable( jagint opcode, const Jstr &dbName, const Jstr &tableName, 
										 const JagTableSchema *tschema, int replicType )
{
	Jstr repstr = intToStr( replicType );
	AbaxString dbtab = dbName + "." + tableName;
	AbaxString lockdbtabrep = dbtab + "." + repstr;

    dnlock("s1122029 getTable lockdbtabrep=[%s]", lockdbtabrep.s() );

	JagHashMap<AbaxString, AbaxBuffer> *dbmap = NULL, *tabmap = NULL;

	if ( 0 == replicType ) {
		dbmap = _databases;
		tabmap = _tables;
	} else if ( 1 == replicType ) {
		dbmap = _prevdatabases;
		tabmap = _prevtables;
	} else if ( 2 == replicType ) {
		dbmap = _nextdatabases;
		tabmap = _nexttables;
	} else {
		return NULL;
	}
	
	if ( ! dbmap->keyExist( dbName ) ) {
		return NULL;
	}

	AbaxBuffer bfr;
	if ( ! tabmap->getValue( lockdbtabrep, bfr ) ) {
		if ( JAG_CREATETABLE_OP == opcode ) {
			const JagSchemaRecord *record = tschema->getAttr( dbtab.c_str() );
			if ( ! record ) {
				return NULL;
			}

			JagTable *ptab = new JagTable( replicType, _servobj, dbName.s(), tableName.s(), *record );
			AbaxBuffer bfr = (void*) ptab;
            dnlock("s29393 getTable() make new ptab=%p for lockdbtabrep=[%s]", ptab, lockdbtabrep.s() );
			if ( !tabmap->addKeyValue( lockdbtabrep, bfr ) ) {
				delete ptab;
				return NULL;
			}
			return ptab;
		}
		return NULL;
	}

	JagTable *ptab = (JagTable*) bfr.addr();
    dnlock("s939393 getTable()  got ptab=%p for lockdbtabrep=[%s]", ptab, lockdbtabrep.s() );
	return ptab;
}

JagTable *JagServerObjectLock
::writeTruncateTable( jagint opcode, const Jstr &dbName, const Jstr &tableName, 
				      const JagTableSchema *tschema, int replicType, bool lockSelfLevelOnly )
{
	Jstr repstr = intToStr( replicType );
	AbaxString lockdbrep = dbName + "." + repstr;
	AbaxString dbtab = dbName + "." + tableName;
	AbaxString lockdbtabrep = dbtab + "." + repstr;

	JagHashMap<AbaxString, AbaxBuffer> *dbmap = NULL, *tabmap = NULL;

	if ( 0 == replicType ) {
		dbmap = _databases;
		tabmap = _tables;
	} else if ( 1 == replicType ) {
		dbmap = _prevdatabases;
		tabmap = _prevtables;
	} else if ( 2 == replicType ) {
		dbmap = _nextdatabases;
		tabmap = _nexttables;
	}
	
	if ( JAG_TRUNCATE_OP == opcode ) {
		const JagSchemaRecord *record = tschema->getAttr( dbtab.c_str() );
		if ( !tabmap->removeKey( lockdbtabrep ) || ! record ) {
			_hashLock->writeUnlock( lockdbtabrep );
			if ( !lockSelfLevelOnly ) {
				_hashLock->readUnlock( lockdbrep );
				_hashLock->readUnlock( repstr );
			}
			return NULL;
		}

		JagTable *ptab = new JagTable( replicType, _servobj, dbName.c_str(), tableName.c_str(), *record );
		AbaxBuffer bfr = (void*) ptab;
		if ( !tabmap->addKeyValue( lockdbtabrep, bfr ) ) {
			delete ptab;
			_hashLock->writeUnlock( lockdbtabrep );
            dnlock("lock3222019 writeUnlock %s", lockdbtabrep.s() );
			if ( !lockSelfLevelOnly ) {
				_hashLock->readUnlock( lockdbrep );
				_hashLock->readUnlock( repstr );
			}
			return NULL;
		} else {
			return ptab;
		}
	} else {
		_hashLock->writeUnlock( lockdbtabrep );
        dnlock("lock3222813 writeUnlock %s", lockdbtabrep.s() );

		if ( !lockSelfLevelOnly ) {
			_hashLock->readUnlock( lockdbrep );
			_hashLock->readUnlock( repstr );
		}
		return NULL;
	}
}

JagIndex *JagServerObjectLock::readLockIndex( jagint opcode, const Jstr &dbName, Jstr &tableName, const Jstr &indexName,
											  int replicType, bool lockSelfLevelOnly, int &rc )
{
	Jstr repstr = intToStr( replicType );
    //AbaxString dbidx = dbName + "." + indexName;

	//AbaxString dbidxrep = dbidx + "." + repstr;
    // dbidxrep == lockdbidxrep

	AbaxString lockdbrep = dbName + "." + repstr;
	AbaxString dbtab = dbName + "." + tableName;
	AbaxString lockdbtabrep = dbtab + "." + repstr;
	AbaxString lockdbidxrep = dbName + "." + indexName + "." + repstr;

	AbaxString tname;

	if ( tableName.size() < 1 ) {
		if ( !_idxtableNames->getValue( lockdbidxrep, tname ) ) {
			rc = -9;
			return NULL;
		}

		tableName = tname.c_str();
        dnlock("lock0002938 got tableName=[%s]", tableName.s() );
	}

	JagHashMap<AbaxString, AbaxBuffer> *dbmap = NULL, *tabmap = NULL, *idxmap = NULL;

	if ( 0 == replicType ) {
		dbmap = _databases;
		tabmap = _tables;
		idxmap = _indexs;
	} else if ( 1 == replicType ) {
		dbmap = _prevdatabases;
		tabmap = _prevtables;
		idxmap = _previndexs;
	} else if ( 2 == replicType ) {
		dbmap = _nextdatabases;
		tabmap = _nexttables;
		idxmap = _nextindexs;
	} else {
		rc = -1;
		return NULL;
	}

    /***
	if ( !dbmap->keyExist( dbName ) || !tabmap->keyExist( lockdbtabrep ) || 
		!_idxtableNames->keyExist( lockdbidxrep ) || !idxmap->keyExist( lockdbidxrep ) ) {
			rc = -10;
			return NULL; 
	}
    **/

	if ( !lockSelfLevelOnly ) {
		JAG_BLURT _hashLock->readLock( repstr ); JAG_OVER
		JAG_BLURT _hashLock->readLock( lockdbrep ); JAG_OVER
		JAG_BLURT _hashLock->readLock( lockdbtabrep ); JAG_OVER
	}

    dnlock("lock3440012 readLock %s ...", lockdbidxrep.s() );
	JAG_BLURT _hashLock->readLock( lockdbidxrep ); JAG_OVER
    dnlock("lock3440012 readLock %s done", lockdbidxrep.s() );

	if ( !dbmap->keyExist( dbName ) || !tabmap->keyExist( lockdbtabrep ) ||
		!_idxtableNames->keyExist( lockdbidxrep ) || !idxmap->keyExist( lockdbidxrep ) ) {

		_hashLock->readUnlock( lockdbidxrep );
        dnlock("lock000148 readUnlock lockdbidxrep=%s", lockdbidxrep.s() );

		if ( !lockSelfLevelOnly ) {
			_hashLock->readUnlock( lockdbtabrep );
			_hashLock->readUnlock( lockdbrep );
			_hashLock->readUnlock( repstr );
		}
		rc = -20;
		return NULL;
	}

	AbaxBuffer bfr;
	if ( !idxmap->getValue( lockdbidxrep, bfr ) ) {

		_hashLock->readUnlock( lockdbidxrep );
        dnlock("lock000151 readUnlock lockdbidxrep=%s", lockdbidxrep.s() );

		if ( !lockSelfLevelOnly ) {
			_hashLock->readUnlock( lockdbtabrep );
			_hashLock->readUnlock( lockdbrep );
			_hashLock->readUnlock( repstr );
		}

		rc = -30;
		return NULL;
	}

	JagIndex *pindex = (JagIndex*) bfr.addr();
	rc = 0;
	return pindex;
}

int JagServerObjectLock::readUnlockIndex( jagint opcode, const Jstr &dbName, const Jstr &tableName, const Jstr &indexName,
										  int replicType, bool lockSelfLevelOnly )
{
	Jstr repstr = intToStr( replicType );
	AbaxString lockdbrep = dbName + "." + repstr;
	AbaxString dbtab = dbName + "." + tableName;
	AbaxString lockdbtabrep = dbtab + "." + repstr;
	//AbaxString lockidxrep = dbtab + "." + indexName + "." + repstr;

	AbaxString lockdbidxrep = dbName + "." + indexName + "." + repstr;

	_hashLock->readUnlock( lockdbidxrep );
    dnlock("lock000153 readUnlock lockdbidxrep=%s", lockdbidxrep.s() );

	if ( !lockSelfLevelOnly ) {
		_hashLock->readUnlock( lockdbtabrep );
		_hashLock->readUnlock( lockdbrep );
		_hashLock->readUnlock( repstr );
	}
	return 1;	
}	

JagIndex *JagServerObjectLock
::writeLockIndex( jagint opcode, const Jstr &dbName, const Jstr &tableName, 
					const Jstr &indexName, const JagTableSchema *tschema, 
					const JagIndexSchema *ischema, int replicType, 
					bool lockSelfLevelOnly, int &rc )
{
	Jstr repstr = intToStr( replicType );

	//AbaxString dbidx = dbName + "." + indexName;
	//AbaxString dbidxrep = dbidx + "." + repstr;

	AbaxString lockdbrep = dbName + "." + repstr;

	AbaxString dbtab = dbName + "." + tableName;
	AbaxString lockdbtabrep = dbtab + "." + repstr;

	AbaxString lockdbidxrep = dbName + "." + indexName + "." + repstr;

    dnlock("lock2020291 writeLockIndex lockSelfLevelOnly=%d lockdbidxrep=%s", lockSelfLevelOnly, lockdbidxrep.s() );

	JagHashMap<AbaxString, AbaxBuffer> *dbmap = NULL, *tabmap = NULL, *idxmap = NULL;

	if ( 0 == replicType ) {
		dbmap = _databases;
		tabmap = _tables;
		idxmap = _indexs;
	} else if ( 1 == replicType ) {
		dbmap = _prevdatabases;
		tabmap = _prevtables;
		idxmap = _previndexs;
	} else if ( 2 == replicType ) {
		dbmap = _nextdatabases;
		tabmap = _nexttables;
		idxmap = _nextindexs;
	} else {
		rc = -1;
		return NULL;
	}

    /*****
	if ( JAG_CREATEINDEX_OP == opcode ) {
		if ( !dbmap->keyExist( dbName ) || !tabmap->keyExist( lockdbtabrep ) || _idxtableNames->keyExist( lockdbidxrep ) ) {
			rc = -10;
			return NULL;
		}

		if ( dbmap->keyExist( dbName ) && tabmap->keyExist( lockdbtabrep ) && idxmap->keyExist( lockdbidxrep ) ) {
			rc = -15;
			return NULL;
		}

	} else {
		if ( !dbmap->keyExist( dbName ) ) {
			rc = -20;
			dnlock("SL0020 dbName=[%s] notfound in dbmap", dbName.s() );
			return NULL;
		}

		if ( !tabmap->keyExist( lockdbtabrep ) ) {
			rc = -21;
			dnlock("SL0021 dbtab=[%s] notfound in tabmap", lockdbtabrep.s() );
			return NULL;
		}

		if ( !_idxtableNames->keyExist( lockdbidxrep ) ) {
			rc = -22;
			dnlock("SL0022 dbidxrep=[%s] notfound in _idxtableNames", lockdbidxrep.s() );
			return NULL;
		}

		if (  !idxmap->keyExist( lockdbidxrep ) ) {
			rc = -23;
			dnlock("SL0023 dbidx=[%s] notfound in idxmap", lockdbidxrep.s() );
			return NULL;
		}
	}
    ****/


	if ( !lockSelfLevelOnly ) {
		JAG_BLURT _hashLock->readLock( repstr ); JAG_OVER
		JAG_BLURT _hashLock->readLock( lockdbrep ); JAG_OVER

        dnlock("lock320010 writeLock %s ...", lockdbtabrep.s() );
		JAG_BLURT _hashLock->writeLock( lockdbtabrep ); JAG_OVER
        dnlock("lock320010 writeLock %s done", lockdbtabrep.s() );
	}

    dnlock("lock3030881 writeLock lockdbidxrep=%s ...", lockdbidxrep.s() );
	JAG_BLURT _hashLock->writeLock( lockdbidxrep ); JAG_OVER
    dnlock("lock3030881 writeLock lockdbidxrep=%s done", lockdbidxrep.s() );

	if ( JAG_CREATEINDEX_OP == opcode ) {
		if (    ! dbmap->keyExist( dbName ) 
             || ! tabmap->keyExist( lockdbtabrep ) 
             || _idxtableNames->keyExist( lockdbidxrep ) ) {

			_hashLock->writeUnlock( lockdbidxrep );
            dnlock("lock00163 writeUnlock lockdbidxrep=%s", lockdbidxrep.s() );

			if ( !lockSelfLevelOnly ) {
				_hashLock->writeUnlock( lockdbtabrep );
                dnlock("m3330198 writeUnlock %s", lockdbtabrep.s() );
				_hashLock->readUnlock( lockdbrep );
				_hashLock->readUnlock( repstr );
			}
			rc = -30;
			return NULL;
        } else if ( dbmap->keyExist( dbName ) 
                    && tabmap->keyExist( lockdbtabrep ) 
                    && idxmap->keyExist( lockdbidxrep ) ) {

			_hashLock->writeUnlock( lockdbidxrep );
            dnlock("lock00165 writeUnlock lockdbidxrep=%s", lockdbidxrep.s() );

			if ( !lockSelfLevelOnly ) {
				_hashLock->writeUnlock( lockdbtabrep );
                dnlock("lock22271 writeUnlock %s", lockdbtabrep.s() );
				_hashLock->readUnlock( lockdbrep );
				_hashLock->readUnlock( repstr );
			}
			rc = -33;
			return NULL;

		} else {
			Jstr path;
			const JagSchemaRecord *trecord, *irecord;
			irecord = ischema->getOneIndexAttr( dbName, indexName, path );
            dnlock("s020288 ischema->getOneIndexAttr dbName=[%s] indexName=[%s] path=[%s]", dbName.s(), indexName.s(), path.s() );

			trecord = tschema->getAttr( path );
			if ( ! irecord || ! trecord ) {

				_hashLock->writeUnlock( lockdbidxrep );
                dnlock("lock00012 writeUnlock lockdbidxrep=%s", lockdbidxrep.s() );

				if ( !lockSelfLevelOnly ) {
					_hashLock->writeUnlock( lockdbtabrep );
                    dnlock("lock3444081 writeUnlock %s", lockdbtabrep.s() );
					_hashLock->readUnlock( lockdbrep );
					_hashLock->readUnlock( repstr );
				}
				rc = -40;
				return NULL;
			}

			path += Jstr(".") + indexName;
            dnlock("s0282811 path=[%s]  new JagIndex()", path.s() );

			JagIndex *pindex = new JagIndex( replicType, _servobj, path, *trecord, *irecord );
			AbaxBuffer bfr = (void*) pindex;

			if ( !idxmap->addKeyValue( lockdbidxrep, bfr ) ) {
				delete pindex;

				_hashLock->writeUnlock( lockdbidxrep );
                dnlock("lock000141 writeUnlock lockdbidxrep=%s", lockdbidxrep.s() );

				if ( !lockSelfLevelOnly ) {
					_hashLock->writeUnlock( lockdbtabrep );
                    dnlock("lock2222091 writeUnlock %s", lockdbtabrep.s() );
					_hashLock->readUnlock( lockdbrep );
					_hashLock->readUnlock( repstr );
				}
				rc = -50;
				return NULL;
			}
			
			if ( !_idxtableNames->addKeyValue( lockdbidxrep, tableName ) ) {
				delete pindex;

				_hashLock->writeUnlock( lockdbidxrep );
                dnlock("lock000143 writeUnlock lockdbidxrep=%s", lockdbidxrep.s() );

				if ( !lockSelfLevelOnly ) {
					_hashLock->writeUnlock( lockdbtabrep );
                    dnlock("lock22091003 writeUnlock %s", lockdbtabrep.s() );
					_hashLock->readUnlock( lockdbrep );
					_hashLock->readUnlock( repstr );
				}
				rc = -60;
				return NULL;
			}
			dnlock("SL003451 return pindex=%p", pindex );
			return pindex;
		}
	} else {
        dnlock("lock01122  not create index");
		// not create index
		if ( !dbmap->keyExist( dbName ) || !tabmap->keyExist( lockdbtabrep ) || 
			!_idxtableNames->keyExist(lockdbidxrep) || !idxmap->keyExist( lockdbidxrep ) ) {

			_hashLock->writeUnlock( lockdbidxrep );
            dnlock("lock000144 writeUnlock lockdbidxrep=%s", lockdbidxrep.s() );

			if ( !lockSelfLevelOnly ) {
				_hashLock->writeUnlock( lockdbtabrep );
                dnlock("m3333018 writeUnlock %s", lockdbtabrep.s() );
				_hashLock->readUnlock( lockdbrep );
				_hashLock->readUnlock( repstr );
			}
			rc = -70;
			return NULL;
		}

		AbaxBuffer bfr;
		if ( !idxmap->getValue( lockdbidxrep, bfr ) ) {

			_hashLock->writeUnlock( lockdbidxrep );
            dnlock("lock000146 writeUnlock lockdbidxrep=%s", lockdbidxrep.s() );

			if ( !lockSelfLevelOnly ) {
				_hashLock->writeUnlock( lockdbtabrep );
                dnlock("lock202810087 writeUnlock %s", lockdbtabrep.s() );
				_hashLock->readUnlock( lockdbrep );
				_hashLock->readUnlock( repstr );
			}
			rc = -80;
			return NULL;
		}
		
		dnlock("SL0288112 _idxtableNames->keyExist(lockdbidxrep=%s)=%d", lockdbidxrep.s(), _idxtableNames->keyExist(lockdbidxrep) ); 
		JagIndex *pindex = (JagIndex*) bfr.addr();
		dnlock("SL003452 return pindex=%p", pindex );
		rc = 0;
		return pindex;
	}

    dnlock("lock0822299 end writeLockIndex() ");
    return NULL;
}  // end of writeLockIndex()


JagIndex *JagServerObjectLock::getIndex( const Jstr &dbName, const Jstr &indexName, int replicType )
{
    Jstr repstr = intToStr( replicType );
	//AbaxString dbidx = dbName + "." + indexName;
	AbaxString lockdbidxrep = dbName + "." + indexName + "." + repstr;

	JagHashMap<AbaxString, AbaxBuffer> *idxmap = NULL;

	if ( 0 == replicType ) {
		idxmap = _indexs;
	} else if ( 1 == replicType ) {
		idxmap = _previndexs;
	} else if ( 2 == replicType ) {
		idxmap = _nextindexs;
	} else {
		return NULL;
	}

	AbaxBuffer bfr;
	if ( !idxmap->getValue( lockdbidxrep, bfr ) ) {
		return NULL;
	}

	JagIndex *pindex = (JagIndex*) bfr.addr();
	return pindex;
}

int JagServerObjectLock::writeUnlockIndex( jagint opcode, const Jstr &dbName, const Jstr &tableName, 
											const Jstr &indexName, int replicType, bool lockSelfLevelOnly )
{
	Jstr repstr = intToStr( replicType );
	AbaxString lockdbrep = dbName + "." + repstr;
	AbaxString dbtab = dbName + "." + tableName;
	AbaxString lockdbtabrep = dbtab + "." + repstr;

    //	AbaxString dbidx = dbName + "." + indexName;
    // AbaxString dbidxrep = dbidx + "." + repstr;

	//AbaxString lockidx = dbtab + "." + indexName + "." + repstr;
	AbaxString lockdbidxrep = dbName + "." + indexName + "." + repstr;


	JagHashMap<AbaxString, AbaxBuffer> *dbmap = NULL, *tabmap = NULL, *idxmap = NULL;


	if ( 0 == replicType ) {
		dbmap = _databases;
		tabmap = _tables;
		idxmap = _indexs;
	} else if ( 1 == replicType ) {
		dbmap = _prevdatabases;
		tabmap = _prevtables;
		idxmap = _previndexs;
	} else if ( 2 == replicType ) {
		dbmap = _nextdatabases;
		tabmap = _nexttables;
		idxmap = _nextindexs;
	} else {
		return -1;
	}

	if ( JAG_DROPINDEX_OP == opcode ) {
		if ( !idxmap->removeKey( lockdbidxrep ) ) {

            dnlock("lock877015 writeUnlockIndex lockdbidxrep=%s", lockdbidxrep.s() );
			_hashLock->writeUnlock( lockdbidxrep );

			if ( !lockSelfLevelOnly ) {
				_hashLock->writeUnlock( lockdbtabrep );
                dnlock("lock400238 writeUnlock %s", lockdbtabrep.s() );
				_hashLock->readUnlock( lockdbrep );
				_hashLock->readUnlock( repstr );
			}
			return -10;
		} else {
			if ( !_idxtableNames->removeKey( lockdbidxrep ) ) {

                dnlock("lock837015 writeUnlockIndex lockdbidxrep=%s", lockdbidxrep.s() );
				_hashLock->writeUnlock( lockdbidxrep );

				if ( !lockSelfLevelOnly ) {
					_hashLock->writeUnlock( lockdbtabrep );
                    dnlock("lock400138 writeUnlock %s", lockdbtabrep.s() );
					_hashLock->readUnlock( lockdbrep );
					_hashLock->readUnlock( repstr );
				}
				return -20;
			}

            dnlock("lock897115 writeUnlockIndex lockdbidxrep=%s", lockdbidxrep.s() );
			_hashLock->writeUnlock( lockdbidxrep );

			if ( !lockSelfLevelOnly ) {
				_hashLock->writeUnlock( lockdbtabrep );
                dnlock("lock400128 writeUnlock %s", lockdbtabrep.s() );
				_hashLock->readUnlock( lockdbrep );
				_hashLock->readUnlock( repstr );
			}
			return 10;
		}
	} else {

        dnlock("lock095115 writeUnlockIndex lockdbidxrep=%s", lockdbidxrep.s() );
		_hashLock->writeUnlock( lockdbidxrep );

		if ( !lockSelfLevelOnly ) {
			_hashLock->writeUnlock( lockdbtabrep );
            dnlock("lock400028 writeUnlock %s", lockdbtabrep.s() );
			_hashLock->readUnlock( lockdbrep );
			_hashLock->readUnlock( repstr );
		}
		return 20;
	}	
}  // end of writeUnlockIndex

Jstr JagServerObjectLock::getAllTableNames( int replicType )
{
	JagHashMap<AbaxString, AbaxBuffer> *tabmap = NULL;
	if ( 0 == replicType ) {
		tabmap = _tables;
	} else if ( 1 == replicType ) {
		tabmap = _prevtables;
	} else if ( 2 == replicType ) {
		tabmap = _nexttables;
	} else {
		return "";
	}

	Jstr str; 
	const AbaxPair<AbaxString, AbaxBuffer> *arr = tabmap->array(); 
	jagint len = tabmap->arrayLength();
    for ( jagint i = 0; i < len; ++i ) {
		if ( tabmap->isNull(i) ) continue;
		
		if ( str.size() < 1 ) {
			str = arr[i].key.c_str();
		} else {
			str += Jstr("|") + arr[i].key.c_str();
		}
	}
	return str;
}
