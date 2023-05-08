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

#include <malloc.h>
#include <JagTable.h>
#include <JagIndex.h>
#include <JaguarCPPClient.h>
#include <JagDBServer.h>
#include <JagUtil.h>
#include <JagUUID.h>
#include <JagIndexString.h>
#include <JagDiskArrayServer.h>
#include <JagSession.h>
#include <JagFastCompress.h>
#include <JagDataAggregate.h>
#include <JagDBConnector.h>
#include <JagBuffBackReader.h>
#include <JagServerObjectLock.h>
#include <JagStrSplitWithQuote.h>
#include <JagParser.h>
#include <JagLineFile.h>
#include <JagPriorityQueue.h>
#include <JagMath.h>

JagTable::JagTable( int replicType, const JagDBServer *servobj, const Jstr &dbname, const Jstr &tableName, 
					  const JagSchemaRecord &record, bool buildInitIndex ) 
		: _tableRecord(record), _servobj( servobj )
{
	d("s111022 JagTable ctor ...\n");
	_cfg = _servobj->_cfg;
	_objectLock = servobj->_objectLock;
	_dbname = dbname;
	_tableName = tableName;
	_dbtable = dbname + "." + tableName;
	_darrFamily = NULL;
	_tablemap = NULL;
	_schAttr = NULL;
	_tableschema = NULL;
	_indexschema = NULL;
	_numKeys = 0;
	_numCols = 0;
	_replicType = replicType;
	_isExporting = 0;
	_KEYLEN = 0;
	_VALLEN = 0;
	_KEYVALLEN = 0;
	init( buildInitIndex );
}

JagTable::~JagTable ()
{
	if ( _darrFamily ) {
		delete _darrFamily;
		_darrFamily = NULL;
	}
	
	if ( _tablemap ) {
		delete _tablemap;
		_tablemap = NULL;
	}	

	if ( _schAttr ) {
		delete [] _schAttr;
		_schAttr = NULL;
	}

	pthread_mutex_destroy( &_parseParamParentMutex );

}

void JagTable::init( bool buildInitIndex ) 
{
	if ( NULL != _darrFamily ) { 
		d("s111023 JagTable init() _darrFamily not NULL, return\n");
		return; 
	}

	d("s111023 JagTable init() _darrFamily is NULL, .....\n");
	_cfg = _servobj->_cfg;
	
	Jstr dbpath, fpath, dbcolumn;
	Jstr rdbdatahome = _cfg->getJDBDataHOME( _replicType );
	dbpath = rdbdatahome + "/" + _dbname;

	fpath = dbpath +  "/" + _tableName + "/" + _tableName;
	JagFileMgr::makedirPath( dbpath +  "/" + _tableName );
	JagFileMgr::makedirPath( dbpath +  "/" + _tableName + "/files" );

	_darrFamily = new JagDiskArrayFamily (JAG_TABLE, _servobj, fpath, &_tableRecord, 0, buildInitIndex );
	d("s210822 jagtable init() new _darrFamily\n");

	_KEYLEN = _tableRecord.keyLength;
	_VALLEN = _tableRecord.valueLength;
	_KEYVALLEN = _KEYLEN + _VALLEN;
	_numCols = _tableRecord.columnVector->size();	
	_schAttr = new JagSchemaAttribute[_numCols];	
	_schAttr[0].record = _tableRecord;
	_tablemap = newObject<JagHashStrInt>();

	if ( 0 == _replicType ) {
		_tableschema = _servobj->_tableschema;
		_indexschema = _servobj->_indexschema;
	} else if ( 1 == _replicType ) {
		_tableschema = _servobj->_prevtableschema;
		_indexschema = _servobj->_previndexschema;
	} else if ( 2 == _replicType ) {
		_tableschema = _servobj->_nexttableschema;
		_indexschema = _servobj->_nextindexschema;
	} else {
		_tableschema = _servobj->_tableschema;
		_indexschema = _servobj->_indexschema;
	}

	_objectType = _tableschema->objectType( _dbtable );
	setupSchemaMapAttr( _numCols );

	pthread_mutex_init( &_parseParamParentMutex, NULL );

	_counterOffset = -1;
	_counterLength = 0;
	bool iskey;
	const auto &cv = *(_tableRecord.columnVector);
	int totCols = cv.size();

	for ( int i = 0; i < totCols; i ++ ) {

		iskey = cv[i].iskey;
		if ( iskey ) continue;  // "counter" field is not in keys
		if ( 0 == strcmp( cv[i].name.s(), "counter" ) ) {
			_counterOffset = cv[i].offset;
			_counterLength = cv[i].length;
			break;
		}
	}

}

void JagTable::buildInitIndexlist()
{
	int 		rc; 
	Jstr 		dbtabindex, dbindex; 
	const 		JagSchemaRecord *irecord;
	int 		lockrc;
	JagVector<Jstr> vec;

    dn("s61120 JagTable::buildInitIndexlist() getIndexNamesFromMem ...");
	rc = _indexschema->getIndexNamesFromMem( _dbname, _tableName, vec );
    dn("s61120 JagTable::buildInitIndexlist() getIndexNamesFromMem done. Found %d items rc=%d", vec.size(), rc );

    if ( rc < 1 ) {
        dn("s020277 found 0 items from mem, call getIndexNames..."); 
	    rc = _indexschema->getIndexNames( _dbname, _tableName, vec );
        dn("s020297 found %d items (vec.size=%d)", rc, vec.size() ); 
    }

    for ( int i = 0; i < vec.length(); ++i ) {
		dbtabindex = _dbtable + "." + vec[i];
		dbindex = _dbname + "." + vec[i];
        dn("s30447 vec[i=%d]=[%s] dbtabindex=[%s] dbindex=[%s]", i, vec[i].s(), dbtabindex.s(), dbindex.s() );

		AbaxBuffer bfr;
		AbaxPair<AbaxString, AbaxBuffer> pair( AbaxString(dbindex), bfr );
        dn("s77220 getAttr(%s) ...", dbtabindex.s() );
		irecord = _indexschema->getAttr( dbtabindex );
        dn("s77220 getAttr(%s) got irecord=%p", dbtabindex.s(), irecord );
		if ( irecord ) {
			if ( _objectLock->writeLockIndex( JAG_CREATEINDEX_OP, _dbname, _tableName, vec[i],
										      _tableschema, _indexschema, _replicType, 1, lockrc ) ) {
				_objectLock->writeUnlockIndex( JAG_CREATEINDEX_OP, _dbname, _tableName, vec[i], _replicType, 1 );
			}		
			_indexlist.append( vec[i] );
		} 
    }

}

bool JagTable::getPair( JagDBPair &pair ) 
{ 
	if ( _darrFamily->get( pair ) ) return true;
	else return false;
}

// 1: OK  0: error
int JagTable::parsePair( int tzdiff, JagParseParam *parseParam, JagVector<JagDBPair> &retpair, Jstr &errmsg ) const
{
	int getpos = 0;
	int metrics;
	int rc, rc2;
	
	Jstr dbcolumn;
    Jstr dbtab = parseParam->objectVec[0].dbName + "." + parseParam->objectVec[0].tableName;
	if ( _numCols < parseParam->valueVec.size() ) {
		d("s4893 Error parsePair _numCols=%d parseParam->valueVec.size()=%d\n", _numCols, parseParam->valueVec.size() );
		errmsg = "E3143  _numCols-1 != parseParam->valueVec.size";
		return 0;
	}

	// debug
	#ifdef DEBUG_TABLE 
	const auto &cv = *_tableRecord.columnVector;
    dn("s03938031 _tableRecord.columnVector.size=%d", cv.size() );
	for ( int i = 0; i < cv.size(); ++i ) {
		d("s1014 colvec i=%d name=[%s] type=[%s] issubcol=%d offset=%d len=%d\n", i, cv[i].name.s(), cv[i].type.s(), cv[i].issubcol, cv[i].offset, cv[i].length );
	}
	d("\n" );

	d("s2091 initial parseParam->valueVec: size=%d\n", parseParam->valueVec.size() );
	for ( int i = 0; i < parseParam->valueVec.size(); ++i ) {
		d("s1015  i=%d valueVec name=[%s] value=[%s] type=[%s] point.x=[%s] point.y=[%s] line.point[0].x=[%s] line.point[0].y=[%s]\n", 
			i, parseParam->valueVec[i].objName.colName.s(), parseParam->valueVec[i].valueData.s(), 
			parseParam->valueVec[i].type.s(), parseParam->valueVec[i].point.x, parseParam->valueVec[i].point.y,
			parseParam->valueVec[i].linestr.point[0].x, parseParam->valueVec[i].linestr.point[0].y );
			// i, parseParam->valueVec[i].objName.colName.s(), parseParam->valueVec[i].valueData.s(), _schAttr[i].type.s() ));
	}
	d("\n" );

	// add extra blanks in  parseParam->valueVec if it has fewer items than _tableRecord.columnVector
	d("s8613 columnVector.size=%d valueVec.size=%d\n", cv.size(), parseParam->valueVec.size() );
	d("s8614 valueVec: print\n" );
	for ( int k = 0; k < parseParam->valueVec.size(); ++k ) {
		d("s8614 k=%d other valuedata=[%s] issubcol=%d linestr.size=%d \n",
			k, parseParam->valueVec[k].valueData.s(), parseParam->valueVec[k].issubcol, parseParam->valueVec[k].linestr.size());
	}
	#endif

	// find bbx coords
	Jstr otype;

    #ifdef  JAG_KEEP_MIN_MAX
	double xmin, ymin, xmax, ymax, zmin, zmax;
    // min max for all columns
	xmin=JAG_LONG_MAX; ymin=JAG_LONG_MAX;
    zmin=JAG_LONG_MAX; xmax=JAG_LONG_MIN;
    ymax=JAG_LONG_MIN; zmax=JAG_LONG_MIN;

    bool isBoundBox3D = false;
  	dbcolumn = dbtab + ".geo:zmin"; 
    rc = _tablemap->getValue(dbcolumn, getpos);
    if ( rc ) {
        isBoundBox3D = true;
    }

	for ( int i = 0; i < parseParam->valueVec.size(); ++i ) {
		rc = 0;
		otype = parseParam->valueVec[i].type;
		dn("s4928 parseParam->valueVec[%d].type=[%s] valueData=[%s]\n", 
           i, parseParam->valueVec[i].type.s(), parseParam->valueVec[i].valueData.s() );

		if ( otype.size() > 0 ) {

    		if ( otype  == JAG_C_COL_TYPE_LINESTRING || otype == JAG_C_COL_TYPE_MULTIPOINT  ) {
                dn("s456028 JAG_C_COL_TYPE_LINESTRING JAG_C_COL_TYPE_MULTIPOINT value=[%s]", parseParam->valueVec[i].valueData.s() );
    			rc = JagParser::getLineStringMinMax( ',', parseParam->valueVec[i].valueData.s(), xmin, ymin, xmax, ymax );
                if ( ! isBoundBox3D ) { zmin = zmax = 0; }
                dn("s37335013 linestr  getLineStringMinMax returns i=%d  xmin=%f xmax=%f    ymin=%f ymax=%f", i, xmin, xmax, ymin, ymax );
    		} else if ( otype == JAG_C_COL_TYPE_LINESTRING3D || otype == JAG_C_COL_TYPE_MULTIPOINT3D ) {
    			rc = JagParser::getLineString3DMinMax(',', parseParam->valueVec[i].valueData.s(), xmin, ymin, zmin, xmax, ymax, zmax );
    		} else if ( otype == JAG_C_COL_TYPE_POLYGON || otype == JAG_C_COL_TYPE_MULTILINESTRING ) {
    			rc = JagParser::getPolygonMinMax(  parseParam->valueVec[i].valueData.s(), xmin, ymin, xmax, ymax );
                if ( ! isBoundBox3D ) { zmin = zmax = 0; }
                dn("s37335013 linestr  getLineStringMinMax returns i=%d  xmin=%f xmax=%f    ymin=%f ymax=%f", i, xmin, xmax, ymin, ymax );
    		} else if ( otype == JAG_C_COL_TYPE_POLYGON3D || otype == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
    			rc = JagParser::getPolygon3DMinMax( parseParam->valueVec[i].valueData.s(), xmin, ymin, zmin, xmax, ymax, zmax );
    		} else if ( otype == JAG_C_COL_TYPE_MULTIPOLYGON ) {
    			rc = JagParser::getMultiPolygonMinMax(  parseParam->valueVec[i].valueData.s(), xmin, ymin, xmax, ymax );
                if ( ! isBoundBox3D ) { zmin = zmax = 0; }
    		} else if ( otype == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
    			rc = JagParser::getMultiPolygon3DMinMax(  parseParam->valueVec[i].valueData.s(), xmin, ymin, zmin, xmax, ymax, zmax );
    		}
		}

		if ( rc < 0 ) {
			char ebuf[64];
			sprintf( ebuf, "E3145 Error: data type=%s column i=%d\n", parseParam->valueVec[i].type.s(), i );
			errmsg = ebuf;
			return 0;
		}
	}
    #endif


	// just need to make up matching number of columns, data is not important
	if ( (*_tableRecord.columnVector).size() > parseParam->valueVec.size() ) {
		JagVector<ValueAttribute> valueVec;

		int j = -1;
		Jstr colType;
		int numMetrics;

        // make new valueVec 
		for ( int i = 0; i < parseParam->valueVec.size(); ++i ) {
			//dn("s6675 parseParam->hasPoly=%d _tableRecord.lastKeyColumn=%d parseParam->polyDim=%d",
				  //parseParam->hasPoly, _tableRecord.lastKeyColumn, parseParam->polyDim );

			numMetrics = parseParam->valueVec[i].point.metrics.size();

			if ( parseParam->hasPoly && j == _tableRecord.lastKeyColumn ) {
				if ( 2 == parseParam->polyDim ) {
					// appendOther( valueVec, 7, 0 );  // bbox(4) + id col i = 7  no min/max z-dim
					//appendOther( valueVec, JAG_POLY_HEADER_COLS, 0 );  // bbox(4) + id col i = 7  no min/max z-dim
                    #ifdef JAG_KEEP_MIN_MAX
					appendOther( valueVec, JAG_POLY_HEADER_COLS_2D, 0 );  // bbox(4) + id col i = 7  no min/max z-dim
                    #else
					appendOther( valueVec, JAG_POLY_HEADER_COLS_NOMINMAX, 0 );  // bbox(4) + id col i = 7  no min/max z-dim
                    #endif
					//d("s6760 i=%d j=%d appendOther( valueVec, JAG_POLY_HEADER_COLS, 0 );\n", i, j);
				} else {
					//appendOther( valueVec, JAG_POLY_HEADER_COLS, 0 );  // bbox(6) + id col i = 9
                    #ifdef JAG_KEEP_MIN_MAX
					appendOther( valueVec, JAG_POLY_HEADER_COLS_3D, 0 );  // bbox(6) + id col i = 9
                    #else
					appendOther( valueVec, JAG_POLY_HEADER_COLS_NOMINMAX, 0 );  // bbox(6) + id col i = 9
                    #endif
					//d("s6761 i=%d j=%d appendOther( valueVec, JAG_POLY_HEADER_COLS, 0 );\n", i, j);
				}
				//d("s6761 i=%d j=%d appendOther( valueVec, JAG_POLY_HEADER_COLS, 0 );\n", i, j);
			}

			colType = parseParam->valueVec[i].type;
			dn("s1110202 i=%d colType=[%s]\n", i, colType.s() );
			if ( colType.size() > 0 ) {
				// d("s6761 i=%d colType=[%s]\n", i, colType.s() );
    			if ( colType == JAG_C_COL_TYPE_POINT  ) {
    				valueVec.append(  parseParam->valueVec[i] );
    				appendOther(  valueVec, JAG_POINT_DIM );  // x y
    				j += JAG_POINT_DIM;
    			} else if ( colType == JAG_C_COL_TYPE_POINT3D) {
    				valueVec.append(  parseParam->valueVec[i] );
    				appendOther(  valueVec, JAG_POINT3D_DIM );
    				j += JAG_POINT3D_DIM;
    			} else if ( colType == JAG_C_COL_TYPE_CIRCLE ) {
    				valueVec.append(  parseParam->valueVec[i] );
    				appendOther(  valueVec, JAG_CIRCLE_DIM );  // x y r
    				j += JAG_CIRCLE_DIM;
    			} else if ( colType == JAG_C_COL_TYPE_SQUARE ) {
    				valueVec.append(  parseParam->valueVec[i] );
    				appendOther(  valueVec, JAG_SQUARE_DIM );  // x y r nx
    				j += JAG_SQUARE_DIM;
    			} else if ( colType == JAG_C_COL_TYPE_SPHERE ) {
    				valueVec.append(  parseParam->valueVec[i] );
    				appendOther(  valueVec, JAG_SPHERE_DIM );  // x y z r
    				j += JAG_SPHERE_DIM;
    				// x y z radius
    			} else if ( colType == JAG_C_COL_TYPE_CIRCLE3D
    						|| colType == JAG_C_COL_TYPE_SQUARE3D
    						|| colType == JAG_C_COL_TYPE_CUBE ) {
    				valueVec.append(  parseParam->valueVec[i] );
    				appendOther(  valueVec, JAG_CIRCLE3D_DIM );  // x y z r nx ny
    				j += JAG_CIRCLE3D_DIM;
    				// x y z radius
    			} else if (  colType == JAG_C_COL_TYPE_LINE ) {
    				valueVec.append(  parseParam->valueVec[i] );
    				appendOther(  valueVec, JAG_LINE_DIM );  // x1 y1  x2 y2 
    				j += JAG_LINE_DIM;
    			} else if ( colType == JAG_C_COL_TYPE_RECTANGLE 
    						|| colType == JAG_C_COL_TYPE_ELLIPSE ) {
    				valueVec.append(  parseParam->valueVec[i] );
    				appendOther(  valueVec, JAG_RECTANGLE_DIM );  // x y a b nx
    				j += JAG_RECTANGLE_DIM;
    			} else if ( colType == JAG_C_COL_TYPE_LINE3D ) {
    				valueVec.append(  parseParam->valueVec[i] );
    				appendOther(  valueVec, JAG_LINE3D_DIM );  // x1 y1 z1 x2 y2 z2
    				j += JAG_LINE3D_DIM;
    			} else if (  colType == JAG_C_COL_TYPE_VECTOR ) {
    				valueVec.append(  parseParam->valueVec[i] );
    				appendOther(  valueVec, JAG_VECTOR_DIM );  // i x
    				j += JAG_VECTOR_DIM;
    			} else if (  colType == JAG_C_COL_TYPE_LINESTRING ) {
    				valueVec.append(  parseParam->valueVec[i] );
    				appendOther(  valueVec, JAG_LINESTRING_DIM );  // i x1 y1
    				j += JAG_LINESTRING_DIM;
    			} else if (  colType == JAG_C_COL_TYPE_LINESTRING3D ) {
    				valueVec.append(  parseParam->valueVec[i] );
    				appendOther(  valueVec, JAG_LINESTRING3D_DIM );  // i x1 y1 z1
    				j += JAG_LINESTRING3D_DIM;
    			} else if (  colType == JAG_C_COL_TYPE_MULTIPOINT ) {
    				valueVec.append(  parseParam->valueVec[i] );
    				appendOther(  valueVec, JAG_MULTIPOINT_DIM );  // i x1 y1
    				j += JAG_MULTIPOINT_DIM;
    			} else if (  colType == JAG_C_COL_TYPE_MULTIPOINT3D ) {
    				valueVec.append(  parseParam->valueVec[i] );
    				appendOther(  valueVec, JAG_MULTIPOINT3D_DIM );  // i x1 y1 z1
    				j += JAG_MULTIPOINT3D_DIM;
    			} else if (  colType == JAG_C_COL_TYPE_POLYGON || colType == JAG_C_COL_TYPE_MULTIPOLYGON ) {
    				valueVec.append(  parseParam->valueVec[i] );
    				appendOther(  valueVec, JAG_POLYGON_DIM );  // i x1 y1
    				j += JAG_POLYGON_DIM;
    			} else if (  colType == JAG_C_COL_TYPE_POLYGON3D || colType == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
    				valueVec.append(  parseParam->valueVec[i] );
    				appendOther(  valueVec, JAG_POLYGON3D_DIM );  // i x1 y1
    				j += JAG_POLYGON3D_DIM;
    			} else if (  colType == JAG_C_COL_TYPE_MULTILINESTRING ) {
    				valueVec.append(  parseParam->valueVec[i] );
    				appendOther(  valueVec, JAG_MULTILINESTRING_DIM );  // i x1 y1
    				j += JAG_MULTILINESTRING_DIM;
    			} else if (  colType == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
    				valueVec.append(  parseParam->valueVec[i] );
    				appendOther(  valueVec, JAG_MULTILINESTRING3D_DIM );  // i x1 y1
    				j += JAG_MULTILINESTRING3D_DIM;
    			} else if ( colType == JAG_C_COL_TYPE_TRIANGLE ) {
    				valueVec.append(  parseParam->valueVec[i] );
    				appendOther(  valueVec, JAG_TRIANGLE_DIM );  // x1 y1 x2 y2 x3 y3 
    				j += JAG_TRIANGLE_DIM;
    			} else if ( colType == JAG_C_COL_TYPE_ELLIPSOID ) {
    				valueVec.append(  parseParam->valueVec[i] );
    				appendOther(  valueVec, JAG_ELLIPSOID_DIM );  // x y z width depth height nx ny
    				j += JAG_ELLIPSOID_DIM;
    			} else if (  colType == JAG_C_COL_TYPE_BOX ) {
    				valueVec.append(  parseParam->valueVec[i] );
    				appendOther(  valueVec, JAG_BOX_DIM );  // x y z width depth height nx ny
    				j += JAG_BOX_DIM;
    			} else if ( colType == JAG_C_COL_TYPE_CYLINDER
    			            || colType == JAG_C_COL_TYPE_CONE ) {
    				valueVec.append(  parseParam->valueVec[i] );
    				appendOther(  valueVec, JAG_CONE_DIM );  // x y z r height  nx ny
    				j += JAG_CONE_DIM;
    			} else if ( colType == JAG_C_COL_TYPE_TRIANGLE3D ) {
    				valueVec.append(  parseParam->valueVec[i] );
    				appendOther(  valueVec, JAG_TRIANGLE3D_DIM );  //x1y1z1 x2y2z2 x3y3z3
    				j += JAG_TRIANGLE3D_DIM;
    			} else if ( colType == JAG_C_COL_TYPE_ELLIPSE3D || colType == JAG_C_COL_TYPE_RECTANGLE3D ) {
    				valueVec.append(  parseParam->valueVec[i] );
    				appendOther(  valueVec, JAG_ELLIPSE3D_DIM );  // x y z a b nx ny
    				j += JAG_ELLIPSE3D_DIM;
    			} else if ( colType == JAG_C_COL_TYPE_RANGE ) {
    				valueVec.append(  parseParam->valueVec[i] );
    				appendOther(  valueVec, 2 );  // begin end
    				j += 2;
    			} else {
					//d("s202008 here  valueVec.append \n" );
    				valueVec.append(  parseParam->valueVec[i] );
    				// valueVec.append(  parseParam->valueVec[j] );
    			}

    			appendOther(  valueVec, numMetrics );  
    			j += numMetrics; 
			} else {
				dn("s20293 notype valueVec.append( i=%d )", i );
    			valueVec.append(  parseParam->valueVec[i] );
			}

			++j;
		}

		// d("s5022 parseParam->hasPoly=%d _tableRecord.lastKeyColumn=%d\n", parseParam->hasPoly, _tableRecord.lastKeyColumn );
        // replaced parseParam->valueVec with new extended valueVec
		parseParam->valueVec = valueVec;
	}

	// debug only
	#ifdef DEBUG_TABLE 
	d("\ns2093 final parseParam->valueVec:\n" );
	for ( int i = 0; i < parseParam->valueVec.size(); ++i ) {
		d("s1015  i=%d final valueVec name=[%s] value=[%s] type=[%s] point.x=[%s] point.y=[%s]\n", 
			i, parseParam->valueVec[i].objName.colName.s(), parseParam->valueVec[i].valueData.s(), 
			parseParam->valueVec[i].type.s(), parseParam->valueVec[i].point.x, parseParam->valueVec[i].point.y );

		d("s1015  i=%d line.point[0].x=[%s] line.point[0].y=[%s]\n", 
			  i, parseParam->valueVec[i].linestr.point[0].x, parseParam->valueVec[i].linestr.point[0].y );
		d("s1015  i=%d line.point[1].x=[%s] line.point[1].y=[%s]\n", 
			  i, parseParam->valueVec[i].linestr.point[1].x, parseParam->valueVec[i].linestr.point[1].y );
	}
	d("\n" );

	d("s8623 final columnVector.size=%d valueVec.size=%d\n", (*_tableRecord.columnVector).size(), parseParam->valueVec.size() );
	d("s8624 valueVec: print\n" );
	for ( int k = 0; k < parseParam->valueVec.size(); ++k ) {
		d("s8624 k=%d other valuedata=[%s] issubcol=%d linestr.size=%d\n",
			k, parseParam->valueVec[k].valueData.s(), parseParam->valueVec[k].issubcol, parseParam->valueVec[k].linestr.size() );
	}
	#endif


	Jstr point, pointi, pointx, pointy, pointz, pointr, pointw, pointd, pointh, colname;
	Jstr pointx1, pointy1, pointz1;
	Jstr pointx2, pointy2, pointz2;
	Jstr pointx3, pointy3, pointz3;
	Jstr pointnx, pointny, colType;
	bool is3D = false, hasDoneAppend = false;
	int  mlineIndex = 0;
	int  getxmin, getymin, getzmin, getxmax , getymax , getzmax;
	int  getid, getcol, getm, getn, geti, getx, gety, getz;

	char *tablekvbuf = (char*)jagmalloc(_KEYVALLEN+1);
	memset(tablekvbuf, 0, _KEYVALLEN+1);

    dn("s77030031 _KEYVALLEN=%d", _KEYVALLEN );

	int srvtmdiff  = _servobj->servtimediff;

    /**
	Jstr lsuuid = _servobj->_jagUUID->getStringAt( _servobj->getHostCluster() );
    dn("s03002292 lsuuid=[%s]", lsuuid.s() );
    **/
	Jstr lsuuid;

	const JagVector<JagColumn> &columnVector = *(_tableRecord.columnVector);

    // insert non-geo columns to tablebuf
	for ( int i = 0; i < parseParam->valueVec.size(); ++i ) {
		colname = columnVector[i].name.s();
		dbcolumn = dbtab + "." + colname;
		rc = _tablemap->getValue(dbcolumn, getpos);
		if ( ! rc ) {
			free( tablekvbuf );
			errmsg = Jstr("E15003  _tablemap->getValue(") + dbcolumn + ") error i=" + intToStr(i);
			d("s20281 i=%d  dbtab=[%s] colname=[%s] issubcol=false\n", i, dbtab.s(), colname.s() );  
			return 0;
		}

        assert( i == getpos );

		//d("s5031 i=%d dbcolumn=[%s] getpos=%d\n", i, dbcolumn.s(), getpos );

		ValueAttribute &otherAttr = parseParam->valueVec[i];

		if ( columnVector[i].spare[1] == JAG_C_COL_TYPE_ENUM[0] ) {
			if ( otherAttr.valueData.size() > 0 ) {
    			rc2 = false;
    			d("i=%d is enum getpos=%d vData=[%s] listlen=%d\n", i, getpos, otherAttr.valueData.s(), _schAttr[getpos].enumList.length() );
    			for ( int j = 0; j < _schAttr[getpos].enumList.length(); ++j ) {
    				//d("s331109 oompare  enum=[%s]  valueData=[%s]\n", _schAttr[getpos].enumList[j].s(), otherAttr.valueData.s() );
    				if ( strcmp( _schAttr[getpos].enumList[j].s(), otherAttr.valueData.s() ) == 0 ) {
    					rc2 = true;
    					break;
    				}
    			}
    			//d("s22229 rc2=%d\n", rc2 );
    			if ( !rc2 ) {
    				free( tablekvbuf );
    				errmsg = Jstr("E12036 Error: invalid ENUM value [") + otherAttr.valueData + "]";
    				return 0;
    			}
			} else {
			}
		}

		if ( columnVector[i].spare[4] == JAG_CREATE_DEFINSERTVALUE 
			 && otherAttr.valueData.size() < 1 ) {
			//d("s33022 JAG_CREATE_DEFINSERTVALUE i=%d\n", i);
			otherAttr.valueData = _schAttr[getpos].defValue;
			//d("s33022 JAG_CREATE_DEFINSERTVALUE otherAttr.valueData=[%s]\n", otherAttr.valueData.s() );
		} else {
		}

		colType = columnVector[i].type;
		metrics = columnVector[i].metrics;

        if ( JagParser::isGeoType(colType) ) {
            continue;
        } 

    	if ( otherAttr.valueData.size() < 1 ) {
			d("s404408 otherAttr.valueData.size is 0\n" );
        } else if ( columnVector[i].issubcol ) {
			d("s404428 i=%d is subcol\n", i );
        } else if ( columnVector[i].length < 1 ) {
			d("s404428 i=%d length=0\n", i );
    	} else {
			d("s50016 formatOneCol col=%s getpos=%d offset=%d length=%d sig=%d\n", 
				columnVector[i].name.s(), getpos, _schAttr[getpos].offset, _schAttr[getpos].length,  _schAttr[getpos].sig );

   		    formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.valueData.s(), errmsg, 
   			 	          columnVector[i].name.s(), _schAttr[getpos].offset, 
   				          _schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
   		}
    }


    // look at geo type columns
	for ( int i = 0; i < parseParam->valueVec.size(); ++i ) {
		if ( parseParam->valueVec[i].issubcol ) { 
			d("s21003 i=%d parseParam->valueVec[i].issubcol is true name=[%s] value=[%s] skip\n", 
			  i, parseParam->valueVec[i].objName.colName.s(), parseParam->valueVec[i].valueData.s() );
			continue; 
		}


		hasDoneAppend = false;
		colname = columnVector[i].name.s();
		dbcolumn = dbtab + "." + colname;
        /***
		rc = _tablemap->getValue(dbcolumn, getpos);
		//d("s2239 _tablemap->getValue dbcolumn=[%s] rc=%d\n", dbcolumn.s(), rc );
		if ( ! rc ) {
			free( tablekvbuf );
			errmsg = Jstr("E15003  _tablemap->getValue(") + dbcolumn + ") error i=" + intToStr(i);
			//d("s233032 return 0\n");
			d("s20281 i=%d  dbtab=[%s] colname=[%s] issubcol=false\n", i, dbtab.s(), colname.s() );  
			return 0;
		}
        ***/

		//d("s5031 i=%d dbcolumn=[%s] getpos=%d\n", i, dbcolumn.s(), getpos );

		ValueAttribute &otherAttr = parseParam->valueVec[i];
        /**
		dn("s7801 otherAttr=%0x  i=%d otherAttr col=[%s]  valueData=[%s] linestr.size=%d", 
			&otherAttr, i, otherAttr.objName.colName.s(), otherAttr.valueData.s(), otherAttr.linestr.size() );
            **/

        /***
		if ( columnVector[i].spare[1] == JAG_C_COL_TYPE_ENUM[0] ) {
			if ( otherAttr.valueData.size() > 0 ) {
    			rc2 = false;
    			d("i=%d is enum getpos=%d vData=[%s] listlen=%d\n", i, getpos, otherAttr.valueData.s(), _schAttr[getpos].enumList.length() );
    			for ( int j = 0; j < _schAttr[getpos].enumList.length(); ++j ) {
    				//d("s331109 oompare  enum=[%s]  valueData=[%s]\n", _schAttr[getpos].enumList[j].s(), otherAttr.valueData.s() );
    				if ( strcmp( _schAttr[getpos].enumList[j].s(), otherAttr.valueData.s() ) == 0 ) {
    					rc2 = true;
    					break;
    				}
    			}
    			//d("s22229 rc2=%d\n", rc2 );
    			if ( !rc2 ) {
    				free( tablekvbuf );
    				errmsg = Jstr("E12036 Error: invalid ENUM value [") + otherAttr.valueData + "]";
    				return 0;
    			}
			} else {
			}
		}
        ***/

		//d("s4220228 i=%d spare+4=[%c]  othAt.vD.size=%d \n", i, columnVector[i].spare[4],  otherAttr.valueData.size() );
		if ( columnVector[i].spare[4] == JAG_CREATE_DEFINSERTVALUE 
			 && otherAttr.valueData.size() < 1 ) {
			//d("s33022 JAG_CREATE_DEFINSERTVALUE i=%d\n", i);
			otherAttr.valueData = _schAttr[getpos].defValue;
			//d("s33022 JAG_CREATE_DEFINSERTVALUE otherAttr.valueData=[%s]\n", otherAttr.valueData.s() );
		} else {
		}

		colType = columnVector[i].type;
		metrics = columnVector[i].metrics;
		//d("s521051 other i=%d dbcolumn=[%s] colType=[%s]\n", i, dbcolumn.s(), colType.s() );

		if ( colType.size() > 0 ) {
            
    		if ( colType == JAG_C_COL_TYPE_POINT ) {
    			pointx = colname + ":x"; pointy = colname + ":y";
    			dbcolumn = dbtab + "." + pointx;
    			rc = _tablemap->getValue(dbcolumn, getpos);
				//d("s50025 JAG_C_COL_TYPE_POINT dbcolumn=% getpos=%d rc=%d\n", dbcolumn.s(), getpos, rc );
    			if ( rc ) {
    				rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.x, errmsg, 
    			 						pointx.s(), _schAttr[getpos].offset, _schAttr[getpos].length, 
    									_schAttr[getpos].sig, _schAttr[getpos].type );
    
    				dbcolumn = dbtab + "." + pointy;
    				rc = _tablemap->getValue(dbcolumn, getpos);
					//d("s50026 JAG_C_COL_TYPE_POINT dbcolumn=% getpos=%d rc=%d\n", dbcolumn.s(), getpos, rc );
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.y, errmsg, 
    			 			pointy.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				formatMetricCols( tzdiff, srvtmdiff, dbtab, colname, metrics, otherAttr.point.metrics, tablekvbuf );
    			}
    		} else if ( colType == JAG_C_COL_TYPE_RANGE ) {
    			pointx = colname + ":begin"; pointy = colname + ":end";
    			dbcolumn = dbtab + "." + pointx;
    			//d("s9383 otherAttr.valueData=[%s]\n", otherAttr.valueData.s() );
    			char sep;
    			if ( strchr( otherAttr.valueData.s(), ',') ) {
    				sep = ',';
    			} else {
    				sep = ' ';
    			}
    			JagStrSplit sp(otherAttr.valueData, sep );
    
    			rc = _tablemap->getValue(dbcolumn, getpos);
    			if ( rc ) {
    				rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, sp[0].s(), errmsg, 
    			 						pointx.s(), _schAttr[getpos].offset, _schAttr[getpos].length, 
    									_schAttr[getpos].sig, _schAttr[getpos].type );
    
    				dbcolumn = dbtab + "." + pointy;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, sp[1].s(), errmsg, 
    			 			pointy.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    			}
    		} else if ( colType == JAG_C_COL_TYPE_POINT3D ) {
    			pointx = colname + ":x"; pointy = colname + ":y"; pointz = colname + ":z"; 
    
    			dbcolumn = dbtab + "." + pointx;
    			rc = _tablemap->getValue(dbcolumn, getpos);
    			if ( rc ) {
    				rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.x, errmsg, 
    			 		pointx.s(), _schAttr[getpos].offset, 
    					_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    
    				dbcolumn = dbtab + "." + pointy;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.y, errmsg, 
    			 			pointy.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				dbcolumn = dbtab + "." + pointz;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.z, errmsg, 
    			 			pointz.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				formatMetricCols( tzdiff, srvtmdiff, dbtab, colname, metrics, otherAttr.point.metrics, tablekvbuf );
    			}
    		} else if ( colType == JAG_C_COL_TYPE_CIRCLE || colType == JAG_C_COL_TYPE_SQUARE ) {
    			pointx = colname + ":x"; pointy = colname + ":y"; pointr = colname + ":a"; 
    			pointnx = colname + ":nx"; 
    
    			dbcolumn = dbtab + "." + pointx;
    			rc = _tablemap->getValue(dbcolumn, getpos);
    			if ( rc ) {
    				rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.x, errmsg, 
    			 						pointx.s(), _schAttr[getpos].offset, _schAttr[getpos].length, 
    									_schAttr[getpos].sig, _schAttr[getpos].type );
    				dbcolumn = dbtab + "." + pointy;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.y, errmsg, 
    			 			pointy.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				} 
    				dbcolumn = dbtab + "." + pointr;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.a, errmsg, 
    			 						   pointr.s(), _schAttr[getpos].offset, _schAttr[getpos].length, 
    									   _schAttr[getpos].sig, _schAttr[getpos].type );
    				} 
    
    				if ( colType == JAG_C_COL_TYPE_SQUARE ) {
        					dbcolumn = dbtab + "." + pointnx;
        					rc = _tablemap->getValue(dbcolumn, getpos);
        					if ( rc ) {
        						rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.nx, errmsg, 
        				 					   pointnx.s(), _schAttr[getpos].offset, _schAttr[getpos].length, 
        									   _schAttr[getpos].sig, _schAttr[getpos].type );
        					} 
    				}
    
    				formatMetricCols( tzdiff, srvtmdiff, dbtab, colname, metrics, otherAttr.point.metrics, tablekvbuf );
    			} 
    		} else if ( colType == JAG_C_COL_TYPE_SPHERE || colType == JAG_C_COL_TYPE_CUBE ) {
    			pointx = colname + ":x"; pointy = colname + ":y";
    			pointz = colname + ":z"; pointr = colname + ":a"; 
    			pointnx = colname + ":nx"; pointny = colname + ":ny"; 
    
    			dbcolumn = dbtab + "." + pointx;
    			rc = _tablemap->getValue(dbcolumn, getpos);
    			if ( rc ) {
    				rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.x, errmsg, 
    			 		pointx.s(), _schAttr[getpos].offset, 
    					_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    
    				dbcolumn = dbtab + "." + pointy;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.y, errmsg, 
    			 			pointy.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				dbcolumn = dbtab + "." + pointz;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.z, errmsg, 
    			 			pointz.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				dbcolumn = dbtab + "." + pointr;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.a, errmsg, 
    			 			pointr.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				if (  columnVector[i].type == JAG_C_COL_TYPE_CUBE ) {
    					dbcolumn = dbtab + "." + pointnx;
    					rc = _tablemap->getValue(dbcolumn, getpos);
    					if ( rc ) {
    						rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.nx, errmsg, 
    			 				pointnx.s(), _schAttr[getpos].offset, 
    							_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    					}
    
    					dbcolumn = dbtab + "." + pointny;
    					rc = _tablemap->getValue(dbcolumn, getpos);
    					if ( rc ) {
    						rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.ny, errmsg, 
    			 				pointny.s(), _schAttr[getpos].offset, 
    							_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    					}
    				}
    
    				formatMetricCols( tzdiff, srvtmdiff, dbtab, colname, metrics, otherAttr.point.metrics, tablekvbuf );
    
    			}
    		} else if ( colType == JAG_C_COL_TYPE_CIRCLE3D || colType == JAG_C_COL_TYPE_SQUARE3D ) {
    			pointx = colname + ":x"; pointy = colname + ":y"; pointz = colname + ":z"; 
    			pointr = colname + ":a"; 
    			pointnx = colname + ":nx"; pointny = colname + ":ny"; 
    
    			dbcolumn = dbtab + "." + pointx;
    			rc = _tablemap->getValue(dbcolumn, getpos);
    			if ( rc ) {
    				rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.x, errmsg, 
    			 		pointx.s(), _schAttr[getpos].offset, 
    					_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    
    				dbcolumn = dbtab + "." + pointy;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.y, errmsg, 
    			 			pointy.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				dbcolumn = dbtab + "." + pointz;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.z, errmsg, 
    			 			pointz.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				dbcolumn = dbtab + "." + pointr;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.a, errmsg, 
    			 			pointr.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				dbcolumn = dbtab + "." + pointnx;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.nx, errmsg, 
    			 			pointnx.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				dbcolumn = dbtab + "." + pointny;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.ny, errmsg, 
    			 			pointny.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				formatMetricCols( tzdiff, srvtmdiff, dbtab, colname, metrics, otherAttr.point.metrics, tablekvbuf );
    			}
    		} else if ( colType  == JAG_C_COL_TYPE_RECTANGLE || colType == JAG_C_COL_TYPE_ELLIPSE ) {
    			pointx = colname + ":x"; pointy = colname + ":y";
    			pointw = colname + ":a"; pointh = colname + ":b"; 
    			pointnx = colname + ":nx"; 
    
    			dbcolumn = dbtab + "." + pointx;
    			rc = _tablemap->getValue(dbcolumn, getpos);
    			if ( rc ) {
    				rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.x, errmsg, 
    			 		pointx.s(), _schAttr[getpos].offset, 
    					_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    
    				dbcolumn = dbtab + "." + pointy;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.y, errmsg, 
    			 			pointy.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				dbcolumn = dbtab + "." + pointw;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.a, errmsg, 
    			 			pointw.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				dbcolumn = dbtab + "." + pointh;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.b, errmsg, 
    			 			pointh.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				dbcolumn = dbtab + "." + pointnx;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.nx, errmsg, 
    			 			pointnx.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    				formatMetricCols( tzdiff, srvtmdiff, dbtab, colname, metrics, otherAttr.point.metrics, tablekvbuf );
    			}
    		} else if ( colType == JAG_C_COL_TYPE_ELLIPSE3D || colType == JAG_C_COL_TYPE_RECTANGLE3D ) {
    			pointx = colname + ":x"; pointy = colname + ":y"; pointz = colname + ":z";
    			pointnx = colname + ":nx"; pointny = colname + ":ny"; 
    			pointw = colname + ":a"; pointh = colname + ":b"; 
    
    			dbcolumn = dbtab + "." + pointx;
    			rc = _tablemap->getValue(dbcolumn, getpos);
    			//d("s2838 _tablemap->getValue %s rc=%d getpos=%d\n", dbcolumn.s(), rc, getpos );
    			//d("s1028 otherAttr.point.x=[%s] otherAttr.point.y=[%s]\n", otherAttr.point.x, otherAttr.point.y );
    			if ( rc ) {
    				rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.x, errmsg, 
    			 		pointx.s(), _schAttr[getpos].offset, 
    					_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    
    				dbcolumn = dbtab + "." + pointy;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				//d("s2830 _tablemap->getValue %s rc=%d getpos=%d\n", dbcolumn.s(), rc, getpos );
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.y, errmsg, 
    			 			pointy.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				dbcolumn = dbtab + "." + pointz;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				//d("s2831 _tablemap->getValue %s rc=%d getpos=%d\n", dbcolumn.s(), rc, getpos );
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.z, errmsg, 
    			 			pointz.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    
    				dbcolumn = dbtab + "." + pointw;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				//d("s2832 _tablemap->getValue %s rc=%d getpos=%d\n", dbcolumn.s(), rc, getpos );
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.a, errmsg, 
    			 			pointw.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				dbcolumn = dbtab + "." + pointh;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.b, errmsg, 
    			 			pointh.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				dbcolumn = dbtab + "." + pointnx;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.nx, errmsg, 
    			 			pointnx.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				dbcolumn = dbtab + "." + pointny;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.ny, errmsg, 
    			 			pointny.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				formatMetricCols( tzdiff, srvtmdiff, dbtab, colname, metrics, otherAttr.point.metrics, tablekvbuf );
    			}
    		} else if ( colType == JAG_C_COL_TYPE_BOX || colType == JAG_C_COL_TYPE_ELLIPSOID ) {
    			pointx = colname + ":x"; pointy = colname + ":y"; pointz = colname + ":z"; 
    			pointw = colname + ":a"; pointh = colname + ":c"; 
    			pointd = colname + ":b";
    			pointnx = colname + ":nx"; pointny = colname + ":ny"; 
    
    			dbcolumn = dbtab + "." + pointx;
    			rc = _tablemap->getValue(dbcolumn, getpos);
    			if ( rc ) {
    				rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.x, errmsg, 
    			 		pointx.s(), _schAttr[getpos].offset, 
    					_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    
    				dbcolumn = dbtab + "." + pointy;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.y, errmsg, 
    			 			pointy.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				dbcolumn = dbtab + "." + pointz;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.z, errmsg, 
    			 			pointz.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				dbcolumn = dbtab + "." + pointw;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.a, errmsg, 
    			 			pointw.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				dbcolumn = dbtab + "." + pointd;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.b, errmsg, 
    			 			pointd.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				dbcolumn = dbtab + "." + pointh;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.c, errmsg, 
    			 			pointh.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				dbcolumn = dbtab + "." + pointnx;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.nx, errmsg, 
    			 			pointnx.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				dbcolumn = dbtab + "." + pointny;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.ny, errmsg, 
    			 			pointny.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				formatMetricCols( tzdiff, srvtmdiff, dbtab, colname, metrics, otherAttr.point.metrics, tablekvbuf );
    
    			}
    		} else if ( colType == JAG_C_COL_TYPE_CYLINDER || colType == JAG_C_COL_TYPE_CONE ) {
    			pointx = colname + ":x"; pointy = colname + ":y"; pointz = colname + ":z"; 
    			pointr = colname + ":a"; pointh = colname + ":c"; 
    			pointnx = colname + ":nx"; pointny = colname + ":ny"; 
    
    			dbcolumn = dbtab + "." + pointx;
    			rc = _tablemap->getValue(dbcolumn, getpos);
    			if ( rc ) {
    				rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.x, errmsg, 
    			 		pointx.s(), _schAttr[getpos].offset, 
    					_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    
    				dbcolumn = dbtab + "." + pointy;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.y, errmsg, 
    			 			pointy.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				dbcolumn = dbtab + "." + pointz;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.z, errmsg, 
    			 			pointz.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				dbcolumn = dbtab + "." + pointr;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.a, errmsg, 
    			 			pointr.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				dbcolumn = dbtab + "." + pointh;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.c, errmsg, 
    			 			pointh.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				dbcolumn = dbtab + "." + pointnx;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.nx, errmsg, 
    			 			pointnx.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    				dbcolumn = dbtab + "." + pointny;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.point.ny, errmsg, 
    			 			pointny.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    				formatMetricCols( tzdiff, srvtmdiff, dbtab, colname, metrics, otherAttr.point.metrics, tablekvbuf );
    			}
    		} else if ( colType == JAG_C_COL_TYPE_LINE3D || colType == JAG_C_COL_TYPE_LINE ) {
    			pointx1 = colname + ":x1"; pointy1 = colname + ":y1"; pointz1 = colname + ":z1";
    			pointx2 = colname + ":x2"; pointy2 = colname + ":y2"; pointz2 = colname + ":z2";
    			is3D = false;
    			if ( colType == JAG_C_COL_TYPE_LINE3D ) { is3D = true; }
    
    			dbcolumn = dbtab + "." + pointx1;
    			rc = _tablemap->getValue(dbcolumn, getpos);
    			if ( rc ) {
    				rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.linestr.point[0].x, errmsg, 
    			 		pointx1.s(), _schAttr[getpos].offset, 
    					_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    
    				dbcolumn = dbtab + "." + pointy1;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.linestr.point[0].y, errmsg, 
    			 			pointy1.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				if ( is3D ) {
    					dbcolumn = dbtab + "." + pointz1;
    					rc = _tablemap->getValue(dbcolumn, getpos);
    					if ( rc ) {
    						rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.linestr.point[0].z, errmsg, 
    			 				pointz1.s(), _schAttr[getpos].offset, _schAttr[getpos].length, _schAttr[getpos].sig, 
    							_schAttr[getpos].type );
    					}
    				}
    
    				dbcolumn = dbtab + "." + pointx2;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.linestr.point[1].x, errmsg, 
    			 			pointx2.s(), _schAttr[getpos].offset, _schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				dbcolumn = dbtab + "." + pointy2;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				//d("s2828 dbcolumn=%s rc=%d getpos=%d\n", dbcolumn.s(), rc, getpos );
    				//d("2381 pointy2=[%s] otherAttr.linestr.point[1].y=[%s]\n", pointy2.s(), otherAttr.linestr.point[1].y );
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.linestr.point[1].y, errmsg, 
    			 			pointy2.s(), _schAttr[getpos].offset, _schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				if ( is3D ) {
    					dbcolumn = dbtab + "." + pointz2;
    					rc = _tablemap->getValue(dbcolumn, getpos);
    					if ( rc ) {
    						rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.linestr.point[1].z, errmsg, 
    			 				pointz2.s(), _schAttr[getpos].offset, _schAttr[getpos].length, _schAttr[getpos].sig, 
    							_schAttr[getpos].type );
    					}
    				}
    
    				formatMetricCols( tzdiff, srvtmdiff, dbtab, colname, metrics, otherAttr.linestr.point[0].metrics, tablekvbuf );
    				//formatMetricCols( tzdiff, srvtmdiff, dbtab, colname, metrics, otherAttr.linestr.point[1].metrics, tablekvbuf );
    			}
    		} else if ( colType == JAG_C_COL_TYPE_VECTOR ) {
    			d("s258135 colType=[%s] colname=[%s]\n", colType.s(), colname.s() );
    			is3D = false;

    			getColumnIndex( dbtab, colname, is3D, getx, gety, getz, getxmin, getymin, getzmin,
    								getxmax, getymax, getzmax, getid, getcol, getm, getn, geti );
    			d("s2234038 getColumnIndex is3D=%d getxmin=%d getx=%d gety=%d getz=%d\n", is3D, getxmin, getx, gety, getz );
    			//if ( getxmin >= 0 ) 
    			if ( getx >= 0 ) {
    				JagVectorString vline;
   					JagParser::addVectorData( vline, otherAttr.valueData.s() );

    				JagPolyPass ppass;
    				ppass.is3D = is3D;

    				ppass.tzdiff = tzdiff; 
                    ppass.srvtmdiff = srvtmdiff;

    				ppass.getxmin = getxmin; 
                    // ppass.getymin = getymin; ppass.getzmin = getzmin;
    				ppass.getxmax = getxmax; 
                    // ppass.getymax = getymax; ppass.getzmax = getzmax;

    				ppass.getid = getid; ppass.getcol = getcol;
    				ppass.getm = getm; ppass.getn = getn; ppass.geti = geti;
    				ppass.getx = getx; 

                    // ppass.gety = gety; ppass.getz = getz;

                    if ( lsuuid.size() < 1 ) {
                        lsuuid = _servobj->_jagUUID->getGidString();
                    }

    				ppass.lsuuid = lsuuid;
    				ppass.dbtab = dbtab;
    				ppass.colname = colname;
    				ppass.m = 0;
    				ppass.n = 0;
    				ppass.col = i;
    
    				formatPointsInVector( metrics, vline, tablekvbuf, ppass, retpair, errmsg );
    
    				++mlineIndex;
        			hasDoneAppend = true;
    				rc = 1;
    			}
    		} else if ( colType == JAG_C_COL_TYPE_LINESTRING || colType == JAG_C_COL_TYPE_LINESTRING3D
    					 || colType == JAG_C_COL_TYPE_MULTIPOINT || colType == JAG_C_COL_TYPE_MULTIPOINT3D
    			       ) {
    			//d("s7650 i=%d colType=[%s] colname=[%s] \n", i, colType.s(), colname.s() );
    			//pointx = colname + ":x"; pointy = colname + ":y"; pointz = colname + ":z";
    			d("s218135 colType=[%s] colname=[%s]\n", colType.s(), colname.s() );
    			is3D = false;
    			if ( colType == JAG_C_COL_TYPE_LINESTRING3D || colType == JAG_C_COL_TYPE_MULTIPOINT3D ) { 
                    is3D = true; 
                }

                /***
	            xmin=JAG_LONG_MAX; ymin=JAG_LONG_MAX; zmin=JAG_LONG_MAX; 
                xmax=JAG_LONG_MIN; ymax=JAG_LONG_MIN; zmax=JAG_LONG_MIN;
                ***/

    			getColumnIndex( dbtab, colname, is3D, getx, gety, getz, getxmin, getymin, getzmin,
    								getxmax, getymax, getzmax, getid, getcol, getm, getn, geti );
    			d("s2234038 getColumnIndex is3D=%d getxmin=%d getx=%d gety=%d getz=%d\n", is3D, getxmin, getx, gety, getz );
    			//if ( getxmin >= 0 ) 
    			if ( getx >= 0 ) {
    				JagLineString line;
    				if ( colType == JAG_C_COL_TYPE_LINESTRING || colType == JAG_C_COL_TYPE_MULTIPOINT ) {
                        /**
                        dn("s34054004 2d getLineStringMinMax valueData=[%s]", otherAttr.valueData.s() );
                        rc = JagParser::getLineStringMinMax( ',', otherAttr.valueData.s(), xmin, ymin, xmax, ymax );
                        zmin = zmax = 0;
                        **/

    					JagParser::addLineStringData(line, otherAttr.valueData.s() );
                        /**
    					d("s8360832 addLineStringData va=[%s] xmin=%0.3f ymin=%.3f zmin=%.3f arc=%d\n", otherAttr.valueData.s(), xmin, ymin, zmin, arc );
                        dn("s7242001 2d lstr colname=%s i=%d xmin=%f  xmax=%f", colname.s(), i, xmin, xmax );
                        dn("s7242001 2d lstr colname=%s i=%d ymin=%f  ymax=%f", colname.s(), i, ymin, ymax );
                        **/
    				} else {
    					JagParser::addLineString3DData(line, otherAttr.valueData.s() );
                        /**
    					d("s80833 addLineString3DData xmin=%0.3f ymin=%.3f zmin=%.3f vData=[%s]\n", xmin, ymin, zmin, otherAttr.valueData.s() );
                        dn("s7242004 3d lstr i=%d xmin=%f  xmax=%f", i, xmin, xmax );
                        dn("s7242004 3d lstr i=%d ymin=%f  ymax=%f", i, ymin, ymax );
                        **/
    				}

    				JagPolyPass ppass;
    				ppass.is3D = is3D;

                    /*** if no bbox
    				ppass.xmin = xmin; ppass.ymin = ymin; ppass.zmin = zmin;
    				ppass.xmax = xmax; ppass.ymax = ymax; ppass.zmax = zmax;
                    ***/

    				ppass.tzdiff = tzdiff; ppass.srvtmdiff = srvtmdiff;

    				ppass.getxmin = getxmin; ppass.getymin = getymin; ppass.getzmin = getzmin;
    				ppass.getxmax = getxmax; ppass.getymax = getymax; ppass.getzmax = getzmax;

    				ppass.getid = getid; ppass.getcol = getcol;
    				ppass.getm = getm; ppass.getn = getn; ppass.geti = geti;
    				ppass.getx = getx; ppass.gety = gety; ppass.getz = getz;

                    if ( lsuuid.size() < 1 ) {
                        lsuuid = _servobj->_jagUUID->getGidString();
                    }

    				ppass.lsuuid = lsuuid;
    				ppass.dbtab = dbtab;
    				ppass.colname = colname;
    				ppass.m = 0;
    				ppass.n = 0;
    				ppass.col = i;
    
    				formatPointsInLineString( metrics, line, tablekvbuf, ppass, retpair, errmsg );
    
    				++mlineIndex;
        			hasDoneAppend = true;
    				rc = 1;
    			}
    		} else if ( colType == JAG_C_COL_TYPE_POLYGON || colType == JAG_C_COL_TYPE_POLYGON3D
    		            || colType == JAG_C_COL_TYPE_MULTILINESTRING || colType == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
    			//d("s7651 i=%d colType=[%s]\n", i, colType.s() );
    			// pointx = colname + ":x"; pointy = colname + ":y"; pointz = colname + ":z";
    			is3D = false;
    			if ( colType == JAG_C_COL_TYPE_POLYGON3D || colType == JAG_C_COL_TYPE_MULTILINESTRING3D ) { is3D = true; }

                /***
	            xmin=JAG_LONG_MAX; ymin=JAG_LONG_MAX; zmin=JAG_LONG_MAX; 
                xmax=JAG_LONG_MIN; ymax=JAG_LONG_MIN; zmax=JAG_LONG_MIN;
                ***/

    			getColumnIndex( dbtab, colname, is3D, getx, gety, getz, getxmin, getymin, getzmin,
    								getxmax, getymax, getzmax, getid, getcol, getm, getn, geti );
    			//if ( getxmin >= 0 ) 
    			if ( getx >= 0 ) {
    				//char ibuf[32];
    				//ValueAttribute other;
    				JagPolygon pgon;
    				if ( colType == JAG_C_COL_TYPE_POLYGON || colType == JAG_C_COL_TYPE_MULTILINESTRING ) {
                        /***
                        JagParser::getPolygonMinMax( otherAttr.valueData.s(), xmin, ymin, xmax, ymax );
                        zmin = zmax = 0;
                        **/

    					if ( colType == JAG_C_COL_TYPE_POLYGON ) {
    						rc = JagParser::addPolygonData( pgon, otherAttr.valueData.s(), false, true );
    					} else {
    						rc = JagParser::addPolygonData( pgon, otherAttr.valueData.s(), false, false );
    					}
    					//d("s8041 addPolygonData xmin=%0.3f ymin=%.3f zmin=%.3f \n", xmin, ymin, zmin );
    				} else {
                        //JagParser::getPolygon3DMinMax( otherAttr.valueData.s(), xmin, ymin, zmin, xmax, ymax, zmax );

    					if ( colType == JAG_C_COL_TYPE_POLYGON3D ) {
    						rc = JagParser::addPolygon3DData( pgon, otherAttr.valueData.s(), false, true );
    					} else {
    						rc = JagParser::addPolygon3DData( pgon, otherAttr.valueData.s(), false, false );
    					}
    					//d("s8042 addPolygonData3D xmin=%0.3f ymin=%.3f zmin=%.3f\n", xmin, ymin, zmin );
    				}
    
    				if ( 0 == rc ) { continue; }
    				if ( rc < 0 ) {
    					errmsg = Jstr("E3014  Polygon input data error ") + intToStr(rc);
    					d("E3014 Polygon input data error rc=%d\n", rc );
    					free( tablekvbuf );
    					return 0;
    				}
    
    				//if ( ! is3D ) { zmin = zmax = 0.0; }
    				//pgon.print();
    
    				JagPolyPass ppass;
    				ppass.is3D = is3D;
                    /**
    				ppass.xmin = xmin; ppass.ymin = ymin; ppass.zmin = zmin;
    				ppass.xmax = xmax; ppass.ymax = ymax; ppass.zmax = zmax;
                    **/
    				ppass.tzdiff = tzdiff; ppass.srvtmdiff = srvtmdiff;
    				ppass.getxmin = getxmin; ppass.getymin = getymin; ppass.getzmin = getzmin;
    				ppass.getxmax = getxmax; ppass.getymax = getymax; ppass.getzmax = getzmax;
    				ppass.getid = getid; ppass.getcol = getcol;
    
    				ppass.getx = getx; ppass.gety = gety; ppass.getz = getz;
                    if ( lsuuid.size() < 1 ) {
                        lsuuid = _servobj->_jagUUID->getGidString();
                    }
    				ppass.lsuuid = lsuuid;
    				ppass.dbtab = dbtab;
    				ppass.colname = colname;
    				ppass.getm = getm; ppass.getn = getn; ppass.geti = geti;
    				ppass.col = i;
    
    				//d("s7830 pgon.size()=%d\n", pgon.size() );
    				ppass.m = 0;
    				JagLineString linestr;
    				for ( int n=0; n < pgon.size(); ++n ) {
    					ppass.n = n;
    					linestr = pgon.linestr[n]; // convert
    					formatPointsInLineString( metrics, linestr, tablekvbuf, ppass, retpair, errmsg );
    				}
    
    				rc = 1;
    
    				++mlineIndex;
        			hasDoneAppend = true;
    			} else {
    				d("s2773 error polygon\n" );
    			}
    		} else if ( colType == JAG_C_COL_TYPE_MULTIPOLYGON || colType == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
    			//d("s7651 i=%d colType=[%s]\n", i, colType.s() );
    			// pointx = colname + ":x"; pointy = colname + ":y"; pointz = colname + ":z";
    			is3D = false;
    			if ( colType == JAG_C_COL_TYPE_MULTIPOLYGON3D ) { is3D = true; }

                /***
	            xmin=JAG_LONG_MAX; ymin=JAG_LONG_MAX; zmin=JAG_LONG_MAX; 
                xmax=JAG_LONG_MIN; ymax=JAG_LONG_MIN; zmax=JAG_LONG_MIN;
                ***/

    			getColumnIndex( dbtab, colname, is3D, getx, gety, getz, getxmin, getymin, getzmin,
    								getxmax, getymax, getzmax, getid, getcol, getm, getn, geti );
    			//if ( getxmin >= 0 ) 
    			if ( getx >= 0 ) {
    				//char ibuf[32];
    				//ValueAttribute other;
    				JagVector<JagPolygon> pgvec;
    				if ( is3D ) {
    					//d("s8260 addMultiPolygonData 3d\n" );
    					rc = JagParser::addMultiPolygonData( pgvec, otherAttr.valueData.s(), false, false, true );
    				} else {
    					//d("s8260 addMultiPolygonData 2d\n" );
                        /**
                        JagParser::getMultiPolygonMinMax( otherAttr.valueData.s(), xmin, ymin, xmax, ymax );
                        zmin = zmax = 0;
                        **/

    					rc = JagParser::addMultiPolygonData( pgvec, otherAttr.valueData.s(), false, false, false );
    				}
    
    				if ( 0 == rc ) continue;
    				if ( rc < 0 ) {
    					errmsg = Jstr("E3015  MultiPolygon input data error ") + intToStr(rc);
    					//d("E3015 Polygon input data error rc=%d\n", rc );
    					free( tablekvbuf );
    					return 0;
    				}
    
    				//if ( ! is3D ) { zmin = zmax = 0.0; }
    				//pgon.print();
    
    				JagPolyPass ppass;
    				ppass.is3D = is3D;
                    /**
    				ppass.xmin = xmin; ppass.ymin = ymin; ppass.zmin = zmin;
    				ppass.xmax = xmax; ppass.ymax = ymax; ppass.zmax = zmax;
                    **/
    				ppass.tzdiff = tzdiff; ppass.srvtmdiff = srvtmdiff;
    				ppass.getxmin = getxmin; ppass.getymin = getymin; ppass.getzmin = getzmin;
    				ppass.getxmax = getxmax; ppass.getymax = getymax; ppass.getzmax = getzmax;
    				ppass.getid = getid; ppass.getcol = getcol;
    
    				ppass.getx = getx; ppass.gety = gety; ppass.getz = getz;
                    if ( lsuuid.size() < 1 ) {
                        lsuuid = _servobj->_jagUUID->getGidString();
                    }
    				ppass.lsuuid = lsuuid;
    				ppass.dbtab = dbtab;
    				ppass.colname = colname;
    				ppass.getm = getm; ppass.getn = getn; ppass.geti = geti;
    				ppass.col = i;
    
    				JagLineString linestr;
        			//d("s7830 pgvec.size()=%d\n", pgvec.size() );
    				for ( int i = 0; i < pgvec.size(); ++i ) {
        				ppass.m = i;
        				//d("s7830 pgvec.size()=%d  ppass.m=%d\n", pgvec.size(),  ppass.m );
    					const JagPolygon& pgon = pgvec[i];
        				for ( int n=0; n < pgon.size(); ++n ) {
        					ppass.n = n;
    						linestr = pgon.linestr[n];
        					formatPointsInLineString( metrics, linestr, tablekvbuf, ppass, retpair, errmsg );
        				}
    				}
    
    				rc = 1;
    
    				++mlineIndex;
        			hasDoneAppend = true;
    			}
    		} else if ( colType == JAG_C_COL_TYPE_TRIANGLE || colType == JAG_C_COL_TYPE_TRIANGLE3D ) {
    			pointx1 = colname + ":x1"; pointy1 = colname + ":y1"; pointz1 = colname + ":z1"; 
    			pointx2 = colname + ":x2"; pointy2 = colname + ":y2"; pointz2 = colname + ":z2";
    			pointx3 = colname + ":x3"; pointy3 = colname + ":y3"; pointz3 = colname + ":z3";
    
    			is3D = false;
    			if ( colType == JAG_C_COL_TYPE_TRIANGLE3D ) { is3D = true; }
    
    			dbcolumn = dbtab + "." + pointx1;
    			rc = _tablemap->getValue(dbcolumn, getpos);
    
    			//d("s5828 dbcolumn=%s rc=%d getpos=%d\n", dbcolumn.s(), rc, getpos );
    			//d("s5381 pointy2=[%s] otherAttr.linestr.point[1].y=[%s]\n", pointy2.s(), otherAttr.linestr.point[1].y );
    
    			if ( rc ) {
    				rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.linestr.point[0].x, errmsg, 
    			 		pointx1.s(), _schAttr[getpos].offset, 
    					_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    
    				dbcolumn = dbtab + "." + pointy1;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.linestr.point[0].y, errmsg, 
    			 			pointy1.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				if ( is3D ) {
    					dbcolumn = dbtab + "." + pointz1;
    					rc = _tablemap->getValue(dbcolumn, getpos);
    					if ( rc ) {
    						rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.linestr.point[0].z, errmsg, 
    			 				pointz1.s(), _schAttr[getpos].offset, 
    							_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    					}
    				}
    
    				dbcolumn = dbtab + "." + pointx2;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.linestr.point[1].x, errmsg, 
    			 			pointx2.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				dbcolumn = dbtab + "." + pointy2;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				//d("s5428 dbcolumn=%s rc=%d getpos=%d\n", dbcolumn.s(), rc, getpos );
    				//d("s5581 pointy2=[%s] otherAttr.linestr.point[1].y=[%s]\n", pointy2.s(), otherAttr.linestr.point[1].y );
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.linestr.point[1].y, errmsg, 
    			 			pointy2.s(), _schAttr[getpos].offset, 
    						_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				if ( is3D ) {
    					dbcolumn = dbtab + "." + pointz2;
    					rc = _tablemap->getValue(dbcolumn, getpos);
    					if ( rc ) {
    						rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.linestr.point[1].z, errmsg, 
    			 				pointz2.s(), _schAttr[getpos].offset, 
    							_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    					}
    				}
    
    				dbcolumn = dbtab + "." + pointx3;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.linestr.point[2].x, errmsg, 
    			 			pointx3.s(), _schAttr[getpos].offset, _schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				dbcolumn = dbtab + "." + pointy3;
    				rc = _tablemap->getValue(dbcolumn, getpos);
    				if ( rc ) {
    					rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.linestr.point[2].y, errmsg, 
    			 			pointy3.s(), _schAttr[getpos].offset, _schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    				}
    
    				if ( is3D ) {
    					dbcolumn = dbtab + "." + pointz3;
    					rc = _tablemap->getValue(dbcolumn, getpos);
    					// d("s5201 is3D getValue pointz3 rc=%d\n", rc );
    					if ( rc ) {
    						rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.linestr.point[2].z, errmsg, 
    			 				pointz3.s(), _schAttr[getpos].offset, 
    							_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    						// d("s5203 formatOneCol rc=%d\n", rc );
    					}
    				}
    
    				formatMetricCols( tzdiff, srvtmdiff, dbtab, colname, metrics, otherAttr.linestr.point[0].metrics, tablekvbuf );
    				//formatMetricCols( tzdiff, srvtmdiff, dbtab, colname, metrics, otherAttr.linestr.point[1].metrics, tablekvbuf );
    				//formatMetricCols( tzdiff, srvtmdiff, dbtab, colname, metrics, otherAttr.linestr.point[2].metrics, tablekvbuf );
    			} 
    		} else {
                /***
				d("s30358 colType is non-geo not empty colType=[%s]\n", colType.s() );
    			if ( otherAttr.valueData.size() < 1 ) {
    				rc = 1;
					d("s49448 otherAttr.valueData.size is 0\n" );
    			} else {
					d("s52014 formatOneCol col=%s getpos=%d offset=%d length=%d sig=%d\n", 
						 columnVector[i].name.s(), getpos, _schAttr[getpos].offset, _schAttr[getpos].length, _schAttr[getpos].sig );

    			    rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.valueData.s(), errmsg, 
    				 	columnVector[i].name.s(), _schAttr[getpos].offset, 
    					_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
					d("s42448 formatOneCol %s rc=%d\n", columnVector[i].name.s(), rc );
    			}
                ***/
    		}
		} else {
			// colType is empty
			//d("s22020 \n");
            /***
    		if ( otherAttr.valueData.size() < 1 ) {
    			rc = 1;
				d("s464408 otherAttr.valueData.size is 0\n" );
    		} else {
				d("s52016 formatOneCol col=%s getpos=%d offset=%d length=%d sig=%d\n", 
					columnVector[i].name.s(), getpos, _schAttr[getpos].offset, _schAttr[getpos].length,  _schAttr[getpos].sig );

    		    rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, otherAttr.valueData.s(), errmsg, 
    			 	columnVector[i].name.s(), _schAttr[getpos].offset, 
    				_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
    		}
            ***/
		}


		if ( !rc ) {
			free( tablekvbuf );
			d("s29023 return 0 here\n" );
			return 0;
		}

		// debug only
		/***
		d("s2033 i=%d getpos dbcolumn=[%s] tablekvbuf: \n", i, dbcolumn.s() );
		jagfwrite( tablekvbuf,  KEYVALLEN, stdout );
		***/

	}  // end for ( int i = 0; i < parseParam->valueVec.size(); ++i ) 

	if ( *tablekvbuf == '\0' ) {
		errmsg = "E1101 First key is NULL";
		free( tablekvbuf );
		return 0;
	}

	// setup default value if exists
	/***
	d("s444448 _defvallist.size=%d\n",  _defvallist.size() );
	for ( int i = 0; i < _defvallist.size(); ++i ) {
		if ( _defvallist[i] < parseParam->valueVec.size() ) {
			d("s4555508 i=%d defvaliit=%d < valueVec=%d continue\n", i, _defvallist[i], parseParam->valueVec.size() );
			continue;
		}
		d("s3028371 i=%d _defvallist[i]]=[%d]\n", i, _defvallist[i] );

		rc = formatOneCol( tzdiff, srvtmdiff, tablekvbuf, schAttr[_defvallist[i]].defValue.s(), errmsg, 
			 (*tableRecord.columnVector)[_defvallist[i]].name.s(), schAttr[_defvallist[i]].offset, 
			schAttr[_defvallist[i]].length, schAttr[_defvallist[i]].sig, schAttr[_defvallist[i]].type );

		if ( !rc ) {
			free( tablekvbuf );
			errmsg = "E3039 formatOneCol default error";
			return 0;
		}			
	}
	***/

	// debug only
	/**
	d("s2037 tablekvbuf: \n" );
	jagfwrite( tablekvbuf,  KEYVALLEN, stdout );
	d("\n");
	**/
	//d("s5042 hasDoneAppend=%d\n", hasDoneAppend );
	if ( ! hasDoneAppend ) {
		dbNaturalFormatExchange( tablekvbuf, _numKeys, _schAttr, 0, 0, " " ); 
		retpair.append( JagDBPair( tablekvbuf, _KEYLEN, tablekvbuf+_KEYLEN, _VALLEN, true ) );
		/**
		d("s4106 no hasDoneAppend  tablekvbuf:\n" );
		dumpmem( tablekvbuf, KEYLEN+VALLEN);
		**/
		//d("s30066 parsed pair:\n"); // 
		//retpair[retpair.size() - 1].print();
		//retpair[retpair.size() - 1].printkv();
	}

	free( tablekvbuf );
	d("s42831 parsePair return 1\n");
	return 1;
}

// insert one record 
// int JagTable::insert( const JagRequest &req, JagParseParam *parseParam, Jstr &errmsg, int &insertCode, bool direct )
// return 1: OK   0: error
int JagTable::insert( const JagRequest &req, JagParseParam *parseParam, Jstr &errmsg )
{
	JagVector<JagDBPair> pairVec;  // geo data returns multiple pairs
	int rc = parsePair( req.session->timediff, parseParam, pairVec, errmsg );
	d("s14920 table-this=%p parsePair rc=%d pairVec.size=%d _replicType=%d\n", this, rc, pairVec.size(), _replicType );
	// pair.print();
    
    if ( ! rc ) {
		d("s222201 parsePair error [%s] rc=%d\n", errmsg.s(), rc );
        return 0;
    }

	for ( int i=0; i < pairVec.length(); ++i ) {

		rc = insertPair( pairVec[i], false );

		d("s34781 table-this=%p inserrPair i=%d  insertPair rc=%d\n", this, i, rc );
		if ( !rc ) {
			errmsg = Jstr("E2108 InsertPair error key: ") + pairVec[i].key.s();
		} else {
			d("s33308 insertPair OK rc=%d\n", rc );
		}
	}

	return rc;  //0 or 1
}

// return: 0 for error
int JagTable::insertPair( JagDBPair &pair, bool doIndexLock ) 
{
	d("s5130 insertPair pair.key=[%s]\n", pair.key.s() );
	d("s5130 insertPair pair.value: " );
	//pair.value.print();  // 
	d("\n");

	int rc;
	d("s333058 JagTable::insertPair _replicType=%d pair=[%s][%s]\n", _replicType, pair.key.s(), pair.value.s() );

    //pair.setTabRec( &_tableRecord );
    //pair.printColumns();

    bool hasFlushed;

	rc = _darrFamily->insert( pair, hasFlushed );

	// check if table has indexs, if yes, insert to index on local server
	d("s53974 _darrFamily->insert rc=%d hasFlushed=%d\n", rc, hasFlushed );
	if ( rc ) {
	    d("s53974 insertIndex ...\n" );
		int idxcnt = insertIndex( pair, doIndexLock, hasFlushed );
	    d("s53974 insertIndex done idxcnt=%d\n", idxcnt );
	} else {
        dn("s61206 JagTable::insertPair insert.rc=%d error", rc );
    }

    // debug only 
    /***
    if ( _indexlist.size() > 0 ) {
        jagint tabBufferSize = memoryBufferSize();
        jagint idxBufferSize = getAllIndexBufferSize();
        dn("s6012262 tabBufferSize=%ld  idxBufferSize=%ld", tabBufferSize, idxBufferSize );
        assert( tabBufferSize == idxBufferSize );
    }
    ***/

	d("s30992 insertPair_done rc=%d\n", rc );
	return rc;
}

// single insert, used by inserted data with file transfer
// 1; OK    <=0 error
int JagTable::finsert( const JagRequest &req, JagParseParam *parseParam, Jstr &errmsg )
{
	d("s602221 finsert ...\n");
	JagVector<JagDBPair> pair;
	int rc = parsePair( req.session->timediff, parseParam, pair, errmsg );

    Jstr hdir;

	if ( rc ) {
		for ( int k=0; k < pair.size(); ++k ) {

    		//rc = insertPair( pair[k], true );
    		rc = insertPair( pair[k], false );

    		if ( !rc ) {
    			errmsg = Jstr("E4003 Insert error key: ") + pair[k].key.s();
                rc = 0;
    		} else {
    			rc = 0;

                dn("s38329 getFileHashDir ...");
                hdir = getFileHashDir( _tableRecord, pair[k].key );

				if ( req.session->sock > 0 ) {
    				for ( int i = 0; i < _numCols; ++i ) {
    					if ( _schAttr[i].isFILE ) {
							Jstr fpath = _darrFamily->_sfilepath + "/" + hdir;
    						JagFileMgr::makedirPath( fpath );
							d("s52004 oneFileReceiver [%s]\n", fpath.s() );

    						//rc = oneFileReceiver( req.session->sock, fpath );
    						rc = oneFileReceiver( req.session->sock, _darrFamily->_sfilepath, hdir, true );
                            // error: rc < 0;  OK:  1
                            dn("s2025301 oneFileReceiver fpath=[%s] rc=%d", fpath.s(), rc );
    					}
    				}
				}
    		}
		}
	} else {
		errmsg = Jstr("E4123 Error insert pair");
        rc = 0;
	}
	
	return rc;
}

// method to form insert/upsert into index cmd for each table insert/upsert cmd
int JagTable::insertIndex( JagDBPair &pair, bool doIndexLock, bool hasFlushed  )
{
	if ( _indexlist.size() < 1 ) return 0;
	d("s500132 insertIndex ...\n");

    //pair.setTabRec( &_tableRecord );
    //pair.printColumns();

	char *tablebuf = (char*)jagmalloc(_KEYVALLEN+1);
	memcpy( tablebuf, pair.key.s(), _KEYLEN );
	memcpy( tablebuf+_KEYLEN, pair.value.s(), _VALLEN );
	tablebuf[_KEYVALLEN] = '\0';

    dn("si29299999 insertIndex tablebuf:");
    //dumpmem( tablebuf, _KEYVALLEN);

	dbNaturalFormatExchange( tablebuf, _numKeys, _schAttr, 0, 0, " " ); // db format -> natural format
	
	jagint cnt = 0;
	AbaxBuffer bfr;
	JagIndex *pindex = NULL;
	int lockrc;

	for ( int i = 0; i < _indexlist.size(); ++i ) {
		//d("s40124 writeLockIndex [%s] ... \n", _indexlist[i].s() );
		if ( doIndexLock ) {
			pindex = _objectLock->writeLockIndex( JAG_INSERT_OP, _dbname, _tableName, _indexlist[i],
										      _tableschema, _indexschema, _replicType, true, lockrc );
		} else {
			pindex = _objectLock->getIndex(  _dbname, _indexlist[i], _replicType );
		}

		if ( pindex ) {

            dn("s3333038 debug tablebuf:\n");
            //pair.printColumns( tablebuf );

            dn("s366001 insertIndexFromTable ...");
			pindex->insertIndexFromTable( tablebuf, hasFlushed );

			++ cnt;
			if ( doIndexLock ) {
				_objectLock->writeUnlockIndex( JAG_INSERT_OP, _dbname, _tableName, _indexlist[i], _replicType, 1 );
			}
			//d("s40124 writeLockIndex [%s] done\n", _indexlist[i].s() );
		}
	}

	free( tablebuf );
	d("s500135 insertIndex done\n");
	return cnt;
}

// For debugging only
jagint JagTable::getAllIndexBufferSize()
{
	if ( _indexlist.size() < 1 ) return 0;

	jagint cnt = 0;
	JagIndex *pindex;
	for ( int i = 0; i < _indexlist.size(); ++i ) {
		//d("s40124 writeLockIndex [%s] ... \n", _indexlist[i].s() );
		pindex = _objectLock->getIndex(  _dbname, _indexlist[i], _replicType );
		if ( pindex ) {
			cnt += pindex->memoryBufferSize();
			//d("s40124 writeLockIndex [%s] done\n", _indexlist[i].s() );
		}
	}

	d("s504182 getAllIndexBufferSize cnt=%ld\n", cnt);
	return cnt;
}

// method to do create index concurrently
// pindex was writelock protected
int JagTable::formatCreateIndex( JagIndex *pindex ) 
{
	ParallelCmdPass *psp = newObject<ParallelCmdPass>();
	psp->ptab = this;
	psp->pindex = pindex;
	parallelCreateIndexStatic ((void*)psp );
	return 1;
}

void *JagTable::parallelCreateIndexStatic( void * ptr )
{	
	ParallelCmdPass *pass = (ParallelCmdPass*)ptr;

    // delayed-crating-index table records
    jagint maxRecords = pass->ptab->_cfg->getLongValue("DELAYED_INDEX_CREATE_RECORDS", 100000);

	int KLEN = pass->ptab->_darrFamily->_KLEN;
	int KVLEN = pass->ptab->_darrFamily->_KVLEN;

	char *tablebuf = (char*)jagmalloc(KVLEN+1);
	memset( tablebuf, 0, KVLEN+1 );

	char minbuf[KLEN+1];
	char maxbuf[KLEN+1];
	memset( minbuf, 0,   KLEN+1 );
	memset( maxbuf, 255, KLEN+1 );

	JagMergeReader *ntu = NULL;
	pass->ptab->_darrFamily->setFamilyRead( ntu, true, minbuf, maxbuf );

    jagint  cnt = 0;
	if ( ntu ) {
		while ( ntu->getNext( tablebuf ) ) {
			dbNaturalFormatExchange( tablebuf, pass->ptab->_numKeys, pass->ptab->_schAttr, 0, 0, " " ); 
			pass->pindex->insertIndexFromTable( tablebuf, false );
            ++cnt;
            if ( cnt >= maxRecords ) {
                break;
            }
		}
	}

	free( tablebuf );
	delete pass;
	return NULL;
}


// return < 0 for error; 0: no key found;  > 0 OK
jagint JagTable::update( const JagRequest &req, const JagParseParam *parseParam, bool upsert, Jstr &errmsg )
{
	// chain not able to update
	if ( JAG_CHAINTABLE_TYPE == _objectType ) {
		errmsg = "E23072 Chain cannot be updated";
		return -100;
	}

	d("s344082 JagTable::update parseParam->updSetVec.print():\n" );
	//parseParam->updSetVec.print();

	d("s344083 JagTable::update parseParam->valueVec.print():\n" );
	//parseParam->valueVec.print();

	int 	numUpdateCols = parseParam->updSetVec.size();
	int 	rc, collen, siglen, setindexnum = _indexlist.size(); 
	int 	constMode = 0, typeMode = 0, tabnum = 0, treelength = 0;
	Jstr 	treetype;
	bool 	uniqueAndHasValueCol = 0, setKey;
	jagint 	cnt = 0, scanned = 0;
	const char *buffers[1];
	jagint 	setposlist[numUpdateCols];
	int 	getpos = 0;
	int     numIndexes = _indexlist.size();
	JagIndex *lpindex[ numIndexes ];
	AbaxBuffer bfr;
	Jstr 	dbcolumn;

	dn("st0282801 numUpdateCols=%d", numUpdateCols);

	ExprElementNode *updroot;
	const JagHashStrInt *maps[1];
	const JagSchemaAttribute *attrs[1];	
	maps[0] = _tablemap;
	attrs[0] = _schAttr;
	
	JagFixString strres, treestr;
	bool needInit = true;
	bool singleInsert = false;
	
	// check updSetVec validation
	setKey = 0;
	for ( int i = 0; i < numUpdateCols; ++i ) {
		dbcolumn = _dbtable + "." + parseParam->updSetVec[i].colName;
		if ( isFileColumn( parseParam->updSetVec[i].colName ) ) {
			errmsg = Jstr("E10283 column ") + parseParam->updSetVec[i].colName + " is file type";
			return -1;
		}

		if ( _tablemap->getValue(dbcolumn, getpos) ) {
			setposlist[i] = getpos;
			if ( getpos < _numKeys ) { 
				setKey = 1;
				errmsg = Jstr("E03875 Key column cannot be updated");
				return -10;
			}

			bool isAggregate = false;
			updroot = parseParam->updSetVec[i].tree->getRoot();
			rc = updroot->setFuncAttribute( maps, attrs, constMode, typeMode, isAggregate, treetype, collen, siglen );
            dn("s211107902 updroot->setFuncAttribute rc=%d", rc );
			if ( 0 == rc || isAggregate ) {
				errmsg = Jstr("E0383 wrong update type. Char column must use single quotes.");
				return -20;
			}
		} else {
			errmsg = Jstr("E0384 column ") + dbcolumn + " not found";
			return -30;
		}
	}
	
	// build init index list
	/**
	if ( setKey ) {
		for ( int i = 0; i < _indexlist.size(); ++i ) {
			lpindex[i] = _objectLock->readLockIndex( JAG_UPDATE_OP, _dbname, _tableName, 
															   _indexlist[i], _replicType, 1 );
		}
	} else {
		setindexnum = 0;
		for ( int i = 0; i < _indexlist.size(); ++i ) {
			lpindex[i] = NULL;
			lpindex[setindexnum] = _objectLock->readLockIndex( JAG_UPDATE_OP, _dbname, _tableName, 
																	     _indexlist[i], _replicType, 1 );
			for ( int i = 0; i < numUpdateCols; ++i ) {
				if ( lpindex[setindexnum] ) {
					if ( lpindex[setindexnum]->needUpdate( parseParam->updSetVec[i].colName ) ) {
						++setindexnum; break;
					} else {
						_objectLock->readUnlockIndex( JAG_UPDATE_OP, _dbname, _tableName, 
																lpindex[setindexnum]->getIndexName(), _replicType, 1 );
					}
				}
			}
		}
	}
	**/

	/***
	setindexnum = 0;
	for ( int i = 0; i < _indexlist.size(); ++i ) {
		lpindex[i] = NULL;
		lpindex[setindexnum] = _objectLock->readLockIndex( JAG_UPDATE_OP, _dbname, _tableName, _indexlist[i], _replicType, 1 );
		for ( int i = 0; i < numUpdateCols; ++i ) {
			if ( lpindex[setindexnum] ) {
				if ( lpindex[setindexnum]->needUpdate( parseParam->updSetVec[i].colName ) ) {
					++setindexnum; 
					break;
				} else {
					_objectLock->readUnlockIndex( JAG_UPDATE_OP, _dbname, _tableName, lpindex[setindexnum]->getIndexName(), _replicType, 1 );
				}
			}
		}
	}
	***/

	setindexnum = 0;
	int lockrc;

	for ( int i = 0; i < numIndexes; ++i ) {

        dn("s0202901001 i=%d writeLockIndex _tableName=%d index=%s  ...", i, _tableName.s(), _indexlist[i].s() );
		lpindex[i] = _objectLock->writeLockIndex( JAG_UPDATE_OP, _dbname, _tableName, _indexlist[i], 
                                                  _tableschema, _indexschema, _replicType, true, lockrc );
        dn("s0202901001 i=%d writeLockIndex _tableName=%d index=%s lockrc=%d done", i, _tableName.s(), _indexlist[i].s(), lockrc );

		if ( lpindex[i] ) {
			bool needUpdate = false;
			for ( int j = 0; j < numUpdateCols; ++j ) {
				if ( lpindex[i]->needUpdate( parseParam->updSetVec[j].colName ) ) {
					needUpdate = true;
				}
			}
			if ( ! needUpdate ) {
                dn("s202228550  ! needUpdate writeUnlockIndex %s", _indexlist[i].s() );
				_objectLock->writeUnlockIndex( JAG_UPDATE_OP, _dbname, _tableName, _indexlist[i], _replicType, true );
				lpindex[i] = NULL;
			} else {
				++ setindexnum;
			}
		} 
	}

	// if setindexnum == 0: then all index locks are freeed, and lpindex[i] set to NULL

	// parse tree		
	int keylen[1];
	int numKeys[1];	
	JagMinMax minmax[1];	
	keylen[0] = _KEYLEN;
	numKeys[0] = _numKeys;
	minmax[0].setbuflen( keylen[0] );

	ExprElementNode *root = parseParam->whereVec[0].tree->getRoot();
	rc = root->setWhereRange( maps, attrs, keylen, numKeys, 1, uniqueAndHasValueCol, minmax, treestr, typeMode, tabnum );
	if ( rc < 0 ) {
		for ( int i = 0; i < numIndexes; ++i ) {
			if ( ! lpindex[i] ) continue;
            dn("s20524550  writeUnlockIndex %s ", _indexlist[i].s() );
			_objectLock->writeUnlockIndex( JAG_UPDATE_OP, _dbname, _tableName, _indexlist[i], _replicType, true );
		}
		errmsg = Jstr("E10385 Invalid where range found");
		return -1;
	}

	if ( 0 == rc ) {
		memset( minmax[0].minbuf, 0, keylen[0]+1 );
		memset( minmax[0].maxbuf, 255, keylen[0] );
		(minmax[0].maxbuf)[keylen[0]] = '\0';
	} 

	char 	*tableoldbuf = (char*)jagmalloc(_KEYVALLEN+1);
	char 	*tablenewbuf = (char*)jagmalloc(_KEYVALLEN+1);
    
	memset( tableoldbuf, 0, _KEYVALLEN+1 );
	memset( tablenewbuf, 0, _KEYVALLEN+1 );
	buffers[0] = tableoldbuf;

	// get a list of auto-update time columns
	JagVector<JagValInt> auto_update_vec;
	char spare4;
	int totCols = _tableRecord.columnVector->size();
	char  ntbuf[48];

	for ( int i = 0; i < totCols; i ++ ) {
		spare4 =  (*(_tableRecord.columnVector))[i].spare[4];
		if ( isAutoUpdateTime(spare4) ) {
			JagValInt vi;
			JagTime::getNowTimeBuf(spare4, ntbuf);
			vi.val = ntbuf;
			vi.idx = i;
			vi.name = (*(_tableRecord.columnVector))[i].name.s();
			auto_update_vec.push_back(vi);
		}
	}

	if ( memcmp(minmax[0].minbuf, minmax[0].maxbuf, _KEYLEN) == 0 ) {
		// single record update
		singleInsert = true;
		d("s244377 single record update\n");
		JagFixString getkey ( minmax[0].minbuf, _KEYLEN, _KEYLEN );
		JagDBPair getpair( getkey );

		JagDBPair setpair;

		d("s441011 single update _darrFamily->setWithRange() \n");
		//setpair.printkv();

		// update data record to family
		if ( _darrFamily->setWithRange( req, getpair, buffers, uniqueAndHasValueCol, root, parseParam, 
								 		_numKeys, _schAttr, setposlist, setpair, auto_update_vec ) ) {
			// out pair is db format
			d("s333400 single setWithRange true\n");
			/**
			memcpy(tableoldbuf, getpair.key.s(), _KEYLEN);
			memcpy(tableoldbuf+KEYLEN, getpair.value.s(), VALLEN);
			memcpy(tablenewbuf, setpair.key.s(), KEYLEN);
			memcpy(tablenewbuf+KEYLEN, setpair.value.s(), VALLEN);
			**/
			++cnt;
		} else {
			d("s333401 setWithRange false\n");
		}
		
		if ( cnt && numIndexes > 0 && setindexnum ) {
			memcpy(tableoldbuf, getpair.key.s(), _KEYLEN);
			memcpy(tableoldbuf+_KEYLEN, getpair.value.s(), _VALLEN);
			memcpy(tablenewbuf, setpair.key.s(), _KEYLEN);
			memcpy(tablenewbuf+_KEYLEN, setpair.value.s(), _VALLEN);

			dbNaturalFormatExchange( tableoldbuf, _numKeys, _schAttr, 0,0, " " ); // db format -> natural format
			dbNaturalFormatExchange( tablenewbuf, _numKeys, _schAttr, 0,0, " " ); // db format -> natural format
			for ( int i = 0; i < numIndexes; ++i ) {
				if ( ! lpindex[i] ) { continue; }
				lpindex[i]->updateFromTable( tableoldbuf, tablenewbuf );
			}
		}

		// if ( cnt < 1 && upsert ) { }
	} else {
		// range update
		d("s244378 multiple record update\n");

		JagMergeReader *ntu = NULL;
		_darrFamily->setFamilyRead( ntu, true, minmax[0].minbuf, minmax[0].maxbuf );

		int idx;
		if ( ntu ) {
			while ( true ) {
				rc = ntu->getNext( tableoldbuf );
				if ( !rc ) { 
					dn("st00123 getNext ending, break here");
					break; 
				}
				++scanned;
				
				dbNaturalFormatExchange( tableoldbuf, _numKeys, _schAttr, 0,0, " " ); // db format -> natural format
				rc = root->checkFuncValid( ntu, maps, attrs, buffers, strres, typeMode, treetype, treelength, needInit, 0, 0 );
				//dn("st66523 checkFuncValid rc=%d", rc );
				if ( rc == 1 ) {

					memcpy(tablenewbuf, tableoldbuf, _KEYVALLEN);

					//dn("st02373 rc==1 look at numUpdateCols=%d", numUpdateCols );
					for ( int i = 0; i < numUpdateCols; ++i ) {
						updroot = parseParam->updSetVec[i].tree->getRoot();
						needInit = true;
						if ( updroot->checkFuncValid( ntu, maps, attrs, buffers, strres, 
													  typeMode, treetype, treelength, needInit, 0, 0 ) == 1 ) {

							memset(tablenewbuf+_schAttr[setposlist[i]].offset, 0, _schAttr[setposlist[i]].length);
							//dn("st023191 formatOneCol i=%d/%d col=[%s] ...", i, numUpdateCols, parseParam->updSetVec[i].colName.s() );

							rc = formatOneCol( req.session->timediff, _servobj->servtimediff, tablenewbuf, 
												strres.s(), errmsg, 
												parseParam->updSetVec[i].colName, _schAttr[setposlist[i]].offset, 
												_schAttr[setposlist[i]].length, _schAttr[setposlist[i]].sig, 
												_schAttr[setposlist[i]].type );
							if ( !rc ) {
								continue;						
							}								
						} else {
							continue;
						}
					}

					// auto time update cols
					for ( int i = 0; i < auto_update_vec.size(); ++i) {
						idx = auto_update_vec[i].idx;
						formatOneCol( req.session->timediff, _servobj->servtimediff, tablenewbuf, 
									auto_update_vec[i].val.s(),
									errmsg, 
									auto_update_vec[i].name,
									_schAttr[idx].offset, _schAttr[idx].length, _schAttr[idx].sig, _schAttr[idx].type );
					}

					dbNaturalFormatExchange( tableoldbuf, _numKeys, _schAttr, 0,0, " " ); // natural format -> db format
					dbNaturalFormatExchange( tablenewbuf, _numKeys, _schAttr, 0,0, " " ); // natural format -> db format

					JagDBPair setpair( tablenewbuf, _KEYLEN,  tablenewbuf+_KEYLEN, _VALLEN, true );

					d("s303390 range _darrFamily->set( setpair )  ...\n");
					// setpair.printkv();

					// update data record in family
					if ( _darrFamily->set( setpair ) ) {
						++cnt;							
						if ( numIndexes  > 0 ) {
							dbNaturalFormatExchange( tableoldbuf, _numKeys, _schAttr, 0,0, " " ); // db format -> natural format
							dbNaturalFormatExchange( tablenewbuf, _numKeys, _schAttr, 0,0, " " ); // db format -> natural format
							for ( int i = 0; i < numIndexes; ++i ) {
								if ( ! lpindex[i] ) continue;
								lpindex[i]->updateFromTable( tableoldbuf, tablenewbuf );
							}
						}
					} else {
						dn("st61102 _darrFamily->set false");
					}
				} else {
					dn("st020383");
				}
			} // end while

			delete ntu;
		}
	}

	if ( tableoldbuf ) free ( tableoldbuf );
	if ( tablenewbuf ) free ( tablenewbuf );

	for ( int i = 0; i < numIndexes; ++i ) {
		if ( ! lpindex[i] ) continue;
        dn("s920003 writeUnlockIndex i=%d, _indexlist[i]=%s", i, _indexlist[i].s() );
		_objectLock->writeUnlockIndex( JAG_UPDATE_OP, _dbname, _tableName, _indexlist[i], _replicType, true );
	}
	
	d("s50284 updated=%lld/scanned=%lld records\n", cnt, scanned );
	return cnt;
}

// return <0 error; >= 0 OK
jagint JagTable::remove( const JagRequest &req, JagParseParam *parseParam, Jstr &errmsg )	
{
	// chain not able to remove
	if ( JAG_CHAINTABLE_TYPE == _objectType ) {
		errmsg = "Chain cannot be deleted";
		return -100;
	}

	int rc, retval, typeMode = 0, tabnum = 0, treelength = 0;
	bool uniqueAndHasValueCol = 0, needInit = true;
	jagint cnt = 0;
	const char *buffers[1];
	char *buf = (char*)jagmalloc(_KEYVALLEN+1);
	Jstr treetype;
	AbaxBuffer bfr;
	memset( buf, 0, _KEYVALLEN+1 );	
	buffers[0] = buf;
	JagFixString strres;
	
	int keylen[1];
	int numKeys[1];
	const JagHashStrInt *maps[1];
	const JagSchemaAttribute *attrs[1];	
	JagMinMax minmax[1];	
	keylen[0] = _KEYLEN;
	numKeys[0] = _numKeys;
	maps[0] = _tablemap;
	attrs[0] = _schAttr;
	minmax[0].setbuflen( keylen[0] );
	JagFixString treestr;

	ExprElementNode *root = parseParam->whereVec[0].tree->getRoot();
	rc = root->setWhereRange( maps, attrs, keylen, numKeys, 1, uniqueAndHasValueCol, minmax, treestr, typeMode, tabnum );
	if ( 0 == rc ) {
		memset( minmax[0].minbuf, 0, keylen[0]+1 );
		memset( minmax[0].maxbuf, 255, keylen[0] );
		(minmax[0].maxbuf)[keylen[0]] = '\0';
	} else if ( rc < 0 ) {
		if ( buf ) free ( buf );
		errmsg = "E0382 invalid range";
		return -1;
	}
	
	if ( memcmp(minmax[0].minbuf, minmax[0].maxbuf, _KEYLEN) == 0 ) {
		//d("s222029 remove single record\n");
		JagDBPair pair( minmax[0].minbuf, _KEYLEN );
		rc = _darrFamily->get( pair );
		if ( rc ) {
			memcpy(buf, pair.key.s(), _KEYLEN);
			memcpy(buf+_KEYLEN, pair.value.s(), _VALLEN);
			dbNaturalFormatExchange( buf, _numKeys, _schAttr, 0,0, " " ); // db format -> natural format
			if ( !uniqueAndHasValueCol ) rc = 1;
			else {
				rc = root->checkFuncValid( NULL, maps, attrs, buffers, strres, typeMode, treetype, treelength, needInit, 0, 0 );
			}
			if ( rc == 1 ) {
				retval = _darrFamily->remove( pair ); // pair is db format, buf is natural format
				if ( retval == 1 ) {
					++cnt;
					_removeIndexRecords( buf );
					removeColFiles(buf );
				}	
			}			
		}
	} else {
		//d("s222039 remove range records ...\n");
		JagMergeReader *ntr = NULL;
		_darrFamily->setFamilyRead( ntr, true, minmax[0].minbuf, minmax[0].maxbuf );

		if ( ntr ) {
			while ( true ) {
				rc = ntr->getNext(buf);
				if ( !rc ) { break; }
				
				dbNaturalFormatExchange( buf, _numKeys, _schAttr, 0,0, " " ); // db format -> natural format
				rc = root->checkFuncValid( ntr, maps, attrs, buffers, strres, typeMode, treetype, treelength, needInit, 0, 0 );
				if ( rc == 1 ) {
					dbNaturalFormatExchange( buf, _numKeys, _schAttr, 0,0, " " ); // natural format -> db format
					JagDBPair pair( buf, _KEYLEN );
					rc = _darrFamily->remove( pair );
					if ( rc ) {
						++cnt;
						dbNaturalFormatExchange( buf, _numKeys, _schAttr, 0,0, " " ); // db format -> natural format
						_removeIndexRecords( buf );
						removeColFiles(buf );
					}
				}
			}
			delete ntr;
		}
	}

	if ( buf ) free ( buf );
	return cnt;
}

// select count  from ...
jagint JagTable::getCount( const char *cmd, const JagRequest &req, JagParseParam *parseParam, Jstr &errmsg )
{
    dn("s203039 getCount _replicType=%d", _replicType );

	if ( parseParam->hasWhere ) {
		JagDataAggregate *jda = NULL;
		jagint cnt = select( jda, cmd, req, parseParam, errmsg, false );
		if ( jda ) delete jda;
		return cnt;
	}
	else {
        dn("s100289 table-this=%p  _darrFamily->getCount() _darrFamily=%p _replicType=%d", this, _darrFamily, _replicType );
		return _darrFamily->getCount( );
	}
}

jagint JagTable::getElements( const char *cmd, const JagRequest &req, JagParseParam *parseParam, Jstr &errmsg )
{
	if ( parseParam->hasWhere ) {
		JagDataAggregate *jda = NULL;
		jagint cnt = select( jda, cmd, req, parseParam, errmsg, false );
		if ( jda ) delete jda;
		return cnt;
	}
	else {
		return _darrFamily->getElements( ); // count from all diskarr
	}
}

// nowherecnt: false if  select count(*) from t123 where
// nowherecnt: true if  select ... from t123 where ... (there is no count in where)
// select and getfile
jagint JagTable::select( JagDataAggregate *&jda, const char *cmd, const JagRequest &req, JagParseParam *parseParam, 
						  Jstr &errmsg, bool nowherecnt, bool isInsertSelect )
{
	// set up timeout for select starting timestamp
	d("s8773 select cmd=[%s]\n", cmd );

	struct 		timeval now;
	bool 		timeoutFlag = 0;
	Jstr 		treetype;
	int 		rc, typeMode = 0, tabnum = 0, treelength = 0;
	bool 		uniqueAndHasValueCol = 0, needInit = true;
	jagint 		nm = parseParam->limit;
	std::atomic<jagint> recordcnt;

	gettimeofday( &now, NULL ); 
	jagint 		bsec = now.tv_sec;

	//d("s87734 _darrFamily->_darrlist.size()=[%d]\n", _darrFamily->_darrlist.size() );
	//d("s87714 _darrFamily->memoryBufferSize().size()=[%d]\n", _darrFamily->memoryBufferSize() );

	d("s222081 parseParam->hasLimit=%d parseParam->limit=%d nowherecnt=%d parseParam=%p\n", parseParam->hasLimit, nm, nowherecnt, parseParam );

	recordcnt = 0;
	if ( parseParam->hasLimit && nm == 0 && nowherecnt ) {
		d("s20171727 here return 0 parseParam->hasLimit && nm == 0 && nowherecnt \n");
		return 0;
	}

	if ( parseParam->exportType == JAG_EXPORT && _isExporting ) {
		d("s222277  parseParam->exportType == JAG_EXPORT && _isExporting return 0\n");
		return 0;
	}

	if ( _darrFamily->_darrlist.size() < 1 && _darrFamily->memoryBufferSize() < 1 ) {
		// check memory part too
	    d("s80910 _darrlist.size < 1 && memBufrSize()<1 return 0 _repType=%d _table=[%s]\n", _replicType, _tableName.s() );
	    return 0;
	} 

    // debugging 
    /**
    dn("s39339 select table:");
    _darrFamily->debugPrintBuffer();
    **/

	if ( parseParam->exportType == JAG_EXPORT ) _isExporting = 1;

	if ( JAG_INSERTSELECT_OP == parseParam->opcode && ! parseParam->hasTimeout ) {
		parseParam->timeout = -1;
	}

	int 		keylen[1];
	int 		numKeys[1];
	const 		JagHashStrInt *maps[1];
	const 		JagSchemaAttribute *attrs[1];
	JagMinMax 	minmax[1];

	Jstr 		newhdr, gbvheader;
	jagint 		finalsendlen = 0;
	jagint 		gbvsendlen = 0;
	JagSchemaRecord nrec;
	JagFixString treestr, strres;
	ExprElementNode *root = NULL;

	keylen[0] = _KEYLEN;
	numKeys[0] = _numKeys;
	maps[0] = _tablemap;
	attrs[0] = _schAttr;

	minmax[0].setbuflen( keylen[0] ); // _KEYLEN
	//d("s2028 minmax[0].setbuflen( keylen=%d )\n", keylen[0] );

	if ( nowherecnt ) {
		// NOT "select count(*) from ...."
		JagVector<SetHdrAttr> hspa;
		SetHdrAttr honespa;
		AbaxString getstr;

		_tableschema->getAttr( _dbtable, getstr );
		honespa.setattr( _numKeys, false, _dbtable, &_tableRecord, getstr.s() );
		hspa.append( honespa );

		d("s5640 nowherecnt rearrangeHdr ...\n" );
		rc = rearrangeHdr( 1, maps, attrs, parseParam, hspa, newhdr, gbvheader, finalsendlen, gbvsendlen );
		if ( !rc ) {
			errmsg = Jstr("E0823 Error header for select [")  + cmd + "] ";
			if ( parseParam->exportType == JAG_EXPORT && _isExporting ) _isExporting = 0;
			return -1;			
		}

		nrec.parseRecord( newhdr.s() );
		d("s0573 nowherecnt newhdr=[%s]\n",  newhdr.s() );
		//nrec.print(); 
	}
	
	d("s92830 parseParam->hasWhere=%d\n", parseParam->hasWhere );
	if ( parseParam->hasWhere ) {
		root = parseParam->whereVec[0].tree->getRoot();
		d("s334087 root->setWhereRange() ...\n");
		rc = root->setWhereRange( maps, attrs, keylen, numKeys, 1, uniqueAndHasValueCol, minmax, 
								  treestr, typeMode, tabnum );
		d("s83734 tab select setWhereRange done rc=%d\n", rc );
		if ( 0 == rc ) {
			// not able to determine min max
			d("s20097 not able to determine min max\n");
			memset( minmax[0].minbuf, 0, keylen[0]+1 );
			memset( minmax[0].maxbuf, 255, keylen[0] );
			(minmax[0].maxbuf)[keylen[0]] = '\0';
		} else if (  rc < 0 ) {
			if ( parseParam->exportType == JAG_EXPORT && _isExporting ) _isExporting = 0;
			errmsg = "E0825 Error where range";
			return -1;
		}

		#if 0
		dn("s7430 dumpmem minbuf:" ); 
		dumpmem( minmax[0].minbuf, keylen[0], true );
		dn("s7431 dumpmem maxbuf:" );
		dumpmem( minmax[0].maxbuf, keylen[0], true );
		#endif
	}
	
	// finalbuf, hasColumn len or KEYVALLEN if !hasColumn
	// gbvbuf, if has group by
	char *finalbuf = (char*)jagmalloc(finalsendlen+1);
	memset(finalbuf, 0, finalsendlen+1);

	//char *gbvbuf = (char*)jagmalloc(gbvsendlen+1);
	//memset(gbvbuf, 0, gbvsendlen+1);
	char *gbvbuf = NULL;

	d("s1028 finalsendlen=%d gbvsendlen=%d\n", finalsendlen, gbvsendlen );
	JagMemDiskSortArray *gmdarr = NULL;
	if ( gbvsendlen > 0 ) {
		gmdarr = newObject<JagMemDiskSortArray>();
		int sortmb = atoi((_cfg->getValue("GROUPBY_SORT_SIZE_MB", "1024")).s()); 
		gmdarr->init( sortmb, gbvheader.s(), "GroupByValue" );
		gmdarr->beginWrite();
	}
	
	// if insert into ... select syntax, create cpp client object to send insert cmd to corresponding server
	JaguarCPPClient *pcli = NULL;

	if ( JAG_INSERTSELECT_OP == parseParam->opcode ) {
		pcli = newObject<JaguarCPPClient>();
		Jstr host = "localhost", unixSocket = Jstr("/TOKEN=") + _servobj->_servToken;
		if ( _servobj->_listenIP.size() > 0 ) { host = _servobj->_listenIP; }
		if ( !pcli->connect( host.s(), _servobj->_port, "admin", "anon", "test", unixSocket.s(), 0 ) ) {
			jd(JAG_LOG_LOW, "s4055 Connect (%s:%s) (%s:%d) error [%s], retry ...\n",
					  "admin", "jaguar", host.s(), _servobj->_port, pcli->error() );
			pcli->close();
			if ( pcli ) delete pcli;
			if ( gmdarr ) delete gmdarr;
			if ( finalbuf ) free ( finalbuf );
			if ( gbvbuf ) free ( gbvbuf );
			errmsg = "E0826 Error connection";
			return -1;
		}
	}
	
	// Point query, one record
	if ( memcmp(minmax[0].minbuf, minmax[0].maxbuf, _KEYLEN) == 0 ) {

        dn("tab02219 point query");

		JagDBPair pair( minmax[0].minbuf, _KEYLEN );

        /**
        dn("tab8800 point query pair.key:");
        dumpmem(pair.key.s(), pair.key.size() );
        **/

        // pair.printkv();

		if ( _darrFamily->get( pair ) ) {
			// get point data from family 
            dn("tab00238 got data from darrfam");

			const char *buffers[1];
			char *buf = (char*)jagmalloc(_KEYVALLEN+1);
			memset( buf, 0, _KEYVALLEN+1 );	
			memcpy(buf, pair.key.s(), _KEYLEN);
			memcpy(buf+_KEYLEN, pair.value.s(), _VALLEN);
			buffers[0] = buf;

			//Jstr hdir = fileHashDir( pair.key );

			dbNaturalFormatExchange( buf, _numKeys, _schAttr, 0,0, " " ); // db format -> natural format

			if ( !uniqueAndHasValueCol ||


				( root && root->checkFuncValid( NULL, maps, attrs, buffers, strres, typeMode, treetype, 
											    treelength, needInit, 0, 0 ) == 1 ) ) {

                Jstr hdir;

				if ( parseParam->opcode == JAG_GETFILE_OP ) { 

                    dn("tab303828 getFileHashDir...");
                    hdir = getFileHashDir( _tableRecord, pair.key );

					setGetFileAttributes( hdir, parseParam, buffers ); 
				}

				nonAggregateFinalbuf( NULL, maps, attrs, &req, buffers, parseParam, finalbuf, finalsendlen, jda, 
									  _dbtable, recordcnt, nowherecnt, NULL, true );

				if ( parseParam->opcode == JAG_GETFILE_OP && parseParam->getFileActualData ) {
                    // get file  .. into ... (there is into to get file actual data)
                    //dn("tab60192 fileHashDir pair.key:");
                    //dumpmem( pair.key.s(), pair.key.size() );

					//Jstr    hdir = fileHashDir( pair.key );
                    //Jstr hdir = getFileHashDir( _tableRecord, pair.key );

					Jstr    ddcol, inpath; 
					char    fname[JAG_FILE_FIELD_LEN+1];
                    int     getpos, actualSent = 0;

					for ( int i = 0; i < parseParam->selColVec.size(); ++i ) {
						ddcol = _dbtable + "." + parseParam->selColVec[i].getfileCol.s();
						if ( _tablemap->getValue(ddcol, getpos) ) {
                            memset( fname, 0, JAG_FILE_FIELD_LEN+1 );
							memcpy( fname, buffers[0]+_schAttr[getpos].offset, _schAttr[getpos].length );
							inpath = _darrFamily->_sfilepath + "/" + hdir + "/" + fname;

							d("s52118 oneFileSender %s ...\n", inpath.s() );
							req.session->active = false;

							oneFileSender( req.session->sock, inpath, 
                                           parseParam->objectVec[0].dbName, parseParam->objectVec[0].tableName, hdir, actualSent );
                            // int oneFileSender( JAGSOCK sock, const Jstr &inpath, const Jstr &jagHome, bool tryPNdata, int &actualSent )
                            dn("s452200801 oneFileSender %s actualSent=%d", inpath.s(), actualSent );

							req.session->active = true;
						}
					}
				} else {
					if ( JAG_INSERTSELECT_OP == parseParam->opcode ) {
						Jstr iscmd;
						if ( formatInsertSelectCmdHeader( parseParam, iscmd ) ) {
							//d("s02938  formatInsertFromSelect ...\n");
							formatInsertFromSelect( parseParam, attrs[0], finalbuf, buffers[0], finalsendlen, 
												    _numCols, pcli, iscmd );
						}
					} else {
						// jd(JAG_LOG_HIGH, "s5541 opcode=%d\n", parseParam->opcode  );
					}
				}
			} else {
			}

			free( buf );
		} else {
			dn("s488501 getpair failed");
		}
	} else { // range query
		jagint  callCounts = -1, lastBytes = 0;

		if ( JAG_INSERTSELECT_OP != parseParam->opcode ) {
			if ( !jda ) jda = newObject<JagDataAggregate>();

            // 2/17/2023
            jda->_keylen = _KEYLEN;
            jda->_vallen = _VALLEN;
            jda->_datalen = _KEYVALLEN;

			d("s222081 jda->setwrite  parseParam->exportType=%d JAG_EXPORT=%d\n", parseParam->exportType, JAG_EXPORT );
			jda->setwrite( _dbtable, _dbtable, parseParam->exportType == JAG_EXPORT );  // to /export file or not
			jda->setMemoryLimit( _darrFamily->getElements()*_KEYVALLEN*2 );
		}

		int numBatches = 1;
        /***
        bool lcpu = false;
        numBatches = _darrFamily->_darrlist.size()/_servobj->_numCPUs;

        if ( numBatches < 1 ) {
            numBatches = _darrFamily->_darrlist.size();
            lcpu = true;
            if ( numBatches < 1 ) { numBatches = 1; }
        }
        ***/

		JagParseParam *pparam[numBatches];
		JagParseAttribute jpa( _servobj, req.session->timediff, _servobj->servtimediff, req.session->dbname, _servobj->_cfg );

		if ( ! parseParam->hasGroup ) {
			// has no "group by"
			// check if has aggregate
			bool hAggregate = false;
			if ( parseParam->hasColumn ) {
				for ( int i = 0; i < parseParam->selColVec.size(); ++i ) {
					if ( parseParam->selColVec[i].isAggregate ) {
						hAggregate = true; break;
					}
				}
			}

			JagParser parser((void*)NULL);

			for ( jagint i = 0; i < numBatches; ++i ) {
				pparam[i] = new JagParseParam(&parser);
				parser.parseCommand( jpa, cmd, pparam[i], errmsg);
			    pparam[i]->_parent = parseParam;
			}

			jagint memlim = availableMemory( callCounts, lastBytes )/8/numBatches/1024/1024;
			if ( memlim <= 0 ) memlim = 1;

			ParallelCmdPass psp[numBatches];
			for ( jagint i = 0; i < numBatches; ++i ) {
				psp[i].cli = pcli;

				#if 1
				psp[i].ptab = this;
				psp[i].pindex = NULL;
				psp[i].pos = i;
				psp[i].sendlen = finalsendlen;
				psp[i].parseParam = pparam[i];
				psp[i].gmdarr = NULL;
				psp[i].req = (JagRequest*)&req;
				psp[i].jda = jda;
				psp[i].writeName = _dbtable;
				psp[i].recordcnt = &recordcnt;
				psp[i].nrec = &nrec;
				psp[i].actlimit = nm;
				psp[i].nowherecnt = nowherecnt;
				psp[i].memlimit = memlim;
				psp[i].minbuf = minmax[0].minbuf;
				psp[i].maxbuf = minmax[0].maxbuf;
				psp[i].starttime = bsec;
				psp[i].kvlen = _KEYVALLEN;

                /***
                if ( lcpu ) {
					psp[i].spos = i; 
					psp[i].epos = i;
				} else {
					psp[i].spos = i*_servobj->_numCPUs;
					psp[i].epos = psp[i].spos + _servobj->_numCPUs-1;
					if ( i == numBatches-1 ) {
						psp[i].epos = _darrFamily->_darrlist.size()-1;
					}
				}
                ***/

				psp[i].spos = 0;
				psp[i].epos = _darrFamily->_darrlist.size()-1;

				#else
				d("s6513 psp[%d]:  ptab=%0x parseParam=%0x gmdarr=%0x jda=%0x cnt=%d minbuf=%0x maxbuf=%0x \n",
					 i,  psp[i].ptab, psp[i].parseParam, psp[i].gmdarr, psp[i].jda, *psp[i].cnt, psp[i].nrec, 
					 	psp[i].minbuf, psp[i].maxbuf );
				d("s6712 gbvsendlen=%d\n", gbvsendlen );
				fillCmdParse( JAG_TABLE, this, i, gbvsendlen, pparam, 0, lgmdarr, req, jda, _dbtable,  
							 cnt, nm, nowherecnt, nrec, memlim, minmax, 
							 bsec, KEYVALLEN, _servobj, numBatches, _darrFamily, lcpu );
				d("s6514 psp[%d]:  ptab=%0x parseParam=%0x gmdarr=%0x jda=%0x cnt=%d minbuf=%0x maxbuf=%0x \n",
					 i,  psp[i].ptab, psp[i].parseParam, psp[i].gmdarr, psp[i].jda, *psp[i].cnt, psp[i].nrec, 
					 psp[i].minbuf, psp[i].maxbuf );
				#endif


				if ( JAG_INSERTSELECT_OP == parseParam->opcode && ! parseParam->hasTimeout ) {
					pparam[i]->timeout = -1;
				}

                if ( 0 == i ) {
                    psp[i].useInsertBuffer = true;
                } else {
                    psp[i].useInsertBuffer = false;
                }

				parallelSelectStatic( (void*)&psp[i] );  // no group by

			} // end of for i in numBatches

			for ( jagint i = 0; i < numBatches; ++i ) {
				if ( psp[i].timeoutFlag ) {
					dn("s056577 timeoutFlag=1");
					timeoutFlag = 1;
				}
			}

			if ( hAggregate ) {
				aggregateFinalbuf( &req, newhdr, numBatches, pparam, finalbuf, finalsendlen, jda, _dbtable, 
								   recordcnt, nowherecnt, &nrec );
			}
		
			for ( jagint i = 0; i < numBatches; ++i ) {
				delete pparam[i];
			}	

			// end of regular select, without group by
		} else {
			// has "group by", no insert into ... select ... syntax allowed
			d("s20393 has groupby numBatches=%d ...\n", numBatches );
			JagMemDiskSortArray *lgmdarr[numBatches];

			JagParser parser((void*)NULL);

			for ( jagint i = 0; i < numBatches; ++i ) {
				pparam[i] = new JagParseParam(&parser);
				d("s40038 parser.parseCommand\n");
				parser.parseCommand( jpa, cmd, pparam[i], errmsg );
			    pparam[i]->_parent = parseParam;

				lgmdarr[i] = newObject<JagMemDiskSortArray>();
				lgmdarr[i]->init( 40, gbvheader.s(), "GroupByValue" );
				lgmdarr[i]->beginWrite();
			}

			jagint memlim = availableMemory( callCounts, lastBytes )/8/numBatches/1024/1024;
			if ( memlim <= 0 ) memlim = 1;
			
			ParallelCmdPass psp[numBatches];
			for ( int i = 0; i < numBatches; ++i ) {
				#if 1
				psp[i].ptab = this;
				psp[i].pindex = NULL;
				psp[i].pos = i;
				psp[i].sendlen = gbvsendlen;
				psp[i].parseParam = pparam[i];
				psp[i].gmdarr = lgmdarr[i];
				psp[i].req = (JagRequest*)&req;
				psp[i].jda = jda;
				psp[i].writeName = _dbtable;
				psp[i].recordcnt = &recordcnt;
				psp[i].actlimit = nm;
				psp[i].nowherecnt = nowherecnt;
				psp[i].nrec = &nrec;
				psp[i].memlimit = memlim;
				psp[i].minbuf = minmax[0].minbuf;
				psp[i].maxbuf = minmax[0].maxbuf;
				psp[i].starttime = bsec;
				psp[i].kvlen = _KEYVALLEN;

                /***
				if ( lcpu ) {
					psp[i].spos = i; psp[i].epos = i;
				} else {
					psp[i].spos = i*_servobj->_numCPUs;
					psp[i].epos = psp[i].spos+_servobj->_numCPUs-1;
					if ( i == numBatches-1 ) psp[i].epos = _darrFamily->_darrlist.size()-1;
				}
                ***/

				psp[i].spos = 0;
				psp[i].epos = _darrFamily->_darrlist.size()-1;

				#else
				d("s6711 gbvsendlen=%d\n", gbvsendlen );
				fillCmdParse( JAG_TABLE, this, i, gbvsendlen, pparam, 1, lgmdarr, req, jda, _dbtable,  
								recordcnt, nm, nowherecnt, nrec, memlim, minmax, bsec, KEYVALLEN, 
								_servobj, numthreads, _darrFamily, lcpu );

				d("s6714 psp[%d]:  ptab=%0x parseParam=%0x gmdarr=%0x jda=%0x cnt=%0x minbuf=%0x maxbuf=%0x \n",
					 i,  psp[i].ptab, psp[i].parseParam, psp[i].gmdarr, psp[i].jda, psp[i].cnt, 
					 psp[i].nrec, psp[i].minbuf, psp[i].maxbuf );
				#endif

				if ( JAG_INSERTSELECT_OP == parseParam->opcode && ! parseParam->hasTimeout ) {
					pparam[i]->timeout = -1;
				}

                if ( 0 == i ) {
                    psp[i].useInsertBuffer = true;
                } else {
                    psp[i].useInsertBuffer = false;
                }

				parallelSelectStatic( (void*)&psp[i] );  // has group by
			}

			gbvbuf = (char*)jagmalloc(gbvsendlen+1);
			memset(gbvbuf, 0, gbvsendlen+1);

			for ( jagint i = 0; i < numBatches; ++i ) {
				if ( psp[i].timeoutFlag ) timeoutFlag = 1;
				lgmdarr[i]->endWrite();
				lgmdarr[i]->beginRead();

				while ( 1 ) {
					rc = lgmdarr[i]->get( gbvbuf );
					if ( !rc ) break;
					JagDBPair pair(gbvbuf, gmdarr->_keylen, gbvbuf+gmdarr->_keylen, gmdarr->_vallen, true );
					rc = gmdarr->groupByUpdate( pair );
				}
				lgmdarr[i]->endRead();
				delete lgmdarr[i];
				delete pparam[i];
			}

			groupByFinalCalculation( gbvbuf, nowherecnt, finalsendlen, recordcnt, nm, _dbtable, 
									 parseParam, jda, gmdarr, &nrec );
		} 
		
		//delete selectPool;

		if ( jda ) {
			//d("s5003 jda->flushwrite() ...\n" );
			jda->flushwrite();
			//d("s5003 jda->flushwrite() done ...\n" );
		}
	}

	if ( timeoutFlag ) {
		if ( _tableName != "_SYS_" ) {
			Jstr timeoutStr = "E0283 Table select has timed out. Results have been truncated;";
			sendER( req, timeoutStr);
		}
	}
	
	if ( parseParam->exportType == JAG_EXPORT ) recordcnt = 0;
	if ( parseParam->exportType == JAG_EXPORT && _isExporting ) _isExporting = 0;
	if ( pcli ) {
		pcli->close();
		delete pcli;
	}
	if ( gmdarr ) delete gmdarr;
	if ( finalbuf ) free ( finalbuf );
	if ( gbvbuf ) free ( gbvbuf );

	//d("s611273 JagTable select recordcnt=%d\n", (int)recordcnt );
	return (jagint)recordcnt;
}

// keystr is binary data of keys
// if keystr exists return true; else false 
bool JagTable::chkkey( const Jstr &keystr )
{
	JagDBPair pair( keystr.c_str(), _KEYLEN );
	if ( _darrFamily->exist( pair ) ) {
		d("s873073 chkkey keystr=[%s] true", keystr.s() );
		return true;
	} 

	d("s873073 chkkey keystr=[%s] false", keystr.s() );
	return false;
}

// static select function
void *JagTable::parallelSelectStatic( void * ptr )
{

	ParallelCmdPass *pass = (ParallelCmdPass*)ptr;
    dn("s602777 JagTable::parallelSelectStatic() ptr->useInsertBuffer=%d ...", pass->useInsertBuffer );

	ExprElementNode *root;
	const JagHashStrInt *maps[1];
	const JagSchemaAttribute *attrs[1];	
	int 			rc, collen, siglen, constMode = 0, typeMode = 0, treelength = 0, tabnum = 0;
	bool 			isAggregate, hasAggregate = false, hasFirst = false, needInit = true, uniqueAndHasValueCol;
	jagint 			offset = 0;
	int 			numCols[1];
	int 			numKeys[1];
	int 			keylen[1];
	JagMinMax 		minmax[1];
	JagFixString 	strres, treestr;
	Jstr 			treetype;

	JagTable 		*ptab = pass->ptab;
	JagIndex        *pindex = pass->pindex;
	JagParseParam   *parseParam = pass->parseParam;

	if ( ptab ) {
		numCols[0] = ptab->_numCols;
		numKeys[0] = ptab->_numKeys;
		maps[0] = ptab->_tablemap;
		attrs[0] = ptab->_schAttr;
		keylen[0] = ptab->_KEYLEN;
		minmax[0].setbuflen( keylen[0] );
	} else {
		numCols[0] = pindex->_numCols;
		numKeys[0] = pindex->_numKeys;
		maps[0] = pindex->_indexmap;
		attrs[0] = pindex->_schAttr;
		keylen[0] = pindex->_KEYLEN;
		minmax[0].setbuflen( keylen[0] );
	}

	// set param select tree and where tree, if needed
	if ( parseParam->hasColumn ) {
		d("s8722 pass->parseParam->hasColumn\n" );
		for ( int i = 0; i < parseParam->selColVec.size(); ++i ) {
			isAggregate = false;
			root = parseParam->selColVec[i].tree->getRoot();
			rc = root->setFuncAttribute( maps, attrs, constMode, typeMode, isAggregate, treetype, collen, siglen );
			d("s2390 setFuncAttribute rc=%d collen=%d\n", rc, collen );
			if ( 0 == rc ) {
				d("s3006 error setFuncAttribute()\n");
				return NULL;
			}

			parseParam->selColVec[i].offset = offset;
			parseParam->selColVec[i].length = collen;
			parseParam->selColVec[i].sig = siglen;
			parseParam->selColVec[i].type = treetype;
			parseParam->selColVec[i].isAggregate = isAggregate;
			if ( isAggregate ) hasAggregate = true;
			offset += collen;

			dn("s276235 parseParam->selColVec i=%d name=[%s] asName=[%s] offset=%d length=%d", 
			    i, parseParam->selColVec[i].name.s(), parseParam->selColVec[i].asName.s(), offset, collen );
		}
	}

	if ( parseParam->hasWhere ) {
		d("s0377 pass->parseParam->hasWhere \n" );
		root = parseParam->whereVec[0].tree->getRoot();
		rc = root->setWhereRange( maps, attrs, keylen, numKeys, 1, uniqueAndHasValueCol, minmax, 
							      treestr, typeMode, tabnum );
		if (  rc < 0 ) {
			d("E3006 Error where range");
			return NULL;
		}
	}
	
	d("s20487 before formatInsertSelectCmdHeader...\n");

	Jstr iscmd;
	formatInsertSelectCmdHeader( parseParam, iscmd );
	d("s20488 after formatInsertSelectCmdHeader...\n");

	char *buf = (char*)jagmalloc(pass->kvlen+1); 
	char *sendbuf = (char*)jagmalloc(pass->sendlen+1); 

	memset(buf, 0, pass->kvlen+1);
	memset(sendbuf, 0, pass->sendlen+1);
	//d("s8333092 kvlen=%d sendlen=%d\n",  pass->kvlen, pass->sendlen );

	const char *buffers[1]; 
	buffers[0] = buf;

	JagDiskArrayFamily *darrfam = NULL;

	if ( ptab ) darrfam = ptab->_darrFamily;
	else darrfam = pindex->_darrFamily;
	
	if ( parseParam->hasOrder && !parseParam->orderVec[0].isAsc ) {
		// order by col1 desc  descending
		JagMergeBackReader *ntr = NULL;
		d("s22039 setFamilyReadBackPartial order desc setFamilyReadBackPartial\n");
		darrfam->setFamilyReadBackPartial( ntr, pass->useInsertBuffer, pass->minbuf, pass->maxbuf, pass->spos, pass->epos, pass->memlimit );
	
		if ( ntr != NULL ) {

			while ( 1 ) {
				if ( !parseParam->hasExport && checkCmdTimeout( pass->starttime, parseParam->timeout ) ) {
					dn("s039388 timeoutFlag=1");
					pass->timeoutFlag = 1;
					break;
				}

				rc = ntr->getNext( buf );  ////// read a new row

                //dn("s092210 JagMergeBackReader getNext buf:");
                //dumpmem( buf, pass->kvlen, true );

				if ( pass->req->session->sessionBroken ) rc = false;
				if ( !rc ) { break; }

				dbNaturalFormatExchange( buf, numKeys[0], attrs[0], 0,0, " " ); // db format -> natural format
				if ( parseParam->hasWhere ) {
					root = parseParam->whereVec[0].tree->getRoot();
					rc = root->checkFuncValid( ntr, maps, attrs, buffers, strres, typeMode, treetype, treelength, needInit, 0, 0 );
				} else {
					rc = 1;
				}

				if ( rc == 1 ) {
					// buf: if pass->parseParam.window.size() > 0 : converToWindow()
					if ( parseParam->window.size() > 0 ) {
						Jstr period, pcolName;
						bool prc =  parseParam->getWindowPeriod( pass->parseParam->window, period, pcolName );
						if ( prc ) {
							d("s552238 convertTimeToWindow period=[%s] pcolName=[%s]\n", period.s(), pcolName.s() );
                            if ( ptab ) {
							    ptab->convertTimeToWindow( pass->req->session->timediff, period, buf, pcolName );
                            }
						}
					}

					if ( pass->gmdarr ) { // has group by
						rc = JagTable::buildDiskArrayForGroupBy( ntr, maps, attrs, pass->req, buffers, 
																 parseParam, pass->gmdarr, sendbuf );
						if ( 0 == rc ) break;
					} else { // no group by
						if ( hasAggregate ) { // has aggregate functions
							JagTable::aggregateDataFormat( ntr, maps, attrs, pass->req, buffers, pass->parseParam, !hasFirst );
						} else { // non aggregate functions
							JagTable::nonAggregateFinalbuf( ntr, maps, attrs, pass->req, buffers, pass->parseParam, 
												            sendbuf, pass->sendlen, pass->jda, pass->writeName, *(pass->recordcnt), 
															pass->nowherecnt, pass->nrec, false );
							if ( JAG_INSERTSELECT_OP == parseParam->opcode ) {
								//d("s202299 formatInsertFromSelect \n");
								JagTable::formatInsertFromSelect( parseParam, attrs[0], sendbuf, buffers[0], pass->sendlen, numCols[0], 
																  pass->cli, iscmd );
							}

							if ( parseParam->hasLimit && *(pass->recordcnt) >= pass->actlimit ) {
								d("s200992 order  pass->parseParam->hasLimit pass->recordcnt=%d pass->actlimit=%d  break\n", 
										(int)*(pass->recordcnt), pass->actlimit );
								break;
							}
						}
					}
					hasFirst = true;
				}	
			}
		}
		if ( ntr ) delete ntr;
	} else {
		// no order or order by asc
		//d("s8302 no order range select pass->spos=%d  pass->epos=%d\n", pass->spos, pass->epos );
        dn("s817272 select has no order by pass->useInsertBuffer=%d", pass->useInsertBuffer);

		JagMergeReader *ntr = NULL;

        darrfam->setFamilyReadPartial( ntr, pass->useInsertBuffer, pass->minbuf, pass->maxbuf, pass->spos, pass->epos, pass->memlimit );
		//d("s8303 setFamilyReadPartial dn=one ntr=%0x\n", ntr );

		if ( ntr ) {
			d("s8305 initHeap done this->ntr=%p\n", ntr);

			while ( 1 ) {

				if ( !parseParam->hasExport && checkCmdTimeout( pass->starttime, parseParam->timeout ) ) {
					d("s1929 timeoutFlag set to 1  pass->starttime=%ld parseParam->timeout=%ld\n", 
						  pass->starttime, parseParam->timeout );
					pass->timeoutFlag = 1;
					break;
				}

				d("s81307 before getNext() \n");
				rc = ntr->getNext( buf );  // read a new row
                if ( rc ) {
				    d("s31366 ntr->getNext( buf ) buf=[%s] rc=%d\n", buf, rc );
                    /**
                    dn("s0811350 dump read buf: keylen=%d KLEN=%d KVLEN=%d", keylen[0], ntr->KEYLEN, ntr->KEYVALLEN );
                    dumpmem( buf, keylen[0]);
                    dumpmem( buf, ntr->KEYVALLEN);
                    **/
                } else {
				    d("s31368 ntr->getNext( buf ) buf=[] rc=%d\n", rc );
                }

				if ( pass->req->session->sessionBroken ) {
                    rc = false;
                }

				if ( !rc ) {
					if ( parseParam->hasWhere ) {
						root = parseParam->whereVec[0].tree->getRoot();
    					if ( root && root->_builder->_pparam->_rowHash ) {
    						d("s2042 root->_builder->_pparam->_rowHash=%0x\n", root->_builder->_pparam->_rowHash );
    						Jstr str =  root->_builder->_pparam->_rowHash->getKVStrings("#");
							if ( ptab ) {
								JAG_BLURT jaguar_mutex_lock ( &ptab->_parseParamParentMutex ); JAG_OVER
							} else {
								JAG_BLURT jaguar_mutex_lock ( &pindex->_parseParamParentMutex ); JAG_OVER
							}

    						if ( ! parseParam->_parent->_lineFile ) {
    							parseParam->_parent->_lineFile = new JagLineFile();
    						} 

    						dn("s2138 _lineFile->append(str=%s) _lineFile=%p", str.s(), parseParam->_parent->_lineFile );
    						parseParam->_parent->_lineFile->append( str );

							if ( ptab ) {
								JAG_BLURT jaguar_mutex_unlock ( &ptab->_parseParamParentMutex ); JAG_OVER
							} else {
								JAG_BLURT jaguar_mutex_unlock ( &pindex->_parseParamParentMutex ); JAG_OVER
							}
    					} 
					}

					break; 
				}

				dbNaturalFormatExchange( buf, numKeys[0], attrs[0],0,0, " " ); // db format -> natural format
				if ( parseParam->hasWhere ) {
					root = parseParam->whereVec[0].tree->getRoot();
					/***
					d("s302933 after tree->getRoot root=%0x\n", root );
					root->print();
					d("\n");
					***/
					rc = root->checkFuncValid( ntr, maps, attrs, buffers, strres, typeMode, treetype, treelength, needInit, 0, 0 );
					d("s2021 checkFuncValid rc=%d root=%0x strres=[%s]\n", rc, root, strres.s() );

					if ( root->_builder->_pparam->_rowHash ) {
						d("s2022 root->_builder->_pparam->_rowHash=%0x\n", root->_builder->_pparam->_rowHash );
						if ( rc ) {
							Jstr str =  root->_builder->_pparam->_rowHash->getKVStrings("#");
							if ( ptab ) {
								JAG_BLURT jaguar_mutex_lock ( &ptab->_parseParamParentMutex ); JAG_OVER
							} else {
								JAG_BLURT jaguar_mutex_lock ( &pindex->_parseParamParentMutex ); JAG_OVER
							}

							if ( ! parseParam->_parent->_lineFile ) {
								parseParam->_parent->_lineFile = newObject<JagLineFile>();
							} 

							dn("s2038 _lineFile->append(%s) _lineFile=%p", str.s(), parseParam->_parent->_lineFile );
							parseParam->_parent->_lineFile->append( str );
							if ( ptab ) {
								JAG_BLURT jaguar_mutex_unlock ( &ptab->_parseParamParentMutex ); JAG_OVER
							} else {
								JAG_BLURT jaguar_mutex_unlock ( &pindex->_parseParamParentMutex ); JAG_OVER
							}
						}
					} else {
						d("s2023 root->_builder->_pparam->_rowHash=NULL root=%0x\n", root );
					}
				} else {
					// no where in select
					rc = 1;
				}

				if ( rc == 1 ) {
					// buf: if pass->parseParam.window.size() > 0 : converToWindow()
					if ( parseParam->window.size() > 0 ) {
						Jstr period, pcolName;
						bool prc =  parseParam->getWindowPeriod( parseParam->window, period, pcolName );
						if ( prc ) {
							d("s552238 convertTimeToWindow period=[%s] pcolName=[%s]\n", period.s(), pcolName.s() );
                            if ( ptab ) {
							    ptab->convertTimeToWindow( pass->req->session->timediff, period, buf, pcolName );
                            }
						}
					}

					if ( pass->gmdarr ) { // has group by
						d("s2112034 gmdarr not null, has group by\n" );
						rc = JagTable::buildDiskArrayForGroupBy( ntr, maps, attrs, pass->req, buffers, 
															     pass->parseParam, pass->gmdarr, sendbuf );
						if ( 0 == rc ) break;
					} else { // no group by
						if ( hasAggregate ) { // has aggregate functions
							JagTable::aggregateDataFormat( ntr, maps, attrs, pass->req, buffers, pass->parseParam, !hasFirst );
						} else { // non aggregate functions
							//d("s0296 JagTable::nonAggregateFinalbuf recordcnt=%d ...\n", (int)*(pass->recordcnt) );
                            dn("tab0913001 no agg read");

                            if ( parseParam->opcode == JAG_GETFILE_OP ) {
                                JagFixString kstr(buf, keylen[0], keylen[0] );

                                if ( ptab ) {
                                    dn("tab3022901 JAG_GETFILE_OP getFileHashDir ...");
                                    Jstr hdir = getFileHashDir( ptab->_tableRecord, kstr );
                                    ptab->setGetFileAttributes( hdir, pass->parseParam, buffers );
                                }
                                // changed pass->parseParam.strResult
                            } 

							JagTable::nonAggregateFinalbuf( ntr, maps, attrs, pass->req, buffers, pass->parseParam, sendbuf, 
												    pass->sendlen, pass->jda, 
													pass->writeName, *(pass->recordcnt), pass->nowherecnt, pass->nrec, false );

                            /*** getfile into must be point query, so this is not supported, considering in M node
				            if ( pass->parseParam->opcode == JAG_GETFILE_OP && pass->parseParam->getFileActualData ) {
                                // get file  .. into ... (there is into to get file actual data)
                                JagFixString kstr(buf, keylen[0], keylen[0] );
                                Jstr    hdir = fileHashDir( kstr );
            					Jstr    dbtable, ddcol, inpath; 
            					char    fname[JAG_FILE_FIELD_LEN+1];
                                int     getpos, actualSent = 0;

                                Jstr  dbName = pass->parseParam->objectVec[0].dbName;
                                Jstr  tableName = pass->parseParam->objectVec[0].tableName;

                                dbtable = dbName + "." + tableName;
            
            					for ( int i = 0; i < pass->parseParam->selColVec.size(); ++i ) {
                                    if ( ! ptab ) break;

            						ddcol = dbtable + "." + pass->parseParam->selColVec[i].getfileCol.s();
            						//if ( ptab->_tablemap->getValue(ddcol, getpos) ) 
            						if ( maps[0]->getValue(ddcol, getpos) ) {
            							//memcpy( fname, buffers[0]+_schAttr[getpos].offset, _schAttr[getpos].length );
                                        memset( fname, 0, JAG_FILE_FIELD_LEN+1 );
            							memcpy( fname, buffers[0]+attrs[0][getpos].offset, attrs[0][getpos].length );
            							inpath = ptab->_darrFamily->_sfilepath + "/" + hdir + "/" + fname;
            
            							d("s52890118 oneFileSender %s ...\n", inpath.s() );
            							pass->req->session->active = false;
            
            							oneFileSender( pass->req->session->sock, inpath, dbName, tableName, hdir, actualSent );
                                        // int oneFileSender( JAGSOCK sock, const Jstr &inpath, const Jstr &jagHome, bool tryPNdata, int &actualSent )
                                        dn("s40028120 after oneFileSender %s actualSent=%d", inpath.s(), actualSent );
            
            							pass->req->session->active = true;
            						}
            					}

                                break;  // only get first file
                            }
                            ****/

							if ( JAG_INSERTSELECT_OP == pass->parseParam->opcode ) {			
								d("s20229 formatInsertFromSelect ...\n");
								JagTable::formatInsertFromSelect( parseParam, attrs[0], sendbuf, buffers[0], 
																  pass->sendlen, numCols[0], pass->cli, iscmd );
							}

							if ( parseParam->hasLimit && *(pass->recordcnt) >= pass->actlimit ) {
								break;
							}
						}
					}
					hasFirst = true;
				}
			}
		}

		if ( ntr ) {
			d("s08484 delete ntr\n");
			delete ntr;
			ntr = NULL;
		}

        dn("s6263637 deleted ntr");
	}

	free( sendbuf );
	free( buf );
	return NULL;
}

// return 0: error or no more data accepted
// return 1: success
// method to calculate group by value, the rearrange buffer to insert into diskarray
// multiple threads calling
// input buffers are natural data
int JagTable::buildDiskArrayForGroupBy( JagMergeReaderBase *ntr, const JagHashStrInt *maps[], const JagSchemaAttribute *attrs[], 
										const JagRequest *req, const char *buffers[], JagParseParam *parseParam, 
										JagMemDiskSortArray *gmdarr, char *gbvbuf )
{
	Jstr treetype;
	jagint totoff = 0;
	ExprElementNode *root;
	Jstr errmsg;
	bool init = 1;
	int rc = 0, treelength = 0, typeMode = 0, treesig;
	d("s40013 buildDiskArrayForGroupBy selColVec.size=%d  ...\n", parseParam->selColVec.size() );

	for ( int i = 0; i < parseParam->selColVec.size(); ++i ) {
		init = 1;
		root = parseParam->selColVec[i].tree->getRoot();
		if ( root->checkFuncValid( ntr, maps, attrs, buffers, parseParam->selColVec[i].strResult, typeMode, 
								   treetype, treelength, init, 1, 1 ) != 1 ) {
			return 0;
		}
		
		treelength = parseParam->selColVec[i].length;
		treesig = parseParam->selColVec[i].sig;
		treetype = parseParam->selColVec[i].type;
		formatOneCol( req->session->timediff, req->session->servobj->servtimediff, gbvbuf, 
					  parseParam->selColVec[i].strResult.s(), errmsg, "GAR", 
					  totoff, treelength, treesig, treetype );
		totoff += treelength;
		d("i=%d gbvbuf=[%s] len=%d sig=%d typ=%s\n", i, gbvbuf, treelength, treesig, treetype.s() );
	}

	// then, insert data to groupby value diskarray
	if ( gbvbuf[0] != '\0' ) {	
		d("s22018 pair ... groupByUpdat.. keylen=%d vallen=%d\n", gmdarr->_keylen, gmdarr->_vallen );
		JagDBPair pair(gbvbuf, gmdarr->_keylen, gbvbuf+gmdarr->_keylen, gmdarr->_vallen, true );
		rc = gmdarr->groupByUpdate( pair );
		rc = 1;
	} else {
		d("s22201 gbvbuf[0] is empty\n");
		rc = 1;
	}
	return rc;
}

// main thread calling
// input buffers are natural data
void JagTable::groupByFinalCalculation( char *gbvbuf, bool nowherecnt, jagint finalsendlen, std::atomic<jagint> &cnt, jagint actlimit,
										const Jstr &writeName, JagParseParam *parseParam, 
										JagDataAggregate *jda, JagMemDiskSortArray *gmdarr, const JagSchemaRecord *nrec )
{
	dn("s9282 groupByFinalCalculation ..." );

	gmdarr->endWrite();
	gmdarr->beginRead();
	while ( true ) {
		if ( !gmdarr->get( gbvbuf ) ) { break; }
		if ( !nowherecnt ) ++cnt;
		else {
			if ( *gbvbuf != '\0' ) {
				if ( jda ) doWriteIt( jda, parseParam, writeName, gbvbuf, finalsendlen, nrec ); 
				++cnt;
				if ( parseParam->hasLimit && cnt >= actlimit ) {
					break;
				}
			}
		}
	}
	gmdarr->endRead();
}

// method to finalize non aggregate range query, point query data
// multiple threads calling
// input buffers are natural data
void JagTable::nonAggregateFinalbuf(JagMergeReaderBase *ntr, const JagHashStrInt *maps[], const JagSchemaAttribute *attrs[], 
									const JagRequest *req, const char *buffers[], 
									JagParseParam *parseParam, char *finalbuf, jagint finalsendlen,
									JagDataAggregate *jda, const Jstr &writeName, std::atomic<jagint> &cnt, 
									bool nowherecnt, const JagSchemaRecord *nrec, bool oneLine )
{
	d("s4817 nonAggregateFinalbuf parseParam->hasColumn=%d writeName=[%s]\n", parseParam->hasColumn, writeName.s() );

	if ( parseParam->hasColumn ) {
		memset(finalbuf, 0, finalsendlen);

		Jstr        treetype;
		int         rc, typeMode = 0, treelength = 0, treesig = 0;
		bool        init = 1;
		jagint      offset;
		Jstr        errmsg;
		ExprElementNode *root;

		for ( int i = 0; i < parseParam->selColVec.size(); ++i ) {
			init = 1;
			root = parseParam->selColVec[i].tree->getRoot();
			rc = root->checkFuncValid( ntr, maps, attrs, buffers, parseParam->selColVec[i].strResult, typeMode, 
									   treetype, treelength, init, 1, 1 );

			dn("s7008263 strResult=[%s]\n", parseParam->selColVec[i].strResult.s() );
			dn("s2080282 in JagTable::nonAggregateFinalbuf() checkFuncValid rc=%d treetype=[%s]\n", rc, treetype.s() );
			if ( rc != 1 ) { 
                return; 
            }

			offset = parseParam->selColVec[i].offset;
			treelength = parseParam->selColVec[i].length;
			treesig = parseParam->selColVec[i].sig;
			treetype = parseParam->selColVec[i].type;
            dn("s802222802 offset=%d treelength=%d treesig=%d treetype=[%s] formatOneCol...", offset, treelength, treesig, treetype.s() );

            /***
            if ( treetype == JAG_C_COL_TYPE_DOUBLE || treetype == JAG_C_COL_TYPE_LONGDOUBLE) {
                dn("s2023800 copy double or longdouble strresult to finalbuf");
                memcpy(finalbuf+offset, parseParam->selColVec[i].strResult.s(), parseParam->selColVec[i].strResult.size() );
            } else {
			    formatOneCol( req->session->timediff, req->session->servobj->servtimediff, finalbuf,
						      parseParam->selColVec[i].strResult.s(), errmsg, "GAR", offset, 
						      treelength, treesig, treetype );
		        d("s3981 formatOneCol done\n" );
            }
            ***/
			formatOneCol( req->session->timediff, req->session->servobj->servtimediff, finalbuf,
						      parseParam->selColVec[i].strResult.s(), errmsg, "GAR", offset, 
						      treelength, treesig, treetype );
		    d("s3981 formatOneCol done\n" );
		}

		dn("s3981 finalbuf=[%s]", finalbuf );
	}
	
	if ( JAG_GETFILE_OP == parseParam->opcode ) {
		// if getfile, arrange point query to be size of file/modify time of file/md5sum of file/file path ( for actural data )
		//Jstr ddcol, inpath, instr, outstr; 
        dn("tab023001 JAG_GETFILE_OP parseParam=%p", parseParam);
        // JAG_FILE_FIELD_LEN

        jagint  offset = 0;
		for ( int i = 0; i < parseParam->selColVec.size(); ++i ) {
            dn("tab030001 i=%d offset=%d length=%d strresult=[%s]", 
               i, parseParam->selColVec[i].offset, parseParam->selColVec[i].length, parseParam->selColVec[i].strResult.s() ); 

            // copy to final buf
			snprintf(finalbuf+offset, JAG_FILE_FIELD_LEN+1, "%s", parseParam->selColVec[i].strResult.s());
            offset += JAG_FILE_FIELD_LEN;
		}
	}
	
	dn("st3020291 oneLine=%d", oneLine);

	if ( oneLine ) {
        // point query
        dn("tab30018 oneLine point query");
		if ( !nowherecnt ) cnt = -111; // select count(*) with where, no actual data sentry
		else { // one line data, direct send to client
			if ( JAG_INSERTSELECT_OP != parseParam->opcode ) {
				if ( parseParam->hasColumn || JAG_GETFILE_OP == parseParam->opcode ) { 
				    // has select coulumn or getfile command, use finalbuf
					//sendMessageLength( *req, finalbuf, finalsendlen, "X1" );
					dn("st202281 sendMessageLength JAG_MSG_X1 JAG_MSG_NEXT_MORE ");
					dn("st202281 sendMessageLength finalbuf=[%s] finalsendlen=%d", finalbuf, finalsendlen);
					//sendMessageLength( *req, finalbuf, finalsendlen, JAG_MSG_X1, JAG_MSG_NEXT_END);
					//dumpmem( finalbuf, finalsendlen );
					sendMessageLength( *req, finalbuf, finalsendlen, JAG_MSG_X1, JAG_MSG_NEXT_MORE);
				} else { // select *, use original buf, buffer[0]
					sendMessageLength( *req, buffers[0], finalsendlen, JAG_MSG_X1, JAG_MSG_NEXT_END);
				}
				cnt = -111;
			}
		}
	} else { 
        // range, non aggregate query
        dn("tab0910003 range, non aggregate query nowherecnt=%d", nowherecnt );

		if ( !nowherecnt ) ++cnt; // select count(*) with where, no actual data sentry
		else { // range data, write to data aggregate file
            dn("tab071112 else");
			if ( JAG_INSERTSELECT_OP != parseParam->opcode ) {

                dn("tab092110 no JAG_INSERTSELECT_OP. parseParam->hasColumn=%d parseParam->opcode=%d",
                     parseParam->hasColumn, parseParam->opcode  );

				if ( parseParam->hasColumn || JAG_GETFILE_OP == parseParam->opcode  ) { 
                    // has select coulumn, use finalbuf
					if ( *finalbuf != '\0' ) {
						if ( jda ) {
							doWriteIt( jda, parseParam, writeName, finalbuf, finalsendlen, nrec ); 
							++cnt;
							dn("s22383 hasColumn/JAG_GETFILE_OP doWriteIt ...finalsendlen=%d cnt=%d\n", finalsendlen, int(cnt) );
						}
					} else {
                        dn("tab09311102 finalbuf is NULL?");
                    }
				} else { // select *, use original buf, buffer[0], include /export
                    /**
                    dn("s23804003 dump buffers[0] finalsendlen=%ld", finalsendlen );
                    dumpmem( buffers[0], finalsendlen );
                    **/

					if ( *buffers[0] != '\0' ) {
						if ( jda ) {
							d("s22388 s999 has no column, doWriteIt ...finalsendlen=%d cnt=%d\n", finalsendlen, int(cnt) );
							doWriteIt( jda, parseParam, writeName, buffers[0], finalsendlen, nrec ); 
							++cnt;
						}
					}
				}
			} else {
                dn("tab333018 JAG_INSERTSELECT_OP");
            }
		}
	}
}

// must be range query, and must have select column, do aggregation 
// multiple threads calling
// input buffers are natural data
void JagTable::aggregateDataFormat( JagMergeReaderBase *ntr, const JagHashStrInt *maps[], const JagSchemaAttribute *attrs[], 
									const JagRequest *req, const char *buffers[], JagParseParam *parseParam, bool init )
{
	ExprElementNode *root;
	int typeMode = 0, treelength = 0;
	Jstr treetype;
	for ( int i = 0; i < parseParam->selColVec.size(); ++i ) {
		root = parseParam->selColVec[i].tree->getRoot();
		if ( root->checkFuncValid( ntr, maps, attrs, buffers, parseParam->selColVec[i].strResult, typeMode, 
								   treetype, treelength, init, 1, 1 ) != 1 ) {
			d("ERROR i=%d [%s]\n", i, parseParam->selColVec[i].asName.s());
			return;
		}
	}
}

// finish all data, merge all aggregation results to form one final result to send
// main thread calling
// input buffers are natural data
void JagTable::aggregateFinalbuf( const JagRequest *req, const Jstr &sendhdr, jagint len, JagParseParam *parseParam[], 
								  char *finalbuf, jagint finalsendlen, JagDataAggregate *jda, const Jstr &writeName, 
								  std::atomic<jagint> &cnt, bool nowherecnt, const JagSchemaRecord *nrec )
{
	JagSchemaRecord aggrec;
	//d("c2036 aggregateFinalbuf sendhdr=[%s]\n", sendhdr.s() );
	aggrec.parseRecord( sendhdr.s() );
	char *aggbuf = (char*)jagmalloc(finalsendlen+1);
	memset(aggbuf, 0, finalsendlen+1);
	JagVector<JagFixString> oneSetData;
	JagVector<int> selectPartsOpcode;
	
	Jstr type;
	jagint offset, length, sig;
	ExprElementNode *root;
	Jstr errmsg;
	
	for ( int m = 0; m < len; ++m ) {
		memset(finalbuf, 0, finalsendlen);
		for ( int i = 0; i < parseParam[m]->selColVec.size(); ++i ) {
			root = parseParam[m]->selColVec[i].tree->getRoot();
			offset = parseParam[m]->selColVec[i].offset;
			length = parseParam[m]->selColVec[i].length;
			sig = parseParam[m]->selColVec[i].sig;
			type = parseParam[m]->selColVec[i].type;
			formatOneCol( req->session->timediff, req->session->servobj->servtimediff, finalbuf,
				parseParam[m]->selColVec[i].strResult.s(), errmsg, "GAR", 
				offset, length, sig, type );
			if ( 0 == m ) selectPartsOpcode.append( root->getBinaryOp() );
		}
		oneSetData.append( JagFixString(finalbuf, finalsendlen, finalsendlen) );		
	}

	JaguarCPPClient::doAggregateCalculation( aggrec, selectPartsOpcode, oneSetData, aggbuf, finalsendlen, 0 );

	//d("s2879 nowherecnt=%d aggbuf=[%s] jda=%0x\n", nowherecnt, aggbuf, jda );
	if ( nowherecnt ) { // send actural data
		if ( *aggbuf != '\0' ) {
			if ( jda ) {
				doWriteIt( jda, parseParam[0], writeName, aggbuf, finalsendlen, nrec );
			}
			++cnt;
			//d("s2287 cnt incremented to %d\n", (int)cnt );
		}
	}

	free( aggbuf );
}

// drop indexes, removed files of a table
Jstr JagTable::drop( Jstr &errmsg, bool isTruncate )
{
	JagIndex *pindex = NULL;
	Jstr idxNames;
	if ( ! _darrFamily ) return idxNames;

	if ( _indexlist.size() > 0 ) {
		if ( isTruncate ) {	
			// if isTruncate, store names to indexNames
			for ( int i = 0; i < _indexlist.size(); ++i ) {
				if ( idxNames.size() < 1 ) {
					idxNames = _indexlist[i];
				} else {
					idxNames += Jstr("|") + _indexlist[i];
				}
			}
		}

		Jstr dbtab = _dbname + "." + _tableName;
		Jstr tabIndex;
		Jstr dbtabIndex;
		int  lockrc;

		while ( _indexlist.size() > 0 ) {
			pindex = _objectLock->writeLockIndex( JAG_DROPINDEX_OP, _dbname, _tableName, _indexlist[_indexlist.size()-1],
											      _tableschema, _indexschema, _replicType, 1, lockrc );
			if ( pindex ) {
				pindex->drop();
				dbtabIndex = dbtab + "." + _indexlist[_indexlist.size()-1];
				if ( !isTruncate ) {
					_indexschema->remove( dbtabIndex );
				}

				if ( pindex ) { delete pindex; pindex = NULL; }

				_objectLock->writeUnlockIndex( JAG_DROPINDEX_OP, _dbname, _tableName, _indexlist[_indexlist.size()-1],
												_replicType, 1 );
			}
			_indexlist.removePos( _indexlist.size()-1 );				
		}
	}	

	// d("s7373 _darrFamily->_tablepath=[%s]\n", _darrFamily->_tablepath.s() );

	_darrFamily->drop();
	delete _darrFamily;
	_darrFamily = NULL;

	// rmdir
	Jstr rdbdatahome = _cfg->getJDBDataHOME( _replicType ), dbpath = rdbdatahome + "/" + _dbname;
	Jstr dbtabpath = dbpath + "/" + _tableName;
	JagFileMgr::rmdir( dbtabpath, true );
	d("s4222939 rmdir(%s)\n", dbtabpath.s() );

	jagmalloc_trim(0);

	return idxNames;
}

void JagTable::dropFromIndexList( const Jstr &indexName )
{
	for ( int k = 0; k < _indexlist.size(); ++k ) {
		const char *idxName = strrchr(_indexlist[k].s(), '.');
		if ( !idxName ) {
			continue;
		}
		++idxName;

		if ( strcmp(indexName.s(), idxName) == 0 ) {
			_indexlist.removePos( k );
			break;
		}
	}
}

// find columns in buf that are keys of some index, and removed the index records
int JagTable::_removeIndexRecords( const char *buf )
{
	dn("s2930 _removeIndexRecords _indexlist.size=%d", _indexlist.size() );

	if ( _indexlist.size() < 1 ) return 0;

	//bool rc;
	AbaxBuffer bfr;
	JagIndex *pindex = NULL;

	int cnt = 0;
	int lockrc;

	for ( int i = 0; i < _indexlist.size(); ++i ) {
		pindex = _objectLock->writeLockIndex( JAG_DELETE_OP, _dbname, _tableName, _indexlist[i], _tableschema, _indexschema, _replicType, 1, lockrc );
		if ( pindex ) {
			pindex->removeFromTable( buf );
			++ cnt;
			_objectLock->writeUnlockIndex( JAG_DELETE_OP, _dbname, _tableName, _indexlist[i], _replicType, 1 );
		}
	}

	return cnt;
}

// return "db.idx1,db.idx2,db.idx3" 
Jstr JagTable::getIndexNameList()
{
	Jstr res;
	if ( ! _darrFamily ) return res;
	if ( _indexlist.size() < 1 ) { return res; }

	for ( int i=0; i < _indexlist.length(); ++i ) {
		if ( res.size() < 1 ) {
			res = _indexlist[i];
		} else {
			res += Jstr(",") + _indexlist[i];
		}
	}

	return res;
}

int JagTable::renameIndexColumn( const JagParseParam *parseParam, Jstr &errmsg )
{
	JagIndex *pindex = NULL;
	AbaxBuffer bfr;
	int lockrc;

	if ( _indexlist.size() > 0 ) {
		Jstr dbtab  = _dbname + "." + _tableName;
		Jstr dbtabIndex;
		for ( int k = 0; k < _indexlist.size(); ++k ) {
			pindex = _objectLock->writeLockIndex( JAG_ALTER_OP, _dbname, _tableName, _indexlist[k],
												  _tableschema, _indexschema, _replicType, 1, lockrc );
			if ( pindex ) {
				pindex->drop();
				dbtabIndex = dbtab + "." + _indexlist[k];
				_indexschema->addOrRenameColumn( dbtabIndex, parseParam );
				pindex->refreshSchema();
				_objectLock->writeUnlockIndex( JAG_ALTER_OP, _dbname, _tableName, _indexlist[k], _replicType, 1 );
			}
		}
	}

	return 1;	
}

int JagTable::setIndexColumn( const JagParseParam *parseParam, Jstr &errmsg )
{
	JagIndex *pindex = NULL;
	AbaxBuffer bfr;
	int  lockrc;

	if ( _indexlist.size() > 0 ) {
		Jstr dbtab  = _dbname + "." + _tableName;
		Jstr dbtabIndex;
		for ( int k = 0; k < _indexlist.size(); ++k ) {
			pindex = _objectLock->writeLockIndex( JAG_ALTER_OP, _dbname, _tableName, _indexlist[k],
												   _tableschema, _indexschema, _replicType, 1, lockrc );
			if ( pindex ) {
				pindex->drop();
				dbtabIndex = dbtab + "." + _indexlist[k];
				_indexschema->setColumn( dbtabIndex, parseParam );
				pindex->refreshSchema();
				_objectLock->writeUnlockIndex( JAG_ALTER_OP, _dbname, _tableName, _indexlist[k], _replicType, 1 );
			}
		}
	}

	return 1;	
}

int JagTable::refreshSchema()
{
	d("s222286 JagTable::refreshSchema()...\n");

	Jstr dbtable = _dbname + "." + _tableName;
	Jstr dbcolumn;
	AbaxString schemaStr;
	const JagSchemaRecord *record =  _tableschema->getAttr( dbtable );
	if ( ! record ) {
		d("s8912 _tableschema->getAttr(%s) error, _tableRecord is not updated\n", dbtable.s() );
		return 0;
	}

	#if 0
	d("s7640 _tableRecord is updated with *record record->columnVector=%0x print:\n", record->columnVector );
	_tableRecord = *record;
	//_tableRecord.print();
	#endif

	if ( _schAttr ) {
		delete [] _schAttr;
	}

	// d("s2008 _origHasSpare=%d _numCols=%d\n", _origHasSpare, _numCols );
	_numCols = record->columnVector->size();
	_schAttr = new JagSchemaAttribute[_numCols];
	// refresh
	_tableRecord = *record;

	_schAttr[0].record = _tableRecord;

	if ( _tablemap ) {
		delete _tablemap;
	}
	//_tablemap = new JagHashStrInt();
	_tablemap = newObject<JagHashStrInt>();
	d("s4982 created a new _tablemap \n" );

	// setupSchemaMapAttr( _numCols, _origHasSpare );
	setupSchemaMapAttr( _numCols );

	return 1;
}

int JagTable::refreshIndexList()
{
	// create index : _indexlist.append( dbindex 
	JagVector<Jstr> vec;
	int cnt = _indexschema->getIndexNames( _dbname, _tableName, vec );
	if ( cnt < 1 ) return 0;

	Jstr dbindex, idxname;
	_indexlist.destroy();
	_indexlist.init( 8 );

  	//char buf[32];
	for ( int i = 0; i < vec.size(); ++i ) {	
		idxname = vec[i];
    	dbindex = _dbname + "." + idxname;
		// printf("s8380 _indexlist.append(%s)\n", dbindex.s() );
		 _indexlist.append( dbindex );
	}

	return 1;
}

void JagTable::flushBlockIndexToDisk()
{
	// flush table 
	if ( _darrFamily ) { _darrFamily->flushBlockIndexToDisk(); }

	// then flush related indexs
	JagIndex *pindex;
	int lockrc;

	for ( int i = 0; i < _indexlist.size(); ++i ) {
		pindex = _objectLock->writeLockIndex( JAG_INSERT_OP, _dbname, _tableName, _indexlist[i], _tableschema, _indexschema, _replicType, 1, lockrc );
		if ( pindex ) {
			pindex->flushBlockIndexToDisk();
			_objectLock->writeUnlockIndex( JAG_INSERT_OP, _dbname, _tableName, _indexlist[i], _replicType, 1 );
		}
	}
}

// static
// currently, doWriteIt has only one host name, so it can only been write one by one 
// ( use mutex in jda to protect, with last flag true )
void JagTable::doWriteIt( JagDataAggregate *jda, const JagParseParam *parseParam, 
						  const Jstr &host, const char *buf, jagint buflen, const JagSchemaRecord *nrec )
{

	// first check if there is any _having clause
	// if having exists, convert natrual format to dbformat,
	if ( parseParam->hasHaving ) {
		// having tree
		// ExprElementNode *root = (parseParam->havingTree).getRoot();
	} else {
		//if ( jda ) jda->writeit( host, buf, buflen, nrec, true );
		if ( jda ) jda->writeit( 0, buf, buflen, nrec, true );
	}
}

// static
// format insert into cmd, and use pcli to find servers to be sent for insert
// iscmd should be either "insert into TABLE (COL1,COL2)" or "insert into TABLE" format
void JagTable::formatInsertFromSelect( const JagParseParam *pParam, const JagSchemaAttribute *attrs, 
									   const char *finalbuf, const char *buffers, jagint finalsendlen, jagint numCols,
									   JaguarCPPClient *pcli, const Jstr &iscmd )
{
	// format insert cmd
	Jstr insertCmd = iscmd + " values ( ", oneData;
	//d("s10098 insertCmd=[%s]\n", insertCmd.s() );
	if ( pParam->hasColumn ) {
		for ( int i = 0; i < pParam->selColVec.size(); ++i ) {
			oneData = formOneColumnNaturalData( finalbuf, pParam->selColVec[i].offset, 
												pParam->selColVec[i].length, pParam->selColVec[i].type );
			if ( 0 == i ) {
				insertCmd += Jstr("'") + oneData.s() + "'";
			} else {
				insertCmd += Jstr(",'") + oneData.s() + "'";
			}
		}
		insertCmd += ");";
		//d("s10028 hasColumn insertCmd=[%s]\n", insertCmd.s() );
	} else {
		for ( int i = 0; i < numCols; ++i ) {
			if ( attrs[i].colname != "spare_" ) {
				oneData = formOneColumnNaturalData( buffers, attrs[i].offset, attrs[i].length, attrs[i].type );
				if ( 0 == i ) {
					insertCmd += Jstr("'") + oneData.s() + "'";
				} else {
					insertCmd += Jstr(",'") + oneData.s() + "'";
				}
			}			
		}
		insertCmd += ");";
		//d("s10024 has no column insertCmd=[%s]\n", insertCmd.s() );
	}
	
	// put insert cmd into cli
	pcli->concurrentDirectInsert( insertCmd.s(), insertCmd.size() );
}

bool JagTable::hasSpareColumn()
{
	Jstr colname;
	for ( int i = 0; i < _numCols; ++i ) {
		colname = (*(_tableRecord.columnVector))[i].name.s();
		// d("s4772 hasSpareColumn i=%d colname=[%s]\n", i, colname.s() );
		if ( colname == "spare_" ) return true;
	}
	return false;
}

void JagTable::setupSchemaMapAttr( int numCols )
{
	//d("s38007 JagTable::setupSchemaMapAttr numCols=%d ...\n", numCols );
	/**
	d("s2039 setupSchemaMapAttr numCols=%d\n", numCols );
	int  N;
	if ( hasSpare ) N = numCols;
	else N = numCols-1;
	**/
	int N = numCols;

	_numKeys = 0;
	int i = 0, rc, rc2;
	int j = 0;
	Jstr dbcolumn, defvalStr;
	const JagVector<JagColumn> &columnVector = *(_tableRecord.columnVector);

	for ( i = 0; i < N; ++i ) {

		dbcolumn = _dbtable + "." + columnVector[i].name.s();
		
		_schAttr[i].dbcol = dbcolumn;
		_schAttr[i].objcol = _tableName + "." + columnVector[i].name.s();
		_schAttr[i].colname = columnVector[i].name.s();
		_schAttr[i].isKey = columnVector[i].iskey;

		if ( _schAttr[i].isKey ) { 
			++ _numKeys; 
			d("s48863 JagTable::setupSchemaMapAttr found _numKeys=%d\n", _numKeys );
		}

		_schAttr[i].offset = columnVector[i].offset;
		_schAttr[i].length = columnVector[i].length;
		_schAttr[i].sig = columnVector[i].sig;
		_schAttr[i].type = columnVector[i].type;
		_schAttr[i].begincol = columnVector[i].begincol;
		_schAttr[i].endcol = columnVector[i].endcol;
		_schAttr[i].srid = columnVector[i].srid;
		_schAttr[i].metrics = columnVector[i].metrics;
		_schAttr[i].isUUID = false;
		_schAttr[i].isFILE = false;

		_tablemap->addKeyValue(dbcolumn, i);
		//d("s4015 _tablemap->addKeyValue(dbcolumn=[%s] ==> i=%d\n", dbcolumn.s(), i );

		rc = columnVector[i].spare[1];
		rc2 = columnVector[i].spare[4];
		if ( rc == JAG_C_COL_TYPE_UUID[0]) {
			_schAttr[i].isUUID = true;
		} else if ( rc == JAG_C_COL_TYPE_FILE[0] ) {
			_schAttr[i].isFILE = true;
		}
		
		// setup enum string list and/or default value for empty
		if ( rc == JAG_C_COL_TYPE_ENUM[0] || rc2 == JAG_CREATE_DEFINSERTVALUE ) {
			defvalStr = "";
			_tableschema->getAttrDefVal( dbcolumn, defvalStr );
			// for default string, strsplit with '|', and use last one as default value
			// regular type has only one defvalStr and use it
			// enum type has many default strings separated by '|', where the first several strings are enum range
			// and the last one is default value if empty
			//d("s2202292 after getAttrDefVal dbcolumn=[%s] defvalStr=[%s]\n", dbcolumn.s(), defvalStr.s() );
			JagStrSplitWithQuote sp( defvalStr.s(), '|' );
			
			if ( rc == JAG_C_COL_TYPE_ENUM[0] ) {
				// _schAttr[i].enumList = new JagVector<Jstr>();
				for ( int k = 0; k < sp.length(); ++k ) {
					_schAttr[i].enumList.append( strRemoveQuote( sp[k].s() ) );
					//d("s111029 i=%d enumList.append [%s]\n", i, sp[k].s() );
				}
			}

			if ( rc2 == JAG_CREATE_DEFINSERTVALUE && sp.length()>0 ) {
				_schAttr[i].defValue = strRemoveQuote( sp[sp.length()-1].s() );
				// last one is default value
				_defvallist.append( i );
			}
		}

		if ( _schAttr[i].length > 0 ) {
			++j;
		}
	}

}

// in kvbuf, there may be files, if so, remove them
// return: 0 no files;  N: # of files removed
int  JagTable::removeColFiles(const char *kvbuf )
{
	if ( kvbuf == NULL ) return 0;
	if ( *kvbuf == '\0' ) return 0;
	int rc = 0;
	Jstr fpath, db, tab, fname;
	JagFixString kstr = JagFixString( kvbuf, _KEYLEN, _KEYLEN );

	//Jstr hdir = fileHashDir( fstr );
    Jstr hdir = getFileHashDir( _tableRecord, kstr );

	for ( int i = 0; i < _numCols; ++i ) {
		if ( _schAttr[i].isFILE ) {
			JagStrSplit sp( _schAttr[i].dbcol, '.' );
			if (sp.length() < 3 ) continue;
			db = sp[0];
			tab = sp[1];
			fname = Jstr( kvbuf+_schAttr[i].offset,  _schAttr[i].length );
			// fpath = jaguarHome() + "/data/" + db + "/" + tab + "/files/" + fname;
			fpath = _darrFamily->_sfilepath + "/" + hdir + "/" + fname;
			jagunlink( fpath.s() );

			/***
			d("s6360 i=%d dbcol=[%s] objcol=[%s] colname=[%s] offset=%d length=%d fpath=[%s]\n",
				i, _schAttr[i].dbcol.s(), _schAttr[i].objcol.s(), _schAttr[i].colname.s(),  
				_schAttr[i].offset, _schAttr[i].length, fpath.s() );
			***/
			++ rc;
		}
	}
	return rc;
}

bool JagTable::isFileColumn( const Jstr &colname )
{
	Jstr col;
	for ( int i = 0; i < _numCols; ++i ) {
		if ( _schAttr[i].isFILE ) {
			JagStrSplit sp( _schAttr[i].dbcol, '.' );
			if (sp.length() < 3 ) continue;
			if ( colname == sp[2] ) {
				// d("s0293 colname=[%s] is a file type\n", colname.s() );
				return true;
			}
		}
	}

	return false;
}

void JagTable::setGetFileAttributes( const Jstr &hdir, JagParseParam *parseParam, const char *buffers[] )
{
	// if getfile, arrange point query to be size of file/modify time of file/md5sum of file/file path ( for actural data )
	Jstr    ddcol, inpath, instr, outstr; 
	int     getpos; 
	char    fname[JAG_FILE_FIELD_LEN+1]; 
	struct  stat sbuf;

	dn( "s3331 setGetFileAttributes selColVec.size=%d\n", parseParam->selColVec.size() );

	for ( int i = 0; i < parseParam->selColVec.size(); ++i ) {

		ddcol = _dbtable + "." + parseParam->selColVec[i].getfileCol.s();

		if ( _tablemap->getValue(ddcol, getpos) ) {

            dn("tab302028 in setGetFileAttributes i=%d ddcol=%s getpos=%d", i, ddcol.s(), getpos );

            memset( fname, 0, JAG_FILE_FIELD_LEN+1);
			memcpy( fname, buffers[0]+_schAttr[getpos].offset, _schAttr[getpos].length );
			inpath = _darrFamily->_sfilepath + "/" + hdir + "/" + fname;

            dn("tab0828710 file inpath=%s", inpath.s() );

			if ( stat(inpath.s(), &sbuf) == 0 ) {
				if ( parseParam->selColVec[i].getfileType == JAG_GETFILE_SIZE ) {
					outstr = longToStr( sbuf.st_size );
                    dn("tab091222 JAG_GETFILE_SIZE outstr=[%s]", outstr.s() );
				} else if ( parseParam->selColVec[i].getfileType == JAG_GETFILE_SIZEGB ) {
					outstr = longToStr( sbuf.st_size/ONE_GIGA_BYTES);
				} else if ( parseParam->selColVec[i].getfileType == JAG_GETFILE_SIZEMB ) {
					outstr = longToStr( sbuf.st_size/ONE_MEGA_BYTES);
				} else if ( parseParam->selColVec[i].getfileType == JAG_GETFILE_TIME ) {
					char octime[JAG_CTIME_LEN]; 
                    memset(octime, 0, JAG_CTIME_LEN);
 					jag_ctime_r(&sbuf.st_mtime, octime);
					octime[JAG_CTIME_LEN-1] = '\0';
					outstr = octime;
                    dn("tab091223 JAG_GETFILE_TIME outstr=[%s]", outstr.s() );
				} else if ( parseParam->selColVec[i].getfileType == JAG_GETFILE_MD5SUM ) {
					if ( lastChar(inpath) != '/' ) {
						instr = Jstr("md5sum ") + inpath;
						outstr = Jstr(psystem( instr.s() ).s(), 32);
					} else {
						outstr = "";
					}
				} else if ( parseParam->selColVec[i].getfileType == JAG_GETFILE_FPATH ) {
					outstr = inpath;
				} else if ( parseParam->selColVec[i].getfileType == JAG_GETFILE_HOST ) {
					outstr = _servobj->_localInternalIP;
				} else if ( parseParam->selColVec[i].getfileType == JAG_GETFILE_HOSTFPATH ) {
					outstr = _servobj->_localInternalIP + ":" + inpath;
				} else if ( parseParam->selColVec[i].getfileType == JAG_GETFILE_TYPE ) {
					JagStrSplit sp(fname, '.');
					if ( sp.length() >= 2 ) {
						outstr = sp[sp.length()-1];
					} else {
						outstr = "?";
					}
				} else {
					// actual data transmit, filename only
                    dn("tab081001 outsr uses fname [%s]", fname );
					outstr = fname;
				}

				parseParam->selColVec[i].strResult = outstr;
                dn("tab0911120 outstr=[%s]", outstr.s() );

			} else {
                dn("tab022220 error stat %s", inpath.s() );
				parseParam->selColVec[i].strResult = "error file status";
				jd(JAG_LOG_HIGH, "s4331 error file status\n" );
			}
		} else {
            dn("tab08123 error file %s", inpath.s() );
			parseParam->selColVec[i].strResult = "error file column";
			jd(JAG_LOG_HIGH, "s4391 error getting file column\n" );
		}
	}
}

void JagTable::formatPointsInLineString( int nmetrics, const JagLineString &line, char *tablekvbuf, const JagPolyPass &pass, 
										 JagVector<JagDBPair> &retpairVec, Jstr &errmsg ) const
{
	d("s4928 formatPointsInLineString line.size=%d\n", line.size() );
	int getpos;
	Jstr point;
	char ibuf[32];
	int rc;
	Jstr pointx = pass.colname + ":x";
	Jstr pointy = pass.colname + ":y";
	//Jstr pointz = pass.colname + ":z";

	//d("s2935 pointx=[%s] pointz=[%s] pass.getxmin=%d pass.getymin=%d \n", pointx.s(), pointz.s(), pass.getxmin, pass.getymin );

	for ( int j=0; j < line.size(); ++j ) {
		const JagPoint &jPoint = line.point[j];

        #ifdef JAG_KEEP_MIN_MAX
		point = "geo:xmin";
		getpos = pass.getxmin;
        dn("s1234003 colname=[%s] xmin=[%s] offset=%d len=%d", pass.colname.s(), doubleToStr(pass.xmin).s(), _schAttr[getpos].offset, _schAttr[getpos].length );
     	rc = formatOneCol( pass.tzdiff, pass.srvtmdiff, tablekvbuf, doubleToStr(pass.xmin).s(), errmsg, point.s(), 
							_schAttr[getpos].offset, _schAttr[getpos].length, _schAttr[getpos].sig, 
							_schAttr[getpos].type );

		if ( pass.getymin >= 0 ) {
    		point = "geo:ymin";
    		getpos = pass.getymin;
            dn("s1234005  ymin=[%s]", doubleToStr(pass.ymin).s() );
       		rc = formatOneCol( pass.tzdiff, pass.srvtmdiff, tablekvbuf, doubleToStr(pass.ymin).s(), errmsg, point.s(), 
								_schAttr[getpos].offset, 
       						   _schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
	    }

   	    if ( pass.getzmin >= 0 ) {
   			point = "geo:zmin";
   			getpos = pass.getzmin;
   			rc = formatOneCol( pass.tzdiff, pass.srvtmdiff, tablekvbuf, doubleToStr(pass.zmin).s(), errmsg, point.s(), 
								_schAttr[getpos].offset, _schAttr[getpos].length, _schAttr[getpos].sig, 
								_schAttr[getpos].type );
   	    }
    
		if ( pass.getxmax >= 0 ) {
   			point = "geo:xmax";
   			getpos = pass.getxmax;
            dn("s3033030  xmax=[%f] lenth=%d", pass.xmax, _schAttr[getpos].length );
   			rc = formatOneCol( pass.tzdiff, pass.srvtmdiff, tablekvbuf, doubleToStr(pass.xmax).s(), errmsg, point.s(), 
								_schAttr[getpos].offset, 
    						   _schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
	    }
    
		if ( pass.getymax >= 0 ) {
    		point =  "geo:ymax";
    		getpos = pass.getymax;
            dn("s3033035  ymax=[%f]", pass.ymax);
       		rc = formatOneCol( pass.tzdiff, pass.srvtmdiff, tablekvbuf, doubleToStr(pass.ymax).s(), errmsg, point.s(), 
								_schAttr[getpos].offset, 
       						   _schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
	    }
   	    // if ( is3D && getzmax >=0 ) 
   	    if (  pass.getzmax >=0 ) {
    		point = "geo:zmax";
    		getpos = pass.getzmax;
       		rc = formatOneCol( pass.tzdiff, pass.srvtmdiff, tablekvbuf, doubleToStr(pass.zmax).s(), errmsg, 
								point.s(), _schAttr[getpos].offset, 
       						   _schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
   	    }
        #endif

		if ( pass.getid >= 0 ) {
    		point = "geo:id";
    		getpos = pass.getid;
       		rc = formatOneCol( pass.tzdiff, pass.srvtmdiff, tablekvbuf, pass.lsuuid.s(), errmsg, point.s(), _schAttr[getpos].offset, 
       						   _schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
	    }

		if ( pass.getcol >= 0 ) {
    		point = "geo:col";
			sprintf(ibuf, "%d", pass.col+1 );
			//d("s5033 getcol=%d ibuf=[%s]\n", pass.getcol, ibuf );
    		getpos = pass.getcol;
       		rc = formatOneCol( pass.tzdiff, pass.srvtmdiff, tablekvbuf, ibuf, errmsg, point.s(), _schAttr[getpos].offset, 
       						   _schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
	    }

		if ( pass.getm >= 0 ) {
    		point = "geo:m"; sprintf(ibuf, "%d", pass.m+1 ); getpos = pass.getm;
       		rc = formatOneCol( pass.tzdiff, pass.srvtmdiff, tablekvbuf, ibuf, errmsg, point.s(), _schAttr[getpos].offset, 
       							_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
		}

		if ( pass.getn >= 0 ) {
    		point = "geo:n"; sprintf(ibuf, "%d", pass.n+1 ); getpos = pass.getn;
       		rc = formatOneCol( pass.tzdiff, pass.srvtmdiff, tablekvbuf, ibuf, errmsg, point.s(), _schAttr[getpos].offset, 
        									_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
		}

		if ( pass.geti >= 0 ) {
    		point = "geo:i";
			sprintf(ibuf, "%09d", j+1 );
			//d("s5034 geti=%d ibuf=[%s]\n", pass.geti, ibuf );
    		getpos = pass.geti;
       		rc = formatOneCol( pass.tzdiff, pass.srvtmdiff, tablekvbuf, ibuf, errmsg, point.s(), _schAttr[getpos].offset, 
       							_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
		}
        
        dn("s359001 pass.getx=%d", pass.getx );
       	if ( pass.getx >= 0 ) {
    		getpos = pass.getx;
			#if 1
			d("s4005 getx=%d pointx=[%s] line.point[j].x=[%s]\n", getpos, pointx.s(), line.point[j].x );
			d("_schAttr[getpos].offset=%d _schAttr[getpos].length=%d type=[%s]\n", 
					_schAttr[getpos].offset, _schAttr[getpos].length, _schAttr[getpos].type.s() );
			#endif
       		rc = formatOneCol( pass.tzdiff, pass.srvtmdiff, tablekvbuf, line.point[j].x, errmsg, 
       	 						pointx.s(), _schAttr[getpos].offset, 
       							_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
       	}

        dn("s359001 pass.gety=%d", pass.gety );
      	if ( pass.gety >= 0 ) {
    		getpos = pass.gety;
			#if 1
			d("s4005 gety=%d pointy=[%s] line.point[j].y=[%s]\n", getpos, pointy.s(), line.point[j].y );
			d("_schAttr[getpos].offset=%d _schAttr[getpos].length=%d type=[%s]\n", 
					_schAttr[getpos].offset, _schAttr[getpos].length, _schAttr[getpos].type.s() );
			#endif
       		rc = formatOneCol( pass.tzdiff, pass.srvtmdiff, tablekvbuf, line.point[j].y, errmsg, 
       		 					pointy.s(), _schAttr[getpos].offset, 
       							_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
       	}
        
		//d("s7830 pass.is3D=%d pass.getz=%d\n", pass.is3D, pass.getz );
       	if ( pass.is3D && pass.getz >= 0 ) {
    		getpos = pass.getz;
	        Jstr pointz = pass.colname + ":z";
			#if 1
			d("s4005 getz=%d pointz=[%s] line.point[j].z=[%s]\n", getpos, pointz.s(), line.point[j].z );
			d("_schAttr[getpos].offset=%d _schAttr[getpos].length=%d type=[%s]\n", 
					_schAttr[getpos].offset, _schAttr[getpos].length, _schAttr[getpos].type.s() );
			#endif
       		rc = formatOneCol( pass.tzdiff, pass.srvtmdiff, tablekvbuf, 
								line.point[j].z, errmsg, 
       							pointz.s(), _schAttr[getpos].offset, _schAttr[getpos].length, 
								_schAttr[getpos].sig, _schAttr[getpos].type );
       	}

		d("s2238 j=%d jPoint.metrics.size=%d\n", j, jPoint.metrics.size() );
		formatMetricCols( pass.tzdiff, pass.srvtmdiff, pass.dbtab, pass.colname, nmetrics, jPoint.metrics, tablekvbuf );

		dbNaturalFormatExchange( tablekvbuf, _numKeys, _schAttr, 0, 0, " " ); // natural format -> db format

		retpairVec.append( JagDBPair( tablekvbuf, _KEYLEN, tablekvbuf+_KEYLEN, _VALLEN, true ) );
        // each point forms an separate pair

		/**
		d("s4105 retpair.append tablekvbuf\n" );
		d("s4105 tablekvbuf:\n" );
		dumpmem( tablekvbuf, KEYLEN+VALLEN);
		**/

       	if ( pass.getx >= 0 ) {
			memset(tablekvbuf+_schAttr[pass.getx].offset, 0, _schAttr[pass.getx].length );
		}
       	if ( pass.gety>= 0 ) {
			memset(tablekvbuf+_schAttr[pass.gety].offset, 0, _schAttr[pass.gety].length );
		}
       	if ( pass.is3D && pass.getz >= 0 ) {
			memset(tablekvbuf+_schAttr[pass.getz].offset, 0, _schAttr[pass.getz].length );
		}


   	} // end of all points of this linestring column

}


void JagTable::formatPointsInVector( int nmetrics, const JagVectorString &line, char *tablekvbuf, const JagPolyPass &pass, 
										 JagVector<JagDBPair> &retpairVec, Jstr &errmsg ) const
{
	d("s4928 formatPointsInLineString line.size=%d\n", line.size() );
	int getpos;
	Jstr point;
	char ibuf[32];
	int rc;

	Jstr pointx = pass.colname + ":x";
	//Jstr pointy = pass.colname + ":y";
	//Jstr pointz = pass.colname + ":z";

	//d("s2935 pointx=[%s] pointz=[%s] pass.getxmin=%d pass.getymin=%d \n", pointx.s(), pointz.s(), pass.getxmin, pass.getymin );

	for ( int j=0; j < line.size(); ++j ) {
		const JagPoint &jPoint = line.point[j];

		if ( pass.getid >= 0 ) {
    		point = "geo:id";
    		getpos = pass.getid;
       		rc = formatOneCol( pass.tzdiff, pass.srvtmdiff, tablekvbuf, pass.lsuuid.s(), errmsg, point.s(), _schAttr[getpos].offset, 
       						   _schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
	    }

		if ( pass.getcol >= 0 ) {
    		point = "geo:col";
			sprintf(ibuf, "%d", pass.col+1 );
			//d("s5033 getcol=%d ibuf=[%s]\n", pass.getcol, ibuf );
    		getpos = pass.getcol;
       		rc = formatOneCol( pass.tzdiff, pass.srvtmdiff, tablekvbuf, ibuf, errmsg, point.s(), _schAttr[getpos].offset, 
       						   _schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
	    }

		if ( pass.getm >= 0 ) {
    		point = "geo:m"; sprintf(ibuf, "%d", pass.m+1 ); getpos = pass.getm;
       		rc = formatOneCol( pass.tzdiff, pass.srvtmdiff, tablekvbuf, ibuf, errmsg, point.s(), _schAttr[getpos].offset, 
       							_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
		}

		if ( pass.getn >= 0 ) {
    		point = "geo:n"; sprintf(ibuf, "%d", pass.n+1 ); getpos = pass.getn;
       		rc = formatOneCol( pass.tzdiff, pass.srvtmdiff, tablekvbuf, ibuf, errmsg, point.s(), _schAttr[getpos].offset, 
        									_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
		}

		if ( pass.geti >= 0 ) {
    		point = "geo:i";
			sprintf(ibuf, "%09d", j+1 );
			//d("s5034 geti=%d ibuf=[%s]\n", pass.geti, ibuf );
    		getpos = pass.geti;
       		rc = formatOneCol( pass.tzdiff, pass.srvtmdiff, tablekvbuf, ibuf, errmsg, point.s(), _schAttr[getpos].offset, 
       							_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
		}
        
        dn("s359001 pass.getx=%d", pass.getx );
       	if ( pass.getx >= 0 ) {
    		getpos = pass.getx;
			#if 1
			d("s4005 getx=%d pointx=[%s] line.point[j].x=[%s]\n", getpos, pointx.s(), line.point[j].x );
			d("_schAttr[getpos].offset=%d _schAttr[getpos].length=%d type=[%s]\n", 
					_schAttr[getpos].offset, _schAttr[getpos].length, _schAttr[getpos].type.s() );
			#endif
       		rc = formatOneCol( pass.tzdiff, pass.srvtmdiff, tablekvbuf, line.point[j].x, errmsg, 
       	 						pointx.s(), _schAttr[getpos].offset, 
       							_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
       	}

        /**
        dn("s359001 pass.gety=%d", pass.gety );
      	if ( pass.gety >= 0 ) {
    		getpos = pass.gety;
			#if 1
			d("s4005 gety=%d pointy=[%s] line.point[j].y=[%s]\n", getpos, pointy.s(), line.point[j].y );
			d("_schAttr[getpos].offset=%d _schAttr[getpos].length=%d type=[%s]\n", 
					_schAttr[getpos].offset, _schAttr[getpos].length, _schAttr[getpos].type.s() );
			#endif
       		rc = formatOneCol( pass.tzdiff, pass.srvtmdiff, tablekvbuf, line.point[j].y, errmsg, 
       		 					pointy.s(), _schAttr[getpos].offset, 
       							_schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
       	}
        
		//d("s7830 pass.is3D=%d pass.getz=%d\n", pass.is3D, pass.getz );
       	if ( pass.is3D && pass.getz >= 0 ) {
    		getpos = pass.getz;
	        Jstr pointz = pass.colname + ":z";
			#if 1
			d("s4005 getz=%d pointz=[%s] line.point[j].z=[%s]\n", getpos, pointz.s(), line.point[j].z );
			d("_schAttr[getpos].offset=%d _schAttr[getpos].length=%d type=[%s]\n", 
					_schAttr[getpos].offset, _schAttr[getpos].length, _schAttr[getpos].type.s() );
			#endif
       		rc = formatOneCol( pass.tzdiff, pass.srvtmdiff, tablekvbuf, 
								line.point[j].z, errmsg, 
       							pointz.s(), _schAttr[getpos].offset, _schAttr[getpos].length, 
								_schAttr[getpos].sig, _schAttr[getpos].type );
       	}
        **/

		d("s2238 j=%d jPoint.metrics.size=%d\n", j, jPoint.metrics.size() );
		formatMetricCols( pass.tzdiff, pass.srvtmdiff, pass.dbtab, pass.colname, nmetrics, jPoint.metrics, tablekvbuf );

		dbNaturalFormatExchange( tablekvbuf, _numKeys, _schAttr, 0, 0, " " ); // natural format -> db format

		retpairVec.append( JagDBPair( tablekvbuf, _KEYLEN, tablekvbuf+_KEYLEN, _VALLEN, true ) );
        // each point forms an separate pair

		/**
		d("s4105 retpair.append tablekvbuf\n" );
		d("s4105 tablekvbuf:\n" );
		dumpmem( tablekvbuf, KEYLEN+VALLEN);
		**/

       	if ( pass.getx >= 0 ) {
			memset(tablekvbuf+_schAttr[pass.getx].offset, 0, _schAttr[pass.getx].length );
		}
        /***
       	if ( pass.gety>= 0 ) {
			memset(tablekvbuf+_schAttr[pass.gety].offset, 0, _schAttr[pass.gety].length );
		}
       	if ( pass.is3D && pass.getz >= 0 ) {
			memset(tablekvbuf+_schAttr[pass.getz].offset, 0, _schAttr[pass.getz].length );
		}
        ***/

   	} // end of all points of this linestring column

}

void JagTable::getColumnIndex( const Jstr &dbtab, const Jstr &colname, bool is3D,
								int &getx, int &gety, int &getz, int &getxmin, int &getymin, int &getzmin,
								int &getxmax, int &getymax, int &getzmax,
								int &getid, int &getcol, int &getm, int &getn, int &geti ) const
{
	int getpos;
	Jstr dbcolumn;
	Jstr pointx = colname + ":x";
	Jstr pointy = colname + ":y";
	//Jstr pointz = colname + ":z";

	getx=gety=getz=-1;
	getxmin = getymin = getzmin = getxmax = getymax = getzmax = -1;
	getid = getcol = getm = getn = geti = -1;

    #ifdef JAG_KEEP_MIN_MAX
	dbcolumn = dbtab + ".geo:xmin";
	d("s1029 in getColumnIndex() dbcolumn=[%s]\n", dbcolumn.s() );
	if ( _tablemap->getValue(dbcolumn, getpos) ) { getxmin = getpos; }

	dbcolumn = dbtab + ".geo:ymin";
	if ( _tablemap->getValue(dbcolumn, getpos) ) { getymin = getpos; }

	if ( is3D ) {
	    dbcolumn = dbtab + ".geo:zmin";
	    if ( _tablemap->getValue(dbcolumn, getpos) ) { getzmin = getpos; }
    }

	dbcolumn = dbtab + ".geo:xmax";
	if ( _tablemap->getValue(dbcolumn, getpos) ) { getxmax = getpos; }

	dbcolumn = dbtab + ".geo:ymax";
	if ( _tablemap->getValue(dbcolumn, getpos) ) { getymax = getpos; }

	if ( is3D ) {
	    dbcolumn = dbtab + ".geo:zmax";
	    if ( _tablemap->getValue(dbcolumn, getpos) ) { getzmax = getpos; }
    }
    #endif

	dbcolumn = dbtab + ".geo:id";
	if ( _tablemap->getValue(dbcolumn, getpos) ) { getid = getpos; }

	dbcolumn = dbtab + ".geo:m";
	if ( _tablemap->getValue(dbcolumn, getpos) ) { getm = getpos; }

	dbcolumn = dbtab + ".geo:n";
	if ( _tablemap->getValue(dbcolumn, getpos) ) { getn = getpos; }

	dbcolumn = dbtab + ".geo:col";
	if ( _tablemap->getValue(dbcolumn, getpos) ) { getcol = getpos; }

	dbcolumn = dbtab + ".geo:i";
	if ( _tablemap->getValue(dbcolumn, getpos) ) { geti = getpos; }

	dbcolumn = dbtab + "." + pointx;
	if ( _tablemap->getValue(dbcolumn, getpos) ) { getx = getpos; }

	dbcolumn = dbtab + "." + pointy;
	if ( _tablemap->getValue(dbcolumn, getpos) ) { gety = getpos; }

	if ( is3D ) {
	    Jstr pointz = colname + ":z";
		dbcolumn = dbtab + "." + pointz;
		if ( _tablemap->getValue(dbcolumn, getpos) ) { getz = getpos; }
	}
}

void JagTable::appendOther( JagVector<ValueAttribute> &valueVec,  int n, bool isSub ) const
{
    ValueAttribute other;
    other.issubcol = isSub;
    for ( int i=0; i < n; ++i ) {
        valueVec.append( other );
    }
}

void JagTable::formatMetricCols( int tzdiff, int srvtmdiff, const Jstr &dbtab, const Jstr &colname, 
								 int nmetrics, const JagVector<Jstr> &metrics, char *tablekvbuf ) const
{
	int len = metrics.size();
	if ( len < 1 ) return;
	Jstr col, dbcolumn;
	int getpos;
	int rc;
	Jstr errmsg;
	for ( int i = 0; i < len; ++i ) {
		col = colname + ":m" + intToStr(i+1);
		dbcolumn = dbtab + "." + col;  // test.tab1.col:m1  test.tab1.col:m2 test.tab1.col:m3 
		rc = _tablemap->getValue(dbcolumn, getpos);
		if ( ! rc ) continue;
		formatOneCol( tzdiff, srvtmdiff, tablekvbuf, metrics[i].s(), errmsg,
		              col.s(), _schAttr[getpos].offset,
		              _schAttr[getpos].length, _schAttr[getpos].sig, _schAttr[getpos].type );
	}
}

bool JagTable::hasRollupColumn() const
{
	return _tableRecord.hasRollupColumn;
}

bool JagTable::hasTimeSeries()  const
{
	Jstr sysTser;
	bool rc = _tableRecord.hasTimeSeries( sysTser );
	return rc;
}

bool JagTable::hasTimeSeries( Jstr &tser )  const
{
	Jstr sysTser;
	bool rc  = _tableRecord.hasTimeSeries( sysTser );
	if (! rc ) return false;

	tser = JagSchemaRecord::translateTimeSeriesToStrs( sysTser );
	return true;

}

Jstr JagTable::timeSeriesRentention() const
{
	return _tableRecord.timeSeriesRentention();
}

int JagTable
::rollupPair( const JagRequest &req, JagDBPair &inspair, const JagVector<ValueAttribute> &rollupVec  )
{
    dn("s2022915 rollupPair");

	int 		setindexnum = _indexlist.size(); 
	int 		cnt = 0;
	JagIndex 	*lpindex[_indexlist.size()];
	AbaxBuffer 	bfr;
	Jstr 		dbcolumn;
	const 		JagHashStrInt *maps[1];
	const 		JagSchemaAttribute *attrs[1];	
	int 		lockrc;

	maps[0] = _tablemap;
	attrs[0] = _schAttr;
	
	setindexnum = 0;
	for ( int i = 0; i < _indexlist.size(); ++i ) {
		lpindex[i] = _objectLock->writeLockIndex( JAG_UPDATE_OP, _dbname, _tableName, _indexlist[i], 
										   	    _tableschema, _indexschema, _replicType, 1, lockrc );
		++setindexnum; 
	}

	int keylen[1];
	int numKeys[1];	
	keylen[0] = _KEYLEN;
	numKeys[0] = _numKeys;
	cnt = 0;

	JagFixString getkey ( inspair.key.s(), _KEYLEN, _KEYLEN );
	JagStrSplit sp( _tableName, '@');
	if ( sp.size() < 2 ) return 0;

	Jstr twindow = sp[1];

	convertTimeToWindow( req.session->timediff, twindow, (char*)getkey.s(), "" );

	char 	*tableoldbuf = (char*)jagmalloc(_KEYVALLEN+1);
	char 	*tablenewbuf = (char*)jagmalloc(_KEYVALLEN+1);

	JagDBPair getDBpair( getkey );
	inspair.key = getkey;

	findPairRollupOrInsert( inspair, getDBpair, tableoldbuf, tablenewbuf, setindexnum, lpindex );
	++cnt;

	int totCols = _tableRecord.columnVector->size();
	int nonDateTimeCols = 0;
	Jstr colType;

	for ( int i = 0; i < totCols; i ++ ) {
		if ( ! (*(_tableRecord.columnVector))[i].iskey ) break;
		colType = (*(_tableRecord.columnVector))[i].type;
		if ( colType == JAG_C_COL_TYPE_DATETIMESEC ) {
			continue;
		}
		++ nonDateTimeCols;
	}

	for ( int K = 1; K <= nonDateTimeCols; ++K ) {
		starCombinations( nonDateTimeCols, K, inspair, getDBpair, tableoldbuf, tablenewbuf, setindexnum, lpindex );
	}

	free( tableoldbuf );
	free( tablenewbuf );
	for ( int i = 0; i < _indexlist.size(); ++i ) {
		if ( ! lpindex[i] ) { continue; }
		_objectLock->writeUnlockIndex( JAG_UPDATE_OP, _dbname, _tableName, _indexlist[i], _replicType, 1 );
	}
	return cnt;
}

void JagTable::doRollUp( const JagDBPair &inspair, const char *dbBuf, char *newbuf )
{
    dn("s10228001 JagTable::doRollUp() ... ");

	Jstr    colType, errmsg;
	int     offset, length, sig;
	bool    rc, iskey;
	double  insv ;

	assert( inspair.key.size() == _KEYLEN );
	assert( inspair.value.size() == _VALLEN );

	memcpy( newbuf, dbBuf, _KEYLEN+_VALLEN );

	if ( _counterOffset < 0 ) {
		return;
	}

	char  *insBuf = inspair.newBuffer(); 

    Jstr norm;

    dn("s562003 _counterOffset=%d _counterLength=%d", _counterOffset, _counterLength );
    //dumpmem( dbBuf + _counterOffset, _counterLength );

    JagMath::fromBase254Len( norm, dbBuf+_counterOffset, _counterLength );
	double      dbCounter = norm.tof();

	bool        hasValidCol = false;
	char        cbuf[64];

	Jstr rootname;
	Jstr name;

    dn("s102228045 _tableRecord.print:");
    //_tableRecord.print();

	const   JagVector<JagColumn> &columnVector = *(_tableRecord.columnVector);
	int     totCols = columnVector.size();
    double  dbAvg;

	for ( int i = 0; i < totCols; i ++ ) {

		iskey = columnVector[i].iskey;
		if ( iskey ) continue;  

		colType = columnVector[i].type;
		if ( !isInteger( colType ) && ! isFloat( colType ) ) {
			continue;
		}

		rootname = columnVector[i].name.s();
		if ( rootname.containsStr("::") || rootname == "counter" || rootname == "spare_" ) {
            // do skip :: columns, only look at columns that have original value
			d("s53215 skip column %s in rollup\n", rootname.s() );
			continue;
		}

        // columns like   v1, v2
		offset = columnVector[i].offset;
		length = columnVector[i].length;
		sig = columnVector[i].sig;

        JagMath::fromBase254Len( norm, insBuf + offset, length );
		insv = norm.tof();

        dn("s222098 dumpmem inBuf + offset, length )");
        //dumpmem( inBuf + offset, length );

        dn("s1029288 i=%d name=%s offset=%d length=%d sig=%d insv=%f", i, rootname.s(), offset, length, sig, insv );

        // for each rollup column, rollup its extra stat fields

		name = rootname + "::sum";
		rc = rollupType( name, colType, insv, dbCounter, dbBuf, newbuf, dbAvg );
		if ( rc ) { hasValidCol = true; }

		name = rootname + "::min";
		rc = rollupType( name, colType, insv, dbCounter, dbBuf, newbuf, dbAvg );
		if ( rc ) { hasValidCol = true; }

		name = rootname + "::max";
		rc = rollupType( name, colType, insv, dbCounter, dbBuf, newbuf, dbAvg );
		if ( rc ) { hasValidCol = true; }

		name = rootname + "::avg";
		rc = rollupType( name, colType, insv, dbCounter, dbBuf, newbuf, dbAvg );
		if ( rc ) { hasValidCol = true; }

		name = rootname + "::var";
		rc = rollupType( name, colType, insv, dbCounter, dbBuf, newbuf, dbAvg );
		if ( rc ) { hasValidCol = true; }

		if ( isInteger( colType ) ) {
			sprintf( cbuf, "%lld", (long long)(round(insv)) );
		} else { 
			sprintf( cbuf, "%.5f", insv );
		} 

		formatOneCol( 0, 0, newbuf, cbuf, errmsg, "dummy", offset, length, sig, colType );
	}

	free( insBuf );

	if ( hasValidCol ) {
		dbCounter = dbCounter + 1.0; 
		sprintf( cbuf, "%lld", (long long)(round(dbCounter)) );

		rc = formatOneCol( 0, 0, newbuf, cbuf, errmsg, "dummy", _counterOffset, _counterLength, 0, JAG_C_COL_TYPE_DBIGINT );

		if ( 1 == rc ) { // OK
			JagDBPair resPair;
			resPair.point( newbuf, _KEYLEN, newbuf+_KEYLEN, _VALLEN );

			rc = _darrFamily->set( resPair );

			if ( rc ) {
				d("s443010 _darrFamily->set OK\n" ); 
			} else {
				d("s444014 _darrFamily->set error\n" ); 
			}
		} else {
			d("s402339 formatOneCol error\n");
		}
	} else {
		d("s444034 rollup not done\n" ); 
	}
}

bool JagTable
::rollupType( const Jstr &name, const Jstr &colType, double insv, 
              double dbCounter, const char *dbBuf, char *newbuf, double &dbAvg )
{
    dn("s194002 rollupType name=[%s] dbCounter=%f insv=%f", name.s(), dbCounter, insv );

	double finv;
	int rc, offset, length, sig, getpos;

	Jstr dbcolName = _dbtable + "." + name;
	rc = _tablemap->getValue( dbcolName, getpos);
    dn("s33020 rollupType dbcolName=[%s] getpos=%d rc=%d", dbcolName.s(), getpos, rc );
	if ( ! rc ) {
		return false;
	}

	const JagVector<JagColumn> &columnVector = *(_tableRecord.columnVector);

    // of col::xyz range
	offset = columnVector[getpos].offset;
	length = columnVector[getpos].length;
	sig = columnVector[getpos].sig;

    double dbv;
    Jstr norm;
    JagMath::fromBase254Len( norm, dbBuf + offset, length );
    dbv = norm.tof();

    dn("s220288888 in rollupType dumpmem dbBuf + offset ...");
    //dumpmem(  dbBuf + offset, length );
    dn("s202200 col=%s offset=%d length=%d dbv=%f insv=%f", dbcolName.s(), offset, length, dbv, insv );

	char  cbuf[64];
	if ( name.containsStr("::sum") ) {
		finv = dbv + insv;
	} else if ( name.containsStr("::min") ) {
		finv = ( insv < dbv ) ? insv : dbv;
	} else if ( name.containsStr("::max")  ) {
		finv = ( insv > dbv ) ? insv : dbv;
	} else if ( name.containsStr("::avg") ) {
		finv = dbv + (insv - dbv)/( dbCounter + 1.0);
        //avgVal = finv;
        dbAvg = dbv;
	} else if ( name.containsStr("::var")  ) {
        /***
		const char *pc = strchr( name.s(), ':');
		Jstr avgname(name.s(), pc-name.s() );
		avgname += "::avg";
		Jstr dbcolName = _dbtable + "." + avgname;
		int avgpos;
		rc = _tablemap->getValue( dbcolName, avgpos);
		if ( ! rc ) {
			return false;
		}

		int avgoffset = columnVector[avgpos].offset;
		int avglength = columnVector[avgpos].length;

        JagMath::fromBase254Len( norm, dbBuf + avgoffset, avglength );
        double avgVal = norm.tof();
        ***/

        /**
		double avgNew = avgVal + ( double(inv) - avgVal)/dbCounter;
		finv = dbv + (inv - avgVal ) * ( inv - avgNew );
        ***/
        finv = dbCounter * dbv +  (dbCounter*(insv-dbAvg)*(insv-dbAvg))/(dbCounter+1);
        finv /= (dbCounter+1);

		d("s43220 ::var dbAvg=%f dbCounter=%f dbv=%.3f insv=%.3f  finv=%.4f\n", 
			  dbAvg, dbCounter, dbv, insv, finv );
	} else {
		return false;
	}

	if ( length >= 63 ) return false;

	if ( isInteger( colType ) ) {
		sprintf( cbuf, "%lld", (long long)(round(finv)) );
        dn("s310082 isInteger cbuf=[%s]", cbuf );
	} else if ( isFloat( colType ) ) {
		sprintf( cbuf, "%.5f", finv );
        dn("s310083 isFloat cbuf=[%s]", cbuf );
	} else {
        dn("s2945002 return false");
		return false;
	}

    Jstr errmsg;
	rc = formatOneCol( 0, 0, newbuf, cbuf, errmsg, "dummy", offset, length, sig, colType );
	if ( 1 == rc ) {
		return true;
	} else {
		return false;
	}
}

bool JagTable::convertTimeToWindow( int tzdiff, const Jstr &twindow, char *kbuf, const Jstr &givenColName )
{
	bool 	iskey;
	Jstr 	colType;
	jagint  tval;
	int  	offset, len;
	jagint 	startTime;
	Jstr 	errmsg, name;
	char  	inbuf[32];
	bool 	rc;
	int 	srvtmdiff  = _servobj->servtimediff;

	dn("s444402 convertTimeToWindow() tzdiff=%d srvtmdiff=%d twindow=[%s] givenColName=[%s] kbuf=[%s]",
		tzdiff, srvtmdiff, twindow.s(), givenColName.s(), kbuf );

	const JagVector<JagColumn> &cv = *(_tableRecord.columnVector);

    Jstr norm;

	for ( int i = 0; i < cv.size(); ++i ) {

		iskey = cv[i].iskey;
		if ( ! iskey ) break; 

		colType = cv[i].type;
		if ( ! isDateAndTime( colType ) ) { continue; }

		name = cv[i].name.s();
		if ( givenColName.size() > 0 ) {
			if ( name != givenColName ) {
				continue; 
			}
		} else {
		}

		offset = cv[i].offset;
		len = cv[i].length;

        JagMath::fromBase254Len(norm, kbuf+offset, len );

		//tval = rayatol( kbuf+offset, len ); 
        tval = norm.toLong();

        dn("s100084003 dumpmem kbuf:");
        //dumpmem( kbuf+offset, len );


		dn("s50000220 colType=[%s] tval=%lld", colType.s(), tval );

		if ( colType == JAG_C_COL_TYPE_TIMESTAMPSEC  || colType == JAG_C_COL_TYPE_DATETIMESEC ) {
			dn("s50000220");
			startTime = segmentTime( tzdiff, tval, twindow ); // seconds
		} else if ( colType == JAG_C_COL_TYPE_TIMESTAMPMILLI || colType == JAG_C_COL_TYPE_DATETIMEMILLI ) {
			dn("s50000222");
			tval = tval / 1000; // to seconds
			startTime = segmentTime( tzdiff, tval, twindow ); // seconds
			startTime *= 1000;
		} else if ( colType == JAG_C_COL_TYPE_DATETIMEMICRO || colType == JAG_C_COL_TYPE_TIMESTAMPMICRO ) {
			dn("s50000224");
			tval = tval / 1000000; // to seconds
			startTime = segmentTime( tzdiff, tval, twindow ); // seconds
			startTime *= 1000000;
		} else if ( colType == JAG_C_COL_TYPE_TIMESTAMPNANO || colType == JAG_C_COL_TYPE_DATETIMENANO ) {
			dn("s50000226");
			tval = tval / 1000000000; // to seconds
			startTime = segmentTime( tzdiff, tval, twindow ); // seconds
			startTime *= 1000000000;
		}

		dn("s353402 process this col cvted tval=%lld", tval);

		sprintf(inbuf, "%lld", startTime );
		rc = formatOneCol( tzdiff, srvtmdiff, kbuf, inbuf, errmsg, name, offset, len, 0, colType );
		if ( 1 != rc ) {
			dn("s0391838 error formatOneCol name=[%s] colType=[%s] rc=%d", name.s(), colType.s(), rc );
			return false;
		} else {
			dn("s333301 formatOneCol name=[%s] colType=[%s] offset=%d len=%d tval=%lld startTime=%lld", 
					name.s(), colType.s(), offset, len, tval, startTime, startTime );
		}

		dn("s033204 inbuf=[%s] kbuf=[%s]", inbuf, kbuf );

		break;
	}

	return true;
}

jagint JagTable::segmentTime( int tzdiff, jagint tval, const Jstr &twindow )
{
	jagint  cycle = twindow.toInt();
	char unit = twindow.lastChar();

	tval += tzdiff*60; 

	dn("s55502123 segmentTime twindow=[%s] cycle=%ld unit=%c tzdiff=%d tval=%ld", twindow.s(), cycle, unit, tzdiff, tval );

	jagint seg = -1;
	if ( 's' == unit ) {
		seg =  JagTime::getStartTimeSecOfSecond( tval, cycle ); // 1--15: start from 0-hour of the day
	} else if ( 'm' == unit ) {
		seg =  JagTime::getStartTimeSecOfMinute( tval, cycle ); // 1--15: start from 0-hour of the day
	} else if ( 'h' == unit ) {
		seg =  JagTime::getStartTimeSecOfHour( tval, cycle ); // 1--15: start from 0-hour of the day
	} else if ( 'd' == unit ) {
		seg =  JagTime::getStartTimeSecOfDay( tval, cycle ); // 1--15: start from 0-hour of the day
	} else if ( 'w' == unit ) {
		seg =  JagTime::getStartTimeSecOfWeek( tval ); // 1 week only; start from Sunday
	} else if ( 'M' == unit ) {
		seg =  JagTime::getStartTimeSecOfMonth( tval, cycle ); // cycle: 1, 2, 3, 4, 6; start from first of month
	} else if ( 'q' == unit ) {
		seg =  JagTime::getStartTimeSecOfQuarter( tval, cycle ); // cycle 1 or 2; start from first day of quarter
	} else if ( 'y' == unit ) {
		seg =  JagTime::getStartTimeSecOfYear( tval, cycle );  // any year; start from new year's day
	} else if ( 'D' == unit ) {
		seg =  JagTime::getStartTimeSecOfDecade( tval, cycle );  // any decade; start from new year's day
	} else {
		dn("s2203398");
	}
	
	return seg;
}

int JagTable::findPairRollupOrInsert( JagDBPair &inspair, JagDBPair &getDBpair, 
									  char *tableoldbuf, char *tablenewbuf, int setindexnum, JagIndex *lpindex[] )
{
    dn("s8841001 findPairRollupOrInsert ...");
	int cnt;

    bool  exist = _darrFamily->get( getDBpair );

	if (  ! exist ) {
        dn("s302872 insertPair ..");
        // inspair.printkv();

		insertPair( inspair, false );
		cnt = 0;
	} else {
		memset( tableoldbuf, 0, _KEYVALLEN+1 );
		memset( tablenewbuf, 0, _KEYVALLEN+1 );

        dn("s302472 doRollUp ..");

        // getDBpair.printkv();

		getDBpair.toBuffer( tableoldbuf );

		doRollUp( inspair, tableoldbuf, tablenewbuf );

		cnt = 1;

	    if ( _indexlist.size() > 0 && setindexnum ) {
		    //dbNaturalFormatExchange( tableoldbuf, _numKeys, _schAttr, 0,0, " " ); // db format -> natural format
		    //dbNaturalFormatExchange( tablenewbuf, _numKeys, _schAttr, 0,0, " " ); // db format -> natural format
		    for ( int i = 0; i < setindexnum; ++i ) {
			    if ( lpindex[i] ) {
				    lpindex[i]->updateFromTable( tableoldbuf, tablenewbuf );
			    }
		    }
	    }
	}

	return cnt;
}

void JagTable::initStarPositions( JagVector<int> &pointer, int K )
{
	bool iskey;
	int totCols = _tableRecord.columnVector->size();
	Jstr colType;

	for ( int i = 0; i < totCols; i ++ ) {
		iskey = (*(_tableRecord.columnVector))[i].iskey;
		if ( ! iskey ) break;
		colType = (*(_tableRecord.columnVector))[i].type;
		if ( isDateAndTime( colType ) ) { continue; }

		pointer.append(i);
		if ( pointer.size() == K ) {
			break;
		}
	}
}

void JagTable::starCombinations( int N, int K, JagDBPair &inspair, JagDBPair &getDBpair, 
						 char *tableoldbuf, char *tablenewbuf, int setindexnum, JagIndex *lpindex[] )
{
	if ( N < K ) return;

	bool rc;
	JagVector<int> pointer(K);
	initStarPositions( pointer, K );

	while ( true ) {
		fillStarsAndRollup( pointer, inspair, getDBpair, tableoldbuf, tablenewbuf, setindexnum, lpindex );
		rc = movePointerPositions( pointer ); 
		if ( ! rc ) break;
	}
}

void JagTable::fillStarsAndRollup( const JagVector<int> &pointer, JagDBPair &inspair, JagDBPair &getDBpair, 
								   char *tableoldbuf, char *tablenewbuf, int setindexnum, JagIndex *lpindex[] )
{
	JagDBPair starGetDBPair( getDBpair.key );
	JagDBPair starInsPair( inspair.key, inspair.value );

	fillStars( pointer, starGetDBPair );
	fillStars( pointer, starInsPair );

	findPairRollupOrInsert( starInsPair, starGetDBPair, tableoldbuf, tablenewbuf, setindexnum, lpindex );
}

bool JagTable::movePointerPositions( JagVector<int> &pointer )
{
	bool rc;
	int k = pointer.size()-1;
	if ( k < 0 ) return false;

	while ( true ) {
		rc = findNextPositions( pointer, k );
		if ( rc ) {
			return true;
		} else {
			k = k -1;  
			if ( k < 0 ) {
				return false;
			}
		}
	}
}

bool JagTable::findNextPositions( JagVector<int> &pointer, int k  )
{
	int N = pointer.size();
	int startPosition;

	if ( pointer[k] == -1 ) {
		startPosition = pointer[k-1]+1;
	} else {
		startPosition = pointer[k]+1;
	}

	int nextPosition = findNextFreePosition( startPosition );
	if ( nextPosition < 0 ) {
		pointer[k] = -1;
		return false;
	}
	pointer[k] = nextPosition;

	for ( int i = k+1; i < N; ++i ) {
		nextPosition = findNextFreePosition( nextPosition+1 );
		if ( nextPosition < 0 ) {
			pointer[i] = -1;
			return false;
		}
		pointer[i] = nextPosition;
	}

	return true;
}

int JagTable::findNextFreePosition( int startPosition )
{
	int pos = -1;
	int totCols = _tableRecord.columnVector->size();
	bool found = false;

	for ( int i = startPosition; i < totCols; ++i ) {
		if ( ! (*(_tableRecord.columnVector))[i].iskey ) break;
		if ( isDateAndTime( (*(_tableRecord.columnVector))[i].type ) ) { continue; }
		found = true;
		pos = i;
		break;
	}

	return pos;
}

void JagTable::fillStars( const JagVector<int> &pointer, JagDBPair &starDBPair )
{
	char *buf = (char*)starDBPair.key.s();
	int  i, offset, len;

	for ( int k = 0; k < pointer.size(); ++k ) {
		i = pointer[k];
		offset = (*(_tableRecord.columnVector))[i].offset;
		len = (*(_tableRecord.columnVector))[i].length;
		memset( buf+offset, 0, len );
		*(buf+offset) = JAG_STARC;
	}
}

jagint JagTable::cleanupOldRecords( time_t tsec )
{
	Jstr colType;
	if ( _tableRecord.isFirstColumnDateTime( colType ) ) {
		time_t ttime = JagTime::getTypeTime( tsec, colType );
		if ( ttime == 0) {
			return 0;
		}

		jagint cnt = cleanupOldRecordsByOrderOrScan( 0, ttime, true );
		return cnt;
	} else {
		int colidx = _tableRecord.getFirstDateTimeKeyCol();
		if ( colidx < 0 ) {
			return 0;
		} 

		colType = (*_tableRecord.columnVector)[colidx].type;
		time_t ttime = JagTime::getTypeTime( tsec, colType );
		if ( ttime == 0) {
			return 0;
		}

		jagint cnt = cleanupOldRecordsByOrderOrScan( colidx, ttime, false );
		return cnt;
	}
}

jagint JagTable::cleanupOldRecordsByOrderOrScan( int idx, time_t ttime, bool byOrder )
{
    dn("s3578002 cleanupOldRecordsByOrderOrScan idx=%d ttime=%ld  byOrder=%d", idx, ttime, byOrder );

	jagint cnt = 0;
	JagMinMax minmax;	
	minmax.setbuflen( _KEYLEN );

 	minmax.offset = (*(_tableRecord.columnVector))[idx].offset;
    minmax.length = (*(_tableRecord.columnVector))[idx].length;

	JagMergeReader *ntr = NULL;
	_darrFamily->setFamilyRead( ntr, true, minmax.minbuf, minmax.maxbuf );
	if ( ! ntr ) {
		return 0;
	}

	char *buf = (char*)jagmalloc(_KEYVALLEN+1);
	bool rc;
	time_t dbtime;
    Jstr norm;

	while ( true ) {
		rc = ntr->getNext(buf);
		if ( !rc ) {
			d("s11103 getNext rc=%d break\n", rc );
			break;
		}

        dn("s492001 in cleanupOldRecordsByOrderOrScan dumpmem  offset=%d length=%d", minmax.offset, minmax.length );
        dn("s20300002 dumpmem");
        //dumpmem(buf+minmax.offset, minmax.length );
        dn("s220239 ttime=%ld", ttime );

        JagMath::fromBase254Len(norm, buf+minmax.offset, minmax.length );
		dbtime = norm.tol();
        dn("s20860031 norm=[%s] dbtime=%ld", norm.s(), dbtime );

		if ( byOrder ) {
			if ( dbtime >= ttime ) {
				break;
			}
		} else {
			if ( dbtime >= ttime ) {
				continue;
			}
		}
		
		dbNaturalFormatExchange( buf, _numKeys, _schAttr, 0,0, " " ); // natural format -> db format
		JagDBPair pair( buf, _KEYLEN );

        dn("s202288 _darrFamily->remove pair");

		rc = _darrFamily->remove( pair );

		if ( rc ) {
			++cnt;
			dbNaturalFormatExchange( buf, _numKeys, _schAttr, 0,0, " " ); // db format -> natural format
			_removeIndexRecords( buf );
			removeColFiles(buf );
		} else {
		}
	}

	delete ntr;
	free( buf );
	return cnt;
}

void JagTable::refreshTableRecord()
{
	const JagSchemaRecord* rec= _tableschema->getAttr( _dbtable );
	_tableRecord = *rec;
}


jagint JagTable::memoryBufferSize()
{
    if ( ! _darrFamily ) {
        return 0;
    }

    return _darrFamily->memoryBufferSize();
}

