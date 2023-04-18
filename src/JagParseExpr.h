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
#ifndef _ParseExpr_h_
#define _ParseExpr_h_

#include <stdio.h>
#include <stack>
#include <string>
#include <regex>
#include <abax.h>

#include <abax.h>
#include <JagDef.h>
#include <JagHashMap.h>
#include <JagColumn.h>
#include <JagSchemaRecord.h>
#include <JagVector.h>
#include <JagUtil.h>
#include <JagExprStack.h>
#include <JagTableUtil.h>
#include <JagHashStrInt.h>
#include <JagMergeReader.h>
#include <JagMergeBackReader.h>
#include <JagHashStrStr.h>

/**
				---------------------------
				| ExprElementNode    |
				---------------------------
					|			 	     |
					|	  			     |
					|				     |
					|	             -------------------------
					|	             |  BinaryOpNode  |
					|	             -------------------------
		-------------------------
	   |   StringElementNode     |
		-------------------------
**/

class JagParser;
class JagHashStrStr;
class BinaryExpressionBuilder;
class JagTableOrIndexAttrs;


class JagColumnBox 
{
  public:
  	int col;
	int offset;
	int len;
};

class ExprElementNode
{
  public:
	ExprElementNode();
	virtual ~ExprElementNode();
	virtual int getBinaryOp() = 0;
	virtual int getName( const char *&p ) = 0;
	virtual bool getValue( const char *&p ) = 0;
	virtual void clear() = 0;
	virtual void print( int mode=0 ) = 0;
	virtual int getClusterByUuid( const JagTableOrIndexAttrs *objAttr) = 0;
	virtual unsigned char getNodeType() = 0;
	
	// for select/where/on tree use
	virtual int setWhereRange( const JagHashStrInt *maps[], const JagSchemaAttribute *attrs[], 
								const int keylen[], const int numKeys[], int numTabs, bool &hasValue, 
								JagMinMax *minmaxbuf, JagFixString &str, int &typeMode, int &tabnum ) = 0;
	virtual int setFuncAttribute( const JagHashStrInt *maps[], const JagSchemaAttribute *attrs[], 
									int &constMode, int &typeMode, bool &isAggregate, Jstr &type, 
									int &collen, int &siglen ) = 0;
	virtual int getFuncAggregate( JagVector<Jstr> &selectParts, JagVector<int> &selectPartsOpcode,
									JagVector<int> &selColSetAggParts, JagHashMap<AbaxInt, AbaxInt> &selColToselParts, 
									int &nodenum, int treenum ) = 0;
	virtual int getAggregateParts( Jstr &parts, int &nodenum ) = 0;
	virtual int setAggregateValue( int nodenum, const char *buf, int length, const Jstr &type ) = 0;
	virtual int checkFuncValid( JagMergeReaderBase *ntr,  const JagHashStrInt *maps[], const JagSchemaAttribute *attrs[],
								const char *buffers[], JagFixString &str, int &typeMode, Jstr &type, 
								int &length, bool &first, bool useZero, bool setGlobal ) = 0;
	virtual int checkFuncValidConstantOnly( JagFixString &str, int &typeMode, Jstr &type, int &length ) = 0;
	virtual int checkFuncValidConstantCalc( JagFixString &str, int &typeMode, Jstr &type, int &length ) = 0;

	JagParseAttribute   		_jpa;
	BinaryExpressionBuilder* 	_builder;
	bool            	_isElement;
	Jstr				_name;
	unsigned int		_srid;
	Jstr				_type;
	int					_nargs;
	int					_metrics;
	bool 	            _isDestroyed;
	unsigned char       _nodeType;
};

class StringElementNode: public ExprElementNode
{
  public:
	StringElementNode();
	StringElementNode( BinaryExpressionBuilder* builder,  const Jstr &name, const JagFixString &value, 
					   const JagParseAttribute &jpa, int tabnum, int typeMode=0 ) ;
	virtual ~StringElementNode();
	virtual int getBinaryOp() { return 0; }
	virtual int getName( const char *&p ) { 
		if ( _name.length() > 0 ) { 
			p = _name.c_str(); 
			return _tabnum; 
		}
		return -1; 
	}
	virtual bool getValue( const char *&p ) { 
		if ( _value.length() > 0 ) { 
			p = _value.c_str(); 
			return true; 
		}
		return false; 
	}
	virtual void clear();
	virtual void print( int mode=0 );
	virtual int getClusterByUuid( const JagTableOrIndexAttrs *objAttr);
	virtual unsigned char getNodeType() { return JAG_ELEMENT_NODE; }

	
	// for select/where/on tree use
	virtual int setWhereRange( const JagHashStrInt *maps[], const JagSchemaAttribute *attrs[], 
								const int keylen[], const int numKeys[], int numTabs, bool &hasValue, 
								JagMinMax *minmaxbuf, JagFixString &str, int &typeMode, int &tabnum );
	virtual int setFuncAttribute( const JagHashStrInt *maps[], const JagSchemaAttribute *attrs[], 
								  int &constMode, int &typeMode, bool &isAggregate, Jstr &type, int &collen, int &siglen );
	virtual int getFuncAggregate( JagVector<Jstr> &selectParts, JagVector<int> &selectPartsOpcode, 
								  JagVector<int> &selColSetAggParts, JagHashMap<AbaxInt, AbaxInt> &selColToselParts, 
								  int &nodenum, int treenum );
	virtual int getAggregateParts( Jstr &parts, int &nodenum );
	virtual int setAggregateValue( int nodenum, const char *buf, int length, const Jstr &type );
	virtual int checkFuncValid(JagMergeReaderBase *ntr, const JagHashStrInt *maps[], const JagSchemaAttribute *attrs[],
								const char *buffers[], JagFixString &str, int &typeMode, Jstr &type, 
								int &length, bool &first, bool useZero, bool setGlobal ) ;
	virtual int checkFuncValidConstantOnly( JagFixString &str, int &typeMode, Jstr &type, int &length );
	virtual int checkFuncValidConstantCalc( JagFixString &str, int &typeMode, Jstr &type, int &length );
	void makeDataString( const JagSchemaAttribute *attrs[], const char *buffers[], 
						 const Jstr &colobjstr, JagFixString &str );
	void makeRangeDataString( const JagSchemaAttribute *attrs[], const char *buffers[], 
						 const Jstr &colobjstr, JagFixString &str );
    void getPolyDataString( JagMergeReaderBase *ntr, const Jstr &polyType, const JagHashStrInt *maps[],
							 const JagSchemaAttribute *attrs[],
	                         const char *buffers[], JagFixString &str );
    void getPolyData( const Jstr &polyType, JagMergeReaderBase *ntr, const JagHashStrInt *maps[], 
							 const JagSchemaAttribute *attrs[],
	                         const char *buffers[], JagFixString &str, bool is3D );
    void savePolyData( const Jstr &polyType, JagMergeReaderBase *ntr, const JagHashStrInt *maps[], 
							 const JagSchemaAttribute *attrs[],
	                         const char *buffers[], const Jstr &uuid, const Jstr &db, const Jstr &tab, 
							 const Jstr &col, bool isBoundBox3D, bool is3D=false );

	void addDataStrting( const char *buffers[], const JagSchemaAttribute *attrs[], int begin, int len, Jstr &bufstr );
	void addMetricString( const char *buffers[], const JagSchemaAttribute *attrs[], int begin, Jstr &bufstr );


	JagFixString		_value;
	unsigned int		_tabnum;
	int					_typeMode;
	int					_nodenum; // a number set to distinguish each node of the tree

	unsigned int    _begincol;
	unsigned int    _endcol;
	unsigned int	_offset;
	unsigned int	_length;
	unsigned int	_sig;

};  // end StringElementNode

class BinaryOpNode: public ExprElementNode
{
  public:
	BinaryOpNode( BinaryExpressionBuilder* builder, short op, int opArgs, ExprElementNode *l, ExprElementNode *r, 
					    const JagParseAttribute &jpa, int arg1=0, int arg2=0, Jstr carg1="" );
	virtual ~BinaryOpNode();
	//BinaryOpNode &operator=(const BinaryOpNode& n) {}
	BinaryOpNode &operator=(const BinaryOpNode& n);
	virtual int getBinaryOp() { return _binaryOp; }
	virtual int getName( const char *&p ) { return -1; }
	virtual bool getValue( const char *&p ) { return false; }
	virtual void clear();
	virtual void print( int mode=0 );
	virtual int getClusterByUuid( const JagTableOrIndexAttrs *objAttr);
	virtual unsigned char getNodeType() { return JAG_BINARY_NODE; }

	// for select/where/on tree use
	virtual int setWhereRange( const JagHashStrInt *maps[], const JagSchemaAttribute *attrs[], 
								const int keylen[], const int numKeys[], int numTabs, bool &hasValue, 
								JagMinMax *minmaxbuf, JagFixString &str, int &typeMode, int &tabnum );
	virtual int setFuncAttribute( const JagHashStrInt *maps[], const JagSchemaAttribute *attrs[], 
									int &constMode, int &typeMode, bool &isAggregate, Jstr &type, 
									int &collen, int &siglen );
	virtual int getFuncAggregate( JagVector<Jstr> &selectParts, JagVector<int> &selectPartsOpcode, 
									JagVector<int> &selColSetAggParts, JagHashMap<AbaxInt, AbaxInt> &selColToselParts, 
									int &nodenum, int treenum );
	virtual int getAggregateParts( Jstr &parts, int &nodenum );
	virtual int setAggregateValue( int nodenum, const char *buf, int length, const Jstr &type );
	virtual int checkFuncValid(JagMergeReaderBase *ntr, const JagHashStrInt *maps[], const JagSchemaAttribute *attrs[],
							    const char *buffers[], JagFixString &str, int &typeMode, Jstr &type, 
								int &length, bool &first, bool useZero, bool setGlobal );
	virtual int checkFuncValidConstantOnly( JagFixString &str, int &typeMode, Jstr &type, int &length );
	virtual int checkFuncValidConstantCalc( JagFixString &str, int &typeMode, Jstr &type, int &length );
	static Jstr binaryOpStr( short binaryOp );
	static bool isAggregateOp( short op );
	static bool isMathOp( short op );
	static bool isCompareOp( short op );
	static bool isStringOp( short op );
	static bool isSpecialOp( short op );
	static bool isTimedateOp( short op );
	static int  timeDateOpLen( short op );
	static short getFuncLength( short op );

	// data members
	short			_binaryOp;
	int				_arg1; // use for substr and datediff (for now)
	int				_arg2; // use for substr and datediff (for now)
	Jstr			_carg1; // use for substr and datediff (for now)
	ExprElementNode	*_left;
	ExprElementNode	*_right;
	int				_nodenum; // a number set to distinguish each node of the tree
	
  protected:
	// current class object use only
	void findOrBuffer( JagMinMax *minmaxbuf, JagMinMax *leftbuf, JagMinMax *rightbuf, 
						const int keylen[], int numTabs );
	void findAndBuffer( JagMinMax *minmaxbuf, JagMinMax *leftbuf, JagMinMax *rightbuf, 
						const JagSchemaAttribute *attrs[], int numTabs, const int numKeys[] );
	void findLeftBuffer( JagMinMax *minmaxbuf, JagMinMax *leftbuf, JagMinMax *rightbuf, 
						const JagSchemaAttribute *attrs[], int numTabs, const int numKeys[] );
	bool formatColumnData( JagMinMax *minmaxbuf, const JagMinMax *iminmaxbuf, const JagFixString &value, int tabnum, int minOrMax );
	bool checkAggregateValid( int lcmode, int rcmode, bool laggr, bool raggr );
	int formatAggregateParts( Jstr &parts, Jstr &lparts, Jstr &rparts );

	int _doWhereCalc( const JagHashStrInt *maps[], const JagSchemaAttribute *attrs[], 
						const int keylen[], const int numKeys[], int numTabs, int ltmode, int rtmode, int ltabnum, int rtabnum,
						JagMinMax *minmaxbuf, JagMinMax *lminmaxbuf, JagMinMax *rminmaxbuf, 
						JagFixString &str, const JagFixString &lstr, const Jstr &ltype, 
                        const JagFixString &rstr, const Jstr &rtype );

	int _doCalculation( JagFixString &lstr, JagFixString &rstr, 
						int &ltmode, int &rtmode, const Jstr& ltype,  const Jstr& rtype, 
						int llength, int rlength, bool &first );	

	bool processBooleanOp( int op, const JagFixString &lstr, const JagFixString &rstr, const Jstr &carg );
	Jstr  processTwoStrOp( int op, const JagFixString &lstr, const JagFixString &rstr, const Jstr &carg );
	bool processSingleDoubleOp( int op, const JagFixString &lstr, const Jstr &carg, double &val );
	bool processSingleStrOp( int op, const JagFixString &lstr, const Jstr &carg, Jstr &val );
	bool doBooleanOp( int op, const Jstr& mark1, const Jstr &colType1, int srid1, const JagStrSplit &sp1,
							 const Jstr& mark2, const Jstr &colType2, int srid2, 
							 const JagStrSplit &sp2, const Jstr &carg );
	Jstr doTwoStrOp( int op, const Jstr& mark1, const Jstr &colType1, int srid1, JagStrSplit &sp1,
							 const Jstr& mark2, const Jstr &colType2, int srid2, 
							 JagStrSplit &sp2, const Jstr &carg );
	bool doSingleDoubleOp( int op, const Jstr& mark1, const Jstr &colType1, int srid1, const JagStrSplit &sp1,
							 const Jstr &carg, double &val );
	bool doSingleStrOp( int op, const Jstr& mark1, const Jstr& hdr, const Jstr &colType1, int srid1, 
						JagStrSplit &sp1, const Jstr &carg, Jstr &val );

	bool doAllWithin( const Jstr& mark1, const Jstr &colType1, int srid1, const JagStrSplit &sp1,
							 const Jstr& mark2, const Jstr &colType2, int srid2, const JagStrSplit &sp2, 
							 bool strict=true );
	bool doAllSame( const Jstr& mark1, const Jstr &colType1, int srid1, const JagStrSplit &sp1,
							 const Jstr& mark2, const Jstr &colType2, int srid2, const JagStrSplit &sp2 );
	bool doAllClosestPoint( const Jstr& mark1, const Jstr &colType1, int srid1, const JagStrSplit &sp1,
							 const Jstr& mark2, const Jstr &colType2, int srid2, const JagStrSplit &sp2, 
							 Jstr &res );
	bool doAllAngle2D( const Jstr& mark1, const Jstr &colType1, int srid1, JagStrSplit &sp1,
							 const Jstr& mark2, const Jstr &colType2, int srid2, JagStrSplit &sp2, 
							 Jstr &res );
	bool doAllAngle3D( const Jstr& mark1, const Jstr &colType1, int srid1, JagStrSplit &sp1,
							 const Jstr& mark2, const Jstr &colType2, int srid2, JagStrSplit &sp2, 
							 Jstr &res );
	bool doAllIntersect( const Jstr& mark1, const Jstr &colType1, int srid1, const JagStrSplit &sp1,
							 const Jstr& mark2, const Jstr &colType2, int srid2, const JagStrSplit &sp2, 
							 bool strict=true );
	bool doAllNearby( const Jstr& mark1, const Jstr &colType1, int srid1, const JagStrSplit &sp1,
							 const Jstr& mark2, const Jstr &colType2, int srid2, const JagStrSplit &sp2, 
							 const Jstr &carg );
	bool doAllArea( const Jstr& mk1, const Jstr &colType1, int srid1, const JagStrSplit &sp1, double &val );
	bool doAllPerimeter( const Jstr& mk1, const Jstr &colType1, int srid1, const JagStrSplit &sp1, double &val );
	bool doAllVolume( const Jstr& mk1, const Jstr &colType1, int srid1, const JagStrSplit &sp1, double &val );
	bool doAllMinMax( int op, int srid, const Jstr& mk1, const Jstr &colType1, const JagStrSplit &sp1, double &val );
	bool doAllPointN( const Jstr& mk1, const Jstr &colType1, int srid1, JagStrSplit &sp1, 
					  const Jstr &carg, Jstr &val );
	bool doAllExtent( int srid, const Jstr& mk1, const Jstr &colType1, const JagStrSplit &sp1, Jstr &val );
	bool doAllStartPoint( const Jstr& mk1, const Jstr &colType1, const JagStrSplit &sp1, Jstr &val );
	bool doAllEndPoint( const Jstr& mk1, const Jstr &colType1, const JagStrSplit &sp1, Jstr &val );
	bool doAllConvexHull( const Jstr& mk1, const Jstr& hdr, const Jstr &colType1, 
						  int srid, const JagStrSplit &sp1, Jstr &val );
	bool doAllConcaveHull( const Jstr& mk1, const Jstr& hdr, const Jstr &colType1, 
						  int srid, const JagStrSplit &sp1, Jstr &val );
	bool doAllToPolygon( const Jstr& mk1, const Jstr& hdr, const Jstr &colType1, 
						  int srid, const JagStrSplit &sp1, const Jstr& carg, Jstr &val );
	bool doAllToMultipoint( const Jstr& mk1, const Jstr& hdr, const Jstr &colType1, 
						  int srid, const JagStrSplit &sp1, const Jstr& carg, Jstr &val );
	bool doAllAsText( const Jstr& mk1, const Jstr& hdr, const Jstr &colType1, 
						  int srid, const JagStrSplit &sp1, const Jstr& carg, Jstr &val );
	bool doAllOuterRing( const Jstr& mk1, const Jstr& hdr, const Jstr &colType1, 
						  int srid, const JagStrSplit &sp1, Jstr &val );
	bool doAllOuterRings( const Jstr& mk1, const Jstr& hdr, const Jstr &colType1, 
						  int srid, const JagStrSplit &sp1, Jstr &val );
	bool doAllInnerRings( const Jstr& mk1, const Jstr& hdr, const Jstr &colType1, 
						  int srid, const JagStrSplit &sp1, Jstr &val );
	bool doAllBuffer( const Jstr& mk1, const Jstr& hdr, const Jstr &colType1, 
						  int srid, const JagStrSplit &sp1, const Jstr& carg, Jstr &val );
	bool doAllCentroid( const Jstr& mk1, const Jstr& hdr, const Jstr &colType1, 
						  int srid, const JagStrSplit &sp1, Jstr &val );
	bool doAllMinMaxPoint( int op, const Jstr& mk1, const Jstr& hdr, const Jstr &colType1, 
						  int srid, const JagStrSplit &sp1, Jstr &val );
	bool doAllIsClosed( const Jstr& mk1, const Jstr &colType1, const JagStrSplit &sp1, Jstr &val );
	bool doAllIsSimple( const Jstr& mk1, const Jstr &colType1, const JagStrSplit &sp1, Jstr &val );
	bool doAllIsConvex( const Jstr& mk1, const Jstr &colType1, const JagStrSplit &sp1, Jstr &val );
	bool doAllInterpolate( int srid, const Jstr &colType, const JagStrSplit &sp, const Jstr &carg, Jstr &val );
	bool doAllLineSubstring( int srid, const Jstr &colType, const JagStrSplit &sp, const Jstr &carg, Jstr &val );
	bool doAllIsValid( const Jstr& mk1, const Jstr &colType1, const JagStrSplit &sp1, Jstr &val );
	bool doAllIsRing( const Jstr& mk1, const Jstr &colType1, const JagStrSplit &sp1, Jstr &val );
	bool doAllIsPolygonCCW( const Jstr& mk1, const Jstr &colType1, const JagStrSplit &sp1, Jstr &val );
	bool doAllIsPolygonCW( const Jstr& mk1, const Jstr &colType1, const JagStrSplit &sp1, Jstr &val );
	bool doAllNumPoints( const Jstr& mk1, const Jstr &colType1, const JagStrSplit &sp1, Jstr &val );
	bool doAllNumSegments( const Jstr& mk1, const Jstr &colType1, const JagStrSplit &sp1, Jstr &val );
	bool doAllNumLines( const Jstr& mk1, const Jstr &colType1, const JagStrSplit &sp1, Jstr &val );
	bool doAllNumRings( const Jstr& mk1, const Jstr &colType1, const JagStrSplit &sp1, Jstr &val );
	bool doAllNumInnerRings( const Jstr& mk1, const Jstr &colType1, const JagStrSplit &sp1, Jstr &val );
	bool doAllPolygonN( const Jstr& mk1, const Jstr& hdr, const Jstr &colType1, 
					   const JagStrSplit &sp1, const Jstr& carg, Jstr &val );
	bool doAllNumPolygons( const Jstr& mk1, const Jstr& hdr, const Jstr &colType1, 
					   const JagStrSplit &sp1, Jstr &val );
	bool doAllSummary( const Jstr& mk, const Jstr &colType, int srid, const JagStrSplit &sp, Jstr &val );
	bool doAllInnerRingN( const Jstr& mk1, const Jstr& hdr, const Jstr &colType1, 
						  int srid, const JagStrSplit &sp1, const Jstr& carg,  Jstr &val );
	bool doAllRingN( const Jstr& mk1, const Jstr& hdr, const Jstr &colType1, 
						  int srid, const JagStrSplit &sp1, const Jstr& carg,  Jstr &val );
	bool doAllMetricN( const Jstr& mk1, const Jstr& hdr, const Jstr &colType1, 
						  int srid, const JagStrSplit &sp1, const Jstr& carg,  Jstr &val );
	bool doAllGeoJson( const Jstr &mk1, const Jstr &colType1, 
						  int srid, JagStrSplit &sp1, const Jstr& carg,  Jstr &val );
	bool doAllWKT( const Jstr &mk1, const Jstr &colType1, 
						  int srid, JagStrSplit &sp1, const Jstr& carg,  Jstr &val );
	bool doAllMinimumBoundingCircle( const Jstr &mk1, const Jstr &colType1, 
						  int srid, JagStrSplit &sp1, const Jstr& carg,  Jstr &val );
	bool doAllMinimumBoundingSphere( const Jstr &mk1, const Jstr &colType1, 
						  int srid, JagStrSplit &sp1, const Jstr& carg,  Jstr &val );
	bool doAllUnique( const Jstr& mk1, const Jstr& hdr, const Jstr &colType1, 
					   const JagStrSplit &sp1, Jstr &val );
    Jstr doAllUnion( Jstr mark1, Jstr colType1,
				     int srid1, JagStrSplit &sp1, Jstr mark2,
				     Jstr colType2, int srid2, JagStrSplit &sp2 );
    Jstr doAllCollect( Jstr mark1, Jstr colType1,
		               int srid1, JagStrSplit &sp1, Jstr mark2,
		               Jstr colType2, int srid2, JagStrSplit &sp2 );
    Jstr doAllIntersection( int srid, Jstr mark1, Jstr colType1,
				               JagStrSplit &sp1, Jstr mark2,
				               Jstr colType2, JagStrSplit &sp2 );
    Jstr doAllDifference( int srid, Jstr mark1, Jstr colType1,
			               JagStrSplit &sp1, Jstr mark2,
			               Jstr colType2, JagStrSplit &sp2 );
    Jstr doAllSymDifference( int srid, Jstr mark1, Jstr colType1,
				             JagStrSplit &sp1, Jstr mark2,
				             Jstr colType2, JagStrSplit &sp2 );

    Jstr doAllLocatePoint( int srid, const Jstr& mark1, const Jstr &colType1,
				               const JagStrSplit &sp1, const Jstr& mark2,
				               const Jstr &colType2, const JagStrSplit &sp2 );

	Jstr doAllAddPoint( int srid, const Jstr& mark1, const Jstr &colType1,
	                    const JagStrSplit &sp1, const Jstr& mark2, const Jstr &colType2, const JagStrSplit &sp2, const Jstr &carg );
	Jstr doAllSetPoint( int srid, const Jstr& mark1, const Jstr &colType1,
	                    const JagStrSplit &sp1, const Jstr& mark2, const Jstr &colType2, const JagStrSplit &sp2, const Jstr &carg );
	
	bool doAllRemovePoint( int srid, const Jstr &colType1, const JagStrSplit &sp1, const Jstr &carg, Jstr &val );
	bool doAllReverse( int srid, const Jstr &colType1, const JagStrSplit &sp1, const Jstr &carg, Jstr &val );
	bool doAllScale( int srid, const Jstr &colType1, const JagStrSplit &sp1, const Jstr &carg, Jstr &val );
	bool doAllScaleSize( int srid, const Jstr &colType1, const JagStrSplit &sp1, const Jstr &carg, Jstr &val );
	Jstr doAllScaleAt( int srid, const Jstr& mark1, const Jstr &colType1,
	                    const JagStrSplit &sp1, const Jstr& mark2, const Jstr &colType2, const JagStrSplit &sp2, const Jstr &carg );
	bool doAllTranslate( int srid, const Jstr &colType1, const JagStrSplit &sp1, const Jstr &carg, Jstr &val );
	bool doAllTransScale( int srid, const Jstr &colType1, const JagStrSplit &sp1, const Jstr &carg, Jstr &val );
	bool doAllRotateAt( int srid, const Jstr &colType1, const JagStrSplit &sp1, const Jstr &carg, Jstr &val );
	bool doAllRotateSelf( int srid, const Jstr &colType1, const JagStrSplit &sp1, const Jstr &carg, Jstr &val );
	bool doAllAffine( int srid, const Jstr &colType1, const JagStrSplit &sp1, const Jstr &carg, Jstr &val );
	bool doAllDelaunayTriangles( int srid, const Jstr &colType1, const JagStrSplit &sp1, const Jstr &carg, Jstr &val );
	Jstr doAllVoronoiPolygons( int srid, const Jstr &colType1, const JagStrSplit &sp1, 
							   const Jstr &colType2, const JagStrSplit &sp2, const Jstr &carg );
	Jstr doAllVoronoiLines( int srid, const Jstr &colType1, const JagStrSplit &sp1, 
							const Jstr &colType2, const JagStrSplit &sp2, const Jstr &carg );
	Jstr doAllIsOnLeftSide( int srid, const Jstr &colType1, const JagStrSplit &sp1, 
							const Jstr &colType2, const JagStrSplit &sp2, const Jstr &carg );
	Jstr doAllIsOnRightSide( int srid, const Jstr &colType1, const JagStrSplit &sp1, 
							const Jstr &colType2, const JagStrSplit &sp2, const Jstr &carg );
	Jstr doAllLeftRatio( int srid, const Jstr &colType1, const JagStrSplit &sp1, 
							const Jstr &colType2, const JagStrSplit &sp2, const Jstr &carg );
	Jstr doAllRightRatio( int srid, const Jstr &colType1, const JagStrSplit &sp1, 
							const Jstr &colType2, const JagStrSplit &sp2, const Jstr &carg );

	Jstr doAllKNN( int srid, const Jstr &colType1, JagStrSplit &sp,
                   const Jstr &colType2, JagStrSplit &sp2, const Jstr &carg );


	static int getTypeMode( short fop );

	// data members
	JagFixString 	_opString;
	jagint 		_numCnts; // use for calcuations
	abaxdouble 		_initK; // use for stddev
	abaxdouble 		_stddevSum; //use for stddev
	abaxdouble 		_stddevSumSqr; // use for stddev
	std::regex      *_reg;

};

class BinaryExpressionBuilder
{
  public:
	BinaryExpressionBuilder();
	~BinaryExpressionBuilder();
	void init( const JagParseAttribute &jpa, JagParseParam *pparam );
	void clean();

	BinaryOpNode *parse( const JagParser *jagParser, const char* str, int type,
								const JagHashMap<AbaxString, AbaxPair<AbaxString, jagint>> &cmap, JagHashStrInt *&jmap, 
								Jstr &colList );
	ExprElementNode *getRoot() const;
	
	JagParseAttribute _jpa;
	JagParseParam 		*_pparam;
	bool 				doneClean;
	int  				_lastOp;
	// holds either (, +, -, *, /, %, ^ or any other function type  
	std::stack<int> operatorStack;	
	std::stack<int> operatorArgStack;	
	// operandStack is made up of BinaryOpNodes and StringElementNode
	JagExprStack operandStack; 

  private:

	int 		_datediffClause;
	int			_substrClause;
	bool 		_isNot;
	bool 		_lastIsOperand;
	bool        _isDestroyed;
	
	// methods
	void processBetween( const JagParser *jpars, const char *&p, const char *&q, StringElementNode &lastNode,
						const JagHashMap<AbaxString, AbaxPair<AbaxString, jagint>> &cmap, JagHashStrInt *&jmap, 
						Jstr &colList );
	void processIn( const JagParser *jpars, const char *&p, const char *&q, StringElementNode &lastNode,
					const JagHashMap<AbaxString, AbaxPair<AbaxString, jagint>> &cmap, JagHashStrInt *&jmap, 
					Jstr &colList );
	void processOperand( const JagParser *jpars, const char *&p, const char *&q, StringElementNode &lastNode,
						const JagHashMap<AbaxString, AbaxPair<AbaxString, jagint>> &cmap, Jstr &colList );

	//void processOperator( short op, int nargs, JagHashStrInt &jmap );
	void processOperator( short op, int nargs, JagHashStrInt *&jmap );
	//void processRightParenthesis( JagHashStrInt &jmap );
	void processRightParenthesis( JagHashStrInt *&jmap );
	void doAddBinary( short op, int nargs, JagHashStrInt *&jmap );
	short precedence( short fop );
	bool funcHasZeroChildren( short fop );
	bool funcHasOneConstant( short fop );
	bool funcHasTwoChildren( short fop );
	bool checkFuncType( short fop );
	bool getCalculationType( const char *p, short &fop, short &len, short &ctype );
	bool nameConvertionCheck( Jstr &name, int &tabnum,
								const JagHashMap<AbaxString, AbaxPair<AbaxString, jagint>> &cmap, 
								Jstr &colList );
    bool nameAndOpGood( const JagParser *jpsr, const Jstr &fullname, const StringElementNode &lastNode );
};

#endif
