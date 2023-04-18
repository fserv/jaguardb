
#include <JagGlobalDef.h>
#include "JagParser.h"

const JagColumn* JagParser::getColumn( const Jstr &db, const Jstr &objname, const Jstr &colName ) const
{
	if ( ! _isCli ) {
        d("cl088838 fatal error getColumn JagParserClient.cc\n");
		exit(32);
	}
	const JaguarCPPClient *cli = (const JaguarCPPClient*)_obj;

    if ( cli ) {
        JagHashMap<AbaxString, JagTableOrIndexAttrs> *schemaMap = cli->_schemaMap;
        if ( ! schemaMap ) {
            return NULL;
        }
         bool rc2;
         AbaxString dbobj = AbaxString( db ) + "." + objname;
         JagTableOrIndexAttrs& objAttr = schemaMap->getValue( dbobj, rc2 );
         if ( ! rc2 ) {
            return NULL;
         }

         int pos =  objAttr.schemaRecord.getPosition( colName );
         if ( pos < 0 ) {
            return NULL;
         }

         return &(*objAttr.schemaRecord.columnVector)[pos];
    }

    return NULL; 
}

