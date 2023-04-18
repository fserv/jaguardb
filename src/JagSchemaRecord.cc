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
#include <JagGlobalDef.h>

#include <abax.h>
#include <JagUtil.h>
#include <JagStrSplit.h>
#include <JagSchemaRecord.h>
#include <JagParseParam.h>
#include <JagParser.h>
#include <vector>
#include <unordered_set>


bool sortByTick(const std::string &lhs, const std::string &rhs)
{
    std::string tick1 = lhs;
    std::string tick2 = rhs;
    const char *p1_ = strchr(lhs.c_str(), '_' );
    const char *p2_ = strchr(rhs.c_str(), '_' );
    if ( p1_ ) {
        tick1 = std::string( lhs.c_str(), p1_ - lhs.c_str() );
    }
    if ( p2_ ) {
        tick2 = std::string( rhs.c_str(), p2_ - rhs.c_str() );
    }
    int d1 = atoi( tick1.c_str() );
    int d2 = atoi( tick2.c_str() );
    return d1 < d2;
}

JagSchemaRecord::JagSchemaRecord( bool newVec )
{
	init( newVec );
}

void JagSchemaRecord::init( bool newVec )
{
	type[0] = 'N';
	type[1] = 'A';
	keyLength = 16;
	valueLength = 16;
	ovalueLength = 0;
	if ( newVec ) {
		columnVector = new JagVector<JagColumn>();
	} else {
		columnVector = NULL;
	}
	numKeys = 0;
	numValues = 0;
	hasMute = false;
	lastKeyColumn = -1;
	tableProperty = "0!0!0!0";
	hasRollupColumn = false;
    keyHasFile = false;
    keyHasGeo = false;
}

JagSchemaRecord::~JagSchemaRecord()
{
	destroy();
}

void JagSchemaRecord::destroy(  AbaxDestroyAction act)
{
	if ( columnVector ) {
		delete columnVector;
		columnVector = NULL;
	}
}

JagSchemaRecord::JagSchemaRecord( const JagSchemaRecord& other )
{
	copyData( other );
	columnVector = NULL;

	if ( other.columnVector ) {
    	jagint  size = other.columnVector->size();
    	columnVector = new JagVector<JagColumn>( size+8 );
    	for ( int i = 0; i < size; ++i ) {
    		columnVector->append( (*other.columnVector)[i] );
    		_nameMap.addKeyValue( (*other.columnVector)[i].name.c_str(), i );
    	}
	}
}

JagSchemaRecord& JagSchemaRecord::operator=( const JagSchemaRecord& other )
{
	if ( columnVector == other.columnVector ) {
		return *this;
	}

	destroy();
	copyData( other );
	_nameMap.reset();

	jagint  size = 0;
	if ( other.columnVector ) {
		size =  other.columnVector->size();
		columnVector = new JagVector<JagColumn>( size ); 
	}

	if ( size > 0 ) {
		for ( int i = 0; i < size; ++i ) {
			columnVector->append( (*other.columnVector)[i] );
			_nameMap.addKeyValue( (*other.columnVector)[i].name.c_str(), i );
		}
	}
	return *this;
}

void JagSchemaRecord::copyData( const JagSchemaRecord& other )
{
	// 7 basic data elements
	type[0] = other.type[0];
	type[1] = other.type[1];
	keyLength = other.keyLength;
	numKeys = other.numKeys;
	numValues = other.numValues;
	valueLength = other.valueLength;
	ovalueLength = other.ovalueLength;
	lastKeyColumn = other.lastKeyColumn;
	tableProperty = other.tableProperty;
	keyHasFile = other.keyHasFile;
	keyHasGeo = other.keyHasGeo;
}

bool JagSchemaRecord::print() const
{
	if ( ! columnVector ) return false;

	printf("%c%c|%d|%d|%s|{", type[0], type[1], keyLength, valueLength, tableProperty.c_str() );
	int len = columnVector->size();

	for ( int i = 0; i < len; ++i ) {
		printf("!%s!%s!%d!%d!%d(func=%d)!",
				(*columnVector)[i].name.c_str(),
				(*columnVector)[i].type.c_str(),
				(*columnVector)[i].offset,
				(*columnVector)[i].length,
				(*columnVector)[i].sig,
				(*columnVector)[i].func );

		for ( int j = 0; j < JAG_SCHEMA_SPARE_LEN; ++j ) {
			printf("%c", (*columnVector)[i].spare[j] );
		}
		printf("!");
		printf("%d!", (*columnVector)[i].srid );
		printf("%d!", (*columnVector)[i].begincol );
		printf("%d!", (*columnVector)[i].endcol );

		printf("%d!", (*columnVector)[i].metrics );
		//printf("%d!", (*columnVector)[i].dummy2 );
		printf("%s!", (*columnVector)[i].rollupWhere.s() );
		printf("%d!", (*columnVector)[i].dummy3 );
		printf("%d!", (*columnVector)[i].dummy4 );
		printf("%d!", (*columnVector)[i].dummy5 );
		printf("%d!", (*columnVector)[i].dummy6 );
		printf("%d!", (*columnVector)[i].dummy7 );
		printf("%d!", (*columnVector)[i].dummy8 );
		printf("%d!", (*columnVector)[i].dummy9 );
		printf("%d!", (*columnVector)[i].dummy10 );

		printf("|");
	}

	printf("}\n");
	return 1;
}

bool JagSchemaRecord::renameColumn( const AbaxString &oldName, const AbaxString & newName )
{
	int len = columnVector->size();
	AbaxString name;
	bool found = false;
	for ( int i = 0; i < len; ++i ) {
		name = (*columnVector)[i].name;
		if ( name == oldName ) {
			(*columnVector)[i].name = newName;
			_nameMap.removeKey( oldName.c_str() );
			_nameMap.addKeyValue( newName.c_str(), i );
			found = true;
			break;
		}
	}

	return found;
}

bool JagSchemaRecord::setColumn( const AbaxString &colName, const AbaxString &attr, const AbaxString & value )
{
	int len = columnVector->size();
	AbaxString name;
	bool found = false;
	for ( int i = 0; i < len; ++i ) {
		name = (*columnVector)[i].name;
		if ( name == colName ) {
			if ( attr == "srid" ) {
				(*columnVector)[i].srid = jagatoi(value.s());
			}
			found = true;
			break;
		}
	}

	return found;
}


bool JagSchemaRecord::addValueColumnFromSpare( const AbaxString &colName, const Jstr &type, 
											   jagint length, jagint sig )
{
	if ( ! columnVector ) {
		columnVector = new JagVector<JagColumn>();
	}

	int len = columnVector->size();
	AbaxString aname; unsigned int aoffset, alength, asig; 
	Jstr atype;
	char aspare[JAG_SCHEMA_SPARE_LEN]; 
	char aiskey; int afunc;
	aname = (*columnVector)[len-1].name;
	atype = (*columnVector)[len-1].type;
	aoffset = (*columnVector)[len-1].offset;
	alength = (*columnVector)[len-1].length;
	asig = (*columnVector)[len-1].sig;

	for ( int i = 0; i < JAG_SCHEMA_SPARE_LEN; ++i ) {
		aspare[i] = (*columnVector)[len-1].spare[i];
	}

	aiskey = (*columnVector)[len-1].iskey;
	afunc = (*columnVector)[len-1].func;
	
	(*columnVector)[len-1].name = colName;
	(*columnVector)[len-1].type = type;
	(*columnVector)[len-1].length = length;
	(*columnVector)[len-1].sig = sig;
	
	AbaxString dum;
	JagColumn onecolrec;
	onecolrec.name = aname;
	onecolrec.type = atype;
	onecolrec.offset = aoffset+length;
	onecolrec.length = alength-length;
	onecolrec.sig = asig;

	for ( int i = 0; i < JAG_SCHEMA_SPARE_LEN; ++i ) {
		onecolrec.spare[i] = aspare[i];
	}

	onecolrec.iskey = aiskey;
	onecolrec.func = afunc;
	columnVector->append( onecolrec );
	_nameMap.addKeyValue( colName.c_str(), len-1 );
	return true;
}

Jstr JagSchemaRecord::getString() const
{
	if ( ! columnVector ) return "";

	Jstr res;
	char mem[4096];
	char buf[32];
	char buf2[2];
	buf2[1] = '\0';

	memset( mem, 0, 4096 );
	sprintf(mem, "%c%c|%d|%d|%s|{", type[0], type[1], keyLength, valueLength, tableProperty.c_str() );
	res += Jstr( mem );

	int len = columnVector->size();
	for ( int i = 0; i < len; ++i ) {
		memset( mem, 0, 4096 );
		sprintf(mem, "!%s!%s!%d!%d!%d!",
				(*columnVector)[i].name.c_str(),
				(*columnVector)[i].type.c_str(),
				(*columnVector)[i].offset,
				(*columnVector)[i].length,
				(*columnVector)[i].sig );

		for ( int k = 0; k < JAG_SCHEMA_SPARE_LEN; ++ k ) {
			buf2[0] = (*columnVector)[i].spare[k];
			strcat( mem, buf2 );
		}
		strcat( mem, "!" );

		sprintf( buf, "%d!", (*columnVector)[i].srid ); strcat( mem, buf );
		sprintf( buf, "%d!", (*columnVector)[i].begincol ); strcat( mem, buf );
		sprintf( buf, "%d!", (*columnVector)[i].endcol ); strcat( mem, buf );
		sprintf( buf, "%d!", (*columnVector)[i].metrics ); strcat( mem, buf );
		// sprintf( buf, "%d!", (*columnVector)[i].dummy2 ); strcat( mem, buf );
		sprintf( buf, "%s!", (*columnVector)[i].rollupWhere.s() ); strcat( mem, buf );
		sprintf( buf, "%d!", (*columnVector)[i].dummy3 ); strcat( mem, buf );
		sprintf( buf, "%d!", (*columnVector)[i].dummy4 ); strcat( mem, buf );
		sprintf( buf, "%d!", (*columnVector)[i].dummy5 ); strcat( mem, buf );
		sprintf( buf, "%d!", (*columnVector)[i].dummy6 ); strcat( mem, buf );
		sprintf( buf, "%d!", (*columnVector)[i].dummy7 ); strcat( mem, buf );
		sprintf( buf, "%d!", (*columnVector)[i].dummy8 ); strcat( mem, buf );
		sprintf( buf, "%d!", (*columnVector)[i].dummy9 ); strcat( mem, buf );
		sprintf( buf, "%d!", (*columnVector)[i].dummy10 ); strcat( mem, buf );

		res += mem;
		res += "|";
	}
	res += "}";

	return res;
}

Jstr JagSchemaRecord::formatHeadRecord() const
{ 
	return Jstr("NN|0|0|") + tableProperty + "|{"; 
}

Jstr JagSchemaRecord::formatColumnRecord( const char *name, const char *type, int offset, int length, 
													int sig, bool isKey ) const
{
	char buf[256];
	sprintf(buf, "!#%s#!%s!%d!%d!%d!", name, type, offset, length, sig);
	Jstr res = buf;
	if ( isKey ) {
		res += "k a ";
	} else {
		res += "v   ";
	}
	// used 4 bytes in spare

	// -4 means deduct 4 bytes above
	for ( int i = 0;  i < JAG_SCHEMA_SPARE_LEN-4; ++i ) {
		res += " ";
	}
	// res += "!|";
	res += "!0!0!0!0!0!0!0!0!0!0!0!0!0!|";
	return res;
}


Jstr JagSchemaRecord::formatTailRecord() const
{
	return "}";
}

int JagSchemaRecord::parseRecord( const char *str ) 
{
	if ( ! columnVector ) {
		columnVector = new JagVector<JagColumn>();
	}

	//d("s73980 parseRecord str=[%s]\n", str );
	jagint      actkeylen = 0, actvallen = 0, rc = 0;
	JagColumn   onecolrec;
	const char  *p, *q;

	q = p = str;
	numKeys = 0;
	numValues = 0;	
	
	while ( *q != ':' && *q != '|' && *q != '\0' ) { ++q; }

	if ( *q == '\0' ) return -10;

	if ( *q == ':' ) {
		++q;
		type[0] = *q; type[1] = *(q+1);
		while ( *q != '|' && *q != '\0' ) ++q;
		if ( *q == '\0' ) return -20;
	}  else {
		type[0] = *(q-2); type[1] = *(q-1);
	}
	
	// get keylen
	++q; p = q;
	while ( isdigit(*q) ) ++q;
	if ( *q != '|' ) return -21;
	keyLength = rayatoi(p, q-p);
	
	// get vallen
	++q; p = q;
	while ( isdigit(*q) ) ++q;
	if ( *q != '|' ) return -30;
	valueLength = rayatoi(p, q-p);
	
	++q; p = q;
	while ( *q != '|' && *q != '{' && *q != '\0' ) ++q;
	if ( *q == '\0' ) return -42;
	tableProperty = Jstr(p, q-p);  
	if ( *q == '|' && *(q+1) == '{' ) {
		q += 2;
	} else if ( *q == '{' ) {
		q += 1;
	} else {
		return -43;
	}
	
	while ( 1 ) {
		if ( *q != '!' ) {
			return -50;
		}
		p = ++q;
		if ( *q == '#' ) {
			p = ++q;
			while ( 1 ) {
				while ( *q != '#' && *q != '\'' && *q != '"' && *q != '\0' ) ++q;
				if ( *q == '\0' ) return -60;
				else if ( *q == '\'' || *q == '"' ) {
					q = (const char*)jumptoEndQuote(q);
					if ( *q == '\0' ) return -70;
					++q;
				} else break;
			}
			onecolrec.name = Jstr(p, q-p);
			d("s556013 #name=[%s]#\n", onecolrec.name.s() );
			++q;
		} else {
			while ( 1 ) {
				while ( *q != '!' && *q != '\'' && *q != '"' && *q != '\0' ) ++q;
				if ( *q == '\0' ) return -80;
				else if ( *q == '\'' || *q == '"' ) {
					q = (const char*)jumptoEndQuote(q);
					if ( *q == '\0' ) return -90;
					++q;
				} else break;
			}
			onecolrec.name = Jstr(p, q-p);
			//d("s556014 name=[%s]\n", onecolrec.name.s() );
		}

		if ( strcasestr(onecolrec.name.s(), "avg(" ) ) {
			onecolrec.func = JAG_FUNC_AVG;
		} else if ( strcasestr(onecolrec.name.s(), "sum(" ) ) {
			onecolrec.func = JAG_FUNC_SUM;
			//d("s028381 parseRecord got sum() JAG_FUNC_SUM \n");
		} else if ( strcasestr(onecolrec.name.s(), "min(" ) ) {
			onecolrec.func = JAG_FUNC_MIN;
		} else if ( strcasestr(onecolrec.name.s(), "max(" ) ) {
			onecolrec.func = JAG_FUNC_MAX;
		} else if ( strcasestr(onecolrec.name.s(), "stddev(" ) ) {
			onecolrec.func = JAG_FUNC_STDDEV;
		} else if ( strcasestr(onecolrec.name.s(), "count(" ) ) {
			onecolrec.func = JAG_FUNC_COUNT;
		} else if ( strcasestr(onecolrec.name.s(), "first(" ) ) {
			onecolrec.func = JAG_FUNC_FIRST;
		} else if ( strcasestr(onecolrec.name.s(), "last(" ) ) {
			onecolrec.func = JAG_FUNC_LAST;
		} else {
			onecolrec.func = 0;
		}

		++q; p = q; 
		while ( *q != '!' && *q != '\0' ) ++q;
		if ( *q != '!' ) {
			// d("s1283 q=[%s]\n", q );
			return -100;
		}
		onecolrec.type = Jstr(p, q-p);

        if ( JagParser::isGeoType( onecolrec.type ) ) {
            keyHasGeo = true;
        }

		++q; p = q;
		while ( isdigit(*q) ) ++q;
		if ( *q != '!' ) return -110;
		onecolrec.offset = rayatoi(p, q-p);

		// get length
		++q; p = q;
		while ( isdigit(*q) ) ++q;
		if ( *q != '!' ) return -120;
		onecolrec.length = rayatoi(p, q-p);

		// get sig
		++q; p = q;
		while ( isdigit(*q) ) ++q;
		if ( *q != '!' ) return -130;
		onecolrec.sig = rayatoi(p, q-p);	

		// get spare
		++q; p = q; q += JAG_SCHEMA_SPARE_LEN;
		if ( *q != '!' ) return -140;
		memcpy( onecolrec.spare, p, JAG_SCHEMA_SPARE_LEN );

		if ( onecolrec.spare[1] == JAG_C_COL_TYPE_FILE[0] ) {
			keyHasFile = true;
			dn("s2399230 keyHasFile = true");
		}

		if ( onecolrec.spare[5] == JAG_KEY_MUTE ) {
			hasMute = true;
			// d("s2230 col=[%s] hasMute=true\n", onecolrec.name.c_str() );
		}

		if ( onecolrec.spare[7] == JAG_ROLL_UP ) {
			hasRollupColumn = true;
		}
		
		if ( onecolrec.spare[0] == JAG_C_COL_KEY ) {
			actkeylen += onecolrec.length;
			onecolrec.iskey = true;
			++ numKeys;
		} else {
			actvallen += onecolrec.length;
			++ numValues;
		}

		if ( *(q+1) != '|' ) {
			// there are more fields, composite-fields
			// get sig
			++q; p = q;
			while ( isdigit(*q) ) ++q;
			if ( *q != '!' ) return -150;
			onecolrec.srid = rayatoi(p, q-p);	
			//d("s3410 onecolrec.srid=%d\n", onecolrec.srid );

			// get composite-col begincol
			++q; p = q;
			while ( isdigit(*q) ) ++q;
			if ( *q != '!' ) return -160;
			onecolrec.begincol = rayatoi(p, q-p);	

			// get composite-col endcol
			++q; p = q;
			while ( isdigit(*q) ) ++q;
			if ( *q != '!' ) return -170;
			onecolrec.endcol = rayatoi(p, q-p);	

			// get composite-col dummy1
			++q; p = q;
			while ( isdigit(*q) ) ++q;
			if ( *q != '!' ) return -180;
			onecolrec.metrics = rayatoi(p, q-p);	

			// get composite-col dummy2
			++q; p = q;
			while ( *q != '!' && *q != NBT && *q != '\0' ) ++q;
			if ( *q != '!' ) return -190;
			onecolrec.rollupWhere = Jstr(p, q-p);	

			// get composite-col dummy3
			++q; p = q;
			while ( isdigit(*q) ) ++q;
			if ( *q != '!' ) return -200;
			onecolrec.dummy3 = rayatoi(p, q-p);	

			// get composite-col dummy4
			++q; p = q;
			while ( isdigit(*q) ) ++q;
			if ( *q != '!' ) return -210;
			onecolrec.dummy4 = rayatoi(p, q-p);	

			// get composite-col dummy5
			++q; p = q;
			while ( isdigit(*q) ) ++q;
			if ( *q != '!' ) return -220;
			onecolrec.dummy5 = rayatoi(p, q-p);	

			// get composite-col dummy6
			++q; p = q;
			while ( isdigit(*q) ) ++q;
			if ( *q != '!' ) return -230;
			onecolrec.dummy6 = rayatoi(p, q-p);	

			// get composite-col dummy7
			++q; p = q;
			while ( isdigit(*q) ) ++q;
			if ( *q != '!' ) return -240;
			onecolrec.dummy7 = rayatoi(p, q-p);	

			// get composite-col dummy8
			++q; p = q;
			while ( isdigit(*q) ) ++q;
			if ( *q != '!' ) return -250;
			onecolrec.dummy8 = rayatoi(p, q-p);	

			// get composite-col dummy9
			++q; p = q;
			while ( isdigit(*q) ) ++q;
			if ( *q != '!' ) return -260;
			onecolrec.dummy9 = rayatoi(p, q-p);	


			// get composite-col dummy10
			++q; p = q;
			while ( isdigit(*q) ) ++q;
			if ( *q != '!' ) {
				//d("s2284 q=[%s] not ! return 0\n", q );
				return -280;
			}
			onecolrec.dummy10 = rayatoi(p, q-p);	

		}

		columnVector->append( onecolrec );
		rc = 1;
		++q;
		if ( *q != '|' ) {
			// d("s2029 q=[%s] not '|' return 0\n", q );
			return -290;
		}
		++q;
		if ( *q == '}' ) break;
	}

	++q;
	while ( *q == ' ' ) ++q;
	if ( *q != '\0' ) {
		// d("s2013 q=[%s] not null return 0\n", q );
		return -300;
	}
	
	if ( 0 == keyLength && 0 == valueLength ) {
		keyLength = actkeylen;
		valueLength = actvallen;
	}

	// d("s0931 parseRecord hasMute=%d\n", hasMute );
	setLastKeyColumn();

	//print(); 

	return 1;
}


void JagSchemaRecord::setLastKeyColumn()
{
	lastKeyColumn = -1;
	for ( int i=0; i < (*columnVector).length(); ++ i ) {
		//d("s5140 *columnVector i=%d name=[%s] iskey=%d\n", 
			  // i, (*columnVector)[i].name.c_str(), (*columnVector)[i].iskey );
		if ( (*columnVector)[i].iskey ) {
			if ( ! strstr( (*columnVector)[i].name.c_str(), "geo:" ) ) {
				lastKeyColumn = i;
				//d("s4502 lastKeyColumn=%d\n", lastKeyColumn );
			} else {
				break;
			}
		} else {
			break;
		}
	}
	//d("s2036 lastKeyColumn=%d\n", lastKeyColumn );
}

/**
      rc = columnVector[i].spare[1];
        rc2 = columnVector[i].spare[4];
        if ( rc == JAG_C_COL_TYPE_UUID[0]) {
            _schAttr[i].isUUID = true;
        } else if ( rc == JAG_C_COL_TYPE_FILE[0] ) {
            _schAttr[i].isFILE = true;
        }
**/


// Get key mode from first key column: byte 3 in spare field
int JagSchemaRecord::getKeyMode() const
{
	if ( ! columnVector ) return 0;

	int mode = JAG_RAND;
	for ( int i = 0; i < columnVector->size(); ++i ) {
		// keymode of first key column
		if ( (*columnVector)[i].iskey ) {
			mode = (*columnVector)[i].spare[2];
			break;
		}
	}
	return mode;
}

int JagSchemaRecord::getPosition( const AbaxString& colName ) const
{
	int  n = -1;
	bool rc;
	n = _nameMap.getValue( colName.c_str(), rc );
	if ( ! rc ) return -1;
	return n;
}

bool JagSchemaRecord::hasPoly(int &dim ) const
{
	dim = 0;
	JagStrSplit sp(tableProperty, '!' );
	if ( sp.length() < 1 ) return false;
	dim = jagatoi( sp[0].c_str() );  // first field is polydim
	if ( dim < 1 ) return false;
	return true;
}

bool JagSchemaRecord::hasTimeSeries( Jstr &series )  const
{
	return hasTimeSeries( tableProperty, series );
}

bool JagSchemaRecord::hasTimeSeries( const Jstr &tabProperty, Jstr &series ) 
{
	JagStrSplit sp(tabProperty, '!' );
	if ( sp.length() < 2 ) return false;
	series = sp[1];  // second field is timeseries
	if ( series.size() < 1 || series == "0" ) return false;
	return true;
}

bool JagSchemaRecord::setTimeSeries( const Jstr &normSeries ) 
{
	JagStrSplit sp( tableProperty, '!' );
	if ( sp.length() < 2 ) return false;
	Jstr newTabProperty;
	Jstr prop;
	for ( int i=0; i < sp.size(); ++i ) {
		if ( i == 1 ) {
			prop = normSeries;
		} else {
			prop = sp[i];
		}

		if ( newTabProperty.size() < 1 ) {
			newTabProperty = prop;
		} else {
			newTabProperty += Jstr("!") +  prop;
		}
	}
	d("s32334 replace tabProperty=[%s]   by new [%s]\n", tableProperty.s(), newTabProperty.s() );
	tableProperty = newTabProperty;
	return true;
}

bool JagSchemaRecord::setRetention( const Jstr &retention ) 
{
	JagStrSplit sp( tableProperty, '!' );
	if ( sp.length() < 2 ) return false;
	Jstr newTabProperty;
	Jstr prop;
	for ( int i=0; i < sp.size(); ++i ) {
		if ( i == 2 ) {
			if ( retention.firstChar() == '0' ) {
				prop = "0";
			} else {
				prop = retention;
			}
		} else {
			prop = sp[i];
		}

		if ( newTabProperty.size() < 1 ) {
			newTabProperty = prop;
		} else {
			newTabProperty += Jstr("!") +  prop;
		}
	}
	d("s32234 replace tabProperty=[%s]   by new [%s]\n", tableProperty.s(), newTabProperty.s() );
	tableProperty = newTabProperty;
	return true;
}

bool JagSchemaRecord::isFirstColumnDateTime( Jstr &colType ) const 
{
    colType = (*columnVector)[0].type;
	return isDateAndTime( colType );
}

Jstr JagSchemaRecord::timeSeriesRentention() const
{
	Jstr retain;
	JagStrSplit sp(tableProperty, '!' );
	if ( sp.length() < 3 ) return "";
	retain = sp[2];  // second field is timeseries, third is retention
	if ( retain.size() < 1 ) return "0";
	return retain;
}

Jstr JagSchemaRecord::translateTimeSeries( const Jstr &inputTimeSeries )
{
	Jstr out;
	for ( int i=0; i < inputTimeSeries.size(); ++i ) {
		if ( inputTimeSeries[i] == ' ' || inputTimeSeries[i] == '\t' ) continue;
		if ( inputTimeSeries[i] == ':' ) {
			out += '_';
		} else if ( inputTimeSeries[i] == ',' ) {
			out += ':';
		} else {
			out += inputTimeSeries[i];
		}
	}

	Jstr out2;
	JagStrSplit sp( out, ':' );
	Jstr tok;
	Jstr rets;
	Jstr tick;
	char period;
	bool has_;
	for ( int i = 0 ; i < sp.size(); ++i ) {
		tok = sp[i];

		if ( tok.containsStrCase( "second", rets ) ) {
			tok.replace( rets.s(), "s" );
		} else if ( tok.containsStrCase( "minute", rets ) ) {
			tok.replace( rets.s(), "m" );
		} else if ( tok.containsStrCase( "hour", rets ) ) {
			tok.replace( rets.s(), "h" );
		} else if ( tok.containsStrCase( "day", rets ) ) {
			tok.replace( rets.s(), "d" );
		} else if ( tok.containsStrCase( "week", rets ) ) {
			tok.replace( rets.s(), "w" );
		} else if ( tok.containsStrCase( "month", rets ) ) {
			tok.replace( rets.s(), "M" );
		} else if ( tok.containsStrCase( "quarter", rets ) ) {
			tok.replace( rets.s(), "q" );
		} else if ( tok.containsStrCase( "year", rets ) ) {
			tok.replace( rets.s(), "y" );
		} else if ( tok.containsStrCase( "decade", rets ) ) {
			tok.replace( rets.s(), "D" );
		}

		if ( tok.containsChar('_') ) {
			JagStrSplit sp( tok, '_' );
			tick = sp[0]; // 2d
			has_ = true;
		} else {
			tick = tok; // 3M
			has_ = false;
		}
		period = tick.lastChar();

		if ( !has_ ) {
			tok = tok + "_0" + charToStr(period);
		}

		if ( out2.size() < 1 ) {
			out2 = tok;
		} else {
			out2 += Jstr(":") + tok;
		}
	}

	return out2;
}

Jstr JagSchemaRecord::translateTimeSeriesBack( const Jstr &sysTimeSeries )
{
	Jstr out;
	Jstr t;
	bool first = true;
	JagStrSplit sp( sysTimeSeries, ':' );
	char firstc;
	for ( int i =0; i < sp.length(); ++i ) {
		JagStrSplit sp2( sp[i], '_' );
		if ( sp2.length() == 2 ) {
			firstc = sp2[1].firstChar();

			if ( firstc != '0' ) {
				t = sp2[0] + ":" + sp2[1];
			} else {
				t = sp2[0];
			}

			if ( first ) {
				out = t;
				first = false;
			} else {
				out += Jstr(",") + t;
			}
		} else {
			continue;
		}
	}

	return out;
}

Jstr JagSchemaRecord::translateTimeSeriesToStrs( const Jstr &sysTimeSeries )
{
	Jstr out;
	Jstr t;
	bool first = true;
	JagStrSplit sp( sysTimeSeries, ':' );
	for ( int i =0; i < sp.length(); ++i ) {
		JagStrSplit sp2( sp[i], '_' );
		t = sp2[0];
		if ( first ) {
			out = t;
			first = false;
		} else {
			out += Jstr(",") + t;
		}
	}

	return out;
}

int JagSchemaRecord::normalizeTimeSeries( const Jstr &series, Jstr &normalizedSeries ) 
{
	d("s222012 normalizeTimeSeries input series=[%s]\n", series.s() );
	if ( series == "0" ) {
		normalizedSeries = "0";
		return 0;
	}

	normalizedSeries="";

	std::vector<std::string> vecs;
	std::unordered_set<std::string> sets;

	std::vector<std::string> vecm;
	std::unordered_set<std::string> setm;

	std::vector<std::string> vech;
	std::unordered_set<std::string> seth;

	std::vector<std::string> vecd;
	std::unordered_set<std::string> setd;

	std::vector<std::string> vecw;
	std::unordered_set<std::string> setw;

	std::vector<std::string> vecM;
	std::unordered_set<std::string> setM;

	std::vector<std::string> vecq;
	std::unordered_set<std::string> setq;

	std::vector<std::string> vecy;
	std::unordered_set<std::string> sety;

	std::vector<std::string> vecD;
	std::unordered_set<std::string> setD;

	std::string bucket;
	Jstr jbucket;
	char period[2];
	int nums;
	std::string tickret;

	JagStrSplit sp(series, ':', true );
	for ( int i = 0; i < sp.length(); ++i ) {
		JagStrSplit tr( sp[i], '_');
		d("s45016 sp[i]=[%s] len=%d\n", sp[i].s(),  tr.length() );
		if ( tr.length() == 1 ) {
			bucket = tr[0].c_str(); //  2M, 1H, etc
		} else if (  tr.length() == 2 ) {
			bucket = tr[0].c_str(); //  2h, 1M, etc
		} else if ( tr.length() == 0 ) {
			bucket = sp[i].c_str(); //  2h, 1M, etc
		} else {
			d("s52029 skip\n");
			continue;
		}

		tickret = sp[i].s();

		jbucket = bucket.c_str();
		nums = jbucket.toInt(); // 3M --> 3
		period[0] = jbucket.lastChar();  // 3M --> M

		if ( period[0] == 's' ) {
			if ( sets.find(bucket) == sets.end() ) {
				vecs.push_back( tickret );
				sets.emplace( bucket );
				if ( (60 % nums) != 0 ) { return -111; }
			}
		} else if ( period[0] == 'm' ) {
			if ( setm.find(bucket) == setm.end() ) {
				vecm.push_back( tickret );
				setm.emplace( bucket );
				if ( (60 % nums) != 0 ) { return -114; }
			}
		} else if ( period[0] == 'h' ) {
			if ( seth.find(bucket) == seth.end() ) {
				vech.push_back( tickret );
				seth.emplace( bucket );
				if ( (24 % nums) != 0 ) { return -116; }
			}
		} else if ( period[0] == 'd' ) {
			if ( setd.find(bucket) == setd.end() ) {
				vecd.push_back( tickret );
				setd.emplace( bucket );
			}
		} else if ( period[0] == 'w' ) {
			if ( setw.find(bucket) == setw.end() ) {
				vecw.push_back( tickret );
				setw.emplace( bucket );
			}
		} else if ( period[0] == 'M' ) {
			if ( setM.find(bucket) == setM.end() ) {
				vecM.push_back( tickret );
				setM.emplace( bucket );
			}
		} else if ( period[0] == 'q' ) {
			if ( setq.find(bucket) == setq.end() ) {
				vecq.push_back( tickret );
				setq.emplace( bucket );
			}
		} else if ( period[0] == 'y' ) {
			if ( sety.find(bucket) == sety.end() ) {
				vecy.push_back( tickret );
				sety.emplace( bucket );
			}
		} else if ( period[0] == 'D' ) {
			if ( setD.find(bucket) == setD.end() ) {
				vecD.push_back( tickret );
				setD.emplace( bucket );
			}
		} else {
			continue;
		}
	}

	if ( vecs.size() > 0 ) {
		std::sort( vecs.begin(), vecs.end(), sortByTick );
	}
	if ( vecm.size() > 0 ) {
		std::sort( vecm.begin(), vecm.end(), sortByTick );
	}
	if ( vech.size() > 0 ) {
		std::sort( vech.begin(), vech.end(), sortByTick );
	}
	if ( vecd.size() > 0 ) {
		std::sort( vecd.begin(), vecd.end(), sortByTick );
	}
	if ( vecw.size() > 0 ) {
		std::sort( vecw.begin(), vecw.end(), sortByTick );
	}
	if ( vecM.size() > 0 ) {
		std::sort( vecM.begin(), vecM.end(), sortByTick );
	}
	if ( vecq.size() > 0 ) {
		std::sort( vecq.begin(), vecq.end(), sortByTick );
	}
	if ( vecy.size() > 0 ) {
		std::sort( vecy.begin(), vecy.end(), sortByTick );
	}
	if ( vecD.size() > 0 ) {
		std::sort( vecD.begin(), vecD.end(), sortByTick );
	}

	Jstr ts;
	if ( vecs.size() > 0 ) {
		for ( int i = 0; i < vecs.size(); ++i ) {
			ts = vecs[i].c_str();
			if ( ! ts.containsChar('_') ) { ts += Jstr("_0s"); }
    		if ( normalizedSeries.size() < 1 ) {
    			normalizedSeries = ts;
    		} else {
    			normalizedSeries += Jstr(":") + ts;
    		}
		}
	}

	if ( vecm.size() > 0 ) {
		for ( int i = 0; i < vecm.size(); ++i ) {
			ts = vecm[i].c_str();
			if ( ! ts.containsChar('_') ) { ts += Jstr("_0m"); }
    		if ( normalizedSeries.size() < 1 ) {
    			normalizedSeries = ts;
    		} else {
    			normalizedSeries += Jstr(":") + ts;
    		}
		}
	}

	if ( vech.size() > 0 ) {
		for ( int i = 0; i < vech.size(); ++i ) {
			ts = vech[i].c_str();
			if ( ! ts.containsChar('_') ) { ts += Jstr("_0h"); }
    		if ( normalizedSeries.size() < 1 ) {
    			normalizedSeries = ts;
    		} else {
    			normalizedSeries += Jstr(":") + ts;
    		}
		}
	}

	if ( vecd.size() > 0 ) {
		for ( int i = 0; i < vecd.size(); ++i ) {
			ts = vecd[i].c_str();
			if ( ! ts.containsChar('_') ) { ts += Jstr("_0d"); }
    		if ( normalizedSeries.size() < 1 ) {
    			normalizedSeries = ts;
    		} else {
    			normalizedSeries += Jstr(":") + ts;
    		}
		}
	}

	if ( vecw.size() > 0 ) {
		for ( int i = 0; i < vecw.size(); ++i ) {
			ts = vecw[i].c_str();
			if ( ! ts.containsChar('_') ) { ts += Jstr("_0w"); }
    		if ( normalizedSeries.size() < 1 ) {
    			normalizedSeries = ts;
    		} else {
    			normalizedSeries += Jstr(":") + ts;
    		}
		}
	}

	if ( vecM.size() > 0 ) {
		for ( int i = 0; i < vecM.size(); ++i ) {
			ts = vecM[i].c_str();
			if ( ! ts.containsChar('_') ) { ts += Jstr("_0M"); }
    		if ( normalizedSeries.size() < 1 ) {
    			normalizedSeries = ts;
    		} else {
    			normalizedSeries += Jstr(":") + ts;
    		}
		}
	}

	if ( vecq.size() > 0 ) {
		for ( int i = 0; i < vecq.size(); ++i ) {
			ts = vecq[i].c_str();
			if ( ! ts.containsChar('_') ) { ts += Jstr("_0q"); }
    		if ( normalizedSeries.size() < 1 ) {
    			normalizedSeries = ts;
    		} else {
    			normalizedSeries += Jstr(":") + ts;
    		}
		}
	}

	if ( vecy.size() > 0 ) {
		for ( int i = 0; i < vecy.size(); ++i ) {
			ts = vecy[i].c_str();
			if ( ! ts.containsChar('_') ) { ts += Jstr("_0y"); }
    		if ( normalizedSeries.size() < 1 ) {
    			normalizedSeries = ts;
    		} else {
    			normalizedSeries += Jstr(":") + ts;
    		}
		}
	}

	if ( vecD.size() > 0 ) {
		for ( int i = 0; i < vecD.size(); ++i ) {
			ts = vecD[i].c_str();
			if ( ! ts.containsChar('_') ) { ts += Jstr("_0D"); }
    		if ( normalizedSeries.size() < 1 ) {
    			normalizedSeries = ts;
    		} else {
    			normalizedSeries += Jstr(":") + ts;
    		}
		}
	}

	d("s44013 normalizedSeries=[%s]\n", normalizedSeries.s() );
	return 0;
}

#if 0
void JagSchemaRecord::getJoinSchema( long skeylen, long svallen, const JagParseParam &parseParam, const jagint lengths[], Jstr &hstr )
{
        char hbuf[JAG_SCHEMA_SPARE_LEN+1];
        jagint offset = 0;
        memset(hbuf, ' ', JAG_SCHEMA_SPARE_LEN );
        hbuf[JAG_SCHEMA_SPARE_LEN] = '\0';
        hbuf[0] = JAG_C_COL_KEY;
        hbuf[2] = JAG_RAND;
        hstr = Jstr("NN|") + longToStr(skeylen) + "|" + longToStr(svallen) + "|0!0!0!0|{";
        for ( int i = 0; i < parseParam.orderVec.size(); ++i ) {
            hstr += Jstr("!") + "key_" + parseParam.orderVec[i].name + "!s!" + longToStr(offset) + "!" +
                    longToStr(lengths[i]) + "!0!" + hbuf + "|";
            offset += lengths[i];
        }

        hbuf[1] = JAG_C_COL_TYPE_UUID[0];
        hstr += Jstr("!") + "key_" + longToStr( THID ) + "_uuid" + "!s!" + longToStr(offset) + "!" +
                longToStr(JAG_UUID_FIELD_LEN) + "!0!" + hbuf + "!0! ! !|";
        offset += JAG_UUID_FIELD_LEN;

        memset( hbuf, ' ', JAG_SCHEMA_SPARE_LEN );
        hbuf[0] = JAG_C_COL_VALUE;
        hbuf[2] = JAG_RAND;
        for ( int i = 0; i < columnVector->size(); ++i ) {
            hstr += Jstr("!") + (*(columnVector))[i].name.c_str() + "!s!" + longToStr(offset) + "!" +
                    longToStr((*(columnVector))[i].length) + "!0!" + hbuf + "!0! ! !|";
            offset += (*(columnVector))[i].length;
        }
        hstr += "}";
}
#endif

time_t JagSchemaRecord::getRetentionSeconds( const Jstr &retention )
{
	jagint nums = retention.toInt();   
	char period = retention.lastChar();
	time_t  secs;

	if ( period == 's' ) {
		secs = nums;
	} else if ( period == 'm' ) {
		secs = nums * 60;
	} else if ( period == 'h' ) {
		secs = nums * 3600;
	} else if ( period == 'd' ) {
		secs = nums * 86400;
	} else if ( period == 'w' ) {
		secs = nums * 86400 * 7;
	} else if ( period == 'M' ) {
		secs = nums * 86400 * 31;
	} else if ( period == 'q' ) {
		secs = nums * 86400 * 31 * 3;
	} else if ( period == 'y' ) {
		secs = nums * 86400 * 365;
	} else if ( period == 'D' ) {
		secs = nums * 86400 * 365 * 10;
	} else {
		secs = -1;
	}

	return secs;
}

int  JagSchemaRecord::getFirstDateTimeKeyCol() const
{
	int idx = -1;
	for ( int i=0; i < (*columnVector).length(); ++ i ) {
		if ( (*columnVector)[i].iskey ) {
			if ( isDateAndTime( (*columnVector)[i].type ) ) {
				idx = i;
				break;
			}
		} else {
			break;
		}
	}

	return idx;
}


bool JagSchemaRecord::validRetention( char u )
{
    if ( u != 's' && u != 'm' && u != 'h' && u != 'd'
       && u != 'w' && u != 'M' && u != 'q' && u != 'y' && u != 'D' ) {
       return false;
    }

	return true;
}

Jstr JagSchemaRecord::makeTickPair( const Jstr &tok )
{
	if ( tok.containsChar('_') ) {
		return tok;
	} 

	Jstr out;
	char period = tok.lastChar();
	out = tok + "_0" + charToStr(period);
	return out;
}


