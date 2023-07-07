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

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <stack>
#include <vector>
#include <exception>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <math.h>
#include <algorithm>
#include <regex>

#include <JagParseExpr.h>
#include <JagVector.h>
#include <JagTableUtil.h>
#include <JagParseParam.h>
#include <JagStrSplit.h>
#include <JagSession.h>
#include <JagRequest.h>
#include <utfcpp/utf8.h>
#include <JagLang.h>
#include <JagParser.h>
#include <JagHashStrStr.h>
#include <JagRange.h>
#include <JagTime.h>
#include <JagUtil.h>
#include <JagMath.h>
#include <JagTableOrIndexAttrs.h>

#ifdef JAG_SERVER_SIDE
#include <JagGeom.h>
#include <JagCGAL.h>
#endif

ExprElementNode::ExprElementNode()
{
	_isDestroyed = false;
}

ExprElementNode::~ExprElementNode() 
{ 
	_isDestroyed = false;
}
 

StringElementNode::StringElementNode() 
{
	_isElement = true;
	_nargs = 0;
}

StringElementNode::StringElementNode( BinaryExpressionBuilder *builder, const Jstr &name, 
									 const JagFixString &value, 
                       				  const JagParseAttribute &jpa, int tabnum, int typeMode )
{
    dn("s0911233 name=[%s]", name.s() );
    int ndot = strchrnum(name.s(), '.');
    if ( ndot >= 3 ) {
        // db.tab.index.v2
        JagStrSplit sp(name, '.');
        _name = sp[0] + "." + sp[2] + "." + sp[3];
    } else {
        _name = name;
    }

    dn("s0911233 _name=[%s]", _name.s() );

	_value = value; _jpa = jpa; _tabnum = tabnum; _typeMode = typeMode;
    _srid = _offset = _length = _sig = _nodenum = _begincol = _endcol = _metrics = 0;
    _type = ""; 
	_builder = builder;
	_isElement = true;
	_nargs = 0;

	if ( _name.size() > 0 ) {
		_builder->_pparam->initColHash();
		if ( strchr(_name.c_str(), '.' ) ) {
			JagStrSplit sp( _name, '.' );
			int n = sp.length();
			_builder->_pparam->_colHash->addKeyValue( sp[n-1], "1" );
		} else {
			_builder->_pparam->_colHash->addKeyValue( _name, "1" );
		}
	}
}

StringElementNode::~StringElementNode() 
{
	if ( _isDestroyed ) return;
	clear();
	_isDestroyed = true;
}

void StringElementNode::clear()
{
}

void StringElementNode::print( int mode ) {
	i("name=\"%s\" value=\"%s\" ", _name.c_str(), _value.c_str() );
}

int StringElementNode::getClusterByUuid(const JagTableOrIndexAttrs *objAttr )
{
	//d("name=\"%s\" value=\"%s\" ", _name.c_str(), _value.c_str());
	return -1;
}


int StringElementNode::setWhereRange( const JagHashStrInt *maps[], const JagSchemaAttribute *attrs[], 
						const int keylen[], const int numKeys[], int numTabs, bool &hasValue, 
						JagMinMax *minmax, JagFixString &str, int &typeMode, int &tabnum ) 
{
    dn("s540213 StringElementNode::setWhereRange() _name=[%s] ...", _name.s() );
    // test.unittest1.unittest1_idx1.v2

    int ndot = strchrnum(_name.s(), '.');
    if ( ndot >= 3 ) {
        // db.tab.index.v2
        JagStrSplit sp(_name, '.');
        _name = sp[0] + "." + sp[2] + "." + sp[3];
    } else {
        //_name = name;
    }
    dn("s540213 StringElementNode::setWhereRange() _name=[%s] ...", _name.s() );

	tabnum = -1;
	str = "";
	int acqpos;

	if ( _name.length() > 0 ) {
		typeMode = 0;

		if ( maps[_tabnum]->getValue(_name, acqpos) ) {
            dn("s560288 kjkjk11");
			tabnum = _tabnum;
			minmax[_tabnum].offset = _offset = attrs[_tabnum][acqpos].offset;
			minmax[_tabnum].length = _length = attrs[_tabnum][acqpos].length;
			minmax[_tabnum].sig = _sig = attrs[_tabnum][acqpos].sig;
			minmax[_tabnum].type = _type = attrs[_tabnum][acqpos].type;

			minmax[_tabnum].dbcol = attrs[_tabnum][acqpos].dbcol;
			minmax[_tabnum].objcol = attrs[_tabnum][acqpos].objcol;
			minmax[_tabnum].colname = attrs[_tabnum][acqpos].colname;

			_begincol = attrs[_tabnum][acqpos].begincol;
			_endcol = attrs[_tabnum][acqpos].endcol;
			_srid = attrs[_tabnum][acqpos].srid;
			_metrics = attrs[_tabnum][acqpos].metrics;
			if ( !attrs[_tabnum][acqpos].isKey ) hasValue = 1;
		} else {
            dn("s80217 return -1");

            // print maps
            /**
            dn("s222309 maps[_tabnum]->print():");
            maps[_tabnum]->print();
            **/
            // for index:
            // key=test.unittest1_idx1.uid  --> value=1
            // key=test.unittest1_idx1.v2  --> value=0

			return -1;
		}
	} else {
        dn("s980156 no _name str=[%s]", _value.s() );
		str = _value;
		typeMode = _typeMode;
	}
	
	return 1;
}

int StringElementNode
::setFuncAttribute( const JagHashStrInt *maps[], const JagSchemaAttribute *attrs[], 
			        int &constMode, int &typeMode, bool &isAggregate, Jstr &type, int &collen, int &siglen )
{	
    dn("s4330041 setFuncAttribute _name=[%s]", _name.s() );
	int acqpos;

	if ( _name.length() > 0 ) {
		if ( maps[_tabnum]->getValue(_name, acqpos) ) {
			_offset = attrs[_tabnum][acqpos].offset;
			collen = _length = attrs[_tabnum][acqpos].length;
			siglen = _sig = attrs[_tabnum][acqpos].sig;
			type = _type = attrs[_tabnum][acqpos].type;
			_begincol = attrs[_tabnum][acqpos].begincol;
			_endcol = attrs[_tabnum][acqpos].endcol;
			_srid = attrs[_tabnum][acqpos].srid;
			_metrics = attrs[_tabnum][acqpos].metrics;

            dn("s8740012 collen=%d siglen=%d type=%s", collen, siglen, type.s() );
		} else {
			return 0;
		}

		if ( isInteger(_type) ) typeMode = 1;
		else if ( _type == JAG_C_COL_TYPE_FLOAT || _type == JAG_C_COL_TYPE_DOUBLE ) typeMode = 2;
		else if ( _type == JAG_C_COL_TYPE_LONGDOUBLE ) typeMode = 2;
		else typeMode = 0;
		constMode = 1;
	} else {
        dn("s8740015 _name is empty, us string");
		constMode = 0;
		typeMode = _typeMode;
		if ( 0 == typeMode ) {
			collen = _value.length();
			siglen = 0;
			type = _type = JAG_C_COL_TYPE_STR;
            dn("s45200003 _value=[%s] collen=%d siglen=0 type=JAG_C_COL_TYPE_STR", _value.s(), collen );
		} else {
			typeMode = 2;
			type = _type = JAG_C_COL_TYPE_LONGDOUBLE;
            /**
			collen = JAG_MAX_INT_LEN;
			siglen = JAG_MAX_SIG_LEN;
            **/
			collen = JAG_LONGDOUBLE_FIELD_LEN;
			siglen = JAG_LONGDOUBLE_SIG_LEN;
            dn("s3939310 type JAG_C_COL_TYPE_LONGDOUBLE collen JAG_MAX_INT_LEN siglen JAG_MAX_SIG_LEN");
		}
	}
	
	return 1;
}

int StringElementNode
::getFuncAggregate( JagVector<Jstr> &selectParts, JagVector<int> &selectPartsOpcode, 
				    JagVector<int> &selColSetAggParts, JagHashMap<AbaxInt, AbaxInt> &selColToselParts, 
					int &nodenum, int treenum )
{
	_nodenum = nodenum++;
	return 1;
}

int StringElementNode::getAggregateParts( Jstr &parts, int &nodenum )
{
	if ( _name.length() ) parts = _name;
	else parts = _value.c_str();

	_nodenum = nodenum++;
	return 1;
}

int StringElementNode::setAggregateValue( int nodenum, const char *buf, int length, const Jstr &type ) 
{ 
	return 1;
}

int StringElementNode::checkFuncValid(JagMergeReaderBase *ntr, const JagHashStrInt *maps[], const JagSchemaAttribute *attrs[],
					const char *buffers[], JagFixString &str, int &typeMode, 
					Jstr &type, int &length, bool &first, bool useZero, bool setGlobal ) 
{
    dn("s007662 StringElementNode::checkFuncValid _name=[%s] type=[%s] _type=[%s]", _name.s(), type.s(), _type.s() );

	if ( _name.length() > 0 ) {
		if (  ( buffers[_tabnum] == NULL || buffers[_tabnum][0] == '\0' ) ) {
			typeMode = 0;
			type = _type;
			length = _length;
			return 2;
		}

		if ( JagParser::isPolyType( _type ) ) {
            dn("s0902l293 isPolyType _type=%s", _type.s() );
			getPolyDataString( ntr, _type, maps, attrs, buffers, str );
            dn("s0902l293 isPolyType _type=%s str=[%s]", _type.s(), str.s() );
		} else if ( JagParser::isGeoType( _type ) ) {
            dn("s09828293 isGeoType _type=%s", _type.s() );
			int dim = getDimension(_type);
		    Jstr colobjstr = Jstr("OJAG=") + intToStr( _srid ) + "=" + _name + "=" + _type;
			if ( 2 == dim ) {
				colobjstr += "=d 0:0:0:0";
			} else {
				colobjstr += "=d 0:0:0:0:0:0";
			}
			makeDataString( attrs, buffers, colobjstr, str );
		} else if (  _type == JAG_C_COL_TYPE_RANGE  ) {
            dn("s2039339 JAG_C_COL_TYPE_RANGE _type=[%s]", _type.s() );
		    Jstr colobjstr = Jstr("OJAG=") + intToStr( _srid ) + "=" + _name + "=" + _type;
			makeRangeDataString( attrs, buffers, colobjstr, str );
        } else if ( _type == JAG_C_COL_TYPE_DINT 
                   || _type == JAG_C_COL_TYPE_DBIGINT
                   || _type == JAG_C_COL_TYPE_DSMALLINT
                   || _type == JAG_C_COL_TYPE_DTINYINT
                   || _type == JAG_C_COL_TYPE_DMEDINT ) {
            dn("s3330281 not isPolyType  not JAG_C_COL_TYPE_RANGE _type.s=[%s] !!!!!!!! ", _type.s() );
            dn("s5002299 dumpmem encoded str:");
            //dumpmem( buffers[_tabnum]+_offset, _length );

            if ( '*' == *(buffers[_tabnum]+_offset) ) {
                str = "*";
            } else {
                Jstr norm;
                JagMath::fromBase254Len( norm, buffers[_tabnum]+_offset, _length );
                str = JagFixString( norm.s(), norm.size(), norm.size() );
            }
            dn("s220057 JagFixString decoded str=[%s]", str.s() );
        } else if ( _type == JAG_C_COL_TYPE_FLOAT 
                   || _type == JAG_C_COL_TYPE_DOUBLE
                   || _type == JAG_C_COL_TYPE_LONGDOUBLE ) {

            dn("s5002250 dumpmem encoded str: _length=%d", _length);
            /**
            dn("s81110982 buffer=[%s]", buffers[_tabnum]+_offset );
            dumpmem( buffers[_tabnum]+_offset, _length );
            **/

            if ( '*' == *(buffers[_tabnum]+_offset) ) {
                str = "*";
            } else {
                Jstr norm;
                JagMath::fromBase254Len( norm, buffers[_tabnum]+_offset, _length );
                dn("s290012865 fromBase254Len() norm=[%s]  _offset=%d _length=%d", norm.s(), _offset, _length );
                str = JagFixString( norm.s(), norm.size(), norm.size() );
            }
            dn("s227047 float/double/longdouble JagFixString decoded str=[%s]", str.s() );
        } else if ( _type == JAG_C_COL_TYPE_TIMESTAMPMICRO || _type == JAG_C_COL_TYPE_DATETIMEMICRO
                    || _type == JAG_C_COL_TYPE_TIMESTAMPNANO || _type == JAG_C_COL_TYPE_DATETIMENANO 
                    || _type == JAG_C_COL_TYPE_TIMESTAMPSEC  || _type == JAG_C_COL_TYPE_DATETIMESEC 
                    || _type == JAG_C_COL_TYPE_TIMESTAMPMILLI  || _type == JAG_C_COL_TYPE_DATETIMEMILLI
                    || _type == JAG_C_COL_TYPE_TIMEMICRO  || _type == JAG_C_COL_TYPE_TIMENANO 
                    || _type == JAG_C_COL_TYPE_DATE ) {
            dn("s233039 buffers has no-");
            Jstr norm;
            JagMath::fromBase254Len( norm, buffers[_tabnum]+_offset, _length );
            str = JagFixString( norm.s(), norm.size(), norm.size() );
            dn("s92928 JAG_C_COL_TYPE_TIMESTAMP norm=[%s] str=[%s] _type=[%s]", norm.s(), str.s(), _type.s() );
		} else {
			str = JagFixString(buffers[_tabnum]+_offset, _length, _length);
            dn("s3330281 not isPolyType  not JAG_C_COL_TYPE_RANGE _type.s=[%s] !!!!!!!! ", _type.s() );
            dn("s220055 JagFixString str=[%s]", str.s() );
		}

		if ( isInteger(_type) ) typeMode = 1;
		else if ( _type == JAG_C_COL_TYPE_FLOAT || _type == JAG_C_COL_TYPE_DOUBLE ) typeMode = 2;
		else if ( _type == JAG_C_COL_TYPE_LONGDOUBLE ) typeMode = 2;
		else if ( JagParser::isGeoType( _type ) ) typeMode = 2;
		else typeMode = 0;

		str.setDtype( _type.s() );

		type = _type;
		length = _length;
        dn("s29013 type=[%s] length=%d", type.s(), length );
	} else {
		str = _value;
		typeMode = _typeMode;
		type = "";
		length = 0;
	}

	return 1;
}

int StringElementNode::checkFuncValidConstantCalc( JagFixString &str, int &typeMode, Jstr &type, int &length )
{
    return checkFuncValidConstantOnly( str, typeMode, type, length );
}

int StringElementNode::checkFuncValidConstantOnly( JagFixString &str, int &typeMode, Jstr &type, int &length )
{
	if ( _value.length() > 0 ) {
		str = _value;
		typeMode = _typeMode;
		type = "";
		length = 0;
	}

	return 1;
}

void StringElementNode::makeDataString( const JagSchemaAttribute *attrs[], const char *buffers[],
										const Jstr &colobjstr, JagFixString &str )
{
	int  ncols = _endcol - _begincol +1;
	Jstr bufstr = colobjstr;
	int begin;

	begin = _begincol;
	addDataStrting( buffers, attrs, begin, ncols - _metrics, bufstr );

	begin += ncols - _metrics;
	addMetricString( buffers, attrs, begin, bufstr  );

	str = JagFixString( bufstr.c_str(), bufstr.size(), bufstr.size() );
}

void StringElementNode::addDataStrting( const char *buffers[], const JagSchemaAttribute *attrs[], int begincol, int ncols, Jstr &bufstr )
{
	if ( begincol < 0 ) return;
	if ( ncols <= 0 ) return;

	int offset, len;
   	Jstr t;
   	for ( int i = 0; i < ncols; ++i ) {
   		offset = attrs[_tabnum][begincol+i].offset;
   		len = attrs[_tabnum][begincol+i].length;

        if ( '*' == *(buffers[_tabnum]+offset) )  {
            dn("s8722220 see star *");
            t = "*";
        } else {
            JagMath::fromBase254Len( t, buffers[_tabnum]+offset, len );
        }

		//t.trimNull();
		t.trim0();
   		bufstr += Jstr(" ") + t; 
   	}
}

void StringElementNode::addMetricString(  const char *buffers[], const JagSchemaAttribute *attrs[], int begin, Jstr &bufstr  )
{
	if ( begin < 0 ) return;

   	Jstr t;
	int offset, len;
   	for ( int m=0; m < _metrics; ++m ) {
   		offset = attrs[_tabnum][begin+m].offset;
   		len = attrs[_tabnum][begin+m].length;
   		t = Jstr(buffers[_tabnum]+offset, len);
   		t.trimNull();
   		if ( t.size() > 0 ) {
   			bufstr += Jstr(" ") + t;  
   		} else {
   			bufstr += Jstr(" ") + JAG_DEFAULT_METRIC;
   		}
   	}
}

void StringElementNode::makeRangeDataString( const JagSchemaAttribute *attrs[], const char *buffers[],
										const Jstr &incolobjstr, JagFixString &str )
{
    dn("s032820 makeRangeDataString ");
	int  ncols = _endcol - _begincol +1;
	Jstr subtype;
	subtype = attrs[_tabnum][_begincol+0].type;
	Jstr bufstr = incolobjstr + "=" + subtype + " 0:0:0:0";
	jagint len; 
    Jstr  norm;

    dn("s30303990 subtype=[%s]", subtype.s() );

	for ( int i = 0; i < ncols; ++i ) {
		len = attrs[_tabnum][_begincol+i].length;

        JagMath::fromBase254Len(norm, buffers[_tabnum]+attrs[_tabnum][_begincol+i].offset, len );

		bufstr += Jstr(" ") + norm.trim0();
	}

	str = JagFixString( bufstr.c_str(), bufstr.size(), bufstr.size() );
}

void StringElementNode::getPolyDataString( JagMergeReaderBase *ntr, const Jstr &polyType, 
											const JagHashStrInt *maps[],
											const JagSchemaAttribute *attrs[], const char *buffers[], 
											JagFixString &str )
{
	if ( polyType == JAG_C_COL_TYPE_POLYGON 
		|| polyType == JAG_C_COL_TYPE_VECTOR 
		|| polyType == JAG_C_COL_TYPE_LINESTRING 
		|| polyType == JAG_C_COL_TYPE_MULTIPOINT
		|| polyType == JAG_C_COL_TYPE_MULTIPOLYGON
	    || polyType == JAG_C_COL_TYPE_MULTILINESTRING ) {
		getPolyData( polyType, ntr, maps, attrs, buffers, str, false );
	} else if ( polyType == JAG_C_COL_TYPE_POLYGON3D 
				|| polyType == JAG_C_COL_TYPE_LINESTRING3D
				|| polyType == JAG_C_COL_TYPE_MULTIPOINT3D
				|| polyType == JAG_C_COL_TYPE_MULTIPOLYGON3D
		        || polyType == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
		getPolyData( polyType, ntr, maps, attrs, buffers, str, true );
	}
}

void StringElementNode::getPolyData( const Jstr &polyType, JagMergeReaderBase *ntr, 
									const JagHashStrInt *maps[], const JagSchemaAttribute *attrs[], 
									const char *buffers[], JagFixString &str, bool is3D )
{
	Jstr polyColumns;
	int acqpos;
	JagStrSplit dbc( _name, '.' );
	Jstr db = dbc[0];
	Jstr tab = dbc[1];

	if ( _builder->_pparam->_allColumns.size() < 1 ) {
		if ( _builder->_pparam->_colHash ) {
			_builder->_pparam->_allColumns = _builder->_pparam->_colHash->getKeyStrings();
		}
	}
	JagStrSplit sp( _builder->_pparam->_allColumns, '|' );

    dn("s459003 getPolyData _name=[%s] _allColumns=[%s]", _name.s(), _builder->_pparam->_allColumns.s() );

	Jstr colType;
	Jstr fullname;
	JagHashStrInt hash;

	for ( int i=0; i<sp.length(); ++i ) {
		fullname = db + "." + tab + "." + sp[i];
		if ( maps[_tabnum]->getValue( fullname, acqpos) ) {
			colType = attrs[_tabnum][acqpos].type;
			if ( JagParser::isPolyType( colType ) && ! hash.keyExist( sp[i] )  ) {
				if ( polyColumns.size() < 1 ) {
					polyColumns = sp[i];
				} else {
					polyColumns += Jstr("|") + sp[i];
				}
				hash.addKeyValue( sp[i], 1 );
			}
		} else {
		}
	}

	bool isBoundBox3D;
	fullname = db + "." + tab + ".geo:zmin";
	if ( maps[_tabnum]->getValue( fullname, acqpos) ) {
	    isBoundBox3D = true;  
    } else {
	    isBoundBox3D = false;  
    }

	int ncols = attrs[_tabnum][0].record.columnVector->size();
	int offsetuuid=0;
	int collenuuid=0;
	int uuidcol=0;

    dn("s340082 ncols=%d polyColumns=[%s]", ncols, polyColumns.s() );

	for ( int i=0; i < ncols; ++i ) {
		if ( 0==strcmp(attrs[_tabnum][i].colname.c_str(), "geo:id") ) {
			uuidcol = i;
			offsetuuid = attrs[_tabnum][i].offset;
			collenuuid = attrs[_tabnum][i].length;
			break;
		} 
	}

	Jstr uuid(buffers[_tabnum]+offsetuuid, collenuuid );
	bool rc;

	if ( NULL == _builder->_pparam->_rowHash ) {
		_builder->_pparam->_rowHash = newObject<JagHashStrStr>();
		savePolyData( polyType, ntr, maps, attrs, buffers, uuid, db, tab, polyColumns, isBoundBox3D, is3D );
	} else if ( _builder->_pparam->_rowUUID.size() > 0 && uuid != _builder->_pparam->_rowUUID ) {
        dn("s2330084 different uuid delete _rowHash, create new _rowHash");
		delete _builder->_pparam->_rowHash;
		_builder->_pparam->_rowHash = newObject<JagHashStrStr>();
		savePolyData( polyType, ntr, maps, attrs, buffers, uuid, db, tab, polyColumns, isBoundBox3D, is3D );
	} 
	
	if ( _builder->_pparam->_rowHash ) {
		Jstr val = _builder->_pparam->_rowHash->getValue( _name, rc );
		if ( rc ) {
			str  = JagFixString(val.c_str(), val.size(), val.size() );
            dn("s203936013 _name=[%s] got str=[%s]", _name.s(), str.s() );
		} else {
		}
		return;
	} else {
		return; 
	}
}

void StringElementNode::savePolyData( const Jstr &polyType, JagMergeReaderBase *ntr, const JagHashStrInt *maps[], 
									const JagSchemaAttribute *attrs[], 
									const char *buffers[],  const Jstr &uuid,
									const Jstr &db, const Jstr &tab, const Jstr &polyColumns,
									bool isBoundBox3D, bool is3D )
{
    dn("s233309 savePolyData polyColumns=[%s]", polyColumns.s() );

    int  dim = 2;
    if ( polyType == JAG_C_COL_TYPE_VECTOR ) {
        dim = 1;
    } else if ( is3D ) {
        dim = 3;
    }


	JagStrSplit psp(polyColumns, '|', true);

	int numCols = psp.length();

	int offsetx[numCols];
	int collenx[numCols];

	int offsety[numCols];
	int colleny[numCols];

	int offsetz[numCols];
	int collenz[numCols];
	JagVector<int> metricoffset[numCols];
	JagVector<int> metriclen[numCols];

	Jstr colobjstr[numCols];
	Jstr fullname[numCols];
	Jstr str[numCols];
	Jstr nm;
	int acqpos, metrics;
	bool rc;
	Jstr mstr;

	for ( int k=0; k < numCols; ++k ) {
		fullname[k] = db + "." + tab + "." + psp[k];
		if ( ! maps[_tabnum]->getValue( fullname[k], acqpos) ) { 
            dn("s33309 k=%d fullname=%s maps cannot find", k, fullname[k].s() );
			return; 
		}

		colobjstr[k] = Jstr( JAG_OJAG ) + "=" + intToStr( attrs[_tabnum][acqpos].srid ) 
			               + "=" + fullname[k] + "=" + attrs[_tabnum][acqpos].type + "=d";
		metrics = attrs[_tabnum][acqpos].metrics;

        dn("s333023776 k=%d fullname[k]=[%s] acqpos=%d  colobjstr[k]=[%s]", k, fullname[k].s(), acqpos, colobjstr[k].s() );

		nm = fullname[k] + ":x";
		if ( ! maps[_tabnum]->getValue( nm, acqpos) ) { 
            dn("s222092 errorrrrrrrrrrrrrrrrr return");
            return; 
        }

        dn("s30998399 nm=[%s]  acqpos=%d", nm.s(), acqpos );

		offsetx[k] =  attrs[_tabnum][acqpos].offset;
		collenx[k] =  attrs[_tabnum][acqpos].length;
        dn("s22233804 k=%d offsetx[k]=%d collenx[k]=%d", k, offsetx[k], collenx[k] );

        if ( dim >= 2 ) {
		    offsety[k] =  attrs[_tabnum][acqpos+1].offset;  // y
		    colleny[k] =  attrs[_tabnum][acqpos+1].length;  // y
            dn("s22233804 k=%d offsety[k]=%d colleny[k]=%d", k, offsety[k], colleny[k] );
        }

		if ( dim >= 3 ) {
			offsetz[k] =  attrs[_tabnum][acqpos+2].offset; // z
			collenz[k] =  attrs[_tabnum][acqpos+2].length; // z
		}


		// m1.offset  m2.offset  m3.offset
		// col1 may have 2 metrics, col2 may have 5 metrics
		for ( int m=0; m < metrics; ++m ) {
			nm = fullname[k] + ":m" + intToStr(m+1);
			if ( ! maps[_tabnum]->getValue( nm, acqpos) ) { return; }
			metricoffset[k].append( attrs[_tabnum][acqpos].offset );
			metriclen[k].append( attrs[_tabnum][acqpos].length );
		}
	}

    dn("s0292423 acqpos=%d", acqpos );

    #ifdef JAG_KEEP_MIN_MAX
	int BBUUIDLEN = 7;
    int  uuidCol;
	JagColumnBox bbox[BBUUIDLEN];  // bbox and uuid
	nm = db + "." + tab + ".geo:xmin";
	if ( ! maps[_tabnum]->getValue( nm, acqpos) ) { 
        dn("s0292820 notfound nm=[%s] return", nm.s() );
        return; 
    }

    dn("s0292823 nm=[%s] acqpos=%d", nm.s(), acqpos );

	if ( is3D || isBoundBox3D ) {
	    for ( int bi = 0; bi < BBUUIDLEN; ++bi ) {
		    bbox[bi].col = acqpos;
		    bbox[bi].offset = attrs[_tabnum][acqpos].offset;
		    bbox[bi].len = attrs[_tabnum][acqpos].length;
		    ++acqpos;
	    }
        uuidCol = BBUUIDLEN-1;
    } else {
        for ( int bi = 0; bi < BBUUIDLEN-2; ++bi ) {
	        bbox[bi].col = acqpos;
	        bbox[bi].offset = attrs[_tabnum][acqpos].offset;
	        bbox[bi].len = attrs[_tabnum][acqpos].length;
	        ++acqpos;
               dn("s344001 bbox i=%d col=%d offset=%d len=%d", bi, bbox[bi].col, bbox[bi].offset, bbox[bi].len );
        }
        uuidCol = BBUUIDLEN-3;
    }
    #else
    // if not JAG_KEEP_MIN_MAX
    int  uuidCol = 0;
	JagColumnBox bbox[1];  // bbox and uuid
	nm = db + "." + tab + ".geo:id";
	if ( ! maps[_tabnum]->getValue( nm, acqpos) ) { 
        dn("s0292820 notfound nm=[%s] return", nm.s() );
        return; 
    }

    bbox[uuidCol].col = acqpos;
    bbox[uuidCol].offset = attrs[_tabnum][acqpos].offset;
    bbox[uuidCol].len = attrs[_tabnum][acqpos].length;

    dn("s0292823 nm=[%s] acqpos=%d", nm.s(), acqpos );
    #endif

	
    Jstr v1, v2, v3;
	int offset, collen;
	bool hasValue = 0;

    dn("s3455088 numCols=%d", numCols );

	for ( int k = 0; k < numCols; ++k ) {
		metrics = metricoffset[k].size();
		str[k] = colobjstr[k] + " ";

        dn("s3003819 k=%d str[k] init to [%s]", k, str[k].s() );

        #ifdef JAG_KEEP_MIN_MAX
		if ( is3D || isBoundBox3D ) {
    		for ( int j=0; j < BBUUIDLEN-1; ++j ) {
        		//v1 = Jstr(buffers[_tabnum]+ bbox[j].offset, bbox[j].len ).trim0();
                JagMath::fromBase254Len(v1, buffers[_tabnum]+ bbox[j].offset, bbox[j].len );
                v1.trim0();
                dn("s92938 is3D v1=%s", v1.s() );

				if ( 0 == j ) {
					str[k] += v1;
				} else {
					str[k] += Jstr(":") + v1;
				}
    		}
		} else {
    		for ( int j=0; j < BBUUIDLEN-3; ++j ) {
                /***
                dn("s93303 bbox j=%d dumpmem: offset=%d  len=%d", j,  bbox[j].offset, bbox[j].len );
                dumpmem( buffers[_tabnum]+ bbox[j].offset, bbox[j].len );
                **/

                JagMath::fromBase254Len(v2, buffers[_tabnum]+ bbox[j].offset, bbox[j].len );
                dn("s929380344 bbox is2D v=[%s]", v2.s() );
                v2.trim0();
                dn("s929380345 bbox is2D v=[%s]", v2.s() );
				if ( 0 == j ) {
					str[k] += v2;
                    dn("s3334408 bbox k=%d j=%d  str[k]=[%s]", k, j, str[k].s() );
				} else {
					str[k] += Jstr(":") + v2;
                    dn("s3334409 bbox k=%d  j=%d str[k]=[%s]", k, j, str[k].s() );
				}
            }
		} 

        dn("s3834409 bbox str[%d]=[%s]", k, str[k].s() );
        #else
            if ( is3D ) {
			    str[k] += Jstr("0:0:0:0:0:0");
            } else {
			    str[k] += Jstr("0:0:0:0");
            }
        #endif

        if ( dim >= 2 ) {
            if ( *(buffers[_tabnum]+offsetx[k]) == '\0' &&
                *(buffers[_tabnum]+offsety[k]) == '\0' ) {
                dn("s222203 k=%d x y fields null offsetx=%d offety=%d", k, offsetx[k], offsety[k] );
                continue;
            }
        } else if ( dim == 1 ) {
            if ( *(buffers[_tabnum]+offsetx[k]) == '\0' ) {
                dn("s228203 k=%d x fields null offsetx=%d", k, offsetx[k] );
                continue;
            }
        }

        Jstr  xval, yval;
        JagMath::fromBase254Len(xval, buffers[_tabnum]+offsetx[k], collenx[k] );
        xval.trim0();

        if ( dim >= 2 ) {
            JagMath::fromBase254Len(yval, buffers[_tabnum]+offsety[k], colleny[k] );
            yval.trim0();
        }


        /***
        dn("s3833008 k=%d offsetx[k]=%d  collenx[k]=%d  dumpmem:", k, offsetx[k], collenx[k] );
        dumpmem( buffers[_tabnum]+offsetx[k], collenx[k] );

        dn("s3833008 k=%d offsety[k]=%d  colleny[k]=%d  dumpmem:", k, offsety[k], colleny[k] );
        dumpmem( buffers[_tabnum]+offsety[k], colleny[k] );
        ***/

         dn("s339501 dim=%d", dim ); 

        if ( dim >= 2 ) {
            dn("s339001 col-k=%d xval=[%s]  yval=[%s]", k, xval.s(), yval.s() );
    	    if ( xval.isNotNull() && yval.isNotNull() ) {
			    hasValue = 1;
			    str[k] += Jstr(" ") + xval + ":" + yval;
                dn("s09444005 hasValue=1 k=%d str[k]=[%s]", k, str[k].s() );
    	    }
        } else if ( dim == 1 ) {
    	    if ( xval.isNotNull() ) {
			    hasValue = 1;
			    str[k] += Jstr(" ") + xval;
                dn("s09444008 hasValue=1 k=%d str[k]=[%s]", k, str[k].s() );
    	    }
        }

		if ( is3D ) {
    		//Jstr zval(buffers[_tabnum]+offsetz[k], collenz[k] ); zval.trim0();
    		Jstr zval;
            JagMath::fromBase254Len(zval, buffers[_tabnum]+offsetz[k], collenz[k] );
            zval.trim0();

			if ( zval.isNotNull() ) {
				str[k] += Jstr(":") + zval;
			}
		}

		if ( metrics > 0 ) { 
            str[k] += Jstr(":"); 
        }

		for ( int m =0; m < metrics; ++m ) {
			offset = metricoffset[k][m];
			collen = metriclen[k][m];
    		Jstr mval(buffers[_tabnum]+offset, collen ); 
			mval.trimNull();
			if ( 0 == m ) {
				mstr = mval;
			} else {
				mstr += Jstr("#") + mval;
			}
		}
		if ( metrics > 0 ) { str[k] += mstr; }

    	if ( ! ntr ) {
			if ( hasValue ) {
    			_builder->_pparam->_rowHash->addKeyValue( fullname[k], str[k] );
			}
    		_builder->_pparam->_rowUUID = uuid;
    		return;
    	}
	} // end of numCols loop


	Jstr xyzbuf;
	int geocoloffset, geocollen;
	int geomoffset, geomlen;
	int geonoffset, geonlen;
	int lastcol, lastm, lastn;

	nm = db + "." + tab + ".geo:col";
	if ( maps[_tabnum]->getValue( nm, acqpos) ) {
		geocoloffset =  attrs[_tabnum][acqpos].offset;
		geocollen =  attrs[_tabnum][acqpos].length;
	} else { 
        dn("s23810 mals getvale :col false return");
        return; 
    }

    dn("s2022801 dim=%d", dim );

    if ( dim > 1 ) {
    	nm = db + "." + tab + ".geo:m";
    	if ( maps[_tabnum]->getValue( nm, acqpos) ) {
    		geomoffset =  attrs[_tabnum][acqpos].offset;
    		geomlen =  attrs[_tabnum][acqpos].length;
    	} else { return; }
    
    	nm = db + "." + tab + ".geo:n";
    	if ( maps[_tabnum]->getValue( nm, acqpos) ) {
    		geonoffset =  attrs[_tabnum][acqpos].offset;
    		geonlen =  attrs[_tabnum][acqpos].length;
    	} else { return; }
    
    }


	char *kvbuf = (char*)jagmalloc(ntr->KEYVALLEN+1);
	memset( kvbuf, 0, ntr->KEYVALLEN+1 );
	Jstr us;
	lastcol = lastm = lastn = -1;
	int newcol, newm, newn;
	char sep = 0;
	int numAdded[numCols];

	for ( int k = 0; k < numCols; ++k ) { 
        numAdded[k] = 0; 
    }

	int mgroup = 0;
	Jstr gf;

	while ( ntr->getNext(kvbuf) ) {

		//if ( 0 == strncmp(kvbuf+bbox[BBUUIDLEN-1].offset, uuid.c_str(), uuid.size() ) )
		if ( 0 == strncmp(kvbuf+bbox[uuidCol].offset, uuid.c_str(), uuid.size() ) ) {

			//gf = Jstr(kvbuf+geocoloffset, geocollen ); 

            JagMath::fromBase254Len(gf, kvbuf+geocoloffset, geocollen );
			gf.trimNull();
			newcol = jagatoi( gf.s() );

            if ( dim > 1 ) {
    			//gf = Jstr( kvbuf+geomoffset, geomlen ); 
                JagMath::fromBase254Len(gf, kvbuf+geomoffset, geomlen );
    			gf.trimNull();
    			newm = jagatoi( gf.s() );
    
    			//gf = Jstr( kvbuf+geonoffset, geonlen ); 
                JagMath::fromBase254Len(gf, kvbuf+geonoffset, geonlen );
    			gf.trimNull();
    			newn = jagatoi( gf.s() );
            } else {
                newm = newn = 0;
            }


			if ( lastcol >=0 && newcol != lastcol ) {
				lastcol = newcol;
				lastm = -1;
				lastn = -1;
			} else if ( lastm >=0 && newm != lastm ) {
				lastcol = newcol;
				lastm = newm;
				lastn = -1;
				sep = '!';  // new polygon or new multistring
				++ mgroup;
			} else if ( lastn >=0 && newn != lastn ) {
				lastcol = newcol;
				lastm = newm;
				lastn = newn;
				sep = '|';  // new ring or new linestring
			} else {
				lastcol = newcol;
				lastm = newm;
				lastn = newn;
				sep = 0;
			}

            bool emptyField;

   			for ( int k=0; k < numCols; ++k ) {
    			// each column  col1 col2
    			//if ( *(kvbuf+offsetx[k]) != '\0' && *(kvbuf+offsety[k]) != '\0' ) 
                emptyField = true;
                if ( dim >= 2 ) {
    			    if ( *(kvbuf+offsetx[k]) != '\0' && *(kvbuf+offsety[k]) != '\0' ) {
                        emptyField = false;
                    }
                } else if ( dim == 1 ) {
    			    if ( *(kvbuf+offsetx[k]) != '\0' ) {
                        emptyField = false;
                    }
                }

                dn("s11108 k=%d emptyField=%d", k, emptyField );

    			if ( ! emptyField ) {
                    JagMath::fromBase254Len( v1, kvbuf+offsetx[k], collenx[k] );
                    v1.trim0();

                    if ( dim >= 2 ) {
                        JagMath::fromBase254Len( v2, kvbuf+offsety[k], colleny[k] );
                        v2.trim0();
                    }

                    /***
                    dn("s34004005 rawx:");
                    dumpmem(kvbuf+offsetx[k], collenx[k]);
                    dn("s34004005 rawy:");
                    dumpmem(kvbuf+offsety[k], colleny[k]);
                    dn("s33408 k=%d v1-x=[%s]  v2-y=[%s]", k, v1.s(), v2.s() );
                    ***/

                    if ( dim >= 2 ) {
					    xyzbuf = Jstr(" ") + v1 + ":" + v2;
                    } else {
					    xyzbuf = Jstr(" ") + v1;
                        dn("s2022938 zyxbuf=[%s]", xyzbuf.s() );
                    }

					if ( is3D ) {
                        JagMath::fromBase254Len( v3, kvbuf+offsetz[k], collenz[k] );
                        v3.trim0();
						xyzbuf += Jstr(":") + v3;
					}

                    emptyField = true;
                    if ( dim >=2 ) {
					    if ( v1.isNotNull() && v2.isNotNull() ) {
                            emptyField = false;
                        }
                    } else {
					    if ( v1.isNotNull() ) {
                            emptyField = false;
                        }
                    }

                    dn("s110297 k=%d emptyField=%d", k, emptyField );
					if (  ! emptyField  ) {
					    if ( sep ) { 
						    str[k] += " ";
						    str[k] += sep; 
					    }
    				    str[k] += xyzbuf;
					    ++ numAdded[k];
                        dn("s0833381 xyzbuf=[%s]", xyzbuf.s() );
                        dn("s0833381 str[k=%d]=[%s]", k, str[k].s() );
					} else {
                        dn("s5093004 v1 and v2 are NULL");
					}

					metrics = metricoffset[k].size();
					for ( int m=0; m < metrics; ++m ) {
						offset = metricoffset[k][m];
						collen = metriclen[k][m];
    					Jstr mval( kvbuf+offset, collen ); 
						mval.trimNull();
						if ( 0 == m ) {
							mstr = mval;
						} else {
							mstr += Jstr("#") + mval;
						}
					}
					if ( metrics > 0 ) {
						str[k] += Jstr(":") + mstr;
					}
    			}
   			}
		} else {
			ntr->putBack( kvbuf );
			break;
		}
	}  // end of getNext()

	for ( int k=0; k < numCols; ++k ) {
		if ( str[k].size() > 0 && numAdded[k] > 0 ) {
			rc = _builder->_pparam->_rowHash->addKeyValue( fullname[k], str[k] );
            dn("s23073748 _rowHash->addKeyValue k=%d fullname=[%s]    str=[%s]",  k, fullname[k].s(), str[k].s() );
		} else {
            dn("s333038 str[k=%d].size()=%ld numAdded[k]=%d no addKeyValue", k, str[k].size(), numAdded[k] );
		}
	}

	_builder->_pparam->_rowUUID = uuid;
	free( kvbuf );
}


BinaryOpNode::BinaryOpNode( BinaryExpressionBuilder* builder, short op, int opArgs, ExprElementNode *l, ExprElementNode *r,
    				const JagParseAttribute &jpa, int arg1, int arg2, Jstr carg1 )
{
    dn("s6781008 BinaryOpNode::BinaryOpNode ctor op=%d", op );

	_binaryOp = op; _left = l; _right = r; _jpa = jpa; _arg1 = arg1; _arg2 = arg2; _carg1 = carg1;
	_numCnts = _initK = _stddevSum = _stddevSumSqr = _nodenum = 0;
	_builder = builder;
	_isElement = false;
	_nargs = opArgs;
	_reg = NULL;
}

// dtor
BinaryOpNode::~BinaryOpNode()
{
	if ( _isDestroyed ) return;
	clear();
	_isDestroyed = true;
}

void BinaryOpNode::clear()
{
	if ( _left ) { _left->clear(); delete _left; _left = NULL; }
	if ( _right ) { _right->clear(); delete _right; _right = NULL; }
	if ( _reg ) { delete _reg; _reg = NULL; }
}

int BinaryOpNode::setWhereRange( const JagHashStrInt *maps[], const JagSchemaAttribute *attrs[], 
						const int keylen[], const int numKeys[], int numTabs, bool &hasValue, 
						JagMinMax *minmax, JagFixString &str, int &typeMode, int &tabnum ) 
{
	#ifdef DEBUG_STACK
	i("s2039381 BinaryOpNode::setWhereRange ... \n" );
	i("s333329 minmax->printd()\n"); 
	for (int i=0; i < numTabs; ++i ) { minmax[i].printd(); }
	i("\n");
	#endif

    dn("s452201 BinaryOpNode::setWhereRange()");

	str = "";
	JagMinMax  leftbuf[numTabs];
	JagMinMax  rightbuf[numTabs];

	for ( int i = 0; i < numTabs; ++i ) {
		leftbuf[i].setbuflen( keylen[i] );
		rightbuf[i].setbuflen( keylen[i] );
	}

	JagFixString lstr, rstr;
    Jstr         ltype, rtype;
	bool         lhasVal = 0, rhasVal = 0;
	int          leftVal = 1, rightVal = 1, ltmode = 0, rtmode = 0, ltabnum = -1, rtabnum = -1, result = 0;

	if ( _left ) {
		leftVal = _left->setWhereRange( maps, attrs, keylen, numKeys, numTabs, lhasVal, leftbuf, lstr, ltmode, ltabnum );
        dn("c3330981 _left->setWhereRange leftVal=%d", leftVal );
        ltype = _left->_type;
	}

	if ( _right ) {
		rightVal = _right->setWhereRange( maps, attrs, keylen, numKeys, numTabs, rhasVal, rightbuf, rstr, rtmode, rtabnum );
        dn("c3330982 _right->setWhereRange leftVal=%d", rightVal );
        rtype = _right->_type;
	}

	hasValue = lhasVal || rhasVal;
    if ( leftVal < 0 || rightVal < 0 ) {
        dn("s31220 leftVal=%d  rightVal=%d  result=-1", leftVal, rightVal );
		result = -1;
	} else if ( isAggregateOp(_binaryOp) ) {
		result = -2; 
	} else {
        dn("s2300817 _doWhereCalc _type=[%s] ...", _type.s() );
		result = _doWhereCalc( maps, attrs, keylen, numKeys, numTabs, ltmode, rtmode, ltabnum, rtabnum,
							   minmax, leftbuf, rightbuf, str, lstr, ltype, rstr, rtype );
		if ( result > 0 ) {
			if ( leftVal == 0 && rightVal == 0 ) result = 0;
			else if ( leftVal == 1 && rightVal == 1 ) result = 1;
			else if ( leftVal != rightVal && JAG_LOGIC_OR == _binaryOp ) result = 0;
			else if ( leftVal != rightVal && JAG_LOGIC_AND == _binaryOp ) result = 1;
			else result = 0; 
		} else {
			#ifdef DEBUG_STACK
			d("s203337 _doWhereCalc result=%d \n", result );
			this->print(0);
			d("\n");
			d("s341938 _left=%0x _right=%0x\n", _left, _right );
			d("s333754 leftVal=%d rightVal=%d   lhasVal=%d rhasVal=%d\n", leftVal, rightVal, lhasVal, rhasVal );
			#endif
		}
	}
	
	if ( ltmode > rtmode ) typeMode = ltmode;
	else typeMode = rtmode;

	tabnum = -1;
	
    return result;
}


void BinaryOpNode::print( int mode ) {
	if ( 0 == mode ) {
		if ( _left && _right) {
			i("L("); _left->print( mode ); i(")");
			i(" [op=%s] ", binaryOpStr( _binaryOp ).c_str() );
			i("R("); _right->print( mode );  i(")");
		} else if ( _left && !_right ) {
			i(" [op=%s] ", binaryOpStr( _binaryOp ).c_str() );
			i("L("); _left->print( mode ); i(")");
		} else if ( !_left && _right ) {
			i(" [op=%s] ", binaryOpStr( _binaryOp ).c_str() );
			i("R("); _right->print( mode ); i(")");
		}
	} else {
		if ( _left && _right) {
			i("L("); _left->print( mode ); i(")");
			i(" [op=%d] ", _binaryOp );
			i("R("); _right->print( mode );  i(")");
		} else if ( _left && !_right ) {
			i(" [op=%d] ", _binaryOp );
			i("L("); _left->print( mode ); i(")");
		} else if ( !_left && _right ) {
			i(" [op=%d] ", _binaryOp );
			i("R("); _right->print( mode ); i(")");
		} else {
			i("\nBadBinCode=%d\n",  _binaryOp );
		}
	}
}

int BinaryOpNode::getClusterByUuid(const JagTableOrIndexAttrs *objAttr )
{
	if ( ! _left  && ! _right) {
		// left right all NULL
		return -100;
	}

	if ( _left && ! _right ) {
		// left not NULL,  right NULL
		unsigned char nodeT = _left->getNodeType(); 
		if ( nodeT == JAG_ELEMENT_NODE ) {
			return -4;
		}
		return _left->getClusterByUuid( objAttr );
	}

	if ( ! _left && _right ) {
		// left NULL,  right not NULL
		unsigned char nodeT = _right->getNodeType(); 
		if ( nodeT == JAG_ELEMENT_NODE ) {
			return -6;
		}
		return _right->getClusterByUuid( objAttr );
	}

	// left and right Not NULL
	unsigned char nodeT1 = _left->getNodeType(); 
	unsigned char nodeT2 = _right->getNodeType(); 

	if ( nodeT1 == JAG_ELEMENT_NODE  && nodeT2 == JAG_ELEMENT_NODE ) {
		const char *pn;
		if ( _left->getName( pn ) < 0 ) {
			return -11;
		}

		const char *pv;
		if ( ! _right->getValue( pv ) ) {
			return -13;
		}

		// check is pn is uuid
		const char *pdot = strrchr(pn, '.'); 
		if ( pdot ) {
			pn = pdot+1;
		} 

		int getpos = objAttr->schemaRecord.getPosition( pn );
		if ( getpos < 0 ) {
			d("c33210 pn=[%s] getpos=%d return -15\n", pn, getpos );
			return -15;
		}

		d("c33220 pn=[%s] getpos=%d\n", pn, getpos );
		char spare1 = (*objAttr->schemaRecord.columnVector)[getpos].spare[1];
		if ( spare1 == JAG_C_COL_TYPE_UUID[0] ) {
			if ( ! strchr(pv, '@') ) {
				return -16;
			}

			JagStrSplit sp(pv, '@');
            if ( sp.size() < 2) {
                return -18;
            }

			int len = sp.length();
            Jstr digits;
            JagMath::fromBase62( digits, sp[len-1] );
			int clus = atoi( digits.s() );
			d("c322203 pv=[%s] cluster=%d\n", pv, clus );
			return clus;
		} else {
			return -20;
		}
	}

	if ( nodeT1 == JAG_ELEMENT_NODE ) {
		return _right->getClusterByUuid(objAttr);

	}
	
	if ( nodeT2 == JAG_ELEMENT_NODE ) {
		return _left->getClusterByUuid(objAttr);
	}

	int cls = _left->getClusterByUuid(objAttr);
	if ( cls >= 0 ) {
		return cls;
	} else {
		return _right->getClusterByUuid(objAttr);
	}
}

Jstr BinaryOpNode::binaryOpStr( short binaryOp )
{
	Jstr str;
	if ( binaryOp == JAG_FUNC_EQUAL ) str = "=";
	else if ( binaryOp == JAG_LEFT_BRA ) str = "(";
	else if ( binaryOp == JAG_FUNC_NOTEQUAL ) str = "!=";
	else if ( binaryOp == JAG_FUNC_LESSTHAN ) str = "<";
	else if ( binaryOp == JAG_FUNC_LESSEQUAL ) str = "<=";
	else if ( binaryOp == JAG_FUNC_GREATERTHAN ) str = ">";
	else if ( binaryOp == JAG_FUNC_GREATEREQUAL ) str = ">=";
	else if ( binaryOp == JAG_FUNC_LIKE ) str = "like";
	else if ( binaryOp == JAG_FUNC_MATCH ) str = "match";
	else if ( binaryOp == JAG_LOGIC_AND ) str = "and";
	else if ( binaryOp == JAG_LOGIC_OR ) str = "or";
	else if ( binaryOp == JAG_NUM_ADD ) str = "+";
	else if ( binaryOp == JAG_STR_ADD ) str = ".";
	else if ( binaryOp == JAG_NUM_SUB ) str = "-";
	else if ( binaryOp == JAG_NUM_MULT ) str = "*";
	else if ( binaryOp == JAG_NUM_DIV ) str = "/";
	else if ( binaryOp == JAG_NUM_REM ) str = "%";
	else if ( binaryOp == JAG_NUM_POW ) str = "^";
	else if ( binaryOp == JAG_FUNC_POW ) str = "pow";
	else if ( binaryOp == JAG_FUNC_MOD ) str = "mod";
	else if ( binaryOp == JAG_FUNC_LENGTH ) str = "length";
	else if ( binaryOp == JAG_FUNC_LINELENGTH ) str = "linelength";
	else if ( binaryOp == JAG_FUNC_ABS ) str = "abs";
	else if ( binaryOp == JAG_FUNC_ACOS ) str = "acos";
	else if ( binaryOp == JAG_FUNC_ASIN ) str = "asin";
	else if ( binaryOp == JAG_FUNC_ATAN ) str = "atan";
	else if ( binaryOp == JAG_FUNC_COS ) str = "cos";
	else if ( binaryOp == JAG_FUNC_MILETOMETER ) str = "miletometer";
	else if ( binaryOp == JAG_FUNC_MILETOKILOMETER ) str = "miletokilometer";
	else if ( binaryOp == JAG_FUNC_METERTOMILE ) str = "metertomile";
	else if ( binaryOp == JAG_FUNC_KILOMETERTOMILE ) str = "kilometertomile";
	else if ( binaryOp == JAG_FUNC_STRDIFF ) str = "strdiff";
	else if ( binaryOp == JAG_FUNC_SIN ) str = "sin";
	else if ( binaryOp == JAG_FUNC_TAN ) str = "tan";
	else if ( binaryOp == JAG_FUNC_COT ) str = "cot";
	else if ( binaryOp == JAG_FUNC_LOG2 ) str = "log2";
	else if ( binaryOp == JAG_FUNC_LOG10 ) str = "log10";
	else if ( binaryOp == JAG_FUNC_LOG ) str = "ln";
	else if ( binaryOp == JAG_FUNC_DEGREES ) str = "degrees";
	else if ( binaryOp == JAG_FUNC_RADIANS ) str = "radians";
	else if ( binaryOp == JAG_FUNC_SQRT ) str = "sqrt";
	else if ( binaryOp == JAG_FUNC_CEIL ) str = "ceil";
	else if ( binaryOp == JAG_FUNC_FLOOR ) str = "floor";
	else if ( binaryOp == JAG_FUNC_UPPER ) str = "upper";
	else if ( binaryOp == JAG_FUNC_LOWER ) str = "lower";
	else if ( binaryOp == JAG_FUNC_LTRIM ) str = "ltrim";
	else if ( binaryOp == JAG_FUNC_RTRIM ) str = "rtrim";
	else if ( binaryOp == JAG_FUNC_TRIM ) str = "trim";
	else if ( binaryOp == JAG_FUNC_SUBSTR ) str = "substr";
	else if ( binaryOp == JAG_FUNC_SECOND ) str = "second";
	else if ( binaryOp == JAG_FUNC_MINUTE ) str = "minute";
	else if ( binaryOp == JAG_FUNC_HOUR ) str = "hour";
	else if ( binaryOp == JAG_FUNC_DAY ) str = "day";
	else if ( binaryOp == JAG_FUNC_MONTH ) str = "month";
	else if ( binaryOp == JAG_FUNC_YEAR ) str = "year";
	else if ( binaryOp == JAG_FUNC_DATE ) str = "date";
	else if ( binaryOp == JAG_FUNC_DATEDIFF ) str = "datediff";
	else if ( binaryOp == JAG_FUNC_DAYOFMONTH ) str = "dayofmonth";
	else if ( binaryOp == JAG_FUNC_DAYOFWEEK ) str = "dayofweek";
	else if ( binaryOp == JAG_FUNC_DAYOFYEAR ) str = "dayofyear";
	else if ( binaryOp == JAG_FUNC_CURDATE ) str = "curdate";
	else if ( binaryOp == JAG_FUNC_CURTIME ) str = "curtime";
	else if ( binaryOp == JAG_FUNC_NOW ) str = "now";
	else if ( binaryOp == JAG_FUNC_TIME ) str = "time";
	else if ( binaryOp == JAG_FUNC_PI ) str = "pi";
	else if ( binaryOp == JAG_FUNC_DISTANCE ) str = "distance";
	else if ( binaryOp == JAG_FUNC_SIMILARITY ) str = "similarity";
	else if ( binaryOp == JAG_FUNC_CONTAIN ) str = "contain";
	else if ( binaryOp == JAG_FUNC_SAME ) str = "equal";
	else if ( binaryOp == JAG_FUNC_WITHIN ) str = "within";
	else if ( binaryOp == JAG_FUNC_CLOSESTPOINT ) str = "closestpoint";
	else if ( binaryOp == JAG_FUNC_ANGLE ) str = "angle";
	else if ( binaryOp == JAG_FUNC_AREA ) str = "area";
	else if ( binaryOp == JAG_FUNC_PERIMETER ) str = "perimeter";
	else if ( binaryOp == JAG_FUNC_VOLUME ) str = "volume";
	else if ( binaryOp == JAG_FUNC_DIMENSION ) str = "dimension";
	else if ( binaryOp == JAG_FUNC_GEOTYPE ) str = "geotype";
	else if ( binaryOp == JAG_FUNC_ISCLOSED ) str = "isclosed";
	else if ( binaryOp == JAG_FUNC_ISSIMPLE ) str = "issimple";
	else if ( binaryOp == JAG_FUNC_ISVALID ) str = "isvalid";
	else if ( binaryOp == JAG_FUNC_ISRING ) str = "isring";
	else if ( binaryOp == JAG_FUNC_ISPOLYGONCCW ) str = "ispolygonccw";
	else if ( binaryOp == JAG_FUNC_ISPOLYGONCW ) str = "ispolygoncw";
	else if ( binaryOp == JAG_FUNC_ISCONVEX ) str = "isconvex";
	else if ( binaryOp == JAG_FUNC_ISONLEFT ) str = "isonleft";
	else if ( binaryOp == JAG_FUNC_ISONRIGHT ) str = "isonright";
	else if ( binaryOp == JAG_FUNC_LEFTRATIO ) str = "leftratio";
	else if ( binaryOp == JAG_FUNC_RIGHTRATIO ) str = "rightratio";
	else if ( binaryOp == JAG_FUNC_POINTN ) str = "pointn";
	else if ( binaryOp == JAG_FUNC_EXTENT ) str = "extent";
	else if ( binaryOp == JAG_FUNC_STARTPOINT ) str = "startpoint";
	else if ( binaryOp == JAG_FUNC_CONVEXHULL ) str = "convexhull";
	else if ( binaryOp == JAG_FUNC_CONCAVEHULL ) str = "concavehull";
	else if ( binaryOp == JAG_FUNC_TOPOLYGON ) str = "topolygon";
	else if ( binaryOp == JAG_FUNC_TOMULTIPOINT ) str = "tomultipoint";
	else if ( binaryOp == JAG_FUNC_UNION ) str = "union";
	else if ( binaryOp == JAG_FUNC_LOCATEPOINT ) str = "locatepoint";
	else if ( binaryOp == JAG_FUNC_INTERSECTION ) str = "intersection";
	else if ( binaryOp == JAG_FUNC_DIFFERENCE ) str = "difference";
	else if ( binaryOp == JAG_FUNC_SYMDIFFERENCE ) str = "symdifference";
	else if ( binaryOp == JAG_FUNC_COLLECT ) str = "collect";
	else if ( binaryOp == JAG_FUNC_OUTERRING ) str = "outerring";
	else if ( binaryOp == JAG_FUNC_OUTERRINGS ) str = "outerrings";
	else if ( binaryOp == JAG_FUNC_INNERRINGS ) str = "innerrings";
	else if ( binaryOp == JAG_FUNC_RINGN ) str = "ringn";
	else if ( binaryOp == JAG_FUNC_METRICN ) str = "metricn";
	else if ( binaryOp == JAG_FUNC_GEOJSON ) str = "geojson";
	else if ( binaryOp == JAG_FUNC_WKT ) str = "wkt";
	else if ( binaryOp == JAG_FUNC_POLYGONN ) str = "polygonn";
	else if ( binaryOp == JAG_FUNC_ASTEXT ) str = "astext";
	else if ( binaryOp == JAG_FUNC_UNIQUE ) str = "unique";
	else if ( binaryOp == JAG_FUNC_BUFFER ) str = "buffer";
	else if ( binaryOp == JAG_FUNC_ENDPOINT ) str = "endpoint";
	else if ( binaryOp == JAG_FUNC_NUMPOINTS ) str = "numpoints";
	else if ( binaryOp == JAG_FUNC_NUMSEGMENTS ) str = "numsegments";
	else if ( binaryOp == JAG_FUNC_NUMLINES ) str = "numlines";
	else if ( binaryOp == JAG_FUNC_NUMRINGS ) str = "numrings";
	else if ( binaryOp == JAG_FUNC_NUMPOLYGONS ) str = "numpolygons";
	else if ( binaryOp == JAG_FUNC_NUMINNERRINGS ) str = "numinnerrings";
	else if ( binaryOp == JAG_FUNC_SRID ) str = "srid";
	else if ( binaryOp == JAG_FUNC_SUMMARY ) str = "summary";
	else if ( binaryOp == JAG_FUNC_XMIN ) str = "xmin";
	else if ( binaryOp == JAG_FUNC_YMIN ) str = "ymin";
	else if ( binaryOp == JAG_FUNC_ZMIN ) str = "zmin";
	else if ( binaryOp == JAG_FUNC_XMAX ) str = "xmax";
	else if ( binaryOp == JAG_FUNC_YMAX ) str = "ymax";
	else if ( binaryOp == JAG_FUNC_ZMAX ) str = "zmax";
	else if ( binaryOp == JAG_FUNC_COVEREDBY ) str = "coveredby";
	else if ( binaryOp == JAG_FUNC_COVER ) str = "cover";
	else if ( binaryOp == JAG_FUNC_INTERSECT ) str = "intersect";
	else if ( binaryOp == JAG_FUNC_DISJOINT ) str = "disjoint";
	else if ( binaryOp == JAG_FUNC_SIMILARITY ) str = "similarity";
	else if ( binaryOp == JAG_FUNC_NEARBY ) str = "nearby";
	else if ( binaryOp == JAG_FUNC_ALL ) str = "all";
	else if ( binaryOp == JAG_FUNC_CENTROID ) str = "centroid";
	else if ( binaryOp == JAG_FUNC_XMINPOINT ) str = "xminpoint";
	else if ( binaryOp == JAG_FUNC_XMAXPOINT ) str = "xmaxpoint";
	else if ( binaryOp == JAG_FUNC_YMINPOINT ) str = "yminpoint";
	else if ( binaryOp == JAG_FUNC_YMAXPOINT ) str = "ymaxpoint";
	else if ( binaryOp == JAG_FUNC_ZMINPOINT ) str = "zminpoint";
	else if ( binaryOp == JAG_FUNC_ZMAXPOINT ) str = "zmaxpoint";
	else if ( binaryOp == JAG_FUNC_REVERSE ) str = "reverse";
	else if ( binaryOp == JAG_FUNC_SCALE ) str = "scale";
	else if ( binaryOp == JAG_FUNC_SCALEAT ) str = "scaleat";
	else if ( binaryOp == JAG_FUNC_SCALESIZE ) str = "scalesize";
	else if ( binaryOp == JAG_FUNC_TRANSLATE ) str = "translate";
	else if ( binaryOp == JAG_FUNC_TRANSSCALE ) str = "transscale";
	else if ( binaryOp == JAG_FUNC_ROTATE ) str = "rotate";
	else if ( binaryOp == JAG_FUNC_ROTATEAT ) str = "rotateat";
	else if ( binaryOp == JAG_FUNC_ROTATESELF ) str = "rotateself";
	else if ( binaryOp == JAG_FUNC_AFFINE ) str = "affine";
	else if ( binaryOp == JAG_FUNC_VORONOIPOLYGONS ) str = "voronoipolygons";
	else if ( binaryOp == JAG_FUNC_VORONOILINES ) str = "voronoilines";
	else if ( binaryOp == JAG_FUNC_DELAUNAYTRIANGLES ) str = "delaunaytriangles";
	else if ( binaryOp == JAG_FUNC_MINIMUMBOUNDINGCIRCLE ) str = "minimumboundingcircle";
	else if ( binaryOp == JAG_FUNC_MINIMUMBOUNDINGSPHERE ) str = "minimumboundingsphere";
	else if ( binaryOp == JAG_FUNC_KNN ) str = "knn";
	else if ( binaryOp == JAG_FUNC_SUM ) str = "sum";
	else if ( binaryOp == JAG_FUNC_COUNT ) str = "count";
	else if ( binaryOp == JAG_FUNC_AVG ) str = "avg";
	else if ( binaryOp == JAG_FUNC_STDDEV ) str = "stddev";
	else if ( binaryOp == JAG_FUNC_MIN ) str = "min";
	else if ( binaryOp == JAG_FUNC_MAX ) str = "max";
	else if ( binaryOp == JAG_FUNC_FIRST ) str = "first";
	else if ( binaryOp == JAG_FUNC_LAST ) str = "last";
	return str;
}


int BinaryOpNode::setFuncAttribute( const JagHashStrInt *maps[], const JagSchemaAttribute *attrs[], 
	                                int &constMode, int &typeMode, bool &isAggregate, Jstr &type, int &collen, int &siglen )
{
    dn("s3450021 BinaryOpNode::setFuncAttribute ");
	if ( !_left && !_right ) {
        dn("s6500012  !_left && !_right ");
		typeMode = 0;
		constMode = 0;
		isAggregate = 0;
		type = JAG_C_COL_TYPE_STR;
		collen = getFuncLength( _binaryOp );
        dn("s49444040 type = JAG_C_COL_TYPE_STR collen = getFuncLength( _binaryOp )=%d", collen );
		siglen = 0;
		return 1;
	}

	bool laggr = 0, raggr = 0;
	Jstr ltype, rtype;
	int leftVal = 1, rightVal = 1, ltmode = 0, rtmode = 0, lcmode = 0, rcmode = 0, 
		lcollen = 0, rcollen = 0, lsiglen = 0, rsiglen = 0;

    if ( _left ) leftVal = _left->setFuncAttribute( maps, attrs, lcmode, ltmode, laggr, ltype, lcollen, lsiglen );
	if ( _right ) rightVal = _right->setFuncAttribute( maps, attrs, rcmode, rtmode, raggr, rtype, rcollen, rsiglen );

	if ( !_left || !leftVal || !rightVal ) {
		return 0;
	}

	if ( !checkAggregateValid( lcmode, rcmode, laggr, raggr ) ) {
        dn("s2330064 checkAggregateValid return 0");
		return 0;
	}
		
	if ( _binaryOp == JAG_FUNC_CONVEXHULL || _binaryOp == JAG_FUNC_BUFFER || _binaryOp == JAG_FUNC_UNIQUE
	     || _binaryOp == JAG_FUNC_RINGN || _binaryOp == JAG_FUNC_INNERRINGN ||  _binaryOp == JAG_FUNC_POLYGONN
		 || _binaryOp == JAG_FUNC_UNION || _binaryOp == JAG_FUNC_COLLECT || _binaryOp == JAG_FUNC_INTERSECTION
		 || _binaryOp == JAG_FUNC_LINESUBSTRING || _binaryOp == JAG_FUNC_ADDPOINT || _binaryOp == JAG_FUNC_SETPOINT
		 || _binaryOp == JAG_FUNC_REMOVEPOINT || _binaryOp == JAG_FUNC_REVERSE ||  _binaryOp == JAG_FUNC_TRANSLATE
		 || _binaryOp == JAG_FUNC_SCALE || _binaryOp == JAG_FUNC_SCALEAT || _binaryOp == JAG_FUNC_SCALESIZE
		 || _binaryOp == JAG_FUNC_TRANSSCALE || _binaryOp == JAG_FUNC_ROTATE || _binaryOp == JAG_FUNC_ROTATEAT
		 || _binaryOp == JAG_FUNC_ROTATESELF || _binaryOp == JAG_FUNC_AFFINE || _binaryOp == JAG_FUNC_VORONOIPOLYGONS
		 || _binaryOp == JAG_FUNC_VORONOILINES || _binaryOp == JAG_FUNC_DELAUNAYTRIANGLES 
		 || _binaryOp == JAG_FUNC_ASTEXT ||  _binaryOp == JAG_FUNC_DIFFERENCE || _binaryOp == JAG_FUNC_SYMDIFFERENCE
		 || _binaryOp == JAG_FUNC_CONCAVEHULL || _binaryOp == JAG_FUNC_KNN || _binaryOp == JAG_FUNC_METRICN
		 || _binaryOp == JAG_FUNC_OUTERRING || _binaryOp == JAG_FUNC_OUTERRINGS || _binaryOp == JAG_FUNC_INNERRINGS ) {
		ltmode = 0;
		type = JAG_C_COL_TYPE_STR;
		collen = JAG_MAX_POINTS_SENT*JAG_POINT_LEN + 2;
		siglen = 0;
	} else if ( _binaryOp == JAG_FUNC_GEOJSON || _binaryOp == JAG_FUNC_WKT ) {
		JagStrSplit ss( _carg1, ':');
		ltmode = 0;
		type = JAG_C_COL_TYPE_STR;
		collen = ss[0].toInt() * JAG_POINT_LEN + 2;
		siglen = 0;
	} else if ( _binaryOp == JAG_FUNC_TOPOLYGON || _binaryOp == JAG_FUNC_TOMULTIPOINT ) {
		ltmode = 0;
		type = JAG_C_COL_TYPE_STR;
		collen = JAG_MAX_POINTS_SENT*JAG_POINT_LEN/3 + 2;
		siglen = 0;
	} else if ( _binaryOp == JAG_FUNC_POINTN ||  _binaryOp == JAG_FUNC_STARTPOINT || _binaryOp == JAG_FUNC_ENDPOINT
				|| _binaryOp == JAG_FUNC_INTERPOLATE
				|| _binaryOp == JAG_FUNC_XMINPOINT || _binaryOp == JAG_FUNC_XMAXPOINT
				|| _binaryOp == JAG_FUNC_YMINPOINT || _binaryOp == JAG_FUNC_YMAXPOINT
				|| _binaryOp == JAG_FUNC_ZMINPOINT || _binaryOp == JAG_FUNC_ZMAXPOINT
                || _binaryOp == JAG_FUNC_CENTROID || _binaryOp == JAG_FUNC_CLOSESTPOINT ) {
		ltmode = 0;
		type = JAG_C_COL_TYPE_STR;
		collen = 3*JAG_POINT_LEN + 2;
		siglen = 0;
	} else if ( _binaryOp == JAG_FUNC_ANGLE || _binaryOp == JAG_FUNC_LINELENGTH ||  _binaryOp == JAG_FUNC_LOCATEPOINT ) {
		ltmode = 0;
		type = JAG_C_COL_TYPE_STR;
		collen = JAG_POINT_LEN + 2;
		siglen = 0;
	} else if ( _binaryOp == JAG_FUNC_EXTENT ) {
		ltmode = 0;
		type = JAG_C_COL_TYPE_STR;
		collen = 6*JAG_POINT_LEN + 5;
		siglen = 0;
	} else if ( _binaryOp == JAG_FUNC_GEOTYPE ) {
		ltmode = 0;
		type = JAG_C_COL_TYPE_STR;
		collen = 32;
		siglen = 0;
	} else if ( _binaryOp == JAG_FUNC_ISCLOSED ||  _binaryOp == JAG_FUNC_ISSIMPLE || _binaryOp == JAG_FUNC_ISCONVEX 
			    || _binaryOp == JAG_FUNC_ISONLEFT || _binaryOp == JAG_FUNC_ISONRIGHT
			    || _binaryOp == JAG_FUNC_ISRING || _binaryOp == JAG_FUNC_ISPOLYGONCCW || _binaryOp == JAG_FUNC_ISPOLYGONCW
			    ||  _binaryOp == JAG_FUNC_ISVALID ) {
		ltmode = 1;
		type = JAG_C_COL_TYPE_STR;
		collen = 2;
		siglen = 0;
	} else if ( _binaryOp == JAG_FUNC_SRID || _binaryOp == JAG_FUNC_NUMPOINTS 
				 || _binaryOp == JAG_FUNC_DIMENSION || _binaryOp == JAG_FUNC_NUMSEGMENTS
				 ||  _binaryOp == JAG_FUNC_NUMPOLYGONS ||  _binaryOp == JAG_FUNC_NUMLINES
	             || _binaryOp == JAG_FUNC_NUMRINGS || _binaryOp == JAG_FUNC_NUMINNERRINGS ) {
		ltmode = 1;
		type = JAG_C_COL_TYPE_DINT;
		collen = JAG_DINT_FIELD_LEN;
		siglen = 0;
	} else if ( _binaryOp == JAG_FUNC_SUMMARY ) {
		ltmode = 0;
		type = JAG_C_COL_TYPE_STR;
		collen = 128; 
		siglen = 0;
	} else if ( _binaryOp == JAG_FUNC_XMIN || _binaryOp == JAG_FUNC_YMIN || _binaryOp == JAG_FUNC_ZMIN
			    || _binaryOp == JAG_FUNC_LEFTRATIO || _binaryOp == JAG_FUNC_RIGHTRATIO
	            || _binaryOp == JAG_FUNC_XMAX || _binaryOp == JAG_FUNC_YMAX || _binaryOp == JAG_FUNC_ZMAX ) {
		ltmode = 2;
		type = JAG_C_COL_TYPE_DOUBLE;
		collen = JAG_DOUBLE_FIELD_LEN; 
		siglen = JAG_DOUBLE_SIG_LEN;
	} else if ( _binaryOp == JAG_FUNC_MINIMUMBOUNDINGCIRCLE ) {
		ltmode = 0;
		type = JAG_C_COL_TYPE_STR;
		collen = 3*JAG_POINT_LEN + 20;
		siglen = 0;
	} else if ( _binaryOp == JAG_FUNC_MINIMUMBOUNDINGSPHERE ) {
		ltmode = 0;
		type = JAG_C_COL_TYPE_STR;
		collen = 4*JAG_POINT_LEN + 20;
		siglen = 0;
	} else if ( !_right ) {
		if ( 0 == ltmode && 
			(( isStringOp(_binaryOp) || isSpecialOp(_binaryOp) || isTimedateOp(_binaryOp) ) 
			   && _binaryOp != JAG_FUNC_LENGTH ) ) {


            dn("s22201887 timeDateOpLen _binaryOp=%d(%s)", _binaryOp, binaryOpStr(_binaryOp).s() );

            int clen = timeDateOpLen( _binaryOp );
            dn("s22201887 timeDateOpLen _binaryOp=%d clen=%d", _binaryOp, clen );

			//if ( isTimedateOp(_binaryOp) ) 
            if ( clen > 0 ) {
				ltmode = 0;
				type = JAG_C_COL_TYPE_STR;
                collen = clen;
                //collen = 2*JAG_DATETIMENANO_FIELD_LEN; // enough to contain op result
				siglen = 0;
                dn("s293030 type = JAG_C_COL_TYPE_STR collen=%d siglen=0", collen );
			} else {
				type = ltype;
				collen = lcollen;
				siglen = lsiglen;
			}
		} else {
			ltmode = 2;
			type = JAG_C_COL_TYPE_LONGDOUBLE;
            /**
			collen = JAG_MAX_INT_LEN;
			siglen = JAG_MAX_SIG_LEN;
            **/
			collen = JAG_LONGDOUBLE_FIELD_LEN;
			siglen = JAG_LONGDOUBLE_SIG_LEN;
            dn("s0298718 ltmode = 2; JAG_C_COL_TYPE_LONGDOUBLE JAG_MAX_INT_LEN JAG_MAX_SIG_LEN");
		}
	} else {
		if ( 0 == ltmode && 0 == rtmode && _binaryOp == JAG_NUM_ADD ) {
			type = ltype;
			collen = lcollen + rcollen;
			siglen = 0;
		} else if ( _binaryOp == JAG_STR_ADD ) {
			type = ltype;
			collen = JAG_MAX_STRLEN_SENT;
			siglen = 0;
		} else {
			ltmode = 2;
			type = JAG_C_COL_TYPE_LONGDOUBLE;
            /**
			collen = JAG_MAX_INT_LEN;
			siglen = JAG_MAX_SIG_LEN;
            **/
			collen = JAG_LONGDOUBLE_FIELD_LEN;
			siglen = JAG_LONGDOUBLE_SIG_LEN;
            dn("s0290748 ltmode = 2; JAG_C_COL_TYPE_LONGDOUBLE JAG_MAX_INT_LEN JAG_MAX_SIG_LEN");
		}
	}
	
	if ( isAggregateOp(_binaryOp) || lcmode == 2 || rcmode == 2 ) {
        dn("s33300881 _binaryOp=[%s] isAggregate true", binaryOpStr(_binaryOp).s() );
		isAggregate = 1;
	} else {
        dn("s33300881 _binaryOp=[%s] isAggregate false", binaryOpStr(_binaryOp).s() );
    }

	if ( ltmode > rtmode ) typeMode = ltmode;
	else typeMode = rtmode;

	if ( isAggregate ) {
		constMode = 2;
	} else {
		if ( lcmode > rcmode ) constMode = lcmode;
		else constMode = rcmode;
	}
	
    return 1;
}

int BinaryOpNode
::getFuncAggregate( JagVector<Jstr> &selectParts, JagVector<int> &selectPartsOpcode, 
				    JagVector<int> &selColSetAggParts, JagHashMap<AbaxInt, AbaxInt> &selColToselParts, int &nodenum, int treenum )
{	
	if ( isAggregateOp( _binaryOp ) ) {

		Jstr parts, parts2;
		_left->getAggregateParts( parts, nodenum );
		_nodenum = nodenum++;

        dn("par10029 isAggregateOp _nodenum=%d", _nodenum);

		if ( _binaryOp == JAG_FUNC_MIN ) {
			parts = Jstr("min(") + parts + ")";
		} else if ( _binaryOp == JAG_FUNC_MAX ) {
			parts = Jstr("max(") + parts + ")";
		} else if ( _binaryOp == JAG_FUNC_AVG ) {
			parts = Jstr("avg(") + parts + ")";
		} else if ( _binaryOp == JAG_FUNC_SUM ) {
			parts = Jstr("sum(") + parts + ")";
		} else if ( _binaryOp == JAG_FUNC_COUNT ) {
			parts = Jstr("count(") + parts + ")";
		} else if ( _binaryOp == JAG_FUNC_STDDEV ) {
			parts2 = Jstr("avg(") + parts + ")";
			parts = Jstr("stddev(") + parts + ")";
		} else if ( _binaryOp == JAG_FUNC_FIRST ) {
			parts = Jstr("first(") + parts + ")";
		} else if ( _binaryOp == JAG_FUNC_LAST ) {
			parts = Jstr("last(") + parts + ")";
		}
		
		if ( parts2.length() > 0 ) {
            // avg
			selectParts.append( parts2 );
			selectPartsOpcode.append( JAG_FUNC_AVG );
			selColSetAggParts.append( _nodenum );
			selColToselParts.setValue( selectParts.length()-1, treenum, true );
		}

		selectParts.append( parts );
		selectPartsOpcode.append( _binaryOp );
		selColSetAggParts.append( _nodenum );  // stddev and prepended avg have same nodenum
		selColToselParts.setValue( selectParts.length()-1, treenum, true );
        dn("par339910 selColSetAggParts.append _nodenum=%d", _nodenum );
	} else {
        dn("par22209");
		if ( _left ) _left->getFuncAggregate( selectParts, selectPartsOpcode, selColSetAggParts, 
			                                  selColToselParts, nodenum, treenum );
		if ( _right ) _right->getFuncAggregate( selectParts, selectPartsOpcode, selColSetAggParts, 
			                                    selColToselParts, nodenum, treenum );
		_nodenum = nodenum++;
        dn("par33014 left and right,  _nodenum=%d", _nodenum );
		
	}
	return 1;
}


int BinaryOpNode::getAggregateParts( Jstr &parts, int &nodenum )
{
	Jstr lparts, rparts;
	if ( _left ) _left->getAggregateParts( lparts, nodenum );
	if ( _right ) _right->getAggregateParts( rparts, nodenum );

	formatAggregateParts( parts, lparts, rparts );

	_nodenum = nodenum++;

	return 1;
}


int BinaryOpNode::setAggregateValue( int nodenum, const char *buf, int length, const Jstr &type )
{
    dn("s82920110 setAggregateValue buf=[%s] type=[%s] optype=%s ", buf, type.s(), binaryOpStr(_binaryOp).s() );
    dn("s2233865 nodenum=%d  _nodenum=%d length=%d", nodenum, _nodenum, length );

	if ( _left ) _left->setAggregateValue( nodenum, buf, length, type );
	if ( _right ) _right->setAggregateValue( nodenum, buf, length, type );

	if ( nodenum == _nodenum ) {
		if ( _binaryOp == JAG_FUNC_AVG || _binaryOp == JAG_FUNC_SUM || _binaryOp == JAG_FUNC_STDDEV ) {
            dn("s8827200 setAggregateValue JAG_FUNC_AVG/JAG_FUNC_SUM/JAG_FUNC_STDDEV  buf=[%s]", buf );
            if ( type == JAG_C_COL_TYPE_FLOAT || type == JAG_C_COL_TYPE_DOUBLE || type == JAG_C_COL_TYPE_LONGDOUBLE   ) {
                Jstr norm;
                JagMath::fromBase254Len(norm, buf, length);
                _opString = norm;
                dn("s3620018 _opString = norm =[%s]", _opString.s() );
            } else {
                dn("s88737384 setAggregateValue not JAG_FUNC_AVG/JAG_FUNC_SUM/JAG_FUNC_STDDEV");
			    _opString = longDoubleToStr(raystrtold(buf, length));
            }
            dn("s8827202 _opString buf=[%s]", _opString.s() );
		} else {
            dn("par5400288 not JAG_FUNC_AVG/JAG_FUNC_SUM/JAG_FUNC_STDDEV");
            Jstr norm;
            JagMath::fromBase254Len(norm, buf, length);
            _opString = norm;
			//_opString = JagFixString( buf, length );
            dn("s266301 _binaryOp=%d(%s) dumpmem buf:", _binaryOp, binaryOpStr(_binaryOp).s() );
            //dumpmem(buf, length);
            dn("s200118 norm=[%s]", norm.s() );
		}
	} else {
        dn("s33395003 !!!!!!!! nodenum=%d != _nodenum=%d", nodenum, _nodenum);
        // 7/6/2023 ignores nodenum
        Jstr norm;
        JagMath::fromBase254Len(norm, buf, length);
        _opString = norm;
    }
	return 1;
}

int BinaryOpNode::checkFuncValid( JagMergeReaderBase *ntr, const JagHashStrInt *maps[], const JagSchemaAttribute *attrs[],
						         const char *buffers[], JagFixString &str, int &typeMode, Jstr &type, 
								 int &length, bool &first, bool useZero, bool setGlobal )
{
    dn("s320028 BinaryOpNode::checkFuncValid() ...");

	JagFixString lstr, rstr;
	Jstr ltype, rtype;
	int leftVal = 1, rightVal = 1, ltmode = 0, rtmode = 0, llength = 0, rlength = 0, result = 0;

	if ( first && setGlobal ) {
		_opString = ""; 
        _numCnts = _initK = _stddevSum = _stddevSumSqr = 0;
	}

    if ( _left ) {
		leftVal = _left->checkFuncValid( ntr, maps, attrs, buffers, lstr, ltmode, ltype, llength, first, useZero, setGlobal );
        dn("s403005 _left leftVal=%d ltype=%s", leftVal, ltype.s() );
	}

    if ( _right ) {
		rightVal = _right->checkFuncValid( ntr, maps, attrs, buffers, rstr, rtmode, rtype, rlength, first, useZero, setGlobal );
        dn("s403006 _right rightVal=%d rtype=%s", rightVal, rtype.s() );
	}

	if ( leftVal < 0 || rightVal < 0 ) { 
		result = -1; 
        dn("s503003  leftVal < 0 || rightVal < 0 result =-1");
	} else {
		if ( JAG_LOGIC_OR == _binaryOp ) {
			result = leftVal || rightVal;
		} else if ( JAG_LOGIC_AND == _binaryOp ) {
			result = leftVal && rightVal;
		} else if ( JAG_LEFT_BRA == _binaryOp ) {
			result = leftVal;
		} else if ( 2 == leftVal || 2 == rightVal ) {
			if ( 0 == leftVal || 0 == rightVal ) result = 0;
			else result = 2;
		} else {
			result = _doCalculation( lstr, rstr, ltmode, rtmode, ltype, rtype, llength, rlength, first );

            dn("s500281 _doCalculation result=%d", result );
			if ( result < 0 && setGlobal ) {
				_opString = ""; 
                _numCnts = _initK = _stddevSum = _stddevSumSqr = 0;
			}

			if ( useZero ) {
				if ( 0 == result ) result = 1;
			}
		}

		if ( ltmode > rtmode ) typeMode = ltmode;
		else typeMode = rtmode;

		if ( ltype.size() > 0 ) {
			type = ltype;
			length = llength;
			str = lstr;
			if ( setGlobal ) _opString = str;
            dn("s860023 ltype.size() > 0 type=ltype=[%s] str=[%s] length=%d", type.s(), str.s(), length );
		} else {
			type = rtype;
			length = rlength;
			str = lstr; 
			if ( setGlobal ) _opString = str;
            dn("s860025 ltype.size() <= 0 type=rtype=[%s] str=[%s] length=%d", type.s(), str.s(), length );
		}
	}
	
	return result;
}

int BinaryOpNode::checkFuncValidConstantCalc( JagFixString &str, int &typeMode, Jstr &type, int &length )
{
	bool first = 0;
	JagFixString lstr, rstr;
	Jstr ltype, rtype;
	int leftVal = 1, rightVal = 1, ltmode = 0, rtmode = 0, llength = 0, rlength = 0, result = 0;

	if ( _left ) {
        dn("12028 _left");
		leftVal = _left->checkFuncValidConstantCalc( lstr, ltmode, ltype, llength );
	}

	if ( _right ) {
        dn("12028 _right");
		rightVal = _right->checkFuncValidConstantCalc( rstr, rtmode, rtype, rlength );
	}

    dn("s347120 checkFuncValidConstantOnly lstr=[%s] rstr=[%s]", lstr.s(), rstr.s() );

	if ( leftVal < 0 || rightVal < 0 ) {
        dn("s767501 leftVal < 0 || rightVal < 0 result=-1");
        result = -1;
	} else {	
        dn("s170004 checkFuncValidConstantOnly _binaryOp=[%s]", binaryOpStr(_binaryOp).s() );

		if ( !isAggregateOp( _binaryOp ) ) {
            dn("s482003 not isAggregateOp [%s]",  binaryOpStr(_binaryOp).s() );

			if ( JAG_LOGIC_OR == _binaryOp ) {
				result = leftVal || rightVal;
			} else if ( JAG_LOGIC_AND == _binaryOp ) {
				result = leftVal && rightVal;
			} else {
                dn("s222229001 _doCalculation ...");
				result = _doCalculation( lstr, rstr, ltmode, rtmode, ltype, rtype, llength, rlength, first );
                dn("s222229001 _doCalculation result=%d", result );

				if ( result < 0 ) {
					_opString = ""; _numCnts = _initK = _stddevSum = _stddevSumSqr = 0;
				} else result = 1;
			}

			if ( ltmode > rtmode ) typeMode = ltmode;
			else typeMode = rtmode;

			if ( ltype.size() > 0 ) {
				type = ltype;
				length = llength;
				str = _opString = lstr;
			} else {
				type = rtype;
				length = rlength;
				str = _opString = lstr;  
			}

		} else {
            dn("s482503 isAggregateOp [%s] true",  binaryOpStr(_binaryOp).s() );

			str = _opString;
			typeMode = 2;
		}
	}
	
	return result;
}

int BinaryOpNode::checkFuncValidConstantOnly( JagFixString &str, int &typeMode, Jstr &type, int &length )
{
	bool first = 0;
	JagFixString lstr, rstr;
	Jstr ltype, rtype;
	int leftVal = 1, rightVal = 1, ltmode = 0, rtmode = 0, llength = 0, rlength = 0, result = 0;

	if ( _left ) {
        dn("12028 _left");
		leftVal = _left->checkFuncValidConstantOnly( lstr, ltmode, ltype, llength );
	}

	if ( _right ) {
        dn("12028 _right");
		rightVal = _right->checkFuncValidConstantOnly( rstr, rtmode, rtype, rlength );
	}

    dn("s347120 checkFuncValidConstantOnly lstr=[%s] rstr=[%s]", lstr.s(), rstr.s() );

	if ( leftVal < 0 || rightVal < 0 ) {
        dn("s767501 leftVal < 0 || rightVal < 0 result=-1");
        result = -1;
	} else {	
        dn("s170004 checkFuncValidConstantOnly _binaryOp=[%s]", binaryOpStr(_binaryOp).s() );

		if ( !isAggregateOp( _binaryOp ) ) {
            dn("s482003 not isAggregateOp [%s]",  binaryOpStr(_binaryOp).s() );

			if ( lstr.length() < 1 && rstr.length() < 1 ) {
                dn("s32875011 lstr rstr all empty return result=%d", result );
                return result;
            }

			if ( JAG_LOGIC_OR == _binaryOp ) {
				result = leftVal || rightVal;
			} else if ( JAG_LOGIC_AND == _binaryOp ) {
				result = leftVal && rightVal;
			} else {
                dn("s222229001 _doCalculation ...");
				result = _doCalculation( lstr, rstr, ltmode, rtmode, ltype, rtype, llength, rlength, first );
                dn("s222229001 _doCalculation result=%d", result );

				if ( result < 0 ) {
					_opString = ""; _numCnts = _initK = _stddevSum = _stddevSumSqr = 0;
				} else result = 1;
			}

			if ( ltmode > rtmode ) typeMode = ltmode;
			else typeMode = rtmode;

			if ( ltype.size() > 0 ) {
				type = ltype;
				length = llength;
				str = _opString = lstr;
			} else {
				type = rtype;
				length = rlength;
				str = _opString = lstr;  
			}

		} else {
            dn("s482503 isAggregateOp [%s] true",  binaryOpStr(_binaryOp).s() );

			str = _opString;
			typeMode = 2;
		}
	}
	
	return result;
}

void BinaryOpNode::findOrBuffer( JagMinMax *minmax, JagMinMax *leftbuf, JagMinMax *rightbuf, const int keylen[], int numTabs )
{
	for ( int i = 0; i < numTabs; ++i ) {
		if ( memcmp(leftbuf[i].minbuf, rightbuf[i].minbuf, keylen[i]) <= 0 ) {
			memcpy(minmax[i].minbuf, leftbuf[i].minbuf, keylen[i]);
		} else {
			memcpy(minmax[i].minbuf, rightbuf[i].minbuf, keylen[i]);				
		} 
		if ( memcmp(leftbuf[i].maxbuf, rightbuf[i].maxbuf, keylen[i]) >= 0 ) {
			memcpy(minmax[i].maxbuf, leftbuf[i].maxbuf, keylen[i]);			
		} else {
			memcpy(minmax[i].maxbuf, rightbuf[i].maxbuf, keylen[i]);				
		}
	}
}

void BinaryOpNode::findAndBuffer( JagMinMax *minmax, JagMinMax *leftbuf, JagMinMax *rightbuf, 
	const JagSchemaAttribute *attrs[], int numTabs, const int numKeys[] )
{
	jagint offset;
	for ( int i = 0; i < numTabs; ++i ) {
		offset = 0;
		for ( int j = 0; j < numKeys[i]; ++j ) {
			if ( memcmp(leftbuf[i].minbuf+offset, rightbuf[i].minbuf+offset, attrs[i][j].length) >= 0 ) {
				memcpy(minmax[i].minbuf+offset, leftbuf[i].minbuf+offset, attrs[i][j].length);
			} else {
				memcpy(minmax[i].minbuf+offset, rightbuf[i].minbuf+offset, attrs[i][j].length);				
			} 
			if ( memcmp(leftbuf[i].maxbuf+offset, rightbuf[i].maxbuf+offset, attrs[i][j].length) <= 0 ) {
				memcpy(minmax[i].maxbuf+offset, leftbuf[i].maxbuf+offset, attrs[i][j].length);				
			} else {
				memcpy(minmax[i].maxbuf+offset, rightbuf[i].maxbuf+offset, attrs[i][j].length);				
			} 
			offset += attrs[i][j].length;
		}
	}	
}

void BinaryOpNode::findLeftBuffer( JagMinMax *minmax, JagMinMax *leftbuf, JagMinMax *rightbuf, 
	const JagSchemaAttribute *attrs[], int numTabs, const int numKeys[] )
{
	jagint offset;
	for ( int i = 0; i < numTabs; ++i ) {
		offset = 0;
		for ( int j = 0; j < numKeys[i]; ++j ) {
			memcpy(minmax[i].minbuf+offset, leftbuf[i].minbuf+offset, attrs[i][j].length);
			memcpy(minmax[i].maxbuf+offset, leftbuf[i].maxbuf+offset, attrs[i][j].length);				
			offset += attrs[i][j].length;
		}
	}	
}

bool BinaryOpNode::formatColumnData( JagMinMax *minmax, const JagMinMax *iminmax, 
								    const JagFixString &value, int tabnum, int minOrMax )
{	
    dn("s2233801 formatColumnData value=[%s]", value.s() );

	if ( minmax[tabnum].buflen <= iminmax[tabnum].offset ) { 
		return true; 
	}

	char *buf = minmax[tabnum].minbuf;
	if ( 2 == minOrMax ) {
		buf = minmax[tabnum].maxbuf;
	}

	Jstr errmsg;
	formatOneCol( _jpa.timediff, _jpa.servtimediff, buf, value.c_str(), errmsg, "GAR",
				  iminmax[tabnum].offset, iminmax[tabnum].length, iminmax[tabnum].sig, 
				  iminmax[tabnum].type );

	dbNaturalFormatExchange( buf, 0, NULL, iminmax[tabnum].offset, iminmax[tabnum].length, 
							 iminmax[tabnum].type ); 

	if ( 0 == minOrMax ) {
		if ( iminmax[tabnum].offset+iminmax[tabnum].length > minmax[tabnum].buflen ) {
		} else {
			memcpy( minmax[tabnum].maxbuf+iminmax[tabnum].offset, buf+iminmax[tabnum].offset, iminmax[tabnum].length);
		}
	}

	return true;
}

bool BinaryOpNode::checkAggregateValid( int lcmode, int rcmode, bool laggr, bool raggr )
{

	if ( _right ) { 
		if ( (2 == lcmode && 1 == rcmode) || (1 == lcmode && 2 == rcmode) ) {
			return 0; 
		}
	} else { 
		if ( 2 == lcmode && isAggregateOp(_binaryOp) ) {
			return 0; 
		}
	}
	return 1;
}

int BinaryOpNode::formatAggregateParts( Jstr &parts, Jstr &lparts, Jstr &rparts )
{
	if ( _binaryOp == JAG_FUNC_EQUAL ) {
		parts = Jstr("(") + lparts + ")=(" + rparts + ")";
	}
	else if ( _binaryOp == JAG_FUNC_NOTEQUAL ) {
		parts = Jstr("(") + lparts + ")!=(" + rparts + ")";
	}
	else if ( _binaryOp == JAG_FUNC_LESSTHAN ) {
		parts = Jstr("(") + lparts + ")<(" + rparts + ")";
	} else if ( _binaryOp == JAG_FUNC_LESSEQUAL ) {
		parts = Jstr("(") + lparts + ")<=(" + rparts + ")";	
	}
	// > || >=
	else if ( _binaryOp == JAG_FUNC_GREATERTHAN ) {
		parts = Jstr("(") + lparts + ")>(" + rparts + ")";	
	} else if ( _binaryOp == JAG_FUNC_GREATEREQUAL ) {
		parts = Jstr("(") + lparts + ")>=(" + rparts + ")";		
	}
	// like
	else if ( _binaryOp == JAG_FUNC_LIKE ) {
		parts = Jstr("(") + lparts + ") like (" + rparts + ")";
	}
	// match
	else if ( _binaryOp == JAG_FUNC_MATCH ) {
		parts = Jstr("(") + lparts + ") match (" + rparts + ")";
	}
	// +
	else if ( _binaryOp == JAG_NUM_ADD ) {
		parts = Jstr("(") + lparts + ")+(" + rparts + ")";
	}	
	// - 
	else if ( _binaryOp == JAG_NUM_SUB ) {
		parts = Jstr("(") + lparts + ")-(" + rparts + ")";
	}	
	// *
	else if ( _binaryOp == JAG_NUM_MULT ) {
		parts = Jstr("(") + lparts + ")*(" + rparts + ")";	
	}	
	// /
	else if ( _binaryOp == JAG_NUM_DIV ) {
		parts = Jstr("(") + lparts + ")/(" + rparts + ")";
	}
	// %
	else if ( _binaryOp == JAG_NUM_REM ) {
		parts = Jstr("(") + lparts + ")%(" + rparts + ")";
	}	
	// ^
	else if ( _binaryOp == JAG_NUM_POW || _binaryOp == JAG_FUNC_POW ) {
		parts = Jstr("(") + lparts + ")^(" + rparts + ")";
	}
	// MOD
	else if ( _binaryOp == JAG_FUNC_MOD ) {
		parts = Jstr("mod(") + lparts + "," + rparts + ")";
	}
	// LENGTH
	else if ( _binaryOp == JAG_FUNC_LENGTH ) {
		parts = Jstr("length(") + lparts + ")";
	}
	// ABS
	else if ( _binaryOp == JAG_FUNC_ABS ) {
		parts = Jstr("abs(") + lparts + ")";
	}
	// ACOS
	else if ( _binaryOp == JAG_FUNC_ACOS ) {
		parts = Jstr("acos(") + lparts + ")";
	}	
	// ASIN
	else if ( _binaryOp == JAG_FUNC_ASIN ) {
		parts = Jstr("asin(") + lparts + ")";
	}	
	// ATAN
	else if ( _binaryOp == JAG_FUNC_ATAN ) {
		parts = Jstr("atan(") + lparts + ")";
	}
	// COS
	else if ( _binaryOp == JAG_FUNC_COS ) {
		parts = Jstr("cos(") + lparts + ")";
	}		
	// METERTOMILE
	else if ( _binaryOp == JAG_FUNC_METERTOMILE ) {
		parts = Jstr("metertomile(") + lparts + ")";
	}		
	// KILOMETERTOMILE
	else if ( _binaryOp == JAG_FUNC_KILOMETERTOMILE ) {
		parts = Jstr("kilometertomile(") + lparts + ")";
	}		
	// MILETOMETER
	else if ( _binaryOp == JAG_FUNC_MILETOMETER ) {
		parts = Jstr("miletometer(") + lparts + ")";
	}		
	// MILETOKILOMETER
	else if ( _binaryOp == JAG_FUNC_MILETOKILOMETER ) {
		parts = Jstr("miletokilometer(") + lparts + ")";
	}		
	// DIFF
	else if ( _binaryOp == JAG_FUNC_STRDIFF ) {
		parts = Jstr("diff(") + lparts + "," + rparts + ")";
	}		
	// SIN
	else if ( _binaryOp == JAG_FUNC_SIN ) {
		parts = Jstr("sin(") + lparts + ")";		
	}
	// TAN
	else if ( _binaryOp == JAG_FUNC_TAN ) {
		parts = Jstr("tan(") + lparts + ")";
	}	
	// COT
	else if ( _binaryOp == JAG_FUNC_COT ) {
		parts = Jstr("cot(") + lparts + ")";
	}	
	// ALL
	else if ( _binaryOp == JAG_FUNC_ALL ) {
		parts = Jstr("all(") + lparts + ")";
	}	
	// LOG2
	else if ( _binaryOp == JAG_FUNC_LOG2 ) {
		parts = Jstr("log2(") + lparts + ")";
	}	
	// LOG10
	else if ( _binaryOp == JAG_FUNC_LOG10 ) {
		parts = Jstr("log10(") + lparts + ")";
	}	
	// LOG
	else if ( _binaryOp == JAG_FUNC_LOG ) {
		parts = Jstr("ln(") + lparts + ")";
	}
	// DEGREE
	else if ( _binaryOp == JAG_FUNC_DEGREES ) {
		parts = Jstr("degrees(") + lparts + ")";
	}
	// RADIAN
	else if ( _binaryOp == JAG_FUNC_RADIANS ) {
		parts = Jstr("radians(") + lparts + ")";
	}
	// SQRT
	else if ( _binaryOp == JAG_FUNC_SQRT ) {
		parts = Jstr("sqrt(") + lparts + ")";
	}
	// CEIL
	else if ( _binaryOp == JAG_FUNC_CEIL ) {
		parts = Jstr("ceil(") + lparts + ")";
	}	
	// FLOOR
	else if ( _binaryOp == JAG_FUNC_FLOOR ) {
		parts = Jstr("floor(") + lparts + ")";
	}
	// UPPER
	else if ( _binaryOp == JAG_FUNC_UPPER ) {
		parts = Jstr("upper(") + lparts + ")";
	}	
	// LOWER
	else if ( _binaryOp == JAG_FUNC_LOWER ) {
		parts = Jstr("lower(") + lparts + ")";
	}	
	// LTRIM
	else if ( _binaryOp == JAG_FUNC_LTRIM ) {
		parts = Jstr("ltrim(") + lparts + ")";
	}	
	// RTRIM
	else if ( _binaryOp == JAG_FUNC_RTRIM ) {
		parts = Jstr("rtrim(") + lparts + ")";
	}	
	// TRIM
	else if ( _binaryOp == JAG_FUNC_TRIM ) {
		parts = Jstr("trim(") + lparts + ")";
	}
	// date, time, datetime etc
	else if ( _binaryOp == JAG_FUNC_SECOND ) {
		parts = Jstr("second(") + lparts + ")";
	}
	else if ( _binaryOp == JAG_FUNC_MINUTE ) {
		parts = Jstr("minute(") + lparts + ")";
	}
	else if ( _binaryOp == JAG_FUNC_HOUR ) {
		parts = Jstr("hour(") + lparts + ")";
	}
	else if ( _binaryOp == JAG_FUNC_DAY ) {
		parts = Jstr("day(") + lparts + ")";
	}
	else if ( _binaryOp == JAG_FUNC_MONTH ) {
		parts = Jstr("month(") + lparts + ")";
	}
	else if ( _binaryOp == JAG_FUNC_YEAR ) {
		parts = Jstr("year(") + lparts + ")";
	}
	else if ( _binaryOp == JAG_FUNC_DATE ) {
		parts = Jstr("date(") + lparts + ")";
	}
	else if ( _binaryOp == JAG_FUNC_DATEDIFF ) {
		parts = Jstr("datediff(");
		if ( *(_carg1.c_str()) == 's' ) parts += "second,";
		else if ( *(_carg1.c_str()) == 'm' ) parts += "minute,";
		else if ( *(_carg1.c_str()) == 'h' ) parts += "hour,";
		else if ( *(_carg1.c_str()) == 'D' ) parts += "day,";
		else if ( *(_carg1.c_str()) == 'M' ) parts += "month,";
		else if ( *(_carg1.c_str()) == 'Y' ) parts += "year,";
		parts += lparts + "," + rparts + ")";
	}
	else if ( _binaryOp == JAG_FUNC_DAYOFMONTH ) {
		parts = Jstr("dayofmonth(") + lparts + ")";
		
	}
	else if ( _binaryOp == JAG_FUNC_DAYOFWEEK ) {
		parts = Jstr("dayofweek(") + lparts + ")";
	}
	else if ( _binaryOp == JAG_FUNC_DAYOFYEAR ) {
		parts = Jstr("dayofyear(") + lparts + ")";
	}
	// SUBSTR
	else if ( _binaryOp == JAG_FUNC_SUBSTR ) {
		parts = Jstr("substr(") + lparts + "," + intToStr(_arg1) + "," + intToStr(_arg2);
		if ( _carg1.length() > 0 ) {
			// has encoding
			parts += Jstr(",") + _carg1;
		}
		parts += ")";
	}
	// CURDATE, CURTIME, NOW with no child funcs
	else if ( _binaryOp == JAG_FUNC_CURDATE ) {
		parts = "curdate()";
	} else if ( _binaryOp == JAG_FUNC_CURTIME ) {
		parts = "curtime()";
	} else if ( _binaryOp == JAG_FUNC_NOW ) {
		parts = "now()";
	} else if ( _binaryOp == JAG_FUNC_TIME ) {
		parts = "time()";
	} else if ( _binaryOp == JAG_FUNC_DISTANCE ) {
		parts = Jstr("distance(") + lparts + "," + rparts + ", " + _carg1 + ")";
	} else if ( _binaryOp == JAG_FUNC_SIMILARITY ) {
		parts = Jstr("similarity(") + lparts + "," + rparts + ", " + _carg1 + ")";
	} else if ( _binaryOp == JAG_FUNC_WITHIN ) {
		parts = Jstr("within(") + lparts + "," + rparts + ")";
	} else if ( _binaryOp == JAG_FUNC_COVEREDBY ) {
		parts = Jstr("coveredby(") + lparts + "," + rparts + ")";
	} else if ( _binaryOp == JAG_FUNC_COVER ) {
		parts = Jstr("cover(") + lparts + "," + rparts + ")";
	} else if ( _binaryOp == JAG_FUNC_CONTAIN ) {
		parts = Jstr("contain(") + lparts + "," + rparts + ")";
	} else if ( _binaryOp == JAG_FUNC_INTERSECT ) {
		parts = Jstr("intersect(") + lparts + "," + rparts + ")";
	} else if ( _binaryOp == JAG_FUNC_DISJOINT ) {
		parts = Jstr("disjoint(") + lparts + "," + rparts + ")";
	} else if ( _binaryOp == JAG_FUNC_NEARBY ) {
		parts = Jstr("nearby(") + lparts + "," + rparts + ", " + _carg1 + ")";
	} else if ( _binaryOp == JAG_FUNC_AREA ) {
		parts = Jstr("area(") + lparts + ")";
	} else if ( _binaryOp == JAG_FUNC_DIMENSION ) {
		parts = Jstr("dimension(") + lparts + ")";
	} else if ( _binaryOp == JAG_FUNC_GEOTYPE ) {
		parts = Jstr("geotype(") + lparts + ")";
	/***
	} else if ( _binaryOp == JAG_FUNC_CLOSESTPOINT ) {
		parts = Jstr("closestpoint(") + lparts + "," + rparts + ")";
	***/
	}
	// ... more calcuations ...
	return 1;
}  // end of formatAggregateParts()


int BinaryOpNode::_doWhereCalc( const JagHashStrInt *maps[], const JagSchemaAttribute *attrs[], 
						const int keylen[], const int numKeys[], int numTabs, int ltmode, int rtmode, 
						int ltabnum, int rtabnum,
						JagMinMax *minmax, JagMinMax *lminmax, JagMinMax *rminmax, 
						JagFixString &str, const JagFixString &lstr,  const Jstr &ltype,
                        const JagFixString &rstr, const Jstr &rtype )
{
    dn("s108902 _doWhereCalc() _type=[%s] lstr=[%s] rstr=[%s]", _type.s(), lstr.s(), rstr.s() );

	int coltype, cmode;
	if ( lstr.length() < 1 && rstr.length() < 1 ) {
		return 0; 
	}

	if ( lstr.length() < 1 && rstr.length() > 0 && ltabnum >= 0 ) { 
		coltype = 1; 
	} else if ( lstr.length() > 0 && rstr.length() < 1 && rtabnum >= 0 ) {
		coltype = 2; 
	} else {
		coltype = 0; 
	}

	if ( ltmode > rtmode ) cmode = ltmode;
	else cmode = rtmode;

	if ( _binaryOp == JAG_FUNC_EQUAL ) {
		if ( 1 == coltype ) {
			if ( formatColumnData( minmax, lminmax, rstr, ltabnum, 0 ) ) {
				str = JagFixString(minmax[ltabnum].minbuf, keylen[ltabnum], keylen[ltabnum]);
			} else {
				str="1";
			}
			return 1;
		} else if ( 2 == coltype ) {
			if ( formatColumnData( minmax, rminmax, lstr, rtabnum, 0 ) ) {
				str = JagFixString(minmax[rtabnum].minbuf, keylen[rtabnum], keylen[rtabnum]);
			} else {
				str="1";
			}
			return 1;
		} else {
			if ( 2 == cmode && jagstrtold(lstr.c_str() ) != jagstrtold(rstr.c_str() ) ) {
				return 0; 
			} else if ( 1 == cmode && jagatoll(lstr.c_str()) != jagatoll(rstr.c_str()) ) {
				return 0; 
			} else if ( strcmp(lstr.c_str(), rstr.c_str()) != 0 ) {
				return 0; 
			}

			str = "1";
			return 1;			
		}
	}
	// != <> ><
	else if ( _binaryOp == JAG_FUNC_NOTEQUAL ) {
		if ( 0 == coltype ) {
			if ( 2 == cmode && jagstrtold(lstr.c_str()) == jagstrtold(rstr.c_str()) ) return 0; 
			else if ( 1 == cmode && jagatoll(lstr.c_str()) == jagatoll(rstr.c_str()) ) return 0; 
			else if ( strcmp(lstr.c_str(), rstr.c_str()) == 0 ) return 0; 
			str = "1";
			return 1;
		}
	}
	// < || <=
	else if ( _binaryOp == JAG_FUNC_LESSTHAN || _binaryOp == JAG_FUNC_LESSEQUAL ) {
        dn("s340040 _doWhereCalc JAG_FUNC_LESSTHAN JAG_FUNC_LESSEQUAL coltype=%d", coltype);
		if ( 1 == coltype ) {
			if ( formatColumnData( minmax, lminmax, rstr, ltabnum, 2 ) ) {
				str = JagFixString(minmax[ltabnum].maxbuf, keylen[ltabnum], keylen[ltabnum] );
			} else {
				str = "1";
			}
			return 1;
		} else if ( 2 == coltype ) {
			if ( formatColumnData( minmax, rminmax, lstr, rtabnum, 1 ) ) {
				str = JagFixString(minmax[rtabnum].minbuf, keylen[rtabnum], keylen[rtabnum]);
			} else {
				str = "1";
			}
			return 1;
		} else {
			if ( _binaryOp == JAG_FUNC_LESSTHAN ) {
                dn("s8282901 cmode=%d lstr=[%s] JAG_FUNC_LESSTHAN rstr=[%s]", cmode, lstr.s(), rstr.s() );
				if ( 2 == cmode && jagstrtold(lstr.c_str() ) >= jagstrtold(rstr.c_str() ) ) {
					return 0; 
				} else if ( 1 == cmode && jagatoll(lstr.c_str()) >= jagatoll(rstr.c_str()) ) {
					return 0; 
				} else if ( strcmp(lstr.c_str(), rstr.c_str()) >= 0 ) {
					return 0; 
				}
			} else if ( _binaryOp == JAG_FUNC_LESSEQUAL ) {
                dn("s8282902 cmode=%d lstr=[%s] JAG_FUNC_LESSEQUAL rstr=[%s]", cmode, lstr.s(), rstr.s() );
				if ( 2 == cmode && jagstrtold(lstr.c_str() ) > jagstrtold(rstr.c_str() ) ) {
					return 0; 
				} else if ( 1 == cmode && jagatoll(lstr.c_str()) > jagatoll(rstr.c_str()) ) {
					return 0; 
				} else if ( strcmp(lstr.c_str(), rstr.c_str()) > 0 ) {
					return 0; 
				}
			} else {
			}
			str = "1";
			return 1;
		}
	}
	// > || >=
	else if ( _binaryOp == JAG_FUNC_GREATERTHAN || _binaryOp == JAG_FUNC_GREATEREQUAL ) {
        dn("s340041 _doWhereCalc JAG_FUNC_GREATERTHAN JAG_FUNC_GREATEREQUAL coltype=%d", coltype );
		if ( 1 == coltype ) {
			formatColumnData( minmax, lminmax, rstr, ltabnum, 1  );
			str = JagFixString(minmax[ltabnum].minbuf, keylen[ltabnum], keylen[ltabnum]);
			return 1;
		} else if ( 2 == coltype ) {
			formatColumnData( minmax, rminmax, lstr, rtabnum, 2  );
			str = JagFixString(minmax[rtabnum].maxbuf, keylen[rtabnum], keylen[rtabnum]);
			return 1;
		} else {
			if ( _binaryOp == JAG_FUNC_GREATERTHAN ) {
                dn("s822902 cmode=%d lstr=[%s] JAG_FUNC_GREATERTHAN rstr=[%s]", cmode, lstr.s(), rstr.s() );
				if ( 2 == cmode && jagstrtold(lstr.c_str()) <= jagstrtold(rstr.c_str() ) ) return 0; 
				else if ( 1 == cmode && jagatoll(lstr.c_str()) <= jagatoll(rstr.c_str()) ) return 0; 
				else if ( strcmp(lstr.c_str(), rstr.c_str()) <= 0 ) return 0; 
			} else if ( _binaryOp == JAG_FUNC_GREATEREQUAL ) {
                dn("s822932 cmode=%d lstr=[%s] JAG_FUNC_GREATEREQUAL rstr=[%s]", cmode, lstr.s(), rstr.s() );
				if ( 2 == cmode && jagstrtold(lstr.c_str()) < jagstrtold(rstr.c_str() ) ) return 0; 
				else if ( 1 == cmode && jagatoll(lstr.c_str()) < jagatoll(rstr.c_str()) ) return 0; 
				else if ( strcmp(lstr.c_str(), rstr.c_str()) < 0 ) return 0; 
			}
			str = "1";
			return 1;
		}
	}
	// like
	else if ( _binaryOp == JAG_FUNC_LIKE ) {
		if ( 1 == coltype ) {
			char *pfirst = (char*)rstr.c_str();
			char *plast = strrchr( (char*)rstr.c_str(), '%');
			if ( *pfirst != '%' && plast ) {
				char onebyte = 255;
				char minbuf[plast-pfirst+1];
				char maxbuf[plast-pfirst+1];
				minbuf[plast-pfirst] = '\0';
				maxbuf[plast-pfirst] = '\0';
				memcpy(minbuf, pfirst, plast-pfirst);
				memcpy(maxbuf, pfirst, plast-pfirst);
				int counter = 1;
				bool out_of_bound = false;
				while ( true ) {
					if ( maxbuf[plast-pfirst-counter] != onebyte ) {
						maxbuf[plast-pfirst-counter] = maxbuf[plast-pfirst-counter] + 1;
						break;
					} else {
						maxbuf[plast-pfirst-counter] = '\0';
						++counter;
					}
					if ( plast-pfirst-counter < 0 ) {
						out_of_bound = true;
						break;
					}
				}
				memcpy(minmax[ltabnum].minbuf+lminmax[ltabnum].offset, rstr.c_str(), plast-pfirst);				
				if ( !out_of_bound ) {
					memcpy(minmax[ltabnum].maxbuf+lminmax[ltabnum].offset, rstr.c_str(), plast-pfirst);
				}
			}
			str = JagFixString(minmax[ltabnum].minbuf, keylen[ltabnum], keylen[ltabnum]);
			return 1;
		} else if ( 2 == coltype ) {
			// must be col like constant or constant like constant
			return 0;
		} else {
			if ( 2 == cmode && jagstrtold(lstr.c_str()) != jagstrtold(rstr.c_str() ) ) return 0; 
			else if ( 1 == cmode && jagatoll(lstr.c_str()) != jagatoll(rstr.c_str()) ) return 0; 
			else if ( strcmp(lstr.c_str(), rstr.c_str()) != 0 ) return 0; 
			str = "1";
			return 1;
		}
	}
	// match
	else if ( _binaryOp == JAG_FUNC_MATCH ) {
		if ( NULL == _reg ) {
			try {
			    _reg = new std::regex( rstr.c_str() );
			} catch ( std::regex_error e ) {
				str = "0";
				return 0;
			} catch ( ... ) {
				str = "0";
				return 0;
			}
		}
		if ( regex_match( lstr.c_str(), *_reg ) ) {
			str = "1";
			return 1;
		} else {
			str = "0";
			return 0;
		}
	}
	// AND &&
	else if ( _binaryOp == JAG_LOGIC_AND ) {
		findAndBuffer( minmax, lminmax, rminmax, attrs, numTabs, numKeys );
		str = "1";
		return 1;
	}
	// OR ||
	else if ( _binaryOp == JAG_LOGIC_OR ) {
		findOrBuffer( minmax, lminmax, rminmax, keylen, numTabs );
		str = "1";
		return 1;
	}
	// '(' 
	else if ( _binaryOp == JAG_LEFT_BRA   ) {
		findLeftBuffer( minmax, lminmax, rminmax, attrs, numTabs, numKeys );
		str = "1";
		return 1;
	}
	
	if ( coltype > 0 ) {
		return 0;
	}
	jagint num = 0; abaxdouble dnum = 0.0;
	
	// +
	if ( _binaryOp == JAG_NUM_ADD ) {
		if ( 2 == cmode ) {
			dnum = jagstrtold(lstr.c_str() ) + jagstrtold(rstr.c_str());
			str = longDoubleToStr(dnum);
		} else if ( 1 == cmode ) {
			num = jagatoll(lstr.c_str()) + jagatoll(rstr.c_str());
			str = longToStr(num);
		} else {
			num = jagatoll(lstr.c_str()) + jagatoll(rstr.c_str());
			str = longToStr(num);
		}
		return 1;
	}	
	// - 
	else if ( _binaryOp == JAG_NUM_SUB ) {
		str = d2s( jagatof(lstr.c_str()) - jagatof(rstr.c_str()) ); 
		return 1;
	}	
	// *
	else if ( _binaryOp == JAG_NUM_MULT ) {
		str = d2s( jagatof(lstr.c_str()) * jagatof(rstr.c_str()) ); 
		return 1;
	}	
	// /
	else if ( _binaryOp == JAG_NUM_DIV ) {
		if ( jagEQ(jagatof(rstr.c_str()), 0.0) ) { str=""; return -1;}
		str = d2s( jagatof(lstr.c_str()) / jagatof(rstr.c_str()) ); 
		return 1;
	}
	// %
	else if ( _binaryOp == JAG_NUM_REM ) {
		if ( jagEQ(jagatof(rstr.c_str()), 0.0) ) { str=""; return -1;}
		num = jagatoll(lstr.c_str()) / jagatoll(rstr.c_str());
		str = longToStr(num);
		return 1;
	}	
	// ^
	else if ( _binaryOp == JAG_NUM_POW || _binaryOp == JAG_FUNC_POW ) {
		if ( 2 == cmode ) {
			dnum = pow(jagstrtold(lstr.c_str() ), jagstrtold(rstr.c_str() ));
			str = longDoubleToStr(dnum);
		} else if ( 1 == cmode ) {
			num = pow(jagatoll(lstr.c_str()), jagatoll(rstr.c_str()));
			str = longToStr(num);
		} else {
			num = pow(jagatoll(lstr.c_str()), jagatoll(rstr.c_str()));
			str = longToStr(num);
		}
		return 1;
	}
	// MOD
	else if ( _binaryOp == JAG_FUNC_MOD ) {
		if ( jagEQ(jagatof(rstr.c_str()), 0.0) ) { str=""; return -1;}
		dnum = fmod(jagstrtold(lstr.c_str() ), jagstrtold(rstr.c_str() ));
		str = longDoubleToStr(dnum);
		return 1;
	}
	// LENGTH
	else if ( _binaryOp == JAG_FUNC_LENGTH ) {
		if ( 2 == cmode ) {
			str = longDoubleToStr(jagstrtold(lstr.c_str()));
		} else if ( 1 == cmode ) {
			str = longToStr(jagatoll(lstr.c_str()));
		} else {
			str = longToStr(lstr.size());
		}
		return 1;
	}
	// ABS
	else if ( _binaryOp == JAG_FUNC_ABS ) {
		if ( 1 == cmode ) {
			str = longToStr(labs(jagatoll(lstr.c_str())));
		} else {
			str = longDoubleToStr(fabs(jagstrtold(lstr.c_str())));
		}
		return 1;
	}
	// ACOS
	else if ( _binaryOp == JAG_FUNC_ACOS ) {
		long double a = jagstrtold(lstr.s());
        if ( a < -1.0 || a > 1.0 ) {
            str = "0";
        } else {
		    str = longDoubleToStr(acosl(a));
        }
        in("s30303910   JAG_FUNC_ACOS lstr=[%s]  jagstrtold=%Lf acos=%Lf str=[%s]", lstr.s(), jagstrtold(lstr.s()), acosl(jagstrtold(lstr.s())), str.s() );
		return 1;
	}	
	// ASIN
	else if ( _binaryOp == JAG_FUNC_ASIN ) {
		long double a  = jagstrtold(lstr.s());
        if ( a < -1.0 || a > 1.0 ) {
            str = "0";
        } else {
		    str = longDoubleToStr(asinl(a));
        }

		return 1;
	}	
	// ATAN
	else if ( _binaryOp == JAG_FUNC_ATAN ) {
		str = longDoubleToStr(atanl(jagstrtold(lstr.c_str() )));
		return 1;
	}
	// DIFF
	else if ( _binaryOp == JAG_FUNC_STRDIFF ) {
		int diff = levenshtein( lstr.c_str(), rstr.c_str() );
		str = intToStr( diff );
		return 1;
	}		
	// COS
	else if ( _binaryOp == JAG_FUNC_COS ) {
		str = longDoubleToStr(cos(jagstrtold(lstr.c_str() )));
		return 1;
	}		
	// METERTOMILE
	else if ( _binaryOp == JAG_FUNC_METERTOMILE ) {
		str = longDoubleToStr( jagstrtold(lstr.c_str() )/1609.344);
		return 1;
	}		
	// KILOMETERTOMILE
	else if ( _binaryOp == JAG_FUNC_KILOMETERTOMILE ) {
		str = longDoubleToStr( jagstrtold(lstr.c_str() )/1.609344);
		return 1;
	}		
	// MILETOMETER
	else if ( _binaryOp == JAG_FUNC_MILETOMETER ) {
		str = longDoubleToStr( jagstrtold(lstr.c_str() )*1609.344);
		return 1;
	}		
	// MILETOKILOMETER
	else if ( _binaryOp == JAG_FUNC_MILETOKILOMETER ) {
		str = longDoubleToStr( jagstrtold(lstr.c_str() )*1.609344);
		return 1;
	}		
	// SIN
	else if ( _binaryOp == JAG_FUNC_SIN ) {
        dn("s039320890 JAG_FUNC_SIN lstr=[%s] lstrval=%Lf sin=%Lf", lstr.s(), jagstrtold(lstr.s()), sin(jagstrtold(lstr.s())) );
		str = longDoubleToStr(sin(jagstrtold(lstr.c_str())));
        dn("s233088 str=[%s]", str.s() );
		return 1;		
	}
	// TAN
	else if ( _binaryOp == JAG_FUNC_TAN ) {
		str = longDoubleToStr(tanl(jagstrtold(lstr.c_str())));
		return 1;
	}	
	// COT
	else if ( _binaryOp == JAG_FUNC_COT ) {
        long double t = tanl(jagstrtold(lstr.c_str() ));
        if ( fabs(t) < 0.0000000001 ) return -1;
		str = longDoubleToStr(1.0/t);
		return 1;
	}	
	// ALL
	else if ( _binaryOp == JAG_FUNC_ALL ) {
		str = "";
		return 1;
	}
	// LOG2
	else if ( _binaryOp == JAG_FUNC_LOG2 ) {
		str = longDoubleToStr(log2(jagstrtold(lstr.c_str() )));
		return 1;
	}	
	// LOG10
	else if ( _binaryOp == JAG_FUNC_LOG10 ) {
		str = longDoubleToStr(log10(jagstrtold(lstr.c_str() )));
		return 1;
	}	
	// LOG
	else if ( _binaryOp == JAG_FUNC_LOG ) {
		str = longDoubleToStr(log(jagstrtold(lstr.c_str() )));
		return 1;
	}
	// DEGREE
	else if ( _binaryOp == JAG_FUNC_DEGREES ) {
		str = longDoubleToStr(jagstrtold(lstr.c_str() )*57.295779513);
		return 1;
	}
	// RADIAN
	else if ( _binaryOp == JAG_FUNC_RADIANS ) {
		str = longDoubleToStr(jagstrtold(lstr.c_str() )/57.295779513);
		return 1;
	}
	// SQRT
	else if ( _binaryOp == JAG_FUNC_SQRT ) {
		str = longDoubleToStr(sqrt(jagstrtold(lstr.c_str() )));
		return 1;
	}
	// CEIL
	else if ( _binaryOp == JAG_FUNC_CEIL ) {
		if ( 1 == cmode ) {
			str = longToStr(ceil(jagatoll(lstr.c_str())));
		} else {
			str = longDoubleToStr(ceil(jagstrtold(lstr.c_str() )));
		}		
		return 1;
	}	
	// FLOOR
	else if ( _binaryOp == JAG_FUNC_FLOOR ) {
		if ( 1 == cmode ) {
			str = longToStr(floor(jagatoll(lstr.c_str())));
		} else {
			str = longDoubleToStr(floor(jagstrtold(lstr.c_str() )));
		}		
		return 1;
	}	
	// UPPER
	else if ( _binaryOp == JAG_FUNC_UPPER ) {
		str = makeUpperOrLowerFixString( lstr, 1 );
		return 1;
	}	
	// LOWER
	else if ( _binaryOp == JAG_FUNC_LOWER ) {
		str = makeUpperOrLowerFixString( lstr, 0 );
		return 1;
	}	
	// LTRIM
	else if ( _binaryOp == JAG_FUNC_LTRIM ) {
		str = lstr;
		str.ltrim();
		return 1;
	}	
	// RTRIM
	else if ( _binaryOp == JAG_FUNC_RTRIM ) {
		str = lstr;
		str.rtrim();
		return 1;
	}	
	// TRIM
	else if ( _binaryOp == JAG_FUNC_TRIM ) {
		str = lstr;
		str.trim();
		return 1;
	}
	// date, time, datetime etc
	else if ( isTimedateOp(_binaryOp) ) {
		dn("s300391 getValueFromTimeOrDate ..");
		str = JagTime::getValueFromTimeOrDate( _jpa, lstr, ltype, rstr, rtype, _binaryOp, _carg1 );
		return 1;
	}
	// SUBSTR
	else if ( _binaryOp == JAG_FUNC_SUBSTR ) {
		str = lstr;
		if ( _carg1.size() < 1 ) {
			str.substr( _arg1-1, _arg2 );
		} else {
			JagStrSplit sp( _carg1, '.' );
			Jstr encode = sp[sp.length()-1];
			JagLang lang;
			jagint n = lang.parse( str.c_str(), encode.c_str() );
			if ( n < 0 ) {
				str.substr( _arg1-1, _arg2 );
			} else {
				lang.rangeFixString( str.size(), _arg1-1, _arg2, str );
			}
		}
		return 1;
	}
	// TOSECOND
	else if ( _binaryOp == JAG_FUNC_TOSECOND ) {
		str = _carg1;
		return 1;
	}
	// TOMICROSECOND
	else if ( _binaryOp == JAG_FUNC_TOMICROSECOND ) {
		str = _carg1;
		return 1;
	}
	// CURDATE, CURTIME, NOW with no child funcs
	else if ( _binaryOp == JAG_FUNC_CURDATE || _binaryOp == JAG_FUNC_CURTIME || _binaryOp == JAG_FUNC_NOW ) {
		short  collen = getFuncLength( _binaryOp );
		struct tm res; 
        time_t now = time(NULL);

		now -= (_jpa.servtimediff - _jpa.timediff) * 60;
		jag_localtime_r( &now, &res );

		char *buf = (char*) jagmalloc ( collen+1 );

		memset( buf, 0,  collen+1 );

		if ( _binaryOp == JAG_FUNC_CURDATE ) {
			strftime( buf, collen+1, "%Y-%m-%d", &res );
		} else if ( _binaryOp == JAG_FUNC_CURTIME ) {
			strftime( buf, collen+1, "%H:%M:%S", &res );
		} else if ( _binaryOp == JAG_FUNC_NOW ) {
			strftime( buf, collen+1, "%Y-%m-%d %H:%M:%S", &res );
		}
		str = JagFixString( buf, collen, collen );
		if ( buf ) free ( buf );
		return 1;
	} else if ( _binaryOp == JAG_FUNC_TIME ) {
		char buf[32];
		sprintf( buf, "%ld", time(NULL) );
		str = JagFixString( buf );
		return 1;
	} else if ( _binaryOp == JAG_FUNC_PI ) {
		char buf[32];
		sprintf( buf, "%f", JAG_PI );
		str = JagFixString( buf );
		return 1;
	}
	// DISTANCE
	else if ( _binaryOp == JAG_FUNC_DISTANCE ) {
		str = "1";
		return 1;
	}
	// SIMILARITY
	else if ( _binaryOp == JAG_FUNC_SIMILARITY ) {
		str = "1";
		return 1;
	}
	// ... more calcuations ...
	
	return -1;
}  // end of _doWhereCalc

int BinaryOpNode::_doCalculation( JagFixString &lstr, JagFixString &rstr, 
	int &ltmode, int &rtmode,  const Jstr& ltype,  const Jstr& rtype, 
	int llength, int rlength, bool &first )
{
    dn("s00827371 _doCalculation lstr=[%s] ltype=[%s]", lstr.s(), ltype.s() );
    dn("s00827371 _doCalculation  rstr=[%s] rtype=[%s]", rstr.s(), rtype.s() );

	int cmode;
	if ( ltmode > rtmode ) cmode = ltmode;
	else cmode = rtmode;

	if ( _right && rstr.size()>0 && 0 != strncmp(rstr.c_str(), JAG_CJAG, JAG_CJAG_LEN) ) {
        dn("s5660928");
	    Jstr errmsg;
		if ( isDateTime(ltype) && 0 == rtype.size() && !strchr(lstr.c_str(), '-') ) {
            dn("s2008120 isDateTime(ltype)  0 == rtype.size() lstr is digits");

            if ( ltype == JAG_C_COL_TYPE_DATETIMEMICRO || ltype == JAG_C_COL_TYPE_TIMESTAMPMICRO ) {
                JagParseAttribute jpa( NULL,  _jpa.timediff, _jpa.servtimediff );
                long tnum = JagTime::getNumDateTime( jpa, rstr.s(), JAG_TIME_SECOND_MICRO );
                char tbuf[32];
                sprintf(tbuf, "%ld", tnum );
			    rstr = JagFixString( tbuf );
            } else if ( ltype == JAG_C_COL_TYPE_DATETIMENANO || ltype == JAG_C_COL_TYPE_TIMESTAMPNANO ) {
                JagParseAttribute jpa( NULL,  _jpa.timediff, _jpa.servtimediff );
                long tnum = JagTime::getNumDateTime( jpa, rstr.s(), JAG_TIME_SECOND_NANO );
                char tbuf[32];
                sprintf(tbuf, "%ld", tnum );
			    rstr = JagFixString( tbuf );
            } else if ( ltype == JAG_C_COL_TYPE_DATETIMEMILLI || ltype == JAG_C_COL_TYPE_TIMESTAMPMILLI ) {
                JagParseAttribute jpa( NULL,  _jpa.timediff, _jpa.servtimediff );
                long tnum = JagTime::getNumDateTime( jpa, rstr.s(), JAG_TIME_SECOND_MILLI );
                char tbuf[16];
                sprintf(tbuf, "%ld", tnum );
			    rstr = JagFixString( tbuf );
            } else if ( ltype == JAG_C_COL_TYPE_DATETIMESEC || ltype == JAG_C_COL_TYPE_TIMESTAMPSEC ) {
                JagParseAttribute jpa( NULL,  _jpa.timediff, _jpa.servtimediff );
                long tnum = JagTime::getNumDateTime( jpa, rstr.s(), JAG_TIME_SECOND );
                char tbuf[16];
                sprintf(tbuf, "%ld", tnum );
			    rstr = JagFixString( tbuf );
            } else if ( ltype == JAG_C_COL_TYPE_TIMEMICRO ) {
                long tnum = JagTime::getNumTime( rstr.s(), JAG_TIME_SECOND_MICRO );
                char tbuf[18];
                sprintf(tbuf, "%ld", tnum );
			    rstr = JagFixString( tbuf );
            } else if ( ltype == JAG_C_COL_TYPE_TIMENANO ) {
                long tnum = JagTime::getNumTime( rstr.s(), JAG_TIME_SECOND_NANO );
                char tbuf[24];
                sprintf(tbuf, "%ld", tnum );
			    rstr = JagFixString( tbuf );
            } else if ( ltype == JAG_C_COL_TYPE_DATE ) {
                long tnum = JagTime::getNumDate( rstr.s() );
                char tbuf[16];
                sprintf(tbuf, "%ld", tnum );
			    rstr = JagFixString( tbuf );
            } else {
                dn("s345003 sdfg");
			    char buf[llength+1];
			    memset( buf, 0, llength+1 );
			    formatOneCol( _jpa.timediff, _jpa.servtimediff, buf, rstr.c_str(), errmsg, "GAR", 0, llength, 0, ltype );
			    rstr = JagFixString( buf, llength, llength );
                dn("s038891 rstr=[%s] ltype=[%s]", rstr.s(), ltype.s() );
            }
		} else if ( isDateTime(rtype) && 0 == ltype.size() && !strchr( rstr.c_str(), '-') ) {
            dn("s2008122 isDateTime(rtype)  0 == ltype.size() rstr is digits");

            if ( rtype == JAG_C_COL_TYPE_DATETIMEMICRO || rtype == JAG_C_COL_TYPE_TIMESTAMPMICRO ) {
                JagParseAttribute jpa( NULL,  _jpa.timediff, _jpa.servtimediff );
                long tnum = JagTime::getNumDateTime( jpa, lstr.s(), JAG_TIME_SECOND_MICRO );
                char tbuf[32];
                sprintf(tbuf, "%ld", tnum );
			    lstr = JagFixString( tbuf );
            } else if ( rtype == JAG_C_COL_TYPE_DATETIMENANO || rtype == JAG_C_COL_TYPE_TIMESTAMPNANO ) {
                JagParseAttribute jpa( NULL,  _jpa.timediff, _jpa.servtimediff );
                long tnum = JagTime::getNumDateTime( jpa, lstr.s(), JAG_TIME_SECOND_NANO );
                char tbuf[32];
                sprintf(tbuf, "%ld", tnum );
			    lstr = JagFixString( tbuf );
            } else if ( rtype == JAG_C_COL_TYPE_DATETIMEMILLI || rtype == JAG_C_COL_TYPE_TIMESTAMPMILLI ) {
                JagParseAttribute jpa( NULL,  _jpa.timediff, _jpa.servtimediff );
                long tnum = JagTime::getNumDateTime( jpa, lstr.s(), JAG_TIME_SECOND_MILLI );
                char tbuf[16];
                sprintf(tbuf, "%ld", tnum );
			    lstr = JagFixString( tbuf );
            } else if ( rtype == JAG_C_COL_TYPE_DATETIMESEC || rtype == JAG_C_COL_TYPE_TIMESTAMPSEC ) {
                JagParseAttribute jpa( NULL,  _jpa.timediff, _jpa.servtimediff );
                long tnum = JagTime::getNumDateTime( jpa, lstr.s(), JAG_TIME_SECOND );
                char tbuf[16];
                sprintf(tbuf, "%ld", tnum );
			    lstr = JagFixString( tbuf );
            } else if ( rtype == JAG_C_COL_TYPE_TIMEMICRO ) {
                JagParseAttribute jpa( NULL,  _jpa.timediff, _jpa.servtimediff );
                long tnum = JagTime::getNumTime( lstr.s(), JAG_TIME_SECOND_MICRO );
                char tbuf[18];
                sprintf(tbuf, "%ld", tnum );
			    lstr = JagFixString( tbuf );
            } else if ( rtype == JAG_C_COL_TYPE_TIMENANO ) {
                JagParseAttribute jpa( NULL,  _jpa.timediff, _jpa.servtimediff );
                long tnum = JagTime::getNumTime( lstr.s(), JAG_TIME_SECOND_NANO );
                char tbuf[24];
                sprintf(tbuf, "%ld", tnum );
			    lstr = JagFixString( tbuf );
            } else if ( rtype == JAG_C_COL_TYPE_DATE ) {
                long tnum = JagTime::getNumDate( lstr.s() );
                char tbuf[16];
                sprintf(tbuf, "%ld", tnum );
			    lstr = JagFixString( tbuf );
            } else {
                // true lstr
                dn("s4540028 lstr ");
			    char buf[rlength+1];
			    memset( buf, 0, rlength+1 );
			    formatOneCol( _jpa.timediff, _jpa.servtimediff, buf, lstr.c_str(), errmsg, "GAR", 0, rlength, 0, rtype );
			    lstr = JagFixString( buf, rlength, rlength );
                dn("s5902339 lstr=[%s]", lstr.s() );
            }
		} else {
            dn("s2022020 lstr=[%s]", lstr.c_str() );
        }
	} else {
        dn("s2022947 lstr=[%s]", lstr.c_str() );
    }

	const char *pleft = lstr.c_str();
	const char *pright = rstr.c_str();

    dn("s230053 _doCalculation  lstr=[%s]", pleft );
    dn("s230053 _doCalculation  rstr=[%s]", pright );
    dn("s230053 _doCalculation  _binaryOp=[%s]", BinaryOpNode::binaryOpStr(_binaryOp).s() );

	// MIN
	if ( _binaryOp == JAG_FUNC_MIN ) {
		if ( 0 == cmode ) {
			if ( first ) {
				return 1;
			} else if ( memcmp(lstr.c_str(), _opString.c_str(), _opString.length()) < 0 ) {
				return 1;
			} else {
				// no error, ignore
				lstr = _opString;
				return 1;
			}
		} else if ( 1 == cmode ) {
			if ( first ) {
				lstr = longToStr(jagatoll(lstr.c_str()));
				return 1;
			} else if ( jagatoll(lstr.c_str()) < jagatoll(_opString.c_str()) ) {
				lstr = longToStr(jagatoll(lstr.c_str()));
				return 1;
			} else {
				// no error, ignore
				lstr = _opString;
				return 1;
			}			
		} else {			
			if ( first ) {
				lstr = longDoubleToStr(jagstrtold(lstr.c_str() ));
				return 1;
			} else if ( jagstrtold(lstr.c_str() ) < jagstrtold(_opString.c_str() ) ) {
				lstr = longDoubleToStr(jagstrtold(lstr.c_str() ));
				return 1;
			} else {
				lstr = _opString;
				return 1;
			}
		}
	}	
	// MAX
	else if ( _binaryOp == JAG_FUNC_MAX ) {
		if ( 0 == cmode ) {
			if ( first ) {
				return 1;
			} else if ( memcmp(lstr.c_str(), _opString.c_str(), _opString.length()) > 0 ) {
				return 1;
			} else {
				lstr = _opString;
				return 1;
			}
		} else if ( 1 == cmode ) {
			if ( first ) {
				lstr = longToStr(jagatoll(lstr.c_str()));
				return 1;
			} else if ( jagatoll(lstr.c_str()) > jagatoll(_opString.c_str()) ) {
				lstr = longToStr(jagatoll(lstr.c_str()));
				return 1;
			} else {
				lstr = _opString;
				return 1;
			}			
		} else {			
			if ( first ) {
				lstr = longDoubleToStr(jagstrtold(lstr.c_str(), NULL));
				return 1;
			} else if ( jagstrtold(lstr.c_str(), NULL) > jagstrtold(_opString.c_str(), NULL) ) {
				lstr = longDoubleToStr(jagstrtold(lstr.c_str(), NULL));
				return 1;
			} else {
				lstr = _opString;
				return 1;
			}
		}		
	}	
	// AVG
	else if ( _binaryOp == JAG_FUNC_AVG ) {
		++_numCnts;
		abaxdouble avg = jagstrtold(_opString.c_str(), NULL);
		avg += (jagstrtold(lstr.c_str(), NULL)-avg)*(1.0/_numCnts);
		lstr = longDoubleToStr(avg);
		ltmode = 2;
		return 1;
	}	
	// SUM
	else if ( _binaryOp == JAG_FUNC_SUM ) {
		if ( 1 == cmode ) {
			lstr = longToStr(jagatoll(lstr.c_str())+jagatoll(_opString.c_str()));
            dn("s20191231 cmode==1 lstr=[%s]", lstr.s() );
			return 1;
		} else {
            dn("s028281180 JAG_FUNC_SUM ltr=[%s] _opString=[%s]", lstr.c_str(), _opString.c_str() );
			lstr = longDoubleToStr(jagstrtold(lstr.c_str(), NULL) + jagstrtold(_opString.c_str(), NULL));
            dn("s028281182 JAG_FUNC_SUM sum ltr=[%s]", lstr.c_str() );
			return 1;
		}
	}
	// COUNT
	else if ( _binaryOp == JAG_FUNC_COUNT ) {
		lstr = longToStr(jagatoll(_opString.c_str())+1);
		return 1;
	}
	// STDDEV
	else if ( _binaryOp == JAG_FUNC_STDDEV ) {
		++_numCnts;
		abaxdouble stdev = jagstrtold(lstr.c_str(), NULL);
		if ( first ) {
			_initK = stdev;
			lstr = "0";
		} else {
			_stddevSum += stdev-_initK;
			_stddevSumSqr += (stdev-_initK)*(stdev-_initK);
			lstr = longDoubleToStr(sqrt((_stddevSumSqr-(_stddevSum*_stddevSum)/_numCnts)/_numCnts));
		}
		ltmode = 2;
		return 1;
	}	
	// FIRST
	else if ( _binaryOp == JAG_FUNC_FIRST ) {
		if ( first ) {
		} else {
			lstr = _opString;
		}
		return 1;
	}	
	// LAST
	else if ( _binaryOp == JAG_FUNC_LAST ) {
		return 1;
	}
	
	
	// non aggregate funcs
	// = ==
	if ( _binaryOp == JAG_FUNC_EQUAL ) {
		if ( isDateTime(ltype) || isDateTime(rtype) ) {
			while (*pleft == '0' ) ++pleft;
			while (*pright == '0' ) ++pright;
		}

		if ((0 == cmode && strcmp(pleft, pright ) == 0) || 
			(1 == cmode && jagatoll(pleft) == jagatoll(pright) ) ||
			(2 == cmode && jagstrtold( pleft, NULL) == jagstrtold(pright, NULL))) {
			lstr = "1";
			return 1;
		} else {
			lstr = "0";
			return 0;
		}
	}
	// != <> ><
	else if ( _binaryOp == JAG_FUNC_NOTEQUAL ) {
		if ( isDateTime(ltype) || isDateTime(rtype) ) {
			while (*pleft == '0' ) ++pleft;
			while (*pright == '0' ) ++pright;
		}
		if ((0 == cmode && strcmp(pleft, pright) != 0) ||
			(1 == cmode && jagatoll(pleft) != jagatoll(pright )) ||
			(2 == cmode && jagstrtold(pleft, NULL) != jagstrtold(pright, NULL))) {
			lstr = "1";
			return 1;
		} else {
			lstr = "0";
			return 0;
		}
	}
	// < || <=
	else if ( _binaryOp == JAG_FUNC_LESSTHAN ) {
		if ( isDateTime(ltype) || isDateTime(rtype) ) {
			while (*pleft == '0' ) ++pleft;
			while (*pright == '0' ) ++pright;
		}

        dn("s234008 JAG_FUNC_LESSTHAN cmode=%d pleft=%s  pright=%s", cmode, pleft, pright);

		if ((0 == cmode && strcmp(pleft, pright) < 0) ||
			(1 == cmode && jagatoll( pleft ) < jagatoll( pright )) ||
			(2 == cmode && jagstrtold(pleft, NULL) < jagstrtold( pright, NULL))) {
            dn("s32208 1");
			lstr = "1";
			return 1;
		} else {
            dn("s32208 0");
			lstr = "0";
			return 0;
		}
	} else if ( _binaryOp == JAG_FUNC_LESSEQUAL ) {
		if ( isDateTime(ltype) || isDateTime(rtype) ) {
			while (*pleft == '0' ) ++pleft;
			while (*pright == '0' ) ++pright;
		}

        dn("s234058 JAG_FUNC_LESSEQUAL cmode=%d", cmode);

		if ((0 == cmode && strcmp(pleft,pright) <= 0) ||
			(1 == cmode && jagatoll(pleft) <= jagatoll(pright)) ||
			(2 == cmode && jagstrtold(pleft, NULL) <= jagstrtold(pright, NULL))) {
			lstr = "1";
			return 1;
		} else {
			lstr = "0";
			return 0;
		}	
	}
	// > || >=
	else if ( _binaryOp == JAG_FUNC_GREATERTHAN ) {
		if ( isDateTime(ltype) || isDateTime(rtype) ) {
			while (*pleft == '0' ) ++pleft;
			while (*pright == '0' ) ++pright;
		}
        dn("s214058 JAG_FUNC_GREATERTHAN cmode=%d", cmode );

		if ((0 == cmode && strcmp(pleft,pright)  > 0) ||
			(1 == cmode && jagatoll(pleft) > jagatoll(pright)) ||
			(2 == cmode && jagstrtold(pleft, NULL) > jagstrtold(pright, NULL))) {
			lstr = "1";
			return 1;
		} else {
			lstr = "0";
			return 0;
		}	
	} else if ( _binaryOp == JAG_FUNC_GREATEREQUAL ) {
		if ( isDateTime(ltype) || isDateTime(rtype) ) {
			while (*pleft == '0' ) ++pleft;
			while (*pright == '0' ) ++pright;
		}

        // debug
        /**
        dn("s214758 JAG_FUNC_GREATEREQUAL, cmode=%d pleft=[%s] pright=[%s]", cmode, pleft, pright );
        if ( 1 == cmode ) {
            dn("s34801 jagatoll(pleft)=%ld  jagatoll(pright)=%ld", jagatoll(pleft), jagatoll(pright) );
        }
        **/
        // end debug

		if ((0 == cmode && strcmp(pleft, pright ) >= 0) ||
			(1 == cmode && jagatoll(pleft) >= jagatoll(pright)) ||
			(2 == cmode && jagstrtold(pleft, NULL) >= jagstrtold(pright, NULL))) {
			lstr = "1";
			return 1;
		} else {
			lstr = "0";
			return 0;
		}		
	}
	// like
	else if ( _binaryOp == JAG_FUNC_LIKE ) {
		if ( cmode > 0 || ltype != JAG_C_COL_TYPE_STR ) {
			lstr = "";
			return -1;
		}

		char *pfirst = (char*)rstr.c_str();
		char *plast = strrchr( (char*)rstr.c_str(), '%');
		if ( rtype.size() > 0 || !plast ) {
			if ( strcmp(lstr.c_str(), rstr.c_str()) == 0 ) {
				lstr = "1";
				return 1;
			} else {
				lstr = "0";
				return 0;
			}
		} else {
			if ( *pfirst != '%' && memcmp(lstr.c_str(), pfirst, plast-pfirst) == 0 ) {
				lstr = "1";
				return 1;
			} else if ( *pfirst == '%' && plast != pfirst ) {
				*plast = '\0';
				if ( strstr(lstr.c_str(), pfirst+1) ) {
					*plast = '%';
					lstr = "1";
					return 1;
				} else {
					*plast = '%';
					lstr = "0";
					return 0;
				}
			} else if ( *pfirst == '%' && plast == pfirst ) {
				if ( lastStrEqual(lstr.c_str(), pfirst+1, lstr.length(), strlen(pfirst+1)) ) {
					lstr = "1";
					return 1;
				} else {
					lstr = "0";
					return 0;
				}
			} else {
				lstr = "";
				return -1;
			}
		}
	}
	// match
	else if ( _binaryOp == JAG_FUNC_MATCH ) {
		if ( NULL == _reg ) {
			try {
			    _reg = new std::regex( rstr.c_str() );
			} catch ( std::regex_error e ) {
				lstr = "0";
				return 0;
			} catch ( ... ) {
				lstr = "0";
				return 0;
			}
		}
		if ( regex_match( lstr.c_str(), *_reg ) ) {
			lstr = "1";
			return 1;
		} else {
			lstr = "0";
			return 0;
		}
	}
	// +
	else if ( _binaryOp == JAG_NUM_ADD ) {
		if ( 2 == cmode ) {
			lstr = longDoubleToStr(jagstrtold(lstr.c_str(), NULL)+jagstrtold(rstr.c_str(), NULL));
			return 1;
		} else if ( 1 == cmode ) {		
			lstr = longToStr(jagatoll(lstr.c_str())+jagatoll(rstr.c_str()));
			return 1;
		} else {
			lstr = d2s( jagatof( lstr.s() ) + jagatof( rstr.s() ));
			return 1;
		}
	}
	else if ( _binaryOp == JAG_STR_ADD ) {
		lstr = lstr.concat(rstr);
		return 1;
	}	
	// - 
	else if ( _binaryOp == JAG_NUM_SUB ) {
		if ( 1 == cmode ) {
			lstr = longToStr(jagatoll(lstr.c_str())-jagatoll(rstr.c_str()));
			return 1;
		} else {			
			lstr = longDoubleToStr(jagstrtold(lstr.c_str(), NULL)-jagstrtold(rstr.c_str(), NULL));
			return 1;
		}
	}	
	// *
	else if ( _binaryOp == JAG_NUM_MULT ) {
		if ( 1 == cmode ) {
			lstr = longToStr(jagatoll(lstr.c_str())*jagatoll(rstr.c_str()));
			return 1;
		} else {
			lstr = longDoubleToStr(jagstrtold(lstr.c_str(), NULL)*jagstrtold(rstr.c_str(), NULL));
			return 1;
		}	
	}	
	// /
	else if ( _binaryOp == JAG_NUM_DIV ) {
		if ( 1 == cmode ) {
			if ( jagatoll(rstr.c_str()) == 0 ) {
				lstr = "";
				return -1;
			} else {
				lstr = longToStr(jagatoll(lstr.c_str())/jagatoll(rstr.c_str()));
				return 1;
			}
		} else {
			if ( jagstrtold(rstr.c_str(), NULL) == 0 ) {
				lstr = "";
				return -1;
			} else {
				lstr = longDoubleToStr(jagstrtold(lstr.c_str(), NULL)/jagstrtold(rstr.c_str(), NULL));
				return 1;
			}
		}
	}
	// %
	else if ( _binaryOp == JAG_NUM_REM ) {
		if ( jagatoll(rstr.c_str()) == 0 ) {
			lstr = "";
			return -1;
		} else {
			lstr = longToStr(jagatoll(lstr.c_str())%atoll(rstr.c_str()));
			return 1;
		}
	}	
	// ^
	else if ( _binaryOp == JAG_NUM_POW || _binaryOp == JAG_FUNC_POW ) {
		if ( 1 == cmode ) {
			lstr = longToStr(pow(jagatoll(lstr.c_str()), jagatoll(rstr.c_str())));
			return 1;
		} else {
			lstr = longDoubleToStr(pow(jagstrtold(lstr.c_str(), NULL), jagstrtold(rstr.c_str(), NULL)));
			return 1;
		}
	}
	// MOD
	else if ( _binaryOp == JAG_FUNC_MOD ) {
		lstr = longDoubleToStr(fmod(jagstrtold(lstr.c_str(), NULL), jagstrtold(rstr.c_str(), NULL)));
		ltmode = 2;
		return 1;
	}
	// LENGTH
	else if ( _binaryOp == JAG_FUNC_LENGTH ) {
    	if ( 2 == cmode ) {
    		lstr = longDoubleToStr(jagstrtold(lstr.c_str(), NULL));
    	} else if ( 1 == cmode ) {
    		lstr = longToStr(jagatoll(lstr.c_str()));
    	} 
    	lstr = longToStr( strlen(lstr.c_str()) );
		return 1;
	}
	// LINELENGTH
	else if ( _binaryOp == JAG_FUNC_LINELENGTH ) {
		#ifdef JAG_SERVER_SIDE
			lstr = doubleToStr(JagGeo::getGeoLength( lstr ));
		#else
			lstr = "0";
		#endif
		return 1;
	}
	// ABS
	else if ( _binaryOp == JAG_FUNC_ABS ) {
		if ( 1 == cmode ) {
			lstr = longToStr(labs(jagatoll(lstr.s())));
		} else {
			lstr = longDoubleToStr(fabs(jagstrtold(lstr.s())));
		}
		return 1;
	}
	// ACOS
	else if ( _binaryOp == JAG_FUNC_ACOS ) {
		long double a  = jagstrtold(lstr.s());
        if ( a < -1.0 || a > 1.0 ) {
            lstr = "0";
        } else {
		    lstr = longDoubleToStr(acosl(a));
        }
        in("s30303920   JAG_FUNC_ACOS lstr=[%s]  jagstrtold=%Lf acos=%Lf lstr=[%s]", lstr.s(), jagstrtold(lstr.s()), acosl(jagstrtold(lstr.s())), lstr.s() );
		ltmode = 2;
		return 1;
	}	
	// ASIN
	else if ( _binaryOp == JAG_FUNC_ASIN ) {
		long double a  = jagstrtold(lstr.s());
        if ( a < -1.0 || a > 1.0 ) {
            lstr = "0";
        } else {
		    lstr = longDoubleToStr(asinl(a));
        }

		ltmode = 2;
		return 1;
	}	
	// ATAN
	else if ( _binaryOp == JAG_FUNC_ATAN ) {
		lstr = longDoubleToStr(atanl(jagstrtold(lstr.s(), NULL)));
		ltmode = 2;
		return 1;
	}
	// DIFF
	else if ( _binaryOp == JAG_FUNC_STRDIFF ) {
		ltmode = 1;
		int diff = levenshtein( lstr.c_str(), rstr.c_str() );
		lstr = intToStr( diff );
		return 1;
	}		
	// COS
	else if ( _binaryOp == JAG_FUNC_COS ) {
		lstr = longDoubleToStr(cos(jagstrtold(lstr.c_str(), NULL)));
		ltmode = 2;
		return 1;
	}
	// METERTOMILE
	else if ( _binaryOp == JAG_FUNC_METERTOMILE ) {
		lstr = longDoubleToStr( jagstrtold(lstr.c_str(), NULL)/1609.344);
		return 1;
	}		
	// KILOMETERTOMILE
	else if ( _binaryOp == JAG_FUNC_KILOMETERTOMILE ) {
		lstr = longDoubleToStr( jagstrtold(lstr.c_str(), NULL)/1.609344);
		return 1;
	}
	// MILETOMETER
	else if ( _binaryOp == JAG_FUNC_MILETOMETER ) {
		lstr = longDoubleToStr( jagstrtold(lstr.c_str(), NULL)*1609.344);
		return 1;
	}		
	// MILETOKILOMETER
	else if ( _binaryOp == JAG_FUNC_MILETOKILOMETER ) {
		lstr = longDoubleToStr( jagstrtold(lstr.c_str(), NULL)*1.609344);
		return 1;
	}		
	// SIN
	else if ( _binaryOp == JAG_FUNC_SIN ) {
    dn("s450922001 JAG_FUNC_SIN lstr=[%s] sin=%Lf", lstr.s(), sin(jagstrtold(lstr.s())) );
		lstr = longDoubleToStr(sin(jagstrtold(lstr.s())));
        dn("s1889003 new lstr=[%s]", lstr.s() );
		ltmode = 2;
		return 1;		
	}
	// TAN
	else if ( _binaryOp == JAG_FUNC_TAN ) {
		lstr = longDoubleToStr(tanl(jagstrtold(lstr.c_str())));
		ltmode = 2;
		return 1;
	}	
	// COT
	else if ( _binaryOp == JAG_FUNC_COT ) {
        long double t = tanl(jagstrtold(lstr.c_str() ));
        if ( fabs(t) < 0.0000000001 ) {
			lstr = "";
			return -1;
        }

		lstr = longDoubleToStr(1.0/t);
		ltmode = 2;
		return 1;
	}	
	// ALL
	else if ( _binaryOp == JAG_FUNC_ALL ) {
		ltmode = 0;
		if ( lstr.size() > 0 ) {
			return 1;
		} else {
			return 0;
		}
	}
	// LOG2
	else if ( _binaryOp == JAG_FUNC_LOG2 ) {
		lstr = longDoubleToStr(log2(jagstrtold(lstr.c_str(), NULL)));
		ltmode = 2;
		return 1;
	}	
	// LOG10
	else if ( _binaryOp == JAG_FUNC_LOG10 ) {
		lstr = longDoubleToStr(log10(jagstrtold(lstr.c_str(), NULL)));
		ltmode = 2;
		return 1;
	}	
	// LOG
	else if ( _binaryOp == JAG_FUNC_LOG ) {
		lstr = longDoubleToStr(log(jagstrtold(lstr.c_str(), NULL)));
		ltmode = 2;
		return 1;
	}
	// DEGREE
	else if ( _binaryOp == JAG_FUNC_DEGREES ) {
		lstr = longDoubleToStr(jagstrtold(lstr.c_str(), NULL)*57.295779513);
		ltmode = 2;
		return 1;
	}
	// RADIAN
	else if ( _binaryOp == JAG_FUNC_RADIANS ) {
		lstr = longDoubleToStr(jagstrtold(lstr.c_str(), NULL)/57.295779513);
		ltmode = 2;
		return 1;
	}
	// SQRT
	else if ( _binaryOp == JAG_FUNC_SQRT ) {
		lstr = longDoubleToStr(sqrt(jagstrtold(lstr.c_str(), NULL)));
		ltmode = 2;
		return 1;
	}
	// CEIL
	else if ( _binaryOp == JAG_FUNC_CEIL ) {
		if ( 1 == cmode ) {
			lstr = longToStr(ceil(jagatoll(lstr.c_str())));
		} else {
			lstr = longDoubleToStr(ceil(jagstrtold(lstr.c_str(), NULL)));
		}		
		return 1;
	}	
	// FLOOR
	else if ( _binaryOp == JAG_FUNC_FLOOR ) {
		if ( 1 == cmode ) {
			lstr = longToStr(floor(jagatoll(lstr.c_str())));
		} else {
			lstr = longDoubleToStr(floor(jagstrtold(lstr.c_str(), NULL)));
		}		
		return 1;
	}	
	// UPPER
	else if ( _binaryOp == JAG_FUNC_UPPER ) {
		lstr = makeUpperOrLowerFixString( lstr, 1 );
		return 1;
	}	
	// LOWER
	else if ( _binaryOp == JAG_FUNC_LOWER ) {
		lstr = makeUpperOrLowerFixString( lstr, 0 );
		return 1;
	}	
	// LTRIM
	else if ( _binaryOp == JAG_FUNC_LTRIM ) {
		lstr.ltrim();
		return 1;
	}	
	// RTRIM
	else if ( _binaryOp == JAG_FUNC_RTRIM ) {
		lstr.rtrim();
		return 1;
	}	
	// TRIM
	else if ( _binaryOp == JAG_FUNC_TRIM ) {
		lstr.trim();
		return 1;
	}
	// date, time, datetime etc
	else if ( isTimedateOp(_binaryOp) ) {
		dn("s330331 getValueFromTimeOrDate ..");
		lstr = JagTime::getValueFromTimeOrDate( _jpa, lstr, ltype, rstr, rtype, _binaryOp, _carg1 );
		dn("s330331 getValueFromTimeOrDate lstr=[%s]", lstr.s() );
		return 1;
	}
	// SUBSTR
	else if ( _binaryOp == JAG_FUNC_SUBSTR ) {
		if ( _carg1.size() < 1 ) {
			lstr.substr( _arg1-1, _arg2 );
		} else {
			JagStrSplit sp( _carg1, '.' );
			Jstr encode = sp[sp.length()-1];
			JagLang lang;
			jagint n = lang.parse( lstr.c_str(), encode.c_str() );
			if ( n < 0 ) {
				lstr.substr( _arg1-1, _arg2 );
			} else {
				lang.rangeFixString( lstr.size(), _arg1-1, _arg2, lstr );
			}
		}
		return 1;
	} else if ( _binaryOp == JAG_FUNC_TOSECOND ) {
		lstr = _carg1;
		return 1;
	} else if ( _binaryOp == JAG_FUNC_TOMICROSECOND  ) {
		lstr = _carg1;
		return 1;
    }
	// CURDATE, CURTIME, NOW with no child funcs
	else if ( _binaryOp == JAG_FUNC_CURDATE || _binaryOp == JAG_FUNC_CURTIME || _binaryOp == JAG_FUNC_NOW ) {
		short collen = getFuncLength( _binaryOp );
		struct tm res; time_t now = time(NULL);

		now -= (_jpa.servtimediff - _jpa.timediff) * 60;
		jag_localtime_r( &now, &res );

		char *buf = (char*) jagmalloc ( collen+1 );
		memset( buf, 0,  collen+1 );

		if ( _binaryOp == JAG_FUNC_CURDATE ) {
			strftime( buf, collen+1, "%Y-%m-%d", &res );
		} else if ( _binaryOp == JAG_FUNC_CURTIME ) {
			strftime( buf, collen+1, "%H:%M:%S", &res );
		} else if ( _binaryOp == JAG_FUNC_NOW ) {
			strftime( buf, collen+1, "%Y-%m-%d %H:%M:%S", &res );
		}
		lstr = JagFixString( buf, collen, collen );
		if ( buf ) free ( buf );
		return 1;
	} else if ( _binaryOp == JAG_FUNC_TIME ) {
		char buf[32];
		sprintf( buf, "%ld", time(NULL) );
		lstr = JagFixString( buf );
		return 1;
	} else if ( _binaryOp == JAG_FUNC_PI ) {
		char buf[32];
		sprintf( buf, "%f", JAG_PI );
		lstr = JagFixString( buf );
		ltmode = 2;
		return 1;
	} else if ( _binaryOp == JAG_FUNC_DISTANCE ) {
        dn("s22250 JAG_FUNC_DISTANCE ...");
		double dist;
		#ifdef JAG_SERVER_SIDE
		if ( ! JagGeo::distance( lstr, rstr, _carg1, dist ) ) {
			return 0;
		}
		#else
			dist = 0.0;
		#endif
		lstr = doubleToStr( dist );
		ltmode = 2;
		return 1;
	} else if ( _binaryOp == JAG_FUNC_SIMILARITY ) {
        dn("s22220 JAG_FUNC_SIMILARITY ...");
		double sim;
		#ifdef JAG_SERVER_SIDE
		if ( ! JagGeo::similarity( lstr, rstr, _carg1, sim ) ) {
			return 0;
		}
		#else
			sim = 0.0;
		#endif
		lstr = doubleToStr( sim );
		ltmode = 2;
		return 1;
	} else if ( _binaryOp == JAG_FUNC_AREA  ||  _binaryOp == JAG_FUNC_VOLUME 
				|| _binaryOp == JAG_FUNC_PERIMETER 
			    || _binaryOp == JAG_FUNC_XMIN || _binaryOp == JAG_FUNC_YMIN || _binaryOp == JAG_FUNC_ZMIN
	            || _binaryOp == JAG_FUNC_XMAX || _binaryOp == JAG_FUNC_YMAX || _binaryOp == JAG_FUNC_ZMAX ) {
		ltmode = 2;
		bool brc = false;
		double val = 0.0;
		try {
			brc = processSingleDoubleOp( _binaryOp, lstr, _carg1, val );
		} catch ( int e ) {
			brc = false;
		} catch ( ... ) {
			brc = false;
		}

		if ( brc ) {
			lstr = doubleToStr( val );
			return 1;
		} else {
			lstr = "0";
			return 0;
		}
	} else if ( _binaryOp == JAG_FUNC_POINTN || _binaryOp == JAG_FUNC_EXTENT || _binaryOp == JAG_FUNC_STARTPOINT
				 || _binaryOp == JAG_FUNC_CONVEXHULL || _binaryOp == JAG_FUNC_BUFFER 
				 || _binaryOp == JAG_FUNC_TOPOLYGON  || _binaryOp == JAG_FUNC_ASTEXT
				 || _binaryOp == JAG_FUNC_TOMULTIPOINT  ||  _binaryOp == JAG_FUNC_CONCAVEHULL
				 || _binaryOp == JAG_FUNC_CENTROID ||  _binaryOp == JAG_FUNC_OUTERRING || _binaryOp == JAG_FUNC_OUTERRINGS
				 || _binaryOp == JAG_FUNC_XMINPOINT ||  _binaryOp == JAG_FUNC_XMAXPOINT
				 || _binaryOp == JAG_FUNC_YMINPOINT ||  _binaryOp == JAG_FUNC_YMAXPOINT
				 || _binaryOp == JAG_FUNC_ZMINPOINT ||  _binaryOp == JAG_FUNC_ZMAXPOINT
				 || _binaryOp == JAG_FUNC_INNERRINGS || _binaryOp == JAG_FUNC_RINGN || _binaryOp == JAG_FUNC_INNERRINGN
	             || _binaryOp == JAG_FUNC_ENDPOINT || _binaryOp == JAG_FUNC_ISCLOSED
	             || _binaryOp == JAG_FUNC_ISSIMPLE  || _binaryOp == JAG_FUNC_ISRING
	             || _binaryOp == JAG_FUNC_ISCONVEX  || _binaryOp == JAG_FUNC_INTERPOLATE
				 || _binaryOp == JAG_FUNC_LINESUBSTRING || _binaryOp == JAG_FUNC_REMOVEPOINT
	             || _binaryOp == JAG_FUNC_ISVALID  || _binaryOp == JAG_FUNC_ISPOLYGONCCW 
				 || _binaryOp == JAG_FUNC_ISPOLYGONCW || _binaryOp == JAG_FUNC_POLYGONN 
				 || _binaryOp == JAG_FUNC_SRID || _binaryOp == JAG_FUNC_SUMMARY || _binaryOp == JAG_FUNC_NUMLINES
				 || _binaryOp == JAG_FUNC_NUMSEGMENTS  || _binaryOp == JAG_FUNC_NUMINNERRINGS
				 || _binaryOp == JAG_FUNC_NUMPOLYGONS || _binaryOp == JAG_FUNC_UNIQUE
				 || _binaryOp == JAG_FUNC_REVERSE || _binaryOp == JAG_FUNC_UNIQUE
				 || _binaryOp == JAG_FUNC_SCALE ||  _binaryOp == JAG_FUNC_SCALESIZE
				 || _binaryOp == JAG_FUNC_TRANSLATE || _binaryOp == JAG_FUNC_TRANSSCALE
				 || _binaryOp == JAG_FUNC_ROTATE || _binaryOp == JAG_FUNC_ROTATEAT || _binaryOp == JAG_FUNC_ROTATESELF
				 || _binaryOp == JAG_FUNC_AFFINE  || _binaryOp == JAG_FUNC_DELAUNAYTRIANGLES 
				 || _binaryOp == JAG_FUNC_GEOJSON ||  _binaryOp == JAG_FUNC_WKT || _binaryOp == JAG_FUNC_METRICN
				 || _binaryOp == JAG_FUNC_MINIMUMBOUNDINGCIRCLE ||  _binaryOp == JAG_FUNC_MINIMUMBOUNDINGSPHERE
				 || _binaryOp == JAG_FUNC_NUMPOINTS || _binaryOp == JAG_FUNC_NUMRINGS ) {
		ltmode = getTypeMode( _binaryOp );

		bool brc = false;
		Jstr val;
		try {
			brc = processSingleStrOp( _binaryOp, lstr, _carg1, val );
		} catch ( int e ) {
			brc = false;
		} catch ( ... ) {
			brc = false;
		}

		if ( brc ) {
			lstr = val;
			return 1;
		} else {
			lstr = "";
			return 0;
		}
	} else if ( _binaryOp == JAG_FUNC_DIMENSION ) {
		ltmode = 1; 
		if (  _left && _left->_isElement ) {
			lstr = intToStr(getDimension( _left->_type ) );
		} else {
			lstr = "0";
		}
		return 1;
	} else if ( _binaryOp == JAG_FUNC_GEOTYPE ) {
		ltmode = 0;
		if (  _left && _left->_isElement ) {
			lstr = getTypeStr( _left->_type );
		} else {
			lstr = "Primitive";
		}
		return 1;
	} else if ( _binaryOp == JAG_FUNC_WITHIN || _binaryOp == JAG_FUNC_COVEREDBY
	            || _binaryOp == JAG_FUNC_CONTAIN || _binaryOp == JAG_FUNC_COVER 
	            || _binaryOp == JAG_FUNC_SAME
	            || _binaryOp == JAG_FUNC_NEARBY 
				|| _binaryOp == JAG_FUNC_INTERSECT || _binaryOp == JAG_FUNC_DISJOINT ) {
		ltmode = 1;
		bool brc;
		try {
            dn("s3540006 processBooleanOp ...");
			brc = processBooleanOp( _binaryOp, lstr, rstr, _carg1 );
		} catch ( int e ) {
			brc = 0;
		} catch ( ... ) {
			brc = 0;
		}

		if ( brc ) {
			lstr = "1";
			return 1;
		} else {
			lstr = "0";
			return 0;
		}
	} else if (  _binaryOp == JAG_FUNC_UNION || _binaryOp == JAG_FUNC_COLLECT ||  _binaryOp == JAG_FUNC_SYMDIFFERENCE
				 || _binaryOp == JAG_FUNC_CLOSESTPOINT ||  _binaryOp == JAG_FUNC_ANGLE
				 || _binaryOp == JAG_FUNC_SCALEAT 
				 || _binaryOp == JAG_FUNC_ISONLEFT  || _binaryOp == JAG_FUNC_LEFTRATIO
				 || _binaryOp == JAG_FUNC_ISONRIGHT || _binaryOp == JAG_FUNC_RIGHTRATIO
				 || _binaryOp == JAG_FUNC_LOCATEPOINT || _binaryOp == JAG_FUNC_ADDPOINT || _binaryOp == JAG_FUNC_SETPOINT
				 || _binaryOp == JAG_FUNC_VORONOIPOLYGONS || _binaryOp == JAG_FUNC_VORONOILINES
				 || _binaryOp == JAG_FUNC_KNN
	             || _binaryOp == JAG_FUNC_INTERSECTION || _binaryOp == JAG_FUNC_DIFFERENCE ) {
		ltmode = getTypeMode( _binaryOp );
		try {
			lstr = processTwoStrOp( _binaryOp, lstr, rstr, _carg1 );
		} catch ( ... ) {
			lstr = "";
		}
		return 1;
	} else if ( JAG_LEFT_BRA == _binaryOp ) {
	}
	// ... more calcuations ...
	
	lstr = "";
	return -1;
}

BinaryExpressionBuilder::BinaryExpressionBuilder()
{
	_isDestroyed = false;
	doneClean = false;
}

BinaryExpressionBuilder::~BinaryExpressionBuilder()
{
	if ( _isDestroyed ) return;
	clean();
	_isDestroyed = true;
}

void BinaryExpressionBuilder::init( const JagParseAttribute &jpa, JagParseParam *pparam )
{
	_jpa = jpa; _datediffClause = _substrClause = -1; _isNot = 0; 
	_lastIsOperand = 0;
	_pparam = pparam;
	doneClean = false;
}

void BinaryExpressionBuilder::clean()
{
	if ( doneClean ) return;

	_jpa.clean();
	operandStack.clean();
	while ( ! operatorStack.empty() ) { operatorStack.pop(); }
	while ( ! operatorArgStack.empty() ) { operatorArgStack.pop(); }
	doneClean = true;
}

ExprElementNode* BinaryExpressionBuilder::getRoot() const
{
	if ( operandStack.size() == 0 ) {
		return NULL;
	}

    ExprElementNode *topp = operandStack.top();
    return topp;
}

BinaryOpNode* BinaryExpressionBuilder::parse( const JagParser *jagParser, const char* str, int type,
		const JagHashMap<AbaxString, AbaxPair<AbaxString, jagint>> &cmap, JagHashStrInt *&jmap, 
		Jstr &colList )
{
	dn("pr0030012 BinaryExpressionBuilder::parse()...");

	if ( 0 == strncasecmp(str, "all(", 4 ) ) {
	} else if (  0 == strncasecmp(str, "geotype(", 8 ) ) {
		// _GEOTYPE
		type = 0;
	} else if (  0 == strncasecmp(str, "pointn(", 7 ) ) {
		// _POINTN
		type = 0;
	} else if (  0 == strncasecmp(str, "extent(", 7 ) || 0 == strncasecmp(str, "envelope(", 9 ) ) {
		type = 0;
	} else if (  0 == strncasecmp(str, "startpoint(", 11 ) ) {
		// _STARTPOINT 
		type = 0;
	} else if (  0 == strncasecmp(str, "endpoint(", 9 )
			  ) {
		// _ENDPOINT  
		type = 0;
	}

    const char *p = str, *q, *m;
	int bcnt;
	short fop = 0, len = 0, isandor = 0, ctype = 0;
	StringElementNode lastNode;
	
	while ( *p != '\0' ) {
		while ( isspace(*p) ) ++p;
		if ( *p == ',' ) {
			if ( _isNot ) throw 2101;
			if ( 1 == type ) {
			}
			++p;
			_lastIsOperand = 0;
		} else if ( *p == '(' ) {
			if ( _isNot ) throw 2100;
			if ( _datediffClause >= 0 || _substrClause >= 0 ) throw 2184;
			bcnt = 1;
			m = p+1;
			while ( *m != '\0' ) {
				if ( *m == '(' ) ++ bcnt;
				else if ( *m == ')' ) -- bcnt;
				if ( 0 == bcnt ) { break; }
				++m;
			}
			Jstr args(p+1, m-p-1);
			JagStrSplitWithQuote spq;
			bcnt = spq.count( args.s(), ',', true );
            operatorStack.push('(');
            operatorArgStack.push(bcnt);
			_lastOp ='(';
            ++p;
			isandor = 0;
			_lastIsOperand = 0;
        } else if ( *p == ')' ) {
			if ( _isNot ) throw 2105;
			if ( isandor ) {
				throw 2136;
			}

            processRightParenthesis( jmap );
            ++p;
			isandor = 0;
			if ( _datediffClause >= 0 ) {
				if ( _datediffClause != 3 ) throw 2090;
				_datediffClause = -1;
			} else if ( _substrClause >= 0 ) {
				if ( _substrClause < 3 || _substrClause > 4 ) throw 2092;
				_substrClause = -1;
			}
			_lastIsOperand = 1;
		} else if ( 0 == strncasecmp( p, "and ", 4 ) ) {
			if ( _isNot ) throw 2227;
			if ( _datediffClause >= 0 || _substrClause >= 0 ) throw 2128;
			if ( !_lastIsOperand ) throw 2180;
			if ( isandor ) { throw 2189; }
			processOperator( JAG_LOGIC_AND, 2, jmap );
			p += 4;
			isandor = 1;
			_lastIsOperand = 0;
		} else if ( 0 == strncasecmp( p, "&& ", 3 ) ) {
			if ( _isNot ) throw 10;
			if ( _datediffClause >= 0 || _substrClause >= 0 ) throw 11;
			if ( !_lastIsOperand ) throw 2181;
			if ( isandor ) {
				throw 2112;
			}
			processOperator( JAG_LOGIC_AND, 2, jmap );
			p += 3;
			isandor = 1;
			_lastIsOperand = 0;
        } else if ( 0 == strncasecmp( p, "or ", 3 ) ) {
			if ( _isNot ) throw 13;
			if ( _datediffClause >= 0 || _substrClause >= 0 ) throw 2114;
			if ( !_lastIsOperand ) throw 2182;
            if ( isandor ) {
                throw 2115;
            }
            processOperator( JAG_LOGIC_OR, 2, jmap );
            p += 3;
            isandor = 1;
			_lastIsOperand = 0;
		} else if ( 0 == strncasecmp( p, "|| ", 3 ) ) {
			if ( _isNot ) throw 16;
			if ( _datediffClause >= 0 || _substrClause >= 0 ) throw 2117;
			if ( !_lastIsOperand ) throw 2183;
			if ( isandor ) {
				throw 2118;
			}
			processOperator( JAG_LOGIC_OR, 2, jmap );
			p += 3;
			isandor = 1;
			_lastIsOperand = 0;
		} else if ( 0 == strncasecmp( p, "between ", 8 ) ) {
			if ( _datediffClause >= 0 || _substrClause >= 0 ) throw 2119;
			if ( !_lastIsOperand ) throw 2184;
			processBetween(jagParser, p, q, lastNode, cmap, jmap, colList );
			_lastIsOperand = 1;
			isandor = 0;
		} else if ( 0 == strncasecmp( p, "in", 2 ) && ( isspace(*(p+2)) || *(p+2) == '(') ) {
			if ( _datediffClause >= 0 || _substrClause >= 0 ) throw 2120;
			if ( !_lastIsOperand ) throw 2185;
			processIn( jagParser, p, q, lastNode, cmap, jmap, colList );
			_lastIsOperand = 1;
			isandor = 0; 
		} else if ( getCalculationType( p, fop, len, ctype ) ) {
			int nargs;
			if ( BinaryOpNode::isMathOp(fop) ) {
				nargs = 2;
			} else {
    			int bcnt = 1;
    			const char *m = p+len+1;
    			while ( *m != '\0' ) {
    				if ( *m == '(' ) ++bcnt; else if ( *m == ')' ) --bcnt;
    				if ( 0 == bcnt ) { break; }
					++m;
    			}
    			Jstr args(p+len, m-p-len);
    			JagStrSplitWithQuote spq;
    			nargs = spq.count( args.s(), ',', true );
			}

			if ( 0 == ctype ) {
				if ( !_lastIsOperand ) {
					if ( isdigit(*(p+1)) ) {
						if ( _isNot ) throw 2188;
						processOperand( jagParser, p, q, lastNode, cmap, colList );
						_lastIsOperand = 1;
						continue;
					}
					throw 2286;
				}
				if ( _isNot ) throw 2221;
				processOperator( *p, nargs, jmap );
			} else if ( 3 == ctype ) {
				if ( !_lastIsOperand ) throw 2287;
				if ( _isNot ) throw 2222;
				_isNot = 1;
			} else {
				if ( _isNot ) throw 2223;
				if ( BinaryOpNode::isAggregateOp( fop ) && type != 0 ) {
					throw 2224;
				}
				operatorStack.push(fop);
				operatorArgStack.push( nargs );
				_lastOp = fop;
			}
			p += len;
			isandor = 0;
			_lastIsOperand = 0;
		} else {
			if ( _isNot ) throw 2225;
			processOperand(jagParser, p, q, lastNode, cmap, colList );
			_lastIsOperand = 1;
		}
	} // end while

    while (!operatorStack.empty()) {

        doAddBinary( operatorStack.top(), operatorArgStack.top(), jmap );

        operatorStack.pop();
        operatorArgStack.pop();
    }

	if ( operandStack.numOperators > 0 ) {
    	if ( operandStack.size() != operandStack.numOperators ) { throw 2326; }
	} else {
    	if ( operandStack.size() != 1 ) { throw 28327; }
	}

    ExprElementNode *topp = operandStack.top();
    return static_cast<BinaryOpNode *> (topp);
}

void BinaryExpressionBuilder
::processBetween( const JagParser *jpsr, const char *&p, const char *&q, StringElementNode &lastNode,
				const JagHashMap<AbaxString, AbaxPair<AbaxString, jagint>> &cmap, JagHashStrInt *&jmap, 
				Jstr &colList )
{
	StringElementNode lnode( this, lastNode._name, lastNode._value, _jpa, 
							 lastNode._tabnum, lastNode._typeMode );

	if ( !_isNot ) processOperator( JAG_FUNC_GREATEREQUAL, 2, jmap );
	else processOperator( JAG_FUNC_LESSTHAN, 2, jmap );

	p += 7;
	while ( isspace(*p) ) ++p;
	processOperand( jpsr,  p, q, lastNode, cmap, colList );
	while ( isspace(*p) ) ++p;

	if ( 0 != strncasecmp( p, "and", 3 ) || *(p+3) != ' ' ) throw 21327;

	p += 3;
	while ( isspace(*p) ) ++p;
	if ( !_isNot ) processOperator( JAG_LOGIC_AND, 2, jmap );
	else processOperator( JAG_LOGIC_OR, 2, jmap );
	StringElementNode *newNode = new StringElementNode( this, lastNode._name, lastNode._value, 
														_jpa, lastNode._tabnum, lastNode._typeMode );

	operandStack.push(newNode);
	if ( !_isNot ) processOperator( JAG_FUNC_LESSEQUAL, 2, jmap );
	else processOperator( JAG_FUNC_GREATERTHAN, 2, jmap );

	processOperand( jpsr, p, q, lastNode, cmap, colList );
	if ( _isNot ) _isNot = 0;
}

void BinaryExpressionBuilder::processIn( const JagParser *jpsr, const char *&p, const char *&q, StringElementNode &lastNode,
										 const JagHashMap<AbaxString, AbaxPair<AbaxString, jagint>> &cmap, JagHashStrInt *&jmap, 
										 Jstr &colList )
{
	StringElementNode lnode( this, lastNode._name, lastNode._value, _jpa, lastNode._tabnum, lastNode._typeMode );

	bool first = 1, hasSep = 0;
	p += 2;
	while ( isspace(*p) ) ++p;
	if ( *p != '(' ) throw 28;
	++p;

	while ( 1 ) {
		while ( isspace(*p) ) ++p;
		if ( *p == ')' || *p == '\0' ) break;
		else if ( *p == ',' ) {
			if ( first ) throw 29;
			++p;
			if ( !_isNot ) processOperator( JAG_LOGIC_OR, 2, jmap );
			else processOperator( JAG_LOGIC_AND, 2, jmap );
			hasSep = 1;
			continue;
		}

		if ( !first ) {	
			StringElementNode *newNode = new StringElementNode( this, lastNode._name, lastNode._value, 
													_jpa, lastNode._tabnum, lastNode._typeMode );
			operandStack.push(newNode);
			#ifdef DEBUG_STACK
			i("s22239 operandStack.push %0x\n", newNode );
			//operandStack.print(); 
			//newNode->print(); 
			#endif
		} else {
			first = 0;
		}

		if ( !_isNot ) processOperator( JAG_FUNC_EQUAL, 2, jmap );
		else processOperator( JAG_FUNC_NOTEQUAL, 2, jmap );
		processOperand( jpsr, p, q, lastNode, cmap, colList );
		hasSep = 0;
	}
	
	if ( *p == '\0' || hasSep == 1 ) throw 30;
	++p;
	if ( _isNot ) _isNot = 0;
}

void BinaryExpressionBuilder
::processOperand( const JagParser *jpsr, const char *&p, const char *&q, StringElementNode &lastNode,
				  const JagHashMap<AbaxString, AbaxPair<AbaxString, jagint>> &cmap, Jstr &colList )
{
	Jstr name, value;
	int typeMode = 1; 
	const char *r;
	q = p;

	if ( *p == '\'' || *p == '"' ) {
		if ( (q = jumptoEndQuote(q)) && *q == '\0' ) {
			throw 31;
		}
		for ( r = p+1; r < q; ++r ) {
			if ( !isdigit(*r) && *r != '.' && *r != '+' && *r != '-' ) {
				typeMode = 0;
			} else if ( *r == '.' ) {
				typeMode = 2;
			}
		}
		typeMode = 0;
		r = p+1;
		value = Jstr(r, q-r);
		StringElementNode *newNode = new StringElementNode( this, name, value, _jpa, 0, typeMode );
		operandStack.push(newNode);
		#ifdef DEBUG_STACK
		d("s1112028 operandStack.push %0x name=%s value=%s\n", newNode, name.c_str(), value.c_str() );
		//operandStack.print();
		//newNode->print(); 
		//i("s0393  name=[%s] value=[%s] typeMode=%d\n", name.c_str(), value.c_str(), typeMode );
		#endif
		if ( _substrClause >= 0 ) ++_substrClause;
		else if ( _datediffClause >= 0 ) ++_datediffClause;
	} else if ( 0 == strncasecmp(p, "point(", 6 ) 
				|| 0==strncasecmp(p, "point3d(", 8 ) 
				|| 0==strncasecmp(p, "circle(", 7 ) 
				|| 0==strncasecmp(p, "circle3d(", 9 ) 
				|| 0==strncasecmp(p, "sphere(", 7 ) 
				|| 0==strncasecmp(p, "square(", 7 ) 
				|| 0==strncasecmp(p, "square3d(", 9 ) 
				|| 0==strncasecmp(p, "cube(", 5 ) 
				|| 0==strncasecmp(p, "rectangle(", 10 ) 
				|| 0==strncasecmp(p, "rectangle3d(", 12 ) 
				|| 0==strncasecmp(p, "box(", 4 ) 
				|| 0==strncasecmp(p, "cone(", 5 ) 
				|| 0==strncasecmp(p, "line(", 5 ) 
				|| 0==strncasecmp(p, "line3d(", 7 ) 
				|| 0==strncasecmp(p, "triangle(", 9 ) 
				|| 0==strncasecmp(p, "triangle3d(", 11 ) 
				|| 0==strncasecmp(p, "cylinder(", 9 ) 
				|| 0==strncasecmp(p, "ellipse(", 8 ) 
				|| 0==strncasecmp(p, "ellipse3d(", 10 ) 
				|| 0==strncasecmp(p, "ellipsoid(", 10 ) 
				|| 0==strncasecmp(p, "vector(", 7 ) 
				|| 0==strncasecmp(p, "linestring(", 11 ) 
				|| 0==strncasecmp(p, "linestring3d(", 13 ) 
				|| 0==strncasecmp(p, "multipoint(", 11 ) 
				|| 0==strncasecmp(p, "multipoint3d(", 13 ) 
				|| 0==strncasecmp(p, "polygon(", 8 ) 
				|| 0==strncasecmp(p, "polygon3d(", 10 ) 
				|| 0==strncasecmp(p, "multilinestring(", 16 ) 
				|| 0==strncasecmp(p, "multilinestring3d(", 18 ) 
				|| 0==strncasecmp(p, "multipolygon(", 13 ) 
				|| 0==strncasecmp(p, "multipolygon3d(", 15 ) 
				|| 0==strncasecmp(p, "range(", 6 ) 
				|| 0==strncasecmp(p, "bbox(", 5 ) 
	          ) {
		bool isPoly = false;
		if ( 0==strncasecmp(p, "linestring", 10 ) || 0==strncasecmp(p, "polygon", 7) 
			 || 0==strncasecmp(p, "multipoint", 10 ) || 0==strncasecmp(p, "multilinestring", 12 ) 
			 || 0==strncasecmp(p, "multipolygon", 10 )
			 || 0==strncasecmp(p, "vector", 6 )
		   ) { 
		   isPoly = true; 
		}

		q = p;
		while ( *q != '(' ) ++q;
		Jstr geotype(p, q-p);
		Jstr typeStr = geotype;
		geotype = convertType2Short( geotype );
		
		Jstr val, dtype;
		bool isRaw = false;
		if ( isPoly ) {
			if ( geotype == JAG_C_COL_TYPE_POLYGON || geotype == JAG_C_COL_TYPE_POLYGON3D 
				 || geotype == JAG_C_COL_TYPE_MULTILINESTRING || geotype == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
				p = q; 
				q = strrchr(p, ')' );
				if ( ! q ) { throw 101; }
				char *d = strdup(p);
				JagParser::removeEndUnevenBracketAll( d );
				val = d;
				free(d );
				q = p + val.size();
				isRaw = true;
			} else {
				p = q+1;
				q = strchr(p, ')' );
				if ( ! q ) { throw 102; }
				val = Jstr(p, q-p);
    			JagStrSplit sp(val, ',' );
    			Jstr c, newval;
    			for ( int i=0; i < sp.length(); ++i ) {
    				JagStrSplit cd(sp[i], ' ', true );
    				if ( cd.length() == 2 ) {
    					c = cd[0] + ":" + cd[1];
    				} else if ( cd.length() == 3 ) {
    					c = cd[0] + ":" + cd[1] + ":" + cd[2];
    				} else {
    					c = sp[i];
    				}
    
    				if ( newval.size() < 1 ) {
    					newval = c;
    				} else {
    					newval += Jstr(" ") + c;
    				}
    			}
    			val = newval;
			}
		} else {
			p = q+1; // (p
			q = strchr(p, ')' );
			if ( ! q ) { throw 102; }
			val = Jstr(p, q-p);
			if ( geotype != JAG_C_COL_TYPE_RANGE ) {
				val.replace( ',', ' ');
			}
		}

		name = "";
		if ( geotype == JAG_C_COL_TYPE_RANGE ) {
			JagStrSplit sp( val, ',');
			if ( sp.length() < 2 ) {
				throw 2319;
			}
			sp[0].remove('\'');
			sp[0].trimSpaces(2);
			sp[0].replace(' ', '_');
			sp[1].remove('\'');
			sp[1].trimSpaces(2);
			sp[1].replace(' ', '_');
			val = sp[0] + "|" + sp[1];
            dn("s230038 JAG_C_COL_TYPE_RANGE val=[%s]", val.s() );
            dtype = JAG_C_COL_TYPE_DT_NONE;
		} else {
            dtype = "d";
        }

		if ( isRaw ) {
			value = typeStr + val;
		} else {
			int dim = getDimension( geotype );
			if ( 2 == dim ) {
				value = Jstr("CJAG=0=0=") + geotype + "=" + dtype + " 0:0:0:0 " + val;
            } else if ( 0 == dim ) {
				value = Jstr("CJAG=0=0=") + geotype + "=" + dtype + " 0:0:0:0 " + val;
			} else {
				value = Jstr("CJAG=0=0=") + geotype + "=" + dtype + " 0:0:0:0:0:0 " + val;
			}
		}

		typeMode = 2;
		StringElementNode *newNode = new StringElementNode( this, name, value, _jpa, 0, typeMode );
		operandStack.push(newNode);
		#ifdef DEBUG_STACK
		//i("s450222 operandStack.push %0x  nme=[%s] val=[%s]\n", newNode, name.c_str(), value.c_str() );
		//operandStack.print(); 
		//newNode->print(); 
		#endif
		++q; p = q;
	} else {
		if ( !isValidNameChar(*q) && *q != '_' && *q != '-' && *q != '+' && *q != '.' && *q != ':' ) throw 4132;
		++q;
		while ( isValidNameChar(*q) || *q == '_' || *q == '.' || *q == ':' ) ++q;
		r = p;
		for ( r = p; r < q; ++r ) {
			if ( !isdigit(*r) && *r != '.' && *r != '-' && *r != '+' ) {
				typeMode = 0;
			} else if ( *r == '.' ) {
				typeMode = 2;
			}
		}

		if ( typeMode > 0 ) {
			value = Jstr(p, q-p);
			StringElementNode *newNode = new StringElementNode( this, name, value, _jpa, 0, typeMode );
			operandStack.push(newNode);

			#ifdef DEBUG_STACK
		    i("s450322 operandStack.push %0x name=[%s] value=[%s]\n", newNode, name.c_str(), value.c_str() );
			operandStack.print(); 
			newNode->print();
			#endif
			if ( _substrClause >= 0 ) ++_substrClause;
			else if ( _datediffClause >= 0 ) ++_datediffClause;
		} else {
			if ( 3 == _substrClause || 0 == _datediffClause ) {
				value = Jstr(p, q-p);
				StringElementNode *newNode = new StringElementNode( this, name, value, _jpa, 0, typeMode );
				operandStack.push(newNode);

				#ifdef DEBUG_STACK
		    	i("s450342 operandStack.push %0x name=[%s] value=[%s]\n", newNode, name.c_str(), value.c_str() );
				operandStack.print();
				#endif

				if ( _substrClause >= 0 ) ++_substrClause;
				else if ( _datediffClause >= 0 ) ++_datediffClause;
			} else {				
				int tabnum = 0;
				name = Jstr(p, q-p);
				if ( !nameConvertionCheck( name, tabnum, cmap, colList ) ) {
					throw 4303;
				}
				if ( ! nameAndOpGood( jpsr, name, lastNode ) ) {
					throw 4304;
				}

				StringElementNode *newNode = new StringElementNode( this, name, value, _jpa, tabnum, typeMode );
				lastNode._name = newNode->_name;
				lastNode._value = newNode->_value;
				lastNode._tabnum = newNode->_tabnum;
				lastNode._typeMode = newNode->_typeMode;
				operandStack.push(newNode);
				#ifdef DEBUG_STACK
				i("s222239 operandStack.push %0x name=[%s] value=[%s]\n", newNode, name.c_str(), value.c_str() );
				operandStack.print(); 
				newNode->print(); 
				#endif

				if ( _substrClause >= 0 ) ++_substrClause;
				else if ( _datediffClause >= 0 ) ++_datediffClause;
			}
		}
	}

	if ( *p == '\'' || *p == '"' ) p = q+1;
	else p = q;
}

void BinaryExpressionBuilder::processOperator( short op, int nargs, JagHashStrInt *&jmap )
{
    short opPrecedence = precedence( op );
    while ((!operatorStack.empty()) && (opPrecedence <= precedence( operatorStack.top() ))) {
        doAddBinary( operatorStack.top(), operatorArgStack.top(), jmap );
        operatorStack.pop();
        operatorArgStack.pop();
    }

    operatorStack.push(op);
    operatorArgStack.push(nargs);
	_lastOp = op;

}

void BinaryExpressionBuilder::processRightParenthesis( JagHashStrInt *&jmap )
{
	if ( operatorStack.empty() ) {
		return;
	}

	int fop;
	int opargs;
	while ( 1 ) {
		fop = operatorStack.top();
		opargs = operatorArgStack.top();
		if ( fop == '(' ) {
			operatorStack.pop(); // remove '('
			operatorArgStack.pop(); // remove '('
			break;
		} else if ( checkFuncType( fop ) ) {
			doAddBinary( fop, opargs, jmap );
			operatorStack.pop();
			operatorArgStack.pop(); // remove '('
			break;
		} else {
			doAddBinary( fop, opargs, jmap );
			operatorStack.pop();
			operatorArgStack.pop(); // remove '('
		}
	}
}

void BinaryExpressionBuilder::doAddBinary( short op, int nargs, JagHashStrInt *&jmap )
{
	Jstr opstr = BinaryOpNode::binaryOpStr(op);
	ExprElementNode *right = NULL;
	ExprElementNode *left = NULL;
	int arg1 = 0, arg2 = 0;
	Jstr carg1;
	const char *p = NULL;;
	ExprElementNode *t3;

	if ( funcHasTwoChildren(op) ) {
    	if ( operandStack.empty() ) { throw 1434; } 
		if ( op == JAG_FUNC_NEARBY ) {
			t3 = operandStack.top();
			if ( t3->getValue(p) ) { carg1 = p; } else throw 1318;
			operandStack.pop();
		} else if ( op == JAG_FUNC_DISTANCE ) {
            dn("s450029 JAG_FUNC_DISTANCE");
			if ( 2 == nargs ) {
				carg1 = "center";
			} else {
				t3 = operandStack.top();
				if ( t3->getValue(p) ) { carg1 = p; } else throw 1419;
				operandStack.pop();
			}
		} else if ( op == JAG_FUNC_SIMILARITY ) {
            dn("s450023 JAG_FUNC_SIMILARITY");
			if ( 2 == nargs ) {
				carg1 = "cosine";
			} else {
				t3 = operandStack.top();
				if ( t3->getValue(p) ) { carg1 = p; } else throw 1519;
				operandStack.pop();
			}
		} else if ( op == JAG_FUNC_ADDPOINT ) {
			if ( 2 == nargs ) {
				carg1 = "-1"; 
			} else {
				t3 = operandStack.top();
				if ( t3->getValue(p) ) { carg1 = p; } else throw 1619;
				operandStack.pop(); 
			}
		} else if ( op == JAG_FUNC_SETPOINT ) {
			t3 = operandStack.top();
			if ( t3->getValue(p) ) { carg1 = p; } else throw 1329;
			operandStack.pop();
		}  else if ( op == JAG_FUNC_SCALEAT ) {
			if (  nargs >= 3 ) {
				t3 = operandStack.top();
				if ( t3->getValue(p) ) { carg1 = p; } else throw 1332;
				operandStack.pop();
			} 

			if ( nargs >= 4 ) {
				t3 = operandStack.top();
				if ( t3->getValue(p) ) { carg1 = Jstr(p) + ":" + carg1; } else throw 1340;
				operandStack.pop();
			} 
			
			if ( nargs >= 5 ) {
				t3 = operandStack.top();
				if ( t3->getValue(p) ) { carg1 = Jstr(p) + ":" + carg1; } else throw 1341;
				operandStack.pop();
			}
		} else if ( op == JAG_FUNC_KNN ) {
			if ( 5 == nargs ) {
				t3 = operandStack.top();
				if ( t3->getValue(p) ) { carg1 = p; } else throw 4332;
				operandStack.pop();

				t3 = operandStack.top();
				if ( t3->getValue(p) ) { carg1 = Jstr(p) + ":" +  carg1; } else throw 4333;
				operandStack.pop();

				t3 = operandStack.top();
				if ( t3->getValue(p) ) { carg1 = Jstr(p) + ":" +  carg1; } else throw 4334;
				operandStack.pop();
			} else if ( 3 == nargs ) {
				t3 = operandStack.top();
				if ( t3->getValue(p) ) { carg1 = Jstr(p) + ":-1:" +  d2s(LONG_MAX); } else throw 4335;
				operandStack.pop();
			} else throw 4938;
		}

		if ( op == JAG_FUNC_VORONOIPOLYGONS || op == JAG_FUNC_VORONOILINES ) {
			if ( 1 == nargs ) {
				right = NULL;
    			left = operandStack.top();
    			operandStack.pop();
			} else if ( 2 == nargs ) {
				right = NULL;

				t3 = operandStack.top();
				if ( t3->getValue(p) ) { carg1 = Jstr(p); } else throw 1342;
				operandStack.pop();

    			if ( operandStack.empty() ) { throw 1434; } 
    			left = operandStack.top();
    			operandStack.pop();
			} else if ( 3 == nargs ) {
    			right = operandStack.top();
    			operandStack.pop();
    			if ( operandStack.empty() ) { throw 1435; } 
				t3 = operandStack.top();
				if ( t3->getValue(p) ) { carg1 = Jstr(p); } else throw 1343;
				operandStack.pop(); 
    			if ( operandStack.empty() ) { throw 1446; } 
    			left = operandStack.top();
    			operandStack.pop();
			}
		} else {
    		if ( operandStack.empty() ) { throw 1534; } 
    		right = operandStack.top();
    		operandStack.pop();
			#ifdef DEBUG_STACK
			i("s233309 operandStack.pop\n");
			operandStack.print(); 
			#endif
    
    		if ( operandStack.empty() ) { throw 1535; } 
    		left = operandStack.top();
    		operandStack.pop();
			#ifdef DEBUG_STACK
			i("s233349 operandStack.pop\n");
			operandStack.print();
			#endif
		}

		if ( op == JAG_FUNC_DATEDIFF ) {
			ExprElementNode *tdiff = NULL;
			const char *p = NULL;
			if ( operandStack.empty() ) throw 2436;
			else tdiff = operandStack.top();

			if ( tdiff->getValue(p) ) {
				carg1 = "m";
				if ( *p == 's' || *p == 'S' ) carg1 = "s";
				else if ( *p == 'h' || *p == 'H' ) carg1 = "h";
				else if ( *p == 'd' || *p == 'D' ) carg1 = "D";
				else if ( (*p == 'm' || *p == 'M') && ( *(p+1) == 'o' || *(p+1) == 'O' ) ) carg1 = "M";
				else if ( (*p == 'm' || *p == 'M') && ( *(p+1) == 'i' || *(p+1) == 'I' ) ) carg1 = "m";
				else if ( *p == 'y' || *p == 'Y' ) carg1 = "Y";
				else throw 437;				
			} else throw 438;
			operandStack.pop();
		}

		if ( BinaryOpNode::isCompareOp( op ) ) {
			const char *p, *q;
			jagint lrc, rrc;
			lrc = left->getName(p);
			rrc = right->getName(q);
			if ( lrc >= 0 && rrc >= 0 && lrc != rrc ) {
				if ( NULL == jmap ) {
					jmap = new JagHashStrInt();
				}
				jmap->addKeyValue( p, lrc );
				jmap->addKeyValue( q, rrc );
			}
		}
	} else if ( funcHasOneConstant(op) ) {
		if ( operandStack.empty() ) throw 4306;
		if ( op == JAG_FUNC_TOSECOND || op == JAG_FUNC_TOMICROSECOND  ) {
			ExprElementNode *en = NULL;
			const char *p = NULL;
			en = operandStack.top();
			if ( en->getValue(p) ) {
				jagint num;
				if ( op == JAG_FUNC_TOSECOND  ) {
					num = convertToSecond(p);
					if ( num < 0 ) throw 4316;
					carg1 = longToStr( num );
				} else {
					num = convertToMicroSecond(p);
					if ( num < 0 ) throw 4326;
					carg1 = longToStr( num );
				}
			} else throw 4308;
			operandStack.pop();
		} 
	} else if ( funcHasZeroChildren(op) ) {
	} else {
		if ( operandStack.empty() ) throw 3439;
		const char *p;
		if ( op == JAG_FUNC_SUBSTR ) {
			ExprElementNode *targ = NULL;
			int hasEncode = 0;
			targ = operandStack.top();
			if ( targ->getValue(p) ) {
				if ( isdigit(*p) ) { arg2 = jagatoi(p); }
				else { 
					hasEncode = 1;
					carg1 = p; 
				}
			} else { throw 3440; }
    		operandStack.pop();

    		if ( operandStack.empty() ) throw 3441;
    		targ = operandStack.top();
			if ( ! hasEncode ) {
    			if ( targ->getValue(p) ) {
    				if ( isdigit(*p) ) arg1 = jagatoi(p);
    			} else throw 3442;
			} else {
				if ( targ->getValue(p) ) {
					if ( isdigit(*p) ) { arg2 = jagatoi(p); }
				} else throw 3443;

    			operandStack.pop();
    			if ( operandStack.empty() ) throw 3444;
    			targ = operandStack.top();
    			if ( targ->getValue(p) ) {
    				if ( isdigit(*p) ) arg1 = jagatoi(p);
    			} else throw 3445;
			}
			operandStack.pop();
		} else if ( op == JAG_FUNC_LINESUBSTRING ) {
			Jstr endf;
			ExprElementNode *t3 = operandStack.top();
			if ( t3->getValue(p) ) {
				endf = p; 
			} else throw 1323;
			operandStack.pop();  

			if ( operandStack.empty() ) throw 1325;
			t3 = operandStack.top();
			Jstr startf;
			if ( t3->getValue(p) ) {
				startf = p; 
			} else throw 1326;
			operandStack.pop(); 
			carg1 = startf + ":" + endf;
		} else if ( op == JAG_FUNC_POINTN || op == JAG_FUNC_POLYGONN
    					 || op == JAG_FUNC_INTERPOLATE || op == JAG_FUNC_REMOVEPOINT
    		             || op == JAG_FUNC_RINGN || op == JAG_FUNC_INNERRINGN ) {
	    	if ( nargs  < 2 ) throw 2425;
        	right = operandStack.top();
    		if ( right->getValue(p) ) {
    			carg1 = p;
    		} else throw 1448;
    		operandStack.pop();
			right = NULL;
    	} else if ( op == JAG_FUNC_TOPOLYGON || op == JAG_FUNC_TOMULTIPOINT ) {
	    	if ( 1 == nargs ) {
    			carg1 = "30";
    		} else {
        		right = operandStack.top();
    			if ( right->getValue(p) ) {
    				carg1 = p;
    			} else throw 2932;
    			operandStack.pop();
				right = NULL;
    		}
    	} else if ( op == JAG_FUNC_GEOJSON || op == JAG_FUNC_WKT ) {
			if ( 1 == nargs ) {
				carg1 = intToStr(JAG_MAX_POINTS_SENT) + ":30";
			} else if ( 2 == nargs ) {
        		right = operandStack.top();
    			if ( right->getValue(p) ) {
    				carg1 = Jstr(p) + ":30";
    			} else throw 2932;
    			operandStack.pop();
				right = NULL;
			} else if ( 3 == nargs ) {
        		right = operandStack.top();
    			if ( right->getValue(p) ) {
    				carg1 = Jstr(p);
    			} else throw 2933;
    			operandStack.pop();

    			if ( operandStack.empty() ) throw 3454;
        		right = operandStack.top();
    			if ( right->getValue(p) ) {
    				carg1 = Jstr(p) + ":" + carg1;
    			} else throw 2934;
    			operandStack.pop();
				right = NULL;
			}
    	} else if ( op == JAG_FUNC_METRICN ) {
			if ( 1 == nargs ) {
    			carg1 = "0:0";
			} else if ( 2 == nargs ) {
        		right = operandStack.top();
    			if ( right->getValue(p) ) {
    				carg1 = Jstr(p) + ":0";
    			} else throw 5012;
    			operandStack.pop();
				right = NULL;
			} else if ( 3 == nargs ) {
        		right = operandStack.top();
    			if ( right->getValue(p) ) {
    				carg1 = Jstr(p);
    			} else throw 5933;
    			operandStack.pop();

    			if ( operandStack.empty() ) throw 5454;
        		right = operandStack.top();
    			if ( right->getValue(p) ) {
    				carg1 = Jstr(p) + ":" + carg1;
    			} else throw 5934;
    			operandStack.pop();
				right = NULL;
			}
    	} else if ( op == JAG_FUNC_BUFFER ) {
	    	if ( 1 == nargs ) {
    			carg1 = "distance=symmetric:10,side=side,join=round:10,end=round:10,point=circle:10";
    		} else {
        		right = operandStack.top();
    			if ( right->getValue(p) ) {
    				carg1 = p;
    			} else throw 2935;
    			operandStack.pop();
				right = NULL;
    		}
		}  else if ( op == JAG_FUNC_SCALE || op == JAG_FUNC_SCALESIZE ) {
			if (  nargs >= 2 ) {
				t3 = operandStack.top();
				if ( t3->getValue(p) ) { carg1 = p; } else throw 1332;
				operandStack.pop();  
			} 

			if ( nargs >= 3 ) {
				t3 = operandStack.top();
				if ( t3->getValue(p) ) { carg1 = Jstr(p) + ":" + carg1; } else throw 1350;
				operandStack.pop(); 
			} 
			
			if ( nargs >= 4 ) {
				t3 = operandStack.top();
				if ( t3->getValue(p) ) { carg1 = Jstr(p) + ":" + carg1; } else throw 1351;
				operandStack.pop();
			}
		}  else if ( op == JAG_FUNC_TRANSLATE ) {
			t3 = operandStack.top();
			if ( t3->getValue(p) ) { carg1 = p; } else throw 2332;
			operandStack.pop(); 
			if ( operandStack.empty() ) throw 4546;

			t3 = operandStack.top();
			if ( t3->getValue(p) ) { carg1 = Jstr(p) + ":" + carg1; } else throw 2350;
			operandStack.pop();
			
			if ( nargs >= 4 ) {
				t3 = operandStack.top();
				if ( t3->getValue(p) ) { carg1 = Jstr(p) + ":" + carg1; } else throw 2351;
				operandStack.pop();
			}
		}  else if ( op == JAG_FUNC_TRANSSCALE ) {
			if ( 4 == nargs ) {
    			t3 = operandStack.top();
    			if ( t3->getValue(p) ) { carg1 = p; } else throw 3362;
    			operandStack.pop();

    			if ( operandStack.empty() ) throw 3246;
    			t3 = operandStack.top();
    			if ( t3->getValue(p) ) { carg1 = Jstr(p) + ":" + carg1; } else throw 3452;
    			operandStack.pop();

    			if ( operandStack.empty() ) throw 3247;
    			t3 = operandStack.top();
    			if ( t3->getValue(p) ) { carg1 = Jstr(p) + ":" + carg1; } else throw 3453;
    			operandStack.pop();
			} else if ( 5 == nargs ) {
    			t3 = operandStack.top();
    			if ( t3->getValue(p) ) { carg1 = p; } else throw 2452;
    			operandStack.pop();

    			if ( operandStack.empty() ) throw 4546;
    			t3 = operandStack.top();
    			if ( t3->getValue(p) ) { carg1 = Jstr(p) + ":" + carg1; } else throw 2452;
    			operandStack.pop();

    			if ( operandStack.empty() ) throw 4547;
    			t3 = operandStack.top();
    			if ( t3->getValue(p) ) { carg1 = Jstr(p) + ":" + carg1; } else throw 2453;
    			operandStack.pop();

    			if ( operandStack.empty() ) throw 4548;
    			t3 = operandStack.top();
    			if ( t3->getValue(p) ) { carg1 = Jstr(p) + ":" + carg1; } else throw 2454;
    			operandStack.pop();

			} else if ( 7 == nargs ) {
				t3 = operandStack.top();
				if ( t3->getValue(p) ) { carg1 = Jstr(p); } else throw 2455;
				operandStack.pop();

    			if ( operandStack.empty() ) throw 4549;
    			t3 = operandStack.top();
    			if ( t3->getValue(p) ) { carg1 = Jstr(p) + ":" + carg1; } else throw 2456;
    			operandStack.pop();

    			if ( operandStack.empty() ) throw 4550;
    			t3 = operandStack.top();
    			if ( t3->getValue(p) ) { carg1 = Jstr(p) + ":" + carg1; } else throw 2457;
    			operandStack.pop(); 

    			if ( operandStack.empty() ) throw 4551;
    			t3 = operandStack.top();
    			if ( t3->getValue(p) ) { carg1 = Jstr(p) + ":" + carg1; } else throw 2458;
    			operandStack.pop(); 

    			if ( operandStack.empty() ) throw 4552;
    			t3 = operandStack.top();
    			if ( t3->getValue(p) ) { carg1 = Jstr(p) + ":" + carg1; } else throw 2459;
    			operandStack.pop(); 

    			if ( operandStack.empty() ) throw 4553;
    			t3 = operandStack.top();
    			if ( t3->getValue(p) ) { carg1 = Jstr(p) + ":" + carg1; } else throw 2460;
    			operandStack.pop(); 
			} else {
				throw 2582;
			}
		}  else if ( op == JAG_FUNC_ROTATE || op == JAG_FUNC_ROTATESELF ) {
			// rotate(geom, 10 )  -- defualt is in degrees, counter-clock-wise
			// rotate(geom, 10, 'degree')
			// rotate(geom, 10, 'radian')
			// rotateself(geom, 10, 'radian')
			t3 = operandStack.top();
			if ( ! t3->getValue(p) ) throw 4601;
			carg1 = p;
			operandStack.pop();  // popped the third element
			if ( ! isdigit(*p) && *p != '.' ) {
				if ( *p == 'd' || *p == 'D' || *p == 'r' || *p == 'R' ) {
					t3 = operandStack.top();
					if ( ! t3->getValue(p) ) throw 4601;
					carg1 = Jstr(p) + ":" + carg1;  // "10.2:degree"
					operandStack.pop();
				} else throw 2340;
			} else {
				carg1 += ":degree"; 
			}
		}  else if ( op == JAG_FUNC_ROTATEAT ) {
			// 2D only
			// rotateat(geom, 10, 'degree', x0,y0)
			t3 = operandStack.top();
			if ( ! t3->getValue(p) ) throw 4601;
			carg1 = p;
			operandStack.pop();  // popped y0

			if ( operandStack.empty() ) throw 4602;
			t3 = operandStack.top();
			if ( ! t3->getValue(p) ) throw 4603;
			carg1 = Jstr(p) + ":" + carg1;  // "10.2:23.1"
			operandStack.pop();  // popped x0

			if ( operandStack.empty() ) throw 4604;
			t3 = operandStack.top();
			if ( ! t3->getValue(p) ) throw 4605;
			carg1 = Jstr(p) + ":" + carg1;  // "degree:10.2:23.1"
			operandStack.pop();  // popped 'degree'

			if ( operandStack.empty() ) throw 4606;
			t3 = operandStack.top();
			if ( ! t3->getValue(p) ) throw 4607;
			carg1 = Jstr(p) + ":" + carg1;  // "12.3:degree:10.2:23.1"
			operandStack.pop();  // popped angle
		}  else if ( op == JAG_FUNC_AFFINE ) {
			// 2D affine(geom, a, b, d, e, xoff, yoff) -- 6 args
			// 3D Affine(geom, a, b, c, d, e, f, g, h, i, xoff, yoff, zoff)  -- 12 args
			if ( nargs >= 6 ) {
				// 2D affine
    			t3 = operandStack.top();
    			if ( ! t3->getValue(p) ) throw 4611;
    			carg1 = p;
    			operandStack.pop(); 
    
    			if ( operandStack.empty() ) throw 4612;
    			t3 = operandStack.top();
    			if ( ! t3->getValue(p) ) throw 4613;
    			carg1 = Jstr(p) + ":" + carg1;  
    			operandStack.pop(); 
    
    			if ( operandStack.empty() ) throw 4614;
    			t3 = operandStack.top();
    			if ( ! t3->getValue(p) ) throw 4615;
    			carg1 = Jstr(p) + ":" + carg1;  
    			operandStack.pop();
    
    			if ( operandStack.empty() ) throw 4616;
    			t3 = operandStack.top();
    			if ( ! t3->getValue(p) ) throw 4617;
    			carg1 = Jstr(p) + ":" + carg1;  
    			operandStack.pop(); 

    			if ( operandStack.empty() ) throw 4618;
    			t3 = operandStack.top();
    			if ( ! t3->getValue(p) ) throw 4617;
    			carg1 = Jstr(p) + ":" + carg1;  
    			operandStack.pop(); 

    			if ( operandStack.empty() ) throw 4618;
    			t3 = operandStack.top();
    			if ( ! t3->getValue(p) ) throw 4617;
    			carg1 = Jstr(p) + ":" + carg1;  
    			operandStack.pop(); 
			} else throw 3939;
			if ( nargs >= 12 ) {
    			if ( operandStack.empty() ) throw 4622;
    			t3 = operandStack.top();
    			if ( ! t3->getValue(p) ) throw 4623;
    			carg1 = Jstr(p) + ":" + carg1;  
    			operandStack.pop();  
    
    			if ( operandStack.empty() ) throw 4624;
    			t3 = operandStack.top();
    			if ( ! t3->getValue(p) ) throw 4625;
    			carg1 = Jstr(p) + ":" + carg1;  
    			operandStack.pop(); 
    
    			if ( operandStack.empty() ) throw 4626;
    			t3 = operandStack.top();
    			if ( ! t3->getValue(p) ) throw 4627;
    			carg1 = Jstr(p) + ":" + carg1;  
    			operandStack.pop();

    			if ( operandStack.empty() ) throw 4628;
    			t3 = operandStack.top();
    			if ( ! t3->getValue(p) ) throw 4627;
    			carg1 = Jstr(p) + ":" + carg1;  
    			operandStack.pop();

    			if ( operandStack.empty() ) throw 4638;
    			t3 = operandStack.top();
    			if ( ! t3->getValue(p) ) throw 4637;
    			carg1 = Jstr(p) + ":" + carg1;  
    			operandStack.pop(); 

    			if ( operandStack.empty() ) throw 4648;
    			t3 = operandStack.top();
    			if ( ! t3->getValue(p) ) throw 4647;
    			carg1 = Jstr(p) + ":" + carg1;  
    			operandStack.pop(); 
			}
		}  else if ( op == JAG_FUNC_DELAUNAYTRIANGLES ) {
			if (  2 == nargs ) {
				t3 = operandStack.top();
				if ( t3->getValue(p) ) { carg1 = p; } else throw 1332;
				operandStack.pop();  // popped the third element
			} 
		}

		if ( operandStack.empty() ) throw 4446;

		left = operandStack.top();
		operandStack.pop();
		#ifdef DEBUG_STACK
		i("s428122 operandStack.pop left=%0x\n", left );
		operandStack.print();
		#endif
	}

    opstr = BinaryOpNode::binaryOpStr(operatorStack.top());
	BinaryOpNode *pn = new BinaryOpNode(this, operatorStack.top(), operatorArgStack.top(), left, right, _jpa, arg1, arg2, carg1 );
	operandStack.push(pn);

	#ifdef DEBUG_STACK
	i("s02932 operandStack.push %0x BinaryOpNode (left=%0x right=%0x)\n", pn, left, right );
	operandStack.print(); 
	pn->print(); 
	i("left:\n"); 
	if ( left ) left->print(); 
	i("\nright:\n"); 
	if (right) right->print();
	i("\n"); 
	i( "s7202 pushed operatorStack.top=%s to operandStack, operandStack.size=%d left=%0x right=%0x\n", 
		  opstr.c_str(), operandStack.size(), left, right );
	#endif
}

short BinaryExpressionBuilder::precedence( short fop )
{
	enum { lowest, low, lowmid, mid, midhigh, high, highest };
	switch (fop) {
		case JAG_LOGIC_OR:  		// or
			return low;
		case JAG_LOGIC_AND: 		// and
			return lowmid;
		case JAG_FUNC_EQUAL:		// = ==
			return mid;
		case JAG_FUNC_NOTEQUAL:		// != <> ><
			return mid;	
		case JAG_FUNC_LESSTHAN:		// <
			return mid;
		case JAG_FUNC_LESSEQUAL:	// <=
			return mid;
		case JAG_FUNC_GREATERTHAN:	// >
			return mid;
		case JAG_FUNC_GREATEREQUAL:	// >=
			return mid;
		case JAG_FUNC_LIKE:			// %
			return mid;
		case JAG_FUNC_MATCH:		// regex
			return mid;
		case JAG_NUM_ADD:			// +
			return midhigh;
		case JAG_STR_ADD:			// .
			return high;
		case JAG_NUM_SUB:			// -
			return midhigh;
		case JAG_NUM_MULT:			// *
			return high;
		case JAG_NUM_DIV:			// /
			return high;
		case JAG_NUM_REM:			// %
			return high;
		case JAG_NUM_POW:			// ^
			return highest;
		default:
			return lowest;
	}
}

bool BinaryExpressionBuilder::funcHasZeroChildren( short fop )
{
	if ( fop == JAG_FUNC_CURDATE || fop == JAG_FUNC_CURTIME  || fop == JAG_FUNC_PI
		 || fop == JAG_FUNC_NOW || fop == JAG_FUNC_TIME ) return true;
	return false;
}

bool BinaryExpressionBuilder::funcHasOneConstant( short fop )
{
	if ( fop == JAG_FUNC_TOSECOND || fop == JAG_FUNC_TOMICROSECOND ) {
		 return true;
	}
	return false;
}

bool BinaryExpressionBuilder::funcHasTwoChildren( short fop )
{
	if ( fop == JAG_FUNC_MOD || fop == JAG_FUNC_POW 
		 || fop == JAG_FUNC_DATEDIFF 
		 || fop == JAG_LOGIC_AND || fop == JAG_LOGIC_OR || BinaryOpNode::isMathOp(fop) 
		 || fop == JAG_FUNC_WITHIN || fop == JAG_FUNC_COVEREDBY || fop == JAG_FUNC_COVER
		 || fop == JAG_FUNC_STRDIFF || fop == JAG_FUNC_CONTAIN || fop == JAG_FUNC_SAME 
		 || fop == JAG_FUNC_INTERSECT || fop == JAG_FUNC_DISJOINT || fop == JAG_FUNC_CLOSESTPOINT 
		 || fop == JAG_FUNC_ANGLE || fop == JAG_FUNC_UNION || fop == JAG_FUNC_LOCATEPOINT 
		 || fop == JAG_FUNC_DIFFERENCE || fop == JAG_FUNC_SYMDIFFERENCE || fop == JAG_FUNC_INTERSECTION 
		 || fop == JAG_FUNC_COLLECT || fop == JAG_FUNC_SCALEAT
		 || fop == JAG_FUNC_ADDPOINT || fop == JAG_FUNC_SETPOINT 
		 || fop == JAG_FUNC_NEARBY || fop == JAG_FUNC_DISTANCE || fop == JAG_FUNC_SIMILARITY
		 || fop == JAG_FUNC_ISONLEFT || fop == JAG_FUNC_ISONRIGHT
		 || fop == JAG_FUNC_LEFTRATIO || fop == JAG_FUNC_RIGHTRATIO
		 || fop == JAG_FUNC_VORONOIPOLYGONS || fop == JAG_FUNC_VORONOILINES
		 || fop == JAG_FUNC_KNN
		 || BinaryOpNode::isCompareOp(fop) ) {
		 	return true;
	}
	return false;
} // end of funcHasTwoChildren()

bool BinaryExpressionBuilder::checkFuncType( short fop )
{
	if (fop == JAG_FUNC_MIN || fop == JAG_FUNC_MAX || fop == JAG_FUNC_AVG || fop == JAG_FUNC_SUM || 
		fop == JAG_FUNC_STDDEV || fop == JAG_FUNC_FIRST || fop == JAG_FUNC_LAST || fop == JAG_FUNC_ABS || 
		fop == JAG_FUNC_ACOS || fop == JAG_FUNC_ASIN || fop == JAG_FUNC_ATAN || fop == JAG_FUNC_CEIL ||
		fop == JAG_FUNC_COS || fop == JAG_FUNC_COT || fop == JAG_FUNC_FLOOR || fop == JAG_FUNC_LOG2 ||
		fop == JAG_FUNC_LOG10 || fop == JAG_FUNC_LOG || fop == JAG_FUNC_MOD || fop == JAG_FUNC_POW ||
		fop == JAG_FUNC_SIN || fop == JAG_FUNC_SQRT || fop == JAG_FUNC_TAN || fop == JAG_FUNC_SUBSTR ||
		fop == JAG_FUNC_UPPER || fop == JAG_FUNC_LOWER || fop == JAG_FUNC_LTRIM || fop == JAG_FUNC_RTRIM ||
		fop == JAG_FUNC_TRIM || fop == JAG_FUNC_SECOND || fop == JAG_FUNC_MINUTE || fop == JAG_FUNC_HOUR ||
		fop == JAG_FUNC_DAY || fop == JAG_FUNC_MONTH || fop == JAG_FUNC_YEAR || fop == JAG_FUNC_RTRIM ||
		fop == JAG_FUNC_DATE ||
		fop == JAG_FUNC_DATEDIFF || fop == JAG_FUNC_DAYOFMONTH || fop == JAG_FUNC_DAYOFWEEK || 
		fop == JAG_FUNC_DAYOFYEAR || fop == JAG_FUNC_CURDATE || fop == JAG_FUNC_CURTIME || fop == JAG_FUNC_NOW || fop == JAG_FUNC_TIME || 
		fop == JAG_FUNC_DISTANCE || fop == JAG_FUNC_WITHIN ||  fop == JAG_FUNC_SIMILARITY ||
		fop == JAG_FUNC_COVEREDBY || fop == JAG_FUNC_COVER || fop == JAG_FUNC_CONTAIN || fop == JAG_FUNC_SAME || 
		fop == JAG_FUNC_INTERSECT || fop == JAG_FUNC_DISJOINT || fop == JAG_FUNC_NEARBY || 
		fop == JAG_FUNC_ALL || JAG_FUNC_PI ||
		fop == JAG_FUNC_AREA || fop == JAG_FUNC_CLOSESTPOINT || fop == JAG_FUNC_ANGLE || fop == JAG_FUNC_PERIMETER || 
		fop == JAG_FUNC_VOLUME || fop == JAG_FUNC_DIMENSION || fop == JAG_FUNC_GEOTYPE || fop == JAG_FUNC_POINTN || 
		fop == JAG_FUNC_EXTENT || fop == JAG_FUNC_STARTPOINT || fop == JAG_FUNC_ENDPOINT || 
		fop == JAG_FUNC_CONVEXHULL || fop == JAG_FUNC_CONCAVEHULL || 
		fop == JAG_FUNC_TOPOLYGON || fop == JAG_FUNC_TOMULTIPOINT  ||
		fop == JAG_FUNC_UNION || fop == JAG_FUNC_DIFFERENCE || fop == JAG_FUNC_SYMDIFFERENCE || 
		fop == JAG_FUNC_INTERSECTION || fop == JAG_FUNC_COLLECT || 
		fop == JAG_FUNC_POLYGONN || fop == JAG_FUNC_OUTERRING || fop == JAG_FUNC_OUTERRINGS || 
		fop == JAG_FUNC_INNERRINGS || fop == JAG_FUNC_RINGN || fop == JAG_FUNC_UNIQUE || 
		fop == JAG_FUNC_INNERRINGN || fop == JAG_FUNC_ASTEXT || fop == JAG_FUNC_BUFFER || 
		fop == JAG_FUNC_CENTROID || 
		fop == JAG_FUNC_XMINPOINT || fop == JAG_FUNC_XMAXPOINT || 
		fop == JAG_FUNC_YMINPOINT || fop == JAG_FUNC_YMAXPOINT || 
		fop == JAG_FUNC_ZMINPOINT || fop == JAG_FUNC_ZMAXPOINT || 
		fop == JAG_FUNC_ISCLOSED || fop == JAG_FUNC_ISSIMPLE || fop == JAG_FUNC_ISCONVEX || 
		fop == JAG_FUNC_ISVALID || fop == JAG_FUNC_ISRING || 
		fop == JAG_FUNC_ISONLEFT || fop == JAG_FUNC_ISONRIGHT || 
		fop == JAG_FUNC_LEFTRATIO || fop == JAG_FUNC_RIGHTRATIO || 
		fop == JAG_FUNC_ISPOLYGONCCW || fop == JAG_FUNC_ISPOLYGONCW || fop == JAG_FUNC_NUMPOINTS || 
		fop == JAG_FUNC_NUMSEGMENTS || fop == JAG_FUNC_NUMRINGS || fop == JAG_FUNC_NUMPOLYGONS || 
		fop == JAG_FUNC_NUMINNERRINGS || fop == JAG_FUNC_SRID || fop == JAG_FUNC_SUMMARY || 
		fop == JAG_FUNC_NUMLINES ||
		fop == JAG_FUNC_XMIN || fop == JAG_FUNC_YMIN || fop == JAG_FUNC_ZMIN || 
		fop == JAG_FUNC_XMAX || fop == JAG_FUNC_YMAX || fop == JAG_FUNC_ZMAX || 
		fop == JAG_FUNC_STRDIFF || fop == JAG_FUNC_TOSECOND || fop == JAG_FUNC_MILETOMETER || fop == JAG_FUNC_METERTOMILE ||
		fop == JAG_FUNC_MILETOKILOMETER || fop == JAG_FUNC_KILOMETERTOMILE ||
		fop == JAG_FUNC_TOMICROSECOND || fop == JAG_FUNC_LINELENGTH || fop == JAG_FUNC_INTERPOLATE || 
		fop == JAG_FUNC_LINESUBSTRING || fop == JAG_FUNC_REVERSE || fop == JAG_FUNC_TRANSLATE ||
		fop == JAG_FUNC_TRANSSCALE || fop == JAG_FUNC_AFFINE || fop == JAG_FUNC_VORONOIPOLYGONS ||
		fop == JAG_FUNC_VORONOILINES || fop == JAG_FUNC_DELAUNAYTRIANGLES ||
		fop == JAG_FUNC_ROTATE || fop == JAG_FUNC_ROTATESELF || fop == JAG_FUNC_ROTATEAT ||
		fop == JAG_FUNC_LOCATEPOINT || fop == JAG_FUNC_ADDPOINT || fop == JAG_FUNC_SETPOINT || 
		fop == JAG_FUNC_REMOVEPOINT || fop == JAG_FUNC_SCALE || fop == JAG_FUNC_SCALEAT ||
		fop == JAG_FUNC_DEGREES || fop == JAG_FUNC_RADIANS || fop == JAG_FUNC_SCALESIZE ||
		fop == JAG_FUNC_GEOJSON || fop == JAG_FUNC_WKT || fop == JAG_FUNC_KNN || fop == JAG_FUNC_METRICN ||
		fop == JAG_FUNC_MINIMUMBOUNDINGCIRCLE || fop == JAG_FUNC_MINIMUMBOUNDINGSPHERE ||
		fop == JAG_FUNC_LENGTH || fop == JAG_FUNC_COUNT ) {
			return true;
	}
	return false;
}  // end checkFuncType()

bool BinaryExpressionBuilder::getCalculationType( const char *p, short &fop, short &len, short &ctype )
{
	const char *q;
	int rc = 1; fop = 0; len = 0; ctype = 0;

	if ( JAG_NUM_ADD == short(*p) ) {
		fop = JAG_NUM_ADD;
		len = 1; ctype = 0;
	} else if ( JAG_STR_ADD == short(*p) ) {
		fop = JAG_STR_ADD;
		len = 1; ctype = 0;
	} else if ( JAG_NUM_SUB == short(*p) ) {
		fop = JAG_NUM_SUB;
		len = 1; ctype = 0;
	} else if ( JAG_NUM_MULT == short(*p) ) {
		fop = JAG_NUM_MULT;
		len = 1; ctype = 0;
	} else if ( JAG_NUM_DIV == short(*p) ) {
		fop = JAG_NUM_DIV;
		len = 1; ctype = 0;
	} else if ( JAG_NUM_REM == short(*p) ) {
		fop = JAG_NUM_REM;
		len = 1; ctype = 0;
	} else if ( JAG_NUM_POW == short(*p) ) {
		fop = JAG_NUM_POW;
		len = 1; ctype = 0;
	} else if ( *p == '<' ) {
		if ( *(p+1) == '=' ) {
			fop = JAG_FUNC_LESSEQUAL; len = 2;
		} else if ( *(p+1) == '>' ) {
			fop = JAG_FUNC_NOTEQUAL; len = 2;
		} else {
			fop = JAG_FUNC_LESSTHAN; len = 1;
		}
		ctype = 1;
	} else if ( *p == '>' ) {
		if ( *(p+1) == '=' ) {
			fop = JAG_FUNC_GREATEREQUAL; len = 2;
		} else if ( *(p+1) == '<' ) {
			fop = JAG_FUNC_NOTEQUAL; len = 2;
		} else {
			fop = JAG_FUNC_GREATERTHAN; len = 1;
		}
		ctype = 1;
	} else if ( *p == '!' ) {
		if ( *(p+1) == '=' ) { fop = JAG_FUNC_NOTEQUAL; len = 2; } else { throw 48; }
		ctype = 1;
	} else if ( *p == '=' ) {
		if ( *(p+1) == '=' ) { len = 2; } else { len = 1; }
		fop = JAG_FUNC_EQUAL; ctype = 1;
	} else if ( 0 == strncasecmp( p, "minimumboundingcircle", 21 ) ) {
		fop = JAG_FUNC_MINIMUMBOUNDINGCIRCLE; len = 21; ctype = 1;
	} else if ( 0 == strncasecmp( p, "minimumboundingsphere", 21 ) ) {
		fop = JAG_FUNC_MINIMUMBOUNDINGSPHERE; len = 21; ctype = 1;
	} else if ( 0 == strncasecmp( p, "like ", 5 ) ) {
		fop = JAG_FUNC_LIKE; len = 4; ctype = 1;
	} else if ( 0 == strncasecmp( p, "match ", 6 ) ) {
		fop = JAG_FUNC_MATCH; len = 5; ctype = 1;
	} else if ( 0 == strncasecmp( p, "not ", 4 ) ) {
		len = 3; ctype = 3;
	} else if ( 0 == strncasecmp(p, "max", 3 ) ) {
		fop = JAG_FUNC_MAX; len = 3; ctype = 2;
	} else if ( 0 == strncasecmp(p, "avg", 3 ) ) {
		fop = JAG_FUNC_AVG; len = 3; ctype = 2;
	} else if ( 0 == strncasecmp(p, "summary", 7 ) ) {
		fop = JAG_FUNC_SUMMARY; len = 7; ctype = 2;
	} else if ( 0 == strncasecmp(p, "sum", 3 ) ) {
		fop = JAG_FUNC_SUM; len = 3; ctype = 2;
	} else if ( 0 == strncasecmp(p, "count", 5 ) ) {
		fop = JAG_FUNC_COUNT; len = 5; ctype = 2;
	} else if ( 0 == strncasecmp(p, "stddev", 6 ) ) {
		fop = JAG_FUNC_STDDEV; len = 6; ctype = 2;
	} else if ( 0 == strncasecmp(p, "first", 5 ) ) {
		fop = JAG_FUNC_FIRST; len = 5; ctype = 2;
	} else if ( 0 == strncasecmp(p, "last", 4 ) ) {
		fop = JAG_FUNC_LAST; len = 4; ctype = 2;
	} else if ( 0 == strncasecmp(p, "abs", 3 ) ) {
		fop = JAG_FUNC_ABS; len = 3; ctype = 2;
	} else if ( 0 == strncasecmp(p, "acos", 4 ) ) {
		fop = JAG_FUNC_ACOS; len = 4; ctype = 2;
	} else if ( 0 == strncasecmp(p, "asin", 4 ) ) {
		fop = JAG_FUNC_ASIN; len = 4; ctype = 2;
	} else if ( 0 == strncasecmp(p, "atan", 4 ) ) {
		fop = JAG_FUNC_ATAN; len = 4; ctype = 2;
	} else if ( 0 == strncasecmp(p, "ceil", 4 ) ) {
		fop = JAG_FUNC_CEIL; len = 4; ctype = 2;
	} else if ( 0 == strncasecmp(p, "cos", 3 ) ) {
		fop = JAG_FUNC_COS; len = 3; ctype = 2;
	} else if ( 0 == strncasecmp(p, "strdiff", 7 ) ) {
		fop = JAG_FUNC_STRDIFF; len = 7; ctype = 2;
	} else if ( 0 == strncasecmp(p, "cot", 3 ) ) {
		fop = JAG_FUNC_COT; len = 3; ctype = 2;
	} else if ( 0 == strncasecmp(p, "floor", 5 ) ) {
		fop = JAG_FUNC_FLOOR; len = 5; ctype = 2;
	} else if ( 0 == strncasecmp(p, "log2", 4 ) ) {
		fop = JAG_FUNC_LOG2; len = 4; ctype = 2;
	} else if ( 0 == strncasecmp(p, "log10", 5 ) ) {
		fop = JAG_FUNC_LOG10; len = 5; ctype = 2;
	} else if ( 0 == strncasecmp(p, "log", 3 ) ) {
		fop = JAG_FUNC_LOG; len = 3; ctype = 2;
	} else if ( 0 == strncasecmp(p, "ln", 2 ) ) {
		fop = JAG_FUNC_LOG; len = 2; ctype = 2;
	} else if ( 0 == strncasecmp(p, "mod", 3 ) ) {
		fop = JAG_FUNC_MOD; len = 3; ctype = 2;
	} else if ( 0 == strncasecmp(p, "pow", 3 ) ) {
		fop = JAG_FUNC_POW; len = 3; ctype = 2;
	} else if ( 0 == strncasecmp(p, "radians", 7 ) ) {
		fop = JAG_FUNC_RADIANS; len = 7; ctype = 2;
	} else if ( 0 == strncasecmp(p, "degrees", 7 ) ) {
		fop = JAG_FUNC_DEGREES; len = 7; ctype = 2;
	} else if ( 0 == strncasecmp(p, "sin", 3 ) ) {
		fop = JAG_FUNC_SIN; len = 3; ctype = 2;
	} else if ( 0 == strncasecmp(p, "sqrt", 4 ) ) {
		fop = JAG_FUNC_SQRT; len = 4; ctype = 2;
	} else if ( 0 == strncasecmp(p, "tan", 3 ) ) {
		fop = JAG_FUNC_TAN; len = 3; ctype = 2;
	} else if ( 0 == strncasecmp(p, "substring", 9 ) ) {
		fop = JAG_FUNC_SUBSTR; len = 9; ctype = 2;
		if ( _substrClause >= 0 || _datediffClause >= 0 ) throw 9006;
		_substrClause = 0;
	} else if ( 0 == strncasecmp(p, "substr", 6 ) ) {
		fop = JAG_FUNC_SUBSTR; len = 6; ctype = 2;
		if ( _substrClause >= 0 || _datediffClause >= 0 ) throw 9007;
		_substrClause = 0;
	} else if ( 0 == strncasecmp(p, "upper", 5 ) ) {
		fop = JAG_FUNC_UPPER; len = 5; ctype = 2;
	} else if ( 0 == strncasecmp(p, "lower", 5 ) ) {
		fop = JAG_FUNC_LOWER; len = 5; ctype = 2;
	} else if ( 0 == strncasecmp(p, "ltrim", 5 ) ) {
		fop = JAG_FUNC_LTRIM; len = 5; ctype = 2;
	} else if ( 0 == strncasecmp(p, "rtrim", 5 ) ) {
		fop = JAG_FUNC_RTRIM; len = 5; ctype = 2;
	} else if ( 0 == strncasecmp(p, "trim", 4 ) ) {
		fop = JAG_FUNC_TRIM; len = 4; ctype = 2;
	} else if ( 0 == strncasecmp(p, "length", 6 ) ) {
		fop = JAG_FUNC_LENGTH; len = 6; ctype = 2;
	} else if ( 0 == strncasecmp(p, "linelength", 10 ) ) {
		fop = JAG_FUNC_LINELENGTH; len = 10; ctype = 2;
	} else if ( 0 == strncasecmp(p, "second", 6 ) ) {
		fop = JAG_FUNC_SECOND; len = 6; ctype = 2;
	} else if ( 0 == strncasecmp(p, "minute", 6 ) ) {
		fop = JAG_FUNC_MINUTE; len = 6; ctype = 2;
	} else if ( 0 == strncasecmp(p, "min", 3 ) ) {
		fop = JAG_FUNC_MIN; len = 3; ctype = 2;
	} else if ( 0 == strncasecmp(p, "hour", 4 ) ) {
		fop = JAG_FUNC_HOUR; len = 4; ctype = 2;
	} else if ( 0 == strncasecmp(p, "datediff", 8 ) ) {
		fop = JAG_FUNC_DATEDIFF; len = 8; ctype = 2;
		if ( _substrClause >= 0 || _datediffClause >= 0 ) throw 95;
		_datediffClause = 0;
	} else if ( 0 == strncasecmp(p, "month", 5 ) ) {
		fop = JAG_FUNC_MONTH; len = 5; ctype = 2;
	} else if ( 0 == strncasecmp(p, "year", 4 ) ) {
		fop = JAG_FUNC_YEAR; len = 4; ctype = 2;
	} else if ( 0 == strncasecmp(p, "date", 4 ) ) {
		fop = JAG_FUNC_DATE; len = 4; ctype = 2;
	} else if ( 0 == strncasecmp(p, "dayofmonth", 10 ) ) {
		fop = JAG_FUNC_DAYOFMONTH; len = 10; ctype = 2;
	} else if ( 0 == strncasecmp(p, "dayofweek", 9 ) ) {
		fop = JAG_FUNC_DAYOFWEEK; len = 9; ctype = 2;
	} else if ( 0 == strncasecmp(p, "dayofyear", 9 ) ) {
		fop = JAG_FUNC_DAYOFYEAR; len = 9; ctype = 2;
	} else if ( 0 == strncasecmp(p, "day", 3 ) ) {
		fop = JAG_FUNC_DAY; len = 3; ctype = 2;
	} else if ( 0 == strncasecmp(p, "curdate", 7 ) ) {
		fop = JAG_FUNC_CURDATE; len = 7; ctype = 2;
	} else if ( 0 == strncasecmp(p, "curtime", 7 ) ) {
		fop = JAG_FUNC_CURTIME; len = 7; ctype = 2;
	} else if ( 0 == strncasecmp(p, "now", 3 ) ) {
		fop = JAG_FUNC_NOW; len = 3; ctype = 2;
	} else if ( 0 == strncasecmp(p, "time", 4 ) ) {
		fop = JAG_FUNC_TIME; len = 4; ctype = 2;
	} else if ( 0 == strncasecmp(p, "sysdate", 7 ) ) {
		fop = JAG_FUNC_NOW; len = 7; ctype = 2;
	} else if ( 0 == strncasecmp(p, "distance", 8 ) ) {
		fop = JAG_FUNC_DISTANCE; len = 8; ctype = 2;
	} else if ( 0 == strncasecmp(p, "similarity", 10 ) ) {
		fop = JAG_FUNC_SIMILARITY; len = 10; ctype = 2;
	} else if ( 0 == strncasecmp(p, "within", 6 ) ) {
		fop = JAG_FUNC_WITHIN; len = 6; ctype = 2;
	} else if ( 0 == strncasecmp(p, "coveredby", 9 ) ) {
		fop = JAG_FUNC_COVEREDBY; len = 9; ctype = 2;
	} else if ( 0 == strncasecmp(p, "cover", 5 ) ) {
		fop = JAG_FUNC_COVER; len = 5; ctype = 2;
	} else if ( 0 == strncasecmp(p, "contain", 7 ) ) {
		fop = JAG_FUNC_CONTAIN; len = 7; ctype = 2;
	} else if ( 0 == strncasecmp(p, "same", 4 ) ) {
		fop = JAG_FUNC_SAME; len = 4; ctype = 2;
	} else if ( 0 == strncasecmp(p, "equal", 5 ) ) {
		fop = JAG_FUNC_SAME; len = 5; ctype = 2;
	} else if ( 0 == strncasecmp(p, "intersection", 12 ) ) {
		fop = JAG_FUNC_INTERSECTION; len = 12; ctype = 2;
	} else if ( 0 == strncasecmp(p, "intersect", 9 ) ) {
		fop = JAG_FUNC_INTERSECT; len = 9; ctype = 2;
	} else if ( 0 == strncasecmp(p, "closestpoint", 12 ) ) {
		fop = JAG_FUNC_CLOSESTPOINT; len = 12; ctype = 2;
	} else if ( 0 == strncasecmp(p, "disjoint", 8 ) ) {
		fop = JAG_FUNC_DISJOINT; len = 8; ctype = 2;
	} else if ( 0 == strncasecmp(p, "nearby", 6 ) ) {
		fop = JAG_FUNC_NEARBY; len = 6; ctype = 2;
	} else if ( 0 == strncasecmp(p, "all", 3 ) ) {
		fop = JAG_FUNC_ALL; len = 3; ctype = 2;
	} else if ( 0 == strncasecmp(p, "tosecond", 8 ) ) {
		fop = JAG_FUNC_TOSECOND; len = 8; ctype = 2;
	} else if ( 0 == strncasecmp(p, "tomicrosecond", 13 ) ) {
		fop = JAG_FUNC_TOMICROSECOND; len = 13; ctype = 2;
	} else if ( 0 == strncasecmp(p, "miletometer", 11 ) ) {
		fop = JAG_FUNC_MILETOMETER; len = 11; ctype = 2;
	} else if ( 0 == strncasecmp(p, "miletokilometer", 15 ) ) {
		fop = JAG_FUNC_MILETOKILOMETER; len = 15; ctype = 2;
	} else if ( 0 == strncasecmp(p, "metertomile", 11 ) ) {
		fop = JAG_FUNC_METERTOMILE; len = 11; ctype = 2;
	} else if ( 0 == strncasecmp(p, "kilometertomile", 15 ) ) {
		fop = JAG_FUNC_KILOMETERTOMILE; len = 15; ctype = 2;
	} else if ( 0 == strncasecmp(p, "area", 4 ) ) {
		fop = JAG_FUNC_AREA; len = 4; ctype = 2;
	} else if ( 0 == strncasecmp(p, "perimeter", 9 ) ) {
		fop = JAG_FUNC_PERIMETER; len = 9; ctype = 2;
	} else if ( 0 == strncasecmp(p, "angle", 5 ) ) {
		fop = JAG_FUNC_ANGLE; len = 5; ctype = 2;
	} else if ( 0 == strncasecmp(p, "volume", 6 ) ) {
		fop = JAG_FUNC_VOLUME; len = 6; ctype = 2;
	} else if ( 0 == strncasecmp(p, "dimension", 9 ) ) {
		fop = JAG_FUNC_DIMENSION; len = 9; ctype = 2;
	} else if ( 0 == strncasecmp(p, "geotype", 7 ) ) {
		fop = JAG_FUNC_GEOTYPE; len = 7; ctype = 2;
	} else if ( 0 == strncasecmp(p, "pointn", 6 ) ) {
		fop = JAG_FUNC_POINTN; len = 6; ctype = 2;
	} else if ( 0 == strncasecmp(p, "startpoint", 10 ) ) {
		fop = JAG_FUNC_STARTPOINT; len = 10; ctype = 2;
	} else if ( 0 == strncasecmp(p, "endpoint", 8 ) ) {
		fop = JAG_FUNC_ENDPOINT; len = 8; ctype = 2;
	} else if ( 0 == strncasecmp(p, "convexhull", 10 ) ) {
		fop = JAG_FUNC_CONVEXHULL; len = 10; ctype = 2;
	} else if ( 0 == strncasecmp(p, "concavehull", 11 ) ) {
		fop = JAG_FUNC_CONCAVEHULL; len = 11; ctype = 2;
	} else if ( 0 == strncasecmp(p, "topolygon", 9 ) ) {
		fop = JAG_FUNC_TOPOLYGON; len = 9; ctype = 2;
	} else if ( 0 == strncasecmp(p, "tomultipoint", 12 ) ) {
		fop = JAG_FUNC_TOMULTIPOINT; len = 12; ctype = 2;
	} else if ( 0 == strncasecmp(p, "union", 5 ) ) {
		fop = JAG_FUNC_UNION; len = 5; ctype = 2;
	} else if ( 0 == strncasecmp(p, "difference", 10 ) ) {
		fop = JAG_FUNC_DIFFERENCE; len = 10; ctype = 2;
	} else if ( 0 == strncasecmp(p, "symdifference", 13 ) ) {
		fop = JAG_FUNC_SYMDIFFERENCE; len = 13; ctype = 2;
	} else if ( 0 == strncasecmp(p, "collect", 7 ) ) {
		fop = JAG_FUNC_COLLECT; len = 7; ctype = 2;
	} else if ( 0 == strncasecmp(p, "polygonn", 8 ) ) {
		fop = JAG_FUNC_POLYGONN; len = 8; ctype = 2;
	} else if ( 0 == strncasecmp(p, "innerrings", 10 ) ) {
		fop = JAG_FUNC_INNERRINGS; len = 10; ctype = 2;
	} else if ( 0 == strncasecmp(p, "outerrings", 10 ) ) {
		fop = JAG_FUNC_OUTERRINGS; len = 10; ctype = 2;
	} else if ( 0 == strncasecmp(p, "outerring", 9 ) ) {
		fop = JAG_FUNC_OUTERRING; len = 9; ctype = 2;
	} else if ( 0 == strncasecmp(p, "ringn", 5 ) ) {
		fop = JAG_FUNC_RINGN; len = 5; ctype = 2;
	} else if ( 0 == strncasecmp(p, "metricn", 7 ) ) {
		fop = JAG_FUNC_METRICN; len = 7; ctype = 2;
	} else if ( 0 == strncasecmp(p, "interpolate", 11 ) ) {
		fop = JAG_FUNC_INTERPOLATE; len = 11; ctype = 2;
	} else if ( 0 == strncasecmp(p, "linesubstring", 13 ) ) {
		fop = JAG_FUNC_LINESUBSTRING; len = 13; ctype = 2;
	} else if ( 0 == strncasecmp(p, "locatepoint", 11 ) ) {
		fop = JAG_FUNC_LOCATEPOINT; len = 11; ctype = 2;
	} else if ( 0 == strncasecmp(p, "astext", 6 ) ) {
		fop = JAG_FUNC_ASTEXT; len = 6; ctype = 2;
	} else if ( 0 == strncasecmp(p, "text", 4 ) ) {
		fop = JAG_FUNC_ASTEXT; len = 4; ctype = 2;
	} else if ( 0 == strncasecmp(p, "unique", 6 ) ) {
		fop = JAG_FUNC_UNIQUE; len = 6; ctype = 2;
	} else if ( 0 == strncasecmp(p, "innerringn", 10 ) ) {
		fop = JAG_FUNC_INNERRINGN; len = 10; ctype = 2;
	} else if ( 0 == strncasecmp(p, "buffer", 6 ) ) {
		fop = JAG_FUNC_BUFFER; len = 6; ctype = 2;
	} else if ( 0 == strncasecmp(p, "centroid", 8 ) ) {
		fop = JAG_FUNC_CENTROID; len = 8; ctype = 2;
	} else if ( 0 == strncasecmp(p, "extent", 6 ) ) {
		fop = JAG_FUNC_EXTENT; len = 6; ctype = 2;
	} else if ( 0 == strncasecmp(p, "envelope", 8 ) ) {
		fop = JAG_FUNC_EXTENT; len = 8; ctype = 2;
	} else if ( 0 == strncasecmp(p, "isclosed", 8 ) ) {
		fop = JAG_FUNC_ISCLOSED; len = 8; ctype = 2;
	} else if ( 0 == strncasecmp(p, "issimple", 8 ) ) {
		fop = JAG_FUNC_ISSIMPLE; len = 8; ctype = 2;
	} else if ( 0 == strncasecmp(p, "isvalid", 7 ) ) {
		fop = JAG_FUNC_ISVALID; len = 7; ctype = 2;
	} else if ( 0 == strncasecmp(p, "isring", 6 ) ) {
		fop = JAG_FUNC_ISRING; len = 6; ctype = 2;
	} else if ( 0 == strncasecmp(p, "ispolygonccw", 12 ) ) {
		fop = JAG_FUNC_ISPOLYGONCCW; len = 12; ctype = 2;
	} else if ( 0 == strncasecmp(p, "ispolygoncw", 11 ) ) {
		fop = JAG_FUNC_ISPOLYGONCW; len = 11; ctype = 2;
	} else if ( 0 == strncasecmp(p, "isconvex", 8 ) ) {
		fop = JAG_FUNC_ISCONVEX; len = 8; ctype = 2;
	} else if ( 0 == strncasecmp(p, "isonleft", 8 ) ) {
		fop = JAG_FUNC_ISONLEFT; len = 8; ctype = 2;
	} else if ( 0 == strncasecmp(p, "isonright", 9 ) ) {
		fop = JAG_FUNC_ISONRIGHT; len = 9; ctype = 2;
	} else if ( 0 == strncasecmp(p, "leftratio", 9 ) ) {
		fop = JAG_FUNC_LEFTRATIO; len = 9; ctype = 2;
	} else if ( 0 == strncasecmp(p, "rightratio", 10 ) ) {
		fop = JAG_FUNC_RIGHTRATIO; len = 10; ctype = 2;
	} else if ( 0 == strncasecmp(p, "numpoints", 9 ) ) {
		fop = JAG_FUNC_NUMPOINTS; len = 9; ctype = 2;
	} else if ( 0 == strncasecmp(p, "numsegments", 11 ) ) {
		fop = JAG_FUNC_NUMSEGMENTS; len = 11; ctype = 2;
	} else if ( 0 == strncasecmp(p, "numlines", 8 ) ) {
		fop = JAG_FUNC_NUMLINES; len = 8; ctype = 2;
	} else if ( 0 == strncasecmp(p, "numrings", 8 ) ) {
		fop = JAG_FUNC_NUMRINGS; len = 8; ctype = 2;
	} else if ( 0 == strncasecmp(p, "numpolygons", 11 ) ) {
		fop = JAG_FUNC_NUMPOLYGONS; len = 11; ctype = 2;
	} else if ( 0 == strncasecmp(p, "numinnerrings", 13 ) ) {
		fop = JAG_FUNC_NUMINNERRINGS; len = 13; ctype = 2;
	} else if ( 0 == strncasecmp(p, "srid", 4 ) ) {
		fop = JAG_FUNC_SRID; len = 4; ctype = 2;
	} else if ( 0 == strncasecmp(p, "xminpoint", 9 ) ) {
		fop = JAG_FUNC_XMINPOINT; len = 9; ctype = 2;
	} else if ( 0 == strncasecmp(p, "yminpoint", 9 ) ) {
		fop = JAG_FUNC_YMINPOINT; len = 9; ctype = 2;
	} else if ( 0 == strncasecmp(p, "zminpoint", 9 ) ) {
		fop = JAG_FUNC_ZMINPOINT; len = 9; ctype = 2;
	} else if ( 0 == strncasecmp(p, "xmaxpoint", 9 ) ) {
		fop = JAG_FUNC_XMAXPOINT; len = 9; ctype = 2;
	} else if ( 0 == strncasecmp(p, "ymaxpoint", 9 ) ) {
		fop = JAG_FUNC_YMAXPOINT; len = 9; ctype = 2;
	} else if ( 0 == strncasecmp(p, "zmaxpoint", 9 ) ) {
		fop = JAG_FUNC_ZMAXPOINT; len = 9; ctype = 2;
	} else if ( 0 == strncasecmp(p, "xmin", 4 ) ) {
		fop = JAG_FUNC_XMIN; len = 4; ctype = 2;
	} else if ( 0 == strncasecmp(p, "ymin", 4 ) ) {
		fop = JAG_FUNC_YMIN; len = 4; ctype = 2;
	} else if ( 0 == strncasecmp(p, "zmin", 4 ) ) {
		fop = JAG_FUNC_ZMIN; len = 4; ctype = 2;
	} else if ( 0 == strncasecmp(p, "xmax", 4 ) ) {
		fop = JAG_FUNC_XMAX; len = 4; ctype = 2;
	} else if ( 0 == strncasecmp(p, "ymax", 4 ) ) {
		fop = JAG_FUNC_YMAX; len = 4; ctype = 2;
	} else if ( 0 == strncasecmp(p, "zmax", 4 ) ) {
		fop = JAG_FUNC_ZMAX; len = 4; ctype = 2;
	} else if ( 0 == strncasecmp(p, "addpoint", 8 ) ) {
		fop = JAG_FUNC_ADDPOINT; len = 8; ctype = 2;
	} else if ( 0 == strncasecmp(p, "setpoint", 8 ) ) {
		fop = JAG_FUNC_SETPOINT; len = 8; ctype = 2;
	} else if ( 0 == strncasecmp(p, "removepoint", 11 ) ) {
		fop = JAG_FUNC_REMOVEPOINT; len = 11; ctype = 2;
	} else if ( 0 == strncasecmp(p, "reverse", 7 ) ) {
		fop = JAG_FUNC_REVERSE; len = 7; ctype = 2;
	} else if ( 0 == strncasecmp(p, "scaleat", 7 ) ) {
		fop = JAG_FUNC_SCALEAT; len = 7; ctype = 2;
	} else if ( 0 == strncasecmp(p, "scalesize", 9 ) ) {
		fop = JAG_FUNC_SCALESIZE; len = 9; ctype = 2;
	} else if ( 0 == strncasecmp(p, "scale", 5 ) ) {
		fop = JAG_FUNC_SCALE; len = 5; ctype = 2;
	} else if ( 0 == strncasecmp(p, "translate", 9 ) ) {
		fop = JAG_FUNC_TRANSLATE; len = 9; ctype = 2;
	} else if ( 0 == strncasecmp(p, "transscale", 10 ) ) {
		fop = JAG_FUNC_TRANSSCALE; len = 10; ctype = 2;
	} else if ( 0 == strncasecmp(p, "rotateself", 10 ) ) {
		fop = JAG_FUNC_ROTATESELF; len = 10; ctype = 2;
	} else if ( 0 == strncasecmp(p, "rotateat", 8 ) ) {
		fop = JAG_FUNC_ROTATEAT; len = 8; ctype = 2;
	} else if ( 0 == strncasecmp(p, "rotate", 6 ) ) {
		fop = JAG_FUNC_ROTATE; len = 6; ctype = 2;
	} else if ( 0 == strncasecmp(p, "pi", 2 ) ) {
		fop = JAG_FUNC_PI; len = 2; ctype = 2;
	} else if ( 0 == strncasecmp(p, "affine", 6 ) ) {
		fop = JAG_FUNC_AFFINE; len = 6; ctype = 2;
	} else if ( 0 == strncasecmp(p, "voronoipolygons", 15 ) ) {
		fop = JAG_FUNC_VORONOIPOLYGONS; len = 15; ctype = 2;
	} else if ( 0 == strncasecmp(p, "voronoilines", 12 ) ) {
		fop = JAG_FUNC_VORONOILINES; len = 12; ctype = 2;
	} else if ( 0 == strncasecmp(p, "delaunaytriangles", 17 ) ) {
		fop = JAG_FUNC_DELAUNAYTRIANGLES; len = 17; ctype = 2;
	} else if ( 0 == strncasecmp(p, "geojson", 7 ) ) {
		fop = JAG_FUNC_GEOJSON; len = 7; ctype = 2;
	} else if ( 0 == strncasecmp(p, "wkt", 3 ) ) {
		fop = JAG_FUNC_WKT; len = 3; ctype = 2;
	} else if ( 0 == strncasecmp(p, "knn", 3 ) ) {
		fop = JAG_FUNC_KNN; len = 3; ctype = 2;
	} else {
		rc = 0;
	}

	if ( 2 == ctype ) {
		q = p+len;
		while ( isspace(*q) ) ++q;
		if ( *q != '(' ) {
			rc = 0;
			if ( JAG_FUNC_SUBSTR == fop ) _substrClause = -1;
			else if ( JAG_FUNC_DATEDIFF == fop ) _datediffClause = -1;
		} else len = q-p+1;
	}
	
	if ( _substrClause >= 0 || _datediffClause >= 0 ) {
		if ( rc != 0 && fop != JAG_FUNC_SUBSTR && fop != JAG_FUNC_DATEDIFF ) throw 94;
	}

	if ( 0 == _datediffClause && fop != JAG_FUNC_DATEDIFF ) {
		if ( JAG_FUNC_YEAR != fop && JAG_FUNC_MONTH != fop && JAG_FUNC_HOUR != fop && JAG_FUNC_DATE != fop &&
			JAG_FUNC_MINUTE != fop && JAG_FUNC_SECOND != fop && 0 != strncasecmp(p, "day", 3 ) ) {
			throw 3293;
		}
	}

	return rc;
}

bool BinaryExpressionBuilder::nameConvertionCheck( Jstr &name, int &tabnum,
													const JagHashMap<AbaxString, AbaxPair<AbaxString, jagint>> &cmap, 
													Jstr &colList )
{
	dn("p028371 nameConvertionCheck name=[%s]", name.s() );

	JagStrSplit sp( name, '.' );
	AbaxPair<AbaxString, jagint> pair;	

	if ( !cmap.getValue( "0", pair ) ) {
        dn("p901778 return 0");
		return 0; 
	}

	Jstr fpath = pair.key.c_str();
	jagint totnum = pair.value;

    dn("p27808 fpath=[%s] totnum=%ld", fpath.s(), totnum );

	if ( sp.length() == 1 && totnum > 1 ) {
        dn("p222009 sp.length() == 1 && totnum=%ld > 1  return 0", totnum ); 
        return 0;
    }

	if ( 1 == totnum ) {
		if ( sp.length() == 1 ) {
			name = fpath +  "." + sp[0];
		} else if ( sp.length() == 2 ) {
			name = fpath +  "." + sp[1];
		} else {
			name = fpath +  "." + sp[2];
		}
		tabnum = 0;
		if ( colList.length() < 1 ) colList = name;
		else colList += Jstr("|") + name;
		return 1;
	}

	fpath = sp[0];
	for ( int i = 1; i < sp.length()-1; ++i ) {
		fpath += Jstr(".") + sp[i];
	}

	if ( !cmap.getValue( fpath, pair ) ) {
        dn("0172828 return 0");
		return 0;
	}

	fpath = pair.key.c_str();
	name = fpath + "." + sp[sp.length()-1];
	tabnum = pair.value;

	if ( colList.length() < 1 ) colList = name;
	else colList += Jstr("|") + name;

	return 1;
}

bool BinaryExpressionBuilder::nameAndOpGood( const JagParser *jpsr, const Jstr &fullname, 
											 const StringElementNode &lastNode )
{
	if ( ! jpsr ) return true;

	Jstr colType;
	JagStrSplit sp(fullname, '.' );
	if ( sp.length() == 3 ) {
		const JagColumn* jcol = jpsr->getColumn( sp[0], sp[1], sp[2] );
		if ( ! jcol ) return true;
		colType = jcol->type;
	}

	if ( colType.size() > 0 && lastNode._name.size() > 0 && BinaryOpNode::isCompareOp(_lastOp) ) {
		JagStrSplit sp1( lastNode._name, '.' );
		if ( sp1.length() == 3 ) {
			const JagColumn* prevJcol = jpsr->getColumn( sp1[JAG_SP_START+0], sp1[JAG_SP_START+1], sp1[JAG_SP_START+2] );
			if ( ! prevJcol ) return true;

			if ( colType != prevJcol->type ) {
				return false;
			}
		}
	}

	return true;
}

bool BinaryOpNode::processBooleanOp( int op, const JagFixString &inlstr, const JagFixString &inrstr, 
											const Jstr &carg )
{
    dn("s1780023 processBooleanOp op=%d inlstr=[%s]", op, inlstr.s() );
    dn("s1780023 processBooleanOp op=%d inrstr=[%s]", op, inrstr.s() );

	if ( inlstr.size() < 1 ) {
        dn("s22220000");
        return false;
    }

	Jstr lstr;
	if ( !strnchr( inlstr.c_str(), '=', 8 ) ) {
		int rc1 = JagParser::convertConstantObjToJAG( inlstr, lstr );
		if ( rc1 <= 0 ) {
			lstr = Jstr("OJAG=0=0=") + inlstr.dtype + " 0:0:0:0 " + inlstr.s();
            dn("s3082001 lstr set to [%s] dtype=[%s]", lstr.s(), inlstr.dtype );
		}
	} else {
		lstr = inlstr.c_str();
        dn("s352117 inlstr has =, copy it to lstr");
	}

    Jstr rstr;
    if ( !strnchr( inrstr.c_str(), '=', 8 ) ) {
        int rc2 = JagParser::convertConstantObjToJAG( inrstr, rstr );
        if ( rc2 <= 0 ) {
            dn("s22588720000 false");
			return false;
		}
    } else {
        rstr = inrstr.c_str();
    }

	Jstr colobjstr1 = lstr.firstToken(' ');
	Jstr colobjstr2 = rstr.firstToken(' ');
	JagStrSplit spcol2(colobjstr2, 5, '=');  
	Jstr colType2;

	Jstr colType1;
	JagStrSplit spcol1(colobjstr1, 5, '='); 
	int srid1 = 0;
	Jstr mark1, colName1;

	if ( spcol1.length() < 4 ) {
		if ( spcol2.length() >= 4 ) {
			colType2 = spcol2[3];
			if ( colType2 == JAG_C_COL_TYPE_RANGE ) {
				if (  _left && _left->_isElement ) {
					mark1 = JAG_OJAG;
					srid1 = _left->_srid;
					colName1 = _left->_name;
					colType1 = _left->_type;
				} else {
					mark1 = JAG_OJAG;
					srid1 = 0;
					colName1 = "dummy";
					colType1 = "dummy";
				}
			} else {
                dn("s22520000 false");
				return false;
			}
		} else {
			mark1 = JAG_OJAG;
			srid1 = 0;
			colName1 = "dummy";
			colType1 = "dummy";
		}
	}

	if ( spcol1.length() >= 4 ) {
		mark1 = spcol1[0];
		srid1 = jagatoi( spcol1[1].c_str() );
		colName1 = spcol1[2];
		colType1 = spcol1[3];
	}

	int srid2 = 0;
	Jstr mark2, colName2;
	if ( spcol2.length() < 4 ) {
        dn("s22520050 false");
		return false;
	}

	mark2 = spcol2[0];
	srid2 = jagatoi( spcol2[1].c_str() );
	colName2 = spcol2[2];
	colType2 = spcol2[3];

	if ( mark2 == JAG_OJAG && mark1  == JAG_OJAG && srid1 != srid2 ) {
        dn("s25529050 false");
		return false;
	}

	if ( mark2 == JAG_OJAG && mark1 == JAG_CJAG ) {
		srid1 = srid2;
	} else if (  mark2 == JAG_CJAG && mark1 == JAG_OJAG ) {
		srid2 = srid1;
	}

	JagStrSplit sp1( lstr.c_str(), ' ', true );
	JagStrSplit sp2( rstr.c_str(), ' ', true );
    dn("s3333378 lstr=[%s]", lstr.s() );
    dn("s3333378 rstr=[%s]", rstr.s() );
	bool rc = doBooleanOp( op, mark1, colType1, srid1, sp1, mark2, colType2, srid2, sp2, carg );
	return rc;
}


#ifdef JAG_SERVER_SIDE
Jstr  BinaryOpNode::processTwoStrOp( int op, const JagFixString &inlstr, const JagFixString &inrstr, const Jstr &carg )
{
	if ( inlstr.size() < 1 ) return "";
	Jstr lstr;
	if ( !strnchr( inlstr.c_str(), '=', 8 ) ) {
		int rc1 = JagParser::convertConstantObjToJAG( inlstr, lstr );
		if ( rc1 <= 0 ) {
			return "";
		}
	} else {
		lstr = inlstr.c_str();
	}

	JagStrSplit sp1( lstr.c_str(), ' ', true );
	Jstr colobjstr1 = lstr.firstToken(' ');
	Jstr colType1;
	JagStrSplit spcol1(colobjstr1, 5, '='); 
	Jstr mark1, colName1;
	int srid1 = 0;
	if ( spcol1.length() >= 4 ) {
		mark1 = spcol1[0];
		srid1 = jagatoi( spcol1[1].c_str() );
		colName1 = spcol1[2];
		colType1 = spcol1[3];
	}

    Jstr rstr;
    if ( !strnchr( inrstr.c_str(), '=', 8 ) ) {
        int rc2 = JagParser::convertConstantObjToJAG( inrstr, rstr );
        if ( rc2 < 0 ) {
			return "";
		} else if ( 0 == rc2 ) {
			if ( op == JAG_FUNC_VORONOIPOLYGONS || op == JAG_FUNC_VORONOILINES ) {
				if ( colType1 ==  JAG_C_COL_TYPE_MULTIPOINT ) {
					JagBox2D b2;
					JagGeo::bbox2D( sp1, b2 );
					double dx = fabs(b2.xmax-b2.xmin)*0.3;
					double dy = fabs(b2.ymax-b2.ymin)*0.3;
					b2.xmin -= dx; b2.ymin -= dx;
					b2.xmax += dx; b2.ymax += dy;
					rstr = "CJAG=" + intToStr(srid1) + "=0=BB=d " + d2s(b2.xmin) + ":" +  d2s(b2.ymin) 
					      + ":" +  d2s(b2.xmax) + ":" +  d2s(b2.ymax)
						  + " " + d2s(b2.xmin) + " " +  d2s(b2.ymin) 
						  + " " +  d2s(b2.xmax) + " " +  d2s(b2.ymax);
				} else if ( colType1 ==  JAG_C_COL_TYPE_MULTIPOINT3D ) {
					JagBox3D b3;
					JagGeo::bbox3D( sp1, b3 );
					double dx = fabs(b3.xmax-b3.xmin)*0.3;
					double dy = fabs(b3.ymax-b3.ymin)*0.3;
					double dz = fabs(b3.zmax-b3.zmin)*0.3;
					b3.xmin -= dx; b3.ymin -= dx; b3.zmin -= dz; 
					b3.xmax += dx; b3.ymax += dy; b3.zmax += dz;
					rstr = "CJAG=" + intToStr(srid1) + "=0=BB=d " + d2s(b3.xmin) + ":" +  d2s(b3.ymin) + ":" + d2s(b3.zmin) 
					      + ":" +  d2s(b3.xmax) + ":" +  d2s(b3.ymax) + ":" + d2s(b3.zmax)
						  + " " + d2s(b3.xmin) + " " +  d2s(b3.ymin) + " " + d2s(b3.zmin)
						  + " " +  d2s(b3.xmax) + " " +  d2s(b3.ymax) + " " + d2s(b3.zmax);
				} else {
					return "";
				}
			} else {
				return "";
			}
		}
    } else {
        rstr = inrstr.c_str();
    }

	Jstr colobjstr2 = rstr.firstToken(' ');
	JagStrSplit spcol2(colobjstr2, 5, '=');  
	Jstr colType2;

	if ( spcol1.length() < 4 ) {
		if ( spcol2.length() >= 4 ) {
			colType2 = spcol2[3];
			if ( colType2 == JAG_C_COL_TYPE_RANGE ) {
				if (  _left && _left->_isElement ) {
					mark1 = JAG_OJAG;
					srid1 = _left->_srid;
					colName1 = _left->_name;
					colType1 = _left->_type;
				} else {
					mark1 = JAG_OJAG;
					srid1 = 0;
					colName1 = "dummy";
					colType1 = "dummy";
				}
			} else {
				return false;
			}
		} else {
			mark1 = JAG_OJAG;
			srid1 = 0;
			colName1 = "dummy";
			colType1 = "dummy";
		}
	}

	int srid2 = 0;
	Jstr mark2, colName2; 

	if ( spcol2.length() < 4 ) {
		return "";
	}

	mark2 = spcol2[0];
	srid2 = jagatoi( spcol2[1].c_str() );
	colName2 = spcol2[2];
	colType2 = spcol2[3];

	if ( mark2 == JAG_OJAG && mark1  == JAG_OJAG && srid1 != srid2 ) {
		return "";
	}

	if ( mark2 == JAG_OJAG && mark1 == JAG_CJAG ) {
		srid1 = srid2;
	} else if (  mark2 == JAG_CJAG && mark1 == JAG_OJAG ) {
		srid2 = srid1;
	}

	JagStrSplit sp2( rstr.c_str(), ' ', true );
	return doTwoStrOp( op, mark1, colType1, srid1, sp1, mark2, colType2, srid2, sp2, carg );
}
#else
Jstr  BinaryOpNode::processTwoStrOp( int op, const JagFixString &inlstr, const JagFixString &inrstr, const Jstr &carg )
{
	return "0";
}
#endif

#ifdef JAG_SERVER_SIDE
bool BinaryOpNode::processSingleStrOp( int op, const JagFixString &inlstr, const Jstr &carg, Jstr &value )
{
	if ( inlstr.size() < 1 ) return false;
	Jstr lstr;
	if ( !strnchr( inlstr.c_str(), '=', 8 ) ) {
		int rc1 = JagParser::convertConstantObjToJAG( inlstr, lstr );
		if ( rc1 <= 0 ) return false;
	} else {
		lstr = inlstr.c_str();
	}

	Jstr colobjstr1 = lstr.firstToken(' ');
	Jstr colType1;
	JagStrSplit spcol1(colobjstr1, 5, '='); 
	int srid1 = 0;
	Jstr mark1, colName1; 

	if ( spcol1.length() >= 4 ) {
		mark1 = spcol1[0];
		srid1 = jagatoi( spcol1[1].c_str() );
		colName1 = spcol1[2];
		colType1 = spcol1[3];
	}

	JagStrSplit sp1( lstr.c_str(), ' ', true );
	Jstr hdr = sp1[0];
	bool rc = doSingleStrOp( op, mark1, hdr, colType1, srid1, sp1, carg, value );
	return rc;
}
#else
bool BinaryOpNode::processSingleStrOp( int op, const JagFixString &inlstr, const Jstr &carg, Jstr &value )
{
	return false;
}
#endif


#ifdef JAG_SERVER_SIDE
bool BinaryOpNode::processSingleDoubleOp( int op, const JagFixString &inlstr, const Jstr &carg, double &value )
{
	if ( inlstr.size() < 1 ) return false;
	Jstr lstr;
	if ( !strnchr( inlstr.c_str(), '=', 8 ) ) {
		int rc1 = JagParser::convertConstantObjToJAG( inlstr, lstr );
		if ( rc1 <= 0 ) return false;
	} else {
		lstr = inlstr.c_str();
	}

	Jstr colobjstr1 = lstr.firstToken(' ');
	Jstr colType1;
	JagStrSplit spcol1(colobjstr1, 5, '=');
	int srid1 = 0;
	Jstr mark1, colName1;

	if ( spcol1.length() >= 4 ) {
		mark1 = spcol1[0];
		srid1 = jagatoi( spcol1[1].c_str() );
		colName1 = spcol1[2];
		colType1 = spcol1[3];
	}

	JagStrSplit sp1( lstr.c_str(), ' ', true );
	bool rc = doSingleDoubleOp( op, mark1, colType1, srid1, sp1, carg, value );
	return rc;
}
#else
bool BinaryOpNode::processSingleDoubleOp( int op, const JagFixString &inlstr, const Jstr &carg, double &value )
{
	return false;
}
#endif

#ifdef JAG_SERVER_SIDE
bool BinaryOpNode::doSingleDoubleOp( int op, const Jstr& mark1, const Jstr &colType1, int srid1, 
										const JagStrSplit &sp1, const Jstr &carg, double &value )
{
	bool rc = false;
	if ( op == JAG_FUNC_AREA ) {
		rc = doAllArea( mark1, colType1, srid1, sp1, value );
	} else if ( op == JAG_FUNC_XMIN || op == JAG_FUNC_YMIN || op == JAG_FUNC_ZMIN
	            || op == JAG_FUNC_XMAX || op == JAG_FUNC_YMAX || op == JAG_FUNC_ZMAX ) {
		rc = doAllMinMax( op, srid1, mark1, colType1, sp1, value );
	} else if (  op == JAG_FUNC_VOLUME ) {
		rc = doAllVolume( mark1, colType1, srid1, sp1, value );
	} else if (  op == JAG_FUNC_PERIMETER ) {
		rc = doAllPerimeter( mark1, colType1, srid1, sp1, value );
	} else {
	}
	return rc;
}
#else
bool BinaryOpNode::doSingleDoubleOp( int op, const Jstr& mark1, const Jstr &colType1, int srid1, 
										const JagStrSplit &sp1, const Jstr &carg, double &value )
{
	return false;
}
#endif

#ifdef JAG_SERVER_SIDE
bool BinaryOpNode::doSingleStrOp( int op, const Jstr& mark1, const Jstr& hdr, const Jstr &colType1, int srid1, 
										JagStrSplit &sp1, const Jstr &carg, Jstr &value )
{
	bool rc = false;
	if ( op == JAG_FUNC_POINTN ) {
		rc = doAllPointN( mark1, colType1, srid1, sp1, carg, value );
	} else if ( op == JAG_FUNC_EXTENT ) {
		rc = doAllExtent( srid1, mark1, colType1, sp1, value );
	} else if ( op == JAG_FUNC_STARTPOINT ) {
		rc = doAllStartPoint( mark1, colType1, sp1, value );
	} else if ( op == JAG_FUNC_ENDPOINT ) {
		rc = doAllEndPoint( mark1, colType1, sp1, value );
	} else if ( op == JAG_FUNC_ISCLOSED ) {
		rc = doAllIsClosed( mark1, colType1, sp1, value );
	} else if ( op == JAG_FUNC_ISSIMPLE ) {
		rc = doAllIsSimple( mark1, colType1, sp1, value );
	} else if ( op == JAG_FUNC_ISVALID ) {
		rc = doAllIsValid( mark1, colType1, sp1, value );
	} else if ( op == JAG_FUNC_ISRING ) {
		rc = doAllIsRing( mark1, colType1, sp1, value );
	} else if ( op == JAG_FUNC_ISPOLYGONCCW ) {
		rc = doAllIsPolygonCCW( mark1, colType1, sp1, value );
	} else if ( op == JAG_FUNC_ISPOLYGONCW ) {
		rc = doAllIsPolygonCW( mark1, colType1, sp1, value );
	} else if ( op == JAG_FUNC_ISCONVEX ) {
		rc = doAllIsConvex( mark1, colType1, sp1, value );
	} else if ( op == JAG_FUNC_NUMPOINTS ) {
		rc = doAllNumPoints( mark1, colType1, sp1, value );
	} else if ( op == JAG_FUNC_NUMSEGMENTS ) {
		rc = doAllNumSegments( mark1, colType1, sp1, value );
	} else if ( op == JAG_FUNC_NUMLINES ) {
		rc = doAllNumLines( mark1, colType1, sp1, value );
	} else if ( op == JAG_FUNC_NUMRINGS ) {
		rc = doAllNumRings( mark1, colType1, sp1, value );
	} else if ( op == JAG_FUNC_NUMINNERRINGS ) {
		rc = doAllNumInnerRings( mark1, colType1, sp1, value );
	} else if ( op == JAG_FUNC_SRID ) {
		value = intToStr(srid1);
		rc = true;
	} else if ( op == JAG_FUNC_SUMMARY ) {
		rc = doAllSummary( mark1, colType1, srid1, sp1, value );
	} else if ( op == JAG_FUNC_CONVEXHULL ) {
		rc = doAllConvexHull( mark1, hdr, colType1, srid1, sp1, value );
	} else if ( op == JAG_FUNC_CONCAVEHULL ) {
		rc = doAllConcaveHull( mark1, hdr, colType1, srid1, sp1, value );
	} else if ( op == JAG_FUNC_OUTERRING ) {
		rc = doAllOuterRing( mark1, hdr, colType1, srid1, sp1, value );
	} else if ( op == JAG_FUNC_UNIQUE ) {
		rc = doAllUnique( mark1, hdr, colType1, sp1, value );
	} else if ( op == JAG_FUNC_OUTERRINGS ) {
		rc = doAllOuterRings( mark1, hdr, colType1, srid1, sp1, value );
	} else if ( op == JAG_FUNC_INNERRINGS ) {
		rc = doAllInnerRings( mark1, hdr, colType1, srid1, sp1, value );
	} else if ( op == JAG_FUNC_RINGN ) {
		rc = doAllRingN( mark1, hdr, colType1, srid1, sp1, carg, value );
	} else if ( op == JAG_FUNC_METRICN ) {
		rc = doAllMetricN( mark1, hdr, colType1, srid1, sp1, carg, value );
	} else if ( op == JAG_FUNC_GEOJSON ) {
		rc = doAllGeoJson( mark1, colType1, srid1, sp1, carg, value );
	} else if ( op == JAG_FUNC_WKT ) {
		rc = doAllWKT( mark1, colType1, srid1, sp1, carg, value );
	} else if ( op == JAG_FUNC_MINIMUMBOUNDINGCIRCLE ) {
		rc = doAllMinimumBoundingCircle( mark1, colType1, srid1, sp1, carg, value );
	} else if ( op == JAG_FUNC_MINIMUMBOUNDINGSPHERE ) {
		rc = doAllMinimumBoundingSphere( mark1, colType1, srid1, sp1, carg, value );
	} else if ( op == JAG_FUNC_INNERRINGN ) {
		rc = doAllInnerRingN( mark1, hdr, colType1, srid1, sp1, carg, value );
	} else if ( op == JAG_FUNC_POLYGONN ) {
		rc = doAllPolygonN( mark1, hdr, colType1, sp1, carg, value );
	} else if ( op == JAG_FUNC_NUMPOLYGONS ) {
		rc = doAllNumPolygons( mark1, hdr, colType1, sp1, value );
	} else if ( op == JAG_FUNC_BUFFER ) {
		rc = doAllBuffer( mark1, hdr, colType1, srid1, sp1, carg, value );
	} else if ( op == JAG_FUNC_CENTROID ) {
		rc = doAllCentroid( mark1, hdr, colType1, srid1, sp1, value );
	} else if ( op == JAG_FUNC_XMINPOINT || op == JAG_FUNC_XMAXPOINT
	              || op == JAG_FUNC_YMINPOINT || op == JAG_FUNC_YMAXPOINT
				  || op == JAG_FUNC_ZMINPOINT || op == JAG_FUNC_ZMAXPOINT ) {
		rc = doAllMinMaxPoint( op, mark1, hdr, colType1, srid1, sp1, value );
	} else if ( op == JAG_FUNC_TOPOLYGON ) {
		rc = doAllToPolygon( mark1, hdr, colType1, srid1, sp1, carg, value );
	} else if ( op == JAG_FUNC_TOMULTIPOINT ) {
		rc = doAllToMultipoint( mark1, hdr, colType1, srid1, sp1, carg, value );
	} else if ( op == JAG_FUNC_ASTEXT ) {
		rc = doAllAsText( mark1, hdr, colType1, srid1, sp1, carg, value );
	} else if ( op == JAG_FUNC_INTERPOLATE ) {
		rc = doAllInterpolate( srid1, colType1, sp1, carg, value );
	} else if ( op == JAG_FUNC_LINESUBSTRING ) {
		rc = doAllLineSubstring( srid1, colType1, sp1, carg, value );
	} else if ( op == JAG_FUNC_REMOVEPOINT ) {
		rc = doAllRemovePoint( srid1, colType1, sp1, carg, value );
	} else if ( op == JAG_FUNC_REVERSE ) {
		rc = doAllReverse( srid1, colType1, sp1, carg, value );
	} else if ( op == JAG_FUNC_SCALE ) {
		rc = doAllScale( srid1, colType1, sp1, carg, value );
	} else if ( op == JAG_FUNC_SCALESIZE ) {
		rc = doAllScaleSize( srid1, colType1, sp1, carg, value );
	} else if ( op == JAG_FUNC_TRANSLATE ) {
		rc = doAllTranslate( srid1, colType1, sp1, carg, value );
	} else if ( op == JAG_FUNC_TRANSSCALE ) {
		rc = doAllTransScale( srid1, colType1, sp1, carg, value );
	} else if ( op == JAG_FUNC_ROTATE || op == JAG_FUNC_ROTATEAT ) {
		rc = doAllRotateAt( srid1, colType1, sp1, carg, value );
	} else if ( op == JAG_FUNC_ROTATESELF ) {
		rc = doAllRotateSelf( srid1, colType1, sp1, carg, value );
	} else if ( op == JAG_FUNC_AFFINE ) {
		rc = doAllAffine( srid1, colType1, sp1, carg, value );
	} else if ( op == JAG_FUNC_DELAUNAYTRIANGLES ) {
		rc = doAllDelaunayTriangles( srid1, colType1, sp1, carg, value );
	} else {
	}
	return rc;
}
#else
bool BinaryOpNode::doSingleStrOp( int op, const Jstr& mark1, const Jstr& hdr, const Jstr &colType1, int srid1, 
										JagStrSplit &sp1, const Jstr &carg, Jstr &value )
{
	return false;
}

#endif


#ifdef JAG_SERVER_SIDE
bool BinaryOpNode::doBooleanOp( int op, const Jstr& mark1, const Jstr &colType1, int srid1, 
										const JagStrSplit &sp1, const Jstr& mark2, 
										const Jstr &colType2, int srid2, const JagStrSplit &sp2, 
										const Jstr &carg )
{
    dn("s392250 doBooleanOp mark1=%s colType1=%s  srid1=%d", mark1.s(), colType1.s(), srid1 );
    dn("s392250 doBooleanOp mark2=%s colType2=%s  srid2=%d carg=[%s]", mark2.s(), colType2.s(), srid2, carg.s() );
	bool rc = false;
	if ( srid1 != srid2 ) {
        dn("s33333081 false");
		return rc;
	}

    dn("s223309 op=%d", op );

	if ( op == JAG_FUNC_WITHIN ) {
		rc = doAllWithin( mark1, colType1, srid1, sp1, mark2, colType2, srid2, sp2, true );
	} else if ( op == JAG_FUNC_COVEREDBY ) {
		rc = doAllWithin( mark1, colType1, srid1, sp1, mark2, colType2, srid2, sp2, false );
	} else if ( op == JAG_FUNC_CONTAIN ) {
		rc = doAllWithin( mark2, colType2, srid2, sp2, mark1, colType1, srid1, sp1,  true );
	} else if ( op == JAG_FUNC_SAME ) {
		rc = doAllSame( mark1, colType1, srid1, sp1, mark2, colType2, srid2, sp2 );
	} else if ( op == JAG_FUNC_COVER ) {
		rc = doAllWithin( mark2, colType2, srid2, sp2, mark1, colType1, srid1, sp1,  false );
	} else if ( op == JAG_FUNC_INTERSECT ) {
        dn("s122028 JAG_FUNC_INTERSECT ...");
		rc = doAllIntersect( mark1, colType1, srid1, sp1, mark2, colType2, srid2, sp2, false );
	} else if ( op == JAG_FUNC_DISJOINT ) {
		rc = ! doAllIntersect( mark1, colType1, srid1, sp1, mark2, colType2, srid2, sp2, false );
	} else if ( op == JAG_FUNC_NEARBY ) {
		rc = doAllNearby( mark1, colType1, srid1, sp1, mark2, colType2, srid2, sp2, carg );
	} else {
	}
	return rc;
}
#else
bool BinaryOpNode::doBooleanOp( int op, const Jstr& mark1, const Jstr &colType1, int srid1, 
										const JagStrSplit &sp1, const Jstr& mark2, 
										const Jstr &colType2, int srid2, const JagStrSplit &sp2, 
										const Jstr &carg ) 
{ 
	return false; 
}
#endif


#ifdef JAG_SERVER_SIDE
Jstr BinaryOpNode::doTwoStrOp( int op, const Jstr& mark1, const Jstr &colType1, int srid1, 
										JagStrSplit &sp1, const Jstr& mark2, 
										const Jstr &colType2, int srid2, JagStrSplit &sp2, 
										const Jstr &carg )
{
    dn("s2810001 doTwoStrOp");

	if ( srid1 != srid2 ) {
		return "";
	}

	bool rc;
	Jstr res;
	if ( op == JAG_FUNC_CLOSESTPOINT ) {
		if ( colType1 == JAG_C_COL_TYPE_POINT || colType1 == JAG_C_COL_TYPE_POINT3D ) {
			rc = doAllClosestPoint( mark1, colType1, srid1, sp1, mark2, colType2, srid2, sp2, res );
		} else if ( colType2 == JAG_C_COL_TYPE_POINT || colType2 == JAG_C_COL_TYPE_POINT3D ) {
			rc = doAllClosestPoint( mark2, colType2, srid2, sp2, mark1, colType1, srid1, sp1, res );
		} else {
			return "";
		}
		if ( ! rc ) return ""; else return res;
	} else if ( op == JAG_FUNC_ANGLE ) {
		if ( colType1 == JAG_C_COL_TYPE_LINE && colType2 == JAG_C_COL_TYPE_LINE ) {
			rc = doAllAngle2D( mark1, colType1, srid1, sp1, mark2, colType2, srid2, sp2, res );
		} else if ( colType1 == JAG_C_COL_TYPE_LINE3D && colType2 == JAG_C_COL_TYPE_LINE3D ) {
			rc = doAllAngle3D( mark1, colType1, srid1, sp1, mark2, colType2, srid2, sp2, res );
		} else {
			return "";
		}
		if ( ! rc ) return ""; else return res;
	} else if ( op == JAG_FUNC_UNION ) {
		return doAllUnion( mark1, colType1, srid1, sp1, mark2, colType2, srid2, sp2 );
	} else if ( op == JAG_FUNC_COLLECT ) {
		return doAllCollect( mark1, colType1, srid1, sp1, mark2, colType2, srid2, sp2 );
	} else if ( op == JAG_FUNC_INTERSECTION ) {
		return doAllIntersection( srid1, mark1, colType1, sp1, mark2, colType2, sp2 );
	} else if ( op == JAG_FUNC_DIFFERENCE ) {
		return doAllDifference( srid1, mark1, colType1, sp1, mark2, colType2, sp2 );
	} else if ( op == JAG_FUNC_SYMDIFFERENCE ) {
		return doAllSymDifference( srid1, mark1, colType1, sp1, mark2, colType2, sp2 );
	} else if ( op == JAG_FUNC_LOCATEPOINT ) {
		return doAllLocatePoint( srid1, mark1, colType1, sp1, mark2, colType2, sp2 );
	} else if ( op == JAG_FUNC_ADDPOINT ) {
		return doAllAddPoint( srid1, mark1, colType1, sp1, mark2, colType2, sp2, carg );
	} else if ( op == JAG_FUNC_SETPOINT ) {
		return doAllSetPoint( srid1, mark1, colType1, sp1, mark2, colType2, sp2, carg );
	} else if ( op == JAG_FUNC_SCALEAT ) {
		if ( colType2 == JAG_C_COL_TYPE_POINT || colType2 == JAG_C_COL_TYPE_POINT3D ) {
			return doAllScaleAt( srid1, mark1, colType1, sp1, mark2, colType2, sp2, carg );
		} else {
			return "";
		}
	} else if ( op == JAG_FUNC_VORONOIPOLYGONS ) {
		return doAllVoronoiPolygons( srid1, colType1, sp1, colType2, sp2, carg );
	} else if ( op == JAG_FUNC_VORONOILINES ) {
		return doAllVoronoiLines( srid1, colType1, sp1, colType2, sp2, carg );
	} else if ( op == JAG_FUNC_ISONLEFT ) {
		return doAllIsOnLeftSide( srid1, colType1, sp1, colType2, sp2, carg );
	} else if ( op == JAG_FUNC_ISONRIGHT ) {
		return doAllIsOnRightSide( srid1, colType1, sp1, colType2, sp2, carg );
	} else if ( op == JAG_FUNC_LEFTRATIO ) {
		return doAllLeftRatio( srid1, colType1, sp1, colType2, sp2, carg );
	} else if ( op == JAG_FUNC_RIGHTRATIO ) {
		return doAllRightRatio( srid1, colType1, sp1, colType2, sp2, carg );
	} else if ( op == JAG_FUNC_KNN ) {
		if ( colType2 == JAG_C_COL_TYPE_POINT || colType2 == JAG_C_COL_TYPE_POINT3D ) {
			return doAllKNN( srid1, colType1, sp1, colType2, sp2, carg );
		} else if ( colType1 == JAG_C_COL_TYPE_POINT || colType1 == JAG_C_COL_TYPE_POINT3D ) {
			return doAllKNN( srid2, colType2, sp2, colType1, sp1, carg );
		}
	} else {
		return "";
	}
	return "";
}
#else
Jstr BinaryOpNode::doTwoStrOp( int op, const Jstr& mark1, const Jstr &colType1, int srid1, 
										JagStrSplit &sp1, const Jstr& mark2, 
										const Jstr &colType2, int srid2, JagStrSplit &sp2, 
										const Jstr &carg ) { return ""; }
#endif

bool BinaryOpNode::isAggregateOp( short op )
{
    if (op == JAG_FUNC_MIN || op == JAG_FUNC_MAX || op == JAG_FUNC_AVG || op == JAG_FUNC_COUNT ||
        op == JAG_FUNC_SUM || op == JAG_FUNC_STDDEV || op == JAG_FUNC_FIRST || op == JAG_FUNC_LAST ) {
        return true;
    }

    return false;
}


bool BinaryOpNode::isMathOp( short op ) 
{
	if (JAG_NUM_ADD == op || JAG_NUM_SUB == op || JAG_NUM_MULT == op || JAG_STR_ADD == op || 
		JAG_NUM_DIV == op || JAG_NUM_REM == op || JAG_NUM_POW == op) {
		return true;
	}

	return false;
}

bool BinaryOpNode::isCompareOp( short op ) 
{
	if (JAG_FUNC_EQUAL == op || JAG_FUNC_NOTEQUAL == op || JAG_FUNC_LESSTHAN == op || JAG_FUNC_LESSEQUAL == op || 
		JAG_FUNC_GREATERTHAN == op || JAG_FUNC_GREATEREQUAL == op || JAG_FUNC_LIKE == op || JAG_FUNC_MATCH == op) {
		return true;
	}
	return false;
}	

bool BinaryOpNode::isStringOp( short op )
{
	if (op == JAG_FUNC_SUBSTR || op == JAG_FUNC_UPPER || op == JAG_FUNC_LOWER || 
		op == JAG_FUNC_LTRIM || op == JAG_FUNC_RTRIM || op == JAG_FUNC_TRIM) {
		return true;
	}
	return false;
}

bool BinaryOpNode::isSpecialOp( short op )
{
	if (op == JAG_FUNC_MIN || op == JAG_FUNC_MAX || op == JAG_FUNC_FIRST || op == JAG_FUNC_LAST || op == JAG_FUNC_LENGTH) {
		return true;
	}
	return false;
}

bool BinaryOpNode::isTimedateOp( short op )
{
	if (op == JAG_FUNC_SECOND || op == JAG_FUNC_MINUTE || op == JAG_FUNC_HOUR || op == JAG_FUNC_DAY || 
		op == JAG_FUNC_MONTH || op == JAG_FUNC_YEAR || op == JAG_FUNC_DATE || 
		op == JAG_FUNC_DATEDIFF || op == JAG_FUNC_DAYOFMONTH || op == JAG_FUNC_DAYOFWEEK || op == JAG_FUNC_DAYOFYEAR) {
		return true;
	}
	
	return false;
}

// it has all cases in isTimedateOp
int BinaryOpNode::timeDateOpLen ( short op )
{
    int len = -1;
    switch (op)
    {
        case JAG_FUNC_SECOND:
            len = 2;
            break;

        case JAG_FUNC_MINUTE:
            len = 2;
            break;

        case JAG_FUNC_HOUR:
            len = 2;
            break;

        case JAG_FUNC_DAY:
            len = 2;
            break;

        case JAG_FUNC_MONTH:
            len = 2;
            break;

        case JAG_FUNC_YEAR:
            len = 4;
            break;

        case JAG_FUNC_DATE:
            len = 10;
            break;

        case JAG_FUNC_DATEDIFF:
            len = 11; // max diff in seconds 
            break;

        case JAG_FUNC_DAYOFMONTH:
            len = 2;
            break;

        case JAG_FUNC_DAYOFWEEK:
            len = 1;
            break;

        case JAG_FUNC_DAYOFYEAR:
            len = 3;
            break;
    };

    return len;
}


short BinaryOpNode::getFuncLength( short op ) 
{
	if ( JAG_FUNC_CURDATE == op ) {
		return JAG_FUNC_CURDATE_LEN;
	} else if ( JAG_FUNC_CURTIME == op ) {
		return JAG_FUNC_CURTIME_LEN;
	} else if ( JAG_FUNC_NOW == op ) {
		return JAG_FUNC_NOW_LEN;
	} else if ( JAG_FUNC_TIME == op ) {
		return JAG_FUNC_NOW_LEN;
	} else if ( JAG_FUNC_PI == op ) {
		return JAG_DOUBLE_FIELD_LEN;
	} else {
		return JAG_FUNC_EMPTYARG_LEN;
	}
}

int BinaryOpNode::getTypeMode( short fop )
{
	int tmode = 0;
	if (  fop ==  JAG_FUNC_NUMPOINTS 
		  || fop == JAG_FUNC_NUMSEGMENTS
		  || fop == JAG_FUNC_NUMLINES
		  || fop == JAG_FUNC_NUMRINGS
		  || fop == JAG_FUNC_NUMPOLYGONS
		  || fop == JAG_FUNC_NUMINNERRINGS
		  || fop == JAG_FUNC_LENGTH
		  || fop == JAG_FUNC_LINELENGTH
		  || fop == JAG_FUNC_DIMENSION
		  || fop == JAG_FUNC_STRDIFF
		  || fop == JAG_FUNC_SRID
		  || fop == JAG_FUNC_ISCLOSED
		  || fop == JAG_FUNC_ISSIMPLE
		  || fop == JAG_FUNC_ISVALID
		  || fop == JAG_FUNC_ISRING
		  || fop == JAG_FUNC_ISPOLYGONCCW
		  || fop == JAG_FUNC_ISPOLYGONCW
		  || fop == JAG_FUNC_ISCONVEX
		  || fop == JAG_FUNC_ISONLEFT
		  || fop == JAG_FUNC_ISONRIGHT
		  || fop == JAG_FUNC_WITHIN
		  || fop == JAG_FUNC_CONTAIN
		  || fop == JAG_FUNC_NEARBY
		  || fop == JAG_FUNC_SAME
		  || fop == JAG_FUNC_COVEREDBY
		  || fop == JAG_FUNC_DISJOINT
		  || fop == JAG_FUNC_INTERSECT
		  || fop == JAG_FUNC_COVER
	   ) {
	   tmode = 1; 
	 } else if ( fop ==  JAG_FUNC_DISTANCE
	 			 || fop == JAG_FUNC_SIMILARITY
	 			 || fop == JAG_FUNC_ANGLE
	 			 || fop == JAG_FUNC_AREA
	 			 || fop == JAG_FUNC_PERIMETER
	 			 || fop == JAG_FUNC_VOLUME
	 			 || fop == JAG_FUNC_XMIN
	 			 || fop == JAG_FUNC_YMIN
	 			 || fop == JAG_FUNC_ZMIN
	 			 || fop == JAG_FUNC_XMAX
	 			 || fop == JAG_FUNC_YMAX
	 			 || fop == JAG_FUNC_ZMAX
	 			 || fop == JAG_FUNC_LOCATEPOINT
	 			 || fop == JAG_FUNC_LEFTRATIO
	 			 || fop == JAG_FUNC_RIGHTRATIO
	           ) {
	   tmode = 2;
	 } else {
	 }

	 return tmode;
}

#ifdef JAG_SERVER_SIDE
bool BinaryOpNode::doAllAngle2D( const Jstr& mark1, const Jstr &colType1, int srid1, 
								 JagStrSplit &sp1, const Jstr& mark2, 
								 const Jstr &colType2, int srid2, JagStrSplit &sp2, Jstr &res )
{
	double x1, y1, x2, y2;
	x1 = jagatof( sp1[JAG_SP_START+0].c_str() );
	y1 = jagatof( sp1[JAG_SP_START+1].c_str() );
	x2 = jagatof( sp1[JAG_SP_START+2].c_str() );
	y2 = jagatof( sp1[JAG_SP_START+3].c_str() );

	double p1, q1, p2, q2;
	p1 = jagatof( sp2[JAG_SP_START+0].c_str() );
	q1 = jagatof( sp2[JAG_SP_START+1].c_str() );
	p2 = jagatof( sp2[JAG_SP_START+2].c_str() );
	q2 = jagatof( sp2[JAG_SP_START+3].c_str() );

	double vx = x2-x1;
	double vy = y2-y1;
	double wx = p2-p1;
	double wy = q2-q1;
	if ( jagEQ(vx, wx) && jagEQ(vy, wy) ) {
		res = "0.0";
		return true;
	}

	long double t = ( vx*wx + vy*wy ) / ( sqrt(vx*vx+vy*vy) * sqrt( wx*wx+wy*wy ));
	res = doubleToStr( acosl(t) * 180.0/ JAG_PI );
	return true;
}

bool BinaryOpNode::doAllAngle3D( const Jstr& mark1, const Jstr &colType1, int srid1, 
								 JagStrSplit &sp1, const Jstr& mark2, 
								 const Jstr &colType2, int srid2, JagStrSplit &sp2, Jstr &res )
{
	double x1, y1, z1, x2, y2, z2;
	x1 = jagatof( sp1[JAG_SP_START+0].c_str() );
	y1 = jagatof( sp1[JAG_SP_START+1].c_str() );
	z1 = jagatof( sp1[JAG_SP_START+2].c_str() );
	x2 = jagatof( sp1[JAG_SP_START+3].c_str() );
	y2 = jagatof( sp1[JAG_SP_START+4].c_str() );
	z2 = jagatof( sp1[JAG_SP_START+5].c_str() );

	double p1, q1, r1,  p2, q2, r2;
	p1 = jagatof( sp2[JAG_SP_START+0].c_str() );
	q1 = jagatof( sp2[JAG_SP_START+1].c_str() );
	r1 = jagatof( sp2[JAG_SP_START+2].c_str() );
	p2 = jagatof( sp2[JAG_SP_START+3].c_str() );
	q2 = jagatof( sp2[JAG_SP_START+4].c_str() );
	r2 = jagatof( sp2[JAG_SP_START+5].c_str() );

	double vx = x2-x1;
	double vy = y2-y1;
	double vz = z2-z1;
	double wx = p2-p1;
	double wy = q2-q1;
	double wz = r2-r1;
	if ( jagEQ(vx, wx) && jagEQ(vy, wy) && jagEQ(vz, wz) ) {
		res = "0.0";
		return true;
	}

	long double t = ( vx*wx + vy*wy + vz*wz ) / ( sqrt(vx*vx+vy*vy+vz*vz) * sqrt(wx*wx+wy*wy+wz*wz));
	res = doubleToStr( acosl(t) * 180.0/ JAG_PI );
	return true;
}

bool BinaryOpNode::doAllClosestPoint( const Jstr& mark1, const Jstr &colType1, int srid1, 
										const JagStrSplit &sp1, const Jstr& mark2, 
										const Jstr &colType2, int srid2, const JagStrSplit &sp2, Jstr &res )
{
	double px, py, pz;
	if ( mark1 == JAG_OJAG ) {
		if ( colType1 == JAG_C_COL_TYPE_POINT ) {
			JagStrSplit c( sp1[JAG_SP_START+0], ':' );
			px = jagatof( c[0].c_str() );
			py = jagatof( c[1].c_str() );
		} else {
			JagStrSplit c( sp1[JAG_SP_START+0], ':' );
			px = jagatof( c[0].c_str() );
			py = jagatof( c[1].c_str() );
			pz = jagatof( c[2].c_str() );
		}
	} else {
		px = jagatof( sp1[JAG_SP_START+0].c_str() );
		py = jagatof( sp1[JAG_SP_START+1].c_str() );
		if ( colType1 == JAG_C_COL_TYPE_POINT3D ) {
			pz = jagatof( sp1[JAG_SP_START+3].c_str() );
		} 
	}

	bool rc =  JagGeo::doClosestPoint(colType1, srid1, px, py, pz, mark2, colType2, sp2, res  );
	return rc;
}

bool BinaryOpNode::doAllWithin( const Jstr& mark1, const Jstr &colType1, int srid1, 
										const JagStrSplit &sp1, const Jstr& mark2, 
										const Jstr &colType2, int srid2, const JagStrSplit &sp2, bool strict )
{
	if ( colType1 == JAG_C_COL_TYPE_POINT ) {
        dn("s03039960 colType1 == JAG_C_COL_TYPE_POINT doPointWithin ...");
		return JagGeo::doPointWithin(  sp1, mark2, colType2,  srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_POINT3D ) {
		return JagGeo::doPoint3DWithin(  sp1, mark2, colType2,  srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_CIRCLE ) {
		return JagGeo::doCircleWithin(  srid1, sp1, mark2, colType2,  srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_CIRCLE3D ) {
		return JagGeo::doCircle3DWithin(  srid1, sp1, mark2, colType2,  srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_SPHERE ) {
		return JagGeo::doSphereWithin(  srid1, sp1, mark2, colType2,  srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_SQUARE ) {
		return JagGeo::doSquareWithin(  srid1, sp1, mark2, colType2,  srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_SQUARE3D ) {
		return JagGeo::doSquare3DWithin(  srid1, sp1, mark2, colType2,  srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_CUBE ) {
		return JagGeo::doCubeWithin(  srid1, sp1, mark2, colType2,  srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_RECTANGLE ) {
		return JagGeo::doRectangleWithin(  srid1, sp1, mark2, colType2,  srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_RECTANGLE3D ) {
		return JagGeo::doRectangle3DWithin(  srid1, sp1, mark2, colType2,  srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_BOX ) {
		return JagGeo::doBoxWithin(  srid1, sp1, mark2, colType2,  srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_TRIANGLE ) {
		return JagGeo::doTriangleWithin(  srid1, sp1, mark2, colType2,  srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_TRIANGLE3D ) {
		return JagGeo::doTriangle3DWithin(  srid1, sp1, mark2, colType2,  srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_CYLINDER ) {
		return JagGeo::doCylinderWithin(  srid1, sp1, mark2, colType2,  srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_CONE ) {
		return JagGeo::doConeWithin(  srid1, sp1, mark2, colType2,  srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_ELLIPSE ) {
		return JagGeo::doEllipseWithin(  srid1, sp1, mark2, colType2,  srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_ELLIPSOID ) {
		return JagGeo::doEllipsoidWithin(  srid1, sp1, mark2, colType2,  srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_LINE ) {
		return JagGeo::doLineWithin(  srid1, sp1, mark2, colType2,  srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_LINE3D ) {
		return JagGeo::doLine3DWithin(  srid1, sp1, mark2, colType2,  srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING ) {
		return JagGeo::doLineStringWithin(  mark1, srid1, sp1, mark2, colType2,  srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING3D ) {
		return JagGeo::doLineString3DWithin(  mark1, srid1, sp1, mark2, colType2,  srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON ) {
		return JagGeo::doPolygonWithin(  mark1, srid1, sp1, mark2, colType2,  srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON3D ) {
		return JagGeo::doPolygon3DWithin(  mark1, srid1, sp1, mark2, colType2,  srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOINT ) {
		return JagGeo::doLineStringWithin(  mark1, srid1, sp1, mark2, colType2,  srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOINT3D ) {
		return JagGeo::doLineString3DWithin(  mark1, srid1, sp1, mark2, colType2,  srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_MULTILINESTRING ) {
		return JagGeo::doPolygonWithin(  mark1, srid1, sp1, mark2, colType2,  srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
		return JagGeo::doPolygon3DWithin(  mark1, srid1, sp1, mark2, colType2,  srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		return JagGeo::doMultiPolygonWithin(  mark1, srid1, sp1, mark2, colType2,  srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
		return JagGeo::doMultiPolygon3DWithin(  mark1, srid1, sp1, mark2, colType2,  srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_RANGE
	             || colType1 == JAG_C_COL_TYPE_DOUBLE 
	             || colType1 == JAG_C_COL_TYPE_LONGDOUBLE 
	             || colType1 == JAG_C_COL_TYPE_FLOAT
				 || isDateTime( colType1 )
	             || colType1 == JAG_C_COL_TYPE_DINT
	             || colType1 == JAG_C_COL_TYPE_DBIGINT
	             || colType1 == JAG_C_COL_TYPE_DSMALLINT
	             || colType1 == JAG_C_COL_TYPE_DTINYINT
	             || colType1 == JAG_C_COL_TYPE_DMEDINT
	             || colType1 == JAG_C_COL_TYPE_STR
			  ) {
		return JagRange::doRangeWithin(_jpa, mark1, colType1, srid1, sp1, mark2, colType2, srid2, sp2, strict);
	} else {
        dn("s9030123 colType1=[%s] not covered return false", colType1.s() );
    }
	return false;
}

bool BinaryOpNode::doAllSame( const Jstr& mark1, const Jstr &colType1, int srid1, 
								const JagStrSplit &sp1, const Jstr& mark2, 
								const Jstr &colType2, int srid2, const JagStrSplit &sp2 )
{
	if ( colType1 == JAG_C_COL_TYPE_POINT ) {
		return JagGeo::doPointSame(  sp1, mark2, colType2,  srid2, sp2);
	} else if ( colType1 == JAG_C_COL_TYPE_POINT3D ) {
		return JagGeo::doPoint3DSame(  sp1, mark2, colType2,  srid2, sp2);
	} else if ( colType1 == JAG_C_COL_TYPE_CIRCLE ) {
		return JagGeo::doCircleSame(  srid1, sp1, mark2, colType2,  srid2, sp2);
	} else if ( colType1 == JAG_C_COL_TYPE_CIRCLE3D ) {
		return JagGeo::doCircle3DSame(  srid1, sp1, mark2, colType2,  srid2, sp2);
	} else if ( colType1 == JAG_C_COL_TYPE_SPHERE ) {
		return JagGeo::doSphereSame(  srid1, sp1, mark2, colType2,  srid2, sp2);
	} else if ( colType1 == JAG_C_COL_TYPE_SQUARE ) {
		return JagGeo::doSquareSame(  srid1, sp1, mark2, colType2,  srid2, sp2);
	} else if ( colType1 == JAG_C_COL_TYPE_SQUARE3D ) {
		return JagGeo::doSquare3DSame(  srid1, sp1, mark2, colType2,  srid2, sp2);
	} else if ( colType1 == JAG_C_COL_TYPE_CUBE ) {
		return JagGeo::doCubeSame(  srid1, sp1, mark2, colType2,  srid2, sp2);
	} else if ( colType1 == JAG_C_COL_TYPE_RECTANGLE ) {
		return JagGeo::doRectangleSame(  srid1, sp1, mark2, colType2,  srid2, sp2);
	} else if ( colType1 == JAG_C_COL_TYPE_RECTANGLE3D ) {
		return JagGeo::doRectangle3DSame(  srid1, sp1, mark2, colType2,  srid2, sp2);
	} else if ( colType1 == JAG_C_COL_TYPE_BOX ) {
		return JagGeo::doBoxSame(  srid1, sp1, mark2, colType2,  srid2, sp2);
	} else if ( colType1 == JAG_C_COL_TYPE_TRIANGLE ) {
		return JagGeo::doTriangleSame(  srid1, sp1, mark2, colType2,  srid2, sp2);
	} else if ( colType1 == JAG_C_COL_TYPE_TRIANGLE3D ) {
		return JagGeo::doTriangle3DSame(  srid1, sp1, mark2, colType2,  srid2, sp2);
	} else if ( colType1 == JAG_C_COL_TYPE_CYLINDER ) {
		return JagGeo::doCylinderSame(  srid1, sp1, mark2, colType2,  srid2, sp2);
	} else if ( colType1 == JAG_C_COL_TYPE_CONE ) {
		return JagGeo::doConeSame(  srid1, sp1, mark2, colType2,  srid2, sp2);
	} else if ( colType1 == JAG_C_COL_TYPE_ELLIPSE ) {
		return JagGeo::doEllipseSame(  srid1, sp1, mark2, colType2,  srid2, sp2);
	} else if ( colType1 == JAG_C_COL_TYPE_ELLIPSOID ) {
		return JagGeo::doEllipsoidSame(  srid1, sp1, mark2, colType2,  srid2, sp2);
	} else if ( colType1 == JAG_C_COL_TYPE_LINE ) {
		return JagGeo::doLineSame(  srid1, sp1, mark2, colType2,  srid2, sp2);
	} else if ( colType1 == JAG_C_COL_TYPE_LINE3D ) {
		return JagGeo::doLine3DSame(  srid1, sp1, mark2, colType2,  srid2, sp2);
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING ) {
		return JagGeo::doLineStringSame(  mark1, srid1, sp1, mark2, colType2,  srid2, sp2);
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING3D ) {
		return JagGeo::doLineString3DSame(  mark1, srid1, sp1, mark2, colType2,  srid2, sp2);
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON ) {
		return JagGeo::doPolygonSame(  mark1, srid1, sp1, mark2, colType2,  srid2, sp2);
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON3D ) {
		return JagGeo::doPolygon3DSame(  mark1, srid1, sp1, mark2, colType2,  srid2, sp2);
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOINT ) {
		return JagGeo::doLineStringSame(  mark1, srid1, sp1, mark2, colType2,  srid2, sp2);
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOINT3D ) {
		return JagGeo::doLineString3DSame(  mark1, srid1, sp1, mark2, colType2,  srid2, sp2);
	} else if ( colType1 == JAG_C_COL_TYPE_MULTILINESTRING ) {
		return JagGeo::doPolygonSame(  mark1, srid1, sp1, mark2, colType2,  srid2, sp2);
	} else if ( colType1 == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
		return JagGeo::doPolygon3DSame(  mark1, srid1, sp1, mark2, colType2,  srid2, sp2);
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		return JagGeo::doMultiPolygonSame(  mark1, srid1, sp1, mark2, colType2,  srid2, sp2);
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
		return JagGeo::doMultiPolygon3DSame(  mark1, srid1, sp1, mark2, colType2,  srid2, sp2);
	} else if ( colType1 == JAG_C_COL_TYPE_RANGE ) {
		return JagRange::doRangeSame(_jpa, mark1, colType1, srid1, sp1, mark2, colType2, srid2, sp2);
	} 

	return false;
}

bool BinaryOpNode::doAllPointN( const Jstr& mk1, const Jstr &colType1, int srid1, 
									 JagStrSplit &sp1, const  Jstr &carg, Jstr &value )
{
	int narg = jagatoi( carg.c_str());
	if ( narg < 1 ) return false;
	int i = narg - 1;
	bool rc = false;
	value = "";
	if ( colType1 == JAG_C_COL_TYPE_POINT ) {
		if ( narg == 1 && sp1.length() >= 2 ) {
			value = sp1[JAG_SP_START+0].trim0() + " " + sp1[JAG_SP_START+1].trim0();
			rc =  true;
		} 
	} else if ( colType1 == JAG_C_COL_TYPE_POINT3D ) {
		if ( narg == 1 && sp1.length() >= 3 ) {
			value = sp1[JAG_SP_START+0].trim0() + " " + sp1[JAG_SP_START+1].trim0() + " " + sp1[JAG_SP_START+2].trim0();
			rc  = true;
		} 
	} else if ( colType1 == JAG_C_COL_TYPE_TRIANGLE ) {
		if (  narg >= 1 && narg <= 3 && sp1.length() >= 6 ) {
			value = sp1[JAG_SP_START+2*i].trim0() + " " + sp1[JAG_SP_START+2*i+1].trim0();
			rc = true;
		} 
	} else if ( colType1 == JAG_C_COL_TYPE_TRIANGLE3D ) {
		if (  narg >= 1 && narg <= 3 && sp1.length() >= 9 ) {
			value = sp1[JAG_SP_START+3*i].trim0() + " " + sp1[JAG_SP_START+3*i+1].trim0() + " " + sp1[JAG_SP_START+3*i+2].trim0();
			rc = true;
		} 
	} else if ( colType1 == JAG_C_COL_TYPE_LINE ) {
		if (  narg >= 1 && narg <= 2 && sp1.length() >= 4 ) {
			value = sp1[JAG_SP_START+2*i].trim0() + " " + sp1[JAG_SP_START+2*i+1].trim0();
			rc = true;
		} 
	} else if ( colType1 == JAG_C_COL_TYPE_LINE3D ) {
		if (  narg >= 1 && narg <= 2 && sp1.length() >= 6 ) {
			value = sp1[JAG_SP_START+3*i].trim0() + " " + sp1[JAG_SP_START+3*i+1].trim0() + " " + sp1[JAG_SP_START+3*i+2].trim0();
			rc = true;
		} 
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING || colType1 == JAG_C_COL_TYPE_MULTIPOINT ) {
		if (  narg >= 1 && narg <= sp1.length() ) {
			value = sp1[JAG_SP_START+i];
			value.replace( ':', ' ');
			rc = true;
		} 
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING3D || colType1 == JAG_C_COL_TYPE_MULTIPOINT3D ) {
		if (  narg >= 1 && narg <= sp1.length() ) {
			value = sp1[JAG_SP_START+i];
			value.replace( ':', ' ');
			rc = true;
		} 
	} 

	return rc;
}

bool BinaryOpNode::doAllExtent( int srid, const Jstr& mk, const Jstr &colType, const JagStrSplit &sp, Jstr &value )
{
    dn("s3581200 doAllExtent ...");

	bool rc = false;
	value = "";
	int nc = strchrnum( sp[1].c_str(), ':' );
	if ( nc < 3 ) return false;

	int dim = 2;
	if ( nc >= 5 ) dim = 3;
	double xmin,ymin,zmin,xmax,ymax,zmax;

	if ( 0 && mk == JAG_OJAG && sp[1] != "0:0:0:0" && sp[1] != "0:0:0:0:0:0" ) {
		Jstr bb = sp[1];
		JagStrSplit ss(bb, ':');
		if ( 2 == dim ) {
			xmin = ss[0].tof();
			ymin = ss[1].tof();
			xmax = ss[2].tof();
			ymax = ss[3].tof();
			value = bboxstr2D(srid, xmin, ymin, xmax, ymax);
		} else if ( 3 == dim ) {
			xmin = ss[0].tof();
			ymin = ss[1].tof();
			zmin = ss[2].tof();
			xmax = ss[3].tof();
			ymax = ss[4].tof();
			zmax = ss[5].tof();
			value = bboxstr3D(srid, xmin, ymin, zmin, xmax, ymax, zmax );
		} else return false;
		rc = true;
	} else {
		int rc2;
		if (  colType == JAG_C_COL_TYPE_POINT ) {
			xmin = xmax = sp[JAG_SP_START+0].tof();
			ymin = ymax = sp[JAG_SP_START+1].tof();
			value = bboxstr2D(srid, xmin, ymin, xmax, ymax);
		} else if ( colType == JAG_C_COL_TYPE_POINT3D ) {
			xmin = xmax = sp[JAG_SP_START+0].tof();
			ymin = ymax = sp[JAG_SP_START+1].tof();
			zmin = zmax = sp[JAG_SP_START+2].tof();
			value = bboxstr3D(srid, xmin, ymin, zmin, xmax, ymax, zmax );
		} else if ( colType == JAG_C_COL_TYPE_LINE ) {
			xmin = sp[JAG_SP_START+0].tof();
			ymin = sp[JAG_SP_START+1].tof();
			xmax = sp[JAG_SP_START+2].tof();
			ymax = sp[JAG_SP_START+3].tof();
			xmin = jagmin(xmin, xmax);
			xmax = jagmax(xmin, xmax);
			ymin = jagmin(ymin, ymax);
			ymax = jagmax(ymin, ymax);
			value = bboxstr2D(srid, xmin, ymin, xmax, ymax);
		} else if ( colType == JAG_C_COL_TYPE_LINE3D ) {
			xmin = sp[JAG_SP_START+0].tof();
			ymin = sp[JAG_SP_START+1].tof();
			zmin = sp[JAG_SP_START+2].tof();
			xmax = sp[JAG_SP_START+3].tof();
			ymax = sp[JAG_SP_START+4].tof();
			zmax = sp[JAG_SP_START+5].tof();
			xmin = jagmin(xmin, xmax);
			xmax = jagmax(xmin, xmax);
			ymin = jagmin(ymin, ymax);
			ymax = jagmax(ymin, ymax);
			zmin = jagmin(zmin, zmax);
			zmax = jagmax(zmin, zmax);
			value = bboxstr3D(srid, xmin, ymin, zmin, xmax, ymax, zmax );
		} else if ( colType == JAG_C_COL_TYPE_TRIANGLE ) {
			JagTriangle2D s( sp, srid );
			JagPolygon pgon( s );
			if ( pgon.bbox2D( xmin, ymin, xmax, ymax ) ) {
				value = bboxstr2D(srid, xmin, ymin, xmax, ymax);
			}
		} else if ( colType == JAG_C_COL_TYPE_TRIANGLE3D ) {
			JagTriangle3D s( sp, srid );
			JagPolygon pgon( s );
			if ( pgon.bbox3D( xmin, ymin, zmin, xmax, ymax, zmax ) ) {
				value = bboxstr3D(srid, xmin, ymin, zmin, xmax, ymax, zmax );
			}
		} else if ( colType == JAG_C_COL_TYPE_SQUARE ) {
			JagSquare2D sq( sp, srid );
			JagPolygon pgon( sq );
			if ( pgon.bbox2D( xmin, ymin, xmax, ymax ) ) {
				value = bboxstr2D(srid, xmin, ymin, xmax, ymax);
			}
		} else if ( colType == JAG_C_COL_TYPE_RECTANGLE ) {
			JagRectangle2D sq( sp, srid );
			JagPolygon pgon( sq );
			if ( pgon.bbox2D( xmin, ymin, xmax, ymax ) ) {
				value = bboxstr2D(srid, xmin, ymin, xmax, ymax);
			}
		} else if ( colType == JAG_C_COL_TYPE_SQUARE3D ) {
			JagRectangle3D sq( sp, srid );
			JagPolygon pgon( sq );
			if ( pgon.bbox3D( xmin, ymin, zmin, xmax, ymax, zmax ) ) {
				value = bboxstr3D(srid, xmin, ymin, zmin, xmax, ymax, zmax );
			}
		} else if ( colType == JAG_C_COL_TYPE_CIRCLE ) {
			JagCircle2D sq( sp, srid );
			JagPolygon pgon( sq );
			if ( pgon.bbox2D( xmin, ymin, xmax, ymax ) ) {
				value = bboxstr2D(srid, xmin, ymin, xmax, ymax);
			}
		} else if ( colType == JAG_C_COL_TYPE_CIRCLE3D ) {
			JagCircle3D sq( sp, srid );
			JagPolygon pgon( sq );
			if ( pgon.bbox3D( xmin, ymin, zmin, xmax, ymax, zmax ) ) {
				value = bboxstr3D(srid, xmin, ymin, zmin, xmax, ymax, zmax );
			}
		} else if ( colType == JAG_C_COL_TYPE_ELLIPSE ) {
			JagEllipse2D e( sp, srid );
			e.bbox2D( xmin, ymin, xmax, ymax );
			value = bboxstr2D(srid, xmin, ymin, xmax, ymax);
		} else if ( colType == JAG_C_COL_TYPE_ELLIPSE3D ) {
			JagEllipse3D e( sp, srid );
			e.bbox3D( xmin, ymin, zmin, xmax, ymax, zmax );
			value = bboxstr3D(srid, xmin, ymin, zmin, xmax, ymax, zmax );
		} else if ( colType == JAG_C_COL_TYPE_CUBE ) {
			JagCube s( sp, srid );
			s.bbox3D( xmin, ymin, zmin, xmax, ymax, zmax );
			value = bboxstr3D(srid, xmin, ymin, zmin, xmax, ymax, zmax );
		} else if ( colType == JAG_C_COL_TYPE_BOX ) {
			JagBox s( sp, srid );
			s.bbox3D( xmin, ymin, zmin, xmax, ymax, zmax );
			value = bboxstr3D(srid, xmin, ymin, zmin, xmax, ymax, zmax );
		} else if ( colType == JAG_C_COL_TYPE_SPHERE ) {
			JagSphere s( sp, srid );
			s.bbox3D( xmin, ymin, zmin, xmax, ymax, zmax );
			value = bboxstr3D(srid, xmin, ymin, zmin, xmax, ymax, zmax );
		} else if ( colType == JAG_C_COL_TYPE_ELLIPSOID ) {
			JagEllipsoid s( sp, srid );
			s.bbox3D( xmin, ymin, zmin, xmax, ymax, zmax );
			value = bboxstr3D(srid, xmin, ymin, zmin, xmax, ymax, zmax );
		} else if ( colType == JAG_C_COL_TYPE_CYLINDER ) {
			JagCylinder s( sp, srid );
			s.bbox3D( xmin, ymin, zmin, xmax, ymax, zmax );
			value = bboxstr3D(srid, xmin, ymin, zmin, xmax, ymax, zmax );
		} else if ( colType == JAG_C_COL_TYPE_CONE ) {
			JagCone s( sp, srid );
			s.bbox3D( xmin, ymin, zmin, xmax, ymax, zmax );
			value = bboxstr3D(srid, xmin, ymin, zmin, xmax, ymax, zmax );
		} else if ( colType == JAG_C_COL_TYPE_POLYGON ) {
			JagPolygon pgon;
			rc2 = JagParser::addPolygonData( pgon, sp, true );
			if ( rc2 <= 0 ) return false; 
			pgon.bbox2D( xmin, ymin, xmax, ymax );
			value = bboxstr2D(srid, xmin, ymin, xmax, ymax);
		} else if ( colType == JAG_C_COL_TYPE_POLYGON3D ) {
			JagPolygon pgon;
			rc2 = JagParser::addPolygon3DData( pgon, sp, true );
			if ( rc2 <= 0 ) return false;
			pgon.bbox3D( xmin, ymin, zmin, xmax, ymax, zmax );
			value = bboxstr3D(srid, xmin, ymin, zmin, xmax, ymax, zmax );
        } else if ( colType == JAG_C_COL_TYPE_LINESTRING || colType == JAG_C_COL_TYPE_MULTIPOINT ) {
            JagLineString linestr;
            JagParser::addLineStringData( linestr, sp);
			linestr.bbox2D( xmin, ymin, xmax, ymax );
			value = bboxstr2D(srid, xmin, ymin, xmax, ymax);
        } else if ( colType == JAG_C_COL_TYPE_LINESTRING3D || colType == JAG_C_COL_TYPE_MULTIPOINT3D ) {
            JagLineString3D linestr;
            JagParser::addLineString3DData( linestr, sp);
			linestr.bbox3D( xmin, ymin, zmin, xmax, ymax, zmax );
			value = bboxstr3D(srid, xmin, ymin, zmin, xmax, ymax, zmax );
		} else if ( colType == JAG_C_COL_TYPE_MULTILINESTRING ) {
			JagPolygon pgon;
			rc2 = JagParser::addPolygonData( pgon, sp, true );
			if ( rc2 <= 0 ) return false;
			pgon.bbox2D( xmin, ymin, xmax, ymax );
			value = bboxstr2D(srid, xmin, ymin, xmax, ymax);
		} else if ( colType == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
			JagPolygon pgon;
			rc2 = JagParser::addPolygon3DData( pgon, sp, true );
			if ( rc2 <= 0 ) return false;
			pgon.bbox3D( xmin, ymin, zmin, xmax, ymax, zmax );
			value = bboxstr3D(srid, xmin, ymin, zmin, xmax, ymax, zmax );
		} else if ( colType == JAG_C_COL_TYPE_MULTIPOLYGON ) {
			JagVector<JagPolygon> pgvec;
			rc2 = JagParser::addMultiPolygonData( pgvec, sp, true, false );
			if ( rc2 <= 0 ) return false;
			bool rb = JagGeo::getBBox2D( pgvec, xmin,ymin,xmax,ymax );
			if ( rb ) {
				value = bboxstr2D(srid, xmin, ymin, xmax, ymax);
			} else {
				return false;
			}
		} else if ( colType == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
			JagVector<JagPolygon> pgvec;
			rc2 = JagParser::addMultiPolygonData( pgvec, sp, true, true );
			if ( rc2 <= 0 ) return false;
			bool rb = JagGeo::getBBox3D( pgvec, xmin,ymin,zmin, xmax,ymax,zmax );
			if ( rb ) {
				value = bboxstr3D(srid, xmin, ymin, zmin, xmax, ymax, zmax );
			} else {
				return false;
			}
		} else {
			if ( 2 == dim ) {
				JagBox2D box;
				JagGeo::bbox2D( sp, box );
				value = bboxstr2D(srid, box.xmin, box.ymin, box.xmax, box.ymax);
			} else {
				JagBox3D box;
				JagGeo::bbox3D( sp, box );
				value = bboxstr3D(srid, box.xmin, box.ymin, box.zmin, box.xmax, box.ymax, box.zmax );
			}
		}
		rc = true;
	}

	return rc;
}

bool BinaryOpNode::doAllStartPoint( const Jstr& mk, const Jstr &colType, const JagStrSplit &sp, Jstr &value )
{
	value = "";
	if ( JagParser::isVectorGeoType( colType ) 
	     && colType != JAG_C_COL_TYPE_TRIANGLE 
	     && colType != JAG_C_COL_TYPE_TRIANGLE3D 
	   ) {
		 return false;
	 }

	if ( colType == JAG_C_COL_TYPE_LINE ) {
    	value = trimEndZeros(sp[JAG_SP_START+0]) + " " + trimEndZeros(sp[JAG_SP_START+1]);
	} else if ( colType == JAG_C_COL_TYPE_LINE3D  ) {
    	value = trimEndZeros(sp[JAG_SP_START+0]) + " " + trimEndZeros(sp[JAG_SP_START+1]) + " " + trimEndZeros(sp[JAG_SP_START+2]);
	} else if ( colType == JAG_C_COL_TYPE_TRIANGLE ) {
		value = trimEndZeros(sp[JAG_SP_START+0]) + " " + trimEndZeros(sp[JAG_SP_START+1]);
	} else if ( colType == JAG_C_COL_TYPE_TRIANGLE3D ) {
		value = trimEndZeros(sp[JAG_SP_START+0]) + " " + trimEndZeros(sp[JAG_SP_START+1]) + " " + trimEndZeros(sp[JAG_SP_START+2]);
	} else {
    	JagStrSplit s( sp[JAG_SP_START+0], ':' );
    	if ( s.length() < 4 ) {
    		value = sp[JAG_SP_START+0]; 
    	} else {
    		value = JagGeo::safeGetStr(sp, JAG_SP_START+1);
    	}
    	value.replace(':', ' ' );
	}

	return true;
}

bool BinaryOpNode::doAllCentroid( const Jstr& mk, const Jstr& hdr, const Jstr &colType, 
								    int srid, const JagStrSplit &sp1, Jstr &value )
{
	value = "";
	double cx, cy, cz;
	bool rc = true;
	bool is3D = false;

		JagLineString line;
		JagPolygon pgon;
        if ( colType == JAG_C_COL_TYPE_LINESTRING || colType == JAG_C_COL_TYPE_MULTIPOINT ) {
			JagParser::addLineStringData( line, sp1 );
			line.center2D( cx, cy, false );
        } else if ( colType == JAG_C_COL_TYPE_LINESTRING3D || colType == JAG_C_COL_TYPE_MULTIPOINT3D ) {
			JagLineString3D line3D;
			JagParser::addLineString3DData( line3D, sp1 );
			line3D.center3D( cx, cy, cz, false );
			is3D = true;
        } else if ( colType == JAG_C_COL_TYPE_POLYGON ) {
		    JagParser::addPolygonData( pgon, sp1, true );
			line.copyFrom( pgon.linestr[0], true );
			line.center2D( cx, cy, false );
        } else if ( colType == JAG_C_COL_TYPE_MULTILINESTRING ) {
		    JagParser::addPolygonData( pgon, sp1, true );
			line.copyFrom( pgon.linestr[0], false );
			line.center2D( cx, cy, false );
        } else if ( colType == JAG_C_COL_TYPE_POLYGON3D ) {
		    JagParser::addPolygon3DData( pgon, sp1, true );
			line.copyFrom( pgon.linestr[0], true );
			line.center3D( cx, cy, cz, false );
			is3D = true;
        } else if ( colType == JAG_C_COL_TYPE_LINESTRING3D ) {
		    JagParser::addPolygon3DData( pgon, sp1, true );
			line.copyFrom( pgon.linestr[0], false );
			line.center3D( cx, cy, cz, false );
			is3D = true;
        } else if ( colType == JAG_C_COL_TYPE_MULTIPOLYGON ) {
			JagVector<JagPolygon> pgvec;
			JagParser::addMultiPolygonData( pgvec, sp1, true, false );
			line.copyFrom( pgvec[0].linestr[0], true );
			line.center2D( cx, cy, false );
        } else if ( colType == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
			JagVector<JagPolygon> pgvec;
			JagParser::addMultiPolygonData( pgvec, sp1, true, true );
			line.copyFrom( pgvec[0].linestr[0], true );
			line.center3D( cx, cy, cz, false );
			is3D = true;
		} else if ( colType == JAG_C_COL_TYPE_POINT || colType == JAG_C_COL_TYPE_SQUARE
                    || colType == JAG_C_COL_TYPE_RECTANGLE 
					|| colType == JAG_C_COL_TYPE_CIRCLE || colType == JAG_C_COL_TYPE_ELLIPSE ) {
			cx = jagatof( sp1[JAG_SP_START+0].c_str() );
			cy = jagatof( sp1[JAG_SP_START+1].c_str() );
		} else if ( colType == JAG_C_COL_TYPE_LINE ) {
			double cx1 = jagatof( sp1[JAG_SP_START+0].c_str() );
			double cy1 = jagatof( sp1[JAG_SP_START+1].c_str() );
			double cx2 = jagatof( sp1[JAG_SP_START+2].c_str() );
			double cy2 = jagatof( sp1[JAG_SP_START+3].c_str() );
			cx = ( cx1 + cx2 ) /2.0;
			cy = ( cy1 + cy2 ) /2.0;
		} else if ( colType == JAG_C_COL_TYPE_TRIANGLE ) {
			double cx1 = jagatof( sp1[JAG_SP_START+0].c_str() );
			double cy1 = jagatof( sp1[JAG_SP_START+1].c_str() );
			double cx2 = jagatof( sp1[JAG_SP_START+2].c_str() );
			double cy2 = jagatof( sp1[JAG_SP_START+3].c_str() );
			double cx3 = jagatof( sp1[JAG_SP_START+4].c_str() );
			double cy3 = jagatof( sp1[JAG_SP_START+5].c_str() );
			cx = ( cx1 + cx2 + cx3 ) /3.0;
			cy = ( cy1 + cy2 + cy3 ) /3.0;
		} else if ( colType == JAG_C_COL_TYPE_POINT3D || colType == JAG_C_COL_TYPE_CUBE 
				    || colType == JAG_C_COL_TYPE_SPHERE || colType == JAG_C_COL_TYPE_ELLIPSOID
					|| colType == JAG_C_COL_TYPE_CONE ) {
			cx = jagatof( sp1[JAG_SP_START+0].c_str() );
			cy = jagatof( sp1[JAG_SP_START+1].c_str() );
			cz = jagatof( sp1[JAG_SP_START+2].c_str() );
			is3D = true;
		} else if ( colType == JAG_C_COL_TYPE_LINE3D ) {
			double cx1 = jagatof( sp1[JAG_SP_START+0].c_str() );
			double cy1 = jagatof( sp1[JAG_SP_START+1].c_str() );
			double cz1 = jagatof( sp1[JAG_SP_START+2].c_str() );
			double cx2 = jagatof( sp1[JAG_SP_START+3].c_str() );
			double cy2 = jagatof( sp1[JAG_SP_START+4].c_str() );
			double cz2 = jagatof( sp1[JAG_SP_START+5].c_str() );
			cx = ( cx1 + cx2 ) /2.0;
			cy = ( cy1 + cy2 ) /2.0;
			cz = ( cz1 + cz2 ) /2.0;
			is3D = true;
		} else if ( colType == JAG_C_COL_TYPE_TRIANGLE3D ) {
			double cx1 = jagatof( sp1[JAG_SP_START+0].c_str() );
			double cy1 = jagatof( sp1[JAG_SP_START+1].c_str() );
			double cz1 = jagatof( sp1[JAG_SP_START+2].c_str() );
			double cx2 = jagatof( sp1[JAG_SP_START+3].c_str() );
			double cy2 = jagatof( sp1[JAG_SP_START+4].c_str() );
			double cz2 = jagatof( sp1[JAG_SP_START+5].c_str() );
			double cx3 = jagatof( sp1[JAG_SP_START+6].c_str() );
			double cy3 = jagatof( sp1[JAG_SP_START+7].c_str() );
			double cz3 = jagatof( sp1[JAG_SP_START+8].c_str() );
			cx = ( cx1 + cx2 + cx3 ) /3.0;
			cy = ( cy1 + cy2 + cy3 ) /3.0;
			cz = ( cz1 + cz2 + cz3 ) /3.0;
			is3D = true;
		} else  {
			rc = false;
		}

	if ( rc ) {
		if ( is3D ) {
			value = Jstr("CJAG=0=0=PT3=d 0:0:0:0:0:0 ");
			value += trimEndZeros( doubleToStr(cx) ) + " " + trimEndZeros( doubleToStr(cy) ) + " " + trimEndZeros(doubleToStr(cz));
		} else {
			value = Jstr("CJAG=0=0=PT=d 0:0:0:0 ");
			value += trimEndZeros( doubleToStr(cx) ) + " " + trimEndZeros( doubleToStr(cy) );
		}
	}

	return rc;
}

bool BinaryOpNode::doAllMinMaxPoint( int op, const Jstr& mk, const Jstr& hdr, const Jstr &colType, 
								     int srid, const JagStrSplit &sp1, Jstr &value )
{
	value = "";
	bool rc = true;
	bool is3D = false;
	JagLineString line;
	JagPolygon pgon;

	double xmin=0.0, xmax=0.0, ymin=0.0, ymax=0.0, zmin=0.0, zmax=0.0;

    if ( colType == JAG_C_COL_TYPE_LINESTRING || colType == JAG_C_COL_TYPE_MULTIPOINT ) {
		JagParser::addLineStringData( line, sp1 );
		line.minmax2D( op, xmin, ymin, xmax, ymax );
    } else if ( colType == JAG_C_COL_TYPE_VECTOR ) {
        JagVectorString vs;
		JagParser::addVectorData( vs, sp1 );
		vs.minmax( op, xmin, xmax );
    } else if ( colType == JAG_C_COL_TYPE_LINESTRING3D || colType == JAG_C_COL_TYPE_MULTIPOINT3D ) {
		JagLineString3D line3D;
		JagParser::addLineString3DData( line3D, sp1 );
		line3D.minmax3D( op, xmin, ymin, zmin, xmax, ymax, zmax );
		is3D = true;
    } else if ( colType == JAG_C_COL_TYPE_POLYGON ) {
	    JagParser::addPolygonData( pgon, sp1, true );
		line.copyFrom( pgon.linestr[0], true );
		line.minmax2D( op, xmin, ymin, xmax, ymax );
    } else if ( colType == JAG_C_COL_TYPE_MULTILINESTRING ) {
	    JagParser::addPolygonData( pgon, sp1, true );
		line.copyFrom( pgon.linestr[0], false );
		line.minmax2D( op, xmin, ymin, xmax, ymax );
    } else if ( colType == JAG_C_COL_TYPE_POLYGON3D ) {
	    JagParser::addPolygon3DData( pgon, sp1, true );
		line.copyFrom( pgon.linestr[0], true );
		line.minmax3D( op, xmin, ymin, zmin, xmax, ymax, zmax );
		is3D = true;
    } else if ( colType == JAG_C_COL_TYPE_LINESTRING3D ) {
	    JagParser::addPolygon3DData( pgon, sp1, true );
		line.copyFrom( pgon.linestr[0], false );
		line.minmax3D( op, xmin, ymin, zmin, xmax, ymax, zmax );
		is3D = true;
    } else if ( colType == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		JagVector<JagPolygon> pgvec;
		JagParser::addMultiPolygonData( pgvec, sp1, true, false );
		line.copyFrom( pgvec[0].linestr[0], true );
		line.minmax2D( op, xmin, ymin, xmax, ymax );
    } else if ( colType == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
		JagVector<JagPolygon> pgvec;
		JagParser::addMultiPolygonData( pgvec, sp1, true, true );
		line.copyFrom( pgvec[0].linestr[0], true );
		line.minmax3D( op, xmin, ymin, zmin, xmax, ymax, zmax );
		is3D = true;
	} else if ( colType == JAG_C_COL_TYPE_POINT ) {
		xmin = xmax = jagatof( sp1[JAG_SP_START+0] );
		ymin = ymax = jagatof( sp1[JAG_SP_START+1] );
	} else if (  colType == JAG_C_COL_TYPE_SQUARE) {
		JagSquare2D sq( sp1, srid );
		if ( JAG_FUNC_XMINPOINT == op ) { 
			xmin = sq.getXMin( ymin );
		} else if ( JAG_FUNC_YMINPOINT == op ) {
			ymin = sq.getYMin( xmin );
		} else if ( JAG_FUNC_XMAXPOINT == op ) {
			xmax = sq.getXMax( ymax );
		} else if ( JAG_FUNC_YMAXPOINT == op ) {
			ymax = sq.getYMax( xmax );
		}
	} else if (  colType == JAG_C_COL_TYPE_RECTANGLE  ) {
		JagRectangle2D rect2d( sp1, srid );
		if ( JAG_FUNC_XMINPOINT == op ) { 
			xmin = rect2d.getXMin( ymin );
		} else if ( JAG_FUNC_YMINPOINT == op ) {
			ymin = rect2d.getYMin( xmin );
		} else if ( JAG_FUNC_XMAXPOINT == op ) {
			xmax = rect2d.getXMax( ymax );
		} else if ( JAG_FUNC_YMAXPOINT == op ) {
			ymax = rect2d.getYMax( xmax );
		}

	} else if ( colType == JAG_C_COL_TYPE_CIRCLE ) { 
		double cx = jagatof( sp1[JAG_SP_START+0] );
		double cy = jagatof( sp1[JAG_SP_START+1] );
		double r = jagatof( sp1[JAG_SP_START+2] );
		if ( JAG_FUNC_XMINPOINT == op ) { 
			xmin  = cx - r;
			ymin  = cy;
		} else if ( JAG_FUNC_YMINPOINT == op ) {
			xmin  = cx;
			ymin  = cy -r;
		} else if ( JAG_FUNC_XMAXPOINT == op ) {
			xmax = cx + r;
			ymax = cy;
		} else if ( JAG_FUNC_YMAXPOINT == op ) {
			xmax = cx;
			ymax = cy + r;
		} 
	} else if (  colType == JAG_C_COL_TYPE_ELLIPSE ) {
		JagEllipse2D e( sp1, srid );
		e.minmax2D( op, xmin, ymin, xmax, ymax );
	} else if ( colType == JAG_C_COL_TYPE_LINE ) {
		double px1 = jagatof( sp1[JAG_SP_START+0] );
		double py1 = jagatof( sp1[JAG_SP_START+1] );
		double px2 = jagatof( sp1[JAG_SP_START+2] );
		double py2 = jagatof( sp1[JAG_SP_START+3] );
		if ( JAG_FUNC_XMINPOINT == op ) { 
			if ( px1 < px2 ) {
				xmin  = px1; ymin = py1;
			} else {
				xmin  = px2; ymin = py2;
			}
		} else if ( JAG_FUNC_YMINPOINT == op ) {
			if ( py1 < py2 ) {
				xmin  = px1; ymin = py1;
			} else {
				xmin  = px2; ymin = py2;
			}
		} else if ( JAG_FUNC_XMAXPOINT == op ) {
			if ( px1 > px2 ) {
				xmax  = px1; ymax = py1;
			} else {
				xmax  = px2; ymax = py2;
			}
		} else if ( JAG_FUNC_YMAXPOINT == op ) {
			if ( py1 > py2 ) {
				xmax  = px1; ymax = py1;
			} else {
				xmax  = px2; ymax = py2;
			}
		} 
	} else if ( colType == JAG_C_COL_TYPE_TRIANGLE ) {
		JagTriangle2D t( sp1, srid );
		JagPolygon pgon( t );
		pgon.minmax2D( op, xmin, ymin, xmax, ymax );
	} else if ( colType == JAG_C_COL_TYPE_POINT3D ) {
		xmin = xmax = jagatof( sp1[JAG_SP_START+0] );
		ymin = ymax = jagatof( sp1[JAG_SP_START+1] );
		zmin = zmax = jagatof( sp1[JAG_SP_START+2] );
		is3D = true;
	} else if ( colType == JAG_C_COL_TYPE_SPHERE ) {
		double cx = jagatof( sp1[JAG_SP_START+0] );
		double cy = jagatof( sp1[JAG_SP_START+1] );
		double cz = jagatof( sp1[JAG_SP_START+2] );
		double r = jagatof( sp1[JAG_SP_START+3] );
		if ( JAG_FUNC_XMINPOINT == op ) {
			xmin = cx - r;
			ymin = cy;
			zmin = cz;
		} else if ( JAG_FUNC_XMAXPOINT == op ) {
			xmax = cx + r;
			ymax = cy;
			zmax = cz;
		} else if ( JAG_FUNC_YMINPOINT == op ) {
			xmin = cx;
			ymin = cy - r;
			zmin = cz;
		} else if ( JAG_FUNC_YMAXPOINT == op ) {
			xmax = cx;
			ymax = cy + r;
			zmax = cz;
		} else if ( JAG_FUNC_ZMINPOINT == op ) {
			xmin = cx;
			ymin = cy;
			zmin = cz - r;
		} else if ( JAG_FUNC_ZMAXPOINT == op ) {
			xmax = cx;
			ymax = cy;
			zmax = cz + r;
		}
		is3D = true;
	} else if (  colType == JAG_C_COL_TYPE_CUBE ) {
		JagCube s( sp1, srid );
		s.minmax3D( op, xmin, ymin, zmin, xmax, ymax, zmax );
		is3D = true;
	} else if (  colType == JAG_C_COL_TYPE_ELLIPSOID
				|| colType == JAG_C_COL_TYPE_CONE ) {
		/**
		cx = jagatof( sp1[JAG_SP_START+0].c_str() );
		cy = jagatof( sp1[JAG_SP_START+1].c_str() );
		cz = jagatof( sp1[JAG_SP_START+2].c_str() );
		**/
		is3D = true;
		//todo JAG_C_COL_TYPE_ELLIPSOID JAG_C_COL_TYPE_CONE
	} else if ( colType == JAG_C_COL_TYPE_LINE3D ) {
		double cx1 = jagatof( sp1[JAG_SP_START+0] );
		double cy1 = jagatof( sp1[JAG_SP_START+1] );
		double cz1 = jagatof( sp1[JAG_SP_START+2] );
		double cx2 = jagatof( sp1[JAG_SP_START+3] );
		double cy2 = jagatof( sp1[JAG_SP_START+4] );
		double cz2 = jagatof( sp1[JAG_SP_START+5] );
		line.add( cx1, cy1, cz1 ); 
		line.add( cx2, cy2, cz2 ); 
		line.minmax3D( op, xmin, ymin, zmin, xmax, ymax, zmax );
		is3D = true;
	} else if ( colType == JAG_C_COL_TYPE_TRIANGLE3D ) {
		double cx1 = jagatof( sp1[JAG_SP_START+0] );
		double cy1 = jagatof( sp1[JAG_SP_START+1] );
		double cz1 = jagatof( sp1[JAG_SP_START+2] );
		double cx2 = jagatof( sp1[JAG_SP_START+3] );
		double cy2 = jagatof( sp1[JAG_SP_START+4] );
		double cz2 = jagatof( sp1[JAG_SP_START+5] );
		double cx3 = jagatof( sp1[JAG_SP_START+6] );
		double cy3 = jagatof( sp1[JAG_SP_START+7] );
		double cz3 = jagatof( sp1[JAG_SP_START+8] );
		line.add( cx1, cy1, cz1 ); 
		line.add( cx2, cy2, cz2 ); 
		line.add( cx3, cy3, cz3 ); 
		line.minmax3D( op, xmin, ymin, zmin, xmax, ymax, zmax );
		is3D = true;
	} else  {
		rc = false;
	}

	if ( rc ) {
		if ( is3D ) {
			value = Jstr("CJAG=0=0=PT3=d 0:0:0:0:0:0 ");
			double  vx, vy, vz;
    		if ( JAG_FUNC_XMINPOINT == op ) { 
				vx = xmin; vy = ymin; vz = zmin;
    		} else if ( JAG_FUNC_YMINPOINT == op ) {
				vx = xmin; vy = ymin; vz = zmin;
    		} else if ( JAG_FUNC_XMAXPOINT == op ) {
				vx = xmax; vy = ymax; vz = zmax;
    		} else if ( JAG_FUNC_YMAXPOINT == op ) {
				vx = xmax; vy = ymax; vz = zmax;
    		} else if ( JAG_FUNC_ZMINPOINT == op ) {
				vx = xmin; vy = ymin; vz = zmin;
    		} else if ( JAG_FUNC_ZMAXPOINT == op ) {
				vx = xmax; vy = ymax; vz = zmax;
    		}
			value += trimEndZeros( doubleToStr(vx) ) + " " + trimEndZeros( doubleToStr(vy) ) + " " + trimEndZeros(doubleToStr(vz));
		} else {
			value = Jstr("CJAG=0=0=PT=d 0:0:0:0 ");
			double  vx, vy;
    		if ( JAG_FUNC_XMINPOINT == op ) { 
				vx = xmin; vy = ymin;
    		} else if ( JAG_FUNC_YMINPOINT == op ) {
				vx = xmin; vy = ymin;
    		} else if ( JAG_FUNC_XMAXPOINT == op ) {
				vx = xmax; vy = ymax;
    		} else if ( JAG_FUNC_YMAXPOINT == op ) {
				vx = xmax; vy = ymax;
    		}
			value += trimEndZeros( doubleToStr(vx) ) + " " + trimEndZeros( doubleToStr(vy) );
		}
	}

	return rc;
}


bool BinaryOpNode::doAllConvexHull( const Jstr& mk, const Jstr& hdr, const Jstr &colType, 
								    int srid, const JagStrSplit &sp, Jstr &value )
{
	if ( sp.size() < 2 ) return false;
	value = "";
	Jstr bbox;
	bbox = sp[1];

		JagLineString line;
		JagPolygon pgon;
        if ( colType == JAG_C_COL_TYPE_LINESTRING || colType == JAG_C_COL_TYPE_MULTIPOINT ) {
			JagParser::addLineStringData( line, sp );
			JagCGAL::getConvexHull2DStr( line, hdr, bbox, value );
        } else if ( colType == JAG_C_COL_TYPE_LINESTRING3D || colType == JAG_C_COL_TYPE_MULTIPOINT3D ) {
			JagLineString3D line3D;
			JagParser::addLineString3DData( line3D, sp );
			line = line3D;
			JagCGAL::getConvexHull3DStr( line, hdr, bbox, value );
        } else if ( colType == JAG_C_COL_TYPE_POLYGON ) {
		    JagParser::addPolygonData( pgon, sp, true );
			line.copyFrom( pgon.linestr[0], true );
			JagCGAL::getConvexHull2DStr( line, hdr, bbox, value );
        } else if ( colType == JAG_C_COL_TYPE_MULTILINESTRING ) {
		    JagParser::addPolygonData( pgon, sp, false );
			for ( int i=1; i < pgon.size(); ++i ) {
				line.appendFrom( pgon.linestr[i], false );
			}
			JagCGAL::getConvexHull2DStr( line, hdr, bbox, value );
        } else if ( colType == JAG_C_COL_TYPE_POLYGON3D ) {
		    JagParser::addPolygon3DData( pgon, sp, true );
			line.copyFrom( pgon.linestr[0], true );
			JagCGAL::getConvexHull3DStr( line, hdr, bbox, value );
        } else if ( colType == JAG_C_COL_TYPE_LINESTRING3D ) {
		    JagParser::addPolygon3DData( pgon, sp, true );
			line.copyFrom( pgon.linestr[0], false );
			JagCGAL::getConvexHull3DStr( line, hdr, bbox, value );
        } else if ( colType == JAG_C_COL_TYPE_MULTIPOLYGON ) {
			JagVector<JagPolygon> pgvec;
			JagParser::addMultiPolygonData( pgvec, sp, false, false );
			for ( int i=0; i < pgvec.size(); ++i ) {
				line.appendFrom( pgvec[i].linestr[0], true );
			}
			JagCGAL::getConvexHull2DStr( line, hdr, bbox, value );
        } else if ( colType == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
			JagVector<JagPolygon> pgvec;
			JagParser::addMultiPolygonData( pgvec, sp, false, true );
			for ( int i=0; i < pgvec.size(); ++i ) {
				line.appendFrom( pgvec[i].linestr[0], true );
			}
			JagCGAL::getConvexHull3DStr( line, hdr, bbox, value );
		} else  {
		}

	return true;
}

bool BinaryOpNode::doAllConcaveHull( const Jstr& mk, const Jstr& hdr, const Jstr &colType, 
								    int srid, const JagStrSplit &sp, Jstr &value )
{
	if ( sp.size() < 3 ) return false;
	value = "";
	Jstr bbox;
	bbox = sp[1]; 

	JagLineString3D line;
    if ( colType == JAG_C_COL_TYPE_LINESTRING || colType == JAG_C_COL_TYPE_MULTIPOINT ) {
		JagParser::addLineStringData( line, sp );
		JagCGAL::getConcaveHull2DStr( line, hdr, bbox, value );
    } else if ( colType == JAG_C_COL_TYPE_POLYGON ) {
		JagPolygon pgon;
	    JagParser::addPolygonData( pgon, sp, true );
		JagCGAL::getConcaveHull2DStr( pgon.linestr[0], hdr, bbox, value );
    } else if ( colType == JAG_C_COL_TYPE_MULTILINESTRING ) {
		JagPolygon pgon;
	    JagParser::addPolygonData( pgon, sp, false );
		for ( int i=1; i < pgon.size(); ++i ) {
			line.appendFrom( pgon.linestr[i], false );
		}
		JagCGAL::getConcaveHull2DStr( line, hdr, bbox, value );
    } else if ( colType == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		JagVector<JagPolygon> pgvec;
		JagParser::addMultiPolygonData( pgvec, sp, false, false );
		for ( int i=0; i < pgvec.size(); ++i ) {
			line.appendFrom( pgvec[i].linestr[0], true );
		}
		JagCGAL::getConcaveHull2DStr( line, hdr, bbox, value );
	} 

	return true;
}

bool BinaryOpNode::doAllAsText( const Jstr& mk, const Jstr& hdr, const Jstr &colType, 
								    int srid, const JagStrSplit &sp, const Jstr& carg, Jstr &value )
{
	value = sp.c_str();
	return true;
}

bool BinaryOpNode::doAllInterpolate( int srid, const Jstr &colType, const JagStrSplit &sp, const Jstr& carg, Jstr &value )
{
	value = "";

	double frac = carg.tof();
	JagLineString3D line;
	JagPolygon pgon;
	bool rc = false;
	JagPoint3D point;

	if ( colType == JAG_C_COL_TYPE_LINE ) {
		value = Jstr("CJAG=0=0=PT=d 0:0:0:0 ");
		double px, py;
		px = jagatof( sp[JAG_SP_START+0] );
		py = jagatof( sp[JAG_SP_START+1] );
		line.add(px, py);
		px = jagatof( sp[JAG_SP_START+2] );
		py = jagatof( sp[JAG_SP_START+3] );
		line.add(px, py);
		rc = line.interpolatePoint( JAG_3D, srid, frac, point );
		if ( ! rc ) return false;
		value += d2s( point.x ) + " " + d2s( point.y ); 
	} else if ( colType == JAG_C_COL_TYPE_LINE3D ) {
		value = Jstr("CJAG=0=0=PT3=d 0:0:0:0:0:0 ");
		double px, py, pz;
		px = jagatof( sp[JAG_SP_START+0] );
		py = jagatof( sp[JAG_SP_START+1] );
		pz = jagatof( sp[JAG_SP_START+2] );
		line.add(px, py);
		px = jagatof( sp[JAG_SP_START+3] );
		py = jagatof( sp[JAG_SP_START+4] );
		pz = jagatof( sp[JAG_SP_START+5] );
		line.add(px, py);
		rc = line.interpolatePoint( JAG_2D, srid, frac, point );
		if ( ! rc ) return false;
		value += d2s( point.x ) + " " + d2s( point.y )  + " " + d2s(point.z);
    } else if ( colType == JAG_C_COL_TYPE_LINESTRING ) {
		value = Jstr("CJAG=0=0=PT=d 0:0:0:0 ");
	    JagParser::addPolygonData( pgon, sp, true );
		rc = pgon.linestr[0].interpolatePoint( JAG_2D, srid, frac, point );
		if ( ! rc ) return false;
		value += d2s( point.x ) + " " + d2s( point.y ); 
    } else if ( colType == JAG_C_COL_TYPE_LINESTRING3D ) {
		value = Jstr("CJAG=0=0=PT3=d 0:0:0:0:0:0 ");
	    JagParser::addPolygon3DData( pgon, sp, true );
		rc = pgon.linestr[0].interpolatePoint( JAG_3D, srid, frac, point );
		if ( ! rc ) return false;
		value += d2s( point.x ) + " " + d2s( point.y )  + " " + d2s(point.z);
	} else {
		return false;
	}

	return true;
}

bool BinaryOpNode::doAllLineSubstring( int srid, const Jstr &colType, const JagStrSplit &sp, const Jstr& carg, Jstr &value )
{
	value = "";

	JagStrSplit fs(carg, ':' );
	if ( fs.size() < 2 ) return false;

	double startfrac = fs[0].tof();
	double endfrac = fs[1].tof();
	if ( startfrac > endfrac ) return false;
	JagLineString3D line;
	JagPolygon pgon;
	bool rc = false;
	Jstr retLstr;

	if ( colType == JAG_C_COL_TYPE_LINE ) {
		JagPoint3D point;
		value = Jstr("CJAG=0=0=LS=d 0:0:0:0 ");
		double px, py;
		px = jagatof( sp[JAG_SP_START+0] );
		py = jagatof( sp[JAG_SP_START+1] );
		line.add(px, py);
		px = jagatof( sp[JAG_SP_START+2] );
		py = jagatof( sp[JAG_SP_START+3] );
		line.add(px, py);
		rc = line.interpolatePoint( JAG_2D, srid, startfrac, point );
		if ( ! rc ) return false;
		value += d2s(point.x) + ":" + d2s(point.y) +  " ";
		rc = line.interpolatePoint( JAG_2D, srid, endfrac, point );
		if ( ! rc ) return false;
		value += d2s(point.x) + ":" + d2s(point.y);
	} else if ( colType == JAG_C_COL_TYPE_LINE3D ) {
		JagPoint3D point;
		value = Jstr("CJAG=0=0=LS3=d 0:0:0:0:0:0 ");
		double px, py, pz;
		px = jagatof( sp[JAG_SP_START+0] );
		py = jagatof( sp[JAG_SP_START+1] );
		pz = jagatof( sp[JAG_SP_START+2] );
		line.add(px, py);
		px = jagatof( sp[JAG_SP_START+3] );
		py = jagatof( sp[JAG_SP_START+4] );
		pz = jagatof( sp[JAG_SP_START+5] );
		line.add(px, py);

		rc = line.interpolatePoint( JAG_3D, srid, startfrac, point );
		if ( ! rc ) return false;
		value += d2s(point.x) + ":" + d2s(point.y) + ":" + d2s(point.z) +  " ";
		rc = line.interpolatePoint( JAG_2D, srid, endfrac, point );
		if ( ! rc ) return false;
		value += d2s(point.x) + ":" + d2s(point.y) + ":" + d2s(point.z);
    } else if ( colType == JAG_C_COL_TYPE_LINESTRING ) {
		value = Jstr("CJAG=0=0=LS=d 0:0:0:0 ");
	    JagParser::addPolygonData( pgon, sp, true );
		rc = pgon.linestr[0].substring( JAG_2D, srid, startfrac, endfrac, retLstr );
		if ( ! rc ) return false;
		value += retLstr;
    } else if ( colType == JAG_C_COL_TYPE_LINESTRING3D ) {
		value = Jstr("CJAG=0=0=LS3=d 0:0:0:0:0:0 ");
	    JagParser::addPolygon3DData( pgon, sp, true );
		rc = pgon.linestr[0].substring( JAG_3D, srid, startfrac, endfrac, retLstr );
		if ( ! rc ) return false;
		value += retLstr;
	} else {
		return false;
	}

	return true;
}

// convert shapes to polygon
bool BinaryOpNode::doAllToPolygon( const Jstr& mk, const Jstr& hdr, const Jstr &colType, 
								    int srid, const JagStrSplit &sp, const Jstr& carg, Jstr &value )
{
	if ( colType == JAG_C_COL_TYPE_POLYGON ||  colType == JAG_C_COL_TYPE_POLYGON3D ) {
		value = sp.c_str();
		return true;
	}

	if ( ! JagParser::isVectorGeoType( colType ) ) {
		value = "";
		return false;
	}

	value = "";
    if ( colType == JAG_C_COL_TYPE_SQUARE ) {
		JagSquare2D sq( sp, srid );
		JagPolygon pgon( sq );
		pgon.toJAG( false, true, "", srid, value );
    } else if ( colType == JAG_C_COL_TYPE_RECTANGLE ) {
		JagRectangle2D rect( sp, srid );
		JagPolygon pgon( rect );
		pgon.toJAG( false, true, "", srid, value );
    } else if ( colType == JAG_C_COL_TYPE_CIRCLE ) {
		JagCircle2D cir( sp, srid );
		JagPolygon pgon( cir, jagatoi( carg.c_str()) );
		pgon.toJAG( false, true, "", srid, value );
    } else if ( colType == JAG_C_COL_TYPE_ELLIPSE ) {
		JagEllipse2D e( sp, srid );
		JagPolygon pgon( e, jagatoi( carg.c_str()) );
		pgon.toJAG( false, true, "", srid, value );
    } else if ( colType == JAG_C_COL_TYPE_TRIANGLE ) {
		JagTriangle2D t( sp, srid );
		JagPolygon pgon( t );
		pgon.toJAG( false, true, "", srid, value );
	}

	return true;
}

bool BinaryOpNode::doAllToMultipoint( const Jstr& mk, const Jstr& hdr, const Jstr &colType, 
								    int srid, const JagStrSplit &sp, const Jstr& carg, Jstr &value )
{
	int dim = getDimension( colType );
	if ( 3 == dim ) {
		value = Jstr("CJAG=0=0=MP3=d 0:0:0:0:0:0");
	} else {
		value = Jstr("CJAG=0=0=MP=d 0:0:0:0");
	}

	if ( ! JagParser::isVectorGeoType( colType ) ) {
		for ( int i=JAG_SP_START; i < sp.size(); ++i ) {
			if ( sp[i] == "|" || sp[i] == "!" ) { continue; }
			value += Jstr(" ") + sp[i];
		}
		return true;
	}

	Jstr data;
    if ( colType == JAG_C_COL_TYPE_SQUARE ) {
		JagSquare2D sq( sp, srid );
		JagPolygon pgon( sq, false );
		pgon.toJAG( false, false, "", srid, data );
		value += data;
    } else if ( colType == JAG_C_COL_TYPE_SQUARE3D ) {
		JagSquare3D sq( sp, srid );
		JagPolygon pgon( sq, false );
		pgon.toJAG( true, false, "", srid, data );
		value += data;
    } else if ( colType == JAG_C_COL_TYPE_RECTANGLE ) {
		JagRectangle2D rect( sp, srid );
		JagPolygon pgon( rect, false );
		pgon.toJAG( false, false, "", srid, data );
		value += data;
    } else if ( colType == JAG_C_COL_TYPE_RECTANGLE3D ) {
		JagRectangle3D rect( sp, srid );
		JagPolygon pgon( rect, false );
		pgon.toJAG( true, false, "", srid, data );
		value += data;
    } else if ( colType == JAG_C_COL_TYPE_CIRCLE ) {
		JagCircle2D cir( sp, srid );
		JagPolygon pgon( cir, jagatoi( carg.c_str()), false );
		pgon.toJAG( false, false, "", srid, data );
		value += data;
    } else if ( colType == JAG_C_COL_TYPE_CIRCLE3D ) {
		JagCircle3D cir( sp, srid );
		JagPolygon pgon( cir, jagatoi( carg.c_str()), false );
		pgon.toJAG( true, false, "", srid, data );
		value += data;
    } else if ( colType == JAG_C_COL_TYPE_ELLIPSE ) {
		JagEllipse2D e( sp, srid );
		JagPolygon pgon( e, jagatoi( carg.c_str()), false );
		pgon.toJAG( false, false, "", srid, data );
		value += data;
    } else if ( colType == JAG_C_COL_TYPE_ELLIPSE3D ) {
		JagEllipse3D e( sp, srid );
		JagPolygon pgon( e, jagatoi( carg.c_str()), false );
		pgon.toJAG( true, false, "", srid, data );
		value += data;
    } else if ( colType == JAG_C_COL_TYPE_TRIANGLE ) {
		JagTriangle2D t( sp, srid );
		JagPolygon pgon( t, false );
		pgon.toJAG( false, false, "", srid, data );
		value += data;
    } else if ( colType == JAG_C_COL_TYPE_TRIANGLE3D ) {
		JagTriangle3D t( sp, srid );
		JagPolygon pgon( t, false );
		pgon.toJAG( true, false, "", srid, data );
		value += data;
	}

	return true;
}

bool BinaryOpNode::doAllOuterRing( const Jstr& mk, const Jstr& hdr, const Jstr &colType, 
								    int srid, const JagStrSplit &sp, Jstr &value )
{
	value = "";
	Jstr bbox;
	if ( mk == JAG_OJAG ) { bbox = sp[1]; } 

	JagLineString line;
	JagPolygon pgon;
    if ( colType == JAG_C_COL_TYPE_POLYGON ) {
	    JagParser::addPolygonData( pgon, sp, true );
		line.copyFrom( pgon.linestr[0], false );
		JagCGAL::getRingStr( line, hdr, bbox, false, value );
    } else if ( colType == JAG_C_COL_TYPE_POLYGON3D ) {
	    JagParser::addPolygon3DData( pgon, sp, true );
		line.copyFrom( pgon.linestr[0], false );
		JagCGAL::getRingStr( line, hdr, bbox, true, value );
    } else {
		return false;
	}

	return true;
}

bool BinaryOpNode::doAllOuterRings( const Jstr& mk, const Jstr& hdr, const Jstr &colType, 
								    int srid, const JagStrSplit &sp, Jstr &value )
{
	value = "";
	Jstr bbox;
	if ( mk == JAG_OJAG ) { bbox = sp[1]; } 
	JagVector<JagPolygon> pgvec;
    if ( colType == JAG_C_COL_TYPE_MULTIPOLYGON ) {
	    JagParser::addMultiPolygonData( pgvec, sp, true, false );
		JagCGAL::getOuterRingsStr( pgvec, hdr, bbox, false, value );
    } else if ( colType == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
	    JagParser::addMultiPolygonData( pgvec, sp, true, true );
		JagCGAL::getOuterRingsStr( pgvec, hdr, bbox, true, value );
    } else if ( colType == JAG_C_COL_TYPE_POLYGON ) {
		JagPolygon pgon;
	    JagParser::addPolygonData( pgon, sp, true );
		pgvec.append( pgon );
		JagCGAL::getOuterRingsStr( pgvec, hdr, bbox, false, value );
    } else if ( colType == JAG_C_COL_TYPE_POLYGON3D ) {
		JagPolygon pgon;
	    JagParser::addPolygon3DData( pgon, sp, true );
		pgvec.append( pgon );
		JagCGAL::getOuterRingsStr( pgvec, hdr, bbox, true, value );
    } else {
		return false;
	}

	return true;
}

bool BinaryOpNode::doAllInnerRings( const Jstr& mk, const Jstr& hdr, const Jstr &colType, 
								    int srid, const JagStrSplit &sp, Jstr &value )
{
	value = "";
	Jstr bbox;
	if ( mk == JAG_OJAG ) { bbox = sp[1]; } 
	JagVector<JagPolygon> pgvec;
	int rc;
    if ( colType == JAG_C_COL_TYPE_POLYGON ) {
		JagPolygon pgon;
	    rc = JagParser::addPolygonData( pgon, sp, false );
		pgvec.append( pgon );
		JagCGAL::getInnerRingsStr( pgvec, hdr, bbox, false, value );
    } else if ( colType == JAG_C_COL_TYPE_POLYGON3D ) {
		JagPolygon pgon;
	    rc = JagParser::addPolygon3DData( pgon, sp, false );
		pgvec.append( pgon );
		JagCGAL::getInnerRingsStr( pgvec, hdr, bbox, true, value );
    } else if ( colType == JAG_C_COL_TYPE_MULTIPOLYGON ) {
	    JagParser::addMultiPolygonData( pgvec, sp, false, false );
		JagCGAL::getInnerRingsStr( pgvec, hdr, bbox, false, value );
    } else if ( colType == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
	    JagParser::addMultiPolygonData( pgvec, sp, false, true );
		JagCGAL::getInnerRingsStr( pgvec, hdr, bbox, true, value );
    } else {
		return false;
	}

	return true;
}

bool BinaryOpNode::doAllRingN( const Jstr& mk, const Jstr& hdr, const Jstr &colType, 
								    int srid, const JagStrSplit &sp, const Jstr &carg,  Jstr &value )
{
    dn("s22207110 doAllRingN mk=%s hdr=%s colType=%s carg=%s", mk.s(), hdr.s(), colType.s(), carg.s() );
    // sp.print();

	int N = jagatoi( carg.c_str() );
	if ( N <= 0 ) {
        dn("sdoAllRingN N <= 0 return false");
		return false;
	}

	value = "";
	Jstr bbox;

	JagLineString line;
	JagPolygon pgon;

    if ( colType == JAG_C_COL_TYPE_POLYGON ) {
        dn("s30008 JAG_C_COL_TYPE_POLYGON");
	    JagParser::addPolygonData( pgon, sp, true );
		if ( pgon.linestr.size() < N ) {
            dn("s202209  false");
            return false;
        }
		line.copyFrom( pgon.linestr[N-1], false );
		JagCGAL::getRingStr( line, hdr, bbox, false, value );
    } else if ( colType == JAG_C_COL_TYPE_POLYGON3D ) {
        dn("s30008 JAG_C_COL_TYPE_POLYGON3D");
	    JagParser::addPolygon3DData( pgon, sp, true );
		if ( pgon.linestr.size() < N ) {
            dn("s282209  false");
            return false;
        }
		line.copyFrom( pgon.linestr[N-1], false );
		JagCGAL::getRingStr( line, hdr, bbox, true, value );
    } else {
        dn("s033819 false");
		return false;
	}

	return true;
}

bool BinaryOpNode::doAllMetricN( const Jstr& mk, const Jstr& hdr, const Jstr &colType, 
								    int srid, const JagStrSplit &sp, const Jstr &carg,  Jstr &value )
{
	JagStrSplit ssa( carg, ':');
	if ( ssa.size() < 2 ) return false;
	int N = ssa[0].toInt();
	int m = ssa[1].toInt();
	bool found = false;
	int cnt = 1;
	Jstr mstr;
	value = "";

	int coords = JagParser::vectorShapeCoordinates( colType );
	if ( coords > 0 ) {
		if ( N < 1 ) {
    		for ( int i=JAG_SP_START + coords; i < sp.size(); ++i ) {
    			if ( sp[i] == "|" || sp[i] == "!" ) continue;
				if ( value.size() < 1 ) { value = sp[i]; } else { value += Jstr("#") + sp[i]; }
			}
			return true;
		} else {
			if ( N > sp.size() - JAG_SP_START - coords ) return false;
    		cnt = 1;
    		for ( int i=JAG_SP_START + coords; i < sp.size(); ++i ) {
    			if ( sp[i] == "|" || sp[i] == "!" ) continue;
    			if ( N == cnt ) {
					value = sp[i];
					return true;
    			}
    				++cnt;
   			}
			return false;
		}
	}

	cnt = 1;
	found = false;
	for ( int i=JAG_SP_START; i < sp.size(); ++i ) {
		if ( sp[i] == "|" || sp[i] == "!" ) continue;
		if ( N == cnt ) {
			found = true;
			JagStrSplit ss(sp[i], ':');
			mstr = ss[ ss.size() - 1 ];
			break;
		}
		++cnt;
	}

	if ( ! found ) return false;
	if ( m < 1 ) {
		value = mstr;
		return true;
	}

	JagStrSplit ss( mstr, '#' );
	if ( m > ss.size() ) {
		return false;
	}

	value = ss[m-1];
	return true;
}

bool BinaryOpNode::doAllGeoJson( const Jstr& mk, const Jstr &colType, 
								    int srid, JagStrSplit &sp, const Jstr &carg,  Jstr &value )
{
	int dim = getDimension(colType);
	bool rc;
	Jstr pgon;
	if ( 2 == dim ) {
		if ( JagParser::isVectorGeoType( colType ) ) {
			JagStrSplit ssa( carg, ':');
			rc = doAllToPolygon( mk, "", colType, srid, sp, ssa[1], pgon );
			if ( ! rc ) return false;
			sp.init( pgon.c_str(), -1, ' ', true );
		}
	}

	JagStrSplit csp(sp[0], '=');
	const char *data = thirdTokenStart( sp.c_str() );
	if ( ! data || strlen(data) < 1 ) {
		return false;
	}
	value = makeGeoJson(csp, data );
	return true;
}

bool BinaryOpNode::doAllWKT( const Jstr& mk, const Jstr &colType, 
								    int srid, JagStrSplit &sp, const Jstr &carg,  Jstr &value )
{
	JagStrSplit ssa( carg, ':');
	int samples = ssa[1].toInt();

    if ( colType == JAG_C_COL_TYPE_SQUARE ) {
		JagSquare2D sq( sp, srid );
		JagPolygon pgon( sq );
		pgon.toWKT( false, true, "polygon", value );
		return true;
    } else if ( colType == JAG_C_COL_TYPE_RECTANGLE ) {
		JagRectangle2D rect( sp, srid );
		JagPolygon pgon( rect );
		pgon.toWKT( false, true, "polygon", value );
		return true;
    } else if ( colType == JAG_C_COL_TYPE_CIRCLE ) {
		JagCircle2D cir( sp, srid );
		JagPolygon pgon( cir, samples );
		pgon.toWKT( false, true, "polygon", value );
		return true;
    } else if ( colType == JAG_C_COL_TYPE_ELLIPSE ) {
		JagEllipse2D e( sp, srid );
		JagPolygon pgon( e, samples );
		pgon.toWKT( false, true, "polygon", value );
		return true;
    } else if ( colType == JAG_C_COL_TYPE_TRIANGLE ) {
		JagTriangle2D t( sp, srid );
		JagPolygon pgon( t );
		pgon.toWKT( false, true, "polygon", value );
		return true;
	}

	if ( colType == JAG_C_COL_TYPE_POINT ) {
		value = Jstr("point(") + sp[JAG_SP_START+0] + " " + sp[JAG_SP_START+1] + ")";
	} else if ( colType == JAG_C_COL_TYPE_POINT3D ) {
		value = Jstr("point(") + sp[JAG_SP_START+0] + " " + sp[JAG_SP_START+1] + " " + sp[JAG_SP_START+2] + ")";
	} else if ( colType == JAG_C_COL_TYPE_LINE  ) {
		value = Jstr("linestring(") + sp[JAG_SP_START+0] + " " + sp[JAG_SP_START+1] 
					+ "," + sp[JAG_SP_START+2] + " " + sp[JAG_SP_START+3] + ")";
	} else if ( colType == JAG_C_COL_TYPE_LINE3D ) {
		value = Jstr("linestring(") + sp[JAG_SP_START+0] + " " + sp[JAG_SP_START+1] + " " + sp[JAG_SP_START+2] 
					+ "," + sp[JAG_SP_START+3] + " " + sp[JAG_SP_START+4] + " " + sp[JAG_SP_START+5] + ")";
	} else if ( colType == JAG_C_COL_TYPE_TRIANGLE3D  ) {
		double x1 = jagatof( sp[JAG_SP_START+0] );
		double y1 = jagatof( sp[JAG_SP_START+1] );
		double z1 = jagatof( sp[JAG_SP_START+2] );
		double x2 = jagatof( sp[JAG_SP_START+3] );
		double y2 = jagatof( sp[JAG_SP_START+4] );
		double z2 = jagatof( sp[JAG_SP_START+5] );
		double x3 = jagatof( sp[JAG_SP_START+6] );
		double y3 = jagatof( sp[JAG_SP_START+7] );
		double z3 = jagatof( sp[JAG_SP_START+8] );
		JagTriangle3D t(x1,y1,z1, x2,y2,z2, x3,y3,z3 );
		JagPolygon pgon( t );
		pgon.toWKT( true, true, "polygon", value );
	} else if ( colType == JAG_C_COL_TYPE_SQUARE3D ) {
		double px = jagatof( sp[JAG_SP_START+0].c_str() );
		double py = jagatof( sp[JAG_SP_START+1].c_str() );
		double pz = jagatof( sp[JAG_SP_START+2].c_str() );
		double r = jagatof( sp[JAG_SP_START+3].c_str() );
		double nx = JagGeo::safeget(sp, JAG_SP_START+4);
		double ny = JagGeo::safeget(sp, JAG_SP_START+5);
		JagSquare3D sq(px,py,pz, r, nx,ny );
		JagPolygon pgon( sq );
		pgon.toWKT( true, true, "polygon", value );
	} else if ( colType == JAG_C_COL_TYPE_RECTANGLE3D ) {
		double x = jagatof( sp[JAG_SP_START+0].c_str() );
		double y = jagatof( sp[JAG_SP_START+1].c_str() );
		double z = jagatof( sp[JAG_SP_START+2].c_str() );
		double a = jagatof( sp[JAG_SP_START+3].c_str() );
		double b = jagatof( sp[JAG_SP_START+4].c_str() );
		double nx = JagGeo::safeget(sp, JAG_SP_START+5);
		double ny = JagGeo::safeget(sp, JAG_SP_START+6);
		JagRectangle3D rect(x,y,z, a, b, nx,ny );
		JagPolygon pgon( rect );
		pgon.toWKT( true, true, "polygon", value );
	} else if ( colType == JAG_C_COL_TYPE_CIRCLE3D ) {
		double x = jagatof( sp[JAG_SP_START+0].c_str() );
		double y = jagatof( sp[JAG_SP_START+1].c_str() );
		double z = jagatof( sp[JAG_SP_START+2].c_str() );
		double r = jagatof( sp[JAG_SP_START+3].c_str() );
		double nx = JagGeo::safeget(sp, JAG_SP_START+4);
		double ny = JagGeo::safeget(sp, JAG_SP_START+5);
		JagCircle3D cir(x,y,z, r, nx,ny );
		JagPolygon pgon( cir);
		pgon.toWKT( true, true, "polygon", value );
	} else if ( colType == JAG_C_COL_TYPE_ELLIPSE3D ) {
		double x = jagatof( sp[JAG_SP_START+0].c_str() );
		double y = jagatof( sp[JAG_SP_START+1].c_str() );
		double z = jagatof( sp[JAG_SP_START+2].c_str() );
		double a = jagatof( sp[JAG_SP_START+3].c_str() );
		double b = jagatof( sp[JAG_SP_START+4].c_str() );
		double nx = JagGeo::safeget(sp, JAG_SP_START+5);
		double ny = JagGeo::safeget(sp, JAG_SP_START+6);
		JagEllipse3D cir(x,y,z, a,b,nx,ny );
		JagPolygon pgon( cir);
		pgon.toWKT( true, true, "polygon", value );
	} else if ( colType == JAG_C_COL_TYPE_LINESTRING ) {
		JagPolygon pgon;
		int rc = JagParser::addPolygonData( pgon, sp, false );
		if ( rc < 0 ) return false;
		pgon.toOneWKT( false, true, "linestring", value );
	} else if ( colType == JAG_C_COL_TYPE_MULTILINESTRING ) {
		JagPolygon pgon;
		int rc = JagParser::addPolygonData( pgon, sp, false );
		if ( rc < 0 ) return false;
		pgon.toWKT( false, true, "multilinestring", value );
	} else if ( colType == JAG_C_COL_TYPE_LINESTRING3D ) {
		JagPolygon pgon;
		int rc = JagParser::addPolygon3DData( pgon, sp, false );
		if ( rc < 0 ) return false;
		pgon.toOneWKT( true, true, "linestring", value );
	} else if ( colType == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
		JagPolygon pgon;
		int rc = JagParser::addPolygon3DData( pgon, sp, false );
		if ( rc < 0 ) return false;
		pgon.toWKT( true, true, "multilinestring", value );
	} else if ( colType == JAG_C_COL_TYPE_MULTIPOINT ) {
		JagPolygon pgon;
		int rc = JagParser::addPolygonData( pgon, sp, false );
		if ( rc < 0 ) return false;
		pgon.toOneWKT( false, true, "multipoint", value );
	} else if ( colType == JAG_C_COL_TYPE_MULTIPOINT3D ) {
		JagPolygon pgon;
		int rc = JagParser::addPolygon3DData( pgon, sp, false );
		if ( rc < 0 ) return false;
		pgon.toOneWKT( true, true, "multipoint", value );
	} else if ( colType == JAG_C_COL_TYPE_POLYGON ) {
		JagPolygon pgon;
		int rc = JagParser::addPolygonData( pgon, sp, false );
		if ( rc < 0 ) return false;
		pgon.toWKT( false, true, "polygon", value );
	} else if ( colType == JAG_C_COL_TYPE_POLYGON3D ) {
		JagPolygon pgon;
		int rc = JagParser::addPolygon3DData( pgon, sp, false );
		if ( rc < 0 ) return false;
		pgon.toWKT( true, true, "polygon", value );
	} else if ( colType == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		JagVector<JagPolygon> pgvec;
        int n = JagParser::addMultiPolygonData( pgvec, sp, false, false );
        if ( n <= 0 ) return false;
		JagGeo::multiPolygonToWKT( pgvec, false, value );
	} else if ( colType == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
		JagVector<JagPolygon> pgvec;
        int n = JagParser::addMultiPolygonData( pgvec, sp, false, true );
        if ( n <= 0 ) return false;
		JagGeo::multiPolygonToWKT( pgvec, true, value );
	} else {
		return false;
	}

	return true;
}

bool BinaryOpNode::doAllMinimumBoundingCircle( const Jstr& mk, const Jstr &colType, 
								    int srid, JagStrSplit &sp, const Jstr &carg,  Jstr &value )
{
	value = Jstr("CJAG=") + intToStr(srid) + "=0=CR=d 0:0:0:0 "; 
    if ( colType == JAG_C_COL_TYPE_SQUARE ) {
		double r = sp[JAG_SP_START+2].tof() * 1.41421356237;
		value += sp[JAG_SP_START+0] + " " + sp[JAG_SP_START+1] + " " + d2s(r);
		return true;
    } else if ( colType == JAG_C_COL_TYPE_RECTANGLE ) {
		double a = sp[JAG_SP_START+2].tof();
		double b = sp[JAG_SP_START+3].tof();
		double r = sqrt(a*a+ b*b);
		value += sp[JAG_SP_START+0] + " " + sp[JAG_SP_START+1] + " " + d2s(r);
		return true;
    } else if ( colType == JAG_C_COL_TYPE_CIRCLE ) {
		value += sp[JAG_SP_START+0] + " " + sp[JAG_SP_START+1] + " " + sp[JAG_SP_START+2];
		return true;
    } else if ( colType == JAG_C_COL_TYPE_ELLIPSE ) {
		double a = sp[JAG_SP_START+2].tof();
		double b = sp[JAG_SP_START+3].tof();
		double r = jagmax(a, b);
		value += sp[JAG_SP_START+0] + " " + sp[JAG_SP_START+1] + " " + d2s(r);
		return true;
    } else if ( colType == JAG_C_COL_TYPE_TRIANGLE ) {
		JagVector<JagPoint2D> vec;
		vec.append( JagPoint2D( sp[JAG_SP_START+0].tof(), sp[JAG_SP_START+1].tof() ) );
		vec.append( JagPoint2D( sp[JAG_SP_START+2].tof(), sp[JAG_SP_START+3].tof() ) );
		vec.append( JagPoint2D( sp[JAG_SP_START+4].tof(), sp[JAG_SP_START+5].tof() ) );
		Jstr cir;
		if ( ! JagCGAL::getMinBoundCircle( srid, vec, cir ) ) return false;
		value += cir;
		return true;
	} else if ( colType == JAG_C_COL_TYPE_POINT ) {
		value += sp[JAG_SP_START+0] + " " + sp[JAG_SP_START+1] + " 0.0";
	} else if ( colType == JAG_C_COL_TYPE_LINE  ) {
		JagVector<JagPoint2D> vec;
		vec.append( JagPoint2D( sp[JAG_SP_START+0].tof(), sp[JAG_SP_START+1].tof() ) );
		vec.append( JagPoint2D( sp[JAG_SP_START+2].tof(), sp[JAG_SP_START+3].tof() ) );
		Jstr cir;
		if ( ! JagCGAL::getMinBoundCircle( srid, vec, cir ) ) return false;
		value += cir;
	} else if ( colType == JAG_C_COL_TYPE_LINESTRING || JAG_C_COL_TYPE_MULTILINESTRING ) {
		JagPolygon pgon;
		int rc = JagParser::addPolygonData( pgon, sp, false );
		if ( rc < 0 ) return false;
		JagVector<JagPoint2D> vec;
		pgon.toVector2D( srid, vec, true );
		Jstr cir;
		if ( ! JagCGAL::getMinBoundCircle( srid, vec, cir ) ) return false;
		value += cir;
	} else if ( colType == JAG_C_COL_TYPE_MULTIPOINT ) {
		JagPolygon pgon;
		int rc = JagParser::addPolygonData( pgon, sp, false );
		if ( rc < 0 ) return false;
		JagVector<JagPoint2D> vec;
		pgon.toVector2D( srid, vec, true );
		Jstr cir;
		if ( ! JagCGAL::getMinBoundCircle( srid, vec, cir ) ) return false;
		value += cir;
	} else if ( colType == JAG_C_COL_TYPE_POLYGON ) {
		JagPolygon pgon;
		int rc = JagParser::addPolygonData( pgon, sp, false );
		if ( rc < 0 ) return false;
		JagVector<JagPoint2D> vec;
		pgon.toVector2D( srid, vec, true );
		Jstr cir;
		if ( ! JagCGAL::getMinBoundCircle( srid, vec, cir ) ) return false;
		value += cir;
	} else if ( colType == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		JagVector<JagPolygon> pgvec;
        int n = JagParser::addMultiPolygonData( pgvec, sp, false, false );
        if ( n <= 0 ) return false;
		JagVector<JagPoint2D> vec;
		JagGeo::multiPolygonToVector2D( srid, pgvec, true, vec );
		Jstr cir;
		if ( ! JagCGAL::getMinBoundCircle( srid, vec, cir ) ) return false;
		value += cir;
	} else {
		return false;
	}

	return true;
}

bool BinaryOpNode::doAllMinimumBoundingSphere( const Jstr& mk, const Jstr &colType, 
								    int srid, JagStrSplit &sp, const Jstr &carg,  Jstr &value )
{
	value = Jstr("CJAG=") + intToStr(srid) + "=0=SR=d 0:0:0:0:0:0 "; 
	if ( colType == JAG_C_COL_TYPE_POINT3D ) {
		value += sp[JAG_SP_START+0] + " " + sp[JAG_SP_START+1] + " " + sp[JAG_SP_START+2] + " 0.0";
	} else if ( colType == JAG_C_COL_TYPE_LINE3D ) {
		JagVector<JagPoint3D> vec;
		vec.append( JagPoint3D( sp[JAG_SP_START+0].tof(), sp[JAG_SP_START+1].tof(), sp[JAG_SP_START+2].tof() ) );
		vec.append( JagPoint3D( sp[JAG_SP_START+3].tof(), sp[JAG_SP_START+4].tof(), sp[JAG_SP_START+5].tof() ) );
		Jstr sphere;
		if ( ! JagCGAL::getMinBoundSphere( srid, vec, sphere ) ) return false;
		value += sphere;
	} else if ( colType == JAG_C_COL_TYPE_TRIANGLE3D  ) {
		JagVector<JagPoint3D> vec;
		vec.append( JagPoint3D( sp[JAG_SP_START+0].tof(), sp[JAG_SP_START+1].tof(), sp[JAG_SP_START+2].tof() ) );
		vec.append( JagPoint3D( sp[JAG_SP_START+3].tof(), sp[JAG_SP_START+4].tof(), sp[JAG_SP_START+5].tof() ) );
		vec.append( JagPoint3D( sp[JAG_SP_START+6].tof(), sp[JAG_SP_START+7].tof(), sp[JAG_SP_START+8].tof() ) );
		Jstr sphere;
		if ( ! JagCGAL::getMinBoundSphere( srid, vec, sphere ) ) return false;
		value += sphere;
	} else if ( colType == JAG_C_COL_TYPE_SQUARE3D ) {
		double px = jagatof( sp[JAG_SP_START+0].c_str() );
		double py = jagatof( sp[JAG_SP_START+1].c_str() );
		double pz = jagatof( sp[JAG_SP_START+2].c_str() );
		double r = jagatof( sp[JAG_SP_START+3].c_str() );
		double nx = JagGeo::safeget(sp, JAG_SP_START+4);
		double ny = JagGeo::safeget(sp, JAG_SP_START+5);
		JagSquare3D sq(px,py,pz, r, nx,ny );
		JagPolygon pgon( sq );
		JagVector<JagPoint3D> vec;
		pgon.toVector3D( srid, vec, true );
		Jstr sphere;
		if ( ! JagCGAL::getMinBoundSphere( srid, vec, sphere ) ) return false;
		value += sphere;
	} else if ( colType == JAG_C_COL_TYPE_RECTANGLE3D ) {
		double x = jagatof( sp[JAG_SP_START+0].c_str() );
		double y = jagatof( sp[JAG_SP_START+1].c_str() );
		double z = jagatof( sp[JAG_SP_START+2].c_str() );
		double a = jagatof( sp[JAG_SP_START+3].c_str() );
		double b = jagatof( sp[JAG_SP_START+4].c_str() );
		double nx = JagGeo::safeget(sp, JAG_SP_START+5);
		double ny = JagGeo::safeget(sp, JAG_SP_START+6);
		JagRectangle3D rect(x,y,z, a, b, nx,ny );
		JagPolygon pgon( rect );
		JagVector<JagPoint3D> vec;
		pgon.toVector3D( srid, vec, true );
		Jstr sphere;
		if ( ! JagCGAL::getMinBoundSphere( srid, vec, sphere ) ) return false;
		value += sphere;
	} else if ( colType == JAG_C_COL_TYPE_CIRCLE3D ) {
		value = sp.c_str();
	} else if ( colType == JAG_C_COL_TYPE_ELLIPSE3D ) {
		double a = jagatof( sp[JAG_SP_START+3].c_str() );
		double b = jagatof( sp[JAG_SP_START+4].c_str() );
		double r = jagmax( a, b);
		value += sp[JAG_SP_START+0] + " " + sp[JAG_SP_START+1] + " " + sp[JAG_SP_START+2] + " " + d2s(r); 
	} else if ( colType == JAG_C_COL_TYPE_LINESTRING3D || colType == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
		JagPolygon pgon;
		int rc = JagParser::addPolygon3DData( pgon, sp, false );
		if ( rc < 0 ) return false;
		JagVector<JagPoint3D> vec;
		pgon.toVector3D( srid, vec, true );
		Jstr sphere;
		if ( ! JagCGAL::getMinBoundSphere( srid, vec, sphere ) ) return false;
		value += sphere;
	} else if ( colType == JAG_C_COL_TYPE_MULTIPOINT3D ) {
		JagPolygon pgon;
		int rc = JagParser::addPolygon3DData( pgon, sp, false );
		if ( rc < 0 ) return false;
		JagVector<JagPoint3D> vec;
		pgon.toVector3D( srid, vec, true );
		Jstr sphere;
		if ( ! JagCGAL::getMinBoundSphere( srid, vec, sphere ) ) return false;
		value += sphere;
	} else if ( colType == JAG_C_COL_TYPE_POLYGON3D ) {
		JagPolygon pgon;
		int rc = JagParser::addPolygon3DData( pgon, sp, false );
		if ( rc < 0 ) return false;
		JagVector<JagPoint3D> vec;
		pgon.toVector3D( srid, vec, true );
		Jstr sphere;
		if ( ! JagCGAL::getMinBoundSphere( srid, vec, sphere ) ) return false;
		value += sphere;
	} else if ( colType == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
		JagVector<JagPolygon> pgvec;
        int n = JagParser::addMultiPolygonData( pgvec, sp, false, true );
        if ( n <= 0 ) return false;
		JagVector<JagPoint3D> vec;
		JagGeo::multiPolygonToVector3D( srid, pgvec, true, vec );
		Jstr sphere;
		if ( ! JagCGAL::getMinBoundSphere( srid, vec, sphere ) ) return false;
		value += sphere;
	} else {
		return false;
	}

	return true;
}

Jstr BinaryOpNode::doAllKNN( int srid, const Jstr &colType, JagStrSplit &sp, 
							 const Jstr &colType2, JagStrSplit &sp2, const Jstr &carg )
{
	int dim1 = getDimension(colType);
	Jstr value;
	JagStrSplit ss(carg, ':');
	if ( ss.size() < 3 ) return "";
	int K = ss[0].toInt();
	double min = ss[1].tof();
	double max = ss[2].tof();

	if ( 2 == dim1 ) {
		value = Jstr("CJAG=") + intToStr(srid) + "=0=MP=d 0:0:0:0 "; 
	} else {
		value = Jstr("CJAG=") + intToStr(srid) + "=0=MP3=d 0:0:0:0:0:0 "; 
	}

	double px, py, pz;
	if ( colType2 == JAG_C_COL_TYPE_POINT ) {
		px = sp2[JAG_SP_START+0].tof();
		py = sp2[JAG_SP_START+1].tof();
	} else if ( colType2 == JAG_C_COL_TYPE_POINT3D ) {
		px = sp2[JAG_SP_START+0].tof();
		py = sp2[JAG_SP_START+1].tof();
		pz = sp2[JAG_SP_START+2].tof();
	} else {
		return "";
	}

	int rc2;

		if (  colType == JAG_C_COL_TYPE_POINT ) {
			value += sp[JAG_SP_START+0] + ":" + sp[JAG_SP_START+1];
		} else if ( colType == JAG_C_COL_TYPE_POINT3D ) {
			value += sp[JAG_SP_START+0] + ":" + sp[JAG_SP_START+1] + ":" + sp[JAG_SP_START+2];
		} else if ( colType == JAG_C_COL_TYPE_LINE ) {
			JagPolygon pgon;
			pgon.add( sp[JAG_SP_START+0].tof(), sp[JAG_SP_START+1].tof() );
			pgon.add( sp[JAG_SP_START+2].tof(), sp[JAG_SP_START+3].tof() );
			Jstr val; pgon.knn( 2, srid, px,py,pz,K, min, max, val );
			value += val;
		} else if ( colType == JAG_C_COL_TYPE_LINE3D ) {
			JagPolygon pgon;
			pgon.add( sp[JAG_SP_START+0].tof(), sp[JAG_SP_START+1].tof(), sp[JAG_SP_START+2].tof() );
			pgon.add( sp[JAG_SP_START+3].tof(), sp[JAG_SP_START+4].tof(), sp[JAG_SP_START+5].tof() );
			Jstr val; pgon.knn( 3, srid, px,py,pz,K, min, max, val );
			value += val;
		} else if ( colType == JAG_C_COL_TYPE_TRIANGLE ) {
			JagTriangle2D s( sp, srid );
			JagPolygon pgon( s );
			Jstr val; pgon.knn( 2, srid, px,py,pz,K, min, max, val );
			value += val;
		} else if ( colType == JAG_C_COL_TYPE_TRIANGLE3D ) {
			JagTriangle3D s( sp, srid );
			JagPolygon pgon( s );
			Jstr val; pgon.knn( 3, srid, px,py,pz,K, min, max, val );
			value += val;
		} else if ( colType == JAG_C_COL_TYPE_SQUARE ) {
			JagSquare2D sq( sp, srid );
			JagPolygon pgon( sq );
			Jstr val; pgon.knn( 2, srid, px,py,pz,K, min, max, val );
			value += val;
		} else if ( colType == JAG_C_COL_TYPE_SQUARE3D ) {
			JagRectangle3D sq( sp, srid );
			JagPolygon pgon( sq );
			Jstr val; pgon.knn( 3, srid, px,py,pz,K, min, max, val );
			value += val;
		} else if ( colType == JAG_C_COL_TYPE_CIRCLE ) {
			JagCircle2D sq( sp, srid );
			JagPolygon pgon( sq );
			Jstr val; pgon.knn( 2, srid, px,py,pz,K, min, max, val );
			value += val;
		} else if ( colType == JAG_C_COL_TYPE_CIRCLE3D ) {
			JagCircle3D sq( sp, srid );
			JagPolygon pgon( sq );
			Jstr val; pgon.knn( 3, srid, px,py,pz,K, min, max, val );
			value += val;
		} else if ( colType == JAG_C_COL_TYPE_ELLIPSE ) {
			JagEllipse2D e( sp, srid );
			JagPolygon pgon( e );
			Jstr val; pgon.knn( 2, srid, px,py,pz,K, min, max, val );
			value += val;
		} else if ( colType == JAG_C_COL_TYPE_ELLIPSE3D ) {
			JagEllipse3D e( sp, srid );
			JagPolygon pgon( e );
			Jstr val; pgon.knn( 3, srid, px,py,pz,K, min, max, val );
			value += val;
		} else if ( colType == JAG_C_COL_TYPE_POLYGON || colType == JAG_C_COL_TYPE_LINESTRING || colType == JAG_C_COL_TYPE_MULTIPOINT ) {
			JagPolygon pgon;
			rc2 = JagParser::addPolygonData( pgon, sp, false );
			if ( rc2 <= 0 ) return false; 
			Jstr val; pgon.knn( 2, srid, px,py,pz,K, min, max, val );
			value += val;
		} else if ( colType == JAG_C_COL_TYPE_POLYGON3D || colType == JAG_C_COL_TYPE_LINESTRING3D || colType == JAG_C_COL_TYPE_MULTIPOINT3D ) {
			JagPolygon pgon;
			rc2 = JagParser::addPolygon3DData( pgon, sp, false );
			if ( rc2 <= 0 ) return false;
			Jstr val; pgon.knn( 3, srid, px,py,pz,K, min, max, val );
			value += val;
		} else if ( colType == JAG_C_COL_TYPE_MULTILINESTRING ) {
			JagPolygon pgon;
			rc2 = JagParser::addPolygonData( pgon, sp, false );
			if ( rc2 <= 0 ) return false;
			Jstr val; pgon.knn( 2, srid, px,py,pz,K, min, max, val );
			value += val;
		} else if ( colType == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
			JagPolygon pgon;
			rc2 = JagParser::addPolygon3DData( pgon, sp, false );
			if ( rc2 <= 0 ) return false;
			Jstr val; pgon.knn( 3, srid, px,py,pz,K, min, max, val );
			value += val;
		} else if ( colType == JAG_C_COL_TYPE_MULTIPOLYGON ) {
			JagVector<JagPolygon> pgvec;
			rc2 = JagParser::addMultiPolygonData( pgvec, sp, false, false );
			if ( rc2 <= 0 ) return false;
			Jstr val; JagGeo::knnfromVec( pgvec, 2, srid, px,py,pz,K, min, max, val );
			value += val;
		} else if ( colType == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
			JagVector<JagPolygon> pgvec;
			rc2 = JagParser::addMultiPolygonData( pgvec, sp, false, true );
			if ( rc2 <= 0 ) return false;
			Jstr val; JagGeo::knnfromVec( pgvec, 3, srid, px,py,pz,K, min, max, val );
			value += val;
		} 
		return value;
}

bool BinaryOpNode::doAllInnerRingN( const Jstr& mk, const Jstr& hdr, const Jstr &colType, 
								    int srid, const JagStrSplit &sp, const Jstr &carg,  Jstr &value )
{
	int N = jagatoi( carg.c_str() ) + 1;
	if ( N <= 1 ) return false;
	value = "";
	Jstr bbox;
	if ( mk == JAG_OJAG ) { bbox = sp[1]; } 

	JagLineString line;
	JagPolygon pgon;
    if ( colType == JAG_C_COL_TYPE_POLYGON ) {
	    JagParser::addPolygonData( pgon, sp, false );
		if ( pgon.linestr.size() < N ) return false;
		line.copyFrom( pgon.linestr[N-1], false );
		JagCGAL::getRingStr( line, hdr, bbox, false, value );
    } else if ( colType == JAG_C_COL_TYPE_POLYGON3D ) {
	    JagParser::addPolygon3DData( pgon, sp, false );
		if ( pgon.linestr.size() < N ) return false;
		line.copyFrom( pgon.linestr[N-1], false );
		JagCGAL::getRingStr( line, hdr, bbox, true, value );
    } else {
		return false;
	}

	return true;
}

bool BinaryOpNode::doAllPolygonN( const Jstr& mk, const Jstr& hdr, const Jstr &colType, 
								    const JagStrSplit &sp, const Jstr &carg, Jstr &value )
{
	value = "";
	int N = jagatoi( carg.c_str() );
	if ( N <= 0 ) return false;

	Jstr bbox;
	if ( mk == JAG_OJAG ) { bbox = sp[1]; } 
	JagVector<JagPolygon> pgvec;
    if ( colType == JAG_C_COL_TYPE_MULTIPOLYGON ) {
	    JagParser::addMultiPolygonData( pgvec, sp, false, false );
		JagCGAL::getPolygonNStr( pgvec, hdr, bbox, false, N, value );
    } else if ( colType == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
	    JagParser::addMultiPolygonData( pgvec, sp, false, true );
		JagCGAL::getPolygonNStr( pgvec, hdr, bbox, true, N, value );
    } else {
		return false;
	}

	return true;
}

bool BinaryOpNode::doAllNumPolygons( const Jstr& mk, const Jstr& hdr, const Jstr &colType, 
								    const JagStrSplit &sp, Jstr &value )
{
	int num;
	value = "0";
    if ( colType == JAG_C_COL_TYPE_MULTIPOLYGON || colType == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
		num = 1;
		for ( int i=JAG_SP_START; i < sp.length(); ++i ) {
			if ( sp[i] == "!" ) ++num;
		}
		value = intToStr( num );
	} else if ( colType == JAG_C_COL_TYPE_POLYGON || colType == JAG_C_COL_TYPE_POLYGON3D ) {
		value = "1"; 
    } else {
		return true;
	}

	return true;
}

bool BinaryOpNode::doAllUnique( const Jstr& mk, const Jstr& hdr, const Jstr &colType, 
								    const JagStrSplit &sp, Jstr &value )
{
	Jstr bbox = sp[1];
	value = "";
    if ( colType == JAG_C_COL_TYPE_POLYGON
	     || colType == JAG_C_COL_TYPE_POLYGON3D 
	     || colType == JAG_C_COL_TYPE_LINESTRING 
	     || colType == JAG_C_COL_TYPE_LINESTRING3D 
	     || colType == JAG_C_COL_TYPE_MULTILINESTRING
	     || colType == JAG_C_COL_TYPE_MULTILINESTRING3D 
	     || colType == JAG_C_COL_TYPE_MULTIPOLYGON
	     || colType == JAG_C_COL_TYPE_MULTIPOLYGON3D
		 ) {
		JagCGAL::getUniqueStr( sp, hdr, bbox, value );
    } else {
		value = sp.c_str();
	}

	return true;
}

bool BinaryOpNode::doAllBuffer( const Jstr& mk, const Jstr& hdr, const Jstr &colType, 
								    int srid, const JagStrSplit &sp, const Jstr &carg,  Jstr &value )
{
	value = "";
	JagLineString line;
	JagPolygon pgon;
	JagVector<JagPolygon> pgvec;
	double px, py;
	bool rc;
    if ( colType == JAG_C_COL_TYPE_LINESTRING || colType == JAG_C_COL_TYPE_MULTIPOINT ) {
		JagParser::addLineStringData( line, sp );
    } else if ( colType == JAG_C_COL_TYPE_POLYGON ) {
	    JagParser::addPolygonData( pgon, sp, true );
		line.copyFrom( pgon.linestr[0], false );
    } else if ( colType == JAG_C_COL_TYPE_MULTILINESTRING ) {
	    JagParser::addPolygonData( pgon, sp, false );
    } else if ( colType == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		JagParser::addMultiPolygonData( pgvec, sp, false, false );
	} else {
	}


    if ( colType == JAG_C_COL_TYPE_POINT ) {
		px = jagatof( sp[JAG_SP_START+0] );
		py = jagatof( sp[JAG_SP_START+1] );
		line.add(px, py);
		rc = JagCGAL::getBufferMultiPoint2DStr( line, srid, carg, value );
	} else if ( colType == JAG_C_COL_TYPE_LINE ) {
		px = jagatof( sp[JAG_SP_START+0] );
		py = jagatof( sp[JAG_SP_START+1] );
		line.add(px, py);
		px = jagatof( sp[JAG_SP_START+2] );
		py = jagatof( sp[JAG_SP_START+3] );
		line.add(px, py);
		rc = JagCGAL::getBufferLineString2DStr( line, srid, carg, value );
	} else if ( colType == JAG_C_COL_TYPE_LINESTRING ) {
		rc = JagCGAL::getBufferLineString2DStr( line, srid, carg, value );
    } else if (  colType == JAG_C_COL_TYPE_MULTIPOINT ) {
		rc = JagCGAL::getBufferMultiPoint2DStr( line, srid, carg, value );
    } else if ( colType == JAG_C_COL_TYPE_POLYGON ) {
		rc = JagCGAL::getBufferPolygon2DStr( pgon, srid, carg, value );
    } else if ( colType == JAG_C_COL_TYPE_MULTILINESTRING ) {
		rc = JagCGAL::getBufferMultiLineString2DStr( pgon, srid, carg, value );
    } else if ( colType == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		rc = JagCGAL::getBufferMultiPolygon2DStr( pgvec, srid, carg, value );
    } else if ( colType == JAG_C_COL_TYPE_SQUARE ) {
		px = jagatof( sp[JAG_SP_START+0] );
		py = jagatof( sp[JAG_SP_START+1] );
		double a = jagatof( sp[JAG_SP_START+2] );
		double nx = jagatof( sp[JAG_SP_START+3] );
		JagSquare2D sq(px,py, a, nx );
		JagPolygon pgon( sq );
		rc = JagCGAL::getBufferPolygon2DStr( pgon, srid, carg, value );
    } else if ( colType == JAG_C_COL_TYPE_CIRCLE ) {
		px = jagatof( sp[JAG_SP_START+0] );
		py = jagatof( sp[JAG_SP_START+1] );
		double a = jagatof( sp[JAG_SP_START+2] );
		JagCircle2D cir(px,py, a );
		JagPolygon pgon( cir );
		rc = JagCGAL::getBufferPolygon2DStr( pgon, srid, carg, value );
    } else if ( colType == JAG_C_COL_TYPE_RECTANGLE ) {
		px = jagatof( sp[JAG_SP_START+0] );
		py = jagatof( sp[JAG_SP_START+1] );
		double a = jagatof( sp[JAG_SP_START+2] );
		double b = jagatof( sp[JAG_SP_START+3] );
		double nx = jagatof( sp[JAG_SP_START+4] );
		JagRectangle2D rect(px,py, a, b, nx );
		JagPolygon pgon( rect );
		rc = JagCGAL::getBufferPolygon2DStr( pgon, srid, carg, value );
    } else if ( colType == JAG_C_COL_TYPE_ELLIPSE ) {
		px = jagatof( sp[JAG_SP_START+0] );
		py = jagatof( sp[JAG_SP_START+1] );
		double a = jagatof( sp[JAG_SP_START+2] );
		double b = jagatof( sp[JAG_SP_START+3] );
		double nx = jagatof( sp[JAG_SP_START+4] );
		JagEllipse2D e(px,py, a, b, nx );
		JagPolygon pgon( e );
		rc = JagCGAL::getBufferPolygon2DStr( pgon, srid, carg, value );
    } else if ( colType == JAG_C_COL_TYPE_TRIANGLE ) {
		double x1 = jagatof( sp[JAG_SP_START+0] );
		double y1 = jagatof( sp[JAG_SP_START+1] );
		double x2 = jagatof( sp[JAG_SP_START+2] );
		double y2 = jagatof( sp[JAG_SP_START+3] );
		double x3 = jagatof( sp[JAG_SP_START+4] );
		double y3 = jagatof( sp[JAG_SP_START+5] );
		JagTriangle2D t(x1,y1, x2,y2, x3,y3 );
		JagPolygon pgon( t );
		rc = JagCGAL::getBufferPolygon2DStr( pgon, srid, carg, value );
	} else  {
		rc = false;
	}

	return rc;
}

bool BinaryOpNode::doAllEndPoint( const Jstr& mk, const Jstr &colType, const JagStrSplit &sp, Jstr &value )
{
	value = "";
	if ( JagParser::isVectorGeoType( colType ) 
	     && colType != JAG_C_COL_TYPE_TRIANGLE 
	     && colType != JAG_C_COL_TYPE_TRIANGLE3D 
	   ) {
		 return false;
	 }

	if ( colType == JAG_C_COL_TYPE_LINE ) {
    	value = trimEndZeros(sp[2]) + " " + trimEndZeros(sp[3]);
	} else if ( colType == JAG_C_COL_TYPE_LINE3D  ) {
    	value = trimEndZeros(sp[3]) + " " + trimEndZeros(sp[4]) + " " + trimEndZeros(sp[5]);
	} else if ( colType == JAG_C_COL_TYPE_TRIANGLE ) {
		value = trimEndZeros(sp[4]) + " " + trimEndZeros(sp[5]);
	} else if ( colType == JAG_C_COL_TYPE_TRIANGLE3D ) {
		value = trimEndZeros(sp[6]) + " " + trimEndZeros(sp[7]) + " " + trimEndZeros(sp[8]);
	} else {
		int len = sp.length();
		value = sp[len-1];
		value.replace(':', ' ' );
	}

	return true;
}

bool BinaryOpNode::doAllIsRing( const Jstr& mk, const Jstr &colType, const JagStrSplit &sp, Jstr &value )
{
	if ( JagParser::isVectorGeoType( colType ) ) {
		value = "1";
		return true;
	}

	bool rc;
    if ( colType == JAG_C_COL_TYPE_LINESTRING ) {
		JagLineString line;
		JagParser::addLineStringData( line, sp );
		rc = JagCGAL::getIsRingLineString2DStr( line );
    } else {
		rc = false;
	}
	
	if ( rc ) value = "1";
	else value = "0";
	return true;
}

bool BinaryOpNode::doAllIsPolygonCCW( const Jstr& mk, const Jstr &colType, const JagStrSplit &sp, Jstr &value )
{
    if ( colType != JAG_C_COL_TYPE_POLYGON ) {
		value = "0";
		return true;
	}
	bool rc = JagGeo::isPolygonCCW( sp );
	if ( rc ) value = "1";
	else value = "0";
	return true;
}

bool BinaryOpNode::doAllIsPolygonCW( const Jstr& mk, const Jstr &colType, const JagStrSplit &sp, Jstr &value )
{
    if ( colType != JAG_C_COL_TYPE_POLYGON ) {
		value = "0";
		return true;
	}
	bool rc = JagGeo::isPolygonCW( sp );
	if ( rc ) value = "1";
	else value = "0";
	return true;
}


bool BinaryOpNode::doAllIsValid( const Jstr& mk, const Jstr &colType, const JagStrSplit &sp, Jstr &value )
{
	if ( JagParser::isVectorGeoType( colType ) ) {
		value = "1";
		return true;
	}

	bool rc;

	rc = false;
    if ( colType == JAG_C_COL_TYPE_LINESTRING ) {
		JagLineString line;
		JagParser::addLineStringData( line, sp );
		rc = JagCGAL::getIsValidLineString2DStr( line );
    } else if ( colType == JAG_C_COL_TYPE_POLYGON ) {
		JagPolygon pgon;
	    JagParser::addPolygonData( pgon, sp, true );
		rc = JagCGAL::getIsValidPolygon2DStr( pgon );
    } else if ( colType == JAG_C_COL_TYPE_MULTILINESTRING ) {
		JagPolygon pgon;
	    JagParser::addPolygonData( pgon, sp, false );
		rc = JagCGAL::getIsValidMultiLineString2DStr( pgon );
    } else if ( colType == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		JagVector<JagPolygon> pgvec;
		JagParser::addMultiPolygonData( pgvec, sp, false, false );
		rc = JagCGAL::getIsValidMultiPolygon2DStr( pgvec );
    } else if ( colType == JAG_C_COL_TYPE_MULTIPOINT ) {
		JagLineString line;
		JagParser::addLineStringData( line, sp );
		rc = JagCGAL::getIsValidMultiPoint2DStr( line );
	} else  {
	}

	if ( rc ) value = "1";
	else value = "0";
	return true;
}

bool BinaryOpNode::doAllIsSimple( const Jstr& mk, const Jstr &colType, const JagStrSplit &sp, Jstr &value )
{
	if ( JagParser::isVectorGeoType( colType ) ) {
		value = "1";
		return true;
	}

	bool rc;

	rc = false;
    if ( colType == JAG_C_COL_TYPE_LINESTRING ) {
		JagLineString line;
		JagParser::addLineStringData( line, sp );
		rc = JagCGAL::getIsSimpleLineString2DStr( line );
    } else if ( colType == JAG_C_COL_TYPE_POLYGON ) {
		JagPolygon pgon;
	    JagParser::addPolygonData( pgon, sp, true );
		rc = JagCGAL::getIsSimplePolygon2DStr( pgon );
    } else if ( colType == JAG_C_COL_TYPE_MULTILINESTRING ) {
		JagPolygon pgon;
	    JagParser::addPolygonData( pgon, sp, false );
		rc = JagCGAL::getIsSimpleMultiLineString2DStr( pgon );
    } else if ( colType == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		JagVector<JagPolygon> pgvec;
		JagParser::addMultiPolygonData( pgvec, sp, false, false );
		rc = JagCGAL::getIsSimpleMultiPolygon2DStr( pgvec );
	} else  {
	}

	if ( rc ) value = "1";
	else value = "0";
	return true;
}

bool BinaryOpNode::doAllIsConvex( const Jstr& mk, const Jstr &colType, const JagStrSplit &sp, Jstr &value )
{
	if ( JagParser::isVectorGeoType( colType ) ) {
		value = "1";
		return true;
	}

	bool rc = false;
    if ( colType == JAG_C_COL_TYPE_POLYGON ) {
		JagPolygon pgon;
	    JagParser::addPolygonData( pgon, sp, true );
		rc = JagCGAL::isPolygonConvex( pgon );
	} else  {
	}

	if ( rc ) value = "1";
	else value = "0";
	return true;
}

bool BinaryOpNode::doAllIsClosed( const Jstr& mk, const Jstr &colType, const JagStrSplit &sp, Jstr &value )
{
	value = "0";
	if ( colType == JAG_C_COL_TYPE_LINE || colType == JAG_C_COL_TYPE_LINE3D ) {
		return true;
	}

	if ( colType == JAG_C_COL_TYPE_POLYGON || colType == JAG_C_COL_TYPE_POLYGON3D ) {
		value = "1";
		return true;
	}

	if ( colType == JAG_C_COL_TYPE_MULTIPOLYGON || colType == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
		value = "1";
		return true;
	}

	if ( JagParser::isVectorGeoType( colType ) ) {
		value = "1";
		return true;
	}

	Jstr first, last;
   	first = sp[JAG_SP_START]; 
	int len = sp.length();
	last = sp[len-1];
	double x1, y1, z1, x2, y2, z2;
	const char *str; char *p;
	if ( colType == JAG_C_COL_TYPE_LINESTRING ||  colType == JAG_C_COL_TYPE_MULTIPOINT ) {
		str = first.c_str();
		if ( strchrnum( str, ':') < 1 ) return true;
		get2double(str, p, ':', x1, y1 );

		str = last.c_str();
		if ( strchrnum( str, ':') < 1 ) return true;
		get2double(str, p, ':', x2, y2 );
		if ( jagEQ(x1,x2) && jagEQ(y1,y2) ) {
			value = "1";
		}
	} else if ( colType == JAG_C_COL_TYPE_LINESTRING3D || colType == JAG_C_COL_TYPE_MULTIPOINT3D ) {
		str = first.c_str();
		if ( strchrnum( str, ':') < 2 ) return true;
		get3double(str, p, ':', x1, y1, z1 );

		str = last.c_str();
		if ( strchrnum( str, ':') < 2 ) return true;
		get3double(str, p, ':', x2, y2, z2 );
		if ( jagEQ(x1,x2) && jagEQ(y1,y2) && jagEQ(z1,z2) ) {
			value = "1";
		}
	}

	return true;
}

bool BinaryOpNode::doAllNumPoints( const Jstr& mk, const Jstr &colType, const JagStrSplit &sp, Jstr &value )
{
	value = "0";

	if ( colType == JAG_C_COL_TYPE_POINT || colType == JAG_C_COL_TYPE_POINT3D ) {
		value = "1";
		return true;
	}

	if ( colType == JAG_C_COL_TYPE_LINE || colType == JAG_C_COL_TYPE_LINE3D ) {
		value = "2";
		return true;
	}

	if ( colType == JAG_C_COL_TYPE_TRIANGLE || colType == JAG_C_COL_TYPE_TRIANGLE3D ) {
		value = "3";
		return true;
	}

	if ( colType == JAG_C_COL_TYPE_SQUARE || colType == JAG_C_COL_TYPE_SQUARE3D ) {
		value = "4";
		return true;
	}

	if ( colType == JAG_C_COL_TYPE_BOX || colType == JAG_C_COL_TYPE_CUBE ) {
		value = "8";
		return true;
	}

	if ( JagParser::isVectorGeoType( colType ) ) {
		value = "0";
		return true;
	}

	int cnt = 0;
	for ( int i=JAG_SP_START; i < sp.size(); ++i ) {
		if ( sp[i] != "|" && sp[i] != "!" ) {
			++cnt;
		}
	}
	value = intToStr( cnt );
	return true;
}

bool BinaryOpNode::doAllNumSegments( const Jstr& mk, const Jstr &colType, const JagStrSplit &sp, Jstr &value )
{
	value = "0";
	if ( colType == JAG_C_COL_TYPE_LINE || colType == JAG_C_COL_TYPE_LINE3D ) {
		value = "1";
		return true;
	}

	if ( colType == JAG_C_COL_TYPE_TRIANGLE || colType == JAG_C_COL_TYPE_TRIANGLE3D ) {
		value = "3";
		return true;
	}

	if ( colType == JAG_C_COL_TYPE_SQUARE || colType == JAG_C_COL_TYPE_SQUARE3D ||
	      colType == JAG_C_COL_TYPE_RECTANGLE || colType == JAG_C_COL_TYPE_RECTANGLE3D) {
		value = "4";
		return true;
	}

	if ( JagParser::isVectorGeoType( colType ) ) {
		value = "0";
		return true;
	}

	if ( colType == JAG_C_COL_TYPE_LINESTRING || colType == JAG_C_COL_TYPE_LINESTRING3D
		|| colType == JAG_C_COL_TYPE_POLYGON || colType == JAG_C_COL_TYPE_POLYGON3D 
		|| colType == JAG_C_COL_TYPE_MULTIPOLYGON || colType == JAG_C_COL_TYPE_MULTIPOLYGON3D 
	    || colType == JAG_C_COL_TYPE_MULTILINESTRING || colType == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
	    int n = JagGeo::numberOfSegments( sp ); 
		value = intToStr( n );
		return true;
	}

	value = "0";
	return true;
}

bool BinaryOpNode::doAllNumLines( const Jstr& mk, const Jstr &colType, const JagStrSplit &sp, Jstr &value )
{
	value = "0";
	if ( colType == JAG_C_COL_TYPE_LINE || colType == JAG_C_COL_TYPE_LINE3D ) {
		value = "1";
		return true;
	}

	if ( colType == JAG_C_COL_TYPE_TRIANGLE || colType == JAG_C_COL_TYPE_TRIANGLE3D ) {
		value = "3";
		return true;
	}

	if ( colType == JAG_C_COL_TYPE_SQUARE || colType == JAG_C_COL_TYPE_SQUARE3D ||
	      colType == JAG_C_COL_TYPE_RECTANGLE || colType == JAG_C_COL_TYPE_RECTANGLE3D) {
		value = "4";
		return true;
	}

	if ( JagParser::isVectorGeoType( colType ) ) {
		value = "0";
		return true;
	}

	if ( colType == JAG_C_COL_TYPE_LINESTRING || colType == JAG_C_COL_TYPE_LINESTRING3D ) {
		value = "1";
		return true;
	}

	if ( colType == JAG_C_COL_TYPE_MULTIPOLYGON 
	    || colType == JAG_C_COL_TYPE_MULTIPOLYGON3D 
		|| colType == JAG_C_COL_TYPE_POLYGON
		|| colType == JAG_C_COL_TYPE_POLYGON3D ) {
		int cnt = 1;
		for ( int i =JAG_SP_START; i < sp.length(); ++i ) {
			if ( sp[i] == "|" || sp[i] == "!" ) ++cnt;
		}
		value = intToStr(cnt);
		return true;
	}

	if ( colType == JAG_C_COL_TYPE_MULTILINESTRING || colType == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
		int cnt = 1;
		for ( int i =JAG_SP_START; i < sp.length(); ++i ) {
			if ( sp[i] == "|" ) ++cnt;
		}
		value = intToStr(cnt);
		return true;
	}

	value = "0";
	return true;
}

bool BinaryOpNode::doAllNumRings( const Jstr& mk, const Jstr &colType, const JagStrSplit &sp, Jstr &value )
{
	if ( colType == JAG_C_COL_TYPE_MULTIPOLYGON 
	    || colType == JAG_C_COL_TYPE_MULTIPOLYGON3D 
		|| colType == JAG_C_COL_TYPE_POLYGON
		|| colType == JAG_C_COL_TYPE_POLYGON3D ) {
		int cnt = 1;
		for ( int i =JAG_SP_START; i < sp.length(); ++i ) {
			if ( sp[i] == "|" || sp[i] == "!" ) ++cnt;
		}
		value = intToStr(cnt);
		return true;
	}
	 
	value = "0";
	return true;
}

bool BinaryOpNode::doAllNumInnerRings( const Jstr& mk, const Jstr &colType, const JagStrSplit &sp, Jstr &value )
{
	if ( colType == JAG_C_COL_TYPE_MULTIPOLYGON 
	    || colType == JAG_C_COL_TYPE_MULTIPOLYGON3D 
		|| colType == JAG_C_COL_TYPE_POLYGON
		|| colType == JAG_C_COL_TYPE_POLYGON3D ) {
		int cnt = 0;
		for ( int i =JAG_SP_START; i < sp.length(); ++i ) {
			if ( sp[i] == "|" ) ++cnt;
		}
		value = intToStr(cnt);
		return true;
	}
	 
	value = "0";
	return true;
}

bool BinaryOpNode::doAllSummary( const Jstr& mk, const Jstr &colType, int srid, const JagStrSplit &sp, Jstr &value )
{
	Jstr npoints, nrings, isclosed;
	doAllNumPoints( mk, colType, sp, npoints );
	doAllNumRings( mk, colType, sp, nrings );
	doAllIsClosed( mk, colType, sp, isclosed );

	value = Jstr("geotype:") + getTypeStr(colType);
	value += Jstr(" srid:") + intToStr(srid);

    int dim = getPolyDimension( colType );
    if ( dim == 0 ) {
        dim = getDimension( colType );
    }
	value += Jstr(" dimension:") +  intToStr( dim );

	value += Jstr(" numpoints:") + npoints;
	value += Jstr(" numrings:") + nrings;
	value += Jstr(" isclosed:") + isclosed;
	return true;
}

bool BinaryOpNode::doAllVolume( const Jstr& mk1, const Jstr &colType1, int srid1, 
									 const JagStrSplit &sp1, double &value )
{
	if ( colType1 == JAG_C_COL_TYPE_POINT ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_POINT3D ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_CIRCLE ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_CIRCLE3D ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_SPHERE ) {
		value = JagGeo::doSphereVolume(  srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_SQUARE ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_SQUARE3D ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_CUBE ) {
		value = JagGeo::doCubeVolume(  srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_RECTANGLE ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_RECTANGLE3D ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_BOX ) {
		value = JagGeo::doBoxVolume(  srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_TRIANGLE ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_TRIANGLE3D ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_CYLINDER ) {
		value = JagGeo::doCylinderVolume(  srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_CONE ) {
		value = JagGeo::doConeVolume(  srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_ELLIPSE ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_ELLIPSOID ) {
		value = JagGeo::doEllipsoidVolume(  srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_LINE ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_LINE3D ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING3D ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON3D ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOINT ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOINT3D ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_MULTILINESTRING ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
		return false;
	} else { 
		return false;
	} 

	return false;
}

bool BinaryOpNode::doAllArea( const Jstr& mk1, const Jstr &colType1, int srid1, 
									 const JagStrSplit &sp1, double &value )
{
	if ( colType1 == JAG_C_COL_TYPE_POINT ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_POINT3D ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_CIRCLE ) {
		value = JagGeo::doCircleArea(  srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_CIRCLE3D ) {
		value = JagGeo::doCircle3DArea(  srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_SPHERE ) {
		value = JagGeo::doSphereArea(  srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_SQUARE ) {
		value = JagGeo::doSquareArea(  srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_SQUARE3D ) {
		value = JagGeo::doSquare3DArea(  srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_CUBE ) {
		value = JagGeo::doCubeArea(  srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_RECTANGLE ) {
		value = JagGeo::doRectangleArea(  srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_RECTANGLE3D ) {
		value = JagGeo::doRectangle3DArea(  srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_BOX ) {
		value = JagGeo::doBoxArea(  srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_TRIANGLE ) {
		value = JagGeo::doTriangleArea(  srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_TRIANGLE3D ) {
		value = JagGeo::doTriangle3DArea(  srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_CYLINDER ) {
		value = JagGeo::doCylinderArea(  srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_CONE ) {
		value = JagGeo::doConeArea(  srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_ELLIPSE ) {
		value = JagGeo::doEllipseArea(  srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_ELLIPSOID ) {
		value = JagGeo::doEllipsoidArea(  srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_LINE ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_LINE3D ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING3D ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON ) {
		value = JagGeo::doPolygonArea(  mk1, srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON3D ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOINT ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOINT3D ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_MULTILINESTRING ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		value = JagGeo::doMultiPolygonArea( mk1, srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
		return false;
	} else { 
		return false;
	} 

	return false;
}


bool BinaryOpNode::doAllPerimeter( const Jstr& mk1, const Jstr &colType1, int srid1, 
									 const JagStrSplit &sp1, double &value )
{
	if ( colType1 == JAG_C_COL_TYPE_POINT ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_POINT3D ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_CIRCLE ) {
		value = JagGeo::doCirclePerimeter(  srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_CIRCLE3D ) {
		value = JagGeo::doCircle3DPerimeter(  srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_SPHERE ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_SQUARE ) {
		value = JagGeo::doSquarePerimeter(  srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_SQUARE3D ) {
		value = JagGeo::doSquare3DPerimeter(  srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_CUBE ) {
		value = JagGeo::doCubePerimeter(  srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_RECTANGLE ) {
		value = JagGeo::doRectanglePerimeter(  srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_RECTANGLE3D ) {
		value = JagGeo::doRectangle3DPerimeter(  srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_BOX ) {
		value = JagGeo::doBoxPerimeter(  srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_TRIANGLE ) {
		value = JagGeo::doTrianglePerimeter(  srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_TRIANGLE3D ) {
		value = JagGeo::doTriangle3DPerimeter(  srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_CYLINDER ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_CONE ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_ELLIPSE ) {
		value = JagGeo::doEllipsePerimeter(  srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_ELLIPSOID ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_LINE ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_LINE3D ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING3D ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON ) {
		value = JagGeo::doPolygonPerimeter(  mk1, srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON3D ) {
		value = JagGeo::doPolygon3DPerimeter(  mk1, srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOINT ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOINT3D ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_MULTILINESTRING ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
		return false;
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		value = JagGeo::doMultiPolygonPerimeter( mk1, srid1, sp1 );
		return true;
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
		value = JagGeo::doMultiPolygon3DPerimeter( mk1, srid1, sp1 );
		return true;
	} else { 
		return false;
	} 

	return false;
}


bool BinaryOpNode::doAllMinMax( int op, int srid, const Jstr& mk, const Jstr &colType, const JagStrSplit &sp, double &value )
{
    // scan all points
    dn("s6841200 doAllMinMax ...");
	value = 0.0;
	Jstr val;

	doAllExtent( srid, mk, colType, sp, val );
    dn("s329003 doAllExtent done val=[%s]", val.s() );

	if ( val.size() < 1 ) return false;

	char *pend;
	char *pstart = secondTokenStartEnd( val.c_str(), pend, ' ');
	if ( ! pstart ) return false;

	Jstr bbstr(pstart, pend-pstart);

    dn("s348004 bbstr=[%s]", bbstr.s() );

	JagStrSplit ss(bbstr, ':');
	int nc = ss.size();
	double xmin, ymin, zmin, xmax, ymax, zmax;
	bool is3D = false;

	if ( 4 == nc ) {
		xmin = ss[0].tof();
		ymin = ss[1].tof();
		xmax = ss[2].tof();
		ymax = ss[3].tof();
	} else if ( 6 == nc ) {
		is3D = true;
		xmin = ss[0].tof();
		ymin = ss[1].tof();
		zmin = ss[2].tof();
		xmax = ss[3].tof();
		ymax = ss[4].tof();
		zmax = ss[5].tof();
	} else {
		return false;
	}

   	if ( ! is3D ) {
    	if (  op == JAG_FUNC_XMIN ) {
    		value = xmin;
    	} else if ( op == JAG_FUNC_YMIN ) {
    		value = ymin;
    	} else if ( op == JAG_FUNC_XMAX ) {
    		value = xmax;
    	} else if ( op == JAG_FUNC_YMAX ) {
    		value = ymax;
    	} 
   	} else {
    	if (  op == JAG_FUNC_XMIN ) {
    		value = xmin;
    	} else if ( op == JAG_FUNC_YMIN ) {
    		value = ymin;
    	} else if ( op == JAG_FUNC_ZMIN ) {
    		value = zmin;
    	} else if ( op == JAG_FUNC_XMAX ) {
    		value = xmax;
    	} else if ( op == JAG_FUNC_YMAX ) {
    		value = ymax;
    	} else if ( op == JAG_FUNC_ZMAX ) {
    		value = zmax;
    	} 
   	}

	return true;
}

Jstr BinaryOpNode::doAllUnion( Jstr mark1, Jstr colType1, 
										int srid1, JagStrSplit &sp1, Jstr mark2, 
										Jstr colType2, int srid2, JagStrSplit &sp2 )
{
	int dim1 = getDimension(colType1);
	int dim2 = getDimension(colType2);
	bool rc;
	if ( 2 == dim1 && 2 == dim2 ) {
		Jstr pgon;
		if ( JagParser::isVectorGeoType( colType1 ) ) {
			rc = doAllToPolygon( mark1, "", colType1, srid1, sp1, "30", pgon );
			if ( ! rc ) return "";
			sp1.init( pgon.c_str(), -1, ' ', true );
			JagStrSplit ss(sp1[0], 5, '=');
			mark1 = ss[0];
			colType1 = ss[3];
		}

		pgon = "";
		if ( JagParser::isVectorGeoType( colType2 ) ) {
			rc = doAllToPolygon( mark2, "", colType2, srid2, sp2, "30", pgon );
			if ( ! rc ) return "";
			sp2.init( pgon.c_str(), -1, ' ', true );
			JagStrSplit ss(sp2[0], 5, '=');
			mark2 = ss[0];
			colType2 = ss[3];
		}
	}

	if ( colType1 == JAG_C_COL_TYPE_POINT ) {
		return JagGeo::doPointAddition(  srid1, sp1, mark2, colType2, srid2, sp2 );
	} else if ( colType1 == JAG_C_COL_TYPE_POINT3D ) {
		return JagGeo::doPoint3DAddition(  srid1, sp1, mark2, colType2, srid2, sp2 );
	} else if ( colType1 == JAG_C_COL_TYPE_LINE ) {
		return JagGeo::doLineAddition(  srid1, sp1, mark2, colType2, srid2, sp2 );
	} else if ( colType1 == JAG_C_COL_TYPE_LINE3D ) {
		return JagGeo::doLine3DAddition(  srid1, sp1, mark2, colType2, srid2, sp2 );
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING ) {
		return JagGeo::doLineStringAddition( mark1, srid1, sp1, mark2, colType2, srid2, sp2 );
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING3D ) {
		return JagGeo::doLineString3DAddition( mark1, srid1, sp1, mark2, colType2, srid2, sp2 );
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON ) {
		return JagGeo::doPolygonUnion( mark1, srid1, sp1, mark2, colType2, srid2, sp2 );
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
		return JagGeo::doPolygonUnion( mark2, srid2, sp2, mark1, colType1, srid1, sp1 );
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON3D ) {
		return JagGeo::doPolygon3DAddition( mark1, srid1, sp1, mark2, colType2, srid2, sp2 );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTILINESTRING ) {
		return JagGeo::doPolygonAddition( mark1, srid1, sp1, mark2, colType2, srid2, sp2 );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
		return JagGeo::doPolygon3DAddition( mark1, srid1, sp1, mark2, colType2, srid2, sp2 );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		return JagGeo::doMultiPolygonAddition( mark1, srid1, sp1, mark2, colType2, srid2, sp2 );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
		return JagGeo::doMultiPolygon3DAddition( mark1, srid1, sp1, mark2, colType2, srid2, sp2 );
	} else {
		return "";
	}
}

Jstr BinaryOpNode::doAllCollect( Jstr mark1, Jstr colType1, 
								int srid1, JagStrSplit &sp1, Jstr mark2, 
								Jstr colType2, int srid2, JagStrSplit &sp2 )
{
	int dim1 = getDimension(colType1);
	int dim2 = getDimension(colType2);
	bool rc;
	if ( 2 == dim1 && 2 == dim2 ) {
		Jstr pgon;
		if ( JagParser::isVectorGeoType( colType1 ) ) {
			rc = doAllToPolygon( mark1, "", colType1, srid1, sp1, "30", pgon );
			if ( ! rc ) return "";
			sp1.init( pgon.c_str(), -1, ' ', true );
			JagStrSplit ss(sp1[0], 5, '=');
			mark1 = ss[0];
			colType1 = ss[3];
		}

		pgon = "";
		if ( JagParser::isVectorGeoType( colType2 ) ) {
			rc = doAllToPolygon( mark2, "", colType2, srid2, sp2, "30", pgon );
			if ( ! rc ) return "";
			sp2.init( pgon.c_str(), -1, ' ', true );
			JagStrSplit ss(sp2[0], 5, '=');
			mark2 = ss[0];
			colType2 = ss[3];
		}
	}

	if ( colType1 == JAG_C_COL_TYPE_POINT ) {
		return JagGeo::doPointAddition(  srid1, sp1, mark2, colType2, srid2, sp2 );
	} else if ( colType1 == JAG_C_COL_TYPE_POINT3D ) {
		return JagGeo::doPoint3DAddition(  srid1, sp1, mark2, colType2, srid2, sp2 );
	} else if ( colType1 == JAG_C_COL_TYPE_LINE ) {
		return JagGeo::doLineAddition(  srid1, sp1, mark2, colType2, srid2, sp2 );
	} else if ( colType1 == JAG_C_COL_TYPE_LINE3D ) {
		return JagGeo::doLine3DAddition(  srid1, sp1, mark2, colType2, srid2, sp2 );
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING ) {
		return JagGeo::doLineStringAddition( mark1, srid1, sp1, mark2, colType2, srid2, sp2 );
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING3D ) {
		return JagGeo::doLineString3DAddition( mark1, srid1, sp1, mark2, colType2, srid2, sp2 );
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON ) {
		return JagGeo::doPolygonAddition( mark1, srid1, sp1, mark2, colType2, srid2, sp2 );
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON3D ) {
		return JagGeo::doPolygon3DAddition( mark1, srid1, sp1, mark2, colType2, srid2, sp2 );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTILINESTRING ) {
		return JagGeo::doPolygonAddition( mark1, srid1, sp1, mark2, colType2, srid2, sp2 );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
		return JagGeo::doPolygon3DAddition( mark1, srid1, sp1, mark2, colType2, srid2, sp2 );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		return JagGeo::doMultiPolygonAddition( mark1, srid1, sp1, mark2, colType2, srid2, sp2 );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
		return JagGeo::doMultiPolygon3DAddition( mark1, srid1, sp1, mark2, colType2, srid2, sp2 );
	} else {
		return "";
	}
}

Jstr BinaryOpNode::doAllIntersection( int srid1, Jstr mark1, Jstr colType1, 
										JagStrSplit &sp1, Jstr mark2, 
										Jstr colType2, JagStrSplit &sp2 )
{
    dn("s300120 doAllIntersection ");
	int dim1 = getDimension(colType1);
	int dim2 = getDimension(colType2);
	bool rc;

	if ( 2 == dim1 && 2 == dim2 ) {
		Jstr pgon;
		if ( JagParser::isVectorGeoType( colType1 ) ) {
			rc = doAllToPolygon( mark1, "", colType1, srid1, sp1, "30", pgon );
			if ( ! rc ) return "";
			sp1.init( pgon.c_str(), -1, ' ', true );
			JagStrSplit ss(sp1[0], 5, '=');
			mark1 = ss[0];
			colType1 = ss[3];
		}

		pgon = "";
		if ( JagParser::isVectorGeoType( colType2 ) ) {
			rc = doAllToPolygon( mark2, "", colType2, srid1, sp2, "30", pgon );
			if ( ! rc ) return "";
			sp2.init( pgon.c_str(), -1, ' ', true );
			JagStrSplit ss(sp2[0], 5, '=');
			mark2 = ss[0];
			colType2 = ss[3];
		}
	}


	if ( colType1 == JAG_C_COL_TYPE_POINT || colType1 == JAG_C_COL_TYPE_POINT3D ) {
		return JagGeo::doPointIntersection(  colType1,sp1, colType2,sp2 );
	} else if ( colType2 == JAG_C_COL_TYPE_POINT || colType2 == JAG_C_COL_TYPE_POINT3D ) {
		return JagGeo::doPointIntersection(  colType2,sp2, colType1,sp1 );
	} else if ( colType1 == JAG_C_COL_TYPE_LINE || colType1 == JAG_C_COL_TYPE_LINE3D ) {
		return JagGeo::doLineIntersection( colType1,sp1, colType2,sp2 );
	} else if ( colType2 == JAG_C_COL_TYPE_LINE || colType2 == JAG_C_COL_TYPE_LINE3D ) {
		return JagGeo::doLineIntersection( colType2,sp2, colType1,sp1 );
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING || colType1 == JAG_C_COL_TYPE_LINESTRING3D ) {
		return JagGeo::doLineStringIntersection( colType1,sp1, colType2,sp2 );
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING || colType2 == JAG_C_COL_TYPE_LINESTRING3D ) {
		return JagGeo::doLineStringIntersection( colType2,sp2, colType1,sp1 );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTILINESTRING || colType1 == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
		return JagGeo::doMultiLineStringIntersection( colType1,sp1, colType2,sp2 );
	} else if ( colType2 == JAG_C_COL_TYPE_MULTILINESTRING || colType2 == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
		return JagGeo::doMultiLineStringIntersection( colType2,sp2, colType1,sp1 );
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON ) {
		return JagGeo::doPolygonIntersection( colType1,sp1, colType2,sp2 );
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
		return JagGeo::doPolygonIntersection( colType2,sp2, colType1,sp1 );
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON3D ) {
        dn("s3333088 colType1 == JAG_C_COL_TYPE_POLYGON3D ..");
		return JagGeo::doPolygon3DIntersection( colType1,sp1, colType2,sp2 );
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON3D ) {
        dn("s3333089 colType2 == JAG_C_COL_TYPE_POLYGON3D ..");
		return JagGeo::doPolygon3DIntersection( colType2,sp2, colType1,sp1 );
	} else {
		return "";
	}
}

Jstr BinaryOpNode::doAllDifference( int srid1, Jstr mark1, Jstr colType1, 
									JagStrSplit &sp1, Jstr mark2, 
									Jstr colType2, JagStrSplit &sp2 )
{
	int dim1 = getDimension(colType1);
	int dim2 = getDimension(colType2);
	bool rc;

	if ( 2 == dim1 && 2 == dim2 ) {
		Jstr pgon;
		if ( JagParser::isVectorGeoType( colType1 ) ) {
			rc = doAllToPolygon( mark1, "", colType1, srid1, sp1, "30", pgon );
			if ( ! rc ) return "";
			sp1.init( pgon.c_str(), -1, ' ', true );
			JagStrSplit ss(sp1[0], 5, '=');
			mark1 = ss[0];
			colType1 = ss[3];
		}

		pgon = "";
		if ( JagParser::isVectorGeoType( colType2 ) ) {
			rc = doAllToPolygon( mark2, "", colType2, srid1, sp2, "30", pgon );
			if ( ! rc ) return "";
			sp2.init( pgon.c_str(), -1, ' ', true );
			JagStrSplit ss(sp2[0], 5, '=');
			mark2 = ss[0];
			colType2 = ss[3];
		}
	}

	if ( colType1 == JAG_C_COL_TYPE_POINT || colType1 == JAG_C_COL_TYPE_POINT3D ) {
		return JagGeo::doPointDifference(  colType1,sp1, colType2,sp2 );
	} else if ( colType1 == JAG_C_COL_TYPE_LINE || colType1 == JAG_C_COL_TYPE_LINE3D ) {
		return JagGeo::doLineDifference( colType1,sp1, colType2,sp2 );
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING || colType1 == JAG_C_COL_TYPE_LINESTRING3D ) {
		return JagGeo::doLineStringDifference( colType1,sp1, colType2,sp2 );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTILINESTRING || colType1 == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
		return JagGeo::doMultiLineStringDifference( colType1,sp1, colType2,sp2 );
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON ) {
		return JagGeo::doPolygonDifference( colType1,sp1, colType2,sp2 );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		return JagGeo::doMultiPolygonDifference( colType1,sp1, colType2,sp2 );
	} else {
		return "";
	}
}

Jstr BinaryOpNode::doAllSymDifference( int srid1, Jstr mark1, Jstr colType1, 
										JagStrSplit &sp1, Jstr mark2, 
										Jstr colType2, JagStrSplit &sp2 )
{
	int dim1 = getDimension(colType1);
	int dim2 = getDimension(colType2);
	bool rc;
	if ( 2 == dim1 && 2 == dim2 ) {
		Jstr pgon;
		if ( JagParser::isVectorGeoType( colType1 ) ) {
			rc = doAllToPolygon( mark1, "", colType1, srid1, sp1, "30", pgon );
			if ( ! rc ) return "";
			sp1.init( pgon.c_str(), -1, ' ', true );
			JagStrSplit ss(sp1[0], 5, '=');
			mark1 = ss[0];
			colType1 = ss[3];
		}

		pgon = "";
		if ( JagParser::isVectorGeoType( colType2 ) ) {
			rc = doAllToPolygon( mark2, "", colType2, srid1, sp2, "30", pgon );
			if ( ! rc ) return "";
			sp2.init( pgon.c_str(), -1, ' ', true );
			JagStrSplit ss(sp2[0], 5, '=');
			mark2 = ss[0];
			colType2 = ss[3];
		}
	}

	if ( colType1 == JAG_C_COL_TYPE_POINT || colType1 == JAG_C_COL_TYPE_POINT3D ) {
		return JagGeo::doPointSymDifference(  colType1,sp1, colType2,sp2 );
	} else if ( colType2 == JAG_C_COL_TYPE_POINT || colType2 == JAG_C_COL_TYPE_POINT3D ) {
		return JagGeo::doPointSymDifference(  colType2,sp2, colType1,sp1 );
	} else if ( colType1 == JAG_C_COL_TYPE_LINE || colType1 == JAG_C_COL_TYPE_LINE3D ) {
		return JagGeo::doLineSymDifference( colType1,sp1, colType2,sp2 );
	} else if ( colType2 == JAG_C_COL_TYPE_LINE || colType2 == JAG_C_COL_TYPE_LINE3D ) {
		return JagGeo::doLineSymDifference( colType2,sp2, colType1,sp1 );
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING || colType1 == JAG_C_COL_TYPE_LINESTRING3D ) {
		return JagGeo::doLineStringSymDifference( colType1,sp1, colType2,sp2 );
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING || colType2 == JAG_C_COL_TYPE_LINESTRING3D ) {
		return JagGeo::doLineStringSymDifference( colType2,sp2, colType1,sp1 );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTILINESTRING || colType1 == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
		return JagGeo::doMultiLineStringSymDifference( colType1,sp1, colType2,sp2 );
	} else if ( colType2 == JAG_C_COL_TYPE_MULTILINESTRING || colType2 == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
		return JagGeo::doMultiLineStringSymDifference( colType2,sp2, colType1,sp1 );
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON ) {
		return JagGeo::doPolygonSymDifference( colType1,sp1, colType2,sp2 );
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
		return JagGeo::doPolygonSymDifference( colType2,sp2, colType1,sp1 );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		return JagGeo::doMultiPolygonSymDifference( colType1,sp1, colType2,sp2 );
	} else if ( colType2 == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		return JagGeo::doMultiPolygonSymDifference( colType2,sp2, colType1,sp1 );
	} else {
		return "";
	}
}

Jstr BinaryOpNode::doAllLocatePoint( int srid, const Jstr& mark1, const Jstr &colType1, 
										const JagStrSplit &sp1, const Jstr& mark2, 
										const Jstr &colType2, const JagStrSplit &sp2 )
{
	if ( colType1 == JAG_C_COL_TYPE_POINT || colType1 == JAG_C_COL_TYPE_POINT3D ) {
		return JagGeo::doLocatePoint(  srid, colType2,sp2, colType1,sp1 );
	} else if ( colType2 == JAG_C_COL_TYPE_POINT || colType2 == JAG_C_COL_TYPE_POINT3D ) {
		return JagGeo::doLocatePoint(  srid, colType1,sp1, colType2,sp2 );
	} else if ( colType1 == JAG_C_COL_TYPE_LINE || colType1 == JAG_C_COL_TYPE_LINE3D ) {
		return JagGeo::doLocatePoint( srid, colType1,sp1, colType2,sp2 );
	} else if ( colType2 == JAG_C_COL_TYPE_LINE || colType2 == JAG_C_COL_TYPE_LINE3D ) {
		return JagGeo::doLocatePoint( srid, colType2,sp2, colType1,sp1 );
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING || colType1 == JAG_C_COL_TYPE_LINESTRING3D ) {
		return JagGeo::doLocatePoint( srid, colType1,sp1, colType2,sp2 );
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING || colType2 == JAG_C_COL_TYPE_LINESTRING3D ) {
		return JagGeo::doLocatePoint( srid, colType2,sp2, colType1,sp1 );
	} else {
		return "";
	}
}

Jstr BinaryOpNode::doAllAddPoint( int srid, const Jstr& mark1, const Jstr &colType1, 
										const JagStrSplit &sp1, const Jstr& mark2, 
										const Jstr &colType2, const JagStrSplit &sp2, const Jstr &carg )
{
	int pos = jagatoi( carg.c_str() );
	if ( pos <= 0 ) pos = INT_MAX;

	Jstr val;
	if ( colType1 == JAG_C_COL_TYPE_LINESTRING || colType1 == JAG_C_COL_TYPE_LINESTRING3D ) {
		if ( colType2 == JAG_C_COL_TYPE_POINT ) {
			val = Jstr("CJAG=0=0=LS=d 0:0:0:0");
			int current=1;
			for ( int i=JAG_SP_START; i < sp1.size(); ++i ) {
				if ( current == pos ) {
					val += Jstr(" ") + sp2[JAG_SP_START+0] + ":" + sp2[JAG_SP_START+1];
				} 
				val += Jstr(" ") + sp1[i];
				++current;
			}
			if ( INT_MAX == pos ) {
				val += Jstr(" ") + sp2[JAG_SP_START+0] + ":" + sp2[JAG_SP_START+1];
			}
		} else if ( colType2 == JAG_C_COL_TYPE_POINT3D ) {
			val = Jstr("CJAG=0=0=LS3=d 0:0:0:0:0:0");
			int current=1;
			for ( int i=JAG_SP_START; i < sp1.size(); ++i ) {
				if ( current == pos ) {
					val += Jstr(" ") + sp2[JAG_SP_START+0] + ":" + sp2[JAG_SP_START+1] + ":" + sp2[JAG_SP_START+2];
				} 
				val += Jstr(" ") + sp1[i];
				++current;
			}
			if ( INT_MAX == pos ) {
				val += Jstr(" ") + sp2[JAG_SP_START+0] + ":" + sp2[JAG_SP_START+1] + ":" + sp2[JAG_SP_START+2];
			}
		}
	} 
	return val;
}

Jstr BinaryOpNode::doAllSetPoint( int srid, const Jstr& mark1, const Jstr &colType1, 
										const JagStrSplit &sp1, const Jstr& mark2, 
										const Jstr &colType2, const JagStrSplit &sp2, const Jstr &carg )
{
	if ( sp1.size() < 1 ) return "";
	int pos = jagatoi( carg.c_str() );
	if ( pos <= 0 ) pos = INT_MAX;

	Jstr val;
	if ( colType1 == JAG_C_COL_TYPE_LINESTRING || colType1 == JAG_C_COL_TYPE_LINESTRING3D ) {
		if ( colType2 == JAG_C_COL_TYPE_POINT ) {
			val = Jstr("CJAG=0=0=LS=d 0:0:0:0");
			int current=1;
			for ( int i=JAG_SP_START; i < sp1.size(); ++i ) {
				if ( current == pos ) {
					val += Jstr(" ") + sp2[JAG_SP_START+0] + ":" + sp2[JAG_SP_START+1];
				} else { 
					val += Jstr(" ") + sp1[i];
				}
				++current;
			}
		} else if ( colType2 == JAG_C_COL_TYPE_POINT3D ) {
			val = Jstr("CJAG=0=0=LS3=d 0:0:0:0:0:0");
			int current=1;
			for ( int i=JAG_SP_START; i < sp1.size(); ++i ) {
				if ( current == pos ) {
					val += Jstr(" ") + sp2[JAG_SP_START+0] + ":" + sp2[JAG_SP_START+1] + ":" + sp2[JAG_SP_START+2];
				} else {
					val += Jstr(" ") + sp1[i];
				}
				++current;
			}
		}
	} 
	return val;
}

bool BinaryOpNode::doAllRemovePoint( int srid, const Jstr &colType1, 
									const JagStrSplit &sp1, const Jstr &carg, Jstr &val )
{
	if ( sp1.size() < 1 ) return false;
	int pos = jagatoi( carg.c_str() );
	if ( pos <= 0 ) pos = INT_MAX;

	if ( colType1 == JAG_C_COL_TYPE_LINESTRING  ) {
		val = Jstr("CJAG=0=0=LS=d 0:0:0:0");
	} else {
		val = Jstr("CJAG=0=0=LS3=d 0:0:0:0:0:0");
	}

	int current=1;
	for ( int i=JAG_SP_START; i < sp1.size(); ++i ) {
		if ( current != pos ) {
			val += Jstr(" ") + sp1[i];
		}
		++current;
	}

	return true;
}

Jstr BinaryOpNode::doAllScaleAt( int srid, const Jstr& mark1, const Jstr &colType1, 
										const JagStrSplit &sp1, const Jstr& mark2, 
										const Jstr &colType2, const JagStrSplit &sp2, const Jstr &carg )
{
	if ( sp1.size() < 1 ) return false;
	double fx, fy, fz;
	fx = fy = fz = 1.0;
	JagStrSplit sf( carg, ':', true);
	if ( 3 == sf.size() ) {
		fx = fabs(sf[0].tof()); fy = fabs(sf[1].tof()); fz = fabs(sf[2].tof());
	} else if ( 2 == sf.size() ) {
		fx = fabs(sf[0].tof()); fy = fabs(sf[1].tof()); 
	} else {
		fx = fy = fz = fabs(carg.tof());
	}
	double px, py, pz, x, y, z;
	px = sp2[JAG_SP_START+0].tof();
	py = sp2[JAG_SP_START+1].tof();
	pz = sp2[JAG_SP_START+2].tof();
	Jstr val;

	if ( colType1 == JAG_C_COL_TYPE_POINT ) {
		double x,y;
		val = "CJAG=" + intToStr(srid) + "=0=PT=d 0:0:0:0 ";
		x = sp1[JAG_SP_START+0].tof(); y = sp1[JAG_SP_START+1].tof();
		x = px + fx*(x-px); y = py + fy*(y-py);
		val += d2s(x) + Jstr(" ") + d2s(y);
	} else if ( colType1 == JAG_C_COL_TYPE_POINT3D ) {
		double x,y,z;
		val = "CJAG=" + intToStr(srid) + "=0=PT=d 0:0:0:0 ";
		x = sp1[JAG_SP_START+0].tof(); y = sp1[JAG_SP_START+1].tof(); z = sp1[JAG_SP_START+2].tof();
		x = px + fx*(x-px); y = py + fy*(y-py); z = pz + fz*(z-pz);
		val += d2s(x) + Jstr(" ") + d2s(y) + " " + d2s(z);
	} else if ( colType1 == JAG_C_COL_TYPE_LINE  ) {
		val = "CJAG=" + intToStr(srid) + "=0=LN=d 0:0:0:0 ";
		x = sp1[JAG_SP_START+0].tof(); y = sp1[JAG_SP_START+1].tof();
		x = px + fx*(x-px); y = py + fy*(y-py);
		val += d2s(x) + Jstr(" ") + d2s(y);

		x = sp1[JAG_SP_START+2].tof(); y = sp1[JAG_SP_START+3].tof();
		x = px + fx*(x-px); y = py + fy*(y-py);
		val += d2s(x) + Jstr(" ") + d2s(y);
	} else if ( colType1 == JAG_C_COL_TYPE_LINE3D ) {
		val = "CJAG=" + intToStr(srid) + "=0=LN3=d 0:0:0:0:0:0 ";
		x = sp1[JAG_SP_START+0].tof(); y = sp1[JAG_SP_START+1].tof(); z = sp1[JAG_SP_START+2].tof();
		x = px + fx*(x-px); y = py + fy*(y-py); z = pz + fz*(z-pz);
		val += d2s(x) + Jstr(" ") + d2s(y) + " " + d2s(z);

		x = sp1[JAG_SP_START+3].tof(); y = sp1[JAG_SP_START+4].tof(); z = sp1[JAG_SP_START+5].tof();
		x = px + fx*(x-px); y = py + fy*(y-py); z = pz + fz*(z-pz);
		val += d2s(x) + Jstr(" ") + d2s(y) + " " + d2s(z);
	} else if ( colType1 == JAG_C_COL_TYPE_TRIANGLE  ) {
		val = "CJAG=" + intToStr(srid) + "=0=TR=d 0:0:0:0 ";
		x = sp1[JAG_SP_START+0].tof(); y = sp1[JAG_SP_START+1].tof();
		x = px + fx*(x-px); y = py + fy*(y-py);
		val += d2s(x) + Jstr(" ") + d2s(y);

		x = sp1[JAG_SP_START+2].tof(); y = sp1[JAG_SP_START+3].tof();
		x = px + fx*(x-px); y = py + fy*(y-py);
		val += Jstr(" ") + d2s(x) + Jstr(" ") + d2s(y);

		x = sp1[JAG_SP_START+4].tof(); y = sp1[JAG_SP_START+5].tof();
		x = px + fx*(x-px); y = py + fy*(y-py);
		val += Jstr(" ") + d2s(x) + Jstr(" ") + d2s(y);
	} else if ( colType1 == JAG_C_COL_TYPE_TRIANGLE3D  ) {
		val = "CJAG=" + intToStr(srid) + "=0=TR3=d 0:0:0:0:0:0 ";
		x = sp1[JAG_SP_START+0].tof(); y = sp1[JAG_SP_START+1].tof(); z = sp1[JAG_SP_START+2].tof();
		x = px + fx*(x-px); y = py + fy*(y-py); z = pz + fz*(z-pz);
		val += d2s(x) + Jstr(" ") + d2s(y) + " " + d2s(z);

		x = sp1[JAG_SP_START+3].tof(); y = sp1[JAG_SP_START+4].tof(); z = sp1[JAG_SP_START+5].tof();
		x = px + fx*(x-px); y = py + fy*(y-py); z = pz + fz*(z-pz);
		val += Jstr(" ") + d2s(x) + Jstr(" ") + d2s(y) + " " + d2s(z);

		x = sp1[JAG_SP_START+6].tof(); y = sp1[JAG_SP_START+7].tof(); z = sp1[JAG_SP_START+8].tof();
		x = px + fx*(x-px); y = py + fy*(y-py); z = pz + fz*(z-pz);
		val += Jstr(" ") + d2s(x) + Jstr(" ") + d2s(y) + " " + d2s(z);
	} else if ( colType1 == JAG_C_COL_TYPE_SQUARE ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() );
		double y = jagatof( sp1[JAG_SP_START+1].c_str() );
		x = px + fx*(x-px); y = py + fy*(y-py);
		double r = jagatof( sp1[JAG_SP_START+2].c_str() );
		double nx = JagGeo::safeget(sp1, JAG_SP_START+3);
		double a = r *fx;
		double b = r *fy;
		val = "CJAG=" + intToStr(srid) + "=0=RC=d 0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(a) + " " + d2s(b) + " " + d2s(nx);
	} else if ( colType1 == JAG_C_COL_TYPE_SQUARE3D ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() )*fx;
		double y = jagatof( sp1[JAG_SP_START+1].c_str() )*fy;
		double z = jagatof( sp1[JAG_SP_START+2].c_str() )*fz;
		double r = jagatof( sp1[JAG_SP_START+3].c_str() );
		double nx = JagGeo::safeget(sp1, JAG_SP_START+4);
		double ny = JagGeo::safeget(sp1, JAG_SP_START+5);
		x = px + fx*(x-px); y = py + fy*(y-py); z = pz + fz*(z-pz);
		double a = r *fx;
		double b = r *fy;
		val = "CJAG=" + intToStr(srid) + "=0=RC3=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + d2s(a) + " " + d2s(b) + " " + d2s(nx) + " " + d2s(ny);
	} else if ( colType1 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() );
		double y = jagatof( sp1[JAG_SP_START+1].c_str() );
		double a = jagatof( sp1[JAG_SP_START+2].c_str() )*fx;
		double b = jagatof( sp1[JAG_SP_START+3].c_str() )*fy;
		double nx = JagGeo::safeget(sp1, JAG_SP_START+4);
		x = px + fx*(x-px); y = py + fy*(y-py);
		val = "CJAG=" + intToStr(srid) + "=0=RC=d 0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(a) + " " + d2s(b) + " " + d2s(nx);
	} else if ( colType1 == JAG_C_COL_TYPE_RECTANGLE3D ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() );
		double y = jagatof( sp1[JAG_SP_START+1].c_str() );
		double z = jagatof( sp1[JAG_SP_START+2].c_str() );
		double a = jagatof( sp1[JAG_SP_START+3].c_str() )*fx;
		double b = jagatof( sp1[JAG_SP_START+4].c_str() )*fy;
		double nx = JagGeo::safeget(sp1, JAG_SP_START+5);
		double ny = JagGeo::safeget(sp1, JAG_SP_START+6);
		x = px + fx*(x-px); y = py + fy*(y-py); z = pz + fz*(z-pz);
		val = "CJAG=" + intToStr(srid) + "=0=RC3=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + d2s(a) + " " + d2s(b) + " " + d2s(nx) + " " + d2s(ny);
	} else if ( colType1 == JAG_C_COL_TYPE_CIRCLE ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() );
		double y = jagatof( sp1[JAG_SP_START+1].c_str() );
		double r = jagatof( sp1[JAG_SP_START+2].c_str() );
		double a = r *fx;
		double b = r *fy;
		x = px + fx*(x-px); y = py + fy*(y-py);
		val = "CJAG=" + intToStr(srid) + "=0=EL=d 0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(a) + " " + d2s(b) + " 0.0";
	} else if ( colType1 == JAG_C_COL_TYPE_CIRCLE3D ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() );
		double y = jagatof( sp1[JAG_SP_START+1].c_str() );
		double z = jagatof( sp1[JAG_SP_START+2].c_str() );
		double r = jagatof( sp1[JAG_SP_START+3].c_str() );
		double nx = JagGeo::safeget(sp1, JAG_SP_START+4);
		double ny = JagGeo::safeget(sp1, JAG_SP_START+5);
		double a = r *fx;
		double b = r *fy;
		x = px + fx*(x-px); y = py + fy*(y-py); z = pz + fz*(z-pz);
		val = "CJAG=" + intToStr(srid) + "=0=EL3=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + d2s(a) + " " + d2s(b) + " " + d2s(nx) + d2s(ny);
	} else if ( colType1 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() );
		double y = jagatof( sp1[JAG_SP_START+1].c_str() );
		double a = jagatof( sp1[JAG_SP_START+2].c_str() )*fx;
		double b = jagatof( sp1[JAG_SP_START+3].c_str() )*fy;
		double nx = JagGeo::safeget(sp1, JAG_SP_START+3);
		x = px + fx*(x-px); y = py + fy*(y-py);
		val = "CJAG=" + intToStr(srid) + "=0=EL=d 0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(a) + " " + d2s(b) + " " + d2s(nx);
	} else if ( colType1 == JAG_C_COL_TYPE_ELLIPSE3D ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() );
		double y = jagatof( sp1[JAG_SP_START+1].c_str() );
		double z = jagatof( sp1[JAG_SP_START+2].c_str() );
		double a = jagatof( sp1[JAG_SP_START+3].c_str() )*fx;
		double b = jagatof( sp1[JAG_SP_START+4].c_str() )*fy;
		double nx = JagGeo::safeget(sp1, JAG_SP_START+5);
		double ny = JagGeo::safeget(sp1, JAG_SP_START+6);
		x = px + fx*(x-px); y = py + fy*(y-py); z = pz + fz*(z-pz);
		val = "CJAG=" + intToStr(srid) + "=0=EL3=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + d2s(a) + " " + d2s(b) + " " + d2s(nx) + " " + d2s(ny);
	} else if ( colType1 == JAG_C_COL_TYPE_CUBE  ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() );
		double y = jagatof( sp1[JAG_SP_START+1].c_str() );
		double z = jagatof( sp1[JAG_SP_START+2].c_str() );
		double r = jagatof( sp1[JAG_SP_START+3].c_str() );
		double nx = jagatof( sp1[JAG_SP_START+4].c_str() );
		double ny = jagatof( sp1[JAG_SP_START+5].c_str() );
		double a = r *fx;
		double b = r *fy;
		double c = r *fz;
		x = px + fx*(x-px); y = py + fy*(y-py); z = pz + fz*(z-pz);
		val = "CJAG=" + intToStr(srid) + "=0=BX=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + d2s(a) + " " + d2s(b) + " " + d2s(c) 
				+ " " + d2s(nx) + " " + d2s(ny);
	} else if ( colType1 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() );
		double y = jagatof( sp1[JAG_SP_START+1].c_str() );
		double z = jagatof( sp1[JAG_SP_START+2].c_str() );
		double r = jagatof( sp1[JAG_SP_START+3].c_str() );
		double a = r *fx;
		double b = r *fy;
		double c = r *fz;
		x = px + fx*(x-px); y = py + fy*(y-py); z = pz + fz*(z-pz);
		val = "CJAG=" + intToStr(srid) + "=0=ES=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + d2s(a) + " " + d2s(b) + " " + d2s(c) + " 0.0 0.0";
	} else if ( colType1 == JAG_C_COL_TYPE_BOX || colType1 == JAG_C_COL_TYPE_ELLIPSOID  ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() );
		double y = jagatof( sp1[JAG_SP_START+1].c_str() );
		double z = jagatof( sp1[JAG_SP_START+2].c_str() );
		double a = jagatof( sp1[JAG_SP_START+3].c_str() )*fx;
		double b = jagatof( sp1[JAG_SP_START+4].c_str() )*fy;
		double c = jagatof( sp1[JAG_SP_START+5].c_str() )*fz;
		double nx = jagatof( sp1[JAG_SP_START+6].c_str() );
		double ny = jagatof( sp1[JAG_SP_START+7].c_str() );
		x = px + fx*(x-px); y = py + fy*(y-py); z = pz + fz*(z-pz);
		if ( colType1 == JAG_C_COL_TYPE_BOX ) {
			val = "CJAG=" + intToStr(srid) + "=0=BX=d 0:0:0:0:0:0 ";
		} else {
			val = "CJAG=" + intToStr(srid) + "=0=ES=d 0:0:0:0:0:0 ";
		}
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + d2s(a) + " " + d2s(b) + " " + d2s(c) 
				+ " " + d2s(nx) + " " + d2s(ny);
	} else if ( colType1 == JAG_C_COL_TYPE_CONE || colType1 == JAG_C_COL_TYPE_CYLINDER  ) {
		if ( jagEQ(fx,fy) && jagEQ(fx,fz) ) {
            double x = jagatof( sp1[JAG_SP_START+0].c_str() );
            double y = jagatof( sp1[JAG_SP_START+1].c_str() );
            double z = jagatof( sp1[JAG_SP_START+2].c_str() );
            double r = jagatof( sp1[JAG_SP_START+3].c_str() )*fx;
            double h = jagatof( sp1[JAG_SP_START+4].c_str() )*fx;
            double nx = JagGeo::safeget(sp1, JAG_SP_START+5);
            double ny = JagGeo::safeget(sp1, JAG_SP_START+6);
			if ( colType1 == JAG_C_COL_TYPE_CONE ) {
				val = "CJAG=" + intToStr(srid) + "=0=CN=d 0:0:0:0:0:0 ";
			} else {
				val = "CJAG=" + intToStr(srid) + "=0=CL=d 0:0:0:0:0:0 ";
			}
			x = px + fx*(x-px); y = py + fy*(y-py); z = pz + fz*(z-pz);
			val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + d2s(r) + " " + d2s(h) + 
				+ " " + d2s(nx) + " " + d2s(ny);
		}
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING ) {
		JagLineString3D linestr;
		JagParser::addLineStringData( linestr, sp1 );
		linestr.scaleat( px,py,pz, fx,fy, 1.0, false );
		linestr.toJAG( JAG_C_COL_TYPE_LINESTRING, false, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING3D ) {
		JagLineString3D linestr;
		JagParser::addLineString3DData( linestr, sp1 );
		linestr.scaleat(px,py,pz, fx,fy,fz, true );
		linestr.toJAG( JAG_C_COL_TYPE_LINESTRING3D, true, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOINT ) {
		JagLineString3D linestr;
		JagParser::addLineStringData( linestr, sp1 );
		linestr.scaleat(px,py,pz, fx,fy, 1.0, false );
		linestr.toJAG( JAG_C_COL_TYPE_MULTIPOINT, false, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOINT3D ) {
		JagLineString3D linestr;
		JagParser::addLineString3DData( linestr, sp1 );
		linestr.scaleat(px, py, pz, fx,fy, fz, true );
		linestr.toJAG( JAG_C_COL_TYPE_MULTIPOINT3D, true, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON ) {
		JagPolygon pgon;
		int rc = JagParser::addPolygonData( pgon, sp1, false );
		if ( rc < 0 ) return val;
		pgon.scaleat(px,py,pz, fx,fy, 1.0, false );
		pgon.toJAG( false, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON3D ) {
		JagPolygon pgon;
		int rc = JagParser::addPolygon3DData( pgon, sp1, false );
		if ( rc < 0 ) return val;
		pgon.scaleat(px,py,pz, fx,fy,fz, true );
		pgon.toJAG( true, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		JagVector<JagPolygon> pgvec;
        int n = JagParser::addMultiPolygonData( pgvec, sp1, false, false );
        if ( n <= 0 ) return val;
		for ( int i=0; i < pgvec.size(); ++i ) { pgvec[i].scaleat(px,py,pz, fx,fy,1.0, false ); }
		bool rc = JagGeo::toJAG( pgvec, false, true, sp1[1], srid, val );
		if ( ! rc ) return val; 
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
		JagVector<JagPolygon> pgvec;
        int n = JagParser::addMultiPolygonData( pgvec, sp1, false, true );
        if ( n <= 0 ) return val;
		for ( int i=0; i < pgvec.size(); ++i ) { pgvec[i].scaleat(px,py,pz, fx,fy,fz, true ); }
		bool rc = JagGeo::toJAG( pgvec, true, true, sp1[1], srid, val );
		if ( ! rc ) return val; 
	} else {
		return val;
	}

	return val;
}

bool BinaryOpNode::doAllReverse( int srid, const Jstr &colType1, const JagStrSplit &sp1, const Jstr &carg, Jstr &val )
{
	if ( sp1.size() < 1 ) return false;

	if ( colType1 == JAG_C_COL_TYPE_LINE  ) {
		val = sp1[0] + " " + sp1[1];
		val += Jstr(" ") + sp1[JAG_SP_START+2] + Jstr(" ") + sp1[JAG_SP_START+3] + Jstr(" ") 
			  + sp1[JAG_SP_START+0] + Jstr(" ") + sp1[JAG_SP_START+1];  
	} else if ( colType1 == JAG_C_COL_TYPE_LINE3D ) {
		val = sp1[0] + " " + sp1[1];
		val += Jstr(" ") + sp1[JAG_SP_START+3] + " " + sp1[JAG_SP_START+4] + " " + sp1[JAG_SP_START+5]
			  + sp1[JAG_SP_START+0] + Jstr(" ") + sp1[JAG_SP_START+1] + sp1[JAG_SP_START+2];  
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING ) {
		JagLineString linestr;
		JagParser::addLineStringData( linestr, sp1 );
		linestr.reverse();
		linestr.toJAG( JAG_C_COL_TYPE_LINESTRING, false, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING3D ) {
		JagLineString3D linestr;
		JagParser::addLineString3DData( linestr, sp1 );
		linestr.reverse();
		linestr.toJAG( JAG_C_COL_TYPE_LINESTRING3D, true, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON ) {
		JagPolygon pgon;
		int rc = JagParser::addPolygonData( pgon, sp1, false );
		if ( rc < 0 ) return false;
		pgon.reverse();
		pgon.toJAG( false, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON3D ) {
		JagPolygon pgon;
		int rc = JagParser::addPolygon3DData( pgon, sp1, false );
		if ( rc < 0 ) return false;
		pgon.reverse();
		pgon.toJAG( true, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		JagVector<JagPolygon> pgvec;
        int n = JagParser::addMultiPolygonData( pgvec, sp1, false, false );
        if ( n <= 0 ) return false;
		for ( int i=0; i < pgvec.size(); ++i ) { pgvec[i].reverse(); }
		bool rc = JagGeo::toJAG( pgvec, false, true, sp1[1], srid, val );
		if ( ! rc ) return false; 
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
		JagVector<JagPolygon> pgvec;
        int n = JagParser::addMultiPolygonData( pgvec, sp1, false, true );
        if ( n <= 0 ) return false;
		for ( int i=0; i < pgvec.size(); ++i ) { pgvec[i].reverse(); }
		bool rc = JagGeo::toJAG( pgvec, true, true, sp1[1], srid, val );
		if ( ! rc ) return false; 
	} else {
		return false;
	}

	return true;
}

bool BinaryOpNode::doAllScale( int srid, const Jstr &colType1, const JagStrSplit &sp1, const Jstr &carg, Jstr &val )
{
	if ( sp1.size() < 1 ) return false;
	double fx, fy, fz;
	fx = fy = fz = 1.0;
	JagStrSplit sf( carg, ':', true);
	if ( 3 == sf.size() ) {
		fx = fabs(sf[0].tof()); fy = fabs(sf[1].tof()); fz = fabs(sf[2].tof());
	} else if ( 2 == sf.size() ) {
		fx = fabs(sf[0].tof()); fy = fabs(sf[1].tof()); 
	} else {
		fx = fy = fz = fabs(carg.tof());
	}

	if ( colType1 == JAG_C_COL_TYPE_POINT ) {
		val = "CJAG=" + intToStr(srid) + "=0=PT=d 0:0:0:0 ";
		val += d2s(sp1[JAG_SP_START+0].tof()*fx) + Jstr(" ") + d2s(sp1[JAG_SP_START+1].tof()*fy);
	} else if ( colType1 == JAG_C_COL_TYPE_POINT3D ) {
		val = "CJAG=" + intToStr(srid) + "=0=PT=d 0:0:0:0 ";
		val += d2s(sp1[JAG_SP_START+0].tof()*fx) + Jstr(" ") + d2s(sp1[JAG_SP_START+1].tof()*fy)
			   + Jstr(" ") + d2s(sp1[JAG_SP_START+2].tof()*fz);
	} else if ( colType1 == JAG_C_COL_TYPE_LINE  ) {
		val = "CJAG=" + intToStr(srid) + "=0=LN=d 0:0:0:0 ";
		val += d2s(sp1[JAG_SP_START+0].tof()*fx) + Jstr(" ") + d2s(sp1[JAG_SP_START+1].tof()*fy) + Jstr(" ") 
			  + d2s(sp1[JAG_SP_START+2].tof()*fx) + Jstr(" ") + d2s(sp1[JAG_SP_START+3].tof()*fy);  
	} else if ( colType1 == JAG_C_COL_TYPE_LINE3D ) {
		val = "CJAG=" + intToStr(srid) + "=0=LN3=d 0:0:0:0:0:0 ";
		val += d2s(sp1[JAG_SP_START+0].tof()*fx) + " " + d2s(sp1[JAG_SP_START+1].tof()*fy) 
				+ " " + d2s(sp1[JAG_SP_START+2].tof()*fz) + d2s(sp1[JAG_SP_START+3].tof()*fx) + Jstr(" ") 
			  + d2s(sp1[JAG_SP_START+4].tof()*fy) + d2s(sp1[JAG_SP_START+5].tof()*fz);  
	} else if ( colType1 == JAG_C_COL_TYPE_TRIANGLE  ) {
		val = "CJAG=" + intToStr(srid) + "=0=TR=d 0:0:0:0 ";
		val += d2s(sp1[JAG_SP_START+0].tof()*fx) + Jstr(" ") + d2s(sp1[JAG_SP_START+1].tof()*fy) 
				+ Jstr(" ") + d2s(sp1[JAG_SP_START+2].tof()*fx) 
				+ Jstr(" ") + d2s(sp1[JAG_SP_START+3].tof()*fy)
				+ Jstr(" ") + d2s(sp1[JAG_SP_START+4].tof()*fx)
				+ Jstr(" ") + d2s(sp1[JAG_SP_START+5].tof()*fy);
	} else if ( colType1 == JAG_C_COL_TYPE_TRIANGLE3D  ) {
		val = "CJAG=" + intToStr(srid) + "=0=TR3=d 0:0:0:0:0:0 ";
		val += d2s(sp1[JAG_SP_START+0].tof()*fx) + Jstr(" ") + d2s(sp1[JAG_SP_START+1].tof()*fy) 
				+ Jstr(" ") + d2s(sp1[JAG_SP_START+2].tof()*fz) 
				+ Jstr(" ") + d2s(sp1[JAG_SP_START+3].tof()*fx) + Jstr(" ") + d2s(sp1[JAG_SP_START+4].tof()*fy)
				+ Jstr(" ") + d2s(sp1[JAG_SP_START+5].tof()*fz) + Jstr(" ") + d2s(sp1[JAG_SP_START+6].tof()*fx)
				+ Jstr(" ") + d2s(sp1[JAG_SP_START+7].tof()*fy) + Jstr(" ") + d2s(sp1[JAG_SP_START+8].tof()*fz);
	} else if ( colType1 == JAG_C_COL_TYPE_SQUARE ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() )*fx;
		double y = jagatof( sp1[JAG_SP_START+1].c_str() )*fy;
		double r = jagatof( sp1[JAG_SP_START+2].c_str() );
		double nx = JagGeo::safeget(sp1, JAG_SP_START+3);
		double a = r *fx;
		double b = r *fy;
		val = "CJAG=" + intToStr(srid) + "=0=RC=d 0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(a) + " " + d2s(b) + " " + d2s(nx);
	} else if ( colType1 == JAG_C_COL_TYPE_SQUARE3D ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() )*fx;
		double y = jagatof( sp1[JAG_SP_START+1].c_str() )*fy;
		double z = jagatof( sp1[JAG_SP_START+2].c_str() )*fz;
		double r = jagatof( sp1[JAG_SP_START+3].c_str() );
		double nx = JagGeo::safeget(sp1, JAG_SP_START+4);
		double ny = JagGeo::safeget(sp1, JAG_SP_START+5);
		double a = r *fx;
		double b = r *fy;
		val = "CJAG=" + intToStr(srid) + "=0=RC3=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + d2s(a) + " " + d2s(b) + " " + d2s(nx) + " " + d2s(ny);
	} else if ( colType1 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() )*fx;
		double y = jagatof( sp1[JAG_SP_START+1].c_str() )*fy;
		double a = jagatof( sp1[JAG_SP_START+2].c_str() )*fx;
		double b = jagatof( sp1[JAG_SP_START+3].c_str() )*fy;
		double nx = JagGeo::safeget(sp1, JAG_SP_START+4);
		val = "CJAG=" + intToStr(srid) + "=0=RC=d 0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(a) + " " + d2s(b) + " " + d2s(nx);
	} else if ( colType1 == JAG_C_COL_TYPE_RECTANGLE3D ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() )*fx;
		double y = jagatof( sp1[JAG_SP_START+1].c_str() )*fy;
		double z = jagatof( sp1[JAG_SP_START+2].c_str() )*fz;
		double a = jagatof( sp1[JAG_SP_START+3].c_str() )*fx;
		double b = jagatof( sp1[JAG_SP_START+4].c_str() )*fy;
		double nx = JagGeo::safeget(sp1, JAG_SP_START+5);
		double ny = JagGeo::safeget(sp1, JAG_SP_START+6);
		val = "CJAG=" + intToStr(srid) + "=0=RC3=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + d2s(a) + " " + d2s(b) + " " + d2s(nx) + " " + d2s(ny);
	} else if ( colType1 == JAG_C_COL_TYPE_CIRCLE ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() )*fx;
		double y = jagatof( sp1[JAG_SP_START+1].c_str() )*fy;
		double r = jagatof( sp1[JAG_SP_START+2].c_str() );
		double a = r *fx;
		double b = r *fy;
		val = "CJAG=" + intToStr(srid) + "=0=EL=d 0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(a) + " " + d2s(b) + " 0.0";
	} else if ( colType1 == JAG_C_COL_TYPE_CIRCLE3D ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() )*fx;
		double y = jagatof( sp1[JAG_SP_START+1].c_str() )*fy;
		double z = jagatof( sp1[JAG_SP_START+2].c_str() )*fz;
		double r = jagatof( sp1[JAG_SP_START+3].c_str() );
		double nx = JagGeo::safeget(sp1, JAG_SP_START+4);
		double ny = JagGeo::safeget(sp1, JAG_SP_START+5);
		double a = r *fx;
		double b = r *fy;
		val = "CJAG=" + intToStr(srid) + "=0=EL3=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + d2s(a) + " " + d2s(b) + " " + d2s(nx) + d2s(ny);
	} else if ( colType1 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() )*fx;
		double y = jagatof( sp1[JAG_SP_START+1].c_str() )*fy;
		double a = jagatof( sp1[JAG_SP_START+2].c_str() )*fx;
		double b = jagatof( sp1[JAG_SP_START+3].c_str() )*fy;
		double nx = JagGeo::safeget(sp1, JAG_SP_START+3);
		val = "CJAG=" + intToStr(srid) + "=0=EL=d 0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(a) + " " + d2s(b) + " " + d2s(nx);
	} else if ( colType1 == JAG_C_COL_TYPE_ELLIPSE3D ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() )*fx;
		double y = jagatof( sp1[JAG_SP_START+1].c_str() )*fy;
		double z = jagatof( sp1[JAG_SP_START+2].c_str() )*fz;
		double a = jagatof( sp1[JAG_SP_START+3].c_str() )*fx;
		double b = jagatof( sp1[JAG_SP_START+4].c_str() )*fy;
		double nx = JagGeo::safeget(sp1, JAG_SP_START+5);
		double ny = JagGeo::safeget(sp1, JAG_SP_START+6);
		val = "CJAG=" + intToStr(srid) + "=0=EL3=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + d2s(a) + " " + d2s(b) + " " + d2s(nx) + " " + d2s(ny);
	} else if ( colType1 == JAG_C_COL_TYPE_CUBE  ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() )*fx;
		double y = jagatof( sp1[JAG_SP_START+1].c_str() )*fy;
		double z = jagatof( sp1[JAG_SP_START+2].c_str() )*fz;
		double r = jagatof( sp1[JAG_SP_START+3].c_str() );
		double nx = jagatof( sp1[JAG_SP_START+4].c_str() );
		double ny = jagatof( sp1[JAG_SP_START+5].c_str() );
		double a = r *fx;
		double b = r *fy;
		double c = r *fz;
		val = "CJAG=" + intToStr(srid) + "=0=BX=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + d2s(a) + " " + d2s(b) + " " + d2s(c) 
				+ " " + d2s(nx) + " " + d2s(ny);
	} else if ( colType1 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() )*fx;
		double y = jagatof( sp1[JAG_SP_START+1].c_str() )*fy;
		double z = jagatof( sp1[JAG_SP_START+2].c_str() )*fz;
		double r = jagatof( sp1[JAG_SP_START+3].c_str() );
		double a = r *fx;
		double b = r *fy;
		double c = r *fz;
		val = "CJAG=" + intToStr(srid) + "=0=ES=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + d2s(a) + " " + d2s(b) + " " + d2s(c) + " 0.0 0.0";
	} else if ( colType1 == JAG_C_COL_TYPE_BOX || colType1 == JAG_C_COL_TYPE_ELLIPSOID  ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() )*fx;
		double y = jagatof( sp1[JAG_SP_START+1].c_str() )*fy;
		double z = jagatof( sp1[JAG_SP_START+2].c_str() )*fz;
		double a = jagatof( sp1[JAG_SP_START+3].c_str() )*fx;
		double b = jagatof( sp1[JAG_SP_START+4].c_str() )*fy;
		double c = jagatof( sp1[JAG_SP_START+5].c_str() )*fz;
		double nx = jagatof( sp1[JAG_SP_START+6].c_str() );
		double ny = jagatof( sp1[JAG_SP_START+7].c_str() );
		if ( colType1 == JAG_C_COL_TYPE_BOX ) {
			val = "CJAG=" + intToStr(srid) + "=0=BX=d 0:0:0:0:0:0 ";
		} else {
			val = "CJAG=" + intToStr(srid) + "=0=ES=d 0:0:0:0:0:0 ";
		}
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + d2s(a) + " " + d2s(b) + " " + d2s(c) 
				+ " " + d2s(nx) + " " + d2s(ny);
	} else if ( colType1 == JAG_C_COL_TYPE_CONE || colType1 == JAG_C_COL_TYPE_CYLINDER  ) {
		if ( jagEQ(fx,fy) && jagEQ(fx,fz) ) {
            double x = jagatof( sp1[JAG_SP_START+0].c_str() )*fx;
            double y = jagatof( sp1[JAG_SP_START+1].c_str() )*fx;
            double z = jagatof( sp1[JAG_SP_START+2].c_str() )*fx;
            double r = jagatof( sp1[JAG_SP_START+3].c_str() )*fx;
            double h = jagatof( sp1[JAG_SP_START+4].c_str() )*fx;
            double nx = JagGeo::safeget(sp1, JAG_SP_START+5);
            double ny = JagGeo::safeget(sp1, JAG_SP_START+6);
			if ( colType1 == JAG_C_COL_TYPE_CONE ) {
				val = "CJAG=" + intToStr(srid) + "=0=CN=d 0:0:0:0:0:0 ";
			} else {
				val = "CJAG=" + intToStr(srid) + "=0=CL=d 0:0:0:0:0:0 ";
			}
			val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + d2s(r) + " " + d2s(h) + 
				+ " " + d2s(nx) + " " + d2s(ny);
		}
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING ) {
		JagLineString3D linestr;
		JagParser::addLineStringData( linestr, sp1 );
		linestr.scale( fx,fy, 1.0, false );
		linestr.toJAG( JAG_C_COL_TYPE_LINESTRING, false, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING3D ) {
		JagLineString3D linestr;
		JagParser::addLineString3DData( linestr, sp1 );
		linestr.scale( fx,fy,fz, true );
		linestr.toJAG( JAG_C_COL_TYPE_LINESTRING3D, true, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOINT ) {
		JagLineString3D linestr;
		JagParser::addLineStringData( linestr, sp1 );
		linestr.scale( fx,fy, 1.0, false );
		linestr.toJAG( JAG_C_COL_TYPE_MULTIPOINT, false, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOINT3D ) {
		JagLineString3D linestr;
		JagParser::addLineString3DData( linestr, sp1 );
		linestr.scale( fx,fy, fz, true );
		linestr.toJAG( JAG_C_COL_TYPE_MULTIPOINT3D, true, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON ) {
		JagPolygon pgon;
		int rc = JagParser::addPolygonData( pgon, sp1, false );
		if ( rc < 0 ) return false;
		pgon.scale( fx,fy, 1.0, false );
		pgon.toJAG( false, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON3D ) {
		JagPolygon pgon;
		int rc = JagParser::addPolygon3DData( pgon, sp1, false );
		if ( rc < 0 ) return false;
		pgon.scale( fx,fy,fz, true );
		pgon.toJAG( true, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		JagVector<JagPolygon> pgvec;
        int n = JagParser::addMultiPolygonData( pgvec, sp1, false, false );
        if ( n <= 0 ) return false;
		for ( int i=0; i < pgvec.size(); ++i ) { pgvec[i].scale( fx,fy,1.0, false ); }
		bool rc = JagGeo::toJAG( pgvec, false, true, sp1[1], srid, val );
		if ( ! rc ) return false; 
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
		JagVector<JagPolygon> pgvec;
        int n = JagParser::addMultiPolygonData( pgvec, sp1, false, true );
        if ( n <= 0 ) return false;
		for ( int i=0; i < pgvec.size(); ++i ) { pgvec[i].scale( fx,fy,fz, true ); }
		bool rc = JagGeo::toJAG( pgvec, true, true, sp1[1], srid, val );
		if ( ! rc ) return false; 
	} else {
		return false;
	}

	return true;
}


bool BinaryOpNode::doAllScaleSize( int srid, const Jstr &colType1, const JagStrSplit &sp1, const Jstr &carg, Jstr &val )
{
	if ( sp1.size() < 1 ) return false;
	double fx, fy, fz;
	fx = fy = fz = 1.0;
	JagStrSplit sf( carg, ':', true);
	if ( 3 == sf.size() ) {
		fx = fabs(sf[0].tof()); fy = fabs(sf[1].tof()); fz = fabs(sf[2].tof());
	} else if ( 2 == sf.size() ) {
		fx = fabs(sf[0].tof()); fy = fabs(sf[1].tof()); 
	} else {
		fx = fy = fz = fabs(carg.tof());
	}

	if ( colType1 == JAG_C_COL_TYPE_POINT  ) {
		val = sp1[0] + " " + sp1[1] + " " +  sp1[JAG_SP_START+0] + Jstr(" ") + sp1[JAG_SP_START+1];
	} else if ( colType1 == JAG_C_COL_TYPE_POINT3D  ) {
		val = sp1[0] + " " + sp1[1] + " " +  sp1[JAG_SP_START+0] + Jstr(" ") + sp1[JAG_SP_START+1] 
		      + " " + sp1[JAG_SP_START+2];
	} else if ( colType1 == JAG_C_COL_TYPE_LINE  ) {
		val = "CJAG=" + intToStr(srid) + "=0=LN=d 0:0:0:0 ";
		val += d2s(sp1[JAG_SP_START+0].tof()*fx) + Jstr(" ") + d2s(sp1[JAG_SP_START+1].tof()*fy) + Jstr(" ") 
			  + d2s(sp1[JAG_SP_START+2].tof()*fx) + Jstr(" ") + d2s(sp1[JAG_SP_START+3].tof()*fy);  
	} else if ( colType1 == JAG_C_COL_TYPE_LINE3D ) {
		val = "CJAG=" + intToStr(srid) + "=0=LN3=d 0:0:0:0:0:0 ";
		val += d2s(sp1[JAG_SP_START+0].tof()*fx) + " " + d2s(sp1[JAG_SP_START+1].tof()*fy) 
				+ " " + d2s(sp1[JAG_SP_START+2].tof()*fz) + d2s(sp1[JAG_SP_START+3].tof()*fx) + Jstr(" ") 
			  + d2s(sp1[JAG_SP_START+4].tof()*fy) + d2s(sp1[JAG_SP_START+5].tof()*fz);  
	} else if ( colType1 == JAG_C_COL_TYPE_TRIANGLE  ) {
		val = "CJAG=" + intToStr(srid) + "=0=TR=d 0:0:0:0 ";
		val += d2s(sp1[JAG_SP_START+0].tof()*fx) + Jstr(" ") + d2s(sp1[JAG_SP_START+1].tof()*fy) 
				+ Jstr(" ") + d2s(sp1[JAG_SP_START+2].tof()*fx) 
				+ Jstr(" ") + d2s(sp1[JAG_SP_START+3].tof()*fy)
				+ Jstr(" ") + d2s(sp1[JAG_SP_START+4].tof()*fx)
				+ Jstr(" ") + d2s(sp1[JAG_SP_START+5].tof()*fy);
	} else if ( colType1 == JAG_C_COL_TYPE_TRIANGLE3D  ) {
		val = "CJAG=" + intToStr(srid) + "=0=TR3=d 0:0:0:0:0:0 ";
		val += d2s(sp1[JAG_SP_START+0].tof()*fx) + Jstr(" ") + d2s(sp1[JAG_SP_START+1].tof()*fy) 
				+ Jstr(" ") + d2s(sp1[JAG_SP_START+2].tof()*fz) 
				+ Jstr(" ") + d2s(sp1[JAG_SP_START+3].tof()*fx) + Jstr(" ") + d2s(sp1[JAG_SP_START+4].tof()*fy)
				+ Jstr(" ") + d2s(sp1[JAG_SP_START+5].tof()*fz) + Jstr(" ") + d2s(sp1[JAG_SP_START+6].tof()*fx)
				+ Jstr(" ") + d2s(sp1[JAG_SP_START+7].tof()*fy) + Jstr(" ") + d2s(sp1[JAG_SP_START+8].tof()*fz);
	} else if ( colType1 == JAG_C_COL_TYPE_SQUARE ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() );
		double y = jagatof( sp1[JAG_SP_START+1].c_str() );
		double r = jagatof( sp1[JAG_SP_START+2].c_str() );
		double nx = JagGeo::safeget(sp1, JAG_SP_START+3);
		double a = r *fx;
		double b = r *fy;
		val = "CJAG=" + intToStr(srid) + "=0=RC=d 0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(a) + " " + d2s(b) + " " + d2s(nx);
	} else if ( colType1 == JAG_C_COL_TYPE_SQUARE3D ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() );
		double y = jagatof( sp1[JAG_SP_START+1].c_str() );
		double z = jagatof( sp1[JAG_SP_START+2].c_str() );
		double r = jagatof( sp1[JAG_SP_START+3].c_str() );
		double nx = JagGeo::safeget(sp1, JAG_SP_START+4);
		double ny = JagGeo::safeget(sp1, JAG_SP_START+5);
		double a = r *fx;
		double b = r *fy;
		val = "CJAG=" + intToStr(srid) + "=0=RC3=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + d2s(a) + " " + d2s(b) + " " + d2s(nx) + " " + d2s(ny);
	} else if ( colType1 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() );
		double y = jagatof( sp1[JAG_SP_START+1].c_str() );
		double a = jagatof( sp1[JAG_SP_START+2].c_str() )*fx;
		double b = jagatof( sp1[JAG_SP_START+3].c_str() )*fy;
		double nx = JagGeo::safeget(sp1, JAG_SP_START+4);
		val = "CJAG=" + intToStr(srid) + "=0=RC=d 0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(a) + " " + d2s(b) + " " + d2s(nx);
	} else if ( colType1 == JAG_C_COL_TYPE_RECTANGLE3D ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() );
		double y = jagatof( sp1[JAG_SP_START+1].c_str() );
		double z = jagatof( sp1[JAG_SP_START+2].c_str() );
		double a = jagatof( sp1[JAG_SP_START+3].c_str() )*fx;
		double b = jagatof( sp1[JAG_SP_START+4].c_str() )*fy;
		double nx = JagGeo::safeget(sp1, JAG_SP_START+5);
		double ny = JagGeo::safeget(sp1, JAG_SP_START+6);
		val = "CJAG=" + intToStr(srid) + "=0=RC3=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + d2s(a) + " " + d2s(b) + " " + d2s(nx) + " " + d2s(ny);
	} else if ( colType1 == JAG_C_COL_TYPE_CIRCLE ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() );
		double y = jagatof( sp1[JAG_SP_START+1].c_str() );
		double r = jagatof( sp1[JAG_SP_START+2].c_str() );
		double a = r *fx;
		double b = r *fy;
		val = "CJAG=" + intToStr(srid) + "=0=EL=d 0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(a) + " " + d2s(b) + " 0.0";
	} else if ( colType1 == JAG_C_COL_TYPE_CIRCLE3D ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() );
		double y = jagatof( sp1[JAG_SP_START+1].c_str() );
		double z = jagatof( sp1[JAG_SP_START+2].c_str() );
		double r = jagatof( sp1[JAG_SP_START+3].c_str() );
		double nx = JagGeo::safeget(sp1, JAG_SP_START+4);
		double ny = JagGeo::safeget(sp1, JAG_SP_START+5);
		double a = r *fx;
		double b = r *fy;
		val = "CJAG=" + intToStr(srid) + "=0=EL3=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + d2s(a) + " " + d2s(b) + " " + d2s(nx) + d2s(ny);
	} else if ( colType1 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() );
		double y = jagatof( sp1[JAG_SP_START+1].c_str() );
		double a = jagatof( sp1[JAG_SP_START+2].c_str() )*fx;
		double b = jagatof( sp1[JAG_SP_START+3].c_str() )*fy;
		double nx = JagGeo::safeget(sp1, JAG_SP_START+3);
		val = "CJAG=" + intToStr(srid) + "=0=EL=d 0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(a) + " " + d2s(b) + " " + d2s(nx);
	} else if ( colType1 == JAG_C_COL_TYPE_ELLIPSE3D ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() );
		double y = jagatof( sp1[JAG_SP_START+1].c_str() );
		double z = jagatof( sp1[JAG_SP_START+2].c_str() );
		double a = jagatof( sp1[JAG_SP_START+3].c_str() )*fx;
		double b = jagatof( sp1[JAG_SP_START+4].c_str() )*fy;
		double nx = JagGeo::safeget(sp1, JAG_SP_START+5);
		double ny = JagGeo::safeget(sp1, JAG_SP_START+6);
		val = "CJAG=" + intToStr(srid) + "=0=EL3=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + d2s(a) + " " + d2s(b) + " " + d2s(nx) + " " + d2s(ny);
	} else if ( colType1 == JAG_C_COL_TYPE_CUBE  ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() );
		double y = jagatof( sp1[JAG_SP_START+1].c_str() );
		double z = jagatof( sp1[JAG_SP_START+2].c_str() );
		double r = jagatof( sp1[JAG_SP_START+3].c_str() );
		double nx = jagatof( sp1[JAG_SP_START+4].c_str() );
		double ny = jagatof( sp1[JAG_SP_START+5].c_str() );
		double a = r *fx;
		double b = r *fy;
		double c = r *fz;
		val = "CJAG=" + intToStr(srid) + "=0=BX=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + d2s(a) + " " + d2s(b) + " " + d2s(c) 
				+ " " + d2s(nx) + " " + d2s(ny);
	} else if ( colType1 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() );
		double y = jagatof( sp1[JAG_SP_START+1].c_str() );
		double z = jagatof( sp1[JAG_SP_START+2].c_str() );
		double r = jagatof( sp1[JAG_SP_START+3].c_str() );
		double a = r *fx;
		double b = r *fy;
		double c = r *fz;
		val = "CJAG=" + intToStr(srid) + "=0=ES=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + d2s(a) + " " + d2s(b) + " " + d2s(c) + " 0.0 0.0";
	} else if ( colType1 == JAG_C_COL_TYPE_BOX || colType1 == JAG_C_COL_TYPE_ELLIPSOID  ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() );
		double y = jagatof( sp1[JAG_SP_START+1].c_str() );
		double z = jagatof( sp1[JAG_SP_START+2].c_str() );
		double a = jagatof( sp1[JAG_SP_START+3].c_str() )*fx;
		double b = jagatof( sp1[JAG_SP_START+4].c_str() )*fy;
		double c = jagatof( sp1[JAG_SP_START+5].c_str() )*fz;
		double nx = jagatof( sp1[JAG_SP_START+6].c_str() );
		double ny = jagatof( sp1[JAG_SP_START+7].c_str() );
		if ( colType1 == JAG_C_COL_TYPE_BOX ) {
			val = "CJAG=" + intToStr(srid) + "=0=BX=d 0:0:0:0:0:0 ";
		} else {
			val = "CJAG=" + intToStr(srid) + "=0=ES=d 0:0:0:0:0:0 ";
		}
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + d2s(a) + " " + d2s(b) + " " + d2s(c) 
				+ " " + d2s(nx) + " " + d2s(ny);
	} else if ( colType1 == JAG_C_COL_TYPE_CONE || colType1 == JAG_C_COL_TYPE_CYLINDER  ) {
		if ( jagEQ(fx,fy) && jagEQ(fx,fz) ) {
            double x = jagatof( sp1[JAG_SP_START+0].c_str() );
            double y = jagatof( sp1[JAG_SP_START+1].c_str() );
            double z = jagatof( sp1[JAG_SP_START+2].c_str() );
            double r = jagatof( sp1[JAG_SP_START+3].c_str() )*fx;
            double h = jagatof( sp1[JAG_SP_START+4].c_str() )*fx;
            double nx = JagGeo::safeget(sp1, JAG_SP_START+5);
            double ny = JagGeo::safeget(sp1, JAG_SP_START+6);
			if ( colType1 == JAG_C_COL_TYPE_CONE ) {
				val = "CJAG=" + intToStr(srid) + "=0=CN=d 0:0:0:0:0:0 ";
			} else {
				val = "CJAG=" + intToStr(srid) + "=0=CL=d 0:0:0:0:0:0 ";
			}
			val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + d2s(r) + " " + d2s(h) + 
				+ " " + d2s(nx) + " " + d2s(ny);
		}
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING ) {
		JagLineString3D linestr;
		JagParser::addLineStringData( linestr, sp1 );
		linestr.scale( fx,fy, 1.0, false );
		linestr.toJAG( JAG_C_COL_TYPE_LINESTRING, false, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING3D ) {
		JagLineString3D linestr;
		JagParser::addLineString3DData( linestr, sp1 );
		linestr.scale( fx,fy,fz, true );
		linestr.toJAG( JAG_C_COL_TYPE_LINESTRING3D, true, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOINT ) {
		JagLineString3D linestr;
		JagParser::addLineStringData( linestr, sp1 );
		linestr.scale( fx,fy, 1.0, false );
		linestr.toJAG( JAG_C_COL_TYPE_MULTIPOINT, false, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOINT3D ) {
		JagLineString3D linestr;
		JagParser::addLineString3DData( linestr, sp1 );
		linestr.scale( fx,fy, fz, true );
		linestr.toJAG( JAG_C_COL_TYPE_MULTIPOINT3D, true, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON ) {
		JagPolygon pgon;
		int rc = JagParser::addPolygonData( pgon, sp1, false );
		if ( rc < 0 ) return false;
		pgon.scale( fx,fy, 1.0, false );
		pgon.toJAG( false, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON3D ) {
		JagPolygon pgon;
		int rc = JagParser::addPolygon3DData( pgon, sp1, false );
		if ( rc < 0 ) return false;
		pgon.scale( fx,fy,fz, true );
		pgon.toJAG( true, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		JagVector<JagPolygon> pgvec;
        int n = JagParser::addMultiPolygonData( pgvec, sp1, false, false );
        if ( n <= 0 ) return false;
		for ( int i=0; i < pgvec.size(); ++i ) { pgvec[i].scale( fx,fy,1.0, false ); }
		bool rc = JagGeo::toJAG( pgvec, false, true, sp1[1], srid, val );
		if ( ! rc ) return false; 
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
		JagVector<JagPolygon> pgvec;
        int n = JagParser::addMultiPolygonData( pgvec, sp1, false, true );
        if ( n <= 0 ) return false;
		for ( int i=0; i < pgvec.size(); ++i ) { pgvec[i].scale( fx,fy,fz, true ); }
		bool rc = JagGeo::toJAG( pgvec, true, true, sp1[1], srid, val );
		if ( ! rc ) return false; 
	} else {
		return false;
	}

	return true;
}

bool BinaryOpNode::doAllTranslate( int srid, const Jstr &colType1, const JagStrSplit &sp1, const Jstr &carg, Jstr &val )
{
	if ( sp1.size() < 1 ) return false;
	double dx, dy, dz;
	dx = dy = dz = 0.0;
	JagStrSplit sf( carg, ':', true);
	if ( 3 == sf.size() ) {
		dx = fabs(sf[0].tof()); dy = fabs(sf[1].tof()); dz = fabs(sf[2].tof());
	} else if ( 2 == sf.size() ) {
		dx = fabs(sf[0].tof()); dy = fabs(sf[1].tof()); 
	} 
	double x, y, z;

	if ( colType1 == JAG_C_COL_TYPE_POINT  ) {
		val = "CJAG=" + intToStr(srid) + "=0=PT=d 0:0:0:0";
		x = sp1[JAG_SP_START+0].tof() + dx;
		y = sp1[JAG_SP_START+1].tof() + dy;
		val += Jstr(" ") + d2s(x) + Jstr(" ") + d2s(y);
	} else if ( colType1 == JAG_C_COL_TYPE_POINT3D  ) {
		val = "CJAG=" + intToStr(srid) + "=0=PT3=d 0:0:0:0:0:0";
		x = sp1[JAG_SP_START+0].tof() + dx;
		y = sp1[JAG_SP_START+1].tof() + dy;
		z = sp1[JAG_SP_START+2].tof() + dz;
		val += Jstr(" ") + d2s(x) + Jstr(" ") + d2s(y) + " " + d2s(z);
	} else if ( colType1 == JAG_C_COL_TYPE_LINE  ) {
		val = "CJAG=" + intToStr(srid) + "=0=LN=d 0:0:0:0 ";
		val += d2s(sp1[JAG_SP_START+0].tof()+dx) + Jstr(" ") + d2s(sp1[JAG_SP_START+1].tof()+dy) + Jstr(" ") 
			  + d2s(sp1[JAG_SP_START+2].tof()+dx) + Jstr(" ") + d2s(sp1[JAG_SP_START+3].tof()+dy);  
	} else if ( colType1 == JAG_C_COL_TYPE_LINE3D ) {
		val = "CJAG=" + intToStr(srid) + "=0=LN3=d 0:0:0:0:0:0 ";
		val += d2s(sp1[JAG_SP_START+0].tof()+dx) + " " + d2s(sp1[JAG_SP_START+1].tof()+dy) 
				+ " " + d2s(sp1[JAG_SP_START+2].tof()+dz) + d2s(sp1[JAG_SP_START+3].tof()+dx) + Jstr(" ") 
			  + d2s(sp1[JAG_SP_START+4].tof()+dy) + d2s(sp1[JAG_SP_START+5].tof()+dz);  
	} else if ( colType1 == JAG_C_COL_TYPE_TRIANGLE  ) {
		val = "CJAG=" + intToStr(srid) + "=0=TR=d 0:0:0:0 ";
		val += d2s(sp1[JAG_SP_START+0].tof()+dx) + Jstr(" ") + d2s(sp1[JAG_SP_START+1].tof()+dy) 
				+ Jstr(" ") + d2s(sp1[JAG_SP_START+2].tof()+dx) 
				+ Jstr(" ") + d2s(sp1[JAG_SP_START+3].tof()+dy)
				+ Jstr(" ") + d2s(sp1[JAG_SP_START+4].tof()+dx)
				+ Jstr(" ") + d2s(sp1[JAG_SP_START+5].tof()+dy);
	} else if ( colType1 == JAG_C_COL_TYPE_TRIANGLE3D  ) {
		val = "CJAG=" + intToStr(srid) + "=0=TR3=d 0:0:0:0:0:0 ";
		val += d2s(sp1[JAG_SP_START+0].tof()+dx) + Jstr(" ") + d2s(sp1[JAG_SP_START+1].tof()+dy) 
				+ Jstr(" ") + d2s(sp1[JAG_SP_START+2].tof()+dz) 
				+ Jstr(" ") + d2s(sp1[JAG_SP_START+3].tof()+dx) + Jstr(" ") + d2s(sp1[JAG_SP_START+4].tof()+dy)
				+ Jstr(" ") + d2s(sp1[JAG_SP_START+5].tof()+dz) + Jstr(" ") + d2s(sp1[JAG_SP_START+6].tof()+dx)
				+ Jstr(" ") + d2s(sp1[JAG_SP_START+7].tof()+dy) + Jstr(" ") + d2s(sp1[JAG_SP_START+8].tof()+dz);
	} else if ( colType1 == JAG_C_COL_TYPE_SQUARE ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() )+dx;
		double y = jagatof( sp1[JAG_SP_START+1].c_str() )+dy;
		double r = jagatof( sp1[JAG_SP_START+2].c_str() );
		double nx = JagGeo::safeget(sp1, JAG_SP_START+3);
		val = "CJAG=" + intToStr(srid) + "=0=SQ=d 0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(r) + " " + d2s(nx);
	} else if ( colType1 == JAG_C_COL_TYPE_SQUARE3D ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() )+dx;
		double y = jagatof( sp1[JAG_SP_START+1].c_str() )+dy;
		double z = jagatof( sp1[JAG_SP_START+2].c_str() )+dz;
		double r = jagatof( sp1[JAG_SP_START+3].c_str() );
		double nx = JagGeo::safeget(sp1, JAG_SP_START+4);
		double ny = JagGeo::safeget(sp1, JAG_SP_START+5);
		val = "CJAG=" + intToStr(srid) + "=0=SQ3=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + d2s(r) + " " + d2s(nx) + " " + d2s(ny);
	} else if ( colType1 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() )+dx;
		double y = jagatof( sp1[JAG_SP_START+1].c_str() )+dy;
		double a = jagatof( sp1[JAG_SP_START+2].c_str() );
		double b = jagatof( sp1[JAG_SP_START+3].c_str() );
		double nx = JagGeo::safeget(sp1, JAG_SP_START+4);
		val = "CJAG=" + intToStr(srid) + "=0=RC=d 0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(a) + " " + d2s(b) + " " + d2s(nx);
	} else if ( colType1 == JAG_C_COL_TYPE_RECTANGLE3D ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() )+dx;
		double y = jagatof( sp1[JAG_SP_START+1].c_str() )+dy;
		double z = jagatof( sp1[JAG_SP_START+2].c_str() )+dz;
		double a = jagatof( sp1[JAG_SP_START+3].c_str() );
		double b = jagatof( sp1[JAG_SP_START+4].c_str() );
		double nx = JagGeo::safeget(sp1, JAG_SP_START+5);
		double ny = JagGeo::safeget(sp1, JAG_SP_START+6);
		val = "CJAG=" + intToStr(srid) + "=0=RC3=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + d2s(a) + " " + d2s(b) + " " + d2s(nx) + " " + d2s(ny);
	} else if ( colType1 == JAG_C_COL_TYPE_CIRCLE ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() )+dx;
		double y = jagatof( sp1[JAG_SP_START+1].c_str() )+dy;
		val = "CJAG=" + intToStr(srid) + "=0=CR=d 0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + sp1[JAG_SP_START+2];
	} else if ( colType1 == JAG_C_COL_TYPE_CIRCLE3D ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() )+dx;
		double y = jagatof( sp1[JAG_SP_START+1].c_str() )+dy;
		double z = jagatof( sp1[JAG_SP_START+2].c_str() )+dz;
		//double r = jagatof( sp1[JAG_SP_START+3].c_str() );
		double nx = JagGeo::safeget(sp1, JAG_SP_START+4);
		double ny = JagGeo::safeget(sp1, JAG_SP_START+5);
		val = "CJAG=" + intToStr(srid) + "=0=CR3=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + sp1[JAG_SP_START+3]+ " " + d2s(nx) + d2s(ny);
	} else if ( colType1 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() )+dx;
		double y = jagatof( sp1[JAG_SP_START+1].c_str() )+dy;
		double a = jagatof( sp1[JAG_SP_START+2].c_str() );
		double b = jagatof( sp1[JAG_SP_START+3].c_str() );
		double nx = JagGeo::safeget(sp1, JAG_SP_START+3);
		val = "CJAG=" + intToStr(srid) + "=0=EL=d 0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(a) + " " + d2s(b) + " " + d2s(nx);
	} else if ( colType1 == JAG_C_COL_TYPE_ELLIPSE3D ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() )+dx;
		double y = jagatof( sp1[JAG_SP_START+1].c_str() )+dy;
		double z = jagatof( sp1[JAG_SP_START+2].c_str() )+dz;
		double a = jagatof( sp1[JAG_SP_START+3].c_str() );
		double b = jagatof( sp1[JAG_SP_START+4].c_str() );
		double nx = JagGeo::safeget(sp1, JAG_SP_START+5);
		double ny = JagGeo::safeget(sp1, JAG_SP_START+6);
		val = "CJAG=" + intToStr(srid) + "=0=EL3=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + d2s(a) + " " + d2s(b) + " " + d2s(nx) + " " + d2s(ny);
	} else if ( colType1 == JAG_C_COL_TYPE_CUBE  ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() )+dx;
		double y = jagatof( sp1[JAG_SP_START+1].c_str() )+dy;
		double z = jagatof( sp1[JAG_SP_START+2].c_str() )+dz;
		//double r = jagatof( sp1[JAG_SP_START+3].c_str() );
		double nx = jagatof( sp1[JAG_SP_START+4].c_str() );
		double ny = jagatof( sp1[JAG_SP_START+5].c_str() );
		val = "CJAG=" + intToStr(srid) + "=0=CB=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + sp1[JAG_SP_START+3] + " " + d2s(nx) + " " + d2s(ny);
	} else if ( colType1 == JAG_C_COL_TYPE_SPHERE ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() )+dx;
		double y = jagatof( sp1[JAG_SP_START+1].c_str() )+dy;
		double z = jagatof( sp1[JAG_SP_START+2].c_str() )+dz;
		//double r = jagatof( sp1[JAG_SP_START+3].c_str() );
		val = "CJAG=" + intToStr(srid) + "=0=SR=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + sp1[JAG_SP_START+3];
	} else if ( colType1 == JAG_C_COL_TYPE_BOX || colType1 == JAG_C_COL_TYPE_ELLIPSOID  ) {
		double x = jagatof( sp1[JAG_SP_START+0].c_str() )+dx;
		double y = jagatof( sp1[JAG_SP_START+1].c_str() )+dy;
		double z = jagatof( sp1[JAG_SP_START+2].c_str() )+dz;
		//double a = jagatof( sp1[JAG_SP_START+3].c_str() );
		//double b = jagatof( sp1[JAG_SP_START+4].c_str() );
		//double c = jagatof( sp1[JAG_SP_START+5].c_str() );
		double nx = jagatof( sp1[JAG_SP_START+6].c_str() );
		double ny = jagatof( sp1[JAG_SP_START+7].c_str() );
		if ( colType1 == JAG_C_COL_TYPE_BOX ) {
			val = "CJAG=" + intToStr(srid) + "=0=BX=d 0:0:0:0:0:0 ";
		} else {
			val = "CJAG=" + intToStr(srid) + "=0=ES=d 0:0:0:0:0:0 ";
		}
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) 
		     + " " + sp1[JAG_SP_START+3] + " " + sp1[JAG_SP_START+4] + " " + sp1[JAG_SP_START+5]
			 + " " + d2s(nx) + " " + d2s(ny);

	} else if ( colType1 == JAG_C_COL_TYPE_CONE || colType1 == JAG_C_COL_TYPE_CYLINDER  ) {
        double x = jagatof( sp1[JAG_SP_START+0].c_str() )+dx;
        double y = jagatof( sp1[JAG_SP_START+1].c_str() )+dy;
        double z = jagatof( sp1[JAG_SP_START+2].c_str() )+dz;
        //double r = jagatof( sp1[JAG_SP_START+3].c_str() );
        //double h = jagatof( sp1[JAG_SP_START+4].c_str() );
        double nx = JagGeo::safeget(sp1, JAG_SP_START+5);
        double ny = JagGeo::safeget(sp1, JAG_SP_START+6);
		if ( colType1 == JAG_C_COL_TYPE_CONE ) {
				val = "CJAG=" + intToStr(srid) + "=0=CN=d 0:0:0:0:0:0 ";
		} else {
				val = "CJAG=" + intToStr(srid) + "=0=CL=d 0:0:0:0:0:0 ";
		}
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + sp1[JAG_SP_START+3] + " " + sp1[JAG_SP_START+4] + 
				+ " " + d2s(nx) + " " + d2s(ny);
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING ) {
		JagLineString3D linestr;
		JagParser::addLineStringData( linestr, sp1 );
		linestr.translate( dx,dy,dz, false );
		linestr.toJAG( JAG_C_COL_TYPE_LINESTRING, false, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING3D ) {
		JagLineString3D linestr;
		JagParser::addLineString3DData( linestr, sp1 );
		linestr.translate( dx,dy,dz, true );
		linestr.toJAG( JAG_C_COL_TYPE_LINESTRING3D, true, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOINT ) {
		JagLineString3D linestr;
		JagParser::addLineStringData( linestr, sp1 );
		linestr.translate( dx,dy,dz, false );
		linestr.toJAG( JAG_C_COL_TYPE_MULTIPOINT, false, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOINT3D ) {
		JagLineString3D linestr;
		JagParser::addLineString3DData( linestr, sp1 );
		linestr.translate( dx,dy,dz, true );
		linestr.toJAG( JAG_C_COL_TYPE_MULTIPOINT3D, true, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON ) {
		JagPolygon pgon;
		int rc = JagParser::addPolygonData( pgon, sp1, false );
		if ( rc < 0 ) return false;
		pgon.translate( dx,dy,dz, false );
		pgon.toJAG( false, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON3D ) {
		JagPolygon pgon;
		int rc = JagParser::addPolygon3DData( pgon, sp1, false );
		if ( rc < 0 ) return false;
		pgon.translate( dx,dy,dz, true );
		pgon.toJAG( true, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		JagVector<JagPolygon> pgvec;
        int n = JagParser::addMultiPolygonData( pgvec, sp1, false, false );
        if ( n <= 0 ) return false;
		for ( int i=0; i < pgvec.size(); ++i ) { pgvec[i].translate( dx,dy,dz, false ); }
		bool rc = JagGeo::toJAG( pgvec, false, true, sp1[1], srid, val );
		if ( ! rc ) return false; 
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
		JagVector<JagPolygon> pgvec;
        int n = JagParser::addMultiPolygonData( pgvec, sp1, false, true );
        if ( n <= 0 ) return false;
		for ( int i=0; i < pgvec.size(); ++i ) { pgvec[i].translate( dx,dy,dz, true ); }
		bool rc = JagGeo::toJAG( pgvec, true, true, sp1[1], srid, val );
		if ( ! rc ) return false; 
	} else {
		return false;
	}

	return true;
}

bool BinaryOpNode::doAllAffine( int srid, const Jstr &colType1, const JagStrSplit &sp1, const Jstr &carg, Jstr &val )
{
	if ( sp1.size() < 1 ) return false;
	double a,b,c,d,e,f,g,h,i, dx,dy,dz;
	JagStrSplit sf( carg, ':', true);
	if ( 6 == sf.size() ) {
		a = sf[0].tof(); b = sf[1].tof(); d = sf[2].tof(); e = sf[3].tof();
		dx = sf[4].tof(); dy = sf[5].tof();
	} else if ( 12 == sf.size() ) {
		a = sf[0].tof(); b = sf[1].tof(); c = sf[2].tof(); d = sf[3].tof();
		e = sf[4].tof(); f = sf[5].tof(); g = sf[6].tof(); h = sf[7].tof(); i = sf[8].tof();
		dx = sf[9].tof(); dy = sf[10].tof(); dz = sf[11].tof();
	} else return false;

	double x1, y1, z1, x, y, z;

	if ( colType1 == JAG_C_COL_TYPE_POINT  ) {
		val = "CJAG=" + intToStr(srid) + "=0=PT=d 0:0:0:0";
		x1 = sp1[JAG_SP_START+0].tof() ;
		y1 = sp1[JAG_SP_START+1].tof() ;
		affine2d(x1, y1, a, b, d, e, dx, dy, x, y );
		val += Jstr(" ") + d2s(x) + Jstr(" ") + d2s(y);
	} else if ( colType1 == JAG_C_COL_TYPE_POINT3D  ) {
		val = "CJAG=" + intToStr(srid) + "=0=PT3=d 0:0:0:0:0:0";
		x1 = sp1[JAG_SP_START+0].tof() ;
		y1 = sp1[JAG_SP_START+1].tof() ;
		z1 = sp1[JAG_SP_START+2].tof() ;
		affine3d(x1, y1, z1, a, b, c, d, e, f, g, h, i, dx, dy, dz, x, y, z );
		val += Jstr(" ") + d2s(x) + Jstr(" ") + d2s(y) + " " + d2s(z);
	} else if ( colType1 == JAG_C_COL_TYPE_LINE  ) {
		val = "CJAG=" + intToStr(srid) + "=0=LN=d 0:0:0:0";
		x1 = sp1[JAG_SP_START+0].tof() ;
		y1 = sp1[JAG_SP_START+1].tof() ;
		affine2d(x1, y1, a, b, d, e, dx, dy, x, y );
		val += Jstr(" ") + d2s(x) + Jstr(" ") + d2s(y);

		x1 = sp1[JAG_SP_START+2].tof() ;
		y1 = sp1[JAG_SP_START+3].tof() ;
		affine2d(x1, y1, a, b, d, e, dx, dy, x, y );
		val += Jstr(" ") + d2s(x) + Jstr(" ") + d2s(y);
	} else if ( colType1 == JAG_C_COL_TYPE_LINE3D ) {
		val = "CJAG=" + intToStr(srid) + "=0=LN3=d 0:0:0:0:0:0 ";

		x1 = sp1[JAG_SP_START+0].tof() ;
		y1 = sp1[JAG_SP_START+1].tof() ;
		z1 = sp1[JAG_SP_START+2].tof() ;
		affine3d(x1, y1, z1, a, b, c, d, e, f, g, h, i, dx, dy, dz, x, y, z );
		val += Jstr(" ") + d2s(x) + Jstr(" ") + d2s(y) + " " + d2s(z);

		x1 = sp1[JAG_SP_START+3].tof() ;
		y1 = sp1[JAG_SP_START+4].tof() ;
		z1 = sp1[JAG_SP_START+5].tof() ;
		affine3d(x1, y1, z1, a, b, c, d, e, f, g, h, i, dx, dy, dz, x, y, z );
		val += Jstr(" ") + d2s(x) + Jstr(" ") + d2s(y) + " " + d2s(z);
	} else if ( colType1 == JAG_C_COL_TYPE_TRIANGLE  ) {
		val = "CJAG=" + intToStr(srid) + "=0=TR=d 0:0:0:0 ";

		x1 = sp1[JAG_SP_START+0].tof() ;
		y1 = sp1[JAG_SP_START+1].tof() ;
		affine2d(x1, y1, a, b, d, e, dx, dy, x, y );
		val += Jstr(" ") + d2s(x) + Jstr(" ") + d2s(y);

		x1 = sp1[JAG_SP_START+2].tof() ;
		y1 = sp1[JAG_SP_START+3].tof() ;
		affine2d(x1, y1, a, b, d, e, dx, dy, x, y );
		val += Jstr(" ") + d2s(x) + Jstr(" ") + d2s(y);

		x1 = sp1[JAG_SP_START+4].tof() ;
		y1 = sp1[JAG_SP_START+5].tof() ;
		affine2d(x1, y1, a, b, d, e, dx, dy, x, y );
		val += Jstr(" ") + d2s(x) + Jstr(" ") + d2s(y);

	} else if ( colType1 == JAG_C_COL_TYPE_TRIANGLE3D  ) {
		val = "CJAG=" + intToStr(srid) + "=0=TR3=d 0:0:0:0:0:0 ";

		x1 = sp1[JAG_SP_START+0].tof() ;
		y1 = sp1[JAG_SP_START+1].tof() ;
		z1 = sp1[JAG_SP_START+2].tof() ;
		affine3d(x1, y1, z1, a, b, c, d, e, f, g, h, i, dx, dy, dz, x, y, z );
		val += Jstr(" ") + d2s(x) + Jstr(" ") + d2s(y) + " " + d2s(z);

		x1 = sp1[JAG_SP_START+3].tof() ;
		y1 = sp1[JAG_SP_START+4].tof() ;
		z1 = sp1[JAG_SP_START+5].tof() ;
		affine3d(x1, y1, z1, a, b, c, d, e, f, g, h, i, dx, dy, dz, x, y, z );
		val += Jstr(" ") + d2s(x) + Jstr(" ") + d2s(y) + " " + d2s(z);

		x1 = sp1[JAG_SP_START+6].tof() ;
		y1 = sp1[JAG_SP_START+7].tof() ;
		z1 = sp1[JAG_SP_START+8].tof() ;
		affine3d(x1, y1, z1, a, b, c, d, e, f, g, h, i, dx, dy, dz, x, y, z );
		val += Jstr(" ") + d2s(x) + Jstr(" ") + d2s(y) + " " + d2s(z);

	} else if ( colType1 == JAG_C_COL_TYPE_SQUARE ) {
		x1 = jagatof( sp1[JAG_SP_START+0].c_str() )+dx;
		y1 = jagatof( sp1[JAG_SP_START+1].c_str() )+dy;
		double r = jagatof( sp1[JAG_SP_START+2].c_str() );
		double nx = JagGeo::safeget(sp1, JAG_SP_START+3);
		affine2d(x1, y1, a, b, d, e, dx, dy, x, y );
		val = "CJAG=" + intToStr(srid) + "=0=SQ=d 0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(r) + " " + d2s(nx);
	} else if ( colType1 == JAG_C_COL_TYPE_SQUARE3D ) {
		x1 = jagatof( sp1[JAG_SP_START+0].c_str() )+dx;
		y1 = jagatof( sp1[JAG_SP_START+1].c_str() )+dy;
		z1 = jagatof( sp1[JAG_SP_START+2].c_str() )+dz;
		double r = jagatof( sp1[JAG_SP_START+3].c_str() );
		double nx = JagGeo::safeget(sp1, JAG_SP_START+4);
		double ny = JagGeo::safeget(sp1, JAG_SP_START+5);
		affine3d(x1, y1, z1, a, b, c, d, e, f, g, h, i, dx, dy, dz, x, y, z );
		val = "CJAG=" + intToStr(srid) + "=0=SQ3=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + d2s(r) + " " + d2s(nx) + " " + d2s(ny);
	} else if ( colType1 == JAG_C_COL_TYPE_RECTANGLE ) {
		x1 = jagatof( sp1[JAG_SP_START+0].c_str() )+dx;
		y1 = jagatof( sp1[JAG_SP_START+1].c_str() )+dy;
		double a1 = jagatof( sp1[JAG_SP_START+2].c_str() );
		double b1 = jagatof( sp1[JAG_SP_START+3].c_str() );
		double nx = JagGeo::safeget(sp1, JAG_SP_START+4);
		affine2d(x1, y1, a, b, d, e, dx, dy, x, y );
		val = "CJAG=" + intToStr(srid) + "=0=RC=d 0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(a1) + " " + d2s(b1) + " " + d2s(nx);
	} else if ( colType1 == JAG_C_COL_TYPE_RECTANGLE3D ) {
		x1 = jagatof( sp1[JAG_SP_START+0].c_str() )+dx;
		y1 = jagatof( sp1[JAG_SP_START+1].c_str() )+dy;
		z1 = jagatof( sp1[JAG_SP_START+2].c_str() )+dz;
		double a1 = jagatof( sp1[JAG_SP_START+3].c_str() );
		double b1 = jagatof( sp1[JAG_SP_START+4].c_str() );
		double nx = JagGeo::safeget(sp1, JAG_SP_START+5);
		double ny = JagGeo::safeget(sp1, JAG_SP_START+6);
		affine3d(x1, y1, z1, a, b, c, d, e, f, g, h, i, dx, dy, dz, x, y, z );
		val = "CJAG=" + intToStr(srid) + "=0=RC3=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + d2s(a1) + " " + d2s(b1) + " " + d2s(nx) + " " + d2s(ny);
	} else if ( colType1 == JAG_C_COL_TYPE_CIRCLE ) {
		x1 = jagatof( sp1[JAG_SP_START+0].c_str() )+dx;
		y1 = jagatof( sp1[JAG_SP_START+1].c_str() )+dy;
		affine2d(x1, y1, a, b, d, e, dx, dy, x, y );
		val = "CJAG=" + intToStr(srid) + "=0=CR=d 0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + sp1[JAG_SP_START+2];
	} else if ( colType1 == JAG_C_COL_TYPE_CIRCLE3D ) {
		x1 = jagatof( sp1[JAG_SP_START+0].c_str() )+dx;
		y1 = jagatof( sp1[JAG_SP_START+1].c_str() )+dy;
		z1 = jagatof( sp1[JAG_SP_START+2].c_str() )+dz;
		//double r = jagatof( sp1[JAG_SP_START+3].c_str() );
		double nx = JagGeo::safeget(sp1, JAG_SP_START+4);
		double ny = JagGeo::safeget(sp1, JAG_SP_START+5);
		affine3d(x1, y1, z1, a, b, c, d, e, f, g, h, i, dx, dy, dz, x, y, z );
		val = "CJAG=" + intToStr(srid) + "=0=CR3=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + sp1[JAG_SP_START+3]+ " " + d2s(nx) + d2s(ny);
	} else if ( colType1 == JAG_C_COL_TYPE_ELLIPSE ) {
		x1 = jagatof( sp1[JAG_SP_START+0].c_str() )+dx;
		y1 = jagatof( sp1[JAG_SP_START+1].c_str() )+dy;
		double a1 = jagatof( sp1[JAG_SP_START+2].c_str() );
		double b1 = jagatof( sp1[JAG_SP_START+3].c_str() );
		double nx = JagGeo::safeget(sp1, JAG_SP_START+3);
		affine2d(x1, y1, a, b, d, e, dx, dy, x, y );
		val = "CJAG=" + intToStr(srid) + "=0=EL=d 0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(a1) + " " + d2s(b1) + " " + d2s(nx);
	} else if ( colType1 == JAG_C_COL_TYPE_ELLIPSE3D ) {
		x1 = jagatof( sp1[JAG_SP_START+0].c_str() )+dx;
		y1 = jagatof( sp1[JAG_SP_START+1].c_str() )+dy;
		z1 = jagatof( sp1[JAG_SP_START+2].c_str() )+dz;
		double a1 = jagatof( sp1[JAG_SP_START+3].c_str() );
		double b1 = jagatof( sp1[JAG_SP_START+4].c_str() );
		double nx = JagGeo::safeget(sp1, JAG_SP_START+5);
		double ny = JagGeo::safeget(sp1, JAG_SP_START+6);
		affine3d(x1, y1, z1, a, b, c, d, e, f, g, h, i, dx, dy, dz, x, y, z );
		val = "CJAG=" + intToStr(srid) + "=0=EL3=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + d2s(a1) + " " + d2s(b1) + " " + d2s(nx) + " " + d2s(ny);
	} else if ( colType1 == JAG_C_COL_TYPE_CUBE  ) {
		x1 = jagatof( sp1[JAG_SP_START+0].c_str() )+dx;
		y1 = jagatof( sp1[JAG_SP_START+1].c_str() )+dy;
		z1 = jagatof( sp1[JAG_SP_START+2].c_str() )+dz;
		//double r = jagatof( sp1[JAG_SP_START+3].c_str() );
		double nx = jagatof( sp1[JAG_SP_START+4].c_str() );
		double ny = jagatof( sp1[JAG_SP_START+5].c_str() );
		affine3d(x1, y1, z1, a, b, c, d, e, f, g, h, i, dx, dy, dz, x, y, z );
		val = "CJAG=" + intToStr(srid) + "=0=CB=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + sp1[JAG_SP_START+3] + " " + d2s(nx) + " " + d2s(ny);
	} else if ( colType1 == JAG_C_COL_TYPE_SPHERE ) {
		x1 = jagatof( sp1[JAG_SP_START+0].c_str() )+dx;
		y1 = jagatof( sp1[JAG_SP_START+1].c_str() )+dy;
		z1 = jagatof( sp1[JAG_SP_START+2].c_str() )+dz;
		//double r = jagatof( sp1[JAG_SP_START+3].c_str() );
		affine3d(x1, y1, z1, a, b, c, d, e, f, g, h, i, dx, dy, dz, x, y, z );
		val = "CJAG=" + intToStr(srid) + "=0=SR=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + sp1[JAG_SP_START+3];
	} else if ( colType1 == JAG_C_COL_TYPE_BOX || colType1 == JAG_C_COL_TYPE_ELLIPSOID  ) {
		x1 = jagatof( sp1[JAG_SP_START+0].c_str() )+dx;
		y1 = jagatof( sp1[JAG_SP_START+1].c_str() )+dy;
		z1 = jagatof( sp1[JAG_SP_START+2].c_str() )+dz;
		//double a = jagatof( sp1[JAG_SP_START+3].c_str() );
		//double b = jagatof( sp1[JAG_SP_START+4].c_str() );
		//double c = jagatof( sp1[JAG_SP_START+5].c_str() );
		double nx = jagatof( sp1[JAG_SP_START+6].c_str() );
		double ny = jagatof( sp1[JAG_SP_START+7].c_str() );
		affine3d(x1, y1, z1, a, b, c, d, e, f, g, h, i, dx, dy, dz, x, y, z );
		if ( colType1 == JAG_C_COL_TYPE_BOX ) {
			val = "CJAG=" + intToStr(srid) + "=0=BX=d 0:0:0:0:0:0 ";
		} else {
			val = "CJAG=" + intToStr(srid) + "=0=ES=d 0:0:0:0:0:0 ";
		}
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) 
		     + " " + sp1[JAG_SP_START+3] + " " + sp1[JAG_SP_START+4] + " " + sp1[JAG_SP_START+5]
			 + " " + d2s(nx) + " " + d2s(ny);

	} else if ( colType1 == JAG_C_COL_TYPE_CONE || colType1 == JAG_C_COL_TYPE_CYLINDER  ) {
        x1 = jagatof( sp1[JAG_SP_START+0].c_str() )+dx;
        y1 = jagatof( sp1[JAG_SP_START+1].c_str() )+dy;
        z1 = jagatof( sp1[JAG_SP_START+2].c_str() )+dz;
        //double r = jagatof( sp1[JAG_SP_START+3].c_str() );
        //double h = jagatof( sp1[JAG_SP_START+4].c_str() );
        double nx = JagGeo::safeget(sp1, JAG_SP_START+5);
        double ny = JagGeo::safeget(sp1, JAG_SP_START+6);
		affine3d(x1, y1, z1, a, b, c, d, e, f, g, h, i, dx, dy, dz, x, y, z );
		if ( colType1 == JAG_C_COL_TYPE_CONE ) {
				val = "CJAG=" + intToStr(srid) + "=0=CN=d 0:0:0:0:0:0 ";
		} else {
				val = "CJAG=" + intToStr(srid) + "=0=CL=d 0:0:0:0:0:0 ";
		}
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + sp1[JAG_SP_START+3] + " " + sp1[JAG_SP_START+4] + 
				+ " " + d2s(nx) + " " + d2s(ny);
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING ) {
		JagLineString3D linestr;
		JagParser::addLineStringData( linestr, sp1 );
		linestr.affine2d( a, b, d, e, dx,dy );
		linestr.toJAG( JAG_C_COL_TYPE_LINESTRING, false, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING3D ) {
		JagLineString3D linestr;
		JagParser::addLineString3DData( linestr, sp1 );
		linestr.affine3d(a, b, c, d, e, f, g, h, i, dx, dy, dz );
		linestr.toJAG( JAG_C_COL_TYPE_LINESTRING3D, true, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOINT ) {
		JagLineString3D linestr;
		JagParser::addLineStringData( linestr, sp1 );
		linestr.affine2d( a, b, d, e, dx,dy );
		linestr.toJAG( JAG_C_COL_TYPE_MULTIPOINT, false, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOINT3D ) {
		JagLineString3D linestr;
		JagParser::addLineString3DData( linestr, sp1 );
		linestr.affine3d(a, b, c, d, e, f, g, h, i, dx, dy, dz );
		linestr.toJAG( JAG_C_COL_TYPE_MULTIPOINT3D, true, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON ) {
		JagPolygon pgon;
		int rc = JagParser::addPolygonData( pgon, sp1, false );
		if ( rc < 0 ) return false;
		pgon.affine2d( a, b, d, e, dx,dy );
		pgon.toJAG( false, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON3D ) {
		JagPolygon pgon;
		int rc = JagParser::addPolygon3DData( pgon, sp1, false );
		if ( rc < 0 ) return false;
		pgon.affine3d(a, b, c, d, e, f, g, h, i, dx, dy, dz );
		pgon.toJAG( true, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		JagVector<JagPolygon> pgvec;
        int n = JagParser::addMultiPolygonData( pgvec, sp1, false, false );
        if ( n <= 0 ) return false;
		for ( int i=0; i < pgvec.size(); ++i ) { pgvec[i].affine2d( a, b, d, e, dx,dy ); }
		bool rc = JagGeo::toJAG( pgvec, false, true, sp1[1], srid, val );
		if ( ! rc ) return false; 
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
		JagVector<JagPolygon> pgvec;
        int n = JagParser::addMultiPolygonData( pgvec, sp1, false, true );
        if ( n <= 0 ) return false;
		for ( int i=0; i < pgvec.size(); ++i ) { pgvec[i].affine3d(a, b, c, d, e, f, g, h, i, dx, dy, dz ); }
		bool rc = JagGeo::toJAG( pgvec, true, true, sp1[1], srid, val );
		if ( ! rc ) return false; 
	} else {
		return false;
	}

	return true;
}

bool BinaryOpNode::doAllDelaunayTriangles( int srid, const Jstr &colType1, const JagStrSplit &sp1, const Jstr &carg, Jstr &val )
{
	if ( sp1.size() < 1 ) return false;
	double tolerance = carg.tof();

	if ( colType1 == JAG_C_COL_TYPE_MULTIPOINT  ) {
		val = "CJAG=" + intToStr(srid) + "=0=MG=d 0:0:0:0";
		Jstr mpg;
		JagCGAL::getDelaunayTriangles2D( srid, sp1, tolerance, mpg );
		val += mpg;
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOINT3D  ) {
		val = "CJAG=" + intToStr(srid) + "=0=MG3=d 0:0:0:0:0:0";
	} else {
		return false;
	}

	return true;
}

Jstr BinaryOpNode::doAllVoronoiPolygons( int srid, const Jstr &colType1, const JagStrSplit &sp1, 
										 const Jstr &colType2, const JagStrSplit &sp2, const Jstr &carg )
{
	if ( sp1.size() < 1 ) return "";
	double tolerance = carg.tof();
	double xmin, ymin, xmax, ymax, zmin, zmax;
	Jstr val;

	if ( sp2.size() == 6 ) {
		xmin = sp2[JAG_SP_START+0].tof();
		ymin = sp2[JAG_SP_START+1].tof();
		xmax = sp2[JAG_SP_START+2].tof();
		ymax = sp2[JAG_SP_START+3].tof();
	} else if (  sp2.size() == 8 ) {
		xmin = sp2[JAG_SP_START+0].tof();
		ymin = sp2[JAG_SP_START+1].tof();
		zmin = sp2[JAG_SP_START+2].tof();
		xmax = sp2[JAG_SP_START+3].tof();
		ymax = sp2[JAG_SP_START+4].tof();
		zmax = sp2[JAG_SP_START+5].tof();
	} else {
		return "";
	}

	if ( colType1 == JAG_C_COL_TYPE_MULTIPOINT || colType1 == JAG_C_COL_TYPE_LINESTRING  ) {
		val = "CJAG=" + intToStr(srid) + "=0=MG=d 0:0:0:0";
		Jstr vor;
		JagCGAL::getVoronoiPolygons2D( srid, sp1, tolerance, xmin, ymin, xmax, ymax, JAG_C_COL_TYPE_MULTIPOLYGON, vor );
		val += vor;
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOINT3D  ) {
		val = "CJAG=" + intToStr(srid) + "=0=MG3=d 0:0:0:0:0:0";
		Jstr vor;
		val += vor;
	} else {
		return "";
	}

	return val;
}

Jstr BinaryOpNode::doAllVoronoiLines( int srid, const Jstr &colType1, const JagStrSplit &sp1, 
										 const Jstr &colType2, const JagStrSplit &sp2, const Jstr &carg )
{
	if ( sp1.size() < 1 ) return "";
	double tolerance = carg.tof();
	double xmin, ymin, xmax, ymax, zmin, zmax;
	Jstr val;

	if ( sp2.size() == 6 ) {
		xmin = sp2[JAG_SP_START+0].tof();
		ymin = sp2[JAG_SP_START+1].tof();
		xmax = sp2[JAG_SP_START+2].tof();
		ymax = sp2[JAG_SP_START+3].tof();
	} else if (  sp2.size() == 8 ) {
		xmin = sp2[JAG_SP_START+0].tof();
		ymin = sp2[JAG_SP_START+1].tof();
		zmin = sp2[JAG_SP_START+2].tof();
		xmax = sp2[JAG_SP_START+3].tof();
		ymax = sp2[JAG_SP_START+4].tof();
		zmax = sp2[JAG_SP_START+5].tof();
	} else {
		return "";
	}

	if ( colType1 == JAG_C_COL_TYPE_MULTIPOINT || colType1 == JAG_C_COL_TYPE_LINESTRING  ) {
		val = "CJAG=" + intToStr(srid) + "=0=ML=d 0:0:0:0";
		Jstr vor;
		JagCGAL::getVoronoiPolygons2D( srid, sp1, tolerance, xmin, ymin, xmax, ymax, JAG_C_COL_TYPE_MULTILINESTRING, vor );
		val += vor;
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOINT3D  ) {
		val = "CJAG=" + intToStr(srid) + "=0=ML3=d 0:0:0:0:0:0";
		Jstr vor;
		//val += JagCGAL::getVoronoiMultiPolygons3D( sp1, tolerance, xmin, ymin,zmin, xmax, ymax, zmax, vor );
		val += vor;
	} else {
		return "";
	}

	return val;
}

Jstr BinaryOpNode::doAllIsOnLeftSide( int srid, const Jstr &colType1, const JagStrSplit &sp1, 
										 const Jstr &colType2, const JagStrSplit &sp2, const Jstr &carg )
{
	if ( sp1.size() < 1 ) return "0";
	Jstr val = "0";
	double px, py, x1, y1, x2, y2;
	double mx1, my1, mx2, my2;
	if ( colType1 == JAG_C_COL_TYPE_POINT ) { 
		px = sp1[JAG_SP_START+0].tof(); 
		py = sp1[JAG_SP_START+1].tof(); 
		if ( colType2 == JAG_C_COL_TYPE_LINE ) {
			x1 = sp2[JAG_SP_START+0].tof(); 
			y1 = sp2[JAG_SP_START+1].tof(); 
			x2 = sp2[JAG_SP_START+2].tof(); 
			y2 = sp2[JAG_SP_START+3].tof(); 
			if ( JAG_LEFT == JagCGAL::pointRelateLine(px,py, x1, y1, x2, y2) ) {
				val = "1";
			}
		} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING ) {
			JagLineString3D linestr;
			JagParser::addLineStringData( linestr, sp2 );
			if ( ! linestr.pointOnLeft(px,py) ) return "0";
			val = "1";
		} else if ( colType2 == JAG_C_COL_TYPE_MULTILINESTRING ) {
			JagPolygon pgon;
			int rc = JagParser::addPolygonData( pgon, sp2, false );
			if ( rc < 0 ) return "0";
			for ( int k = 0; k < pgon.size(); ++k ) {
				const JagLineString3D &linestr = pgon.linestr[k];
				if ( ! linestr.pointOnLeft(px,py) ) return "0";
			}
			val = "1";
		} 
	} else if ( colType1 == JAG_C_COL_TYPE_LINE  ) {
		x1 = sp1[JAG_SP_START+0].tof(); 
		y1 = sp1[JAG_SP_START+1].tof(); 
		x2 = sp1[JAG_SP_START+2].tof(); 
		y2 = sp1[JAG_SP_START+3].tof(); 
		if ( colType2 == JAG_C_COL_TYPE_POINT ) {
			px = sp2[JAG_SP_START+0].tof(); 
			py = sp2[JAG_SP_START+1].tof(); 
			if ( JAG_RIGHT == JagCGAL::pointRelateLine(px,py, x1, y1, x2, y2) ) {
				val = "1";
			}
		} else if ( colType2 == JAG_C_COL_TYPE_LINE ) {
			mx1 = sp2[JAG_SP_START+0].tof(); 
			my1 = sp2[JAG_SP_START+1].tof(); 
			mx2 = sp2[JAG_SP_START+2].tof(); 
			my2 = sp2[JAG_SP_START+3].tof(); 
			if ( JAG_LEFT == JagCGAL::lineRelateLine( x1, y1, x2, y2, mx1, my1, mx2, my2 ) ) {
				val = "1";
			}
		} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING ) {
			JagLineString3D linestr;
			JagParser::addLineStringData( linestr, sp2 );
			if ( ! linestr.pointOnLeft(x1,y1) ) return "0";
			if ( ! linestr.pointOnLeft(x2,y2) ) return "0";
			val = "1";
		} else if ( colType2 == JAG_C_COL_TYPE_MULTILINESTRING ) {
			JagPolygon pgon;
			int rc = JagParser::addPolygonData( pgon, sp2, false );
			if ( rc < 0 ) return "0";
			for ( int k = 0; k < pgon.size(); ++k ) {
				const JagLineString3D &linestr = pgon.linestr[k];
				if ( ! linestr.pointOnLeft(x1,y1) ) return "0";
				if ( ! linestr.pointOnLeft(x2,y2) ) return "0";
			}
			val = "1";
		} 
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING  ) {
		JagLineString3D linestr1;
		JagParser::addLineStringData( linestr1, sp1 );

		if ( colType2 == JAG_C_COL_TYPE_POINT ) {
			px = sp2[JAG_SP_START+0].tof(); 
			py = sp2[JAG_SP_START+1].tof(); 
			if ( ! linestr1.pointOnRight(px,py) ) return "0";
			val = "1";
		} else if ( colType2 == JAG_C_COL_TYPE_LINE ) {
			mx1 = sp2[JAG_SP_START+0].tof(); 
			my1 = sp2[JAG_SP_START+1].tof(); 
			mx2 = sp2[JAG_SP_START+2].tof(); 
			my2 = sp2[JAG_SP_START+3].tof(); 
			if ( ! linestr1.pointOnRight(mx1,my1) ) return "0";
			if ( ! linestr1.pointOnRight(mx2,my2) ) return "0";
			val = "1";
		} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING ) {
			JagLineString3D linestr2;
			JagParser::addLineStringData( linestr2, sp2 );
			double x,y;
			for ( int i=0; i < linestr1.size(); ++i ) {
				x = linestr1.point[i].x;
				y = linestr1.point[i].y;
				if ( ! linestr2.pointOnLeft(x,y) ) return "0";
			}
			val = "1";
		} else if ( colType2 == JAG_C_COL_TYPE_MULTILINESTRING ) {
			JagPolygon pgon;
			double x,y;
			int rc = JagParser::addPolygonData( pgon, sp2, false );
			if ( rc < 0 ) return "0";
			for ( int i=0; i < linestr1.size(); ++i ) {
				x = linestr1.point[i].x;
				y = linestr1.point[i].y;
				if ( ! pgon.pointOnLeft(x,y) ) return "0";
			}
			val = "1";
		} 
	} else if ( colType1 == JAG_C_COL_TYPE_MULTILINESTRING  ) {
		JagPolygon pgon1;
		int rc = JagParser::addPolygonData( pgon1, sp1, false );
		if ( rc < 0 ) return "0";

		if ( colType2 == JAG_C_COL_TYPE_POINT ) {
			px = sp2[JAG_SP_START+0].tof(); 
			py = sp2[JAG_SP_START+1].tof(); 
			if ( ! pgon1.pointOnRight(px,py) )  return "0";
			val = "1";
		} else if ( colType2 == JAG_C_COL_TYPE_LINE ) {
			mx1 = sp2[JAG_SP_START+0].tof(); 
			my1 = sp2[JAG_SP_START+1].tof(); 
			mx2 = sp2[JAG_SP_START+2].tof(); 
			my2 = sp2[JAG_SP_START+3].tof(); 
			if ( ! pgon1.pointOnRight(mx1,my1) )  return "0";
			if ( ! pgon1.pointOnRight(mx2,my2) )  return "0";
			val = "1";
		} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING ) {
			JagLineString3D linestr2;
			JagParser::addLineStringData( linestr2, sp2 );
			double x,y;
			for ( int i=0; i < linestr2.size(); ++i ) {
				x = linestr2.point[i].x;
				y = linestr2.point[i].y;
				if ( ! pgon1.pointOnRight(x,y) )  return "0";
			}
			val = "1";
		} else if ( colType2 == JAG_C_COL_TYPE_MULTILINESTRING ) {
			JagPolygon pgon2;
			double x,y;
			int rc = JagParser::addPolygonData( pgon2, sp2, false );
			if ( rc < 0 ) return "0";
			for ( int k=0; k < pgon1.size(); ++k ) {
				const JagLineString3D &linestr1 = pgon1.linestr[k];
				for ( int i=0; i < linestr1.size(); ++i ) {
					x = linestr1.point[i].x;
					y = linestr1.point[i].y;
					if ( ! pgon2.pointOnLeft(x,y) )  return "0";
				}
			}
			val = "1";
		} 
	} 

	return val;
}

Jstr BinaryOpNode::doAllIsOnRightSide( int srid, const Jstr &colType1, const JagStrSplit &sp1, 
										 const Jstr &colType2, const JagStrSplit &sp2, const Jstr &carg )
{
	if ( sp1.size() < 1 ) return "0";
	Jstr val = "0";
	double px, py, x1, y1, x2, y2;
	double mx1, my1, mx2, my2;
	if ( colType1 == JAG_C_COL_TYPE_POINT ) { 
		px = sp1[JAG_SP_START+0].tof(); 
		py = sp1[JAG_SP_START+1].tof(); 
		if ( colType2 == JAG_C_COL_TYPE_LINE ) {
			x1 = sp2[JAG_SP_START+0].tof(); 
			y1 = sp2[JAG_SP_START+1].tof(); 
			x2 = sp2[JAG_SP_START+2].tof(); 
			y2 = sp2[JAG_SP_START+3].tof(); 
			if ( JAG_RIGHT == JagCGAL::pointRelateLine(px,py, x1, y1, x2, y2) ) {
				val = "1";
			}
		} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING ) {
			JagLineString3D linestr;
			JagParser::addLineStringData( linestr, sp2 );
			if ( ! linestr.pointOnRight(px,py) ) return "0";
			val = "1";
		} else if ( colType2 == JAG_C_COL_TYPE_MULTILINESTRING ) {
			JagPolygon pgon;
			int rc = JagParser::addPolygonData( pgon, sp2, false );
			if ( rc < 0 ) return "0";
			for ( int k = 0; k < pgon.size(); ++k ) {
				const JagLineString3D &linestr = pgon.linestr[k];
				if ( ! linestr.pointOnRight(px,py) ) return "0";
			}
			val = "1";
		} 
	} else if ( colType1 == JAG_C_COL_TYPE_LINE  ) {
		x1 = sp1[JAG_SP_START+0].tof(); 
		y1 = sp1[JAG_SP_START+1].tof(); 
		x2 = sp1[JAG_SP_START+2].tof(); 
		y2 = sp1[JAG_SP_START+3].tof(); 
		if ( colType2 == JAG_C_COL_TYPE_POINT ) {
			px = sp2[JAG_SP_START+0].tof(); 
			py = sp2[JAG_SP_START+1].tof(); 
			if ( JAG_LEFT == JagCGAL::pointRelateLine(px,py, x1, y1, x2, y2) ) {
				val = "1";
			}
		} else if ( colType2 == JAG_C_COL_TYPE_LINE ) {
			mx1 = sp2[JAG_SP_START+0].tof(); 
			my1 = sp2[JAG_SP_START+1].tof(); 
			mx2 = sp2[JAG_SP_START+2].tof(); 
			my2 = sp2[JAG_SP_START+3].tof(); 
			if ( JAG_RIGHT == JagCGAL::lineRelateLine( x1, y1, x2, y2, mx1, my1, mx2, my2 ) ) {
				val = "1";
			}
		} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING ) {
			JagLineString3D linestr;
			JagParser::addLineStringData( linestr, sp2 );
			if ( ! linestr.pointOnRight(x1,y1) ) return "0";
			if ( ! linestr.pointOnRight(x2,y2) ) return "0";
			val = "1";
		} else if ( colType2 == JAG_C_COL_TYPE_MULTILINESTRING ) {
			JagPolygon pgon;
			int rc = JagParser::addPolygonData( pgon, sp2, false );
			if ( rc < 0 ) return "0";
			for ( int k = 0; k < pgon.size(); ++k ) {
				const JagLineString3D &linestr = pgon.linestr[k];
				if ( ! linestr.pointOnRight(x1,y1) ) return "0";
				if ( ! linestr.pointOnRight(x2,y2) ) return "0";
			}
			val = "1";
		} 
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING  ) {
		JagLineString3D linestr1;
		JagParser::addLineStringData( linestr1, sp1 );

		if ( colType2 == JAG_C_COL_TYPE_POINT ) {
			px = sp2[JAG_SP_START+0].tof(); 
			py = sp2[JAG_SP_START+1].tof(); 
			if ( ! linestr1.pointOnLeft(px,py) ) return "0";
			val = "1";
		} else if ( colType2 == JAG_C_COL_TYPE_LINE ) {
			mx1 = sp2[JAG_SP_START+0].tof(); 
			my1 = sp2[JAG_SP_START+1].tof(); 
			mx2 = sp2[JAG_SP_START+2].tof(); 
			my2 = sp2[JAG_SP_START+3].tof(); 
			if ( ! linestr1.pointOnLeft(mx1,my1) ) return "0";
			if ( ! linestr1.pointOnLeft(mx2,my2) ) return "0";
			val = "1";
		} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING ) {
			JagLineString3D linestr2;
			JagParser::addLineStringData( linestr2, sp2 );
			double x,y;
			for ( int i=0; i < linestr1.size(); ++i ) {
				x = linestr1.point[i].x;
				y = linestr1.point[i].y;
				if ( ! linestr2.pointOnRight(x,y) ) return "0";
			}
			val = "1";
		} else if ( colType2 == JAG_C_COL_TYPE_MULTILINESTRING ) {
			JagPolygon pgon;
			double x,y;
			int rc = JagParser::addPolygonData( pgon, sp2, false );
			if ( rc < 0 ) return "0";
			for ( int i=0; i < linestr1.size(); ++i ) {
				x = linestr1.point[i].x;
				y = linestr1.point[i].y;
				if ( ! pgon.pointOnRight(x,y) ) return "0";
			}
			val = "1";
		} 
	} else if ( colType1 == JAG_C_COL_TYPE_MULTILINESTRING  ) {
		JagPolygon pgon1;
		int rc = JagParser::addPolygonData( pgon1, sp1, false );
		if ( rc < 0 ) return "0";

		if ( colType2 == JAG_C_COL_TYPE_POINT ) {
			px = sp2[JAG_SP_START+0].tof(); 
			py = sp2[JAG_SP_START+1].tof(); 
			if ( ! pgon1.pointOnLeft(px,py) )  return "0";
			val = "1";
		} else if ( colType2 == JAG_C_COL_TYPE_LINE ) {
			mx1 = sp2[JAG_SP_START+0].tof(); 
			my1 = sp2[JAG_SP_START+1].tof(); 
			mx2 = sp2[JAG_SP_START+2].tof(); 
			my2 = sp2[JAG_SP_START+3].tof(); 
			if ( ! pgon1.pointOnLeft(mx1,my1) )  return "0";
			if ( ! pgon1.pointOnLeft(mx2,my2) )  return "0";
			val = "1";
		} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING ) {
			JagLineString3D linestr2;
			JagParser::addLineStringData( linestr2, sp2 );
			double x,y;
			for ( int i=0; i < linestr2.size(); ++i ) {
				x = linestr2.point[i].x;
				y = linestr2.point[i].y;
				if ( ! pgon1.pointOnLeft(x,y) )  return "0";
			}
			val = "1";
		} else if ( colType2 == JAG_C_COL_TYPE_MULTILINESTRING ) {
			JagPolygon pgon2;
			double x,y;
			int rc = JagParser::addPolygonData( pgon2, sp2, false );
			if ( rc < 0 ) return "0";
			for ( int k=0; k < pgon1.size(); ++k ) {
				const JagLineString3D &linestr1 = pgon1.linestr[k];
				for ( int i=0; i < linestr1.size(); ++i ) {
					x = linestr1.point[i].x;
					y = linestr1.point[i].y;
					if ( ! pgon2.pointOnRight(x,y) ) return "0";
				}
			}
			val = "1";
		} 
	} 

	return val;
}

Jstr BinaryOpNode::doAllLeftRatio( int srid, const Jstr &colType1, const JagStrSplit &sp1, 
								   const Jstr &colType2, const JagStrSplit &sp2, const Jstr &carg )
{
	if ( sp1.size() < 1 ) return "0";
	Jstr val = "0";
	double px, py, x1, y1, x2, y2;
	double mx1, my1, mx2, my2;
	if ( colType1 == JAG_C_COL_TYPE_POINT ) { 
		px = sp1[JAG_SP_START+0].tof(); 
		py = sp1[JAG_SP_START+1].tof(); 
		if ( colType2 == JAG_C_COL_TYPE_LINE ) {
			x1 = sp2[JAG_SP_START+0].tof(); 
			y1 = sp2[JAG_SP_START+1].tof(); 
			x2 = sp2[JAG_SP_START+2].tof(); 
			y2 = sp2[JAG_SP_START+3].tof(); 
			if ( JAG_LEFT == JagCGAL::pointRelateLine(px,py, x1, y1, x2, y2) ) {
				val = "1";
			}
		} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING ) {
			JagLineString3D linestr;
			JagParser::addLineStringData( linestr, sp2 );
			val = d2s(linestr.pointOnLeftRatio(px,py));
		} else if ( colType2 == JAG_C_COL_TYPE_MULTILINESTRING ) {
			JagPolygon pgon;
			int rc = JagParser::addPolygonData( pgon, sp2, false );
			if ( rc < 0 ) return "0";
			val = pgon.pointOnLeftRatio(px,py);
		} 
	} else if ( colType1 == JAG_C_COL_TYPE_LINE  ) {
		x1 = sp1[JAG_SP_START+0].tof(); 
		y1 = sp1[JAG_SP_START+1].tof(); 
		x2 = sp1[JAG_SP_START+2].tof(); 
		y2 = sp1[JAG_SP_START+3].tof(); 
		if ( colType2 == JAG_C_COL_TYPE_POINT ) {
			px = sp2[JAG_SP_START+0].tof(); 
			py = sp2[JAG_SP_START+1].tof(); 
			if ( JAG_RIGHT == JagCGAL::pointRelateLine(px,py, x1, y1, x2, y2) ) {
				val = "1";
			}
		} else if ( colType2 == JAG_C_COL_TYPE_LINE ) {
			mx1 = sp2[JAG_SP_START+0].tof(); 
			my1 = sp2[JAG_SP_START+1].tof(); 
			mx2 = sp2[JAG_SP_START+2].tof(); 
			my2 = sp2[JAG_SP_START+3].tof(); 
			int cnt = 0;
			if ( JAG_LEFT == JagCGAL::pointRelateLine(x1,y1, mx1, my1, mx2, my2) ) ++cnt;
			if ( JAG_LEFT == JagCGAL::pointRelateLine(x2,y2, mx1, my1, mx2, my2) ) ++cnt;
			val = d2s( (double)cnt/2.0 );
		} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING ) {
			JagLineString3D linestr;
			JagParser::addLineStringData( linestr, sp2 );
			double r1 = linestr.pointOnLeftRatio(x1,y1);
			double r2 = linestr.pointOnLeftRatio(x2,y2);
			val = d2s( (r1+r2)/2.0 );
		} else if ( colType2 == JAG_C_COL_TYPE_MULTILINESTRING ) {
			JagPolygon pgon;
			int rc = JagParser::addPolygonData( pgon, sp2, false );
			if ( rc < 0 ) return "0";
			val = d2s(pgon.pointOnLeftRatio(px,py));
		} 
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING  ) {
		JagLineString3D linestr1;
		JagParser::addLineStringData( linestr1, sp1 );

		if ( colType2 == JAG_C_COL_TYPE_POINT ) {
			px = sp2[JAG_SP_START+0].tof(); 
			py = sp2[JAG_SP_START+1].tof(); 
			val = d2s(linestr1.pointOnRightRatio(px,py));
		} else if ( colType2 == JAG_C_COL_TYPE_LINE ) {
			mx1 = sp2[JAG_SP_START+0].tof(); 
			my1 = sp2[JAG_SP_START+1].tof(); 
			mx2 = sp2[JAG_SP_START+2].tof(); 
			my2 = sp2[JAG_SP_START+3].tof(); 
			double r1 = linestr1.pointOnRightRatio(mx1,my1);
			double r2 = linestr1.pointOnRightRatio(mx2,my2);
			val = d2s(0.5*(r1+r2));
		} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING ) {
			JagLineString3D linestr2;
			JagParser::addLineStringData( linestr2, sp2 );
			double x,y;
			int tot = 0; double cnt = 0.0;
			for ( int i=0; i < linestr1.size(); ++i ) {
				x = linestr1.point[i].x;
				y = linestr1.point[i].y;
				cnt += linestr2.pointOnLeftRatio(x,y);
				++tot;
			}
			val = ( tot > 0 ) ? d2s(cnt/tot) : "0.0";
		} else if ( colType2 == JAG_C_COL_TYPE_MULTILINESTRING ) {
			JagPolygon pgon;
			double x,y;
			int rc = JagParser::addPolygonData( pgon, sp2, false );
			if ( rc < 0 ) return "0";
			int tot = 0; double cnt = 0.0;
			for ( int i=0; i < linestr1.size(); ++i ) {
				x = linestr1.point[i].x;
				y = linestr1.point[i].y;
				cnt += pgon.pointOnLeftRatio(x,y);
				++tot;
			}
			val = ( tot > 0 ) ? d2s(cnt/tot) : "0.0";
		} 
	} else if ( colType1 == JAG_C_COL_TYPE_MULTILINESTRING  ) {
		JagPolygon pgon1;
		int rc = JagParser::addPolygonData( pgon1, sp1, false );
		if ( rc < 0 ) return "0";

		if ( colType2 == JAG_C_COL_TYPE_POINT ) {
			px = sp2[JAG_SP_START+0].tof(); 
			py = sp2[JAG_SP_START+1].tof(); 
			val = d2s(pgon1.pointOnRightRatio(px,py));
		} else if ( colType2 == JAG_C_COL_TYPE_LINE ) {
			mx1 = sp2[JAG_SP_START+0].tof(); 
			my1 = sp2[JAG_SP_START+1].tof(); 
			mx2 = sp2[JAG_SP_START+2].tof(); 
			my2 = sp2[JAG_SP_START+3].tof(); 
			double r1 = pgon1.pointOnRightRatio(mx1,my1);
			double r2 = pgon1.pointOnRightRatio(mx2,my2);
			val = d2s( (r1+r2)/2.0);
		} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING ) {
			JagLineString3D linestr2;
			JagParser::addLineStringData( linestr2, sp2 );
			double x,y;
			int tot = 0; double cnt=0.0;
			for ( int i=0; i < linestr2.size(); ++i ) {
				x = linestr2.point[i].x;
				y = linestr2.point[i].y;
				cnt += pgon1.pointOnRightRatio(x,y);
				++tot;
			}
			val = ( tot > 0 ) ? d2s(cnt/tot) : "0.0";
		} else if ( colType2 == JAG_C_COL_TYPE_MULTILINESTRING ) {
			JagPolygon pgon2;
			double x,y;
			int rc = JagParser::addPolygonData( pgon2, sp2, false );
			if ( rc < 0 ) return "0";
			double cnt=0.0;
			for ( int k=0; k < pgon1.size(); ++k ) {
				const JagLineString3D &linestr1 = pgon1.linestr[k];
				for ( int i=0; i < linestr1.size(); ++i ) {
					x = linestr1.point[i].x;
					y = linestr1.point[i].y;
					cnt += pgon2.pointOnLeftRatio(x,y);
				}
			}
		} 
	} 

	return val;
}

Jstr BinaryOpNode::doAllRightRatio( int srid, const Jstr &colType1, const JagStrSplit &sp1, 
								   const Jstr &colType2, const JagStrSplit &sp2, const Jstr &carg )
{
	if ( sp1.size() < 1 ) return "0";
	Jstr val = "0";
	double px, py, x1, y1, x2, y2;
	double mx1, my1, mx2, my2;
	if ( colType1 == JAG_C_COL_TYPE_POINT ) { 
		px = sp1[JAG_SP_START+0].tof(); 
		py = sp1[JAG_SP_START+1].tof(); 
		if ( colType2 == JAG_C_COL_TYPE_LINE ) {
			x1 = sp2[JAG_SP_START+0].tof(); 
			y1 = sp2[JAG_SP_START+1].tof(); 
			x2 = sp2[JAG_SP_START+2].tof(); 
			y2 = sp2[JAG_SP_START+3].tof(); 
			if ( JAG_RIGHT == JagCGAL::pointRelateLine(px,py, x1, y1, x2, y2) ) {
				val = "1";
			}
		} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING ) {
			JagLineString3D linestr;
			JagParser::addLineStringData( linestr, sp2 );
			val = d2s(linestr.pointOnRightRatio(px,py));
		} else if ( colType2 == JAG_C_COL_TYPE_MULTILINESTRING ) {
			JagPolygon pgon;
			int rc = JagParser::addPolygonData( pgon, sp2, false );
			if ( rc < 0 ) return "0";
			val = pgon.pointOnRightRatio(px,py);
		} 
	} else if ( colType1 == JAG_C_COL_TYPE_LINE  ) {
		x1 = sp1[JAG_SP_START+0].tof(); 
		y1 = sp1[JAG_SP_START+1].tof(); 
		x2 = sp1[JAG_SP_START+2].tof(); 
		y2 = sp1[JAG_SP_START+3].tof(); 
		if ( colType2 == JAG_C_COL_TYPE_POINT ) {
			px = sp2[JAG_SP_START+0].tof(); 
			py = sp2[JAG_SP_START+1].tof(); 
			if ( JAG_LEFT == JagCGAL::pointRelateLine(px,py, x1, y1, x2, y2) ) {
				val = "1";
			}
		} else if ( colType2 == JAG_C_COL_TYPE_LINE ) {
			mx1 = sp2[JAG_SP_START+0].tof(); 
			my1 = sp2[JAG_SP_START+1].tof(); 
			mx2 = sp2[JAG_SP_START+2].tof(); 
			my2 = sp2[JAG_SP_START+3].tof(); 
			int cnt = 0;
			if ( JAG_RIGHT == JagCGAL::pointRelateLine(x1,y1, mx1, my1, mx2, my2) ) ++cnt;
			if ( JAG_RIGHT == JagCGAL::pointRelateLine(x2,y2, mx1, my1, mx2, my2) ) ++cnt;
			val = d2s( (double)cnt/2.0 );
		} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING ) {
			JagLineString3D linestr;
			JagParser::addLineStringData( linestr, sp2 );
			double r1 = linestr.pointOnRightRatio(x1,y1);
			double r2 = linestr.pointOnRightRatio(x2,y2);
			val = d2s( (r1+r2)/2.0 );
		} else if ( colType2 == JAG_C_COL_TYPE_MULTILINESTRING ) {
			JagPolygon pgon;
			int rc = JagParser::addPolygonData( pgon, sp2, false );
			if ( rc < 0 ) return "0";
			val = d2s(pgon.pointOnRightRatio(px,py));
		} 
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING  ) {
		JagLineString3D linestr1;
		JagParser::addLineStringData( linestr1, sp1 );

		if ( colType2 == JAG_C_COL_TYPE_POINT ) {
			px = sp2[JAG_SP_START+0].tof(); 
			py = sp2[JAG_SP_START+1].tof(); 
			val = d2s(linestr1.pointOnLeftRatio(px,py));
		} else if ( colType2 == JAG_C_COL_TYPE_LINE ) {
			mx1 = sp2[JAG_SP_START+0].tof(); 
			my1 = sp2[JAG_SP_START+1].tof(); 
			mx2 = sp2[JAG_SP_START+2].tof(); 
			my2 = sp2[JAG_SP_START+3].tof(); 
			double r1 = linestr1.pointOnLeftRatio(mx1,my1);
			double r2 = linestr1.pointOnLeftRatio(mx2,my2);
			val = d2s(0.5*(r1+r2));
		} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING ) {
			JagLineString3D linestr2;
			JagParser::addLineStringData( linestr2, sp2 );
			double x,y;
			int tot = 0; double cnt = 0.0;
			for ( int i=0; i < linestr1.size(); ++i ) {
				x = linestr1.point[i].x;
				y = linestr1.point[i].y;
				cnt += linestr2.pointOnRightRatio(x,y);
				++tot;
			}
			val = ( tot > 0 ) ? d2s(cnt/tot) : "0.0";
		} else if ( colType2 == JAG_C_COL_TYPE_MULTILINESTRING ) {
			JagPolygon pgon;
			double x,y;
			int rc = JagParser::addPolygonData( pgon, sp2, false );
			if ( rc < 0 ) return "0";
			int tot = 0; double cnt = 0.0;
			for ( int i=0; i < linestr1.size(); ++i ) {
				x = linestr1.point[i].x;
				y = linestr1.point[i].y;
				cnt += pgon.pointOnRightRatio(x,y);
				++tot;
			}
			val = ( tot > 0 ) ? d2s(cnt/tot) : "0.0";
		} 
	} else if ( colType1 == JAG_C_COL_TYPE_MULTILINESTRING  ) {
		JagPolygon pgon1;
		int rc = JagParser::addPolygonData( pgon1, sp1, false );
		if ( rc < 0 ) return "0";

		if ( colType2 == JAG_C_COL_TYPE_POINT ) {
			px = sp2[JAG_SP_START+0].tof(); 
			py = sp2[JAG_SP_START+1].tof(); 
			val = d2s(pgon1.pointOnLeftRatio(px,py));
		} else if ( colType2 == JAG_C_COL_TYPE_LINE ) {
			mx1 = sp2[JAG_SP_START+0].tof(); 
			my1 = sp2[JAG_SP_START+1].tof(); 
			mx2 = sp2[JAG_SP_START+2].tof(); 
			my2 = sp2[JAG_SP_START+3].tof(); 
			double r1 = pgon1.pointOnLeftRatio(mx1,my1);
			double r2 = pgon1.pointOnLeftRatio(mx2,my2);
			val = d2s( (r1+r2)/2.0);
		} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING ) {
			JagLineString3D linestr2;
			JagParser::addLineStringData( linestr2, sp2 );
			double x,y;
			int tot = 0; double cnt=0.0;
			for ( int i=0; i < linestr2.size(); ++i ) {
				x = linestr2.point[i].x;
				y = linestr2.point[i].y;
				cnt += pgon1.pointOnLeftRatio(x,y);
				++tot;
			}
			val = ( tot > 0 ) ? d2s(cnt/tot) : "0.0";
		} else if ( colType2 == JAG_C_COL_TYPE_MULTILINESTRING ) {
			JagPolygon pgon2;
			double x,y;
			int rc = JagParser::addPolygonData( pgon2, sp2, false );
			if ( rc < 0 ) return "0";
			double cnt=0.0;
			for ( int k=0; k < pgon1.size(); ++k ) {
				const JagLineString3D &linestr1 = pgon1.linestr[k];
				for ( int i=0; i < linestr1.size(); ++i ) {
					x = linestr1.point[i].x;
					y = linestr1.point[i].y;
					cnt += pgon2.pointOnRightRatio(x,y);
				}
			}
		} 
	} 

	return val;
}


bool BinaryOpNode::doAllRotateAt( int srid, const Jstr &colType1, const JagStrSplit &sp1, const Jstr &carg, Jstr &val )
{
	if ( sp1.size() < 1 ) return false;
	// rotate(geom, 10, 'degree' )
	// rotateat(geom, 10, 'degree', x0,y0 )
	double x0,y0,alpha;
	x0 = y0 = 0.0;
	JagStrSplit sf( carg, ':', true);
	char c;
	if ( 2 == sf.size() ) {
		alpha = sf[0].tof();
		c = *( sf[1].c_str() );
	} else if ( 4 == sf.size() ) {
		alpha = sf[0].tof();
		c = *( sf[1].c_str() );
		x0 = sf[2].tof();
		y0 = sf[3].tof();
	} else {
		return false;
	}

	if ( c == 'r' || c == 'R' ) {
		// radians
	} else if ( c == 'd' || c == 'D' ) {
		// degrees
		//alpha = alpha*180.0/JAG_PI;
		alpha = alpha * 0.01745329252;
	} else {
		return false;
	}

	double x1, y1, x, y, nx;

	if ( colType1 == JAG_C_COL_TYPE_POINT  ) {
		val = "CJAG=" + intToStr(srid) + "=0=PT=d 0:0:0:0";
		x1 = sp1[JAG_SP_START+0].tof();
		y1 = sp1[JAG_SP_START+1].tof();
		rotateat( x1, y1, alpha, x0, y0, x, y );
		val += Jstr(" ") + d2s(x) + Jstr(" ") + d2s(y);
	} else if ( colType1 == JAG_C_COL_TYPE_LINE  ) {
		val = "CJAG=" + intToStr(srid) + "=0=LN=d 0:0:0:0";
		x1 = sp1[JAG_SP_START+0].tof();
		y1 = sp1[JAG_SP_START+1].tof();
		rotateat( x1, y1, alpha, x0, y0, x, y );
		val += Jstr(" ") + d2s(x) + Jstr(" ") + d2s(y);

		x1 = sp1[JAG_SP_START+2].tof();
		y1 = sp1[JAG_SP_START+3].tof();
		rotateat( x1, y1, alpha, x0, y0, x, y );
		val += Jstr(" ") + d2s(x) + Jstr(" ") + d2s(y);
	} else if ( colType1 == JAG_C_COL_TYPE_TRIANGLE  ) {
		val = "CJAG=" + intToStr(srid) + "=0=TR=d 0:0:0:0";
		x1 = sp1[JAG_SP_START+0].tof();
		y1 = sp1[JAG_SP_START+1].tof();
		rotateat( x1, y1, alpha, x0, y0, x, y );
		val += Jstr(" ") + d2s(x) + Jstr(" ") + d2s(y);

		x1 = sp1[JAG_SP_START+2].tof();
		y1 = sp1[JAG_SP_START+3].tof();
		rotateat( x1, y1, alpha, x0, y0, x, y );
		val += Jstr(" ") + d2s(x) + Jstr(" ") + d2s(y);

		x1 = sp1[JAG_SP_START+4].tof();
		y1 = sp1[JAG_SP_START+5].tof();
		rotateat( x1, y1, alpha, x0, y0, x, y );
		val += Jstr(" ") + d2s(x) + Jstr(" ") + d2s(y);
	} else if ( colType1 == JAG_C_COL_TYPE_SQUARE ) {
		double x1 = jagatof( sp1[JAG_SP_START+0].c_str() );
		double y1 = jagatof( sp1[JAG_SP_START+1].c_str() );
		double r = jagatof( sp1[JAG_SP_START+2].c_str() );
		double oldnx = JagGeo::safeget(sp1, JAG_SP_START+3);
		rotateat( x1, y1, alpha, x0, y0, x, y );
		rotatenx( oldnx, alpha, nx );
		val = "CJAG=" + intToStr(srid) + "=0=SQ=d 0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(r) + " " + d2s(nx);
	} else if ( colType1 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x1 = jagatof( sp1[JAG_SP_START+0].c_str() );
		double y1 = jagatof( sp1[JAG_SP_START+1].c_str() );
		double a = jagatof( sp1[JAG_SP_START+2].c_str() );
		double b = jagatof( sp1[JAG_SP_START+3].c_str() );
		double oldnx = JagGeo::safeget(sp1, JAG_SP_START+4);
		rotateat( x1, y1, alpha, x0, y0, x, y );
		rotatenx( oldnx, alpha, nx );
		val = "CJAG=" + intToStr(srid) + "=0=RC=d 0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(a) + " " + d2s(b) + " " + d2s(nx);
	} else if ( colType1 == JAG_C_COL_TYPE_CIRCLE ) {
		double x1 = jagatof( sp1[JAG_SP_START+0].c_str() );
		double y1 = jagatof( sp1[JAG_SP_START+1].c_str() );
		val = "CJAG=" + intToStr(srid) + "=0=CR=d 0:0:0:0 ";
		rotateat( x1, y1, alpha, x0, y0, x, y );
		val += d2s(x) + " " + d2s(y) + " " + sp1[JAG_SP_START+2];
	} else if ( colType1 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x1 = jagatof( sp1[JAG_SP_START+0].c_str() );
		double y1 = jagatof( sp1[JAG_SP_START+1].c_str() );
		double a = jagatof( sp1[JAG_SP_START+2].c_str() );
		double b = jagatof( sp1[JAG_SP_START+3].c_str() );
		double oldnx = JagGeo::safeget(sp1, JAG_SP_START+3);
		val = "CJAG=" + intToStr(srid) + "=0=EL=d 0:0:0:0 ";
		rotateat( x1, y1, alpha, x0, y0, x, y );
		rotatenx( oldnx, alpha, nx );
		val += d2s(x) + " " + d2s(y) + " " + d2s(a) + " " + d2s(b) + " " + d2s(nx);
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING ) {
		JagLineString3D linestr;
		JagParser::addLineStringData( linestr, sp1 );
		linestr.rotateat(  alpha, x0, y0 );
		linestr.toJAG( JAG_C_COL_TYPE_LINESTRING, false, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOINT ) {
		JagLineString3D linestr;
		JagParser::addLineStringData( linestr, sp1 );
		linestr.rotateat(  alpha, x0, y0 );
		linestr.toJAG( JAG_C_COL_TYPE_MULTIPOINT, false, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON ) {
		JagPolygon pgon;
		int rc = JagParser::addPolygonData( pgon, sp1, false );
		if ( rc < 0 ) return false;
		pgon.rotateat(  alpha, x0, y0 );
		pgon.toJAG( false, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		JagVector<JagPolygon> pgvec;
        int n = JagParser::addMultiPolygonData( pgvec, sp1, false, false );
        if ( n <= 0 ) return false;
		for ( int i=0; i < pgvec.size(); ++i ) { pgvec[i].rotateat( alpha, x0, y0 ); }
		bool rc = JagGeo::toJAG( pgvec, false, true, sp1[1], srid, val );
		if ( ! rc ) return false; 
	} else {
		return false;
	}

	return true;
}

bool BinaryOpNode::doAllRotateSelf( int srid, const Jstr &colType1, const JagStrSplit &sp1, const Jstr &carg, Jstr &val )
{
	if ( sp1.size() < 1 ) return false;
	// rotateself(geom, 10, 'degree' )
	double alpha;
	JagStrSplit sf( carg, ':', true);
	char c;
	if ( 2 == sf.size() ) {
		alpha = sf[0].tof();
		c = *( sf[1].c_str() );
	} else {
		return false;
	}

	if ( c == 'r' || c == 'R' ) {
		// radians
	} else if ( c == 'd' || c == 'D' ) {
		// degrees
		//alpha = alpha*180.0/JAG_PI;
		alpha = alpha * 0.01745329252;
	} else {
		return false;
	}

	double x, y, x1, y1, nx, x0, y0;

	if ( colType1 == JAG_C_COL_TYPE_POINT  ) {
		val = "CJAG=" + intToStr(srid) + "=0=PT=d 0:0:0:0";
		x = sp1[JAG_SP_START+0].tof();
		y = sp1[JAG_SP_START+1].tof();
		val += Jstr(" ") + d2s(x) + Jstr(" ") + d2s(y);
	} else if ( colType1 == JAG_C_COL_TYPE_LINE  ) {
		val = "CJAG=" + intToStr(srid) + "=0=LN=d 0:0:0:0";
		x1 = sp1[JAG_SP_START+0].tof();
		y1 = sp1[JAG_SP_START+1].tof();

		double x2 = sp1[JAG_SP_START+2].tof();
		double y2 = sp1[JAG_SP_START+3].tof();

		rotateat( x1, y1, alpha, (x1+x2)/2.0, (y1+y2)/2.0, x, y );
		val += Jstr(" ") + d2s(x) + Jstr(" ") + d2s(y);

		rotateat( x2, y2, alpha, (x1+x2)/2.0, (y1+y2)/2.0, x, y );
		val += Jstr(" ") + d2s(x) + Jstr(" ") + d2s(y);
	} else if ( colType1 == JAG_C_COL_TYPE_TRIANGLE  ) {
		val = "CJAG=" + intToStr(srid) + "=0=TR=d 0:0:0:0";
		x1 = sp1[JAG_SP_START+0].tof();
		y1 = sp1[JAG_SP_START+1].tof();
		double x2 = sp1[JAG_SP_START+2].tof();
		double y2 = sp1[JAG_SP_START+3].tof();
		double x3 = sp1[JAG_SP_START+4].tof();
		double y3 = sp1[JAG_SP_START+5].tof();

		x0 = (x1+x2+x3)/3.0;  // center
		y0 = (y1+y2+y3)/3.0;
		rotateat( x1, y1, alpha, x0, y0, x, y );
		val += Jstr(" ") + d2s(x) + Jstr(" ") + d2s(y);
		rotateat( x2, y2, alpha, x0, y0, x, y );
		val += Jstr(" ") + d2s(x) + Jstr(" ") + d2s(y);
		rotateat( x3, y3, alpha, x0, y0, x, y );
		val += Jstr(" ") + d2s(x) + Jstr(" ") + d2s(y);
	} else if ( colType1 == JAG_C_COL_TYPE_SQUARE ) {
		x = jagatof( sp1[JAG_SP_START+0].c_str() );
		y = jagatof( sp1[JAG_SP_START+1].c_str() );
		double r = jagatof( sp1[JAG_SP_START+2].c_str() );
		double oldnx = JagGeo::safeget(sp1, JAG_SP_START+3);
		rotatenx( oldnx, alpha, nx );
		val = "CJAG=" + intToStr(srid) + "=0=SQ=d 0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(r) + " " + d2s(nx);
	} else if ( colType1 == JAG_C_COL_TYPE_RECTANGLE ) {
		x = jagatof( sp1[JAG_SP_START+0].c_str() );
		y = jagatof( sp1[JAG_SP_START+1].c_str() );
		double a = jagatof( sp1[JAG_SP_START+2].c_str() );
		double b = jagatof( sp1[JAG_SP_START+3].c_str() );
		double oldnx = JagGeo::safeget(sp1, JAG_SP_START+4);
		rotatenx( oldnx, alpha, nx );
		val = "CJAG=" + intToStr(srid) + "=0=RC=d 0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(a) + " " + d2s(b) + " " + d2s(nx);
	} else if ( colType1 == JAG_C_COL_TYPE_CIRCLE ) {
		x = jagatof( sp1[JAG_SP_START+0].c_str() );
		y = jagatof( sp1[JAG_SP_START+1].c_str() );
		val = "CJAG=" + intToStr(srid) + "=0=CR=d 0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + sp1[JAG_SP_START+2];
	} else if ( colType1 == JAG_C_COL_TYPE_ELLIPSE ) {
		x = jagatof( sp1[JAG_SP_START+0].c_str() );
		y = jagatof( sp1[JAG_SP_START+1].c_str() );
		double a = jagatof( sp1[JAG_SP_START+2].c_str() );
		double b = jagatof( sp1[JAG_SP_START+3].c_str() );
		double oldnx = JagGeo::safeget(sp1, JAG_SP_START+3);
		val = "CJAG=" + intToStr(srid) + "=0=EL=d 0:0:0:0 ";
		rotatenx( oldnx, alpha, nx );
		val += d2s(x) + " " + d2s(y) + " " + d2s(a) + " " + d2s(b) + " " + d2s(nx);
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING ) {
		JagLineString3D linestr;
		JagParser::addLineStringData( linestr, sp1 );
		linestr.rotateself(  alpha );
		linestr.toJAG( JAG_C_COL_TYPE_LINESTRING, false, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOINT ) {
		JagLineString3D linestr;
		JagParser::addLineStringData( linestr, sp1 );
		linestr.rotateself(  alpha );
		linestr.toJAG( JAG_C_COL_TYPE_MULTIPOINT, false, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON ) {
		JagPolygon pgon;
		int rc = JagParser::addPolygonData( pgon, sp1, false );
		if ( rc < 0 ) return false;
		pgon.rotateself(  alpha );
		pgon.toJAG( false, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		JagVector<JagPolygon> pgvec;
        int n = JagParser::addMultiPolygonData( pgvec, sp1, false, false );
        if ( n <= 0 ) return false;
		for ( int i=0; i < pgvec.size(); ++i ) { pgvec[i].rotateself( alpha ); }
		bool rc = JagGeo::toJAG( pgvec, false, true, sp1[1], srid, val );
		if ( ! rc ) return false; 
	} else {
		return false;
	}

	return true;
}



bool BinaryOpNode::doAllTransScale( int srid, const Jstr &colType1, const JagStrSplit &sp1, const Jstr &carg, Jstr &val )
{
	if ( sp1.size() < 1 ) return false;
	double dx, dy, dz;
	double fx, fy, fz;
	dx = dy = dz = 0.0;
	fx = fy = fz = 1.0;
	JagStrSplit sf( carg, ':', true);
	if ( 3 ==  sf.size() ) {
		dx = fabs(sf[0].tof()); dy = fabs(sf[1].tof()); 
		fx = fy = fabs(sf[2].tof()); 
	} else if ( 4 == sf.size() ) {
		dx = fabs(sf[0].tof()); dy = fabs(sf[1].tof()); 
		fx = fabs(sf[2].tof()); fy = fabs(sf[3].tof());
	} else if ( 6 == sf.size() ) {
		dx = fabs(sf[0].tof()); dy = fabs(sf[1].tof()); dz = fabs(sf[2].tof());
		fx = fabs(sf[3].tof()); fy = fabs(sf[4].tof()); fz = fabs(sf[5].tof());
	} else {
		return false;
	}
	double x, y, z;

	if ( colType1 == JAG_C_COL_TYPE_POINT  ) {
		val = "CJAG=" + intToStr(srid) + "=0=PT=d 0:0:0:0";
		x = (sp1[JAG_SP_START+0].tof() + dx)*fx;
		y = (sp1[JAG_SP_START+1].tof() + dy)*fy;
		val += Jstr(" ") + d2s(x) + Jstr(" ") + d2s(y);
	} else if ( colType1 == JAG_C_COL_TYPE_POINT3D  ) {
		val = "CJAG=" + intToStr(srid) + "=0=PT3=d 0:0:0:0:0:0";
		x = (sp1[JAG_SP_START+0].tof() + dx)*fx;
		y = (sp1[JAG_SP_START+1].tof() + dy)*fy;
		z = (sp1[JAG_SP_START+2].tof() + dz)*fz;
		val += Jstr(" ") + d2s(x) + Jstr(" ") + d2s(y) + " " + d2s(z);
	} else if ( colType1 == JAG_C_COL_TYPE_LINE  ) {
		val = "CJAG=" + intToStr(srid) + "=0=LN=d 0:0:0:0 ";
		val += d2s((sp1[JAG_SP_START+0].tof()+dx)*fx) + Jstr(" ") + d2s((sp1[JAG_SP_START+1].tof()+dy)*fy) + Jstr(" ") 
			  + d2s((sp1[JAG_SP_START+2].tof()+dx)*fx) + Jstr(" ") + d2s((sp1[JAG_SP_START+3].tof()+dy)*fy);  
	} else if ( colType1 == JAG_C_COL_TYPE_LINE3D ) {
		val = "CJAG=" + intToStr(srid) + "=0=LN3=d 0:0:0:0:0:0 ";
		val += d2s((sp1[JAG_SP_START+0].tof()+dx)*fx) + " " + d2s((sp1[JAG_SP_START+1].tof()+dy)*fy) 
				+ " " + d2s((sp1[JAG_SP_START+2].tof()+dz)*fz) + d2s((sp1[JAG_SP_START+3].tof()+dx)*fx) + Jstr(" ") 
			  + d2s((sp1[JAG_SP_START+4].tof()+dy)*fy) + d2s((sp1[JAG_SP_START+5].tof()+dz)*fz);  
	} else if ( colType1 == JAG_C_COL_TYPE_TRIANGLE  ) {
		val = "CJAG=" + intToStr(srid) + "=0=TR=d 0:0:0:0 ";
		val += d2s((sp1[JAG_SP_START+0].tof()+dx)*fx) + Jstr(" ") + d2s((sp1[JAG_SP_START+1].tof()+dy)*fy) 
				+ Jstr(" ") + d2s((sp1[JAG_SP_START+2].tof()+dx)*fx) 
				+ Jstr(" ") + d2s((sp1[JAG_SP_START+3].tof()+dy)*fy)
				+ Jstr(" ") + d2s((sp1[JAG_SP_START+4].tof()+dx)*fx)
				+ Jstr(" ") + d2s((sp1[JAG_SP_START+5].tof()+dy)*fy);
	} else if ( colType1 == JAG_C_COL_TYPE_TRIANGLE3D  ) {
		val = "CJAG=" + intToStr(srid) + "=0=TR3=d 0:0:0:0:0:0 ";
		val += d2s((sp1[JAG_SP_START+0].tof()+dx)*fx) + Jstr(" ") + d2s((sp1[JAG_SP_START+1].tof()+dy)*fy) 
				+ Jstr(" ") + d2s((sp1[JAG_SP_START+2].tof()+dz)*fz) 
				+ Jstr(" ") + d2s((sp1[JAG_SP_START+3].tof()+dx)*fx) + Jstr(" ") + d2s((sp1[JAG_SP_START+4].tof()+dy)*fy)
				+ Jstr(" ") + d2s((sp1[JAG_SP_START+5].tof()+dz)*fz) + Jstr(" ") + d2s((sp1[JAG_SP_START+6].tof()+dx)*fx)
				+ Jstr(" ") + d2s((sp1[JAG_SP_START+7].tof()+dy)*fy) + Jstr(" ") + d2s((sp1[JAG_SP_START+8].tof()+dz)*fz);
	} else if ( colType1 == JAG_C_COL_TYPE_SQUARE ) {
		double x = (jagatof( sp1[JAG_SP_START+0].c_str() )+dx)*fx;
		double y = (jagatof( sp1[JAG_SP_START+1].c_str() )+dy)*fy;
		double r = jagatof( sp1[JAG_SP_START+2].c_str() );
		double nx = JagGeo::safeget(sp1, JAG_SP_START+3);
		double a = r*fx;
		//double b = r*fy;
		val = "CJAG=" + intToStr(srid) + "=0=RC=d 0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(a) + " " + d2s(a) + " " + d2s(nx);
	} else if ( colType1 == JAG_C_COL_TYPE_SQUARE3D ) {
		double x = (jagatof( sp1[JAG_SP_START+0].c_str() )+dx)*fx;
		double y = (jagatof( sp1[JAG_SP_START+1].c_str() )+dy)*fy;
		double z = (jagatof( sp1[JAG_SP_START+2].c_str() )+dz)*fz;
		double r = jagatof( sp1[JAG_SP_START+3].c_str() );
		double nx = JagGeo::safeget(sp1, JAG_SP_START+4);
		double ny = JagGeo::safeget(sp1, JAG_SP_START+5);
		double a = r*fx;
		double b = r*fy;
		double c = r*fz;
		val = "CJAG=" + intToStr(srid) + "=0=RC3=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + d2s(a) + " " + d2s(b) + " " + d2s(c) + " " + d2s(nx) + " " + d2s(ny);
	} else if ( colType1 == JAG_C_COL_TYPE_RECTANGLE ) {
		double x = (jagatof( sp1[JAG_SP_START+0].c_str() )+dx)*fx;
		double y = (jagatof( sp1[JAG_SP_START+1].c_str() )+dy)*fy;
		double a = jagatof( sp1[JAG_SP_START+2].c_str() )*fx;
		double b = jagatof( sp1[JAG_SP_START+3].c_str() )*fy;
		double nx = JagGeo::safeget(sp1, JAG_SP_START+4);
		val = "CJAG=" + intToStr(srid) + "=0=RC=d 0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(a) + " " + d2s(b) + " " + d2s(nx);
	} else if ( colType1 == JAG_C_COL_TYPE_RECTANGLE3D ) {
		double x = (jagatof( sp1[JAG_SP_START+0].c_str() )+dx)*fx;
		double y = (jagatof( sp1[JAG_SP_START+1].c_str() )+dy)*fy;
		double z = (jagatof( sp1[JAG_SP_START+2].c_str() )+dz)*fz;
		double a = jagatof( sp1[JAG_SP_START+3].c_str() )*fx;
		double b = jagatof( sp1[JAG_SP_START+4].c_str() )*fy;
		double nx = JagGeo::safeget(sp1, JAG_SP_START+5);
		double ny = JagGeo::safeget(sp1, JAG_SP_START+6);
		val = "CJAG=" + intToStr(srid) + "=0=RC3=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + d2s(a) + " " + d2s(b) + " " + d2s(nx) + " " + d2s(ny);
	} else if ( colType1 == JAG_C_COL_TYPE_CIRCLE ) {
		double x = (jagatof( sp1[JAG_SP_START+0].c_str() )+dx)*fx;
		double y = (jagatof( sp1[JAG_SP_START+1].c_str() )+dy)*fy;
		double r = jagatof( sp1[JAG_SP_START+2].c_str() );
		double a = r*fx;
		double b = r*fy;
		val = "CJAG=" + intToStr(srid) + "=0=EL=d 0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(a) + " " + d2s(b) + " 0.0";
	} else if ( colType1 == JAG_C_COL_TYPE_CIRCLE3D ) {
		double x = (jagatof( sp1[JAG_SP_START+0].c_str() )+dx)*fx;
		double y = (jagatof( sp1[JAG_SP_START+1].c_str() )+dy)*fy;
		double z = (jagatof( sp1[JAG_SP_START+2].c_str() )+dz)*fz;
		double r = jagatof( sp1[JAG_SP_START+3].c_str() );
		double nx = JagGeo::safeget(sp1, JAG_SP_START+4);
		double ny = JagGeo::safeget(sp1, JAG_SP_START+5);
		double a = r*fx;
		double b = r*fy;
		double c = r*fz;
		val = "CJAG=" + intToStr(srid) + "=0=EL3=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " 
		     + d2s(a) + " " + d2s(b) + " " + d2s(c) + " " + d2s(nx) + d2s(ny);
	} else if ( colType1 == JAG_C_COL_TYPE_ELLIPSE ) {
		double x = (jagatof( sp1[JAG_SP_START+0].c_str() )+dx)*fx;
		double y = (jagatof( sp1[JAG_SP_START+1].c_str() )+dy)*fy;
		double a = jagatof( sp1[JAG_SP_START+2].c_str() )*fx;
		double b = jagatof( sp1[JAG_SP_START+3].c_str() )*fy;
		double nx = JagGeo::safeget(sp1, JAG_SP_START+3);
		val = "CJAG=" + intToStr(srid) + "=0=EL=d 0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(a) + " " + d2s(b) + " " + d2s(nx);
	} else if ( colType1 == JAG_C_COL_TYPE_ELLIPSE3D ) {
		double x = (jagatof( sp1[JAG_SP_START+0].c_str() )+dx)*fx;
		double y = (jagatof( sp1[JAG_SP_START+1].c_str() )+dy)*fy;
		double z = (jagatof( sp1[JAG_SP_START+2].c_str() )+dz)*fz;
		double a = jagatof( sp1[JAG_SP_START+3].c_str() )*fx;
		double b = jagatof( sp1[JAG_SP_START+4].c_str() )*fy;
		double nx = JagGeo::safeget(sp1, JAG_SP_START+5);
		double ny = JagGeo::safeget(sp1, JAG_SP_START+6);
		val = "CJAG=" + intToStr(srid) + "=0=EL3=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + d2s(a) + " " + d2s(b) + " " + d2s(nx) + " " + d2s(ny);
	} else if ( colType1 == JAG_C_COL_TYPE_CUBE  ) {
		double x = (jagatof( sp1[JAG_SP_START+0].c_str() )+dx)*fx;
		double y = (jagatof( sp1[JAG_SP_START+1].c_str() )+dy)*fy;
		double z = (jagatof( sp1[JAG_SP_START+2].c_str() )+dz)*fz;
		double r = jagatof( sp1[JAG_SP_START+3].c_str() );
		double nx = jagatof( sp1[JAG_SP_START+4].c_str() );
		double ny = jagatof( sp1[JAG_SP_START+5].c_str() );
		double a = r*fx;
		double b = r*fy;
		double c = r*fz;
		val = "CJAG=" + intToStr(srid) + "=0=BX=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " 
				+ d2s(a) + " " + d2s(b) + " " + d2s(c) + " " + d2s(nx) + " " + d2s(ny);
	} else if ( colType1 == JAG_C_COL_TYPE_SPHERE ) {
		double x = (jagatof( sp1[JAG_SP_START+0].c_str() )+dx)*fx;
		double y = (jagatof( sp1[JAG_SP_START+1].c_str() )+dy)*fy;
		double z = (jagatof( sp1[JAG_SP_START+2].c_str() )+dz)*fz;
		double r = jagatof( sp1[JAG_SP_START+3].c_str() );
		double a = r*fx;
		double b = r*fy;
		double c = r*fz;
		val = "CJAG=" + intToStr(srid) + "=0=ES=d 0:0:0:0:0:0 ";
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " 
				+ d2s(a) + " " + d2s(b) + " " + d2s(c) + " 0.0 0.0";
	} else if ( colType1 == JAG_C_COL_TYPE_BOX || colType1 == JAG_C_COL_TYPE_ELLIPSOID  ) {
		double x = (jagatof( sp1[JAG_SP_START+0].c_str() )+dx)*fx;
		double y = (jagatof( sp1[JAG_SP_START+1].c_str() )+dy)*fy;
		double z = (jagatof( sp1[JAG_SP_START+2].c_str() )+dz)*fz;
		double a = jagatof( sp1[JAG_SP_START+3].c_str() )*fx;
		double b = jagatof( sp1[JAG_SP_START+4].c_str() )*fy;
		double c = jagatof( sp1[JAG_SP_START+5].c_str() )*fz;
		double nx = jagatof( sp1[JAG_SP_START+6].c_str() );
		double ny = jagatof( sp1[JAG_SP_START+7].c_str() );
		if ( colType1 == JAG_C_COL_TYPE_BOX ) {
			val = "CJAG=" + intToStr(srid) + "=0=BX=d 0:0:0:0:0:0 ";
		} else {
			val = "CJAG=" + intToStr(srid) + "=0=ES=d 0:0:0:0:0:0 ";
		}
		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " "
			+ d2s(a) + " " + d2s(b) + " " + d2s(c) + " "
			 + d2s(nx) + " " + d2s(ny);

	} else if ( colType1 == JAG_C_COL_TYPE_CONE || colType1 == JAG_C_COL_TYPE_CYLINDER  ) {
		if ( jagEQ(fx,fy) && jagEQ(fx,fz) ) {
            double x = (jagatof( sp1[JAG_SP_START+0].c_str() )+dx)*fx;
            double y = (jagatof( sp1[JAG_SP_START+1].c_str() )+dy)*fx;
            double z = (jagatof( sp1[JAG_SP_START+2].c_str() )+dz)*fx;
            double r = jagatof( sp1[JAG_SP_START+3].c_str() )*fx;
            double h = jagatof( sp1[JAG_SP_START+4].c_str() )*fx;
            double nx = JagGeo::safeget(sp1, JAG_SP_START+5);
            double ny = JagGeo::safeget(sp1, JAG_SP_START+6);
    		if ( colType1 == JAG_C_COL_TYPE_CONE ) {
    				val = "CJAG=" + intToStr(srid) + "=0=CN=d 0:0:0:0:0:0 ";
    		} else {
    				val = "CJAG=" + intToStr(srid) + "=0=CL=d 0:0:0:0:0:0 ";
    		}
    		val += d2s(x) + " " + d2s(y) + " " + d2s(z) + " " + d2s(r) + " " + d2s(h) + 
    				+ " " + d2s(nx) + " " + d2s(ny);
		}
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING ) {
		JagLineString3D linestr;
		JagParser::addLineStringData( linestr, sp1 );
		linestr.transscale( dx,dy,dz, fx,fy,fz, false );
		linestr.toJAG( JAG_C_COL_TYPE_LINESTRING, false, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING3D ) {
		JagLineString3D linestr;
		JagParser::addLineString3DData( linestr, sp1 );
		linestr.transscale( dx,dy,dz, fx,fy,fz, true );
		linestr.toJAG( JAG_C_COL_TYPE_LINESTRING3D, true, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOINT ) {
		JagLineString3D linestr;
		JagParser::addLineStringData( linestr, sp1 );
		linestr.transscale( dx,dy,dz, fx,fy,fz, false );
		linestr.toJAG( JAG_C_COL_TYPE_MULTIPOINT, false, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOINT3D ) {
		JagLineString3D linestr;
		JagParser::addLineString3DData( linestr, sp1 );
		linestr.transscale( dx,dy,dz, fx,fy,fz, true );
		linestr.toJAG( JAG_C_COL_TYPE_MULTIPOINT3D, true, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON ) {
		JagPolygon pgon;
		int rc = JagParser::addPolygonData( pgon, sp1, false );
		if ( rc < 0 ) return false;
		pgon.transscale( dx,dy,dz, fx,fy,fz, false );
		pgon.toJAG( false, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON3D ) {
		JagPolygon pgon;
		int rc = JagParser::addPolygon3DData( pgon, sp1, false );
		if ( rc < 0 ) return false;
		pgon.transscale( dx,dy,dz, fx,fy,fz, true );
		pgon.toJAG( true, true, sp1[1], srid, val );
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		JagVector<JagPolygon> pgvec;
        int n = JagParser::addMultiPolygonData( pgvec, sp1, false, false );
        if ( n <= 0 ) return false;
		for ( int i=0; i < pgvec.size(); ++i ) { pgvec[i].transscale( dx,dy,dz, fx,fy,fz, false ); }
		bool rc = JagGeo::toJAG( pgvec, false, true, sp1[1], srid, val );
		if ( ! rc ) return false; 
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
		JagVector<JagPolygon> pgvec;
        int n = JagParser::addMultiPolygonData( pgvec, sp1, false, true );
        if ( n <= 0 ) return false;
		for ( int i=0; i < pgvec.size(); ++i ) { pgvec[i].transscale( dx,dy,dz, fx,fy,fz, true ); }
		bool rc = JagGeo::toJAG( pgvec, true, true, sp1[1], srid, val );
		if ( ! rc ) return false; 
	} else {
		return false;
	}

	return true;
}

bool BinaryOpNode::doAllIntersect( const Jstr& mark1, const Jstr &colType1, 
										int srid1, const JagStrSplit &sp1, const Jstr& mark2, 
										const Jstr &colType2, int srid2, const JagStrSplit &sp2, bool strict )
{
    dn("s4200901 doAllIntersect mark1=[%s] mark2=[%s] colType1=[%s] colType2=[%s]", mark1.s(), mark2.s(), colType1.s(), colType2.s() );

    if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
        dn("s3330301018 colType1 == JAG_C_COL_TYPE_MULTIPOLYGON3D");
    } else {
        dn("s3330301018 colType1 !== JAG_C_COL_TYPE_MULTIPOLYGON3D");
    }

	if ( colType1 == JAG_C_COL_TYPE_POINT ) {
		return JagGeo::doPointIntersect(  srid1, sp1, mark2, colType2, srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_POINT3D ) {
		return JagGeo::doPoint3DIntersect(  srid1, sp1, mark2, colType2, srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_CIRCLE ) {
		return JagGeo::doCircleIntersect(  srid1, sp1, mark2, colType2, srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_CIRCLE3D ) {
		return JagGeo::doCircle3DIntersect(  srid1, sp1, mark2, colType2, srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_SPHERE ) {
		return JagGeo::doSphereIntersect(  srid1, sp1, mark2, colType2, srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_SQUARE ) {
		return JagGeo::doSquareIntersect(  srid1, sp1, mark2, colType2, srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_SQUARE3D ) {
		return JagGeo::doSquare3DIntersect(  srid1, sp1, mark2, colType2, srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_CUBE ) {
		return JagGeo::doCubeIntersect(  srid1, sp1, mark2, colType2, srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_RECTANGLE ) {
		return JagGeo::doRectangleIntersect(  srid1, sp1, mark2, colType2, srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_RECTANGLE3D ) {
		return JagGeo::doRectangle3DIntersect(  srid1, sp1, mark2, colType2, srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_BOX ) {
		return JagGeo::doBoxIntersect(  srid1, sp1, mark2, colType2, srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_TRIANGLE ) {
		return JagGeo::doTriangleIntersect(  srid1, sp1, mark2, colType2, srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_TRIANGLE3D ) {
		return JagGeo::doTriangle3DIntersect(  srid1, sp1, mark2, colType2, srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_CYLINDER ) {
		return JagGeo::doCylinderIntersect(  srid1, sp1, mark2, colType2, srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_CONE ) {
		return JagGeo::doConeIntersect(  srid1, sp1, mark2, colType2, srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_ELLIPSE ) {
		return JagGeo::doEllipseIntersect(  srid1, sp1, mark2, colType2, srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_ELLIPSOID ) {
		return JagGeo::doEllipsoidIntersect(  srid1, sp1, mark2, colType2, srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_LINE ) {
		return JagGeo::doLineIntersect(  srid1, sp1, mark2, colType2, srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_LINE3D ) {
		return JagGeo::doLine3DIntersect(  srid1, sp1, mark2, colType2, srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING ) {
		return JagGeo::doLineStringIntersect( mark1, srid1, sp1, mark2, colType2, srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_LINESTRING3D ) {
		return JagGeo::doLineString3DIntersect( mark1, srid1, sp1, mark2, colType2, srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON ) {
		return JagGeo::doPolygonIntersect( mark1, srid1, sp1, mark2, colType2, srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_POLYGON3D ) {
        dn("s2828339 JAG_C_COL_TYPE_POLYGON3D ");
		return JagGeo::doPolygon3DIntersect( mark1, srid1, sp1, mark2, colType2, srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_MULTILINESTRING ) {
		return JagGeo::doPolygonIntersect( mark1, srid1, sp1, mark2, colType2, srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
		return JagGeo::doPolygon3DIntersect( mark1, srid1, sp1, mark2, colType2, srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		return JagGeo::doMultiPolygonIntersect( mark1, srid1, sp1, mark2, colType2, srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
        dn("s3608277 JAG_C_COL_TYPE_MULTIPOLYGON3D doMultiPolygon3DIntersect...");
		return JagGeo::doMultiPolygon3DIntersect( mark1, srid1, sp1, mark2, colType2, srid2, sp2, strict);
	} else if ( colType1 == JAG_C_COL_TYPE_RANGE
	             || colType1 == JAG_C_COL_TYPE_DOUBLE 
	             || colType1 == JAG_C_COL_TYPE_LONGDOUBLE 
	             || colType1 == JAG_C_COL_TYPE_FLOAT
	             || isDateTime( colType1 )
	             || colType1 == JAG_C_COL_TYPE_DINT
	             || colType1 == JAG_C_COL_TYPE_DBIGINT
	             || colType1 == JAG_C_COL_TYPE_DSMALLINT
	             || colType1 == JAG_C_COL_TYPE_DTINYINT
	             || colType1 == JAG_C_COL_TYPE_DMEDINT
			  ) {
		return JagRange::doRangeIntersect( _jpa, mark1, colType1, srid1, sp1, mark2, colType2, srid2, sp2 );

	} else if ( colType2 == JAG_C_COL_TYPE_POINT ) {
		return JagGeo::doPointIntersect(  srid2, sp2, mark1, colType1, srid1, sp1, strict);
	} else if ( colType2 == JAG_C_COL_TYPE_POINT3D ) {
		return JagGeo::doPoint3DIntersect(  srid2, sp2, mark1, colType1, srid1, sp1, strict);
	} else if ( colType2 == JAG_C_COL_TYPE_CIRCLE ) {
		return JagGeo::doCircleIntersect(  srid2, sp2, mark1, colType1, srid1, sp1, strict);
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE ) {
		return JagGeo::doSquareIntersect(  srid2, sp2, mark1, colType1, srid1, sp1, strict);
	} else if ( colType2 == JAG_C_COL_TYPE_CIRCLE3D ) {
		return JagGeo::doCircle3DIntersect(  srid2, sp2, mark1, colType1, srid1, sp1, strict);
	} else if ( colType2 == JAG_C_COL_TYPE_SPHERE ) {
		return JagGeo::doSphereIntersect(  srid2, sp2, mark1, colType1, srid1, sp1, strict);
	} else if ( colType2 == JAG_C_COL_TYPE_SQUARE3D ) {
		return JagGeo::doSquare3DIntersect(  srid2, sp2, mark1, colType1, srid1, sp1, strict);
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE ) {
		return JagGeo::doRectangleIntersect(  srid2, sp2, mark1, colType1, srid1, sp1, strict);
	} else if ( colType2 == JAG_C_COL_TYPE_RECTANGLE3D ) {
		return JagGeo::doRectangle3DIntersect(  srid2, sp2, mark1, colType1, srid1, sp1, strict);
	} else if ( colType2 == JAG_C_COL_TYPE_BOX ) {
		return JagGeo::doBoxIntersect(  srid2, sp2, mark1, colType1, srid1, sp1, strict);
	} else if ( colType2 == JAG_C_COL_TYPE_TRIANGLE3D ) {
		return JagGeo::doTriangle3DIntersect(  srid2, sp2, mark1, colType1, srid1, sp1, strict);
	} else if ( colType2 == JAG_C_COL_TYPE_TRIANGLE ) {
		return JagGeo::doTriangleIntersect(  srid2, sp2, mark1, colType1, srid1, sp1, strict);
	} else if ( colType2 == JAG_C_COL_TYPE_CONE ) {
		return JagGeo::doConeIntersect(  srid2, sp2, mark1, colType1, srid1, sp1, strict);
	} else if ( colType2 == JAG_C_COL_TYPE_CYLINDER ) {
		return JagGeo::doCylinderIntersect(  srid2, sp2, mark1, colType1, srid1, sp1, strict);
	} else if ( colType2 == JAG_C_COL_TYPE_RANGE
	             || colType2 == JAG_C_COL_TYPE_DOUBLE 
	             || colType2 == JAG_C_COL_TYPE_LONGDOUBLE 
	             || colType2 == JAG_C_COL_TYPE_FLOAT
	             || isDateTime( colType2 )
	             || colType2 == JAG_C_COL_TYPE_DINT
	             || colType2 == JAG_C_COL_TYPE_DBIGINT
	             || colType2 == JAG_C_COL_TYPE_DSMALLINT
	             || colType2 == JAG_C_COL_TYPE_DTINYINT
	             || colType2 == JAG_C_COL_TYPE_DMEDINT
			  ) {
		return JagRange::doRangeIntersect( _jpa, mark2, colType2, srid2, sp2, mark1, colType1, srid1, sp1 );
	} else if ( colType2 == JAG_C_COL_TYPE_CUBE ) {
        dn("s838383300 colType2 == JAG_C_COL_TYPE_CUBE");
		return JagGeo::doCubeIntersect(  srid2, sp2, mark1, colType1, srid1, sp1, strict);
	} else if ( colType2 == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
		return JagGeo::doMultiPolygon3DIntersect( mark2, srid2, sp2, mark1, colType1, srid1, sp1, strict);
	} else if ( colType2 == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		return JagGeo::doMultiPolygonIntersect( mark2, srid2, sp2, mark1, colType1, srid1, sp1, strict);
	} else if ( colType2 == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
		return JagGeo::doPolygon3DIntersect( mark2, srid2, sp2, mark1, colType1, srid1, sp1, strict);
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSOID ) {
		return JagGeo::doEllipsoidIntersect(  srid2, sp2, mark1, colType1, srid1, sp1, strict);
	} else if ( colType2 == JAG_C_COL_TYPE_ELLIPSE ) {
		return JagGeo::doEllipseIntersect(  srid2, sp2, mark1, colType1, srid1, sp1, strict);
	} else if ( colType2 == JAG_C_COL_TYPE_LINE3D ) {
		return JagGeo::doLine3DIntersect(  srid2, sp2, mark1, colType1, srid1, sp1, strict);
	} else if ( colType2 == JAG_C_COL_TYPE_LINE ) {
		return JagGeo::doLineIntersect(  srid2, sp2, mark1, colType1, srid1, sp1, strict);
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING ) {
		return JagGeo::doLineStringIntersect( mark2, srid2, sp2, mark1, colType1, srid1, sp1, strict);
	} else if ( colType2 == JAG_C_COL_TYPE_LINESTRING3D ) {
		return JagGeo::doLineString3DIntersect( mark2, srid2, sp2, mark1, colType1, srid1, sp1, strict);
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON ) {
		return JagGeo::doPolygonIntersect( mark2, srid2, sp2, mark1, colType1, srid1, sp1, strict);
	} else if ( colType2 == JAG_C_COL_TYPE_POLYGON3D ) {
        dn("s2828333 JAG_C_COL_TYPE_POLYGON3D ");
		return JagGeo::doPolygon3DIntersect( mark2, srid2, sp2, mark1, colType1, srid1, sp1, strict);
	} else if ( colType2 == JAG_C_COL_TYPE_MULTILINESTRING ) {
		return JagGeo::doPolygonIntersect( mark2, srid2, sp2, mark1, colType1, srid1, sp1, strict);
	} else {
        dn("s92200");
		throw 2345;
	}

	return false;
}


bool BinaryOpNode::doAllNearby( const Jstr& mark1, const Jstr &colType1, int srid1, const JagStrSplit &sp1,
     								   const Jstr& mark2, const Jstr &colType2, int srid2, const JagStrSplit &sp2,
									   const Jstr &carg )
{
	bool rc = JagGeo::doAllNearby( mark1, colType1, srid1, sp1, mark2, colType2, srid2, sp2, carg );
	return rc;
}

#endif
// JAG_SERVER_SIDE



