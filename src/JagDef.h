/*
 * Copyright JaguarDB www.jaguardb.com
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
#ifndef _jag_def_h_
#define _jag_def_h_

#define JAG_VERSION			        "3.3.0"
#define JAG_BLOCK_SIZE				32
#define JAG_ARJAG_FILE_HEAD  		0
#define JAG_BID_FILE_HEADER_BYTES	64
#define JAG_REPLICATE_MAX			3
#define JAG_KEYCHECKER_KLEN		    16
#define JAG_KEYCHECKER_VLEN		    2
#define JAG_BYTE_MAX				255

#define JAG_HOTSPOT_MINLIMIT        0.3
#define JAG_HOTSPOT_LIMIT           0.7
#define JAG_UPDOWN_LIMIT            100

#define JAG_RANDOM_FILE_LIMIT		1000000

#define JAG_STRICT_ASCENDING        1
#define JAG_STRICT_RANDOM           2
#define JAG_CYCLE_RIGHT_SMALL       4
#define JAG_CYCLE_RIGHT_LARGE       5
#define JAG_STRICT_ASCENDING_LEFT   6


// Defs
#define ABAX_HASHARRNULL	LLONG_MIN
#ifdef _WINDOWS64_
#define JAG_NOATIME			0
#else
#ifdef _CYGWIN_
#define JAG_NOATIME			0
#else
#define JAG_NOATIME			O_NOATIME
#endif
#endif
#define JAG_LOG_LOW			1
#define JAG_LOG_MEDLOW		3
#define JAG_LOG_MED			5
#define JAG_LOG_MEDHIGH		7
#define JAG_LOG_HIGH		9

#define JAG_GETFILE_SIZE 	10
#define JAG_GETFILE_SIZEMB 	12
#define JAG_GETFILE_SIZEGB 	14
#define JAG_GETFILE_TIME 	20
#define JAG_GETFILE_TYPE 	22
#define JAG_GETFILE_MD5SUM 	30
#define JAG_GETFILE_FPATH 	40
#define JAG_GETFILE_HOST 	50
#define JAG_GETFILE_HOSTFPATH 	60
#define JAG_GETFILE_ACTDATA 0

// Parser Defs
#define JAG_NOOP		        0
#define JAG_CREATETABLE_OP		1
#define JAG_CREATEMEMTABLE_OP	2
#define JAG_CREATEINDEX_OP		3
#define JAG_CREATEDB_OP			4
#define JAG_USEDB_OP			5
#define JAG_CHANGEDB_OP			6
#define JAG_INSERT_OP			8
#define JAG_INSERTSELECT_OP		9
#define JAG_CINSERT_OP			10
#define JAG_FINSERT_OP			11
#define JAG_DINSERT_OP			12
#define JAG_IMPORT_OP			14
#define JAG_SELECT_OP			20
#define JAG_GETFILE_OP			21
#define JAG_INNERJOIN_OP		22
#define JAG_LEFTJOIN_OP			24
#define JAG_RIGHTJOIN_OP		26
#define JAG_FULLJOIN_OP			28
#define JAG_UPDATE_OP			30
#define JAG_DELETE_OP			50
#define JAG_COPY_OP				53
#define JAG_LOAD_OP				55
#define JAG_LOADB_OP			56
#define JAG_LOADR_OP			57
#define JAG_RAND_OP		    	58
#define JAG_DROPTABLE_OP		60
#define JAG_DROPINDEX_OP		65
#define JAG_TRUNCATE_OP			70
#define JAG_COUNT_OP			80
#define JAG_ALTER_OP			90
#define JAG_RENAME_TAB_OP		100
#define JAG_RENAME_DB_OP		102
#define JAG_DESCRIBE_OP			110
#define JAG_SHOWDB_OP			120
#define JAG_DROPDB_OP			122
#define JAG_SHOWTABLE_OP		130
#define JAG_SHOWCHAIN_OP		132
#define JAG_SHOWINDEX_OP		140
#define JAG_SHOWTASK_OP			150
#define JAG_HELP_OP				160
#define JAG_CURRENTDB_OP		162
#define JAG_SHOWSTATUS_OP		163
#define JAG_SHOWSVER_OP			164
#define JAG_SHOWCVER_OP			166
#define JAG_CURRENTUSER_OP		168
#define JAG_DEBUGCHECK_OP		170
#define JAG_REPAIRCHECK_OP		172
#define JAG_CREATEUSER_OP		178
#define	JAG_DROPUSER_OP			180
#define	JAG_CHANGEPASS_OP		182
#define	JAG_SHOWUSER_OP			184
#define	JAG_SHOW_CREATE_TABLE_OP	186
#define JAG_CREATECHAIN_OP	    	188
#define	JAG_SHOW_CREATE_CHAIN_OP	190
#define JAG_SHOWDATACENTER_OP		192
#define JAG_SHOWTOOLS_OP		194
#define JAG_GRANT_OP		    196
#define JAG_REVOKE_OP		    198
#define JAG_SHOWGRANT_OP		200
#define JAG_CLILOG_OP		    202
#define JAG_ADDCLUSTER_OP		204
#define JAG_SHOWCLUSTER_OP		206

// table chain memtable type
#define JAG_ANY_TYPE            0
#define JAG_MEMTABLE_TYPE       10
#define JAG_CHAINTABLE_TYPE     20
#define JAG_TABLE_TYPE          30
#define JAG_TABLE_OR_CHAIN_TYPE  40

// exec cmd Defs
#define JAG_EXEC_DESC_OP		500
#define JAG_EXEC_SHOWTABLE_OP	510
#define JAG_EXEC_SHOWINDEX_OP	520
#define JAG_EXEC_SHOWDB_OP		530
#define JAG_EXEC_KILL_OP		540
#define JAG_EXEC_CHECK_OP		550
#define JAG_EXEC_CHECKALL_OP	560
#define JAG_EXEC_PKEY_OP		584
#define JAG_EXEC_PUBKEY_OP		586

// special commands server accept for non SQL standard
#define JAG_SCMD_NOOP					600
#define JAG_SCMD_CSCHEMA				602
#define JAG_SCMD_CHOST					603
#define JAG_SCMD_CRECOVER				604
#define JAG_SCMD_CHECKDELTA				605
#define JAG_SCMD_BFILETRANSFER			606
#define JAG_SCMD_ABFILETRANSFER			608
#define JAG_SCMD_JOINDATAREQUEST		609
#define JAG_SCMD_OPINFO					610
#define JAG_SCMD_COPYDATA				612
#define JAG_SCMD_DOLOCALBACKUP			614
#define JAG_SCMD_DOREMOTEBACKUP			616
#define JAG_SCMD_DORESTOREREMOTE		618
#define JAG_SCMD_MONDBTAB				620
#define JAG_SCMD_MONINFO				622
#define JAG_SCMD_MONRSINFO				624
#define JAG_SCMD_MONCLUSTERINFO			626
#define JAG_SCMD_MONHOSTS				628
#define JAG_SCMD_MONBACKUPHOSTS			630
#define JAG_SCMD_MONLOCALSTAT			632
#define JAG_SCMD_MONCLUSTERSTAT			634
#define JAG_SCMD_EXPROCLOCALBACKUP		636
#define JAG_SCMD_EXPROCREMOTEBACKUP		638
#define JAG_SCMD_EXRESTOREFROMREMOTE	640
#define JAG_SCMD_EXREPAIRCHECK			642
#define JAG_SCMD_EXREPAIROBJECT			644
#define JAG_SCMD_EXADDCLUSTER			645
#define JAG_SCMD_EXADDCLUSTER_MIGRATE	646
#define JAG_SCMD_EXADDCLUSTER_MIGRATE_CONTINUE	647
#define JAG_SCMD_EXADDCLUSTER_MIGRATE_COMPLETE	648
#define JAG_SCMD_EXSHUTDOWN				649
#define JAG_SCMD_REFRESHACL				650
#define JAG_SCMD_GETPUBKEY				660
#define JAG_SCMD_REQSCHEMAFROMDC		670
#define JAG_SCMD_UNPACKSCHINFO			680
#define JAG_SCMD_ASKDATAFROMDC			685
#define JAG_SCMD_PREPAREDATAFROMDC		690
#define JAG_SCMD_CDEFVAL				692
#define JAG_SCMD_IMPORTTABLE			694
#define JAG_SCMD_TRUNCATETABLE			696

#define JAG_SCMD_CSCHEMA_MORE			700
#define JAG_SCMD_CHKKEY			        702
#define JAG_SCMD_RECVFILE		        704

#define JAG_RCMD_HELP					800
#define JAG_RCMD_USE					802
#define JAG_RCMD_AUTH					804
#define JAG_RCMD_QUIT					806
#define JAG_RCMD_HELLO					808

// schema and actual data Defs
// cannot be changed
#define JAG_C_COL_TYPE_DT_NONE		"z"
#define JAG_C_COL_TYPE_DATE			"r"

#define JAG_C_COL_TYPE_TIMEMICRO	"h"  // microseconds

#define JAG_C_COL_TYPE_TIMENANO		"H"

#define JAG_C_COL_TYPE_TIMESTAMPSEC	"C"  // seconds
#define JAG_C_COL_TYPE_DATETIMESEC	"c"  // seconds

#define JAG_C_COL_TYPE_TIMESTAMPMILLI    "L"  // milliseconds
#define JAG_C_COL_TYPE_DATETIMEMILLI	 "l"  // milliseconds

#define JAG_C_COL_TYPE_DATETIMEMICRO	"T"  // microseconds
#define JAG_C_COL_TYPE_TIMESTAMPMICRO	"t"  // microseconds

#define JAG_C_COL_TYPE_TIMESTAMPNANO  "N" // nanoseconds
#define JAG_C_COL_TYPE_DATETIMENANO	  "n"  // nanoseconds

#define JAG_C_COL_TYPE_STR			"s"
#define JAG_C_COL_TYPE_FLOAT		"f"
#define JAG_C_COL_TYPE_DOUBLE		"d"
#define JAG_C_COL_TYPE_LONGDOUBLE	"D"
#define JAG_C_COL_TYPE_UUID			"u"
#define JAG_C_COL_TYPE_FILE			"F"
#define JAG_C_COL_TYPE_ENUM			"e"
#define JAG_C_COL_TYPE_DBOOLEAN		"E"
#define JAG_C_COL_TYPE_DBIT			"b"
#define JAG_C_COL_TYPE_DBIGINT		"B"
#define JAG_C_COL_TYPE_DINT			"I"
#define JAG_C_COL_TYPE_DSMALLINT	"i"
#define JAG_C_COL_TYPE_DTINYINT		"j"
#define JAG_C_COL_TYPE_DMEDINT		"m"

// geometry types
// saved in schema  -- immutable
#define JAG_DEFAULT_SRID            0
#define JAG_C_COL_TYPE_DATA		    "DT"
#define JAG_C_COL_TYPE_POINT		"PT"
#define JAG_C_COL_TYPE_POINT3D		"PT3"
#define JAG_C_COL_TYPE_MULTIPOINT	"MP"
#define JAG_C_COL_TYPE_MULTIPOINT3D	"MP3"
#define JAG_C_COL_TYPE_LINE		    "LN"
#define JAG_C_COL_TYPE_LINE3D	    "LN3"
#define JAG_C_COL_TYPE_LINESTRING   "LS"
#define JAG_C_COL_TYPE_VECTOR       "VC"
#define JAG_C_COL_TYPE_LINESTRING3D  "LS3"
#define JAG_C_COL_TYPE_MULTILINESTRING    "ML"
#define JAG_C_COL_TYPE_MULTILINESTRING3D  "ML3"
#define JAG_C_COL_TYPE_POLYGON		"PL"
#define JAG_C_COL_TYPE_POLYGON3D	"PL3"
#define JAG_C_COL_TYPE_MULTIPOLYGON	"MG"
#define JAG_C_COL_TYPE_MULTIPOLYGON3D	"MG3"
#define JAG_C_COL_TYPE_CIRCLE	    "CR"
#define JAG_C_COL_TYPE_CIRCLE3D	    "CR3"
#define JAG_C_COL_TYPE_SPHERE	    "SR"
#define JAG_C_COL_TYPE_SQUARE	    "SQ"
#define JAG_C_COL_TYPE_SQUARE3D	    "SQ3"
#define JAG_C_COL_TYPE_CUBE	        "CB"
#define JAG_C_COL_TYPE_RECTANGLE    "RC"
#define JAG_C_COL_TYPE_RECTANGLE3D  "RC3"
#define JAG_C_COL_TYPE_BOX          "BX"
#define JAG_C_COL_TYPE_TRIANGLE     "TR"
#define JAG_C_COL_TYPE_TRIANGLE3D   "TR3"
#define JAG_C_COL_TYPE_CYLINDER     "CL"
#define JAG_C_COL_TYPE_CONE         "CN"
#define JAG_C_COL_TYPE_ELLIPSE      "EL"
#define JAG_C_COL_TYPE_ELLIPSE3D    "EL3"
#define JAG_C_COL_TYPE_ELLIPSOID    "ES"
#define JAG_C_COL_TYPE_BBOX         "BB"


// temp column types
#define JAG_C_COL_TEMPTYPE_STRING		"31"
#define JAG_C_COL_TEMPTYPE_REAL			"33"
#define JAG_C_COL_TEMPTYPE_TINYTEXT		"34"
#define JAG_C_COL_TEMPTYPE_TEXT		    "35"
#define JAG_C_COL_TEMPTYPE_MEDIUMTEXT	"37"
#define JAG_C_COL_TEMPTYPE_LONGTEXT	    "39"

#define JAG_C_COL_KEY		'k'
#define JAG_C_COL_VALUE	    'v'
#define JAG_C_NEG_SIGN		'-'
#define JAG_C_POS_SIGN		'0'

// do not change unless in genesis/recompile
#define JAG_TIMEMICRO_FIELD_LEN			5  // microseconds [0-86400]x10^6

#define JAG_TIMENANO_FIELD_LEN		    6

#define JAG_DATETIMESEC_FIELD_LEN	    5

#define JAG_DATETIMEMICRO_FIELD_LEN		8  // microsecond
#define JAG_TIMESTAMPMICRO_FIELD_LEN	8  // microsecond

#define JAG_TIMESTAMPMILLI_FIELD_LEN    6
#define JAG_DATETIMEMILLI_FIELD_LEN	    6

#define JAG_DATETIMENANO_FIELD_LEN	    8
#define JAG_TIMESTAMPNANO_FIELD_LEN	    8

#define JAG_DATE_FIELD_LEN		        4

#define JAG_GEOID_FIELD_LEN		        10
#define JAG_UUID_FIELD_LEN		        32
#define JAG_FILE_FIELD_LEN		        64
#define JAG_REAL_FIELD_LEN              40
#define JAG_TINYTEXT_FIELD_LEN          256
#define JAG_TEXT_FIELD_LEN              1024
#define JAG_MEDIUMTEXT_FIELD_LEN        2048
#define JAG_LONGTEXT_FIELD_LEN          10240
#define JAG_STRING_FIELD_LEN          	64
#define JAG_CTIME_LEN					25
#define JAG_DBOOLEAN_FIELD_LEN			1
#define JAG_DBIT_FIELD_LEN			    1
#define JAG_S_COL_SPARE_DEFAULT         ' '

// changing field lengths requires cleaning up data and recompiling 
#define JAG_DBIGINT_FIELD_LEN		9  // 1 + 8(10^19) LONG_MAX is 9,223,372,036,854,775,807
#define JAG_DINT_FIELD_LEN			6  // 1 + 5(10^12) 1(+/-) + max 999,999,999,999
#define JAG_DMEDINT_FIELD_LEN		4  // 1 + 3(10^7)  1(+/-) + max 9,999,999
#define JAG_DSMALLINT_FIELD_LEN		3  // 1 + 2(10^4)  1(+/-) + max 9999
#define JAG_DTINYINT_FIELD_LEN		2  // 1 + 1(10^2)  1(+/-) + max 99

#define JAG_FLOAT_FIELD_LEN		    10 //                10^9                   10^7
#define JAG_FLOAT_SIG_LEN		    3  // 10 = 1(sign) + 4(n) + 1(dot) + 1(w) + 3(n) 

#define JAG_DOUBLE_FIELD_LEN		11 //               10^13                   10^7
#define JAG_DOUBLE_SIG_LEN		    3  // 13 = 1(sign) + 5(n) + 1(dot) + 1(w) + 3(n) 

#define JAG_LONGDOUBLE_FIELD_LEN	15  //                10^19                  10^9
#define JAG_LONGDOUBLE_SIG_LEN		4   // 15 = 1(sign) + 8(n) + 1(dot) + 1(w) + 4(n)

// Key mode
#define JAG_ASC				'a'
#define JAG_DESC			'd'
#define JAG_RAND			'r'

#define JAG_CREATE_DEFINSERTVALUE	'I'
#define JAG_CREATE_DEFDATE			'D'
#define JAG_CREATE_DEFDATETIMESEC	'S'
#define JAG_CREATE_DEFDATETIME		'T'
#define JAG_CREATE_DEFDATETIMENANO	't'
#define JAG_CREATE_DEFDATETIMEMILL	'm'

#define JAG_CREATE_UPDATE_DATE		    'F'  
#define JAG_CREATE_UPDATE_DATETIMESEC	's' 
#define JAG_CREATE_UPDATE_DATETIME		'P' 
#define JAG_CREATE_UPDATE_DATETIMEMILL	'L' 
#define JAG_CREATE_UPDATE_DATETIMENANO	'G' 

#define JAG_CREATE_DEFUPDATE_DATE	        'u' 
#define JAG_CREATE_DEFUPDATE_DATETIMESEC	'n' 
#define JAG_CREATE_DEFUPDATE_DATETIME	    'U'   
#define JAG_CREATE_DEFUPDATE_DATETIMEMILL	'm' 
#define JAG_CREATE_DEFUPDATE_DATETIMENANO	'N' 

#define JAG_FLOATDEF				10
#define JAG_FLOATDEFSIG				2
#define JAG_DOUBLEDEF				16
#define JAG_DOUBLEDEFSIG			4
#define JAG_LONGDOUBLEDEF			20
#define JAG_LONGDOUBLEDEFSIG		5

#define JAG_KEY_HASH			    ' '
#define JAG_KEY_MUTE			    'M'
#define JAG_SUB_COL			        's'
#define JAG_ROLL_UP			        'r'

#define JAG_ROLLUP_AVG		        'a'
#define JAG_ROLLUP_SUM		        's'
#define JAG_ROLLUP_MIN		        'm'
#define JAG_ROLLUP_MAX		        'M'
#define JAG_ROLLUP_COUNT 	        'c'

// Operation Defs
// table function default defs
#define JAG_FUNC_EMPTYARG_LEN		32
#define JAG_FUNC_CURDATE_LEN		10
#define JAG_FUNC_CURTIME_LEN		8
#define JAG_FUNC_NOW_LEN			19
#define JAG_FUNC_NOW_LEN_MILL		23
#define JAG_FUNC_NOW_LEN_MICRO		26
#define JAG_FUNC_NOW_LEN_NANO		29

// logic operators
#define JAG_LOGIC_AND			'A'
#define JAG_LOGIC_OR			'O'

// calculation operators
// can be changed
#define JAG_NUM_ADD			'+'
#define JAG_NUM_SUB			'-'
#define JAG_NUM_MULT		'*'
#define JAG_NUM_DIV			'/'
#define JAG_NUM_REM			'%'
#define JAG_NUM_POW			'^'
#define JAG_STR_ADD			'.'

// SQL functions -- be careful, do not use code 40 -- reserved for '(' character
#define JAG_LEFT_BRA			40 // '(' letter
#define JAG_FUNC_NOOP			0 // global no operation

// JAG_FUNC_* can be changed
// one-line data, cannot combine with multi-line data ( aggregate functions )
#define JAG_FUNC_MIN			10 // special type, can hold both string and numeric
#define JAG_FUNC_MAX			20 // special type, can hold both string and numeric
#define JAG_FUNC_AVG			30
#define JAG_FUNC_SUM			50
#define JAG_FUNC_COUNT			52
#define JAG_FUNC_STDDEV			60

// one-line data, can combine with multi-line data ( non-aggregate functions )
#define JAG_FUNC_FIRST			70 // special type, can hold both string and numeric
#define JAG_FUNC_LAST			80 // special type, can hold both string and numeric

// multi-line data ( numeric functions )
#define JAG_FUNC_ABS			100 // abs(v) 
#define JAG_FUNC_ACOS			102
#define JAG_FUNC_ASIN			104
#define JAG_FUNC_ATAN			106 // atan(v1, v2)
#define JAG_FUNC_CEIL			108 // smallest int value
#define JAG_FUNC_COS			110 // for radians
#define JAG_FUNC_COT			112
#define JAG_FUNC_FLOOR			114
#define JAG_FUNC_LOG2			116
#define JAG_FUNC_LOG10			118
#define JAG_FUNC_LOG			120
#define JAG_FUNC_MOD			122
#define JAG_FUNC_POW			124
#define JAG_FUNC_SIN			126 // for radians
#define JAG_FUNC_SQRT			128
#define JAG_FUNC_TAN			130 // for radians
#define JAG_FUNC_DEGREES		132 // radians to degrees
#define JAG_FUNC_RADIANS		134 // degrees to radians
#define JAG_FUNC_EXP			136 // exp(a) is a raised to the power of e. inverse of ln()
#define JAG_FUNC_ALL			138 // all(a) all row data for poly data
#define JAG_FUNC_STRDIFF		140 // diff( str1, str2)  // levinshtein distance between two strs or (col, str)

#define JAG_FUNC_DISTANCE		200  // double
#define JAG_FUNC_WITHIN		    202  // T or F
#define JAG_FUNC_CONTAIN	    204  // T or F
#define JAG_FUNC_INTERSECT	    206  // T or F
#define JAG_FUNC_OVERLAP	    208  // T or F
#define JAG_FUNC_EXTENT         210  // get bbox of shape  type is BB, xmin ymin zmin xmax ymax zmax
#define JAG_FUNC_KM2MILE	    220  // km2mile(km)
#define JAG_FUNC_MILE2KM 	    222  // mile2km(miles)
#define JAG_FUNC_COVER 	        224  // 1 or 0
#define JAG_FUNC_COVEREDBY 	    226  // 1 or 0
#define JAG_FUNC_DISJOINT 	    228  // 1 or 0
#define JAG_FUNC_NEARBY 	    230  // 1 or 0
#define JAG_FUNC_AREA 	        232  // area of 2D closed shape
#define JAG_FUNC_DIMENSION 	    233  // dimension of column 2 or 3
#define JAG_FUNC_GEOTYPE 	    234  // geotype  CIRCLE SQUARE LINE LINEPOINT POLYGON etc
#define JAG_FUNC_POINTN 	    235  // n-th point of linestring/polygon  x y z
#define JAG_FUNC_STARTPOINT 	237  // first point of line/linestring
#define JAG_FUNC_ENDPOINT 	    238  // end point of line/linestring
#define JAG_FUNC_CONVEXHULL     239  // convex hull of linestring/polygon
#define JAG_FUNC_ISCLOSED       240  // if linestring/polygon is closed shape
#define JAG_FUNC_NUMPOINTS      241  // total number of points of line/linestring/multilinestring/polygon/mpolygon
#define JAG_FUNC_SRID           242  // srid of a column
#define JAG_FUNC_SUMMARY        243  // summary of a column: type, dim, srid, size, 
#define JAG_FUNC_X              244  // x coord
#define JAG_FUNC_Y              245  // Y coord
#define JAG_FUNC_Z              246  // Z coord
#define JAG_FUNC_XMIN           247  // XMIN of bbox
#define JAG_FUNC_YMIN           248  // YMIN of bbox
#define JAG_FUNC_ZMIN           249  // YMIN of bbox
#define JAG_FUNC_XMAX           250  // XMAX of bbox
#define JAG_FUNC_YMAX           251  // YMAX of bbox
#define JAG_FUNC_ZMAX           252  // ZMAX of bbox
#define JAG_FUNC_NUMRINGS       253  // total number of rings in polygon/multipolygon
#define JAG_FUNC_CENTROID       254  // center of mass of a shape
#define JAG_FUNC_VOLUME         255  // volume of a 3D shape
#define JAG_FUNC_CLOSESTPOINT   256  // closest point on a shape from a point
#define JAG_FUNC_ANGLE          257  // angle (0-359) between line segments
#define JAG_FUNC_BUFFER         258  // buffer (polygon) of points/multipoints/line/linestring/polygon
#define JAG_FUNC_PERIMETER		259  // length of polygons and multipolygons, squares, rectangles, etc
#define JAG_FUNC_SAME		    260  // is two shapes are equal
#define JAG_FUNC_NUMSEGMENTS    261  // total number of segments in line, linestring/3d, multilinestring/3d
#define JAG_FUNC_ISSIMPLE       262  // if linestring/polygon has no self-intersecting or tangent lines 
#define JAG_FUNC_ISVALID        263  // if linestring/polygon is valid
#define JAG_FUNC_ISRING         264  // if linestring is simple and closed
#define JAG_FUNC_ISPOLYGONCCW   265  // if polygon is counter-clock-wise (outer-ring)
#define JAG_FUNC_ISPOLYGONCW    266  // if polygon is clock-wise (outer-ring)
#define JAG_FUNC_OUTERRING      267  // outer-ring as linestring of a polygon "hdr(LS) [bbox] x:y ..."
#define JAG_FUNC_OUTERRINGS     268  // outer-rings as multi-linestring of a multipolygon "hdr(ML) [bbox] x:y ..."
#define JAG_FUNC_INNERRINGS     269  // inner-rings as multi-linestring of a polygon/multipolygon "hdr(ML) [bbox] x:y ..."
#define JAG_FUNC_NUMINNERRINGS  270  // number of inner-rings of a polygon/multipolygon 
#define JAG_FUNC_POLYGONN       271  // n-th polygon (starting from 1) of multipolygon
#define JAG_FUNC_RINGN          272  // n-th ring as linestring of polygon
#define JAG_FUNC_INNERRINGN     273  // n-th inner ring as linestring of polygon
#define JAG_FUNC_NUMPOLYGONS    274  // total number of polygons in a multipolygon
#define JAG_FUNC_UNIQUE         275  // geom with consecutive duplicate points removed
#define JAG_FUNC_UNION          276  // union of two polygons to forma a new polygon
#define JAG_FUNC_COLLECT        277  // collection (addition) of two geoms to forma a new geom
#define JAG_FUNC_INTERSECTION   278  // intersection (geo) of two geoms
#define JAG_FUNC_TOPOLYGON      279  // convert vector 2D shapes to 2d polygon
#define JAG_FUNC_ASTEXT         280  // sent to client as text of geom
#define JAG_FUNC_DIFFERENCE     281  // diffeence of two geoms  g1 - (g1 common g2)
#define JAG_FUNC_SYMDIFFERENCE  282  // g1+g2 - (g1 common g2)
#define JAG_FUNC_ISCONVEX       283  // if outer ring of polygon is convex
#define JAG_FUNC_INTERPOLATE    284  // find point on a linestring by fraction
#define JAG_FUNC_LINESUBSTRING  285  // linesubstring(lstr, 0.1, 0.8) substring between two end fraction points
#define JAG_FUNC_LOCATEPOINT    286  // fraction number where on linestring a point is closest to a given point
#define JAG_FUNC_ADDPOINT       300  // add a point to a linestring
#define JAG_FUNC_SETPOINT       301  // set n-th point on a linestring
#define JAG_FUNC_REMOVEPOINT    302  // remove a point from a linestring
#define JAG_FUNC_REVERSE        303  // reverse points of line, linestring, polygon, multipolygon
#define JAG_FUNC_SCALE          304  // scale the coordinate values by factors
#define JAG_FUNC_SCALEAT        305  // scale the coordinate values, relative to a point, by factors
#define JAG_FUNC_SCALESIZE      306  // scale the dimension of vector shapes
#define JAG_FUNC_TRANSLATE      307  // translate the location of geom
#define JAG_FUNC_TRANSSCALE     308  // translate and then scale
#define JAG_FUNC_ROTATE         310  // rotate around (0, 0)
#define JAG_FUNC_ROTATEAT       311  // rotate around (x0, y0)
#define JAG_FUNC_ROTATESELF     312  // rotate around self-center
#define JAG_FUNC_AFFINE         313  // affine op on 3D or 2D coordinates
#define JAG_FUNC_VORONOIPOLYGONS    314  // affine op on 3D or 2D coordinates
#define JAG_FUNC_VORONOILINES   315  // affine op on 3D or 2D coordinates
#define JAG_FUNC_NUMLINES       316  // total number of line strings in multilinestring
#define JAG_FUNC_DELAUNAYTRIANGLES       317  // Delaunay triangles 
#define JAG_FUNC_GEOJSON        318  // make GeoJSon string from a shape
#define JAG_FUNC_TOMULTIPOINT   319  // convert geom (vector and others) to multipoints
#define JAG_FUNC_WKT            320  // output WKT text for point, mpoint, linestring, multilinestring, polygon, multipolygon
#define JAG_FUNC_MINIMUMBOUNDINGCIRCLE 321  // minimum bounding circle of 2D geom
#define JAG_FUNC_MINIMUMBOUNDINGSPHERE 322  // minimum bounding sphere of 3D geom
#define JAG_FUNC_ISONLEFT 		   323  // 2D point, line, line on left side
#define JAG_FUNC_ISONRIGHT 		   324  // 2D point, line, line on right side
#define JAG_FUNC_LEFTRATIO 		   325  // 2D how much percentage geom1 is on the left of geom2
#define JAG_FUNC_RIGHTRATIO 	   326  // 2D how much percentage geom1 is on the right of geom2
#define JAG_FUNC_CONCAVEHULL       327  // concave hull of multipoint/linestring/polygon
#define JAG_FUNC_KNN       		   328  // k-NN K nearest neighbor of 2D and 3D point to multipoint/linestring/polygon
#define JAG_FUNC_METRICN   		   329  // metricN(geom, N, m ) metric of N-th point, m-th metric, 1-based. or metricN(g,N)
#define JAG_FUNC_XMINPOINT   	   330  // xminpoint(geom) return point where x-coord is minimum on geom
#define JAG_FUNC_XMAXPOINT   	   331  // xmaxpoint(geom) return point where x-coord is maximum on geom
#define JAG_FUNC_YMINPOINT   	   332  // yminpoint(geom) return point where y-coord is minimum on geom
#define JAG_FUNC_YMAXPOINT   	   333  // ymaxpoint(geom) return point where y-coord is maximum on geom
#define JAG_FUNC_ZMINPOINT   	   334  // zminpoint(geom) return point where z-coord is minimum on geom
#define JAG_FUNC_ZMAXPOINT   	   335  // zmaxpoint(geom) return point where z-coord is maximum on geom
#define JAG_FUNC_SIMILARITY		   337  // similarity between two vectors


// string functions 
// mutable
#define JAG_FUNC_SUBSTR			390  // substr(str, offset, len)  substr(str, offset )
#define JAG_FUNC_UPPER			391
#define JAG_FUNC_LOWER			392
#define JAG_FUNC_LTRIM			393
#define JAG_FUNC_RTRIM			394
#define JAG_FUNC_TRIM			395
#define JAG_FUNC_LENGTH			396
#define JAG_FUNC_LINELENGTH		397

// date and time
// mutable
#define JAG_FUNC_SECOND			400 // 0-59
#define JAG_FUNC_MINUTE			402 // 0-59
#define JAG_FUNC_HOUR			404 // 0-23
#define JAG_FUNC_DAY			406 // 1-31
#define JAG_FUNC_MONTH			408 // 1-12
#define JAG_FUNC_YEAR			410 // yyyy
#define JAG_FUNC_DATE			412 // "yyyy-mm-dd"
#define JAG_FUNC_DATEDIFF		422 // expr1-expr2 number of days diff
#define JAG_FUNC_DAYOFMONTH		424 // 1-31; 0 for empty
#define JAG_FUNC_DAYOFWEEK		426 // 0: sunday  6: saturday
#define JAG_FUNC_DAYOFYEAR		428 // 1-366
#define JAG_FUNC_CURDATE		430 // 2020-01-02 current date
#define JAG_FUNC_CURTIME		432 // hh:mm:ss
#define JAG_FUNC_NOW			434 // "yyyy-mm-dd hh:mm:ss"
#define JAG_FUNC_TIME			436 // "12393949499"  seconds since epoch
#define JAG_FUNC_TOSECOND		438 
#define JAG_FUNC_TOMICROSECOND	440 

#define JAG_FUNC_MILETOMETER	442 
#define JAG_FUNC_METERTOMILE	446 
#define JAG_FUNC_KILOMETERTOMILE	448 
#define JAG_FUNC_MILETOKILOMETER	450 


#define JAG_FUNC_PI				800 

// = != < <= > >= like calculation
#define JAG_FUNC_EQUAL			900 // =
#define JAG_FUNC_NOTEQUAL		902 // != <> ><
#define JAG_FUNC_LESSTHAN		904 // <
#define JAG_FUNC_LESSEQUAL		906 // <=
#define JAG_FUNC_GREATERTHAN	908 // >
#define JAG_FUNC_GREATEREQUAL	910 // >=
#define JAG_FUNC_LIKE			912 // like %
#define JAG_FUNC_MATCH			914 // like %

// client side connect options
#define JAG_CLI_CHILD			1
#define JAG_SERV_PARENT			2
#define JAG_SERV_CHILD			4
#define JAG_CONNECT_ONE			8
#define JAG_CHILD			(JAG_CLI_CHILD|JAG_SERV_CHILD)
#define JAG_SERV			(JAG_SERV_PARENT|JAG_SERV_CHILD)

// limit defs
// max number of columns a table can have
// mutable
#define JAG_COL_MAX			4096

#define JAG_ONEFILESIGNAL	7
#define JAG_MEGABYTEUNIT	1048576

// max length of each column name in bytes
// mutable
#define JAG_COLNAME_LENGTH_MAX 32

#define JAG_SESSIONID_MAX		32
#define JAG_SOCK_COMPRSS_MIN	540 
#define SELECT_DATA_REQUEST_LEN	70
#define JAG_ERR_MSG_LEN			256

// client other defs
// mutable
#define JAG_ORDERBY_READFROM_JDA	   1
#define JAG_ORDERBY_READFROM_MEMARR    2
#define JAG_ORDERBY_READFROM_DISKARR   3


// API macro defs
#define raypread(a, b, c, d)		jdfpread( _jdfs, b, c, d)
#define raypwrite(a, b, c, d)		jdfpwrite( _jdfs, b, c, d)

// threadid
#define THID			    pthread_self()

// conf file type
#define JAG_SERVER			10
#define JAG_CLIENT			20


// schema kvlen
// not mutable
#define JAG_SCHEMA_KEYLEN    		128
#define JAG_SCHEMA_VALLEN    		10240
#define JAG_SCHEMA_SPARE_LEN 	    32

#define JAG_MAX_DBNAME          64


// client setEnd meaning
#define JAG_END_BEGIN                  0
#define JAG_END_NORMAL                 1
#define JAG_END_RECVONE_THEN_DONE      2
#define JAG_END_RECVONE_THEN_DONE_ERR  3
#define JAG_END_OK_BUT_STOP            4

#define JAG_END_NOQUERY_BUT_REPLY      5
#define JAG_END_GOT_DBPAIR             6
#define JAG_END_GOT_DBPAIR_AND_ORDERBY    7
#define JAG_END_SENDQUERY_LOADLOCK        8

// export types
#define JAG_EXPORT_SQL     1
#define JAG_EXPORT         2
#define JAG_EXPORT_CSV     3

// group by type
#define JAG_GROUPBY_NONE    0
#define JAG_GROUPBY_FIRST   1
#define JAG_GROUPBY_LAST    2
#define JAG_GROUPBY_DEBUGCHECK   3

#define ONE_GIGA_BYTES  1073741824
#define ONE_MEGA_BYTES  1046576
#define ONE_HALF_MEGA_BYTES  524288
#define NBT  '\0'
#define CLIENT_SOCKET_BUFFER_BYTES  (2*ONE_HALF_MEGA_BYTES) 
#define SERVER_SOCKET_BUFFER_BYTES  (3*ONE_HALF_MEGA_BYTES) 

// Windows event signal
#define JAG_CTRL_HUP   10
#define JAG_CTRL_CLOSE 20

// sleep mode: 0 - seconds; 1 - milliseconds; 2 - microseconds
#define JAG_SEC 	0
#define JAG_MSEC	1
#define JAG_USEC	2

// client send batch records
// mutable
#define CLIENT_SEND_BATCH_RECORDS  10000

// MEMORY_MODE values
#define JAG_MEM_LOW  100
#define JAG_MEM_HIGH 200

// max number of data centers
#define JAG_DATACENTER_MAX  1024

// type of a datacenter a gate or db host
#define JAG_DATACENTER_HOST  10
#define JAG_DATACENTER_GATE  20
#define JAG_DATACENTER_PGATE  22

// userID and password length limit
#define JAG_USERID_LEN  32
#define JAG_PASSWD_LEN  32


// user permissons
// not mutable
#define JAG_ROLE_SELECT  "S"
#define JAG_ROLE_INSERT  "I"
#define JAG_ROLE_UPDATE  "U"
#define JAG_ROLE_DELETE  "D"
#define JAG_ROLE_CREATE  "C"
#define JAG_ROLE_DROP    "R"
#define JAG_ROLE_ALTER   "A"
#define JAG_ROLE_TRUNCATE  "T"
#define JAG_ROLE_ALL     "*"

// point max len buffer size
#define JAG_POINT_LEN  20

// geometry point in decimal 
//#define JAG_GEOM_TOTLEN     20
#define JAG_GEOM_TOTLEN     15
#define JAG_GEOM_PRECISION  5

// geo type srid
#define JAG_GEO_WGS84     4326

#define JAG_POINT_DIM 		2   // xy
#define JAG_POINT3D_DIM 	3   // xyz 
#define JAG_MULTIPOINT_DIM 	2   // xy
#define JAG_MULTIPOINT3D_DIM 	3   // xyz 
#define JAG_CIRCLE_DIM      3   // x y a
#define JAG_VECTOR_DIM 	    1   // x
#define JAG_LINESTRING_DIM 	2   // x y
#define JAG_LINESTRING3D_DIM 	3   // x y z
#define JAG_MULTILINESTRING_DIM 	2   // x y
#define JAG_MULTILINESTRING3D_DIM 	3   // x y z
#define JAG_POLYGON_DIM 	2   // x y
#define JAG_POLYGON3D_DIM 	3   // x y z
#define JAG_MULTIPOLYGON_DIM 	2   // x y
#define JAG_MULTIPOLYGON3D_DIM 	3   // x y z
#define JAG_LINE_DIM 		4   // xy  xy
#define JAG_SPHERE_DIM	    4   // x y z a 
#define JAG_SQUARE_DIM	    4   // x y a nx
#define JAG_RECTANGLE_DIM   5   // x y a b nx
#define JAG_ELLIPSE_DIM     5   // x y a b nx 
#define JAG_LINE3D_DIM 		6   // xyz xyz
#define JAG_TRIANGLE_DIM	6   // x1y1 x2y2 x3y3
#define JAG_SQUARE3D_DIM	6   // x y z a nx ny
#define JAG_CUBE_DIM	    6   // x y z a nx ny
#define JAG_CIRCLE3D_DIM    6   // x y z a nx ny
#define JAG_RECTANGLE3D_DIM 7   // x y z  a b nx ny
#define JAG_CONE_DIM        7   // x y z  a b nx ny
#define JAG_ELLIPSE3D_DIM   7   // x y z  a b nx ny
#define JAG_BOX_DIM         8   // x y z  a b c nx ny
#define JAG_ELLIPSOID_DIM   8   // x y z  a b c nx ny
#define JAG_TRIANGLE3D_DIM	9   // x1y1z1 x2y2z2  x3y3z3

#define JAG_POLY_HEADER_COLS_NOMINMAX  5   // geo:id geo:col geo:m geo:n geo:i 
#define JAG_POLY_HEADER_COLS_VECTOR    3   // geo:id geo:col geo:i
#define JAG_POLY_HEADER_COLS_2D  9   // geo:xmin geo:ymin          geo:xmax geo:ymax          geo:id geo:col geo:m geo:n geo:i 
#define JAG_POLY_HEADER_COLS_3D 11   // geo:xmin geo:ymin geo:zmin geo:xmax geo:ymax geo:zmax geo:id geo:col geo:m geo:n geo:i 

// default value max length
#define JAG_DEFAULT_MAX     8

#define JAG_TABLE   1
#define JAG_INDEX   2

#define JAG_OJAG  "OJAG"

#define JAG_CJAG  "CJAG"
#define JAG_CJAG_LEN 4

#define JAG_RED    0
#define JAG_BLUE   1
#define JAG_LEFT    0
#define JAG_RIGHT   1
#define JAG_ISON    2

#define JAG_COLINEAR   0
#define JAG_CLOCKWISE   1
#define JAG_COUNTERCLOCKWISE   2


// types only, cannot be changed
#define JAG_C_COL_TYPE_RANGE    "RG"
#define JAG_RANGE_DATE          1

#define JAG_RANGE_TIMEMICRO       3

#define JAG_RANGE_DATETIMEMICRO   6

#define JAG_RANGE_DATETIMESEC   9

#define JAG_RANGE_DATETIMEMILL  12 
#define JAG_RANGE_DATETIMENANO  15 
#define JAG_RANGE_BIGINT        18 
#define JAG_RANGE_INT           21 
#define JAG_RANGE_SMALLINT      24 
#define JAG_RANGE_DOUBLE        27 
#define JAG_RANGE_LONGDOUBLE    28 
#define JAG_RANGE_FLOAT         30 

#define JAG_RANGE_DIM	2   // begin end

#define JAG_DIST_SYMMETRIC   1
#define JAG_DIST_ASYMMETRIC  2
#define JAG_SIDE_STRAIGHT    3
#define JAG_JOIN_ROUND       4
#define JAG_JOIN_MITER       5
#define JAG_END_ROUND        6
#define JAG_END_FLAT         7
#define JAG_POINT_CIRCLE     8
#define JAG_POINT_SQUARE     9

// max number of points to be sent to client for unspecified points
#define JAG_MAX_POINTS_SENT  3000
#define JAG_MAX_STRLEN_SENT  300

#define  JAG_SP_START  2
#define  JAG_3D        3
#define  JAG_2D        2

// alter table commands
#define JAG_SCHEMA_ADD_COLUMN 		10
#define JAG_SCHEMA_RENAME_COLUMN 	20
#define JAG_SCHEMA_SET 				30
#define JAG_SCHEMA_ADD_TICK 		40
#define JAG_SCHEMA_DROP_TICK 		50
#define JAG_SCHEMA_CHANGE_RETENTION	60

#define JAG_ZERO  0.0000000001

// 2D bbox 4 sides
#define JAG_LEFT_SIDE 	    10
#define JAG_BOTTOM_SIDE     20
#define JAG_RIGHT_SIDE 	    30
#define JAG_TOP_SIDE 	    40

#define JAG_PI 						3.141592653589793
#define JAG_EARTH_MAJOR_RADIUS 		6378137
#define JAG_EARTH_MINOR_RADIUS 		6356752.314245
#define JAG_METER_PER_LAT_DEGREE  	110946.2576173445
#define JAG_METER_MAX_PER_LON_DEGREE  111319.4907932736
#define JAG_RADIAN_PER_DEGREE   	0.017453292519943

#define JAG_METRIC_LEN              8
#define JAG_DEFAULT_METRIC          "0"

#define JAG_SOCK_SQL_HDR_LEN        3   // 1 + 2
#define JAG_SOCK_MSG_HDR_LEN        12  // 8 + 4
#define JAG_SOCK_TOTAL_HDR_LEN      (JAG_SOCK_SQL_HDR_LEN+JAG_SOCK_MSG_HDR_LEN)

// Buffer queue size == single simpfile size
#define JAG_SIMPFILE_LIMIT_BYTES  (128*1024*1024)
//#define JAG_SIMPFILE_LIMIT_BYTES   (1*1)

#define JAG_WALLOG_TRIM_RATIO 0.2

#define JAG_MEET_TIME       1
#define JAG_OVER_TIME       8

#define JAG_MAIN	        0
#define JAG_PREV	        1
#define JAG_NEXT	        2

#define JAG_READ_SQL		0
#define JAG_WRITE_SQL	    1

#define JAG_INLINE_STORE    1
#define JAG_OUTLINE_STORE   2

#define JAG_REDO_MSGLEN    10  // must be less than 16

#define JAG_HASHED_NODES_IN_ALL_CLUSTERS   		0
#define JAG_HASHED_NODES_IN_OLD_CLUSTERS   		1
#define JAG_HASHED_NODE_IN_LAST_CLUSTER    		2
#define JAG_HASHED_NODES_IN_LAST_TWO_CLUSTERS  	3
#define JAG_HASHED_NODE_IN_CLUSTER  	        4

#define JAG_SCALE_STATIC  0
#define JAG_SCALE_GOLAST  1

#define JAG_MAX_HOSTS     1024

#define JAG_MAKE_OBJECTS_CONNECTIONS   0
#define JAG_MAKE_OBJECTS_ONLY          1
#define JAG_MAKE_CONNECTIONS_ONLY      2

//#define JAG_NUM_TS        9
#define JAG_STAR          "*"
#define JAG_STARC         '*'

#define JAG_MILLION       1000000
#define JAG_BILLION       1000000000

#define JAG_BINARY_NODE   'B'
#define JAG_ELEMENT_NODE  'E'

#define JAG_PREPARE      0
#define JAG_COMMIT       1

#define JAG_MSG_SCHEMA   'S'
#define JAG_MSG_HOST     'H'
#define JAG_MSG_HB       'B'
#define JAG_MSG_DATA     'D'
#define JAG_MSG_ERR      'E'
#define JAG_MSG_EMPTY    '_' 
#define JAG_MSG_KV       'K'
#define JAG_MSG_JS       'J'
#define JAG_MSG_HDR      'R'
#define JAG_MSG_X1       'X'
#define JAG_MSG_OK       'O'

#define JAG_MSG_NEXT_END     'E'
#define JAG_MSG_NEXT_MORE    'M'

#define JAG_SOME_NODE_DOWN   'X'

#define  JAG_HOSTS_RECFG     -100

#define JAG_TIME_SECOND_MICRO    1
#define JAG_TIME_SECOND_NANO     2
#define JAG_TIME_SECOND          3
#define JAG_TIME_SECOND_MILLI    4

#define  JAG_LONG_MIN   LONG_MIN+1
#define  JAG_LONG_MAX   LONG_MAX

#define JAG_FAKE_FILE   "."

// If bbox is needed in geo types
//#define  JAG_KEEP_MIN_MAX   1

#endif
