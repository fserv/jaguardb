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
#include "Jaguar.h"
#include "JaguarAPI.h"
 
Jaguar::Jaguar() 
{
	_api = new JaguarAPI();
}

Jaguar::~Jaguar()
{
	delete _api;
}

int Jaguar::connect( const char *ipaddress, unsigned int port,
                const char *username, const char *passwd,
                const char *dbname, const char *unixSocket,
                unsigned long long clientFlag )
{
	return _api->connect( ipaddress, port, username, passwd, dbname, unixSocket, clientFlag );
}

void Jaguar::close()
{
	return _api->close();
}

int Jaguar::execute( const char *query )
{
	if ( _api->query( query, true ) ) {
		while ( _api->reply() ) {}
	}
    return 1;
}

int Jaguar::query( const char *query, bool reply )
{
	return _api->query( query, reply );
}

int Jaguar::reply( bool hdr )
{
	return _api->reply( hdr );
}

int Jaguar::printRow()
{
	return _api->printRow();
}

const char *Jaguar::getDatabase()
{
	return _api->getDatabase();
}

const char *Jaguar::error( )
{
	return _api->error();
}
	
int Jaguar::hasError( )
{
	return _api->hasError();
}

int Jaguar::freeResult()
{
	return _api->freeResult();
}

// needs free
char *Jaguar::getNthValue( int nth )
{
	return _api->getNthValue( nth );
}

// needs free
char *Jaguar::getValue(  const char *name )
{
	return _api->getValue( name );
}

// true or false, result in value
int Jaguar::getLong(  const char *name, long long *value )
{
	return _api->getLong( name, value );
}

// true or false, result in value
int Jaguar::getDouble(  const char *name, double *value )
{
	return _api->getDouble( name, value );
}

const char *Jaguar::getMessage( )
{
	return _api->getMessage();
}
const char *Jaguar::jsonString( )
{
	return _api->jsonString();
}

int Jaguar::getColumnCount()
{
	return _api->getColumnCount();
}

// needs free
char *Jaguar::getColumnLabel( int col )
{
	return _api->getColumnLabel( col );
}

// needs free
char *Jaguar::getColumnName( int col )
{
	return _api->getColumnName( col );
}

int Jaguar::getColumnType( int col )
{
	return _api->getColumnType( col );
}

// needs free
char *Jaguar::getColumnTypeName( int col )
{
	return _api->getColumnTypeName( col );
}

// needs free
char *Jaguar::getTableName( int col )
{
	return _api->getTableName( col );
}

void Jaguar::printAll()
{
	_api->printAll();
}

// needs free
char *Jaguar::getAllByName(  const char *name )
{
	return _api->getAllByName( name );
}
// needs free
char *Jaguar::getAll()
{
	return _api->getAll();
}

// needs free
char *Jaguar::getAllByIndex( int nth )
{
	return _api->getAllByIndex( nth );
}

void Jaguar::setDebug( bool flag ) 
{
	_api->setDebug( flag );
}

