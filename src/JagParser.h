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
#ifndef _jag_parser_h_
#define _jag_parser_h_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <string>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/error/en.h"

#include <abax.h>
#include <JagCfg.h>
#include <JagUtil.h>
#include <JagVector.h>
#include <JagStrSplit.h>
#include <JagParseParam.h>
#include <JagParseExpr.h>
#include <JagStrSplitWithQuote.h>
#include <JagColumn.h>

class CreateAttribute;

class JagParser
{
  public:
    JagParser( void *obj, bool isCli=false );
	bool parseCommand( const JagParseAttribute &jpa, const Jstr &cmd, JagParseParam *param, Jstr &err );
	const JagColumn* getColumn( const JagParseParam *pparam, const Jstr &colName ) const;
	const JagColumn* getColumn( const Jstr &db, const Jstr &obj, const Jstr &colName ) const;
	static bool  isComplexType( const Jstr &rcs );
	static bool  isGeoType( const Jstr &rcs );
	static bool  isVectorGeoType( const Jstr &rcs );
	static int   vectorShapeCoordinates( const Jstr &rcs );
	static bool  isPolyType( const Jstr &rcs );
	static Jstr getColumns( const char *str );
	static int  checkLineStringData( const char *p );
	static int  checkLineString3DData( const char *p );
	static int  checkPolygonData( const char *p, bool mustClose );
	static int  checkMultiPolygonData( const char *p, bool mustClose, bool is3D );
	static int  checkPolygon3DData( const char *p, bool mustClose );
	static int  addLineStringData( JagLineString &linestr, const char *p );
	static int  getLineStringMinMax( char sep, const char *p, double &xmin, double &ymin, double &xmax, double &ymax );
	static int  addLineString3DData( JagLineString &linestr, const char *p );
	static int  getLineString3DMinMax( char sep, const char *p, double &xmin, double &ymin, double &zmin, 
										double &xmax, double &ymax, double &zmax );
	static void addLineStringData( JagLineString &linestr, const JagStrSplit &sp );
	static void addLineStringData( JagLineString3D &linestr, const JagStrSplit &sp );
	static void addLineString3DData( JagLineString3D &linestr, const JagStrSplit &sp );
	static int  addPolygonData( JagPolygon &pgon, const char *p, bool firstOnly, bool mustClose );
	static int  addPolygonData( Jstr &pgon, const char *p, bool firstOnly, bool mustClose );
	static int  getPolygonMinMax( const char *p, double &xmin, double &ymin, double &xmax, double &ymax );
	static int  addPolygon3DData( JagPolygon &pgon, const char *p, bool firstOnly, bool mustClose );
	static int  addPolygon3DData( Jstr &pgon, const char *p, bool firstOnly, bool mustClose );
	static int  getPolygon3DMinMax( const char *p, double &xmin, double &ymin, double &zmin, 
								    double &xmax, double &ymax, double &zmax );
	static int  addPolygonData( JagPolygon &pgon, const JagStrSplit &sp, bool firstOnly );
	static int  addPolygon3DData( JagPolygon &pgon, const JagStrSplit &sp, bool firstOnly );

	static int  addMultiPolygonData( JagVector<JagPolygon> &pgvec, const char *p, bool firstOnly, bool mustClose, bool is3D );
	static int  addMultiPolygonData( Jstr &pgs, const char *p, bool firstOnly, bool mustClose, bool is3D );
	static int  addMultiPolygonData( JagVector<JagPolygon> &pgvec, const JagStrSplit &sp, bool firstOnly, bool is3D );
	static int  getMultiPolygonMinMax( const char *p, double &xmin, double &ymin, double &xmax, double &ymax );
	static int  getMultiPolygon3DMinMax( const char *p, double &xmin, double &ymin, double &zmin, 
								         double &xmax, double &ymax, double &zmax );
	static Jstr getFieldType( int srid );
	static Jstr getFieldTypeString( int srid );
	static void removeEndUnevenBracket( char *str );
	static void removeEndUnevenBracketAll( char *str );
	static void replaceChar( char *start, char oldc, char newc, char stopchar );
	static int  convertConstantObjToJAG( const JagFixString &instr, Jstr &outstr );
	static bool isInsertSelect( const char *sql );
	static int  getB255LenSig(int inlen, int insig, int &outlen, int &outsig );

	
  private:
	int parseSQL( const JagParseAttribute &jpa, JagParseParam *parseParam, char *cmd, int len );
	int init( const JagParseAttribute &jpa, JagParseParam *parseParam );
	int setTableIndexList( short setType );
	int getAllClauses( short setType );
	int setAllClauses( short setType );
	int setSelectColumn();
	int setGetfileColumn();
	int setSelectGroupBy();
	int setSelectHaving();
	int setSelectOrderBy();
	int setSelectLimit();
	int setSelectTimeout();
	int setSelectPivot();
	int setSelectExport();
	int setLoadColumn();
	int setLoadLine();
	int setLoadQuote();
	int setInsertVector();
	int setUpdateVector();
	int setCreateVector( short setType );
	int setOneCreateColumnAttribute( CreateAttribute &cattr );
	Jstr fillDataType( const char *gettok );
	int getColumnLength( const Jstr &colType );
	
	bool  isValidGrantPerm( Jstr &perm );
	bool  isValidGrantObj(  Jstr &obj );
	void addCreateAttrAndColumn( bool isValue, CreateAttribute &cattr, int &coloffset );
	void addExtraOtherCols( const JagColumn *pcol, ValueAttribute &val, int &numCols );
	void setToRealType( const Jstr &rcs, CreateAttribute &cattr );
	int getTypeNameArg( const char *gettok, Jstr &tname, Jstr &targ, int &collen, int &sig, int &metrics );
	bool hasPolyGeoType( const char *createSQL, int &dim );
	void addBBoxGeomKeyColumns( CreateAttribute &cattr, int polyDim, bool lead, int &offset ); 
	int convertJsonToOther( ValueAttribute &val, const char *json, int jsonlen ); 
	int convertJsonToPoint( const rapidjson::Document &dom, ValueAttribute &val );
	int convertJsonToLineString( const rapidjson::Document &dom, ValueAttribute &val );
	int convertJsonToPolygon( const rapidjson::Document &dom, ValueAttribute &val );
	int convertJsonToMultiPolygon( const rapidjson::Document &dom, ValueAttribute &val );
	int getEachRangeFieldLength( int srid ) const;
	static bool getMetrics( const JagStrSplitWithQuote &sp, JagVector<Jstr> &metrics );
	const char * parseRollup( const char *ptok, CreateAttribute &cattr );


	// data members
	char        *_gettok, *_saveptr;
	JagStrSplit _split;
	JagStrSplitWithQuote _splitwq;
	JagParseParam *_ptrParam;
	JagCfg      *_cfg;
	void        *_obj; // servobj or cliobj
	bool        _isCli;

	JagColumn   _dummy;
};  // end of JagParser

#endif
