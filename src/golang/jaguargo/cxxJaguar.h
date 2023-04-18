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
#ifndef _MY_PACKAGE_FOO_HPP_
#define _MY_PACKAGE_FOO_HPP_

#include "../../JaguarAPI.h"

class cxxJaguar {
public:
    JaguarAPI jag;
	cxxJaguar(){};
	~cxxJaguar(){};
	int connect(   const char *ipaddress, unsigned int port, const char *username, const char *passwd, 
		       const char *dbname );
	int execute( const char *query );
	int query( const char *query );
	int reply( );
	const char *getSession();
	const char *getDatabase();
	void printRow();
	const char *error( );
	int hasError( );
	void freeResult();
	char *getNthValue( int nth );
	char *getValue( const char *name );
	int getInt(  const char *name, int *value );
	int getLong(  const char *name, long long *value );
	int getFloat(  const char *name, float *value );
	int getDouble(  const char *name, double *value );
	const char *getMessage( );
	void close();
	int getColumnCount();
	int getCluster();
	char *getCatalogName( int col );
	char *getColumnClassName( int col );
	int getColumnDisplaySize( int col );
	char *getColumnLabel( int col );
	char *getColumnName( int col );
	int getColumnType( int col );
	char *getColumnTypeName( int col );
	int getScale( int col );
	char *getSchemaName( int col );
	char *getTableName( int col );
	bool isAutoIncrement( int col );
	bool isCaseSensitive( int col );
	bool isCurrency( int col );
	bool isDefinitelyWritable( int col );
	int  isNullable( int col );
	bool isReadOnly( int col );
	bool isSearchable( int col );
	bool isSigned( int col );

	void printAll();
	char *getAll( );
	char *getLastUuid( );
	char *getAllByName( const char *name );
	char *getAllByIndex( int nth );  // 1--N

};

#endif
