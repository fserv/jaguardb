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
 /**
#include "rice/Class.hpp"
#include "rice/Constructor.hpp"
#include "rice/String.hpp"
**/
#include "rice/rice.hpp"
#include "rice/stl.hpp"
#include "JaguarAPI.h"

using namespace Rice;

// Wrapper class for JaguarAPI
class Jaguar
{
  public:
  	Jaguar() 
	{
	}

	~Jaguar()
	{
	}


	bool connect( String host, unsigned int port, String uid, String pass, String db )
	{
		int rc = _api.connect( host.str().c_str(), port, uid.str().c_str(), pass.str().c_str(), db.str().c_str() );
		if ( rc ) { return true; } else { return false; }
	}

	bool execute( String qs )
	{
		int rc = _api.execute( qs.str().c_str() );
		if ( rc ) { return true; } else { return false; }
	}

	bool query( String qs, int reply=1  )
	{
		int rc = _api.query( qs.str().c_str(), reply );
		if ( rc ) { return true; } else { return false; }
	}

	bool reply()
	{
		int rc = _api.reply();
		if ( rc ) { return true; } else { return false; }
	}

	void printRow()
	{
		_api.printRow();
	}

	void freeResult()
	{
		_api.freeResult();
	}

	void close()
	{
		_api.close();
	}

	String getDatabase()
	{
		return String( _api.getDatabase() );
	}

	bool hasError()
	{
		int rc = _api.hasError();
		if ( rc ) { return true; } else { return false; }
	}

	String error()
	{
		return String(_api.error());
	}

	String getNthValue( int col )
	{
		char *p = _api.getNthValue( col );
		if ( p ) {
			String s(p);
			free(p);
			return s;
		} else {
			String s;
			return s;
		}
	}

	String getValue( String name )
	{
		char *p = _api.getValue( name.str().c_str() );
		if ( p ) {
			String s(p);
			free(p);
			return s;
		} else {
			String s;
			return s;
		}
	}

	String getMessage( )
	{
		const char *p = _api.getMessage();
		return String(p);
	}

	String getAll( )
	{
		const char *p = _api.getAll();
		return String(p);
	}

	String getLastUuid( )
	{
		const char *p = _api.getLastUuid();
		return String(p);
	}

	String jsonString( )
	{
		const char *p = _api.jsonString();
		return String(p);
	}

	long long getLong( String name )
	{
		long long v=0;
		int rc = _api.getLong( name.str().c_str(), &v );
		if ( rc ) {
			return v;
		} else {
			return 0;
		}
	}

	long getInt( String name )
	{
		int v=0;
		int rc = _api.getInt( name.str().c_str(), &v );
		if ( rc ) {
			return v;
		} else {
			return 0;
		}
	}

	double getFloat( String name )
	{
		double v=0.0;
		int rc = _api.getDouble( name.str().c_str(), &v );
		if ( rc ) {
			return v;
		} else {
			return 0.0;
		}
	}

	int getColumnCount()
	{
		return _api.getColumnCount();
	}

	int getCluster()
	{
		return _api.getCluster();
	}

	String getColumnLabel( int col )
	{
		char *p = _api.getColumnLabel( col );
		if ( p ) {
			String s(p);
			free(p);
			return s;
		} else {
			String s;
			return s;
		}
	}

	String getColumnName( int col )
	{
		char *p = _api.getColumnName( col );
		if ( p ) {
			String s(p);
			free(p);
			return s;
		} else {
			String s;
			return s;
		}
	}

	int getColumnType( int col )
	{
		return _api.getColumnType( col );
	}

	String getColumnTypeName( int col )
	{
		char *p = _api.getColumnTypeName( col );
		if ( p ) {
			String s(p);
			free(p);
			return s;
		} else {
			String s;
			return s;
		}
	}

	String getTableName( int col )
	{
		char *p = _api.getTableName( col );
		if ( p ) {
			String s(p);
			free(p);
			return s;
		} else {
			String s;
			return s;
		}
	}


  protected:
	JaguarAPI _api;
};

extern "C"
void Init_jaguarrb( ) { 
    Rice::Class tmp_ = Rice::define_class<Jaguar>("Jaguar")
		.define_constructor(Constructor<Jaguar>())
		.define_method("connect", &Jaguar::connect)
		.define_method("query", &Jaguar::query, (Arg("qstr"), Arg("reply")=1 ) )
		.define_method("reply", &Jaguar::reply, (Arg("hdr")=0) )
		.define_method("printRow", &Jaguar::printRow )
		.define_method("freeResult", &Jaguar::freeResult )
		.define_method("close", &Jaguar::close )
		.define_method("getDatabase", &Jaguar::getDatabase )
		.define_method("hasError", &Jaguar::hasError )
		.define_method("error", &Jaguar::error )
		.define_method("getNthValue", &Jaguar::getNthValue )
		.define_method("getValue", &Jaguar::getValue )
		.define_method("getLong", &Jaguar::getLong )
		.define_method("getInt", &Jaguar::getInt )
		.define_method("getFloat", &Jaguar::getFloat )
		.define_method("getMessage", &Jaguar::getMessage )
		.define_method("getAll", &Jaguar::getAll )
		.define_method("getLastUuid", &Jaguar::getLastUuid )
		.define_method("execute", &Jaguar::execute )
		.define_method("getColumnCount", &Jaguar::getColumnCount )
		.define_method("getCluster", &Jaguar::getCluster )
		.define_method("getColumnLabel", &Jaguar::getColumnLabel )
		.define_method("getColumnName", &Jaguar::getColumnName )
		.define_method("getColumnType", &Jaguar::getColumnType )
		.define_method("getColumnTypeName", &Jaguar::getColumnTypeName )
		.define_method("getTableName", &Jaguar::getTableName )
		;
}

// .define_method("hello", &hello );
// VALUE x; x = rb_str_new_cstr("Hello, world!");
