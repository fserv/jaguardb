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
#ifndef _JagParseParam_h_
#define _JagParseParam_h_

#include <abax.h>
#include <JagVector.h>
#include <JagParseExpr.h>
#include <JagShape.h>

class JagParser;
class JagHashStrStr;
class JagLineFile;

class ObjectNameAttribute
{
  public:
	Jstr dbName;
	Jstr tableName;	
	Jstr indexName; 
	Jstr colName;
	void init() { dbName = tableName = indexName = colName = ""; }
	ObjectNameAttribute()
	{
		// init();
	}

	ObjectNameAttribute( const ObjectNameAttribute& other )
	{
		copyData( other );
	}

	ObjectNameAttribute& operator=( const ObjectNameAttribute& other )
	{
		if ( this == &other ) return *this;
		copyData( other );
		return *this;
	}

	void copyData( const ObjectNameAttribute& other )
	{
		dbName = other.dbName;
		tableName = other.tableName;
		indexName = other.indexName;
		colName = other.colName;
	}

	void toLower()
	{
		dbName = makeLowerString( dbName );
		//tableName = makeLowerString( tableName );
		//indexName = makeLowerString( indexName );
		colName = makeLowerString( colName );
	}
};

class GroupOrderVecAttr
{
  public:
    GroupOrderVecAttr() { isAsc = true; }
	Jstr name;
	bool isAsc;
	void init() { name = ""; isAsc = true; }
};

/***
class OtherAttribute
{
  public:
  	OtherAttribute();
  	~OtherAttribute();
	void init(); 
	void copyData( const OtherAttribute& other );
	OtherAttribute& operator=( const OtherAttribute& other );
	OtherAttribute( const OtherAttribute& other );
	void print();

	bool hasQuote;
	ObjectNameAttribute objName;
	Jstr valueData;
	bool issubcol;
	Jstr  type;

	JagPoint 		point;
	JagLineString 	linestr;

	bool			is3D;
};
***/

class ValueAttribute
{
  public:
  	ValueAttribute();
  	~ValueAttribute();
	void init(); 
	void copyData( const ValueAttribute& other );
	ValueAttribute& operator=( const ValueAttribute& other );
	ValueAttribute( const ValueAttribute& other );
	void print();

	ObjectNameAttribute objName;
	bool hasQuote;
	Jstr valueData;
	bool issubcol;
	Jstr  type;

	JagPoint 		point;
	JagLineString 	linestr;

	bool			is3D;
};

class CreateAttribute
{
  public:
  	CreateAttribute() { init(); }
	ObjectNameAttribute objName;
	Jstr defValues;
	char spare[JAG_SCHEMA_SPARE_LEN+1];
	Jstr type;
	unsigned int offset;
	unsigned int length;
	unsigned int sig;
	int          srid;
	int    begincol;
	int    endcol;

	int    metrics;
	//int    dummy1; becomes metrics
	// int    dummy2; becomes rollupWhere
	Jstr   rollupWhere;

	int    dummy3;
	int    dummy4;
	int    dummy5;
	int    dummy6;
	int    dummy7;
	int    dummy8;
	int    dummy9;
	int    dummy10;
	
	void init() 
	{ 
		objName.init(); 
		spare[JAG_SCHEMA_SPARE_LEN] = '\0';
		memset(spare, JAG_S_COL_SPARE_DEFAULT, JAG_SCHEMA_SPARE_LEN ); 
		offset = length = sig = srid= 0; 
		begincol = endcol = dummy3 = dummy4 = dummy5 = dummy6 = dummy7 = 0;
		rollupWhere = "";
		dummy8 = dummy9 = dummy10 = 0;
		defValues = "";
		type = "";
		metrics = 0;
	}

	CreateAttribute( const CreateAttribute& other )
	{
		copyData( other );
	}
	
	CreateAttribute& operator=( const CreateAttribute& other )
	{
		copyData( other );
		return *this;
	}

	void copyData( const CreateAttribute& other )
	{
		objName = other.objName;
		for ( int i = 0; i < JAG_SCHEMA_SPARE_LEN; ++i ) {
			spare[i] = other.spare[i];
		}

		type = other.type;
		offset = other.offset;
		length = other.length;
		sig = other.sig;
		defValues = other.defValues;
		srid = other.srid;
		begincol = other.begincol;
		metrics = other.metrics;
		endcol = other.endcol;
		//dummy1 = other.dummy1;
		//dummy2 = other.dummy2;
		rollupWhere = other.rollupWhere;
		dummy3 = other.dummy3;
		dummy6 = other.dummy6;
		dummy7 = other.dummy7;
		dummy8 = other.dummy8;
		dummy9 = other.dummy9;
		dummy10 = other.dummy10;
	}
};

class UpdSetAttribute
{
  public:
	BinaryExpressionBuilder *tree;
	Jstr colName;
	Jstr colList;

	UpdSetAttribute() {
		tree = NULL;
	}

	~UpdSetAttribute() {
		destroy();
	}		
	
	void init( const JagParseAttribute &jpa ) {
		destroy();
		tree = new BinaryExpressionBuilder();
		tree->init( jpa, NULL ); 
		colName = "";
	}

	void destroy() {
		if ( tree ) {
			tree->clean();
			delete tree;
			tree = NULL;
		}
	}

	UpdSetAttribute& operator=( const UpdSetAttribute& other )
	{
		colName = other.colName;
		destroy();
		tree = other.tree;
		UpdSetAttribute *a = (UpdSetAttribute*)&other;
		a->tree = NULL; // tree transfer
		return *this;
	}

	void print() 
	{
		in( "colName=[%s] colList=[%s]", colName.s(), colList.s() );
	}
};

class SelColAttribute
{
  public:
    SelColAttribute( JagParseParam *pparam ) { _pparam = pparam; tree=NULL; }
	BinaryExpressionBuilder *tree;
	Jstr origFuncStr;
	Jstr name;
	Jstr asName;
	Jstr getfileCol;
	Jstr getfilePath;
	Jstr colList;
	int getfileType;
	bool givenAsName;
	JagParseParam *_pparam;
	
	// use when process data
	Jstr type;
	unsigned int offset;
	unsigned int length;
	unsigned int sig;
	int   srid;
	bool isAggregate;
	JagFixString strResult;

	SelColAttribute() {
		tree = NULL;
		_pparam = NULL; 
	}

	~SelColAttribute() {
		destroy();
	}		
	
	void init( const JagParseAttribute &jpa ) {
 		destroy();
		tree = new BinaryExpressionBuilder();
		tree->init( jpa, _pparam ); 
		strResult = origFuncStr = asName = getfileCol = getfilePath = ""; 
		type = " ";
		givenAsName = offset = length = sig = isAggregate = srid = 0;
		getfileType = 0;
	}

	void destroy() {
		if ( tree ) {
			tree->clean();
			delete tree;
			tree = NULL;
		}
	}

	SelColAttribute& operator=( const SelColAttribute& other )
	{
		givenAsName = other.givenAsName;
		origFuncStr = other.origFuncStr;
		asName = other.asName;
		getfileCol = other.getfileCol;
		getfilePath = other.getfilePath;
		offset = other.offset;
		length = other.length;
		sig = other.sig;
		type = other.type;
		isAggregate = other.isAggregate;
		strResult = other.strResult;
		srid = other.srid;
		destroy();
		tree = other.tree;
		SelColAttribute *sca = (SelColAttribute*)&other;
		sca->tree = NULL; // tree transfer
		return *this;
	}
};

class OnlyTreeAttribute
{
  public:
	BinaryExpressionBuilder *tree;
	Jstr colList;

	OnlyTreeAttribute( ) {
		tree = NULL;
	}

	~OnlyTreeAttribute() {
		destroy();
	}		

	void init( const JagParseAttribute &jpa, JagParseParam *pram );
	void destroy();
	OnlyTreeAttribute& operator=( const OnlyTreeAttribute& other )
	{
		destroy();
		tree = other.tree;
		OnlyTreeAttribute *ota = (OnlyTreeAttribute*)&other;
		ota->tree = NULL; // tree transfer
		return *this;
	}
};

class JagParseParam
{
  public:
  	JagParseParam( const JagParser *jps = NULL );
	void init( const JagParseAttribute *ijpa=NULL, bool needClean=true );
  	~JagParseParam();
	JagParseParam( const JagParseParam &other ) { *this = other; }
	void clean();
	void destroy();
	int setSelectWhere( );
	int resetSelectWhere( const Jstr &extraWhere );
	int setupCheckMap();
	Jstr formSelectSQL();
	void print();
	int addPointColumns( const CreateAttribute &cattr );
	int addPoint3DColumns( const CreateAttribute &cattr );
	int addCircleColumns( const CreateAttribute &cattr );
	int addSquareColumns( const CreateAttribute &cattr );
	int addBoxColumns( const CreateAttribute &cattr );
	int addCylinderColumns( const CreateAttribute &cattr );
	int addLineColumns( const CreateAttribute &cattr, bool is3D );
	int addVectorColumns( const CreateAttribute &cattr );
	int addLineStringColumns( const CreateAttribute &cattr, bool is3D );
	int addPolygonColumns( const CreateAttribute &cattr, bool is3D );
	int addTriangleColumns( const CreateAttribute &cattr, bool is3D );
	int addRangeColumns( int colLen, const CreateAttribute &cattr );
	int addColumns( const CreateAttribute &pointcattr,
					bool hasX, bool hasY, bool hasZ,
					bool hasWidth, bool hasDepth, bool hasHeight,
					bool hasNX, bool hasNY );

	short checkCmdMode();

	void fillDoubleSubData( CreateAttribute &cattr, int &offset, int isKey, int isMute, bool isSub, bool isRollup );
	void fillIntSubData( CreateAttribute &cattr, int &offset, int isKey, int isMute, int isSub, bool isRollup );
	void fillSmallIntSubData( CreateAttribute &cattr, int &offset, int isKey, int isMute, int isSub, bool isRollup );
	void fillStringSubData( CreateAttribute &cattr, int &offset, int isKey, int len, int isMute, int isSub, bool isRollup );
	void fillRangeSubData( int colLen, CreateAttribute &cattr, int &offset, int isKey, int isSub, bool isRollup );

	void clearRowHash();
	void initColHash();
	bool isSelectConst() const;
	bool isServSelectConst() const;
	bool isJoin() const;
	static bool isJoin( int code );
	void addMetrics( const CreateAttribute &cattr, int offset, int isKey );
	bool isWindowValid( const Jstr &window );
	bool getWindowPeriod( const Jstr &window, Jstr &period, Jstr &colName );

	
	bool impComplete;
	bool hasExist;
	bool hasForce;
	bool hasColumn;
	bool hasCount1;
	bool hasWhere;
	bool hasGroup;
	bool hasHaving;
	bool hasOrder;
	bool hasLimit;
	bool hasTimeout;
	bool hasPivot;
	bool hasExport;	
	bool dorep;
	bool getFileActualData;
	jagint limitStart;
	jagint limit;
	jagint timeout;
	int keyLength;
	int valueLength;
	int ovalueLength;
	int timediff;
	int exportType;
	int groupType;
	int isMemTable;
	int isChainTable;
	int	opcode;
	char optype;
	char fieldSep;
	char lineSep;
	char quoteSep;
	bool detail;
	bool hasPoly;
	int  polyDim;
	bool hasCountAll;

	const char *origpos;
	const char *tabidxpos;
	const char *endtabidxpos;

	Jstr uid;
	Jstr passwd;
	Jstr dbName;
	Jstr batchFileName;
	Jstr selectTablistClause;
	Jstr selectColumnClause;
	Jstr selectWhereClause;
	Jstr selectGroupClause;
	Jstr selectHavingClause;
	Jstr selectOrderClause;
	Jstr selectLimitClause;
	Jstr selectTimeoutClause;
	Jstr selectPivotClause;
	Jstr selectExportClause;
	Jstr loadColumnClause;
	Jstr loadLineClause;
	Jstr loadQuoteClause;
	Jstr grantPerm;
	Jstr grantObj;
	Jstr grantUser;
	Jstr grantWhere;
	Jstr insertDCSyncHost;
	Jstr origCmd;
	Jstr dbNameCmd;
	Jstr  	_rowUUID;
	Jstr    like;
	Jstr    _allColumns;
	bool    _selectStar;
	short   cmd;
	Jstr    value;
	Jstr    timeSeries;
	Jstr    retain;
	Jstr    window;

	JagVector<ObjectNameAttribute> objectVec;
	JagVector<SelColAttribute> selColVec;

	JagVector<GroupOrderVecAttr> orderVec;

	JagVector<GroupOrderVecAttr> groupVec;

	JagVector<ValueAttribute> valueVec;

	JagVector<CreateAttribute> createAttrVec;

	JagVector<UpdSetAttribute> updSetVec;


	JagVector<Jstr> selAllColVec;

	JagVector<OnlyTreeAttribute> joinOnVec;

	JagVector<OnlyTreeAttribute> whereVec;

	JagHashStrInt *insColMap;
	void initInsColMap();

	JagHashStrInt *joinColMap;
	void initJoinColMap();

	JagHashMap<AbaxString, AbaxPair<AbaxString, jagint>> *treeCheckMap;
	void initTreeCheckMap();

	JagParseAttribute jpa;
	const JagParser  *_jagParser;
	JagParseParam    *_parent;
	JagHashStrStr    *_rowHash;
	JagHashStrStr    *_colHash; // save column names in select and where
	JagLineFile	     *_lineFile;
	Jstr  _cluster;

  protected:
		void initCtor();

};

#endif
