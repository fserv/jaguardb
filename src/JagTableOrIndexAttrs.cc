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
#include <JagTableOrIndexAttrs.h>
#include <JagHashMap.h>
#include <JagUtil.h>

JagTableOrIndexAttrs::JagTableOrIndexAttrs() 
{
	schAttr = NULL;
	keylen = vallen = numKeys = numCols = 0;
	dbobjnum = 0;
	defUpdDatetime = defDatetime = updDatetime = JAG_S_COL_SPARE_DEFAULT;
	hasFile = false;
	isSWO = false;

	schAttr = NULL; 
	numschAttr = 0;
}

JagTableOrIndexAttrs::~JagTableOrIndexAttrs() 
{
	//d("s28039 JagTableOrIndexAttrs::~JagTableOrIndexAttrs schAttr=%0x numschAttr=%d\n", schAttr, numschAttr );
	if ( schAttr ) {
		//d("s0334 schAttr=%0x deelte [] it ...\n", schAttr );
		delete [] schAttr;
		schAttr = NULL;
		numschAttr = 0;
		//d("s0334 schAttr=%0x deelte [] it done \n", schAttr );
	}
}

// copy ctor
JagTableOrIndexAttrs::JagTableOrIndexAttrs( const JagTableOrIndexAttrs &o )
{
	//d("s8301 JagTableOrIndexAttrs:: copy ctor ...\n" );
	copyData( o );
	schAttr = NULL;
	numschAttr = 0;
	
	if ( o.numschAttr > 0 ) {
		//d("s1034  o.numschAttr=%d new JagSchemaAttribute[]\n", o.numschAttr );
		schAttr = new JagSchemaAttribute[o.numschAttr];
		for ( int i = 0; i < o.numschAttr; ++i ) {
			schAttr[i] = o.schAttr[i];
			//d("s3036 schAttr[%d] = o.schAttr[i]\n", i );
		}
		schAttr[0].record = o.schAttr[0].record;
	}
	numschAttr = o.numschAttr;
}

JagTableOrIndexAttrs& JagTableOrIndexAttrs::operator=( const JagTableOrIndexAttrs &o )
{
	if ( this == &o ) { return *this; }

	//d("s8301 JagTableOrIndexAttrs::operator= ...\n" );
	copyData( o );
	
	// JagSchemaAttribute *schAttr;  // to be array of JagSchemaAttribute objects 
	if ( schAttr ) {
		//d("s203889 delete [] schAttr=%0x ...\n", schAttr );
		delete [] schAttr;
		schAttr = NULL;
		numschAttr = 0;
		//d("s4093939 delete [] schAttr=%0x doe ...\n", schAttr );
	}

	if ( o.numschAttr > 0 ) {
		//d("s2034  asign o.numschAttr=%d new JagSchemaAttribute[]\n", o.numschAttr );
		schAttr = new JagSchemaAttribute[o.numschAttr];
		for ( int i = 0; i < o.numschAttr; ++i ) {
			schAttr[i] = o.schAttr[i];
			//d("s3036 schAttr[%d] = o.schAttr[i]\n", i );
		}
	}
	numschAttr = o.numschAttr;
	return *this;
}

void JagTableOrIndexAttrs::copyData( const JagTableOrIndexAttrs &o )
{
	keylen = o.keylen;
	vallen = o.vallen;
	numKeys = o.numKeys;
	numCols = o.numCols;
	dbobjnum = o.dbobjnum; 
	defUpdDatetime = o.defUpdDatetime; 
	defDatetime = o.defDatetime; 
	updDatetime = o.updDatetime;
	hasFile = o.hasFile; 
	isSWO = o.isSWO; 
	dbName = o.dbName;
	tableName = o.tableName;
	indexName = o.indexName; 
	schemaString = o.schemaString;
	schemaRecord = o.schemaRecord;

	uuidarr = o.uuidarr;
	deftimestamparr = o.deftimestamparr;
	schmap = o.schmap; 
}

void JagTableOrIndexAttrs::makeSchemaAttrArray( int num )
{
	if ( schAttr ) {
		delete [] schAttr;
		schAttr = NULL;
		numschAttr = 0;
	}

	if ( num> 0 ) {
		schAttr = new JagSchemaAttribute[ num ];
		numschAttr = num;
	}
}

