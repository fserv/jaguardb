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

#include <JagParseParam.h>
#include <JagClock.h>
#include <JagLineFile.h>
#include <JagParser.h>
#include "JagHashSetStr.h"

JagParseParam::JagParseParam( const JagParser *jps ) 
{ 
	_jagParser = jps; 
	initCtor( ); 
}

void JagParseParam::initCtor()
{
	_rowHash = NULL;
	_lineFile = NULL;
	_colHash = NULL;

	impComplete = opcode = 0;
	hasExist = hasColumn = hasWhere = hasGroup = hasHaving = hasOrder = 0;
    hasLimit = hasTimeout = hasPivot = hasExport = hasForce = 0;
	limitStart = limit = timeout = keyLength = valueLength = ovalueLength = 0;
	timediff = exportType = groupType = isMemTable = isChainTable = 0;
	getFileActualData = false;
	optype=' '; fieldSep=','; lineSep='\n'; quoteSep='\'';
	dorep = true;
	origpos = NULL;
	tabidxpos = NULL;
	endtabidxpos = NULL;
	hasCountAll = false;
	_selectStar = false;
	cmd = 0;
	detail = false;
	hasPoly = false;
	polyDim = 0;

	treeCheckMap = NULL;
	insColMap = NULL;
	joinColMap = NULL;
}

JagParseParam::~JagParseParam()
{
	destroy();
}

void JagParseParam::init( const JagParseAttribute *ijpa, bool needClean )
{
	if ( needClean ) {
		clean();
	}

	initCtor();

	uid = passwd = dbName = batchFileName = "";
	selectTablistClause = selectColumnClause = selectWhereClause = selectGroupClause = selectHavingClause = "";
	selectOrderClause = selectLimitClause = selectTimeoutClause = selectPivotClause = selectExportClause = "";
	loadColumnClause = loadLineClause = loadQuoteClause = "";
	insertDCSyncHost = "";
	origCmd = "";
	dbNameCmd = "";
	grantPerm = "";
	grantObj = "";
	grantUser = "";
	grantWhere = "";
	like="";

	if ( ijpa ) jpa = *ijpa;
}

void JagParseParam::clean()
{	
	objectVec.clean(); 
	orderVec.clean(); 
	groupVec.clean(); 
	valueVec.clean(); 
	createAttrVec.clean(); 
	updSetVec.clean(); 
	selColVec.clean(); 
	selAllColVec.clean(); 
	joinOnVec.clean(); 
	whereVec.clean(); 
	jpa.clean();

	_selectStar = false;
	cmd = 0;
	value = "";

	if ( insColMap ) {
		insColMap->removeAllKey(); 
	}

	if ( _rowHash ) {
		_rowHash->clean();
	}

	if ( joinColMap ) {
		joinColMap->removeAllKey(); 
	}

	if ( treeCheckMap ) {
		treeCheckMap->removeAllKey();
	}

	if ( _colHash ) {
		_colHash->clean();
	}

	if ( _lineFile ) {
		delete _lineFile;
		_lineFile = NULL;
	}
}

void JagParseParam::destroy()
{
	if ( insColMap ) {
		delete insColMap;
	}

	if ( _rowHash ) {
		delete _rowHash;
	}

	if ( _colHash ) {
		delete _colHash;
	}

	if ( _lineFile ) {
		delete _lineFile;
	}

	if ( treeCheckMap ) {
		delete treeCheckMap;
	}

	if ( joinColMap ) {
		delete joinColMap;
	}
}


int JagParseParam::setSelectWhere( )
{
	if ( this->selectWhereClause.length() < 1 ) return -2500;

	const char *pwhere = this->selectWhereClause.c_str();	
	OnlyTreeAttribute ota;
	ota.init( this->jpa, this );
	this->whereVec.append( ota );

	int last = this->whereVec.length()-1;

	this->whereVec[last].tree->init( this->jpa, this );
	this->setupCheckMap(); 
	this->whereVec[last].tree->parse( _jagParser, pwhere, 1, *treeCheckMap, joinColMap, 
								       this->whereVec[last].colList );
	this->hasWhere = 1;
	return 1;
}

int JagParseParam::resetSelectWhere( const Jstr &extraWhere )
{
	//JagHashMap<AbaxString, AbaxInt> hash;
	JagHashSetStr hash;
	JagStrSplit sp(extraWhere, '|', true );
	Jstr newWhere;
	for ( int i=0; i < sp.length(); ++i ) {
		if ( sp[i].size() < 1 ) continue;
		if ( ! hash.keyExist( sp[i] ) ) {
			//hash.addKeyValue( sp[i].c_str(), 1 );
			hash.addKey( sp[i] );
			if ( newWhere.size() < 1 ) {
				newWhere = sp[i];
			} else {
				newWhere += Jstr(" and ") + sp[i];
			}
		}
	}

	if ( selectWhereClause.size() > 0 && newWhere.size() > 0 ) {
		selectWhereClause = Jstr("(") + selectWhereClause + ") and (" + newWhere + ")";
	} else if ( newWhere.size() > 0 ) {
		selectWhereClause = newWhere;
	}

	return 1;
}

int JagParseParam::setupCheckMap()
{
	initTreeCheckMap();

	if ( treeCheckMap->size() > 0 ) return 0;
	AbaxString pname, fname;
	AbaxPair<AbaxString, jagint> pair;
	for ( int i = 0; i < objectVec.length(); ++i ) {
		pair.value = i;
		if ( JAG_INSERTSELECT_OP == opcode ) pair.value = 0;
		if ( objectVec[i].indexName.length() > 0 ) {
			fname = objectVec[i].dbName + "." + objectVec[i].tableName + "." + objectVec[i].indexName;
			pair.key = fname;
			pname = fname;
			treeCheckMap->addKeyValue( pname, pair );
			pname = objectVec[i].dbName + "." + objectVec[i].indexName;
			treeCheckMap->addKeyValue( pname, pair );
			pname = objectVec[i].indexName;
			treeCheckMap->addKeyValue( pname, pair );
		} else {
			fname = objectVec[i].dbName + "." + objectVec[i].tableName;
			pair.key = fname;
			pname = fname;
			treeCheckMap->addKeyValue( pname, pair );
			pname = objectVec[i].tableName;
			treeCheckMap->addKeyValue( pname, pair );
		}
	}

	pair.key = fname;
	pair.value = objectVec.length();
	treeCheckMap->addKeyValue( "0", pair );
	return 1;
}

ValueAttribute::ValueAttribute()
{
	init();
}

ValueAttribute::~ValueAttribute()
{
}

void ValueAttribute::init( ) 
{
	objName.init(); 
	valueData = ""; hasQuote = 0; type=""; issubcol = false;
	point.init(); 
	linestr.init(); 
	is3D = false;
}

void ValueAttribute::copyData( const ValueAttribute& other )
{
	hasQuote = other.hasQuote;
	objName = other.objName;
	valueData = other.valueData;
	type = other.type;
	point = other.point;
	linestr = other.linestr;
	issubcol = other.issubcol;
}

ValueAttribute& ValueAttribute::operator=( const ValueAttribute& other )
{
	if ( this == &other ) return *this;
	copyData( other );
	return *this;
}

ValueAttribute::ValueAttribute ( const ValueAttribute& other )
{
	copyData( other );
}

void ValueAttribute::print()
{
	d("s6810 ValueAttribute::print():\n" );
	d("  objName=[%s]\n", objName.colName.c_str() );
	d("  valueData=[%s]\n", valueData.c_str() );
	d("  type=[%s]\n", type.c_str() );
	d("  issubcol=[%d]\n", issubcol );
	d("  linestr.size=[%d]\n", linestr.size() );
}

void OnlyTreeAttribute::init( const JagParseAttribute &jpa, JagParseParam *pram ) 
{
 	destroy();
	tree = new BinaryExpressionBuilder();
	tree->init( jpa, pram ); 
}

void OnlyTreeAttribute::destroy() 
{
	if ( tree ) {
		tree->clean();
		delete tree;
		tree = NULL;
	}
}

Jstr JagParseParam::formSelectSQL()
{
	Jstr q;
	formatInsertSelectCmdHeader( this, q );
	q += Jstr("select ") + selectColumnClause + " from " + selectTablistClause + " ";

	if ( hasWhere ) {
		q += Jstr(" where ") + selectWhereClause + " ";
	}

	if ( hasGroup ) {
		q += Jstr(" group by ") + selectGroupClause + " ";
	}

	if ( hasOrder ) {
		q += Jstr(" order by ") +  selectOrderClause + " ";
	}

	if ( hasLimit ) {
		q += Jstr(" limit ") +  selectLimitClause + " ";
	}

	if ( hasTimeout ) {
		q += Jstr(" timeout ") +  selectTimeoutClause + " ";
	}

	if ( hasExport ) {
		q +=  selectExportClause;
	}

	return q;
}

void JagParseParam::print()
{
	d("\n\n\n\n\ns34818 --------------------print parse param----------------------------\n");
	d("jpa.timediff=[%d] jpa.servtimediff=[%d] jpa.dfdbname=[%s]\n", 
			this->jpa.timediff, this->jpa.servtimediff, this->jpa.dfdbname.c_str());
	d("impComplete=[%d] opcode=[%d] optype=[%c]\n", 
			this->impComplete, this->opcode, this->optype);
	d("hasExist=[%d] hasColumn=[%d] hasWhere=[%d] hasForce=[%d]\n", 
			this->hasExist, this->hasColumn, this->hasWhere, this->hasForce );
	d("hasGroup=[%d] hasHaving=[%d] hasOrder=[%d]\n", this->hasGroup, this->hasHaving, this->hasOrder);
	d("hasLimit=[%d] hasTimeout=[%d] hasPivot=[%d] hasExport=[%d]\n", 
		  this->hasLimit, this->hasTimeout, this->hasPivot, this->hasExport);
	d("limitStart=[%d] limit=[%d] timeout=[%d]\n", this->limitStart, this->limit, this->timeout);
	d("keyLength=[%d] valueLength=[%d] ovalueLength=[%d]\n", 
			this->keyLength, this->valueLength, this->ovalueLength);
	d("timediff=[%d] exportType=[%d] groupType=[%d] isMemTable=[%d]\n", 
		 this->timediff, this->exportType, this->groupType, this->isMemTable);
	d("fieldSep=[%c] ", this->fieldSep );
	d("lineSep=[%c] ", this->lineSep );
	d("quoteSep=[%c]\n", this->quoteSep );
	d("uid=[%s] passwd=[%s] ", this->uid.c_str(), this->passwd.c_str());
	d("dbName=[%s] batchFileName=[%s]\n", this->dbName.c_str(), this->batchFileName.c_str());
	d("origCmd=[%s]\n", this->origCmd.c_str());
	d("dbNameCmd=[%s]\n", this->dbNameCmd.c_str());
	d("selectTablistClause=[%s]\n", this->selectTablistClause.c_str());
	d("selectColumnClause=[%s]\n", this->selectColumnClause.c_str());
	d("selectWhereClause=[%s]\n", this->selectWhereClause.c_str());
	d("selectGroupClause=[%s]\n", this->selectGroupClause.c_str());
	d("selectHavingClause=[%s]\n", this->selectHavingClause.c_str());
	d("selectOrderClause=[%s]\n", this->selectOrderClause.c_str());
	d("selectLimitClause=[%s]\n", this->selectLimitClause.c_str());
	d("selectTimeoutClause=[%s]\n", this->selectTimeoutClause.c_str());
	d("selectPivotClause=[%s]\n", this->selectPivotClause.c_str());
	d("selectExportClause=[%s]\n", this->selectExportClause.c_str());
	d("loadColumnClause=[%s]\n", this->loadColumnClause.c_str());
	d("loadLineClause=[%s]\n", this->loadLineClause.c_str());
	d("loadQuoteClause=[%s]\n", this->loadQuoteClause.c_str());
	
	if ( this->objectVec.size() > 0 ) {
		d("-----------------object vector------------------\n");
		for ( int i = 0; i < this->objectVec.size(); ++i ) {
			d("dbName=[%s]   ", this->objectVec[i].dbName.c_str());
			d("tableName=[%s]   ", this->objectVec[i].tableName.c_str());
			d("indexName=[%s]   ", this->objectVec[i].indexName.c_str());
			d("colName=[%s]\n", this->objectVec[i].colName.c_str());
		}	
	}

	if ( this->orderVec.size() > 0 ) {
		d("-----------------order vector------------------\n");
		for ( int i = 0; i < this->orderVec.size(); ++i ) {
			d("name=[%s]   ", this->orderVec[i].name.c_str());
			d("isasc=[%d]\n", this->orderVec[i].isAsc);
		}	
	}

	if ( this->groupVec.size() > 0 ) {
		d("-----------------group vector------------------\n");
		for ( int i = 0; i < this->groupVec.size(); ++i ) {
			d("name=[%s]   ", this->groupVec[i].name.c_str());
			d("isasc=[%d]\n", this->groupVec[i].isAsc);
		}
	}

	if ( this->valueVec.size() > 0 ) {
		d("-----------------other vector------------------\n");
		for ( int i = 0; i < this->valueVec.size(); ++i ) {
			d("dbName=[%s]   ", this->valueVec[i].objName.dbName.c_str());
			d("tableName=[%s]   ", this->valueVec[i].objName.tableName.c_str());
			d("indexName=[%s]   ", this->valueVec[i].objName.indexName.c_str());
			d("colName=[%s]   ", this->valueVec[i].objName.colName.c_str());
			d("valueData=[%s]\n", this->valueVec[i].valueData.c_str());
		}
	}

	if ( this->createAttrVec.size() > 0 ) {
		d("-----------------create vector------------------\n");
		for ( int i = 0; i < this->createAttrVec.size(); ++i ) {
			d("dbName=[%s]   ", this->createAttrVec[i].objName.dbName.c_str());
			d("tableName=[%s]   ", this->createAttrVec[i].objName.tableName.c_str());
			d("indexName=[%s]   ", this->createAttrVec[i].objName.indexName.c_str());
			d("colName=[%s]   ", this->createAttrVec[i].objName.colName.c_str());
			d("spare=[%c", this->createAttrVec[i].spare[0]);
			d("%c", this->createAttrVec[i].spare[1]);
			d("%c", this->createAttrVec[i].spare[2]);
			d("%c", this->createAttrVec[i].spare[3]);
			d("%c", this->createAttrVec[i].spare[4]);
			d("%c", this->createAttrVec[i].spare[5]);
			d("%c", this->createAttrVec[i].spare[6]);
			d("%c", this->createAttrVec[i].spare[7]);
			d("%c]   ", this->createAttrVec[i].spare[8]);
			d("%d]   ", this->createAttrVec[i].spare[9]);
			d("type=[%s]   ", this->createAttrVec[i].type.c_str());
			d("offset=[%d]   ", this->createAttrVec[i].offset);
			d("length=[%d]   ", this->createAttrVec[i].length);
			d("sig=[%d]\n", this->createAttrVec[i].sig);
		}
	}

	if ( this->updSetVec.size() > 0 ) {
		d("-----------------update set vector------------------\n");
		for ( int i = 0; i < this->updSetVec.size(); ++i ) {
			ExprElementNode *root = this->updSetVec[i].tree->getRoot();
			if ( root ) root->print(0); 
			d("\ncolName=[%s]\n", this->updSetVec[i].colName.c_str());
		}
	}	

	if ( this->selColVec.size() > 0 ) {
		d("-----------------select column vector------------------\n");
		for ( int i = 0; i < this->selColVec.size(); ++i ) {
			ExprElementNode *root = this->selColVec[i].tree->getRoot();
			if ( root ) root->print(0);		
			d("\norigFuncStr=[%s]   ", this->selColVec[i].origFuncStr.c_str());
			d("asName=[%s]   ", this->selColVec[i].asName.c_str());
			d("offset=[%d]   ", this->selColVec[i].offset);
			d("length=[%d]   ", this->selColVec[i].length);
			d("sig=[%d]   ", this->selColVec[i].sig);
			d("type=[%s]   ", this->selColVec[i].type.c_str());
			d("isAggregate=[%d]   ", this->selColVec[i].isAggregate);
			d("strResult=[%s] %0x %0x\n", this->selColVec[i].strResult.c_str(), this->selColVec[i].tree, this);
		}	
	}

	if ( this->selAllColVec.size() > 0 ) {
		d("-----------------select all column vector------------------\n");
		for ( int i = 0; i < this->selAllColVec.size(); ++i ) {
			d("i=%d name=[%s]\n", i, this->selAllColVec[i].c_str() );
		}
	}

	if ( this->joinOnVec.size() > 0 ) {
		d("-----------------select join on vector------------------\n");
		for ( int i = 0; i < this->joinOnVec.size(); ++i ) {
			ExprElementNode *root = this->joinOnVec[i].tree->getRoot();
			if ( root ) root->print(0);
			d("\n");
		}
	}

	if ( this->whereVec.size() > 0 ) {
		d("-----------------select where vector size=%d ------------------\n", this->whereVec.size());
		for ( int i = 0; i < this->whereVec.size(); ++i ) {
			ExprElementNode *root = this->whereVec[i].tree->getRoot();
			if ( root ) root->print(0);
			d("\n");
		}
	}	

	if ( treeCheckMap && treeCheckMap->size() > 0 ) {
		d("-----------------tree check map------------------\n");
		d("\n");
	}

}

void JagParseParam::addMetrics( const CreateAttribute &pointcattr, int offset, int isKey )
{
	if ( pointcattr.metrics < 1 ) return;
	for ( int i=0; i < pointcattr.metrics; ++i ) {
		CreateAttribute cattr;
		cattr.objName.colName = pointcattr.objName.colName + ":m" + intToStr(i+1);
		fillStringSubData( cattr, offset, isKey, JAG_METRIC_LEN, 0, true, false );
	}
}

int JagParseParam::addPointColumns( const CreateAttribute &pointcattr )
{
	int iskey = false;
	if ( *(pointcattr.spare) == JAG_C_COL_KEY ) {
		iskey = true;
	}

	int offset = pointcattr.offset;
	CreateAttribute cattr;
	cattr.objName.colName = pointcattr.objName.colName + ":x";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

	cattr.objName.colName = pointcattr.objName.colName + ":y";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

 	addMetrics( pointcattr, offset, iskey );
	return 2 + pointcattr.metrics;
}

int JagParseParam::addPoint3DColumns( const CreateAttribute &pointcattr )
{
	int iskey = false;
	if ( *(pointcattr.spare) == JAG_C_COL_KEY ) {
		iskey = true;
	}

	int offset = pointcattr.offset;
	CreateAttribute cattr;
	cattr.objName.colName = pointcattr.objName.colName + ":x";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

	cattr.objName.colName = pointcattr.objName.colName + ":y";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

	cattr.objName.colName = pointcattr.objName.colName + ":z";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

 	addMetrics( pointcattr, offset, iskey );

	return 3 + pointcattr.metrics;
}

int JagParseParam::addCircleColumns( const CreateAttribute &pointcattr )
{
	int iskey = false;
	if ( *(pointcattr.spare) == JAG_C_COL_KEY ) {
		iskey = true;
	}

	int offset = pointcattr.offset;
	CreateAttribute cattr;
	cattr.objName.colName = pointcattr.objName.colName + ":x";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

	cattr.objName.colName = pointcattr.objName.colName + ":y";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

	cattr.objName.colName = pointcattr.objName.colName + ":a";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

 	addMetrics( pointcattr, offset, iskey );

	return 3;
}

int JagParseParam::addSquareColumns( const CreateAttribute &pointcattr )
{
	int iskey = false;
	if ( *(pointcattr.spare) == JAG_C_COL_KEY ) {
		iskey = true;
	}

	int offset = pointcattr.offset;
	CreateAttribute cattr;
	cattr.objName.colName = pointcattr.objName.colName + ":x";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

	cattr.objName.colName = pointcattr.objName.colName + ":y";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

	cattr.objName.colName = pointcattr.objName.colName + ":a";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );
	cattr.objName.colName = pointcattr.objName.colName + ":nx";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

 	addMetrics( pointcattr, offset, iskey );

	return 3;
}

int JagParseParam::addColumns( const CreateAttribute &pointcattr,
					bool hasX, bool hasY, bool hasZ, 
					bool hasWidth, bool hasDepth, bool hasHeight,
					bool hasNX, bool hasNY )
{
	int iskey = false;
	if ( *(pointcattr.spare) == JAG_C_COL_KEY ) {
		iskey = true;
	}

	int offset = pointcattr.offset;
	CreateAttribute cattr;
	if ( hasX ) {
    	cattr.objName.colName = pointcattr.objName.colName + ":x";
		fillDoubleSubData( cattr, offset, iskey, 0, true, false );
	}
    
	if ( hasY ) {
    	cattr.objName.colName = pointcattr.objName.colName + ":y";
		fillDoubleSubData( cattr, offset, iskey, 0, true, false );
	}
    
	if ( hasZ ) {
    	cattr.objName.colName = pointcattr.objName.colName + ":z";
		fillDoubleSubData( cattr, offset, iskey, 0, true, false );
	}

	if ( hasWidth ) {
    	cattr.objName.colName = pointcattr.objName.colName + ":a";
		fillDoubleSubData( cattr, offset, iskey, 0, true, false );
	}
    
	if ( hasDepth ) {
    	cattr.objName.colName = pointcattr.objName.colName + ":b";
		fillDoubleSubData( cattr, offset, iskey, 0, true, false );
	}

	if ( hasHeight ) {
    	cattr.objName.colName = pointcattr.objName.colName + ":c";
		fillDoubleSubData( cattr, offset, iskey, 0, true, false );
	}
    
	if ( hasNX ) {
    	cattr.objName.colName = pointcattr.objName.colName + ":nx";
		fillDoubleSubData( cattr, offset, iskey, 0, true, false );
	}
    
	if ( hasNY ) {
    	cattr.objName.colName = pointcattr.objName.colName + ":ny";
		fillDoubleSubData( cattr, offset, iskey, 0, true, false );
	}

 	addMetrics( pointcattr, offset, iskey );

	return 4;
}

int JagParseParam::addBoxColumns( const CreateAttribute &pointcattr )
{
	int iskey = false;
	if ( *(pointcattr.spare) == JAG_C_COL_KEY ) {
		iskey = true;
	}

	CreateAttribute cattr;
	int offset = pointcattr.offset;

	cattr.objName.colName = pointcattr.objName.colName + ":x";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

	cattr.objName.colName = pointcattr.objName.colName + ":y";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

	cattr.objName.colName = pointcattr.objName.colName + ":z";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

	cattr.objName.colName = pointcattr.objName.colName + ":a";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

	cattr.objName.colName = pointcattr.objName.colName + ":b";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

	cattr.objName.colName = pointcattr.objName.colName + ":c";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

	cattr.objName.colName = pointcattr.objName.colName + ":nx";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

	cattr.objName.colName = pointcattr.objName.colName + ":ny";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

 	addMetrics( pointcattr, offset, iskey );

	return 6;
}

int JagParseParam::addCylinderColumns( const CreateAttribute &pointcattr )
{
	int iskey = false;
	if ( pointcattr.spare[0] == JAG_C_COL_KEY ) {
		iskey = true;
	}

	CreateAttribute cattr;
	int offset = pointcattr.offset;

	cattr.objName.colName = pointcattr.objName.colName + ":x";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

	cattr.objName.colName = pointcattr.objName.colName + ":y";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

	cattr.objName.colName = pointcattr.objName.colName + ":z";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

	cattr.objName.colName = pointcattr.objName.colName + ":a";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

	cattr.objName.colName = pointcattr.objName.colName + ":c";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

	cattr.objName.colName = pointcattr.objName.colName + ":nx";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

	cattr.objName.colName = pointcattr.objName.colName + ":ny";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

 	addMetrics( pointcattr, offset, iskey );
	return 5;
}

/**
  * returns:  1 help/describe/hello 
  * returns:  2 create/schema
  * returns:  3 insert/insert_select
  * returns:  4 update/delete
  * returns:  5 select/count/getfile
  * returns:  6 join
  * returns:  0 other
  */
short JagParseParam::checkCmdMode()
{
	if ( optype == 'D' ) return 1;
	else if ( optype == 'C' ) return 2;
	else if ( optype == 'W' ) {
		if ( JAG_INSERTSELECT_OP == opcode || JAG_INSERT_OP == opcode ) return 3;
		else if ( JAG_UPDATE_OP == opcode || JAG_DELETE_OP == opcode ) return 4;
		return 0;
	} else if ( optype == 'R' ) {
		if ( JAG_SELECT_OP == opcode || JAG_GETFILE_OP == opcode || JAG_COUNT_OP == opcode ) return 5;
		else if ( isJoin() ) return 6;
		return 0;
	} else if ( JAG_CLILOG_OP == opcode ) {
		return 10;
	} else if ( JAG_ADDCLUSTER_OP == opcode ) {
        return 11;
    }

	return 0;
}

void JagParseParam::fillDoubleSubData( CreateAttribute &cattr, int &offset, int iskey, int isMute, bool isSub, bool isRollup )
{
	cattr.spare[JAG_SCHEMA_SPARE_LEN] = '\0';
	memset( cattr.spare, JAG_S_COL_SPARE_DEFAULT, JAG_SCHEMA_SPARE_LEN );
	cattr.type = JAG_C_COL_TYPE_DOUBLE;
	cattr.offset = offset;
	cattr.length = JAG_GEOM_TOTLEN;	
	cattr.sig = JAG_GEOM_PRECISION;

	if ( iskey ) {
		cattr.spare[0] = JAG_C_COL_KEY;
		this->keyLength += cattr.length;
	} else {
		cattr.spare[0] = JAG_C_COL_VALUE;
		this->valueLength += cattr.length;
	}
	cattr.spare[2] = JAG_ASC;

	if ( isMute ) {
		cattr.spare[5] = JAG_KEY_MUTE;
	}

	if ( isSub ) {
		cattr.spare[6] = JAG_SUB_COL;
	}

	if ( isRollup ) { cattr.spare[7] = JAG_ROLL_UP; }

	this->createAttrVec.append( cattr );
	cattr.init();
	offset += JAG_GEOM_TOTLEN;
}

void JagParseParam::fillIntSubData( CreateAttribute &cattr, int &offset, int iskey, int isMute, int isSub, bool isRollup )
{
	cattr.spare[JAG_SCHEMA_SPARE_LEN] = '\0';
	memset( cattr.spare, JAG_S_COL_SPARE_DEFAULT, JAG_SCHEMA_SPARE_LEN );
	cattr.type = JAG_C_COL_TYPE_DINT;
	cattr.offset = offset;
	cattr.length = JAG_DINT_FIELD_LEN;		
	cattr.sig = 0;
	if ( iskey ) {
		cattr.spare[0] = JAG_C_COL_KEY;
		this->keyLength += cattr.length;
	} else {
		cattr.spare[0] = JAG_C_COL_VALUE;
		this->valueLength += cattr.length;
	}

	cattr.spare[2] = JAG_ASC;
	if ( iskey && isMute ) {
		cattr.spare[5] = JAG_KEY_MUTE;
	}

	if ( isSub ) {
		cattr.spare[6] = JAG_SUB_COL;
	}

	if ( isRollup ) { cattr.spare[7] = JAG_ROLL_UP; }

	this->createAttrVec.append( cattr );
	cattr.init();
	offset += JAG_DINT_FIELD_LEN;
}

void JagParseParam::fillSmallIntSubData( CreateAttribute &cattr, int &offset, int iskey, int isMute, int isSub, bool isRollup )
{
	cattr.spare[JAG_SCHEMA_SPARE_LEN] = '\0';
	memset( cattr.spare, JAG_S_COL_SPARE_DEFAULT, JAG_SCHEMA_SPARE_LEN );
	cattr.type = JAG_C_COL_TYPE_DSMALLINT;
	cattr.offset = offset;
	cattr.length = JAG_DSMALLINT_FIELD_LEN;		
	cattr.sig = 0;
	if ( iskey ) {
		cattr.spare[0] = JAG_C_COL_KEY;
		this->keyLength += cattr.length;
	} else {
		cattr.spare[0] = JAG_C_COL_VALUE;
		this->valueLength += cattr.length;
	}

	cattr.spare[2] = JAG_ASC;

	if ( iskey && isMute ) {
		cattr.spare[5] = JAG_KEY_MUTE;
	}

	if ( isSub ) {
		cattr.spare[6] = JAG_SUB_COL;
	}

	if ( isRollup ) { cattr.spare[7] = JAG_ROLL_UP; }

	this->createAttrVec.append( cattr );
	cattr.init();
	offset += JAG_DSMALLINT_FIELD_LEN;
}


void JagParseParam::fillStringSubData( CreateAttribute &cattr, int &offset, int isKey, int len, int isMute, int isSub, bool isRollup )
{
	cattr.spare[JAG_SCHEMA_SPARE_LEN] = '\0';
	memset( cattr.spare, JAG_S_COL_SPARE_DEFAULT, JAG_SCHEMA_SPARE_LEN );
	cattr.type = JAG_C_COL_TYPE_STR;
	cattr.offset = offset;
	cattr.length = len;		
	cattr.sig = 0;
	if ( isKey ) {
		cattr.spare[0] = JAG_C_COL_KEY;
		this->keyLength += cattr.length;
	} else {
		cattr.spare[0] = JAG_C_COL_VALUE;
		this->valueLength += cattr.length;
	}

	cattr.spare[2] = JAG_RAND;

	if ( isMute ) {
		cattr.spare[5] = JAG_KEY_MUTE;
	}

	if ( isSub ) {
		cattr.spare[6] = JAG_SUB_COL;
	}

	if ( isRollup ) { cattr.spare[7] = JAG_ROLL_UP; }

	this->createAttrVec.append( cattr );
	cattr.init();
	offset += len;
}

void JagParseParam::fillRangeSubData( int colLen, CreateAttribute &cattr, int &offset, int iskey, int isSub, bool isRollup )
{
	cattr.spare[JAG_SCHEMA_SPARE_LEN] = '\0';
	memset( cattr.spare, JAG_S_COL_SPARE_DEFAULT, JAG_SCHEMA_SPARE_LEN );

	Jstr ctype = JagParser::getFieldType( cattr.srid ); 
	cattr.type = ctype;

	cattr.offset = offset;
	cattr.length = colLen;		

	if ( JAG_RANGE_DOUBLE == cattr.srid ) {
		cattr.sig = JAG_DOUBLE_SIG_LEN;
	} else if ( JAG_RANGE_FLOAT == cattr.srid ) {
		cattr.sig = JAG_FLOAT_SIG_LEN;
	} else {
		cattr.sig = 0;
	}

	if ( iskey ) {
		cattr.spare[0] = JAG_C_COL_KEY;
		this->keyLength += cattr.length;
	} else {
		cattr.spare[0] = JAG_C_COL_VALUE;
		this->valueLength += cattr.length;
	}

	cattr.spare[2] = JAG_ASC;

	if ( isSub ) {
		cattr.spare[6] = JAG_SUB_COL;
	}

	if ( isRollup ) { cattr.spare[7] = JAG_ROLL_UP; }

	this->createAttrVec.append( cattr );
	cattr.init();
	offset += colLen;
}

int JagParseParam::addLineColumns( const CreateAttribute &pointcattr, bool is3D )
{
	int iskey = false;
	if ( *(pointcattr.spare) == JAG_C_COL_KEY ) {
		iskey = true;
	}

	CreateAttribute cattr;
	int offset = pointcattr.offset;

	cattr.objName.colName = pointcattr.objName.colName + ":x1";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

	cattr.objName.colName = pointcattr.objName.colName + ":y1";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

	if ( is3D ) {
		cattr.objName.colName = pointcattr.objName.colName + ":z1";
		fillDoubleSubData( cattr, offset, iskey, 0, true, false );
	}

	cattr.objName.colName = pointcattr.objName.colName + ":x2";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

	cattr.objName.colName = pointcattr.objName.colName + ":y2";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

	if ( is3D ) {
		cattr.objName.colName = pointcattr.objName.colName + ":z2";
		fillDoubleSubData( cattr, offset, iskey, 0, true, false );
	}

 	addMetrics( pointcattr, offset, iskey );

	return 4;
}

int JagParseParam::addVectorColumns( const CreateAttribute &pointcattr )
{
	int iskey = false;
	if ( pointcattr.spare[0] == JAG_C_COL_KEY ) {
		iskey = true;
	}

	CreateAttribute cattr;
	int offset = pointcattr.offset;

	cattr.objName.colName = pointcattr.objName.colName + ":x";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

 	addMetrics( pointcattr, offset, iskey );

	return 4;
}

int JagParseParam::addLineStringColumns( const CreateAttribute &pointcattr, bool is3D )
{
	int iskey = false;
	if ( pointcattr.spare[0] == JAG_C_COL_KEY ) {
		iskey = true;
	}

	CreateAttribute cattr;
	int offset = pointcattr.offset;

	cattr.objName.colName = pointcattr.objName.colName + ":x";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

	cattr.objName.colName = pointcattr.objName.colName + ":y";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

	if ( is3D ) {
		cattr.objName.colName = pointcattr.objName.colName + ":z";
		fillDoubleSubData( cattr, offset, iskey, 0, true, false );
	}

 	addMetrics( pointcattr, offset, iskey );

	return 4;
}

int JagParseParam::addPolygonColumns( const CreateAttribute &pointcattr, bool is3D )
{
	int iskey = false;
	if ( pointcattr.spare[0] == JAG_C_COL_KEY ) {
		iskey = true;
	}

	CreateAttribute cattr;
	int offset = pointcattr.offset;

	cattr.objName.colName = pointcattr.objName.colName + ":x";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

	cattr.objName.colName = pointcattr.objName.colName + ":y";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

	if ( is3D ) {
		cattr.objName.colName = pointcattr.objName.colName + ":z";
		fillDoubleSubData( cattr, offset, iskey, 0, true, false );
	}

 	addMetrics( pointcattr, offset, iskey );

	return 4;
}

int JagParseParam::addTriangleColumns( const CreateAttribute &pointcattr, bool is3D )
{
	int iskey = false;
	if ( *(pointcattr.spare) == JAG_C_COL_KEY ) {
		iskey = true;
	}

	CreateAttribute cattr;
	int offset = pointcattr.offset;

	cattr.objName.colName = pointcattr.objName.colName + ":x1";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

	cattr.objName.colName = pointcattr.objName.colName + ":y1";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

	if ( is3D ) {
		cattr.objName.colName = pointcattr.objName.colName + ":z1";
		fillDoubleSubData( cattr, offset, iskey, 0, true, false );
	}

	cattr.objName.colName = pointcattr.objName.colName + ":x2";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

	cattr.objName.colName = pointcattr.objName.colName + ":y2";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

	if ( is3D ) {
		cattr.objName.colName = pointcattr.objName.colName + ":z2";
		fillDoubleSubData( cattr, offset, iskey, 0, true, false );
	}

	cattr.objName.colName = pointcattr.objName.colName + ":x3";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

	cattr.objName.colName = pointcattr.objName.colName + ":y3";
	fillDoubleSubData( cattr, offset, iskey, 0, true, false );

	if ( is3D ) {
		cattr.objName.colName = pointcattr.objName.colName + ":z3";
		fillDoubleSubData( cattr, offset, iskey, 0, true, false );
	}
 	addMetrics( pointcattr, offset, iskey );

	return 4;
}

int JagParseParam::addRangeColumns( int colLen, const CreateAttribute &pointcattr )
{
	int iskey = false;
	if ( *(pointcattr.spare) == JAG_C_COL_KEY ) {
		iskey = true;
	}

	CreateAttribute cattr;
	int offset = pointcattr.offset;
	cattr.srid = pointcattr.srid;

	cattr.objName.colName = pointcattr.objName.colName + ":begin";
	fillRangeSubData( colLen, cattr, offset, iskey, true, false );

	cattr.srid = pointcattr.srid;
	cattr.objName.colName = pointcattr.objName.colName + ":end";
	fillRangeSubData( colLen, cattr, offset, iskey, true, false );

	return 2;
}

void JagParseParam::clearRowHash()
{
    if ( ! hasWhere ) return;
    ExprElementNode *root = whereVec[0].tree->getRoot();
    if ( root && root->_builder && root->_builder->_pparam->_rowHash ) {
        delete root->_builder->_pparam->_rowHash;
        root->_builder->_pparam->_rowHash = NULL;
	}
}

void JagParseParam::initColHash()
{
	if ( _colHash ) return;
	_colHash = new JagHashStrStr();
}

bool JagParseParam::isSelectConst() const
{
	if ( objectVec.size() < 1 && opcode == JAG_SELECT_OP ) {
		return true;
	}
	return false;

    /**
	if ( _allColumns.size() < 1 && ! _selectStar && objectVec.size() < 1 ) {
		return true;
	} else {
		return false;
	}
	***/
}

bool JagParseParam::isServSelectConst() const
{
    dn("JagParseParam::isSelectConst() p30381 objectVec.size=%d > 0 tableName=[%s]", objectVec.size(), objectVec[0].tableName.s() );

	if ( objectVec.size() > 0 ) {
        dn("p30381 objectVec.size=%d > 0 tableName=[%s]", objectVec.size(), objectVec[0].tableName.s() );
		if ( objectVec[0].tableName == "_SYS_" ) {
			return true;
		}
	}
	return false;

    /**
	if ( _allColumns.size() < 1 && ! _selectStar && objectVec.size() < 1 ) {
		return true;
	} else {
		return false;
	}
	***/
}

bool JagParseParam::isJoin() const
{
	return isJoin( opcode );
}

bool JagParseParam::isJoin( int code ) 
{
    if (JAG_INNERJOIN_OP == code ||
        JAG_LEFTJOIN_OP == code ||
        JAG_RIGHTJOIN_OP == code ||
        JAG_FULLJOIN_OP == code )
    {
        return true;
    }
    return false;
}


void JagParseParam::initTreeCheckMap()
{
	if ( treeCheckMap == NULL ) {
		treeCheckMap = new JagHashMap<AbaxString, AbaxPair<AbaxString, jagint> >();
	}
}

void JagParseParam::initInsColMap()
{
	if ( insColMap == NULL ) {
		insColMap = new JagHashStrInt();
	}
}

void JagParseParam::initJoinColMap()
{
	if ( joinColMap == NULL ) {
		joinColMap = new JagHashStrInt();
	}
}

bool JagParseParam::isWindowValid( const Jstr &window )
{
	Jstr params = window.substrc('(', ')');
	if ( params.size() < 1 ) return false;
	
	if ( ! params.containsChar(',') ) return false;
	params.replace(',', ' ' );
	JagStrSplit sp(params, ' ', true);

	Jstr w = sp[0];
	Jstr col = sp[1];

	char lastc = w.lastChar();
	if ( lastc == 's' || lastc == 'm' || lastc == 'h' || lastc == 'd'
	     || lastc == 'w' || lastc == 'M' || lastc == 'q' || lastc == 'y' || lastc == 'D' ) {
	} else {
		return false;
	}

	return isValidCol( col.s() );
}

bool JagParseParam::getWindowPeriod( const Jstr &window, Jstr &period, Jstr &colName )
{
	Jstr params = window.substrc('(', ')');
	if ( params.size() < 1 ) return false;
	if ( ! params.containsChar(',') ) return false;
	params.replace(',', ' ' );
	JagStrSplit sp(params, ' ', true);
	period = sp[0];
	colName = sp[1];
	return true;
}

