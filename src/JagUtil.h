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
#ifndef _jag_string_util_h_ 
#define _jag_string_util_h_ 

#include <pthread.h>
#include <stdarg.h>
#include <abax.h>
#include <JagNet.h>
#include <JagDef.h>
#include <JagClock.h>
#include <JagTableUtil.h>
#include <JaguarCPPClient.h>
#include <JagSession.h>
#include <JagLog.h>
#include <time.h>


#ifdef _WINDOWS64_
typedef double abaxdouble;
#else
typedef long double abaxdouble;
#endif

class 	JDFS;
class 	OtherAttribute;
class 	JagParseParam;
template <class K> class JagVector;
template <class K, class V> class JagHashMap;
class 	JagStrSplit;

int  	str_str_ch(const char *longstr, char ch, const char *shortstr );
int  	str_print( const char *ptr, int len );

char 	*jag_strtok_r(char *s, const char *delim, char **lasts );
char 	*jag_strtok_r_bracket(char *s, const char *delim, char **lasts );
const char *strrchrWithQuote(const char *str, int c, bool processParenthese=true);
short 	memreversecmp( const char *buf1, const char *buf2, int len );
int 	reversestrlen( const char *str, int maxlen );
const char *jagstrrstr(const char *haystack, const char *needle);

ssize_t raysafepread( int fd, char *buf, jagint len, jagint startpos );
ssize_t raysafepwrite( int fd, const char *buf, jagint len, jagint startpos );

ssize_t jagpwrite( int fd, const char *buf, jagint len, jagint startpos );
ssize_t jagpread( int fd, char *buf, jagint len, jagint startpos );

ssize_t raysaferead( int fd, char *buf, jagint len );
ssize_t raysaferead( int fd, unsigned char *buf, jagint len );
ssize_t raysafewrite( int fd, const char *buf, jagint len );

ssize_t jdfpread( const JDFS *jdfs, char *buf, jagint len, jagint startpos );
ssize_t jdfpwrite( JDFS *jdfs, const char *buf, jagint len, jagint startpos );

short 	getSimpleEscapeSequenceIndex( const char p );
int 	rayatoi( const char *buf, int length );
jagint 	rayatol( const char *buf, int length );
double 	rayatof( const char *buf, int length );
abaxdouble raystrtold( const char *buf, int length );

char 	*itostr( int i, char *buf );
char 	*ltostr( jagint i, char *buf );
Jstr 	intToStr( int i );
Jstr 	longToStr( jagint i );
Jstr 	doubleToStr( double f );
Jstr 	d2s( double f );
Jstr 	doubleToStr( double f, int maxlen, int sig );
Jstr 	longDoubleToStr( abaxdouble f );
//int 	jagsprintfLongDouble( int mode, bool fill, char *buf, abaxdouble i, jagint maxlen );
jagint 	getNearestBlockMultiple( jagint value );
jagint 	getBuffReaderWriterMemorySize( jagint value ); // max as 1024 ( MB ), value and return in MB

bool 	formatOneCol( int tzdiff, int servtzdiff, char *outbuf, const char *inbuf, Jstr &errmsg, const Jstr &name, 
					int offset, int length, int sig, const Jstr &type );

void    MultiDbNaturalFormatExchange( char *buffers[], int num, int numKeys[], const JagSchemaAttribute *attrs[] );
void    dbNaturalFormatExchange( char *buffer, int numKeys, const JagSchemaAttribute *attrs=NULL, 
							  int offset=0, int length=0, const Jstr &type=" " );

Jstr    makeUpperString( const Jstr &str );
Jstr    makeLowerString( const Jstr &str );
JagFixString makeUpperOrLowerFixString( const JagFixString &str, bool isUpper );
Jstr    trimChar( const Jstr &str, char c );
Jstr    trimHeadChar( const Jstr &str, char c );
Jstr    trimTailChar( const Jstr &str, char c='\n' );
Jstr    trimTailLF( const Jstr &str ); 
Jstr    strRemoveQuote( const char *p );
bool    endWith( const Jstr &str, char c );
bool    endWithStr( const Jstr &str, const Jstr &end );
bool    endWhiteWith( const Jstr &str, char c );
bool    endWith( const AbaxString &str, char c );
bool    endWhiteWith( const AbaxString &str, char c );
bool    startWith(  const Jstr &str, char a );
char    *jumptoEndQuote(const char *p);
int     stripStrEnd( char *msg, int len );
void    replaceStrEnd( char *msg, int len );
void    randDataStringSort( Jstr *vec, int maxlen );
bool    isNumeric( const char *str );

FILE    *loopOpen( const char *path, const char *mode );
FILE    *jagfopen( const char *path, const char *mode );
int     jagfclose( FILE *fp );
int     jagopen( const char *path, int flags );
int     jagopen( const char *path, int flags, mode_t mode );
int     jagclose( int fd );
int     jagaccess( const char *path, int mode );
int     jagunlink( const char *path );
int     jagrename( const char *path, const char *newpath );
#ifdef _WINDOWS64_
int     jagftruncate( int fd, __int64 size );
const char *strcasestr(const char *s1, const char *s2);
#else
int     jagftruncate( int fd, off_t length );
#endif
void    jagsleep( useconds_t time, int mode );
bool    lastStrEqual( const char *bigstr, const char *smallstr, int lenbig, int lensmall );
bool    isInteger( const Jstr &dtype );
bool    isFloat( const Jstr &dtype );
bool    isDateTime( const Jstr &dtype );
bool    isTime( const Jstr &dtype );
bool    isDateAndTime( const Jstr &dtype );
Jstr    intToString( int i ) ;
Jstr    longToString( jagint i ) ;
Jstr    ulongToString( jaguint i ) ;
jagint  strchrnum( const char *str, char ch );
jagint  strchrnumskip( const char *str, char ch );
char    *strnchr(const char *s, int c, int n);
int     strInStr( const char *str, int len, const char *str2 );
void    splitFilePath( const char *fpath, Jstr &first, Jstr &last );
Jstr    makeDBObjName( JAGSOCK sock, const Jstr &dbname, const Jstr &objname );
Jstr    makeDBObjName( JAGSOCK sock, const Jstr &dbdotname );
Jstr    jaguarHome();
int     selectServer( const JagFixString &min, const JagFixString &max, const JagFixString &inkey );
int     trimEndWithChar ( char *msg, int len, char c );
int     trimEndChar ( char *msg, char c );
int     trimEndToChar ( char *msg, int len, char stopc );
int     trimEndWithCharKeepNewline ( char *msg, int len, char c );
jagint  availableMemory( jagint &callCount, jagint lastBytes );
int     checkReadOrWriteCommand( const char *pmesg );
int     checkReadOrWriteCommand( int qmode );
int     checkColumnTypeMode( const Jstr &type );
Jstr    formOneColumnNaturalData( const char *data, jagint offset, jagint length, const Jstr &type );
void    printParseParam( JagParseParam *parseParam );
int     rearrangeHdr( int num, const JagHashStrInt *maps[], const JagSchemaAttribute *attrs[], JagParseParam *parseParam,
				  const JagVector<SetHdrAttr> &spa, Jstr &newhdr, Jstr &gbvhdr,
				  jagint &finalsendlen, jagint &gbvsendlen, bool needGbvs=true );
int     checkGroupByValidation( const JagParseParam *parseParam );
int     checkAndReplaceGroupByAlias( JagParseParam *parseParam );
void    convertToHashMap( const Jstr &kvstr, char sep, JagHashMap<AbaxString, AbaxString> &hashmap );
void    changeHome( Jstr &fpath );
int     jaguar_mutex_lock(pthread_mutex_t *mutex);
int     jaguar_mutex_unlock(pthread_mutex_t *mutex);
int     jaguar_cond_broadcast( pthread_cond_t *cond);
int     jaguar_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int     getPassword( Jstr &outPassword );
void    getWinPass( char *pass );
const char *strcasestrskipquote( const char *str, const char *token );
const char *strcasestrskipspacequote( const char *str, const char *token );
void    trimLenEndColonWhite( char *str, int len );
void    trimEndColonWhite( char *str, int len );
void    escapeNewline( const Jstr &instr, Jstr &outstr );
char    *jagmalloc( jagint sz );
int     jagpthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);
int     jagpthread_join(pthread_t thread, void **retval);
jagint  sendRawData( JAGSOCK sock, const char *buf, jagint len );
jagint  sendShortMessageToSock( JAGSOCK sock, const char *buf, jagint len, const char *code4 );
jagint  recvMessage( JAGSOCK sock, char *hdr, char *&buf );
jagint  recvMessageInBuf( JAGSOCK sock, char *hdr, char *&buf, char *sbuf, int sbuflen );
jagint  recvRawData( JAGSOCK sock, char *buf, jagint len );
jagint  _raysend( JAGSOCK sock, const char *hdr, jagint N );
jagint  _rayrecv( JAGSOCK sock, char *hdr, jagint N );
int 	jagmkdir(const char *path, mode_t mode);
int 	jagfdatasync( int fd ); 
int 	jagsync( ); 
int 	jagfsync( int fd ); 
jagint 	jagsendfile( JAGSOCK sock, int fd, jagint size );
Jstr 	jagerr( int errcode );

#ifdef DEBUG_LOCK
#define JAG_BLURT \
  jd(JAG_LOG_LOW, "BLURT pid=%d file=%s func=%s line=%d\n", getpid(),  __FILE__, __func__, __LINE__ );

#define JAG_OVER \
  jd(JAG_LOG_LOW, "OVER pid=%d file=%s func=%s line=%d\n", getpid(), __FILE__, __func__, __LINE__ );

#else
#define JAG_BLURT  
#define JAG_OVER  
#endif

#ifndef _WINDOWS64_
#include <sys/sysinfo.h>
#endif

int     jagmalloc_trim( jagint n );
Jstr    psystem( const char *cmd );
bool    checkCmdTimeout( jagint startTime, jagint timeoutLimit );
char    *getNameValueFromStr( const char *content, const char *name );
ssize_t jaggetline(char **lineptr, size_t *n, FILE *stream);
Jstr    expandEnvPath( const Jstr &path );
struct tm *jag_localtime_r(const time_t *timep, struct tm *result);
char    *jag_ctime_r(const time_t *timep, char *result);
int     formatInsertSelectCmdHeader( const JagParseParam *parseParam, Jstr &str );
bool    isValidVar( const char *name );
bool    isValidNameChar( char c );
bool    isValidCol( const char *name );
bool    isValidColChar( char c );
void    stripEndSpace( char *qstr, char endc );
jagint  _getFieldInt( const char * rowstr, char fieldToken );
void    makeMapFromOpt( const char *options, JagHashMap<AbaxString, AbaxString> &omap );
Jstr    makeStringFromOneVec( const JagVector<Jstr> &vec, int dquote );
Jstr    makeStringFromTwoVec( const JagVector<Jstr> &xvec, const JagVector<Jstr> &yvec );
//int     oneFileSender( JAGSOCK sock, const Jstr &inpath, const Jstr &hashDir, int &actualSent );
int     oneFileSender( JAGSOCK sock, const Jstr &inpath, const Jstr &dbName, const Jstr &tableName, const Jstr &hashDir, int &actualSent );
//int     oneFileReceiver( JAGSOCK sock, const Jstr &filesPath, const Jstr &hdir, bool isDirPath=true );
int     oneFileReceiver( JAGSOCK sock, const Jstr &filesPath, const Jstr &hdir, bool isDirPath );
jagint  sendDirectToSock( JAGSOCK sock, const char *mesg, jagint len, bool nohdr=false );
jagint  recvDirectFromSock( JAGSOCK sock, char *&buf, char *hdr );
jagint  recvDirectFromSockConsume( JAGSOCK sock, char *hdr, char *&buf);
jagint  sendDirectToSockWithHdr( JAGSOCK sock, const Jstr& hdr, const Jstr &mesg );
int     isValidSciNotation(const char *str );
Jstr    fileHashDir( const JagFixString &fstr );
char    lastChar( const JagFixString &str );

#define jagfree(x) if (x) {free(x); x=NULL;}
#define jagdelete(x) if (x) {delete x; x=NULL;}
#define c2uc(c) ( c > 0 ) ? c : 256 + c


void    jagfwrite( const char *str, jagint len, FILE *outf );
void    jagfwritefloat( const char *str, jagint len, FILE *outf );
void    charFromStr( char *dest, const Jstr &src );
bool    streq( const char *s1, const char *s2 );
long    long jagatoll(const char *nptr);
long    long jagatoll(const Jstr &str );
long    jagatol(const char *nptr);
unsigned long    jagatoul(const char *nptr);
long    double jagstrtold(const char *nptr, char **endptr=NULL);
long    double jagatold(const char *nptr, char **endptr=NULL);
double  jagatof(const char *nptr );
double  jagatof(const Jstr &str );
int     jagatoi(const char *nptr );
int     jagatoi(char *nptr, int len );
void    stripTailZeros( char *buf, int len );
bool    jagisspace( char c);
Jstr    trimEndZeros( const Jstr& str );
bool    likeMatch( const Jstr& str, const Jstr& like );
int     levenshtein(const char *s1, const char *s2);

#define get2double( str, p, sep, d1, d2 ) \
    p=(char*)str;\
	while (*p != sep ) ++p; \
	*p = '\0';\
	d1 = jagatof(str);\
	*p = sep;\
	++p;\
	d2 = jagatof(p)

#define get3double( str, p, sep, d1, d2, d3 ) \
    p=(char*)str;\
	while (*p != sep ) ++p; \
	*p = '\0';\
	d1 = jagatof(str);\
	*p = sep;\
	++p;\
	str = p;\
	while (*p != sep ) ++p; \
	*p = '\0';\
	d2 = jagatof(str);\
	*p = sep;\
	++p;\
	d3 = jagatof(p)

#define get2long( str, p, sep, n1, n2 ) \
	if ( !strchr(str, sep) ) { n1=jagatol(str); n2=0; } \
	else { \
      p=(char*)str;\
	  while (*p != sep ) ++p; \
	  *p = '\0';\
	  n1 = jagatol(str);\
	  *p = sep;\
	  ++p;\
	  n2 = jagatol(p) \
	}


void    dumpmem( const char *buf, int len, bool newline=true );
void    dumpmemi( const char *buf, int len, bool newline=true );

const   char *KMPstrstr(const char *text, const char *pat);
void    prepareKMP(const char *pat, int M, int *lps);

Jstr    replaceChar( const Jstr& str, char oldc, char newc );
void    printStr( const Jstr &str );
char    *secondTokenStart( const char *str, char sep=' ' );
char    *thirdTokenStart( const char *str, char sep=' ' );
char    *secondTokenStartEnd( const char *str, char *&pend, char sep=' ' );
jagint  convertToSecond( const char *str);
jagint  convertToMicroSecond( const char *str);
void    rotateat( double oldx, double oldy, double alpha, double x0, double y0, double &x, double &y );
void    rotatenx( double oldnx, double alpha, double &nx );
void    affine2d( double x1, double y1, double a, double b, double d, double e, 
				double dx, double dy, double &x, double &y );
void    affine3d( double x1, double y1, double z1, double a, double b, double c, double d, double e, 
				double f, double g, double h, double i, double dx, double dy, double dz, 
				double &x, double &y, double &z );

double  dotProduct( double x1, double y1, double x2, double y2 );
double  dotProduct( double x1, double y1, double z1, double x2, double y2, double z2 );
void    crossProduct( double x1, double y1, double x2, double y2, double &x, double &y );
void    crossProduct( double x1, double y1, double z1, double x2, double y2, double z2,
                   double &x, double &y, double &z );

bool    jagEQ( double f1, double f2 );
bool    jagLE( double f1, double f2 );
bool    jagGE( double f1, double f2 );

void    putXmitHdr( char *buf, const char *sqlhdr, int msglen, const char *code );
void    putXmitHdrAndData( char *buf, const char *sqlhdr, const char *msg, int msglen, const char *code );
void    getXmitSQLHdr( const char *buf, char *sqlhdr );
long long getXmitMsgLen( char *buf );
void    makeSQLHeader( char *sqlhdr );

Jstr    makeGeoJson( const JagStrSplit &sp, const char *str );
Jstr    makeJsonLineString( const Jstr &title, const JagStrSplit &sp, const char *str );
Jstr    makeJsonLineString3D( const Jstr &title, const JagStrSplit &sp, const char *str );
Jstr    makeJsonPolygon( const Jstr &title, const JagStrSplit &sp, const char *str, bool is3D );
Jstr    makeJsonMultiPolygon( const Jstr &title, const JagStrSplit &sp, const char *str, bool is3D );
Jstr    makeJsonDefault( const JagStrSplit &sp, const char *str );
int     getDimension( const Jstr& colType );
int     getPolyDimension( const Jstr& colType );
Jstr    getTypeStr( const Jstr& colType );

jagint sendMessage( const JagRequest &req, const char *mesg, char msgtype, char endtype );
jagint sendMessageLength( const JagRequest &req, const char *mesg, jagint len, char msgtype, char endtype );
jagint sendMessageLength2( JagSession *session, const char *mesg, jagint len, char msgtype, char endtype );
jagint sendEOM( const JagRequest &req, const char *dbgcode );
jagint sendER( const JagRequest &req, const char *err );
jagint sendER( const JagRequest &req, const Jstr &err );
jagint sendDataMore( const JagRequest &req, const char *data );
jagint sendDataMore( const JagRequest &req, const char *data, jagint len );
jagint sendDataMore( const JagRequest &req, const Jstr &data );
jagint sendDataEnd( const JagRequest &req, const char *data );
jagint sendDataEnd( const JagRequest &req, const Jstr &data );

jagint sendOK( const JagRequest &req, const char *data );
jagint sendOKEnd( const JagRequest &req, const Jstr &data );
jagint sendOKMore( const JagRequest &req, const char *data );
jagint sendOKMore( const JagRequest &req, const Jstr &data );
jagint sendOKMore( const JagRequest &req, const char *data, jagint len );

Jstr    convertToStr( const Jstr  &pm );
Jstr    convertManyToStr( const Jstr &pms );
Jstr    convertType2Short( const Jstr &geotypeLong );
Jstr    firstToken( const char *str, char sep );
bool    hasDefaultValue( char spare4 );
bool    hasDefaultDateTimeValue( char spare4 );
bool    hasDefaultUpdateDateTime( char spare4 );
Jstr    charToStr(char c);
bool    endWithSQLRightBra( const char *sql );
long long get8ByteLongLong( char *str );
int     get4ByteInt( char *str );
jagint  sendOneBatch( int sock, int fd, jagint size );
bool    isAutoUpdateTime( char spare4);
void    reverseStr(char *s);
jagint  readSockAndSave( int sock, const Jstr &recvpath, jagint fsize );
void    getMaskedKey(const JagSchemaRecord &record, const JagFixString &key, JagFixString &maskedKey );
Jstr    getFileHashDir( const JagSchemaRecord &record, const JagFixString &kstr );
void    maskKey(const JagSchemaRecord &record, JagFixString &key );
int     leadZeros( const char *str );



template <class T> T* newObject( bool doPrint = true )
{
	T* o = NULL;
	bool rc = true;
	while ( 1 ) {
		rc = true;
		try { 
   			o  =  new T();
		} catch ( std::bad_alloc &ex ) {
			 if ( doPrint ) {
			 	i("s4136 ******** new oject error [%s], retry ...\n", ex.what() );
			 }
			 o = NULL;
			 rc = false;
		} catch ( ... ) {
			 if ( doPrint ) {
			 	i("s4138 ******** new oject error, retry ...\n" );
			 }
			 o = NULL;
			 rc = false;
		}

		if ( rc ) break;
		sleep(5);
	}

	return o;
}

template <class T> T* newObjectArg( bool doPrint, ... )
{
	T* o = NULL;
	va_list args;
	va_start(args, doPrint);
	bool rc = true;
	while ( 1 ) {
		rc = true;
		try { 
   			o = new T( args );
		} catch ( std::bad_alloc &ex ) {
			 if ( doPrint ) {
			 	i("s4236 ******** new oject error [%s], retry ...\n", ex.what() );
			 }
			 o = NULL;
			 rc = false;
		} catch ( ... ) {
			 if ( doPrint ) {
			 	i("s4238 ******** new oject error, retry ...\n" );
			 }
			 o = NULL;
			 rc = false;
		}

		if ( rc ) break;
		sleep(5);
	}

	va_end( args);
	return o;
}

#define bboxstr2D(srid, xmin, ymin, xmax, ymax) \
	Jstr("OJAG=") + intToStr(srid) + "=0=RC=d " \
    + d2s(xmin) + ":" + d2s(ymin) \
	+ ":" + d2s(xmax) + ":" + d2s(ymax) \
    + " " + d2s((xmax+xmin)/2.0) + " " + d2s((ymax+ymin)/2.0) \
	+ " " + d2s((xmax-xmin)/2.0) + " " + d2s((ymax-ymin)/2.0) \

#define bboxstr3D(srid, xmin, ymin, zmin, xmax, ymax, zmax) \
	Jstr("OJAG=") + intToStr(srid) + "=0=BX=d " \
    + d2s(xmin) + ":" + d2s(ymin) + ":" + d2s(zmin) \
	+ ":" + d2s(xmax) + ":" + d2s(ymax) + ":" + d2s(zmax) \
    + " " + d2s((xmax+xmin)/2.0) + " " + d2s((ymax+ymin)/2.0) + " " + d2s((zmax+zmin)/2.0) \
	+ " " + d2s((xmax-xmin)/2.0) + " " + d2s((ymax-ymin)/2.0) + " " +  d2s((zmax-zmin)/2.0)


void ellipseBoundBox( double x0, double y0, double a, double b, double nx, 
                   double &xmin, double &xmax, double &ymin, double &ymax );

void ellipseMinMax(int op, double x0, double y0, double a, double b, double nx,
                      double &xmin, double &xmax, double &ymin, double &ymax );

#endif
