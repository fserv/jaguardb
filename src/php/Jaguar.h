/*
 * Copyright (C) 2018,2019,2020,2021 DataJaguar, Inc.
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
#ifndef _CLASS_JAGUAR_H_
#define _CLASS_JAGUAR_H_
 
class JaguarAPI;

class Jaguar
{
  public:
    Jaguar();
    ~Jaguar();

    int connect( const char *ipaddress, unsigned int port,
                const char *username, const char *passwd,
                const char *dbname, const char *unixSocket,
                unsigned long long clientFlag );

	void close();
	int  execute( const char *cmd );
	int  query( const char *query, bool reply=true );
	int  reply( bool headerOnly=false );
	int  printRow();
	const char *getDatabase();
	const char *error( );
	int hasError( );
	int freeResult();
	const char *getNthValue( int nth );
	char *getValue( const char *name );
	int getLong(  const char *name, long long *value );
	int getDouble(  const char *name, double *value );
	const char *getMessage( );
	const char *jsonString( );
	int getColumnCount();
	int getCluster();
	char *getColumnLabel( int col );
	char *getColumnName( int col );
	int getColumnType( int col );
	char *getColumnTypeName( int col );
	char *getTableName( int col );

	void printAll();
	char *getAll();
	char *getLastUuid();
	char *getAllByName( const char *name);
	const char *getAllByIndex( int nth ); // 1--N
	void setDebug( bool flag );

  protected:
	JaguarAPI *_api;
};
 
#endif

