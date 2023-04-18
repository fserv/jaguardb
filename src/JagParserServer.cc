
#include <JagGlobalDef.h>
#include <JagDBServer.h>
#include <JagTableSchema.h>
#include <JagIndexSchema.h>
#include "JagParser.h"
#include "JagLocalDiskHash.h"

const JagColumn* JagParser::getColumn( const Jstr &db, const Jstr &objname, const Jstr &colName ) const
{
	if ( _isCli ) {
		in("s300823 fatal error getColumn() in ParserServer.cc" );
		exit(31);
	}
	const JagDBServer *srv = (const JagDBServer*)_obj;

    if ( srv ) {
        JagIndexSchema *schema = srv->_indexschema;
        if ( schema ) {
            const JagColumn* ptr = schema->getColumn( db, objname, colName );
            if ( ptr ) return ptr;
        }

        JagTableSchema *schema2 = srv->_tableschema;
        if ( ! schema2 ) {
            return NULL;
        }
        return schema2->getColumn( db, objname, colName );
    }
    return NULL; 
}

