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

#ifdef _MSC_VER
#pragma warning(disable: 4786)
#endif

#include "CXX/Objects.hxx"
#include "CXX/Extensions.hxx"

#include <assert.h>
#include <JaguarAPI.h>


class Jaguar: public Py::PythonClass< Jaguar >
{
public:
    Jaguar( Py::PythonClassInstance *self, Py::Tuple &args, Py::Dict &kwds )
        : Py::PythonClass< Jaguar >::PythonClass( self, args, kwds )
    
    {
		_api = new JaguarAPI();
    }

    virtual ~Jaguar()
    {
        // std::cout << "~Jaguar." << std::endl;
		delete _api;
    }

    static void init_type(void)
    {
        behaviors().name( "Jaguar" );
        behaviors().doc( "Jaguar class support Python 3.*" );

        PYCXX_ADD_NOARGS_METHOD( getSession, Jaguar_getSession, "Get session string for the connection" );
        PYCXX_ADD_NOARGS_METHOD( getDatabase, Jaguar_getDatabase, "Get current database string" );
        PYCXX_ADD_NOARGS_METHOD( printRow, Jaguar_printRow, "Print row" );
        PYCXX_ADD_NOARGS_METHOD( error, Jaguar_error, "Get error string" );
        PYCXX_ADD_NOARGS_METHOD( hasError, Jaguar_hasError, "If there was error" );
        PYCXX_ADD_NOARGS_METHOD( freeResult, Jaguar_freeResult, "Free memory taken by result" );
        PYCXX_ADD_NOARGS_METHOD( getMessage, Jaguar_getMessage, "Get message string" );
        PYCXX_ADD_NOARGS_METHOD( getAll, Jaguar_getAll, "Get all geo objects" );
        PYCXX_ADD_NOARGS_METHOD( getLastUuid, Jaguar_getLastUuid, "Get UUID of last insert" );
        PYCXX_ADD_NOARGS_METHOD( jsonString, Jaguar_jsonString, "Get record in JSON format" );
        PYCXX_ADD_NOARGS_METHOD( close, Jaguar_close, "Close connection to Jaguar server" );
        PYCXX_ADD_NOARGS_METHOD( getColumnCount, Jaguar_getColumnCount, "Get number of columns in the row" );
        PYCXX_ADD_NOARGS_METHOD( getCluster, Jaguar_getCluster, "Get current cluster number" );



        PYCXX_ADD_VARARGS_METHOD( connect, Jaguar_connect, "connect to Jaguar server" );
        PYCXX_ADD_VARARGS_METHOD( execute, Jaguar_execute, "Execute DML command in Jaguar server" );
        PYCXX_ADD_VARARGS_METHOD( query, Jaguar_query, "Make query to Jaguar server" );
        PYCXX_ADD_VARARGS_METHOD( reply, Jaguar_reply, "Get reply from Jaguar servver" );
        PYCXX_ADD_VARARGS_METHOD( getNthValue, Jaguar_getNthValue, "Get value of n-th column in the result row" );
        PYCXX_ADD_VARARGS_METHOD( getValue, Jaguar_getValue, "Get value of a named column in the result row" );
        PYCXX_ADD_VARARGS_METHOD( getInt, Jaguar_getInt, "Get long integer value of a named column in the result row" );
        PYCXX_ADD_VARARGS_METHOD( getLong, Jaguar_getLong, "Get long integer value of a named column in the result row" );
        PYCXX_ADD_VARARGS_METHOD( getFloat, Jaguar_getFloat, "Get float value of a named column in the result row" );
        PYCXX_ADD_VARARGS_METHOD( getDouble, Jaguar_getDouble, "Get float value of a named column in the result row" );

        PYCXX_ADD_VARARGS_METHOD( getCatalogName, Jaguar_getCatalogName, "Get catalog(databae) name of a column" );
        PYCXX_ADD_VARARGS_METHOD( getSchemaName, Jaguar_getSchemaName, "Get schema name of a column" );
        PYCXX_ADD_VARARGS_METHOD( getColumnClassName, Jaguar_getColumnClassName, "Get column class name of a column" );
        PYCXX_ADD_VARARGS_METHOD( getColumnDisplaySize, Jaguar_getColumnDisplaySize, "Get dislay size of a column" );
        PYCXX_ADD_VARARGS_METHOD( getColumnLabel, Jaguar_getColumnLabel, "Get dislay label of a column" );
        PYCXX_ADD_VARARGS_METHOD( getColumnName, Jaguar_getColumnName, "Get name of a column" );
        PYCXX_ADD_VARARGS_METHOD( getColumnTypeName, Jaguar_getColumnTypeName, "Get type name of a column" );
        PYCXX_ADD_VARARGS_METHOD( getColumnType, Jaguar_getColumnType, "Get numeric column type of a column" );
        PYCXX_ADD_VARARGS_METHOD( getScale, Jaguar_getScale, "Get scale of a column" );
        PYCXX_ADD_VARARGS_METHOD( getTableName, Jaguar_getTableName, "Get table name of a column" );
        PYCXX_ADD_VARARGS_METHOD( isAutoIncrement, Jaguar_isAutoIncrement, "Is the given column auto-increment" );
        PYCXX_ADD_VARARGS_METHOD( isCaseSensitive, Jaguar_isCaseSensitive, "Is the given column case sensitive" );
        PYCXX_ADD_VARARGS_METHOD( isCurrency, Jaguar_isCurrency, "Is the given column a currency field" );
        PYCXX_ADD_VARARGS_METHOD( isDefinitelyWritable, Jaguar_isDefinitelyWritable, "Is the given column writable" );
        PYCXX_ADD_VARARGS_METHOD( isNullable, Jaguar_isNullable, "Can the given column have NULLs" );
        PYCXX_ADD_VARARGS_METHOD( isReadOnly, Jaguar_isReadOnly, "Is the given column ReadOnly" );
        PYCXX_ADD_VARARGS_METHOD( isSearchable, Jaguar_isSearchable, "Is the given column Searchable" );
        PYCXX_ADD_VARARGS_METHOD( isSigned, Jaguar_isSigned, "Is the given column Signed" );

        // Call to make the type ready for use
        behaviors().readyType();
    }

    Py::Object  Jaguar_connect( const Py::Tuple &arg )
    {
	    Py::String host( arg[0] );
	    Py::Long port( arg[1] );
	    Py::String user( arg[2] );
	    Py::String pass( arg[3] );
	    Py::String db( arg[4] );
	    // Py::String unixsock( arg[5] );
	    // Py::Long cflag( arg[6] );

		/***
		int rc = _api->connect( host.as_string().c_str(), port.as_long(), user.as_string().c_str(), pass.as_string().c_str(),
		                        db.as_string().c_str(), unixsock.as_string().c_str(), cflag.as_long() );
		***/
		int rc = _api->connect( host.as_string().c_str(), port.as_long(), user.as_string().c_str(), pass.as_string().c_str(),
		                        db.as_string().c_str(), NULL, 0 );

		Py::Object res;
		if ( rc ) {
			res = Py::True();
		} else {
			res = Py::False();
		}

        return res;
    }
    PYCXX_VARARGS_METHOD_DECL( Jaguar, Jaguar_connect )


    Py::Object  Jaguar_query( const Py::Tuple &arg )
    {
	    Py::String qstr( arg[0] );

		int rc; 

		if ( arg.length() < 2  ) {
			//printf("s3744 reply as long true\n");
			rc = _api->query ( qstr.as_string().c_str(), true ); 
		} else {
	    	Py::Long reply( arg[1] );
			//printf("s3745 reply as long %d\n", reply.as_long() );
			rc = _api->query ( qstr.as_string().c_str(), reply.as_long() ); 
		}

		Py::Object res;
		if ( rc ) {
			res = Py::True();
		} else {
			res = Py::False();
		}

        return res;
    }
    PYCXX_VARARGS_METHOD_DECL( Jaguar, Jaguar_query )

    Py::Object  Jaguar_execute( const Py::Tuple &arg )
    {
	    Py::String qstr( arg[0] );

		int rc; 
		rc = _api->execute( qstr.as_string().c_str() ); 

		Py::Object res;
		if ( rc ) {
			res = Py::True();
		} else {
			res = Py::False();
		}

        return res;
    }
    PYCXX_VARARGS_METHOD_DECL( Jaguar, Jaguar_execute )


    Py::Object  Jaguar_reply( const Py::Tuple &arg )
    {
		int rc; 
		//printf("s4098 arg.length=%d\n", arg.length() );

		if ( arg.length() < 1  ) {
			rc = _api->reply();
		} else {
	    	Py::Long hdr( arg[0] );
			rc = _api->reply ( hdr.as_long() ); 
		}

		Py::Object res;
		if ( rc ) {
			res = Py::True();
		} else {
			res = Py::False();
		}

        return res;
    }
    PYCXX_VARARGS_METHOD_DECL( Jaguar, Jaguar_reply )



    Py::Object Jaguar_getSession( void )
    {
        //std::cout << "Jaguar_getSession Called." << std::endl;
		const char *session = _api->getSession();
	    Py::String sess( session );

        return sess;
    }
    PYCXX_NOARGS_METHOD_DECL( Jaguar, Jaguar_getSession )

    Py::Object Jaguar_getDatabase( void )
    {
        //std::cout << "Jaguar_getDatabase Called." << std::endl;
		const char *db = _api->getDatabase();
	    Py::String dbo( db );

        return dbo;
    }
    PYCXX_NOARGS_METHOD_DECL( Jaguar, Jaguar_getDatabase )

    Py::Object Jaguar_getMessage( void )
    {
		const char *m = _api->getMessage();
		if ( ! m ) {
        	return Py::None();
		}

	    Py::String mo( m );
        return mo;
    }
    PYCXX_NOARGS_METHOD_DECL( Jaguar, Jaguar_getMessage )

    Py::Object Jaguar_getLastUuid( void )
    {
		const char *m = _api->getLastUuid();
		if ( ! m ) {
        	return Py::None();
		}

	    Py::String mo( m );
        return mo;
    }
    PYCXX_NOARGS_METHOD_DECL( Jaguar, Jaguar_getLastUuid )

    Py::Object Jaguar_getAll( void )
    {
		const char *m = _api->getAll();
		if ( ! m ) {
        	return Py::None();
		}

	    Py::String mo( m );
        return mo;
    }
    PYCXX_NOARGS_METHOD_DECL( Jaguar, Jaguar_getAll )


    Py::Object Jaguar_jsonString( void )
    {
		const char *m = _api->jsonString();
		if ( ! m ) {
        	return Py::None();
		}

	    Py::String mo( m );
        return mo;
    }
    PYCXX_NOARGS_METHOD_DECL( Jaguar, Jaguar_jsonString )


    Py::Object Jaguar_error( void )
    {
        //std::cout << "Jaguar_getDatabase Called." << std::endl;
		const char *err = _api->error();
	    Py::String eo( err );

        return eo;
    }
    PYCXX_NOARGS_METHOD_DECL( Jaguar, Jaguar_error )


    Py::Object Jaguar_printRow( void )
    {
		_api->printRow();
        return Py::None();
    }
    PYCXX_NOARGS_METHOD_DECL( Jaguar, Jaguar_printRow )

    Py::Object Jaguar_close( void )
    {
		_api->close();
        return Py::None();
    }
    PYCXX_NOARGS_METHOD_DECL( Jaguar, Jaguar_close )


    Py::Object  Jaguar_getNthValue( const Py::Tuple &arg )
    {
		if ( arg.length() < 1  ) {
        	return Py::None();
		} 

	    Py::Long hdr( arg[0] );
		const char *v =  _api->getNthValue( hdr.as_long() );
		if ( ! v ) {
        	return Py::None();
		}

		Py::String res(v);
		//free( v );
        return res;
    }
    PYCXX_VARARGS_METHOD_DECL( Jaguar, Jaguar_getNthValue )


    Py::Object  Jaguar_getValue( const Py::Tuple &arg )
    {
		if ( arg.length() < 1  ) {
        	return Py::None();
		} 

	    Py::String hdr( arg[0] );
		char *v =  _api->getValue( hdr.as_string().c_str() );
		if ( ! v ) {
        	return Py::None();
		}

		Py::String res(v);
		free( v );
        return res;
    }
    PYCXX_VARARGS_METHOD_DECL( Jaguar, Jaguar_getValue )

    Py::Object  Jaguar_getInt( const Py::Tuple &arg )
    {
		if ( arg.length() < 1  ) {
        	return Py::None();
		} 

		char *v =  _api->getValue( arg[0].as_string().c_str() );
		if ( ! v ) {
        	return Py::None();
		}

		Py::Long res( atol(v) );
		free( v );
        return res;
    }
    PYCXX_VARARGS_METHOD_DECL( Jaguar, Jaguar_getInt )


    Py::Object  Jaguar_getLong( const Py::Tuple &arg )
    {
		if ( arg.length() < 1  ) {
        	return Py::None();
		} 

		char *v =  _api->getValue( arg[0].as_string().c_str() );
		if ( ! v ) {
        	return Py::None();
		}

		Py::Long res( atol(v) );
		free( v );
        return res;
    }
    PYCXX_VARARGS_METHOD_DECL( Jaguar, Jaguar_getLong )


    Py::Object  Jaguar_getFloat( const Py::Tuple &arg )
    {
		if ( arg.length() < 1  ) {
        	return Py::None();
		} 

		char *v =  _api->getValue( arg[0].as_string().c_str() );
		if ( ! v ) {
        	return Py::None();
		}

		Py::Float res( atof(v) );
		free( v );
        return res;
    }
    PYCXX_VARARGS_METHOD_DECL( Jaguar, Jaguar_getFloat )

    Py::Object  Jaguar_getDouble( const Py::Tuple &arg )
    {
		if ( arg.length() < 1  ) {
        	return Py::None();
		} 

		char *v =  _api->getValue( arg[0].as_string().c_str() );
		if ( ! v ) {
        	return Py::None();
		}

		Py::Float res( atof(v) );
		free( v );
        return res;
    }
    PYCXX_VARARGS_METHOD_DECL( Jaguar, Jaguar_getDouble )



    Py::Object Jaguar_hasError( void )
    {
		int rc = _api->hasError();
		if ( rc ) {
			return Py::True();
		} else {
			return Py::False();
		}
    }
    PYCXX_NOARGS_METHOD_DECL( Jaguar, Jaguar_hasError )

    Py::Object Jaguar_freeResult( void )
    {
		_api->freeResult();
        return Py::None();
    }
    PYCXX_NOARGS_METHOD_DECL( Jaguar, Jaguar_freeResult )


    Py::Object  Jaguar_getColumnCount( void )
    {
		int c = _api->getColumnCount();
		Py::Long res( c );
        return res;
    }
    PYCXX_NOARGS_METHOD_DECL( Jaguar, Jaguar_getColumnCount )
	
    Py::Object  Jaguar_getCluster( void )
    {
		int c = _api->getCluster();
		Py::Long res( c );
        return res;
    }
    PYCXX_NOARGS_METHOD_DECL( Jaguar, Jaguar_getCluster )


	// dbname
    Py::Object  Jaguar_getCatalogName( const Py::Tuple &arg )
    {
		if ( arg.length() < 1  ) {
        	return Py::None();
		} 

	    Py::Long hdr( arg[0] );
		char *v =  _api->getCatalogName( hdr.as_long() );
		if ( ! v ) {
        	return Py::None();
		}

		Py::String res(v);
		free( v );
        return res;
    }
    PYCXX_VARARGS_METHOD_DECL( Jaguar, Jaguar_getCatalogName )


    Py::Object  Jaguar_getSchemaName( const Py::Tuple &arg )
    {
		if ( arg.length() < 1  ) {
        	return Py::None();
		} 

	    Py::Long hdr( arg[0] );
		char *v =  _api->getSchemaName( hdr.as_long() );
		if ( ! v ) {
        	return Py::None();
		}

		Py::String res(v);
		free( v );
        return res;
    }
    PYCXX_VARARGS_METHOD_DECL( Jaguar, Jaguar_getSchemaName )


    Py::Object  Jaguar_getColumnClassName( const Py::Tuple &arg )
    {
		if ( arg.length() < 1  ) {
        	return Py::None();
		} 

	    Py::Long hdr( arg[0] );
		char *v =  _api->getColumnClassName( hdr.as_long() );
		if ( ! v ) {
        	return Py::None();
		}

		Py::String res(v);
		free( v );
        return res;
    }
    PYCXX_VARARGS_METHOD_DECL( Jaguar, Jaguar_getColumnClassName )


    Py::Object  Jaguar_getColumnDisplaySize( const Py::Tuple &arg )
    {
		if ( arg.length() < 1  ) {
        	return Py::None();
		} 

	    Py::Long hdr( arg[0] );
		int c =  _api->getColumnDisplaySize( hdr.as_long() );
		Py::Long res(c);
        return res;
    }
    PYCXX_VARARGS_METHOD_DECL( Jaguar, Jaguar_getColumnDisplaySize )


    Py::Object  Jaguar_getColumnLabel( const Py::Tuple &arg )
    {
		if ( arg.length() < 1  ) {
        	return Py::None();
		} 

	    Py::Long hdr( arg[0] );
		char *v =  _api->getColumnLabel( hdr.as_long() );
		if ( ! v ) {
        	return Py::None();
		}

		Py::String res(v);
		free( v );
        return res;
    }
    PYCXX_VARARGS_METHOD_DECL( Jaguar, Jaguar_getColumnLabel )


    Py::Object  Jaguar_getColumnName( const Py::Tuple &arg )
    {
		if ( arg.length() < 1  ) {
        	return Py::None();
		} 

	    Py::Long hdr( arg[0] );
		char *v =  _api->getColumnName( hdr.as_long() );
		if ( ! v ) {
        	return Py::None();
		}

		Py::String res(v);
		free( v );
        return res;
    }
    PYCXX_VARARGS_METHOD_DECL( Jaguar, Jaguar_getColumnName )


    Py::Object  Jaguar_getColumnTypeName( const Py::Tuple &arg )
    {
		if ( arg.length() < 1  ) {
        	return Py::None();
		} 

	    Py::Long hdr( arg[0] );
		char *v =  _api->getColumnTypeName( hdr.as_long() );
		if ( ! v ) {
        	return Py::None();
		}

		Py::String res(v);
		free( v );
        return res;
    }
    PYCXX_VARARGS_METHOD_DECL( Jaguar, Jaguar_getColumnTypeName )


    Py::Object  Jaguar_getColumnType( const Py::Tuple &arg )
    {
		if ( arg.length() < 1  ) {
        	return Py::None();
		} 

	    Py::Long hdr( arg[0] );
		int c =  _api->getColumnType( hdr.as_long() );
		Py::Long res(c);
        return res;
    }
    PYCXX_VARARGS_METHOD_DECL( Jaguar, Jaguar_getColumnType )


    Py::Object  Jaguar_getScale( const Py::Tuple &arg )
    {
		if ( arg.length() < 1  ) {
        	return Py::None();
		} 

	    Py::Long hdr( arg[0] );
		int c =  _api->getScale( hdr.as_long() );
		Py::Long res(c);
        return res;
    }
    PYCXX_VARARGS_METHOD_DECL( Jaguar, Jaguar_getScale )


    Py::Object  Jaguar_getTableName( const Py::Tuple &arg )
    {
		if ( arg.length() < 1  ) {
        	return Py::None();
		} 

	    Py::Long hdr( arg[0] );
		char *v =  _api->getTableName( hdr.as_long() );
		if ( ! v ) {
        	return Py::None();
		}

		Py::String res(v);
		free( v );
        return res;
    }
    PYCXX_VARARGS_METHOD_DECL( Jaguar, Jaguar_getTableName )


    Py::Object  Jaguar_isAutoIncrement( const Py::Tuple &arg )
    {
		if ( arg.length() < 1  ) {
        	return Py::False();
		} 

	    Py::Long hdr( arg[0] );
		bool c =  _api->isAutoIncrement( hdr.as_long() );
		if ( c ) {
			return Py::True();
		} else {
			return Py::False();
		}
    }
    PYCXX_VARARGS_METHOD_DECL( Jaguar, Jaguar_isAutoIncrement )


    Py::Object  Jaguar_isCaseSensitive( const Py::Tuple &arg )
    {
		if ( arg.length() < 1  ) {
        	return Py::False();
		} 

	    Py::Long hdr( arg[0] );
		bool c =  _api->isCaseSensitive( hdr.as_long() );
		if ( c ) {
			return Py::True();
		} else {
			return Py::False();
		}
    }
    PYCXX_VARARGS_METHOD_DECL( Jaguar, Jaguar_isCaseSensitive )


    Py::Object  Jaguar_isCurrency( const Py::Tuple &arg )
    {
		if ( arg.length() < 1  ) {
        	return Py::False();
		} 

	    Py::Long hdr( arg[0] );
		bool c =  _api->isCurrency( hdr.as_long() );
		if ( c ) {
			return Py::True();
		} else {
			return Py::False();
		}
    }
    PYCXX_VARARGS_METHOD_DECL( Jaguar, Jaguar_isCurrency )

    Py::Object  Jaguar_isDefinitelyWritable( const Py::Tuple &arg )
    {
		if ( arg.length() < 1  ) {
        	return Py::False();
		} 

	    Py::Long hdr( arg[0] );
		bool c =  _api->isDefinitelyWritable( hdr.as_long() );
		if ( c ) {
			return Py::True();
		} else {
			return Py::False();
		}
    }
    PYCXX_VARARGS_METHOD_DECL( Jaguar, Jaguar_isDefinitelyWritable )


    Py::Object  Jaguar_isNullable( const Py::Tuple &arg )
    {
		if ( arg.length() < 1  ) {
        	return Py::False();
		} 

	    Py::Long hdr( arg[0] );
		bool c =  _api->isNullable( hdr.as_long() );
		if ( c ) {
			return Py::True();
		} else {
			return Py::False();
		}
    }
    PYCXX_VARARGS_METHOD_DECL( Jaguar, Jaguar_isNullable )

    Py::Object  Jaguar_isReadOnly( const Py::Tuple &arg )
    {
		if ( arg.length() < 1  ) {
        	return Py::False();
		} 

	    Py::Long hdr( arg[0] );
		bool c =  _api->isReadOnly( hdr.as_long() );
		if ( c ) {
			return Py::True();
		} else {
			return Py::False();
		}
    }
    PYCXX_VARARGS_METHOD_DECL( Jaguar, Jaguar_isReadOnly )


    Py::Object  Jaguar_isSearchable( const Py::Tuple &arg )
    {
		if ( arg.length() < 1  ) {
        	return Py::False();
		} 

	    Py::Long hdr( arg[0] );
		bool c =  _api->isSearchable( hdr.as_long() );
		if ( c ) {
			return Py::True();
		} else {
			return Py::False();
		}
    }
    PYCXX_VARARGS_METHOD_DECL( Jaguar, Jaguar_isSearchable )


    Py::Object  Jaguar_isSigned( const Py::Tuple &arg )
    {
		if ( arg.length() < 1  ) {
        	return Py::False();
		} 

	    Py::Long hdr( arg[0] );
		bool c =  _api->isSigned( hdr.as_long() );
		if ( c ) {
			return Py::True();
		} else {
			return Py::False();
		}
    }
    PYCXX_VARARGS_METHOD_DECL( Jaguar, Jaguar_isSigned )

  protected:
	JaguarAPI   *_api;

};


#if 0
class JaguarOld: public Py::PythonExtension< JaguarOld >
{
  public:
    JaguarOld()
    {
    }

    virtual ~JaguarOld()
    {
    }

    static void init_type(void)
    {
        behaviors().name( "JaguarOld" );
        behaviors().doc( "JaguarOld class supports Python 2.*" );

		/***
        add_noargs_method( "JaguarOld_func_noargs", &JaguarOld::JaguarOld_func_noargs );
        add_varargs_method( "JaguarOld_func_varargs", &JaguarOld::JaguarOld_func_varargs );
        // add_keyword_method( "JaguarOld_func_keyword", &JaguarOld::JaguarOld_func_keyword );
		***/
    }

	/***
    Py::Object JaguarOld_func_noargs( void )
    {
        std::cout << "JaguarOld_func_noargs Called." << std::endl;
        return Py::None();
    }

    Py::Object JaguarOld_func_varargs( const Py::Tuple &args )
    {
        std::cout << "JaguarOld_func_varargs Called with " << args.length() << " normal arguments." << std::endl;
        return Py::None();
    }
	***/

};
#endif

//PYCXX_USER_EXCEPTION_STR_ARG( JaguarError )

class jaguarpy_module : public Py::ExtensionModule<jaguarpy_module>
{
  public:
    jaguarpy_module() : Py::ExtensionModule<jaguarpy_module>( "jaguarpy" ) 
    {
        // JaguarOld::init_type();
        Jaguar::init_type();

        add_keyword_method("func", &jaguarpy_module::func, "documentation for func()");

        // after initialize the moduleDictionary will exist
        initialize( "documentation for the jaguar module" );

        Py::Dict d( moduleDictionary() );
        // d["var"] = Py::String( "var value" );
        Py::Object x( Jaguar::type() );
        d["Jaguar"] = x;

		// JaguarError::init( *this );
    }

    virtual ~jaguarpy_module()
    {}

private:

    Py::Object func( const Py::Tuple &args, const Py::Dict &kwds )
    {
        return Py::None();
    }

};

#if defined( _WIN32 )
#define EXPORT_SYMBOL __declspec( dllexport )
#else
#define EXPORT_SYMBOL
#endif

extern "C" EXPORT_SYMBOL PyObject *PyInit_jaguarpy()
{
#if defined(PY_WIN32_DELAYLOAD_PYTHON_DLL)
    Py::InitialisePythonIndirectPy::Interface();
#endif

    static jaguarpy_module* jaguarpy = new jaguarpy_module;
    return jaguarpy->module().ptr();
}

// symbol required for the debug version
extern "C" EXPORT_SYMBOL PyObject *PyInit_jaguarpy_d()
{ 
    return PyInit_jaguarpy();
}
