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
#ifndef _TableOrIndexAttrs_h_
#define _TableOrIndexAttrs_h_

#include <JagGlobalDef.h>

#include <JagVector.h>
#include <JagSchemaRecord.h>
#include <JagTableUtil.h>

//class JagSchemaAttribute;
class JagDataAggregate;
template <class K, class V> class JagHashMap;


//for uuid and timestamp/datetime 
class PosOffsetLength
{
  public:
	jagint   pos;  
	jagint   offset;
	jagint   length;
};

//for table/index object data members, used by Client only for now
class JagTableOrIndexAttrs
{
  public:
    JagTableOrIndexAttrs();
	~JagTableOrIndexAttrs();
	JagTableOrIndexAttrs& operator=( const JagTableOrIndexAttrs &other );
	JagTableOrIndexAttrs( const JagTableOrIndexAttrs &other );
	void makeSchemaAttrArray( int num );

	int	keylen;
	int	vallen;
	int	numKeys;
	int	numCols;
	int dbobjnum; // number of dbobj parts; e.g. db.tab && db.idx is 2, and db.tab.idx is 3
	char defUpdDatetime; // has default update time stamp or not ( 'U' or ' ' )
	char defDatetime; // has default update time stamp or not ( 'T' or ' ' )
	char updDatetime; // has default update time stamp or not ( 'P' or ' ' )
	bool hasFile; // has file type -- need to transmit files from client to server, single insert
	bool isSWO; 
	Jstr dbName;
	Jstr tableName;
	Jstr indexName; // if not empty, this object is an index of dbName.tableName
	Jstr schemaString;
	JagSchemaRecord schemaRecord;
	JagVector<PosOffsetLength> uuidarr;
	JagVector<PosOffsetLength> deftimestamparr;
	JagHashStrInt schmap; 
	
	JagSchemaAttribute *schAttr;  // to be array of JagSchemaAttribute objects 
	int    numschAttr;

  protected:
	void copyData( const JagTableOrIndexAttrs &other );
	
};

#endif
