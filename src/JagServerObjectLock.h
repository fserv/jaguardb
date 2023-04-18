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
#ifndef _jag_server_object_lock_
#define _jag_server_object_lock_

#include <abax.h>

class JagDBServer;
class JagHashLock;
class JagTable;
class JagIndex;
class JagTableSchema;
class JagIndexSchema;

template <class K, class V> class JagHashMap;

class JagServerObjectLock
{
  public:
  	JagServerObjectLock( const JagDBServer *servobj );
	~JagServerObjectLock();

	void rebuildObjects();
	void setInitDatabases( const Jstr &dblist, int replicType );
	jagint getnumObjects( int objType, int replicType );

	int readLockSchema( int replicType );
	int readUnlockSchema( int replicType );
	int writeLockSchema( int replicType );
	int writeUnlockSchema( int replicType );
	int readLockDatabase( jagint opcode, const Jstr &dbName, int replicType );
	int readUnlockDatabase( jagint opcode, const Jstr &dbName, int replicType );
	int writeLockDatabase( jagint opcode, const Jstr &dbName, int replicType );
	int writeUnlockDatabase( jagint opcode, const Jstr &dbName, int replicType );

	JagTable *readLockTable( jagint opcode, const Jstr &db, const Jstr &table, int repType, bool lockSelfLevel, int &rc );
	int readUnlockTable( jagint opcode, const Jstr &dbName, const Jstr &table, int repType, bool lockSelfLevel );

	JagTable *writeLockTable( jagint opcode, const Jstr &db, const Jstr &table, const JagTableSchema *tschema, int repType, bool lockSelfLevel, int &rc );
	int writeUnlockTable( jagint opcode, const Jstr &db, const Jstr &table, int repType, bool lockSelfLevel );

	JagTable *getTable( jagint opcode, const Jstr &db, const Jstr &table, const JagTableSchema *tschema, int repType );

	JagTable *writeTruncateTable( jagint opcode, const Jstr &dbName, const Jstr &tableName, 
							      const JagTableSchema *tschema, int replicType, bool lockSelfLevel );

	JagIndex *readLockIndex( jagint opcode, const Jstr &dbName, Jstr &tableName, const Jstr &indexName, int replicType, bool lockSelfLevel, int &rc );
	int readUnlockIndex( jagint opcode, const Jstr &dbName, const Jstr &tableName, const Jstr &indexName, int replicType, bool lockSelfLevel );

	JagIndex *writeLockIndex( jagint opcode, const Jstr &dbName, const Jstr &tableName, const Jstr &indexName,
							  const JagTableSchema *tschema, const JagIndexSchema *ischema, int replicType, bool lockSelfLevel, int &rc );
	int writeUnlockIndex( jagint opcode, const Jstr &dbName, const Jstr &tableName, const Jstr &indexName,
						  int replicType, bool lockSelfLevel );
    JagIndex *getIndex( const Jstr &dbName, const Jstr &indexName, int replicType );
		
	Jstr getAllTableNames( int replicType );

	
  protected:
	const JagDBServer					*_servobj;
	JagHashLock							*_hashLock;
	JagHashMap<AbaxString, AbaxBuffer>	*_databases;
	JagHashMap<AbaxString, AbaxBuffer>	*_prevdatabases;
	JagHashMap<AbaxString, AbaxBuffer>	*_nextdatabases;	
	JagHashMap<AbaxString, AbaxBuffer>	*_tables;
	JagHashMap<AbaxString, AbaxBuffer>	*_prevtables;
	JagHashMap<AbaxString, AbaxBuffer>	*_nexttables;
	JagHashMap<AbaxString, AbaxBuffer>	*_indexs;
	JagHashMap<AbaxString, AbaxBuffer>	*_previndexs;
	JagHashMap<AbaxString, AbaxBuffer>	*_nextindexs;
	JagHashMap<AbaxString, AbaxString>	*_idxtableNames;

	void initObjects();
	void cleanupObjects();
};

#endif
