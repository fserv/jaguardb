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
#include <string>
#include "jaguarphp.h"
#include "Jaguar.h"
#include "Zend/zend.h"

zend_object_handlers jaguar_object_handlers;
typedef struct _jaguar_object {
    Jaguar *jaguar;
    zend_object std;
} jaguar_object;
zend_class_entry *jaguar_ce;

static inline jaguar_object *php_jaguar_obj_from_obj(zend_object *obj) {
    return (jaguar_object*)((char*)(obj) - XtOffsetOf(jaguar_object, std));
}

#define Z_TSTOBJ_P(zv)  php_jaguar_obj_from_obj(Z_OBJ_P((zv)))

zend_object *jaguar_object_new(zend_class_entry *ce TSRMLS_DC)
{
    jaguar_object *intern = (jaguar_object*)ecalloc(1, sizeof(jaguar_object) + zend_object_properties_size(ce));
    zend_object_std_init(&intern->std, ce TSRMLS_CC);
    object_properties_init(&intern->std, ce);
    intern->std.handlers = &jaguar_object_handlers;
    return &intern->std;
}

PHP_METHOD(Jaguar, __construct)
{
    zval *id = getThis();
    jaguar_object *intern;

    intern = Z_TSTOBJ_P(id);
    if(intern != NULL) {
        intern->jaguar = new Jaguar();
		//intern->jaguar->setDebug(true); 
    }
}

PHP_METHOD(Jaguar, connect)
{
	char *ip, *uid, *pass, *dbname, *usock;
	size_t iplen, uidlen, passlen, dblen, usocklen;
	usock = NULL;

	long port, cflag;
	cflag = 0;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, (char*)"slsss|sl", 
			&ip, &iplen, &port, &uid, &uidlen, &pass, &passlen, &dbname, 
			&dblen, &usock, &usocklen, &cflag ) == FAILURE) {
		zend_error( E_WARNING, "Usage: connect(host, port, uid, passwd, dbname, unixsock, cflag)");
        RETURN_NULL();
    }

	// printf("c88112 connect ip=[%s] port=[%d] uid=[%s] pass=[%s] dbname=[%s] \n", ip, port, uid, pass, dbname );

	zval *id = getThis();
	jaguar_object *obj  = Z_TSTOBJ_P(id);
	if (  obj ) { 
        RETURN_LONG( obj->jaguar->connect( ip, port, uid, pass, dbname, usock, cflag ) );
	} else {
        RETURN_NULL();
	}
	
}


PHP_METHOD(Jaguar, query)
{
	zend_bool  reply = 1;
	char *querys;
	size_t qlen;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, (char*)"s|b", 
			&querys, &qlen, &reply ) == FAILURE) {
		zend_error( E_WARNING, "Usage: query( qstr, needReply) ");
        RETURN_NULL();
    }

    jaguar_object *obj = (jaguar_object *)Z_TSTOBJ_P( getThis() );
    if ( obj != NULL) {
		//php_printf("p82824 obj->jaguar->query( querys, reply ) ...\n" );
        RETURN_LONG( obj->jaguar->query( querys, reply ) );
    } else {
		//php_printf("p82829 RETURN_NULL()\n" );
        RETURN_NULL();
	}
}

PHP_METHOD(Jaguar, execute)
{
	char *querys;
	size_t qlen;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, (char*)"s", 
			&querys, &qlen ) == FAILURE) {
		zend_error( E_WARNING, "Usage: execute( command ) ");
        RETURN_NULL();
    }

    jaguar_object *obj = (jaguar_object *)Z_TSTOBJ_P( getThis() );
    if (obj != NULL) {
        RETURN_LONG( obj->jaguar->execute( querys ) );
    } else {
        RETURN_NULL();
	}
}

PHP_METHOD(Jaguar, reply)
{
	zend_bool  hdr = 0;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, (char*)"|b", 
			&hdr ) == FAILURE) {
		// zend_error( E_WARNING, "Usage: reply() function is supplied with wrong arguments");
        RETURN_NULL();
    }

    jaguar_object *obj = (jaguar_object *)Z_TSTOBJ_P( getThis() );
    if ( obj != NULL) {
        RETURN_LONG( obj->jaguar->reply( hdr ) );
    } else {
        RETURN_NULL();
	}
}

PHP_METHOD(Jaguar, printRow )
{
    jaguar_object *obj = (jaguar_object *)Z_TSTOBJ_P( getThis() );
    if ( obj != NULL) {
        RETURN_LONG( obj->jaguar->printRow( ) );
    } else {
        RETURN_NULL();
	}
}


PHP_METHOD(Jaguar, printAll )
{
    Jaguar *jaguar;
    jaguar_object *obj = (jaguar_object *)Z_TSTOBJ_P( getThis() );
    jaguar = obj->jaguar;
    if (jaguar != NULL) {
        jaguar->printAll( );
        RETURN_NULL();
    } else {
        RETURN_NULL();
	}
}

PHP_METHOD(Jaguar, close)
{
    Jaguar *jaguar;
    jaguar_object *obj = (jaguar_object *)Z_TSTOBJ_P( getThis() );
    jaguar = obj->jaguar;
    if (jaguar != NULL) {
        jaguar->close();
    }
}

PHP_METHOD(Jaguar, getDatabase)
{
    Jaguar *jaguar;
    jaguar_object *obj = (jaguar_object *)Z_TSTOBJ_P( getThis() );
    jaguar = obj->jaguar;
    if (jaguar != NULL) {
        //RETURN_STRING(jaguar->getDatabase(), 1 );
        RETURN_STRING(jaguar->getDatabase() );
    }

    RETURN_NULL();
}

PHP_METHOD(Jaguar, error)
{
    Jaguar *jaguar;
    jaguar_object *obj = (jaguar_object *)Z_TSTOBJ_P( getThis() );
    jaguar = obj->jaguar;
    if (jaguar != NULL) {
        RETURN_STRING(jaguar->error() );
    }

    RETURN_NULL();
}

PHP_METHOD(Jaguar, hasError)
{
    Jaguar *jaguar;
    jaguar_object *obj = (jaguar_object *)Z_TSTOBJ_P( getThis() );
    jaguar = obj->jaguar;
    if (jaguar != NULL) {
		if ( jaguar->hasError() ) {
			RETURN_TRUE;
		} else {
			RETURN_FALSE;
		}
    }

    RETURN_NULL();
}

PHP_METHOD(Jaguar, freeResult)
{
    Jaguar *jaguar;
    jaguar_object *obj = (jaguar_object *)Z_TSTOBJ_P( getThis() );
    jaguar = obj->jaguar;
    if (jaguar != NULL) {
		jaguar->freeResult();
    }
}


PHP_METHOD(Jaguar, getNthValue )
{
	zend_long col;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, (char*)"l", 
			&col ) == FAILURE) {
		zend_error( E_WARNING, "Usage: getNthValue(column)  col: 1--N");
        RETURN_NULL();
    }

    Jaguar *jaguar;
    jaguar_object *obj = (jaguar_object *)Z_TSTOBJ_P( getThis() );
    jaguar = obj->jaguar;
    if (jaguar != NULL) {
		const char *val = NULL;
		val = jaguar->getNthValue( col );
		if ( val ) {
			std::string res( val );
			//free( val );
        	RETURN_STRING( res.c_str() );
		} else {
    		RETURN_NULL();
		}
    }

    RETURN_NULL();
}


PHP_METHOD(Jaguar, getValue )
{
	char *name;
	size_t namelen;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, (char*)"s", 
			&name, &namelen ) == FAILURE) {
		zend_error( E_WARNING, "Usage: getValue(name)");
        RETURN_NULL();
    }

    Jaguar *jaguar;
    jaguar_object *obj = (jaguar_object *)Z_TSTOBJ_P( getThis() );
    jaguar = obj->jaguar;
    if (jaguar != NULL) {
		char *val = NULL;
		val = jaguar->getValue(name);
		if ( val ) {
			std::string res( val );
			free( val );
        	RETURN_STRING( res.c_str() );
		} else {
    		RETURN_NULL();
		}
    }

    RETURN_NULL();
}

PHP_METHOD(Jaguar, getAllByName )
{
	char *name;
	size_t namelen;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, (char*)"s", 
			&name, &namelen ) == FAILURE) {
		zend_error( E_WARNING, "Usage: getAllByName(name)");
        RETURN_NULL();
    }

    Jaguar *jaguar;
    jaguar_object *obj = (jaguar_object *)Z_TSTOBJ_P( getThis() );
    jaguar = obj->jaguar;
    if (jaguar != NULL) {
		char *val = NULL;
		val = jaguar->getAllByName(name);
		if ( val ) {
			std::string res( val );
			free( val );
        	RETURN_STRING( res.c_str() );
		} else {
    		RETURN_NULL();
		}
    }

    RETURN_NULL();
}


PHP_METHOD(Jaguar, getLong )
{
	char *name;
	size_t namelen;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, (char*)"s", 
			&name, &namelen ) == FAILURE) {
		zend_error( E_WARNING, "Usage: getLong(name)");
        RETURN_NULL();
    }

    Jaguar *jaguar;
    jaguar_object *obj = (jaguar_object *)Z_TSTOBJ_P( getThis() );
    jaguar = obj->jaguar;
    if (jaguar != NULL) {
		long long val;
		int rc = jaguar->getLong( name, &val);
		if ( rc ) {
        	RETURN_LONG( val );
		} else {
    		RETURN_NULL();
		}
    }

    RETURN_NULL();
}


PHP_METHOD(Jaguar, getDouble )
{
	char *name;
	size_t namelen;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, (char*)"s", 
			&name, &namelen ) == FAILURE) {
		zend_error( E_WARNING, "Usage: getDouble(name)");
        RETURN_NULL();
    }

    Jaguar *jaguar;
    jaguar_object *obj = (jaguar_object *)Z_TSTOBJ_P( getThis() );
    jaguar = obj->jaguar;
    if (jaguar != NULL) {
		double val;
		int rc = jaguar->getDouble( name, &val);
		if ( rc ) {
        	RETURN_DOUBLE( val );
		} else {
    		RETURN_NULL();
		}
    }

    RETURN_NULL();
}

PHP_METHOD(Jaguar, getMessage)
{
    Jaguar *jaguar;
    jaguar_object *obj = (jaguar_object *)Z_TSTOBJ_P( getThis() );
    jaguar = obj->jaguar;
    if (jaguar != NULL) {
        RETURN_STRING(jaguar->getMessage() );
    }

    RETURN_NULL();
}

PHP_METHOD(Jaguar, getLastUuid)
{
    Jaguar *jaguar;
    jaguar_object *obj = (jaguar_object *)Z_TSTOBJ_P( getThis() );
    jaguar = obj->jaguar;
    if (jaguar != NULL) {
        RETURN_STRING(jaguar->getLastUuid() );
    }

    RETURN_NULL();
}


PHP_METHOD(Jaguar, getAll)
{
    Jaguar *jaguar;
    jaguar_object *obj = (jaguar_object *)Z_TSTOBJ_P( getThis() );
    jaguar = obj->jaguar;
    if (jaguar != NULL) {
		char *val = jaguar->getAll();
		if ( val ) {
			std::string res( val );
			free( val );
        	RETURN_STRING( res.c_str() );
		} else {
    		RETURN_NULL();
		}
    }

    RETURN_NULL();
}

PHP_METHOD(Jaguar, getAllByIndex )
{
	zend_long col;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, (char*)"l", 
			&col ) == FAILURE) {
		zend_error( E_WARNING, "Usage: getNthValue(column)  col: 1--N");
        RETURN_NULL();
    }

    Jaguar *jaguar;
    jaguar_object *obj = (jaguar_object *)Z_TSTOBJ_P( getThis() );
    jaguar = obj->jaguar;
    if (jaguar != NULL) {
		const char *val = NULL;
		val = jaguar->getAllByIndex( col );
		if ( val ) {
			std::string res( val );
			//free( val );
        	RETURN_STRING( res.c_str() );
		} else {
    		RETURN_NULL();
		}
    }

    RETURN_NULL();
}


PHP_METHOD(Jaguar, jsonString)
{
    Jaguar *jaguar;
    jaguar_object *obj = (jaguar_object *)Z_TSTOBJ_P( getThis() );
    jaguar = obj->jaguar;
    if (jaguar != NULL) {
        RETURN_STRING(jaguar->jsonString() );
    }

    RETURN_NULL();
}


PHP_METHOD(Jaguar, getColumnCount )
{
    Jaguar *jaguar;
    jaguar_object *obj = (jaguar_object *)Z_TSTOBJ_P( getThis() );
    jaguar = obj->jaguar;
    if (jaguar != NULL) {
       	RETURN_LONG( jaguar->getColumnCount() );
    }

    RETURN_NULL();
}

PHP_METHOD(Jaguar, getCluster )
{
    Jaguar *jaguar;
    jaguar_object *obj = (jaguar_object *)Z_TSTOBJ_P( getThis() );
    jaguar = obj->jaguar;
    if (jaguar != NULL) {
       	RETURN_LONG( jaguar->getCluster() );
    }

    RETURN_NULL();
}

PHP_METHOD(Jaguar, getColumnName )
{
	int col;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, (char*)"l", 
			&col ) == FAILURE) {
		zend_error( E_WARNING, "Usage: getColumnName(column)  column: 1--N");
        RETURN_NULL();
    }

    Jaguar *jaguar;
    jaguar_object *obj = (jaguar_object *)Z_TSTOBJ_P( getThis() );
    jaguar = obj->jaguar;
    if (jaguar != NULL) {
		char *val = NULL;
		val = jaguar->getColumnName( col );
		if ( val ) {
			std::string res( val );
			free( val );
        	RETURN_STRING( res.c_str() );
		} else {
    		RETURN_EMPTY_STRING();
		}
    }

    RETURN_NULL();
}


PHP_METHOD(Jaguar, getColumnLabel )
{
	zend_long col;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, (char*)"l", 
			&col ) == FAILURE) {
		zend_error( E_WARNING, "Usage: getColumnLabel(column)  column: 1--N");
        RETURN_NULL();
    }

    Jaguar *jaguar;
    jaguar_object *obj = (jaguar_object *)Z_TSTOBJ_P( getThis() );
    jaguar = obj->jaguar;
    if (jaguar != NULL) {
		char *val = NULL;
		val = jaguar->getColumnLabel( col );
		if ( val ) {
			std::string res( val );
			free( val );
        	RETURN_STRING( res.c_str() );
		} else {
    		RETURN_EMPTY_STRING();
		}
    }

    RETURN_NULL();
}

PHP_METHOD(Jaguar, getColumnTypeName )
{
	zend_long col;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, (char*)"l", 
			&col ) == FAILURE) {
		zend_error( E_WARNING, "Usage: getColumnTypeName(column)  column: 1--N");
        RETURN_NULL();
    }

    Jaguar *jaguar;
    jaguar_object *obj = (jaguar_object *)Z_TSTOBJ_P( getThis() );
    jaguar = obj->jaguar;
    if (jaguar != NULL) {
		char *val = NULL;
		val = jaguar->getColumnTypeName( col );
		if ( val ) {
			std::string res( val );
			free( val );
        	RETURN_STRING( res.c_str() );
		} else {
    		RETURN_EMPTY_STRING();
		}
    }

    RETURN_NULL();
}

PHP_METHOD(Jaguar, getTableName )
{
	zend_long col;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, (char*)"l", 
			&col ) == FAILURE) {
		zend_error( E_WARNING, "Usage: getTableName(column)  column: 1--N");
        RETURN_NULL();
    }

    Jaguar *jaguar;
    jaguar_object *obj = (jaguar_object *)Z_TSTOBJ_P( getThis() );
    jaguar = obj->jaguar;
    if (jaguar != NULL) {
		char *val = NULL;
		val = jaguar->getTableName( col );
		if ( val ) {
			std::string res( val );
			free( val );
        	RETURN_STRING( res.c_str() );
		} else {
    		RETURN_EMPTY_STRING();
		}
    }

    RETURN_NULL();
}

PHP_METHOD(Jaguar, getColumnType )
{
	int col;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, (char*)"l", 
			&col ) == FAILURE) {
		zend_error( E_WARNING, "Usage: getColumnType(column)  column: 1--N");
        RETURN_NULL();
    }

    Jaguar *jaguar;
    jaguar_object *obj = (jaguar_object *)Z_TSTOBJ_P( getThis() );
    jaguar = obj->jaguar;
    if (jaguar != NULL) {
		int val = jaguar->getColumnType( col );
       	RETURN_LONG( val );
    }

    RETURN_NULL();
}

 
//function_entry jaguar_methods[] = {
const zend_function_entry jaguar_methods[] = {
    PHP_ME(Jaguar,  __construct,     NULL, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
    PHP_ME(Jaguar,  connect,  		 NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Jaguar,  close,  		 NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Jaguar,  query,  		 NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Jaguar,  execute,  		 NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Jaguar,  reply,  		 NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Jaguar,  printRow,  		 NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Jaguar,  getDatabase, 	 NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Jaguar,  error, 	 		 NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Jaguar,  hasError,  		 NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Jaguar,  freeResult, 	 NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Jaguar,  getNthValue, 	 NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Jaguar,  getValue, 	 	 NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Jaguar,  getLong, 	 	 NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Jaguar,  getDouble, 	 	 NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Jaguar,  getMessage, 	 NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Jaguar,  jsonString, 	 NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Jaguar,  getColumnCount, 	 NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Jaguar,  getColumnName, 	 NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Jaguar,  getColumnLabel, 	 NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Jaguar,  getColumnType, 	 NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Jaguar,  getColumnTypeName, 	 NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Jaguar,  getTableName, 	 NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Jaguar,  printAll, 	 NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Jaguar,  getAll, 	 NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Jaguar,  getAllByName, 	 NULL, ZEND_ACC_PUBLIC)
    PHP_ME(Jaguar,  getAllByIndex, 	 NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
    // {NULL, NULL, NULL}
};

static void jaguar_object_destroy(zend_object *object)
{
    jaguar_object *my_obj;
    my_obj = (jaguar_object*)((char *) object - XtOffsetOf(jaguar_object, std));
    zend_objects_destroy_object(object);
}

static void jaguar_object_free(zend_object *object)
{
    jaguar_object *my_obj;
    my_obj = (jaguar_object *)((char *) object - XtOffsetOf(jaguar_object, std));
    delete my_obj->jaguar;
    zend_object_std_dtor(object);
}

 
PHP_MINIT_FUNCTION(jaguar)
{
    zend_class_entry ce;
    INIT_CLASS_ENTRY(ce, "Jaguar", jaguar_methods);
    jaguar_ce = zend_register_internal_class(&ce TSRMLS_CC);
    jaguar_ce->create_object = jaguar_object_new;

    memcpy(&jaguar_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    //jaguar_object_handlers.clone_obj = NULL;
	jaguar_object_handlers.free_obj = jaguar_object_free;
 	jaguar_object_handlers.dtor_obj = jaguar_object_destroy;
	jaguar_object_handlers.offset = XtOffsetOf(jaguar_object, std);
    return SUCCESS;
}
 
zend_module_entry jaguarphp_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    PHP_JAGUAR_EXTNAME,
    NULL,        /* Functions */
    PHP_MINIT(jaguar),        /* MINIT */
    NULL,        /* MSHUTDOWN */
    NULL,        /* RINIT */
    NULL,        /* RSHUTDOWN */
    NULL,        /* MINFO */
#if ZEND_MODULE_API_NO >= 20010901
    PHP_JAGUAR_EXTVER,
#endif
    STANDARD_MODULE_PROPERTIES
};
 
#ifdef COMPILE_DL_JAGUARPHP
extern "C" {
ZEND_GET_MODULE(jaguarphp)
}
#endif


