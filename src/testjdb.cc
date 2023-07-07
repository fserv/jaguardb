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

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <iostream>
#include <sstream>
#include <sys/time.h>
#include <sys/xattr.h>
#include <atomic>
#include <map>
#include <vector>
#include <unordered_map>
#include <fcntl.h>
#include <type_traits>
#include <assert.h>
#include <limits.h>
#include <float.h>
#include <omp.h>

#include <JagCGAL.h>

#include <abax.h>
#include <JagCfg.h>
#include <JagTableSchema.h>
#include <JagIndexSchema.h>
#include <JagDiskArrayServer.h>
#include <JagFileMgr.h>
#include <JagNet.h>
#include <JagNode.h>
#include <JagStrSplit.h>
#include <JagMD5lib.h>
#include <JagUUID.h>
#include <JagParseExpr.h>
#include <JagTime.h>
#include <JagVector.h>
#include <JagParseParam.h>
#include <JagStrSplitWithQuote.h>
#include <JagBlockLock.h>
#include <JagStack.h>
#include <JagMutex.h>
#include <JDFS.h>
#include <JagFixBlock.h>
#include <JagFastCompress.h>
#include <JagHashLock.h>
#include <JagBoundFile.h>
#include <JagLocalDiskHash.h>
#include <JagSchemaRecord.h>
#include <JagSimpleBoundedArray.h>
#include <JagSQLMergeReader.h>
#include <JagDBMap.h>
#include <JagFixHashArray.h>
#include <JagDiskKeyChecker.h>
#include <AbaxCStr.h>
#include <base64.h>
#include <JagGeom.h>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/error/en.h"
#include <JagLineFile.h>
#include <JagParser.h>
#include <JagSortLinePoints.h>
#include <JagHashStrStr.h>
#include <spsc_queue.h>
#include <JagMath.h>
#include <JagLog.h>

#include <functional>
#include "safemap.h"
#include "btree_map.h"

typedef safe::map<std::string, std::string> SafeStrStrMap; 


jagint test_skipstrchrnums( const char *str, char ch );
using namespace std;
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
void *thrdfunc1(void *ptr);
void *thrdfunc2(void *ptr);

unsigned char char2uchar( int c )
{
    if ( c > 0 ) return c;
    return 256 + c;
}

/***
void *cds_insert(void * ptr);
void *cds_find(void * ptr);
void *cds_update(void * ptr);
void *cds_delete(void * ptr);
***/

void *safemap_insert(void * ptr);
void *safemap_find(void * ptr);
void *safemap_update(void * ptr);
void *safemap_delete(void * ptr);

void *deleteMap( void *ptr );
void *insertMap( void *ptr );
void *updateMap( void *ptr );
void *readMap( void *ptr );
void *sleep_func( void *p );

struct TPass {
	TPass() { anum1 = anum2 = anum3 = 0; fd=0; num=0; }
	int fd;
	int num;
	void *ptr1;
	void *ptr2;
	void *ptr3;
	std::atomic<int> anum1;
	std::atomic<int> anum2;
	std::atomic<int> anum3;
};

class Counter
{
		public:
        std::atomic<jagint> value;
    
        void increment(){
			if ( value.is_lock_free() ) {
				printf(" std::atomic<int> is lock free\n");
			}
            ++value;
        }
    
        void decrement(){
            --value;
        }
    
        int get(){
            return value.load();
        }
};
Counter g_cnt;
jagint g_num;

void writefloat( const char *str, jagint len, FILE *outf );
void initCheckLicense();
void testjson();
void testrayrecord();
void testwheretree();
void testtime();
time_t my_convert_utc_tm_to_time_t (struct tm *tm);
void testlaststr();
void testjaguarreader();
void testjaguarreader2();
void testUUID();
void testStrSplitWQuote();
void testAtomic( int N);
int test_schema( const char *tabname );
int test_obj( int N);
int test_blocklock( int N);
int test_fixstring( int N);
int test_time();
int test_misc();
int test_vec( int max );
void *useMutex( void *ptr );
void *useNolock( void *ptr );
void *useAtomic( void *ptr );
void *lockFD( void *ptr );
void *regionLock( void *ptr );
//void *condLock( void *ptr );
int test_threads( int N );
void test_stack();
void test_cds( int N );
void test_lockfreedbmap( int N );
void test_safemap( int N );

void *task1( void *p );
void *task2( void *p );
//void test_threadpool();
void test_fdread();
void test_jdfs();
void test_attr();
void test_blockindex();
void test_fixblockindex();
void test_compress();
void test_hashlock();
void test_hashstr();
void test_size();
void test_parse();
void test_boundfile();
void test_localdiskhash( int N );
void test_diskkeychecker( int N );
// void test_diskarrayclient( int N );
void test_jagarray( int N );
void test_simpleboundedarray( int N );
void test_malloc( int N );
void test_filefamily( );
void test_sqlmerge_reader();
void test_escape();
//void test_keychecker( int N );
void test_map ( int N );
void test_mapvec ( int N );
void test_veconly ( int N );
void test_array ( int N );
void test_lineseg ( int N );
void test_stdhashmap ( int N );
void test_stdmap ( int N );
void test_stdmap_mutex ( int N );
int test_stdmap_threads( int N );
void test_jaghashmap ( int N );
void test_jaghashmap_nofix ( int N );
void test_jagunordmap ( int N );
void test_jagunordmap_nofix ( int N );

void test_jagfixhasharray ( int N );
void test_sys_getline();
void test_jag_getline();
void test_reply ( const char *inputFile );
void test_str( int N );
void test_geo( int n );
void test_sqrt( int n );
void test_split( int n );
void test_btree( int n );
void test_json( int n );
void test_linefile( int n );
void test_distance();
void test_cgal();
void test_new( long n);
void test_equation();
void test_intersection();
void test_fflush( int n );
void test_jstr( int n );
void test_parseparam( int n );
void test_spsc_queue( int n );
void test_numinstr();
void test_base62( char *ss );
void test_base254( char *ss );
void test_faiss( );

int main(int argc, char *argv[] )
{
	int N = 3;
	if ( argc >= 2 ) {
		N = atoi( argv[1] );
	} 

	//test_misc();
	//testjson();
	//testrayrecord();
	//testwheretree();
	//testtime();
	//testlaststr();
	//testjaguarreader();
	//testjaguarreader2();
	//testUUID();
	// testStrSplitWQuote();
	// testAtomic( N );

	// test_schema( argv[2] );
	// test_obj( N );
	//test_blocklock( N );
	// test_fixstring( N );
	// test_time( );
	// test_vec( N );
	//test_threads( N );

	// test_stack();
	// test_threadpool();
	// test_fdread();
	// test_jdfs();
	// test_attr();
	// test_blockindex();
	// test_fixblockindex();
	// test_compress();
	// test_hashlock();
	// test_size();
	// test_parse();
	// test_boundfile();
	// test_localdiskhash( N );
	// test_diskkeychecker( N );
	// test_diskarrayclient( N );
	//test_jagarray( N );
	// test_simpleboundedarray( N );
	// test_malloc( N );
	// test_filefamily();
	// test_sqlmerge_reader();
	// test_escape();
	// test_keychecker( N );

	//test_array( N );
	// test_lineseg( N );
	// test_map( N );
	// test_mapvec( N );
	// test_veconly( N );

	// test_jaghashmap( N );
	// test_jaghashmap_nofix( N );
	// test_jagunordmap( N );
	// test_jagunordmap_nofix( N );
	// test_stdhashmap( N );
	// test_jagfixhasharray( N );
	// test_sys_getline();
	// test_jag_getline();
	// test_reply( argv[1] );
	// test_str( N );
	//test_geo( N );
	//test_sqrt( N );
	//test_split( N );
	//test_stdmap( N );
	//test_stdmap_mutex( N );
	//test_btree( N );
	//test_linefile( N );
	//test_json( N );
	//test_distance();
	// test_cgal();
	//test_new( N );
	//test_equation();
	//test_intersection();
	// test_hashstr();

	//test_fflush( N );
	//test_jstr( N );
	// test_parseparam( N );
    //test_spsc_queue(N );
	//test_stdmap_threads( N );
	//test_cds( N );
	//test_lockfreedbmap( N );
	//test_safemap(N);

	//test_numinstr();
	//test_base62( argv[1] );
	//test_base254( argv[1] );
    test_faiss();
}


#if 0
void testjson()
{
	int max = 200000;

	time_t t1 = time(NULL);
	for ( int i = 0; i < max ; ++i ) {
		Json::Value root;  
		root["uid"] = "u849494944949494";
		root["key"] = "kdk38393jffjfrj";
		root["addr"] = "123 djkdfd  blvd, cr kdk38393jffjfrj";
		root["age"] = "40";
		root["score"] = "90";
	}
	time_t t2 = time(NULL);
	printf("write json %d rows used %d seconds\n", max, t2-t1 );

	/*********
	char *start, *end;
	Json::Value root;  
	root["uid"] = "u849494944949494";
	root["key"] = "kdk38393jffjfrj";
	root["addr"] = "123 djkdfd  blvd, cr kdk38393jffjfrj";
	root["age"] = "40";
	root["score"] = "90";
	// cout << "json root as string " << root.asString() << endl;
	bool rc = root.getString(&start, &end );
	if ( rc ) {
		int len = end - start;
		printf("json start=[%s] length=%d\n", start, len );
	} else {
		printf("root.getString failed\n");
	}

	t1 = time(NULL);
	for ( int i = 0; i < max ; ++i ) {
		root["uid"].asString();
		root["key"].asString();
		root["addr"].asString();
		root["age"].asString();
		root["score"].asString();
		root.getString(&start, &end );
		int len = end - start;
	}
	t2 = time(NULL);
	printf("read json %d rows used %d seconds\n", max, t2-t1 );
	********/

}
#endif

void testrayrecord()
{
	time_t t1 = time(NULL);
	/***
	for ( int i = 0; i < max ; ++i ) {
		JagRecord root;  
		root.addNameValue("uid", "u849494944949494");
		root.addNameValue("key", "kdk38393jffjfrj");
		root.addNameValue("addr", "123 djkdfd  blvd, cr kdk38393jffjfrj");
		root.addNameValue("age", "40");
		root.addNameValue("score", "90");
	}
	time_t t2 = time(NULL);
	printf("write rayrecord %d rows used %d seconds\n", max, t2-t1 );
	***/

	//root.addNameValue("uid", "u849494944949494");
	//root.addNameValue("key", "kdk38393jffjfrj");
	//root.addNameValue("addr", "123 djkdfd  blvd, cr kdk38393jffjfrj");
	//root.addNameValue("age", "40");
	JagRecord root;  
	int c = root.addNameValue("score", "90");
	d("s1118 add score->90 rc=%d\n", c );
	t1 = time(NULL);
	c = root.addNameValue("db:tab123", "myvalue");
	char *p;
	/**
	for ( int i = 0; i < max ; ++i ) {
		p = root.getValue("uid");
		free( p );
		p = root.getValue("key");
		free( p );
		p = root.getValue("addr");
		free( p );
		p = root.getValue("age");
		free( p );
		p = root.getValue("score");
		free( p );
	}
	t2 = time(NULL);
	printf("read rayrecord %d rows used %d seconds\n", max, t2-t1 );
	***/

	p = root.getValue("score");
	d("s1028 score=[%s] src=[%s]\n", p, root.getSource() );
	if ( p) free(p);

	p = root.getValue("db:tab123");
	d("s1028 db:tab123=[%s] src=[%s]\n", p, root.getSource() );
	if ( p) free(p);

	p = root.getValue("db32");
	d("s1028 db32=[%s] src=[%s]\n", p, root.getSource() );
	if ( p) free(p);

	#if 0
	using namespace rapidjson;
	StringBuffer s;
	Writer<StringBuffer> writer(s);
	writer.StartObject();   
	writer.Key("key1");  
	writer.String("val1");
	writer.Key("key2");  
	writer.String("val2");
	writer.Key("x1");  
	writer.Double(12.42);
	writer.Key("Null1");  
	writer.Null();
	writer.Key("y1");  
	writer.Double(19.42);
	writer.Double( jagatof("23.434") );
	writer.Key("Null2");  
	writer.Null();

	writer.Key("points");  
	writer.StartArray();   
	for (unsigned i = 0; i < 4; i++) {
		writer.Double(i);
	}
	writer.EndArray();   

	//cout << s.GetString() << endl;
	#endif

	/**
	JagRecord rec2;
	rec2.fromJSON( s.GetString() );
	rec2.print();

	rec2.toJSON();
	rec2.print();
	**/
	JagRecord rec;
	Jstr n = "innerrings(polygon((0 0, 22 44, 98 12, 0 0), (9 3 , 9 0, 82 83, 9 3 ))) dkdjdjdd djdd jdkdjkdjkdjkd djkdjdk djdjd jdkdjdkj";
	//Jstr n = "innerrings(polygon((0 0, 22 44,";
	Jstr v = "CJAG=0=0=PL=d 9.0:0.0:82.0:83.0 9.0:3.0 9.0:0.0 82.0:83.0 9.0:3.0 03jejde9 jdkv njvneje fj vvj eij ehehfnffffffffffff jeihej ee ene eeneddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd==";
	rec.addNameValue( n.c_str(), v.c_str() );
	d("s1728 src=[%s]\n", rec.getSource() );

	p = root.getValue( n.c_str() );
	d("s2831 n=[%s]  v=[%s]\n", n.c_str(), v.c_str() );
	if (p) free(p);

	std::vector<string> output;
	output.push_back("dsdsd11");
	output.push_back("dsdsd2");
	output.push_back("dsdsd3");

	/***
	std::stringstream ifs;
	ifs << boost::geometry::wkt(output);
	Jstr out;
	out = ifs.str().c_str();
	***/
}

void testwheretree()
{
	/**
    BinaryOpNode *root;
    BinaryExpressionBuilder b;
    Jstr expression = "1=1 and f=2 or ( 2=2 and f=3) or ( b in ( 1,2) )";
	cout << expression << endl;
    try {
		JagVector<OneNameAttribute> onetable;
        JagVector<OneNameAttribute> oneindex;
        root = b.parse(expression, onetable, oneindex);
        root->print();
        printf("\n");
        //cout << " result = " << root->getValue();
        cout << endl;
    } catch ( const NotWellFormed &e ) {
        printf("exception BinaryExpressionBuilder::NotWellFormed %s \n", e.what());
    } catch ( const char *s ) {
        printf("exception %s\n", s );
    } catch ( ... ) {
        printf("unknown exception\n");
    }
	root->clear();
	delete root;
	***/
}

void testtime()
{
	time_t nowt = time(NULL);
	printf("nowt=%ld\n", nowt );

	struct tm * gmt_tms = gmtime( &nowt );
	printf(" gmtime hour=%d min=%d\n", gmt_tms->tm_hour, gmt_tms->tm_min );

	struct tm * local_tms = localtime( &nowt );
	printf(" local  hour=%d min=%d\n", local_tms->tm_hour, local_tms->tm_min );
	// same !!!

	time_t t1 =  my_convert_utc_tm_to_time_t ( gmt_tms );
	time_t t2 =  my_convert_utc_tm_to_time_t ( local_tms );

	printf(" gmt t1=%ld  diff=%ld\n", t1, t1 - nowt );
	printf(" lcoal t2=%ld diff=%ld\n", t2, t2-nowt );

#if 0
          struct tm {
               int tm_sec;         /* seconds */
               int tm_min;         /* minutes */
               int tm_hour;        /* hours */
               int tm_mday;        /* day of the month */
               int tm_mon;         /* month */
               int tm_year;        /* year */
               int tm_wday;        /* day of the week */
               int tm_yday;        /* day in the year */
               int tm_isdst;       /* daylight saving time */
           };
#endif

}


time_t my_convert_utc_tm_to_time_t (struct tm *tm)
{
	int tdiff = JagTime::getTimeZoneDiff();
	printf("tdiff = %d min \n", tdiff/60 );
	
	struct timeval tval;
	struct timezone tzone;

    gettimeofday( &tval, &tzone );
	printf(" tz_minuteswest=%d tz_dsttime=%d \n", tzone.tz_minuteswest, tzone.tz_dsttime );

	/********
           struct timeval {
               time_t      tv_sec;   
               suseconds_t tv_usec;  
           };

       and gives the number of seconds and microseconds since the Epoch (see time(2)).  The tz argument is a struct timezone:

           struct timezone {
               int tz_minuteswest;     ///  minutes west of Greenwich 
               int tz_dsttime;         // type of DST correction 
           };

	***/

    return tval.tv_sec;
}

void testlaststr()
{
	bool rc;
	Jstr s1 = "hello world";
	Jstr s2 = "world";
	Jstr s3 = "World";

	rc = lastStrEqual(s1.c_str(), s2.c_str(), 12, 4 );
	if ( rc ) {
		printf("s1=%s  last equal s2=%s\n", s1.c_str(), s2.c_str() );
	} else {
		printf("s1=%s  NOT last equal s2=%s\n", s1.c_str(), s2.c_str() );
	}
}

#if 0
void testjaguarreader()
{
    Jstr getstr = "";
    int getint = 0;
    jagint getlong = 0;
    float getfloat = 0.0;
    double getdouble = 0.0;
    int gettype = 0;

    JaguarReader testjagreader( "test", "u1", "k1 > Apple01234567890 and k1 < Epple01234567890 or k1 = 4ESjbmBauozwu5Rz" );
    JaguarReader testjagreader2( "test", "u2", "uid < 100000" );

    printf("testjagreader 1 begin\n");
    testjagreader.begin();
    while ( testjagreader.next() ) {
        getstr = testjagreader.getString( "k1" );
        gettype = testjagreader.getType( "v1" );
        printf("jagreader 1 string is [%s]\n", getstr.c_str());
    }
    printf("testjagreader 1 end\n");
    printf("testjagreader 2 begin\n");
    testjagreader2.begin();
    while ( testjagreader2.next() ) {
        getint = testjagreader2.getInt( "uid" );
        getlong = testjagreader2.getLong( "uid" );
        getfloat = testjagreader2.getFloat( "amt" );
        getdouble = testjagreader2.getDouble( "unit" );
        getstr = testjagreader2.getTimeString( "daytime" );
        gettype = testjagreader2.getType( "amt" );
        printf("jagreader 2 int [%d] [%d] jagint [%lld] [%lld] float [%f] [%f] double [%f] [%f] timestring [%s] type [%d]\n", getint, getint*2, getlong, getlong*2, getfloat, getfloat*2, getdouble, getdouble*2, getstr.c_str(), gettype);
    }
    printf("testjagreader 2 end\n");
}


void testjaguarreader2()
{
    Jstr getstr;
    int getint = 0;
    jagint getlong = 0;
    float getfloat = 0.0;
    double getdouble = 0.0;
    int gettype = 0;

    JaguarReader reader( "test", "jbench" );

    reader.begin();
	int cnt = 0;
    while ( reader.next() ) {
        getstr = reader.getString( "uid" );
		++cnt;
    }

	printf("testjaguarreader2 cnt=%d\n", cnt );
	sleep ( 10 );
}
#endif

void testUUID()
{
	JagUUID uuid;
	Jstr getstr1 = uuid.getStringAt(1);
	printf("get uuid string1 [%s] length=[%ld]\n", getstr1.c_str(), strlen(getstr1.c_str()));

	Jstr getstr2 = uuid.getStringAt( 99282);
	printf("get uuid string2 [%s] length=[%ld]\n", getstr2.c_str(), strlen(getstr2.c_str()));

	Jstr getstr3 = uuid.getStringAt( 1 );
	printf("get uuid string3 [%s] length=[%ld]\n", getstr3.c_str(), strlen(getstr3.c_str()));

    /////////////////

    //int n = 2;

    char buf[32];
    Jstr suid = uuid.getGidString();

    printf("short geoid=[%s] len=%ld\n", suid.s(), suid.size() );


    struct timeval now;
    gettimeofday( &now, NULL );

    jaguint au = (jaguint)now.tv_usec + jaguint(now.tv_sec)*1000000;

    sprintf(buf, "%llu", au);
    printf("leng of future nowMicroseconds buf=[%s] =%ld\n", buf, strlen(buf) );


}


void testStrSplitWQuote()
{
	Jstr teststr("select * from u1 where (uid = 10 or uid=20); insert into u1 ( uid, addr ) values ( '12435', 'hello;world' ) ; select * from u1 where uid > 100;");
	printf("str [%s]\n", teststr.c_str());
	JagStrSplitWithQuote splitwq(teststr.c_str(), ';');
	splitwq.print();
	printf("len [%lld]\n\n", splitwq.length());

	Jstr teststr2("   select * from u1 where uid = 10; ; select * from u1 where uid > 100 or ( uid < 20)");
	printf("str [%s]\n", teststr2.c_str());
	JagStrSplitWithQuote splitwq2(teststr2.c_str(), ';');
	splitwq2.print();
	printf("len [%lld]\n\n", splitwq2.length());

	Jstr teststr3(" create table t123 (key: a int, value: b int, c linestring(43), d polygon, e enum('a', 'b', 'c'), f point3d, ); ");
	printf("str [%s]\n", teststr3.c_str());
	JagStrSplitWithQuote splitwq3(teststr3.c_str(), ',');
	splitwq3.print();
	printf("len [%lld]\n", splitwq3.length());
	
	const char *p;
	p = strchr( teststr3.c_str(), '(');
	JagStrSplitWithQuote splitwq4(p+1, ',');
	splitwq4.print();
	printf("len [%lld]\n", splitwq4.length());

	Jstr aa = "(a1, f2, 'fdfdfd', 12, 'fdfdfd3333 () ')";
	d("aa=[%s]\n", aa.s() );
	JagStrSplitWithQuote saa(aa.s(), ',', false );
	saa.print();

	aa = "sin(a1,cos(dd), 33), f2, 'fdfdfd', 12, 'fdfdfd3333 ()',99";
	d("aa=[%s]\n", aa.s() );
	saa.init(aa.s(), ',', true );
	saa.print();

	JagStrSplitWithQuote q;
	q.count(aa.s(), ',' );
	//d("aa count=%d\n", nn );

	q.init("aaaa, aaaa, , , ,,, 'fdfdfdfd'   'e    ff(())', dkdd ", ' ', true, true );
	q.print();

	q.init(" 2 4 'ddd dd'  'dddd44' ", ' ', true, true );
	q.print();

	q.init("2 4 'ddd dd'  'dddd44'", ' ', true, true );
	q.print();



}

jagint test_skipstrchrnums( const char *str, char ch )
{
    if ( ! str || *str == '\0' ) return 0;
    jagint cnt = 0;
    while ( *str != '\0' ) {
        if ( *str == '\'' ) { while ( *str != '\'' && *str != '\0' ) ++str; ++str; }
        else if ( *str == '"' ) { while ( *str != '"' && *str != '\0' ) ++str; ++str; }
        else if ( *str == ch ) { while ( *str == ch ) ++str; ++cnt; }
		else ++str;
    }

    return cnt;
}


void testAtomic( int N )
{
	g_cnt.value = 0;
	JagClock clock;
	clock.start();
	for ( int i = 0; i < N; ++i ) {
		g_cnt.increment();
	}
	clock.stop();
	printf("%d atomic increment %lld usec\n", N, clock.elapsedusec() );
	cout << g_cnt.value << endl;
}


int test_obj( int N)
{
	printf("test_obj ...\n");

	char *p;
	char buf[301];
	int i;
	for ( i = 0; i < 300; ++i ) { buf[i] = 'a'; } buf[i] = '\0';

	JagClock clock;

	clock.start();
	for ( int i = 0; i < N; ++i ) {
		p = (char*)malloc(300);
		free(p );
	}
	clock.stop();
	printf("%d  malloc-free(300)=%lld usec \n", N,  clock.elapsedusec() );

	clock.start();
	for ( int i = 0; i < N; ++i ) {
		Jstr s(buf);
	}
	clock.stop();
	printf("%d  Jstr=%lld usec \n", N,  clock.elapsedusec() );


	srand(10);
	clock.start();
	JagHashMap<AbaxString,jagint> *map = new JagHashMap<AbaxString,jagint>();
	AbaxString rs;
	for ( int i = 0; i < N; ++i ) {
		rs = rs.randomValue(16);
		map->addKeyValue(rs, i );
	}
	clock.stop();
	printf("%d  JagHashMap(addKeyValue)=%lld usec \n", N,  clock.elapsedusec() );

	srand(10);
	clock.start();
	jagint v;
	for ( int i = 0; i < N; ++i ) {
		rs = rs.randomValue(16);
		map->getValue(rs, v );
	}
	clock.stop();
	printf("%d  JagHashMap(getValue)=%lld usec \n", N,  clock.elapsedusec() );
	delete map;



	srand(10);
	clock.start();
	jag_hash_t hashtab;
	jag_hash_init ( &hashtab, 10 );
	for ( int i = 0; i < N; ++i ) {
		rs = rs.randomValue(16);
		jag_hash_insert_str_int( &hashtab, rs.c_str(), i );
	}
	clock.stop();
	printf("%d  jag_hash_t(jag_hash_insert_str_int)=%lld usec \n", N,  clock.elapsedusec() );

	srand(10);
	clock.start();
	int vv;
	for ( int i = 0; i < N; ++i ) {
		rs = rs.randomValue(16);
		jag_hash_lookup_str_int( &hashtab, rs.c_str(), &vv );
	}
	clock.stop();
	printf("%d  jag_hash_t(jag_hash_lookup_str_int)=%lld usec \n", N,  clock.elapsedusec() );
	jag_hash_destroy(  &hashtab );


	clock.start();
	char buf2[300];
	for ( int i = 0; i < N; ++i ) {
		memset( buf2, 0, 300 );
	}
	clock.stop();
	printf("%d  memset(300)=%lld usec \n", N,  clock.elapsedusec() );

	char fpath[] = "/tmp/test.txt";
	FILE *fp;
	clock.start();
	for ( int i = 0; i < N; ++i ) {
		fp = fopen( fpath, "w" );
		fclose( fp );
	}
	clock.stop();
	printf("%d  fopen/fclose=%lld usec \n", N,  clock.elapsedusec() );

	int ofd;
	clock.start();
	for ( int i = 0; i < N; ++i ) {
		ofd =  open("/tm/testobj.txt", O_CREAT|O_RDWR|JAG_NOATIME, S_IRWXU);
		close( ofd );
	}
	clock.stop();
	printf("%d  open/close=%lld usec \n", N,  clock.elapsedusec() );

	JagBlockLock                    *_blockLock;
	_blockLock = new JagBlockLock();
	clock.start();
	for ( int i = 0; i < N; ++i ) {
		_blockLock->writeLock( -1 );
		_blockLock->writeUnlock( -1 );
	}
	clock.stop();
	printf("%d  _blockLock->writeLock/unLock(-1)=%lld usec \n", N,  clock.elapsedusec() );

	clock.start();
	for ( int i = 0; i < N; ++i ) {
		_blockLock->readLock( -1 );
		_blockLock->readUnlock( -1 );
	}
	clock.stop();
	printf("%d  _blockLock->writeLock/unLock(-1)=%lld usec \n", N,  clock.elapsedusec() );


	clock.start();
	for ( int i = 0; i < N; ++i ) {
		_blockLock->writeLock( 0 );
		_blockLock->writeUnlock( 0 );
	}
	clock.stop();
	printf("%d  _blockLock->writeLock/unLock(0)=%lld usec \n", N,  clock.elapsedusec() );

	clock.start();
	for ( int i = 0; i < N; ++i ) {
		_blockLock->readLock( 0 );
		_blockLock->readUnlock( 0 );
	}
	clock.stop();
	printf("%d  _blockLock->writeLock/unLock(0)=%lld usec \n", N,  clock.elapsedusec() );


	fp = fopen( fpath, "w" );
	clock.start();
	for ( int i = 0; i < N; ++i ) {
		fprintf(fp, "%s\n", buf);
	}
	clock.stop();
	printf("%d  fprintf=%lld usec \n", N,  clock.elapsedusec() );
	fclose( fp );

	fp = fopen( fpath, "r" );
	clock.start();
	for ( int i = 0; i < N; ++i ) {
		fgets( buf, 299, fp);
	}
	clock.stop();
	printf("%d  fgets=%lld usec \n", N,  clock.elapsedusec() );
	fclose( fp );

	int fd;
	clock.start();
	for ( int i = 0; i < N; ++i ) {
		fd = open((char *)fpath, O_RDONLY|JAG_NOATIME, S_IRWXU);
		close(fd );
	}
	clock.stop();
	printf("%d  open-close=%lld usec \n", N,  clock.elapsedusec() );

	/****
	clock.start();
	for ( i = 0; i < 300; ++i ) { buf[i] = 'a'; } buf[i] = '\0';
	for ( int i = 0; i < N; ++i ) {
		JagFileMgr::writeTextFile( "/tmp/aaa.txt", buf );
	}
	clock.stop();
	printf("%d  JagFileMgr::writeTextFile( /tmp/aaa.txt 300)=%lld usec \n", N,  clock.elapsedusec() );
	*****/

	pthread_mutex_init( &g_mutex, NULL );

	int a = 0;
	clock.start();
	for ( int i = 0; i < N; ++i ) {
		pthread_mutex_lock ( &g_mutex );
		++a;
		pthread_mutex_unlock ( &g_mutex );
	}
	clock.stop();
	printf("%d  mutext lock/unlock=%lld usec \n", N,  clock.elapsedusec() );
	pthread_mutex_destroy( &g_mutex );
	return 0;


}

int test_blocklock( int N)
{
	printf("test_blocklock ...\n");

	JagClock clock;
	//JagReadWriteLock    *_lock = new JagReadWriteLock();
	pthread_rwlock_t    *_lock = newJagReadWriteLock();
	clock.start();
	int a = 0;
	for ( int i = 0; i < N; ++i ) {
		JagReadWriteMutex mutex( _lock, JagReadWriteMutex::WRITE_LOCK );
		++a;
	}
	clock.stop();
	printf("%d  JagReadWriteMutex::WRITE_LOCK=%lld usec \n", N,  clock.elapsedusec() );


	clock.start();
	for ( int i = 0; i < N; ++i ) {
		JagReadWriteMutex mutex( _lock, JagReadWriteMutex::READ_LOCK );
		++a;
	}
	clock.stop();
	printf("%d  JagReadWriteMutex::READ_LOCK=%lld usec \n", N,  clock.elapsedusec() );

	JagBlockLock                    *_blockLock;
	_blockLock = new JagBlockLock();
	clock.start();
	for ( int i = 0; i < N; ++i ) {
		_blockLock->writeLock( -1 );
		++a;
		_blockLock->writeUnlock( -1 );
	}
	clock.stop();
	printf("%d  _blockLock->writeLock/unLock(-1)=%lld usec \n", N,  clock.elapsedusec() );

	clock.start();
	for ( int i = 0; i < N; ++i ) {
		_blockLock->readLock( -1 );
		++a;
		_blockLock->readUnlock( -1 );
	}
	clock.stop();
	printf("%d  _blockLock->writeLock/unLock(-1)=%lld usec \n", N,  clock.elapsedusec() );


	clock.start();
	for ( int i = 0; i < N; ++i ) {
		_blockLock->writeLock( 0 );
		++a;
		_blockLock->writeUnlock( 0 );
	}
	clock.stop();
	printf("%d  _blockLock->writeLock/unLock(0)=%lld usec \n", N,  clock.elapsedusec() );

	clock.start();
	for ( int i = 0; i < N; ++i ) {
		_blockLock->readLock( 0 );
		++a;
		_blockLock->readUnlock( 0 );
	}
	clock.stop();
	printf("%d  _blockLock->writeLock/unLock(0)=%lld usec \n", N,  clock.elapsedusec() );


	delete _lock;
	delete _blockLock;

	pthread_mutex_init( &g_mutex, NULL );
	clock.start();
	for ( int i = 0; i < N; ++i ) {
		pthread_mutex_lock ( &g_mutex );
		++a;
		pthread_mutex_unlock ( &g_mutex );
	}
	clock.stop();
	printf("%d  mutext lock/unlock=%lld usec \n", N,  clock.elapsedusec() );
	pthread_mutex_destroy( &g_mutex );

	pthread_rwlock_t rwlock;
	pthread_rwlock_init(&rwlock, NULL);

	clock.start();
	for ( int i = 0; i < N; ++i ) {
		pthread_rwlock_wrlock(&rwlock);
		++a;
		pthread_rwlock_unlock(&rwlock);
	}
	clock.stop();
	printf("%d  pthread_rwlock_t wrlock/unlock=%lld usec \n", N,  clock.elapsedusec() );

	clock.start();
	for ( int i = 0; i < N; ++i ) {
		pthread_rwlock_rdlock(&rwlock);
		++a;
		pthread_rwlock_unlock(&rwlock);
	}
	clock.stop();
	printf("%d  pthread_rwlock_t rdlock/unlock=%lld usec \n", N,  clock.elapsedusec() );

	return 0;

}


int test_fixstring( int N)
{
	JagClock clock;

	clock.start();
	for ( int i = 0; i < N; ++i ) {
		JagFixString str("  0123456789  ", 14 );
		str.ltrim();
		// printf("ltrim=[%s]\n", str.c_str() );
	}
	clock.stop();
	printf("%d  ltrim=%lld usec \n", N,  clock.elapsedusec() );


	clock.start();
	for ( int i = 0; i < N; ++i ) {
		JagFixString str("  0123456789  ", 14 );
		str.rtrim();
		// printf("rtrim=[%s]\n", str.c_str() );
	}
	clock.stop();
	printf("%d  rtrim=%lld usec \n", N,  clock.elapsedusec() );


	clock.start();
	for ( int i = 0; i < N; ++i ) {
		JagFixString str("  0123456789  ", 14 );
		str.trim();
		// printf("trim=[%s]\n", str.c_str() );
	}
	clock.stop();
	printf("%d  trim=%lld usec \n", N,  clock.elapsedusec() );

	JagFixString str("  0123456789  ", 14 );
	str.substr(2);
	printf("subdstr(2)=[%s]\n", str.c_str() );

	str.substr(3, 5);
	printf("subdstr(3,5)=[%s]\n", str.c_str() );

	return 0;
}

int test_time( )
{
	int tt = 100;
	jagint sec = JagTime::getTimeZoneDiff();
	printf("timezone diff %lld\n", sec );

	int aa = atoi("2013-23-23");
	printf("aa=%d\n", aa );

	aa = atoi("23-13");
	printf("aa=%d\n", aa );

	sec = tt/1000000;
	//jagint usec = tt % 1000000;
	struct tm *tmptr = localtime( (time_t *)&sec );
	char tmstr[48];
	strftime( tmstr, sizeof(tmstr), "%Y-%m-%d %H:%M:%S", tmptr );
	printf("%s\n", tmstr );

	memset( tmstr, 0, 48 );
	bool rc = JagTime::getDateFromStr("2012-12-23", tmstr );
	printf("tmstr=[%s] rc=%d\n", tmstr, rc );

	char buf[8];
	memset( buf, 0, 8 );
	time_t now = time(NULL);
	struct tm *ptm;
	ptm = localtime( &now );
	strftime( buf, 8,  "%z", ptm );

    int hrmin =  atoi( buf );
    int hour = hrmin/100;
    int min = hrmin % 100;
    int totmin;
	if ( hour < 0 ) {
		totmin = hour * 60 - min;
	} else {
		totmin = hour * 60 + min;
	}

	printf("tzone diff=%s   hr=%d min=%d  totmin=%d\n", buf, hour, min, totmin );

    unsigned long nwm = JagTime::nowMicroSeconds();
    char tbf[32];
    sprintf(tbf, "%lu", nwm );
    printf("now microseconds=%lu len=%ld\n", nwm, strlen(tbf) );

	return 0;
}


int test_vec( int max )
{
	for ( int i = 0; i < max; ++i ) {
		JagVector<Jstr> vec;
		vec.append( "jkjdkjdjkdjfdf d" );
	}

	JagVector<char> vc;

	vc.append('a');
	vc.append('b');
	vc.append('c');
	vc.append('d');

	printf("test_vec:\n");
	vc.printChars();

	vc.reverse();
	printf("test_vec reversed:\n");
	vc.printChars();

	return 0;
}

int test_threads( int N )
{
	pthread_t  thread[200];
	//TPass pass[200];

	g_num = 0;
	JagClock  clock;

	int fd =  open("testthread.txt", O_CREAT|O_RDWR|JAG_NOATIME, S_IRWXU);
	if ( fd < 0 ) {
		printf("Error open testthread.txt for O_CREAT|O_RDWR\n");
	}
	write(fd, "OKOKOKOKOK", 10 );

	/*****
	clock.start();
	for ( int i =0; i < N; ++i ) {
		pass[i].fd = fd;
		pthread_create( &thread[i], NULL, lockFD, &(pass[i]) );
	}
	for ( int i =0; i < N; ++i ) {
    	pthread_join(thread[i], NULL );
	}
	clock.stop();
	printf("%d threads useNolock=%lld usec\n", N, clock.elapsedusec() );
	****/

	clock.start();
	for ( int i =0; i < N; ++i ) {
		pthread_create( &thread[i], NULL, useNolock, NULL );
	}
	for ( int i =0; i < N; ++i ) {
    	pthread_join(thread[i], NULL );
	}
	clock.stop();
	printf("%d threads useNolock=%lld usec\n", N, clock.elapsedusec() );

	clock.start();
	for ( int i =0; i < N; ++i ) {
		pthread_create( &thread[i], NULL, useMutex, NULL );
	}
	for ( int i =0; i < N; ++i ) {
    	pthread_join(thread[i], NULL );
	}
	clock.stop();
	printf("%d threads useMutex=%lld usec\n", N, clock.elapsedusec() );

	clock.start();
	for ( int i =0; i < N; ++i ) {
		pthread_create( &thread[i], NULL, useAtomic, NULL );
	}
	for ( int i =0; i < N; ++i ) {
    	pthread_join(thread[i], NULL );
	}
	clock.stop();
	printf("%d threads useAtomic=%lld usec\n", N, clock.elapsedusec() );

	clock.start();
	for ( int i =0; i < N; ++i ) {
		pthread_create( &thread[i], NULL, useNolock, NULL );
	}
	for ( int i =0; i < N; ++i ) {
    	pthread_join(thread[i], NULL );
	}
	clock.stop();
	printf("%d threads useNolock=%lld usec\n", N, clock.elapsedusec() );

	clock.start();
	for ( int i =0; i < N; ++i ) {
		pthread_create( &thread[i], NULL, useMutex, NULL );
	}
	for ( int i =0; i < N; ++i ) {
    	pthread_join(thread[i], NULL );
	}
	clock.stop();
	printf("%d threads useMutex=%lld usec\n", N, clock.elapsedusec() );

	clock.start();
	JagBlockLock *blk = new JagBlockLock();
	TPass tp[N];
	for ( int i =0; i < N; ++i ) {
		tp[i].ptr1 = (void*)blk;
		tp[i].num = i;
		pthread_create( &thread[i], NULL, regionLock, &tp[i] );
	}
	for ( int i =0; i < N; ++i ) {
    	pthread_join(thread[i], NULL );
	}
	clock.stop();
	printf("%d threads regionLock=%lld usec\n", N, clock.elapsedusec() );

	/******
	JagCondLock *cond = new JagCondLock();
	clock.start();
	for ( int i =0; i < N; ++i ) {
		tp[i].ptr1 = (void*)cond;
		tp[i].num = i;
		pthread_create( &thread[i], NULL, condLock, &tp[i] );
	}
	for ( int i =0; i < N; ++i ) {
    	pthread_join(thread[i], NULL );
	}
	clock.stop();
	printf("%d threads condLock=%lld usec\n", N, clock.elapsedusec() );
	******/

	close( fd );

	return 0;

}

void *useMutex( void *ptr )
{
	pthread_mutex_lock ( &g_mutex );
	++ g_num;
	pthread_mutex_unlock ( &g_mutex );
	return NULL;

}

void *useAtomic( void *ptr )
{
	g_cnt.increment();
	return NULL;

}

void *useNolock( void *ptr )
{
	++ g_num;
	return NULL;

}

void *lockFD( void *ptr )
{
	int rc;
	//TPass *p = (TPass*) ptr;
	//int fd1 = p->fd;  // same fd cannot cause multi-threads block on lock
	struct flock fl;

	// only when each thread opens the same file can each thread be blocked
	// on the F_OFD_SETLKW
	int fd =  open("testthread.txt", O_CREAT|O_RDWR|JAG_NOATIME, S_IRWXU);
	if ( fd < 0 ) {
		printf("Error open testthread.txt for O_CREAT|O_RDWR\n");
	}
	// int fd = dup( fd1 );  // dup cannot cause lock

	fl.l_type = F_WRLCK;
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 5;
	fl.l_pid = 0;
	//rc = fcntl(fd, F_OFD_SETLKW, &fl);
	printf("thread=%ld fcntl F_OFD_SETLKW rc=%d errno=%d  err=[%s]\n", pthread_self(), rc, errno, strerror( errno ) );

	rc = write( fd, "11", 2 );
	printf("thread=%ld wrote %d bytes to fd=%d\n", pthread_self(), rc, fd );

	close( fd );
	return NULL;

}


void *regionLock( void *ptr )
{
	int i;

	TPass *p = (TPass*) ptr;

	JagBlockLock *blockLock = ( JagBlockLock*) p->ptr1;
	int num = p->num;

	i =  rand()%3;
	if ( num == 14 ) {
		printf("%ld %d ****** blockLock->writeLock(-1)...\n", pthread_self(), num );
		fflush( stdout );
		blockLock->writeLock( -1 );
		printf("%ld %d blockLock->writeLock(-1) done\n", pthread_self(), num );
		fflush( stdout );
		printf("%ld %d write sleep 2 (-1) ...\n", pthread_self(), num );
		sleep(2);
		printf("%ld %d write sleep 2 (-1) done\n", pthread_self(), num );
		blockLock->writeUnlock( -1 );
		printf("%ld %d ####### blockLock->writeUnLock(-1) done\n", pthread_self(), num );
		fflush( stdout );
	} else if ( num == 15 ) {
		printf("%ld %d <<<<<< blockLock->readLock(-1)...\n", pthread_self(), num );
		fflush( stdout );
		blockLock->readLock( -1 );
		printf("%ld %d blockLock->readLock(-1) done\n", pthread_self(), num );
		fflush( stdout );
		printf("%ld %d read sleep 2 (-1) ...\n", pthread_self(), num );
		sleep(2);
		printf("%ld %d read sleep 2 (-1) done\n", pthread_self(), num );
		blockLock->readUnlock( -1 );
		printf("%ld %d >>>>>>> blockLock->readUnLock(-1) done\n", pthread_self(), num );
		fflush( stdout );
	} else if ( ( num%3) == 0 ) {
		printf("%ld %d blockLock->writeLock(%d)...\n", pthread_self(), num, i );
		fflush( stdout );
		blockLock->writeLock( i );
		printf("%ld %d blockLock->writeLock(%d) done\n", pthread_self(), num, i );
		fflush( stdout );
		printf("%ld %d sleep 1 ...\n", pthread_self(), num );
		sleep(1);
		printf("%ld %d sleep 1 done\n", pthread_self(), num );
		blockLock->writeUnlock( i );
		printf("%ld %d blockLock->writeUnLock(%d) done\n", pthread_self(), num, i );
		fflush( stdout );
	} else {
		printf("%ld %d blockLock->readLock(%d)...\n",  pthread_self(), num, i );
		fflush( stdout );
		blockLock->readLock( i );
		printf("%ld %d blockLock->readLock(%d) done\n", pthread_self(), num, i );
		fflush( stdout );
		blockLock->readUnlock( i );
		printf("%ld %d blockLock->readUnLock(%d) done\n", pthread_self(), num, i );
		fflush( stdout );
	}

	return NULL;

}

/***
void *condLock( void *ptr )
{
	int i;

	TPass *p = (TPass*) ptr;
	JagCondLock *condLock = ( JagCondLock*) p->ptr1;
	int num = p->num;

	if ( (num%5) == 0 ) {
		printf("%lld condLock->setLock()...\n", pthread_self() );
		fflush( stdout );
		condLock->setLock();
		printf("%lld condLock->setLock() done\n", pthread_self() );
		fflush( stdout );
		sleep(3);
		condLock->unlock();
		printf("%lld condLock->unlock() done\n", pthread_self() );
		fflush( stdout );
	} else {
		printf("%lld condLock->checkLock()...\n", pthread_self() );
		fflush( stdout );
		condLock->checkLock();
		printf("%lld condLock->checkLock() done\n", pthread_self() );
		fflush( stdout );
	}
}
***/

int test_misc()
{
	char buf[234];

	char onemax = 255;
	printf("%c %d  %d  %d  %d %d\n", *buf, *buf, *buf == 255, *buf == '\255', *buf == onemax , *buf == -1 );
	Jstr s = "\"thisifdkfd\"";
	Jstr s2 = trimHeadChar(s, '"');
	Jstr s1 = trimTailChar(s, '"');
	Jstr s3 = trimChar(s, '"');
	printf("s=[%s]  nohead=[%s]  notail=[%s]  noheadtail=[%s]\n", s.c_str(), s2.c_str(), s1.c_str(), s3.c_str() );

	s = "\"\"\"";
	s2 = trimHeadChar(s, '"');
	s1 = trimTailChar(s, '"');
	s3 = trimChar(s, '"');
	printf("s=[%s]  nohead=[%s]  notail=[%s]  noheadtail=[%s]\n", s.c_str(), s2.c_str(), s1.c_str(), s3.c_str() );

	JagUUID uuid;
	Jstr str1 = uuid.getStringAt(1);
	printf("uuid=%s\n", str1.c_str() );


	Jstr str = "||a||b|c||";
	JagStrSplit split( str, '|', true );
	for ( int i =0 ; i < split.length(); ++i ) {
		printf("i=%d  strplit=[%s]\n", i, split[i].c_str() );
	}

	str = "12w|";
	JagStrSplit sp2( str, '|' );
	printf("sp2.size=%lld\n", sp2.size() );
	for ( int i=0; i < sp2.size(); ++i ) {
		printf("i=%d [%s]\n", i, sp2[i].s() );
	}
	JagStrSplit sp3( str, '|', true );
	printf("sp3.size=%lld\n", sp3.size() );
	for ( int i=0; i < sp3.size(); ++i ) {
		printf("i=%d [%s]\n", i, sp3[i].s() );
	}

	JagStrSplit sp4( "12w", '|', true );
	printf("sp4.size=%lld\n", sp4.size() );
	for ( int i=0; i < sp4.size(); ++i ) {
		printf("i=%d [%s]\n", i, sp4[i].s() );
	}


	JagClock clock;
	clock.start();
	for ( int i = 0; i < 10000; ++i ) {
		MDString("hellojdjdjd fjfjfjf9393irjfNfne jejejiejeje jckdjffeje jejre ejr ejeiiuer jfdkjfkdj jfd");
	}
	clock.stop();
	printf("s1809 MDString %d times took %lld ms\n", 10000, clock.elapsed() );

	char *md5 = MDString("hello");
	printf("hello ==> md5:  [%s]\n", md5 );
	free( md5 );

	md5 = MDString("hello");
	printf("hello ==> md5:  [%s]\n", md5 );
	free( md5 );

	md5 = MDString("helloworld");
	printf("helloworld ==> md5:  [%s]\n", md5 );
	free( md5 );


	Jstr fdir = "/tmp/a/b/c/d";
	JagFileMgr::makedirPath ( fdir );
	printf("mkdir %s \n", fdir.c_str() );

	pthread_t trd;
	pthread_create( &trd, NULL, &sleep_func, (void*)NULL);

	while ( 1 ) {
		jagsleep( 30, JAG_SEC );
	}
}

void *sleep_func( void *p)
{
	while ( 1 ) {
		jagsleep( 60, JAG_SEC );
	}
	return NULL;

}

void test_stack()
{
	std::stack<int>  ints;

	ints.push(11);
	ints.push(19);
	ints.push(21);
	ints.push(29);
	//ints.print();
	ints.pop();
	//ints.print();

	struct A { int a; };
}

void *task1( void *p){
	printf("Thread #%u working on task1\n", (int)pthread_self());
	fflush( stdout );
	return NULL;
}

void *task2( void *p ){
	printf("Thread #%u working on task2\n", (int)pthread_self());
	fflush( stdout );
	return NULL;
}

/**
void test_threadpool()
{
	puts("Making threadpool with 4 threads");
	threadpool thpool = thpool_init(4);

	puts("Adding 40 tasks to threadpool");
	int i;
	for (i=0; i<20; i++){
		thpool_add_work(thpool, task1, NULL);
		thpool_add_work(thpool, task2, NULL);
	};

	sleep (2);
	puts("Killing threadpool");
	thpool_destroy(thpool);
	
	return;
}
**/

void test_attr()
{
	char buf[32];
	memset( buf, 0, 32 );

	// removexattr ( "/tmp/a.txt", "user.stripe", );
	setxattr ( "/tmp/a.txt", "user.stripe", "38", 2, 0 );
	getxattr ( "/tmp/a.txt", "user.stripe", buf, 32 );

	printf("buf=[%s]\n", buf );

}


void test_blockindex()
{
	printf("new bi\n");
	JagBlock<JagDBPair> *_blockIndex = new JagBlock<JagDBPair>();
	printf("done new bi\n");

	JagDBPair pair;
	JagFixString k;
	//jagint idx;
	
	k = "k5";
	pair = k;
	_blockIndex->updateIndex( pair,  5);
	printf("done k5\n");

	k = "k4";
	pair = k;
	_blockIndex->updateIndex( pair, 4 );
	printf("done k4\n");

	k = "k61";
	pair = k;
	_blockIndex->updateIndex( pair, 21 );
	printf("done k61\n");

	k = "k1";
	pair = k;
	_blockIndex->updateIndex( pair, 1 );
	printf("done k1\n");

	k = "k2";
	pair = k;
	_blockIndex->updateIndex( pair, 2 );
	printf("done k2\n");


	JagFixString min = _blockIndex->getMinKey();
	JagFixString max = _blockIndex->getMaxKey();
	_blockIndex->print();

	printf("min=[%s]  max=[%s]\n", min.c_str(), max.c_str() );
}

void test_fixblockindex()
{
	printf("new bi\n");
	JagFixBlock *_blockIndex = new JagFixBlock( 16 );
	printf("done new bi\n");

	JagDBPair pair;
	JagFixString k;
	//jagint idx;
	
	k = "k5";
	pair = k;
	_blockIndex->updateIndex( pair,  5);
	printf("done k5\n");

	k = "k4";
	pair = k;
	_blockIndex->updateIndex( pair, 4 );
	printf("done k4\n");

	k = "k61";
	pair = k;
	_blockIndex->updateIndex( pair, 21 );
	printf("done k61\n");

	k = "k1";
	pair = k;
	_blockIndex->updateIndex( pair, 1 );
	printf("done k1\n");

	k = "k2";
	pair = k;
	_blockIndex->updateIndex( pair, 2 );
	printf("done k2\n");


	JagFixString min = _blockIndex->getMinKey();
	JagFixString max = _blockIndex->getMaxKey();
	_blockIndex->print();

	printf("min=[%s]  max=[%s]\n", min.c_str(), max.c_str() );

}

void test_compress()
{
	Jstr src = "insert into jbench values ( kkkkk1, 2vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv2 )";
	// Jstr src = "desc t1";

	JagClock clock;
	clock.start();
	int n = 10000;

	Jstr comp, dest;
	for ( int i = 0; i < n; ++i ) {
		JagFastCompress::compress( src, comp );
    	JagFastCompress::uncompress( comp, dest );
	}
	clock.stop();
	printf(" comp/uncomp %d strings took %lld msec\n", n, clock.elapsed() );
	printf("srclen=%ld   --> compressed len=%ld\n", src.size(), comp.size() );

	src = "thsi ishf inf from fjre wheee jefjfj en = jkdi2848jfdf ";
	JagFastCompress::compress( src, comp );
	printf("srclen=%ld   --> compressed len=%ld\n", src.size(), comp.size() );

	src = "thsi ishf inf from fjre whee";
	JagFastCompress::compress( src, comp );
	printf("srclen=%ld   --> compressed len=%ld\n", src.size(), comp.size() );

	src = "thsi ishf inf from fjre whee in th fjdf difufjf  nvnerh fhfeurfhg fdfjdjif djgh";
	JagFastCompress::compress( src, comp );
	printf("srclen=%ld   --> compressed len=%ld\n", src.size(), comp.size() );

	src = "dfkfj fjgkg gjgthsi ishf inf from fjre whee in";
	JagFastCompress::compress( src, comp );
	printf("srclen=%ld   --> compressed len=%ld\n", src.size(), comp.size() );

	src="PYOrg4qTQIbymMSjmQDB4hMRcNQqRwxG,\0sl7GqwxecGMzsCQM,\0stnxK8oVTcjKGQo6,cuMA0hMcXyJn8z9A";
	JagFastCompress::compress( src, comp );
	Jstr uncomp;
	JagFastCompress::uncompress( comp, uncomp );
	printf(" src=[%s] uncomp=[%s]\n", src.c_str(), uncomp.c_str() );
	printf("srclen=%ld   --> compressed len=%ld  ucomlen=%ld\n", src.size(), comp.size(), uncomp.size() );

	src=Jstr("PYOrg4qTQIbymMSjmQDB4hMRcNQqRwxG,NULL...\0sl7GqwxecGMzsCQM,\0stnxK8oVTcjKGQo6,cuMA0hMcXyJn8z9A", 80 );
	JagFastCompress::compress( src, comp );
	JagFastCompress::uncompress( comp, uncomp );
	printf(" src=[%s] uncomp=[%s]\n", src.c_str(), uncomp.c_str() );
	printf("srclen=%ld   --> compressed len=%ld  ucomlen=%ld\n", src.size(), comp.size(), uncomp.size() );



	src="shortstr";
	JagFastCompress::compress( src, comp );
	JagFastCompress::uncompress( comp, uncomp );
	printf(" src=[%s] uncomp=[%s]\n", src.c_str(), uncomp.c_str() );
	printf("srclen=%ld   --> compressed len=%ld  ucomlen=%ld\n", src.size(), comp.size(), uncomp.size() );


	src="s";
	JagFastCompress::compress( src, comp );
	JagFastCompress::uncompress( comp, uncomp );
	printf(" src=[%s] uncomp=[%s]\n", src.c_str(), uncomp.c_str() );
	printf("srclen=%ld   --> compressed len=%ld  ucomlen=%ld\n", src.size(), comp.size(), uncomp.size() );

	src="stu";
	JagFastCompress::compress( src, comp );
	JagFastCompress::uncompress( comp, uncomp );
	printf(" src=[%s] uncomp=[%s]\n", src.c_str(), uncomp.c_str() );
	printf("srclen=%ld   --> compressed len=%ld  ucomlen=%ld\n", src.size(), comp.size(), uncomp.size() );


	Jstr instr;
	Jstr outstr;

	AbaxString rd;
	int len = 10000;
	rd = rd.randomValue( len );
	instr = rd.c_str();
	JagFastCompress::compress( instr, outstr );
	d("%d random bytes compressed to %d byte\n", len, outstr.size() );

}

void test_hashlock()
{
	jagint ps = sysconf( _SC_PAGESIZE );
	jagint np = sysconf( _SC_AVPHYS_PAGES );
	printf("_SC_PAGESIZE=%lld  _SC_AVPHYS_PAGES=%lld\n", ps, np );

	JagHashLock lock;

	JagClock clock;
	clock.start();
	int n = 10000;
	for ( int i =0; i < n; ++i ) {
		lock.writeLock( "a1");
		lock.writeUnlock( "a1");
	}
	clock.stop();
	printf("JagHashLock %d times used %lld millisec\n", n, clock.elapsed() );


	lock.writeLock( "a.a");
	printf("a.a lock obtained\n");

	lock.writeLock( "a.b");
	printf("a.b lock obtained\n");

	lock.readLock( "c.c");
	printf("c.c read lock obtained\n");

	lock.readLock( "-1");
	printf("-1 read lock obtained\n");

	lock.readLock( "a.b");
	printf("a.b read lock obtained\n");

}

void test_size()
{
	JagDBPair pair;

	JagVector<JagDBPair> dbvec;
	dbvec.append(pair);
	int vectot =  sizeof(dbvec) + dbvec.size() * sizeof(JagDBPair);

	printf("size of JagDBPair=%ld  pair.size=%ld sizeof(dbvec)=%ld\n", sizeof(JagDBPair), sizeof(pair), sizeof(dbvec) );
	printf("vectotsize=%d\n", vectot );

	JagFixString key("01234567890123456789", 20 );
	JagFixString val("01234567890123456789", 20 );
	JagDBPair pair2(key, val);
	dbvec.append(pair2);
	vectot =  sizeof(dbvec) + dbvec.size() * sizeof(JagDBPair);
	printf("pair2 size=%ld\n", sizeof( pair2) );

	printf("size of JagDBPair=%ld  pair.size=%ld sizeof(dbvec)=%ld\n", sizeof(JagDBPair), sizeof(pair), sizeof(dbvec) );
	printf("vectotsize=%d\n", vectot );

}


void test_parse()
{
	char cmd[256];
	strcpy( cmd, "this is a [ fjdkjfd ] (dddd,dddd, kdkdd ) sign =; \"jdjdjdjd;ddd[odod]=ddddd\" jfd hhahaha 'hahahahah[dddd] ddd' end" );
	//strcpy( cmd, "this is a (dddd,dddd, kdkdd ) sign ");
	//printf("cmd=[%s]\n", cmd );
    char *saveptr, *gettok;

	/***
    gettok = jag_strtok_r(cmd, " ;", &saveptr );
	printf("111 cmd=[%s]\n", cmd );
	printf("111 tok=[%s] saveptr=[%s]\n", gettok, saveptr );
    while ( gettok ) {
       	gettok = jag_strtok_r(NULL, " ;,[]=", &saveptr );
		printf("tok=[%s] saveptr=[%s]\n", gettok, saveptr );
    }
	***/

	strcpy( cmd, " key: uid int, value: b int default '1', c char(32) default 'SFO', d enum('a  ','b') default 'a' " );
    gettok = jag_strtok_r_bracket(cmd, ",", &saveptr );
	printf("222 cmd=[%s]\n", cmd );
	printf("tok=[%s] saveptr=[%s]\n", gettok, saveptr );
    while ( gettok ) {
       	gettok = jag_strtok_r_bracket(NULL, ",", &saveptr );
		printf("tok=[%s] saveptr=[%s]\n", gettok, saveptr );
    }

	JagVector<JagPolygon> pgvec;
	const char *p = "( ( (0 0, 3 3, 2 2, 9 8, 0 0)), ( (10 0, 38 39, 22 2, 91 87, 10 0)) ) "; 
	JagParser::addMultiPolygonData( pgvec, p, true, false, false );
	pgvec.print();

    Jstr is = "Insert into aaa select aaa aaa from  tab2; ";
    bool isis = JagParser::isInsertSelect( is.s() );
    printf("is=[%s}  isInsertSelect=%d\n", is.s(), isis );

    is = "Insert into aaa values ('aaa', 'ffff') ; ";
    isis = JagParser::isInsertSelect( is.s() );
    printf("is=[%s}  isInsertSelect=%d\n", is.s(), isis );

}
   

void test_boundfile()
{
	JagBoundFile bf ( "/tmp/bound.txt", 5 );
	bf.openAppend();
	bf.appendLine( "line 1");
	bf.appendLine( "line 2");
	bf.appendLine( "line 3");
	bf.appendLine( "line 4");
	bf.appendLine( "line 5");
	bf.appendLine( "line 6");
	bf.appendLine( "line 7");
	bf.appendLine( "line 8");
	bf.close();
	
	bf.openRead();
	JagVector<Jstr> vec;
	bf.readLines( 20, vec );
	for ( int i = 0; i < vec.length(); ++ i ) {
		printf("s1922 line=[%s]\n", vec[i].c_str() );
	}

}

void test_localdiskhash( int N )
{
	int klen = 10;
	int vlen = 20;
	JagLocalDiskHash dh( "test", klen, vlen );

	AbaxString k, v;
	JagFixString  key, val;
	int rc;
	srand( 0 );
	int num = 0;
	for ( int i = 0; i < N; ++i ) {
		k = AbaxString::randomValue( klen );
		// printf("insert key=[%s]\n", k.c_str() );
		v = AbaxString::randomValue( vlen );
		JagFixString key( k.c_str(), klen );
		JagFixString val( v.c_str(), vlen );
		JagDBPair pair( key, val);

		rc = dh.insert( pair );
		if ( rc ) {
			++num;
			// dh.print();
		}
	}
	printf("inserted num=%d\n", num );
	//dh.print();

	srand( 0 );
	num = 0;
	for ( int i = 0; i < N; ++i ) {
		k = AbaxString::randomValue( klen );
		//printf("read key=[%s]\n", k.c_str() );
		v = AbaxString::randomValue( vlen );
		JagFixString key( k.c_str(), klen );
		JagDBPair pair( key );
		rc = dh.get( pair );
		if ( rc ) {
			++num;
		} else {
			printf("s3848 *** key=[%s] not found\n", k.c_str() );
			// dh.print();
		}
	}
	printf("read num=%d\n", num );

	srand( 0 );
	num = 0;
	for ( int i = 0; i < N; ++i ) {
		k = AbaxString::randomValue( klen );
		v = AbaxString::randomValue( vlen );
		v = "00000000000000000000009292929999****************929293939339";
		JagFixString key( k.c_str(), klen );
		JagFixString val( v.c_str(), vlen );
		JagDBPair pair( key, val);
		rc = dh.set( pair );
		if ( rc ) {
			++num;
		} else {
			printf("s3848 *** key=[%s] not set\n", k.c_str() );
			// dh.print();
		}
	}
	printf("update num=%d\n", num );
	//dh.print();

	printf("Start remove ...****\n");
	srand( 0 );
	num = 0;
	for ( int i = 0; i < N; ++i ) {
		k = AbaxString::randomValue( klen );
		v = AbaxString::randomValue( vlen );
		JagFixString key( k.c_str(), klen );
		JagFixString val( v.c_str(), vlen );
		JagDBPair pair( key, val);
		// printf("s2239 remove key=[%s] ...\n", k.c_str() );
		rc = dh.remove( pair );
		if ( rc ) {
			++num;
		} else {
			printf("s3848 *** key=[%s] not del\n", k.c_str() );
		}
		// dh.print();
	}
	printf("del num=%d \n", num );

	dh.print();

}

/*****
void test_diskarrayclient( int N )
{
	Jstr fpath = Jstr(getenv("HOME")) + "/commit/DiskArrayClient";
	JagSchemaRecord record;
	int KLEN = 16;
	int VLEN = 32;
	Jstr s = record.makeSimpleSchema( "uid", 16, "addr", 32 );
	record.parseRecord( s.c_str() );
	JagDiskArrayClient *darr = new JagDiskArrayClient( fpath, &record);

	JagDBPair retpair;
	char buf[KLEN+VLEN+1];
	memset( buf, 0, KLEN+VLEN+1 );
	int insertCode;
	AbaxString k, v;

	for ( int i = 0; i < N; ++i ) {
		k = AbaxString::randomValue( KLEN );
		v = AbaxString::randomValue( VLEN );
		memcpy( buf, k.c_str(), KLEN );
		memcpy( buf+KLEN, v.c_str(), VLEN );
		JagDBPair tpair(buf, KLEN, buf+KLEN, VLEN, false );
		darr->insertSync(tpair);
	}

	darr->flushInsertBufferSync();
	d("Done insert %d rows\n", N );

    JagSingleBuffReader navig( darr->getFD(), darr->_arrlen, KLEN, VLEN );
	jagint i;
	jagint cnt=0;
    while ( navig.getNext( buf, KLEN+VLEN, i ) ) {
		++cnt;
	}
	d("Done read %d rows\n", cnt );

}
****/


void test_jagarray( int N )
{
	Jstr s1, s2;

	printf("test_jagarray N=%d\n", N );
	JagArray<AbaxPair<AbaxInt, AbaxInt> > jar;
	AbaxInt k,v;
	for ( long i = 0; i < N; ++i ) {
		//k = AbaxInt::randomValue();
		k = (329187 * i) % 2739971;
		v =  100;
		AbaxPair<AbaxInt, AbaxInt> pair(k, v );
		jar.insert( pair );
	}
	printf("JagArray has %lld elements   %lld cells\n", jar.elements(), jar.size() );

	int cnt = 0;
	for ( long i = 0; i < N; ++i ) {
		k = (329187 * i) % 2739971;
		v = 100;
		AbaxPair<AbaxInt, AbaxInt> pair(k, v );
		if ( jar.exist( pair ) ) {
			++cnt;
		}
	}
	printf("JagArray found %d elements\n", cnt );
}

void test_simpleboundedarray( int N )
{
	JagSimpleBoundedArray<AbaxPair<AbaxInt, AbaxInt> > jar(10);

	AbaxInt k,v;
	for ( int i = 0; i < N; ++i ) {
		k = i;
		v =  999;
		AbaxPair<AbaxInt, AbaxInt> pair(k, v );
		jar.insert( pair );
	}

	jar.begin();
	AbaxPair<AbaxInt, AbaxInt> pair;
	while ( jar.next( pair ) ) {
		printf("*** key=%d  val=%d\n", pair.key.value(), pair.value.value() );
	}

}

void test_malloc( int N )
{
	d("t3829 test_malloc ...\n");
	jagint len = 2000000000;
	char *p = (char*)malloc( len );
	memset( p, 0, len );
	free( p );
	d("t3829 test_malloc done len=%lld \n", len );
}


void test_filefamily()
{
	Jstr fullpath = "/tmp";
	Jstr fam = JagFileMgr::getFileFamily( 1, fullpath, "t123.jdb" );
	d("fam=[%s]\n", fam.c_str() );

	fam = JagFileMgr::getFileFamily( 2,  fullpath, "t123.idx.jdb" );
	d("fam=[%s]\n", fam.c_str() );

}

void test_sqlmerge_reader()
{
	Jstr files = "a1.sql|a2.sql|a3.sql";
	JagSQLMergeReader mrdr( files );
	Jstr sql;

	while ( mrdr.getNextSQL( sql ) ) {
		d("sql=[%s]\n", sql.c_str() );
	}
}

void test_escape()
{
	Jstr instr = "hihi\n2222\n333\n\n";
	Jstr out;
	escapeNewline( instr, out );
	printf("instr=[%s]\n", instr.c_str() );
	printf("out=[%s]\n", out.c_str() );
}

/***
void test_keychecker( int N )
{
	printf("Begin test_keychecker: N=%d\n", N );
	char vbuf[3]; 
	vbuf[0] = '2';
	vbuf[1] = '3';
	vbuf[2] = '\0';
	JagFixString vstr(  vbuf, 2, 1 ); // point to vbuf

   	JagKeyChecker * _keyChecker = new JagKeyChecker();
   	for ( int i = 0; i < N; ++i ) {
   		AbaxString str  = AbaxString::randomValue(30);
   		JagFixString k  = Jstr( str.c_str() );
   		_keyChecker->addKeyValue( k, vstr );
   	}

	printf("Done test_keychecker elems=%d.\n", _keyChecker->size() );
	fflush( stdout );
	sleep(20);

	delete _keyChecker;
	jagmalloc_trim( 0 );
	printf("delete test_keychecker\n" );
	fflush( stdout );
	sleep(20);
}
***/

void test_map ( int N )
{
	JagDBMap *jmap = new JagDBMap();

	AbaxString k, v;
	JagFixString key("k1");
	JagFixString val("v1");
	JagFixString fk, fv;

	time_t t1 = time(NULL);
	v = AbaxString::randomValue(3000);
	for ( int i = 0; i < N; ++i ) {
		k = AbaxString::randomValue(48);
		fk = k.c_str(); fv = v.c_str();
		JagDBPair pair(fk, fv );
		jmap->insert(pair);
	}
	time_t t2 = time(NULL);
	printf("Insert map %d done in %ld secs\n", N, t2-t1 );
	sleep( 10 );

}

void test_veconly ( int N )
{
	AbaxString k, v;
	JagFixString key("k1");
	JagFixString val("v1");
	JagFixString fk, fv;
	char buf[12];
	std::vector<JagFixString> *vec = new std::vector<JagFixString>(20000);

	time_t t1 = time(NULL);
	v = AbaxString::randomValue(3000);
	for ( int i = 0; i < N; ++i ) {
		k = AbaxString::randomValue(48);
		fk = k.c_str(); fv = v.c_str();

		vec->push_back( fv );
		sprintf( buf, "%d", i );
		fv = buf;
	}
	time_t t2 = time(NULL);
	printf("Insert vec %d done in %ld secs\n", N, t2-t1 );
	sleep( 10 );

}
void test_mapvec ( int N )
{
	JagDBMap *jmap = new JagDBMap();

	AbaxString k, v;
	JagFixString key("k1");
	JagFixString val("v1");
	JagFixString fk, fv;
	char buf[12];
	std::vector<JagFixString> *vec = new std::vector<JagFixString>(20000);


	time_t t1 = time(NULL);
	v = AbaxString::randomValue(3000);
	for ( int i = 0; i < N; ++i ) {
		k = AbaxString::randomValue(48);
		fk = k.c_str(); fv = v.c_str();
		vec->push_back( fv );

		sprintf( buf, "%d", i );
		fv = buf;

		JagDBPair pair(fk, fv );
		jmap->insert(pair);
	}
	time_t t2 = time(NULL);
	printf("Insert mapvec %d done in %ld secs\n", N, t2-t1 );
	sleep( 10 );

}

void test_lineseg ( int N )
{
	JagArray<JagLineSeg2DPair> *arr = new JagArray<JagLineSeg2DPair>(32 );
	AbaxString k, v;
	double x1,y1,x2,y2;
	JagSortPoint2D *points = new JagSortPoint2D[2*N];
	JagClock clock;
	int cnt = 0;
	for  ( int i=0; i < 2*N; ++i ) {
		 x1 = rand() % 1000;
		 y1 = rand() % 1000;
		 x2 = x1 + 100;
		 y2 = y1 + 100;
		 points[i].x1 = x1; points[i].y1 = y1;
		 points[i].x2 = x2; points[i].y2 = y2;
	}

	JagLineSeg2DPair seg; 
	seg.value = '1';
	for ( int i=0; i < N; ++i ) {
		seg.key.x1 =  points[i].x1; seg.key.y1 =  points[i].y1;
		seg.key.x2 =  points[i].x2; seg.key.y2 =  points[i].y2;
		arr->insert(seg);
	}

	//arr->printKey();

	const JagLineSeg2DPair *pp;
	const JagLineSeg2DPair *pp2;
	clock.start();
	cnt = 0;
	int cp = 0;
	for ( int i = 0; i < N; ++i ) {
		seg.key.x1 =  points[i].x1; seg.key.y1 =  points[i].y1;
		seg.key.x2 =  points[i].x2; seg.key.y2 =  points[i].y2;
		pp = arr->getPred( seg );
		if (! pp ) {
			d("s3098 seg has no pred\n"  );
			seg.println();
			continue;
		}
		++cp;
		pp2 = arr->getSucc( *pp );
		if (  pp2 && ( seg == *pp2 ) ) {
			++cnt;
		}
	}
	clock.stop();
	printf("Get array predfound=%d  pred->succ=self=%d  done in %lld millissecs\n", cp, cnt, clock.elapsed() );

	clock.start();
	cnt = 0;
	cp = 0;
	for ( int i = 0; i < N; ++i ) {
		seg.key.x1 =  points[i].x1; seg.key.y1 =  points[i].y1;
		seg.key.x2 =  points[i].x2; seg.key.y2 =  points[i].y2;
		pp = arr->getSucc( seg );
		if (! pp ) {
			d("s3498 seg has no succ\n"  );
			seg.println();
			continue;
		}
		++cp;
		pp2 = arr->getPred( *pp );
		if (  pp2 && ( seg == *pp2 ) ) {
			++cnt;
		}
	}
	clock.stop();
	printf("Get array succfound=%d  succ->pred=self=%d  done in %lld millissecs\n", cp, cnt, clock.elapsed() );



	clock.start();
	cnt = 0;
	cp = 0;
	for ( int i = 0; i < N; ++i ) {
		seg.key.x1 =  points[i].x1; seg.key.y1 =  points[i].y1;
		seg.key.x2 =  points[i].x2; seg.key.y2 =  points[i].y2;
		arr->remove( seg );
	}
	clock.stop();
	printf("Remove done in %lld millissecs\n", clock.elapsed() );

	clock.start();
	cnt = 0;
	cp = 0;
	for ( int i = 0; i < N; ++i ) {
		seg.key.x1 =  points[i].x1; seg.key.y1 =  points[i].y1;
		seg.key.x2 =  points[i].x2; seg.key.y2 =  points[i].y2;
		if ( arr->exist( seg ) ) ++cnt;
	}
	clock.stop();
	printf("Exist done remain=%d in %lld millissecs\n", cnt, clock.elapsed() );

	printf("\n");
	delete arr;
	delete [] points;

}

void test_array ( int N )
{
	printf("\ntest_array ...\n");
	//JagArray<JagDBPair> *arr = new JagArray<JagDBPair>(32, 1);
	JagArray<JagDBPair> *arr = new JagArray<JagDBPair>(32, 0);
	AbaxString k, v;
	JagFixString fk, fv;
	JagDBPair pair;
	JagClock clock;

	std::vector<AbaxString> *vec = new std::vector<AbaxString>(N);
	int cnt = 0;

	for  ( int i=0; i < N; ++i ) {
		(*vec)[i] = AbaxString::randomValue(32);
	}
	clock.start();
	for ( int i = 0; i < N; ++i ) {
		fk = (*vec)[i].c_str(); 
		fv = fk;
		pair.key = fk;
		pair.value = fv;
		arr->insert(pair);
	}
	clock.stop();
	printf("Array random insert %d items done in %lld millisecs\n", N, clock.elapsed() );
	//arr->print( false );

	clock.start();
	cnt = 0;
	for ( int i = 0; i < N; ++i ) {
		fk = (*vec)[i].c_str(); 
		fv = fk;
		pair.key = fk;
		pair.value = fv;
		if ( arr->exist( pair ) ) ++cnt;
	}
	clock.stop();
	printf("Seach array random exist=%d done in %lld millisecs\n", cnt, clock.elapsed() );

	clock.start();
	cnt = 0;
	for ( int i = 0; i < N; ++i ) {
		fk = (*vec)[i].c_str(); 
		pair.key = fk;
		pair.value = fv;
		if ( arr->get( pair ) ) ++cnt;
	}
	clock.stop();
	printf("Get array random get=%d done in %lld millissecs\n", cnt, clock.elapsed() );

	const JagDBPair *pp;
	const JagDBPair *pp2;
	clock.start();
	cnt = 0;
	int cp = 0;
	for ( int i = 0; i < N; ++i ) {
		fk = (*vec)[i].c_str(); 
		pair.key = fk;
		pair.value = fv;
		pp = arr->getPred( pair );
		if (! pp ) {
			d("s3098 pair.key=%s has no pred\n", pair.key.c_str() );
			continue;
		}
		++cp;
		//d("pred=[%s] <--- pair=[%s]\n", pp->key.c_str(), pair.key.c_str() );
		pp2 = arr->getSucc( *pp );
		if (  pp2 && ( pair == *pp2) ) {
			++cnt;
		}
	}
	clock.stop();
	printf("Get array predfound=%d  pred->succ=self=%d  done in %lld millisecs\n", cp, cnt, clock.elapsed() );

	clock.start();
	cnt = 0;
	cp = 0;
	for ( int i = 0; i < N; ++i ) {
		fk = (*vec)[i].c_str(); 
		pair.key = fk;
		pair.value = fv;
		pp = arr->getSucc( pair );
		if (! pp ) {
			d("s3038 pair.key=%s has no succ\n", pair.key.c_str() );
			continue;
		}
		++cp;
		//d("pair=[%s] --> succ=[%s]\n", pair.key.c_str(), pp->key.c_str() );

		pp2 = arr->getPred( *pp );
		if (  pp2 && ( pair == *pp2) ) {
			++cnt;
		}
	}
	clock.stop();
	printf("Get array succfound=%d  succ->pred=self=%d  done in %lld millisecs\n", cp, cnt, clock.elapsed() );

	clock.start();
	cnt = 0;
	cp = 0;
	for ( int i = 0; i < N; ++i ) {
		fk = (*vec)[i].c_str(); 
		pair.key = fk;
		pair.value = fv;
		if ( arr->exist( pair ) ) ++cnt;
	}
	clock.stop();
	printf("Exist done remain=%d in %lld millisecs\n", cnt, clock.elapsed() );

	clock.start();
	cnt = 0;
	cp = 0;
	for ( int i = 0; i < N; ++i ) {
		fk = (*vec)[i].c_str(); 
		pair.key = fk;
		pair.value = fv;
		arr->remove( pair );
	}
	clock.stop();
	printf("Remove done in %lld millisecs\n", clock.elapsed() );

	clock.start();
	cnt = 0;
	cp = 0;
	for ( int i = 0; i < N; ++i ) {
		fk = (*vec)[i].c_str(); 
		pair.key = fk;
		pair.value = fv;
		if ( arr->exist( pair ) ) ++cnt;
	}
	clock.stop();
	printf("Exist done remain=%d in %lld millisecs\n", cnt, clock.elapsed() );
	printf("\n");


	// sequential insert
	delete vec;
	vec = new std::vector<AbaxString>(N);
	for  ( int i=0; i < N; ++i ) {
		(*vec)[i] = intToStr(i);
	}

	clock.start();
	for ( int i = 0; i < N; ++i ) {
		fk = (*vec)[i].c_str(); 
		fv = fk;
		pair.key = fk;
		pair.value = fv;
		arr->insert(pair);
	}
	clock.stop();
	printf("Array sequential insert %d done in %lld millisecs\n", N, clock.elapsed() );
	//arr->print( false );

	clock.start();
	cnt = 0;
	for ( int i = 0; i < N; ++i ) {
		fk = (*vec)[i].c_str(); 
		pair.key = fk;
		pair.value = fv;
		if ( arr->get( pair ) ) ++cnt;
	}
	clock.stop();
	printf("Array sequential get %d done in %lld millissecs\n", cnt, clock.elapsed() );

	printf("\n");
	delete arr;
	delete vec;
}

void test_jaghashmap ( int N )
{
	JagClock clock;
	clock.start();
	JagHashMap<AbaxString, JagFixString> *map = new JagHashMap<AbaxString,JagFixString>();
	AbaxString rs;
	JagFixString v("aa", 2, 1 );
	for ( int i = 0; i < N; ++i ) {
		rs = rs.randomValue(20);
		map->addKeyValue(rs, v );
	}
	clock.stop();
	printf("%d  JagHashMap(addKeyValue)=%lld msec \n", N,  clock.elapsed() );
	sleep(10);

	delete map;
	jagmalloc_trim( 0 );
	printf("done delete\n");
	sleep(10);
}

void test_jaghashmap_nofix ( int N )
{
	JagClock clock;
	clock.start();
	JagHashMap<AbaxString, Jstr> *map = new JagHashMap<AbaxString,Jstr>();
	AbaxString rs;
	Jstr v("aa");
	for ( int i = 0; i < N; ++i ) {
		rs = rs.randomValue(20);
		map->addKeyValue(rs, v );
	}
	clock.stop();
	printf("%d  JagHashMap_nofix(addKeyValue)=%lld msec \n", N,  clock.elapsed() );
	sleep(10);

	delete map;
	jagmalloc_trim( 0 );
	printf("done delete\n");
	sleep(10);
}

/*******
void test_jagunordmap ( int N )
{
	JagClock clock;
	clock.start();
	JagUnordMap<Jstr, JagFixString> *map = new JagUnordMap<Jstr, JagFixString>();
	AbaxString rs;
	JagFixString v("aa", 2, 1 );
	for ( int i = 0; i < N; ++i ) {
		rs = rs.randomValue(20);
		map->addKeyValue( rs.c_str(), v );
	}
	clock.stop();
	printf("%d  JagUnordMap(addKeyValue)=%d msec \n", N,  clock.elapsed() );
	sleep(10);

	delete map;
	jagmalloc_trim( 0 );
	printf("done delete\n");
	sleep(10);
}

void test_jagunordmap_nofix ( int N )
{
	JagClock clock;
	clock.start();
	JagUnordMap<Jstr, Jstr> *map = new JagUnordMap<Jstr, Jstr>();
	AbaxString rs;
	Jstr v("aa");
	for ( int i = 0; i < N; ++i ) {
		rs = rs.randomValue(20);
		map->addKeyValue( rs.c_str(), v );
	}
	clock.stop();
	printf("%d  JagUnordMap_nofix(addKeyValue)=%d msec \n", N,  clock.elapsed() );
	sleep(10);

	delete map;
	jagmalloc_trim( 0 );
	printf("done delete\n");
	sleep(10);
}
*****/


/*****
void test_stdhashmap ( int N )
{
	JagClock clock;
	clock.start();
	std::unordered_map<Jstr,JagFixString> *map = new std::unordered_map< Jstr,JagFixString>();
	AbaxString rs;
	JagFixString v("aa", 2, 1 );
	for ( int i = 0; i < N; ++i ) {
		rs = rs.randomValue(20);
		map->insert( std::make_pair<Jstr,JagFixString>( Jstr(rs.c_str()), v) );
	}
	clock.stop();
	printf("%d  StdHashMap(addKeyValue)=%d msec \n", N,  clock.elapsed() );
	sleep(10);

	delete map;
	jagmalloc_trim( 0 );
	printf("done delete\n");
	sleep(10);
}
*****/

void test_stdmap ( int N )
{
	printf("\ntest_stdmap ...");
	JagClock clock;
	std::map<std::string,std::string> *map = new std::map< std::string,std::string>();
	std::string rs;
	clock.start();
	std::vector<std::string> *vec = new std::vector<std::string>(N);
	for ( int i = 0; i < N; ++i ) {
		rs = AbaxString::randomValue(20).c_str();
		//rs = std::to_string(i);
		(*vec)[i] = rs.c_str();
		map->insert( std::pair<std::string,std::string>( rs.c_str(), rs.c_str() ) );
	}
	clock.stop();
	printf("%d  StdMap(insert) used %lld milliseconds \n", N,  clock.elapsed() );


	clock.start();
	int cnt = 0;
	for ( int i = 0; i < N; ++i ) {
		if ( map->find( (*vec)[i] ) != map->end() ) ++cnt;
	}
	clock.stop();
	printf("%d  StdMap(find) used %lld millseconds\n", cnt,  clock.elapsed() );


	std::map< std::string,std::string>::iterator it;
	clock.start();
	int tot = 0;
	for ( int i=0; i < 100; ++i ) {
		for ( it =  map->begin(); it != map->end(); ++it ) { ++tot; }
	}
	clock.stop();
	printf("%d  StdMap iterator 100 times used %lld millseconds\n", tot, clock.elapsed() );

	delete map;
	delete vec;
	jagmalloc_trim( 0 );
	printf("done delete\n");
}

void test_stdmap_mutex ( int N )
{
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_init( &mutex, NULL );
	printf("\ntest_stdmap_mutex ...");
	JagClock clock;
	std::map<std::string,std::string> *map = new std::map< std::string,std::string>();
	std::string rs;
	clock.start();
	std::vector<std::string> *vec = new std::vector<std::string>(N);
	for ( int i = 0; i < N; ++i ) {
		rs = std::to_string(i);
		(*vec)[i] = rs.c_str();
		//rs = AbaxString::randomValue(20).c_str();
		pthread_mutex_lock ( &mutex );
		map->insert( std::pair<std::string,std::string>( rs.c_str(), rs.c_str() ) );
		pthread_mutex_unlock ( &mutex );
	}
	clock.stop();
	printf("%d  mutex StdMap(insert) used %lld milliseconds \n", N,  clock.elapsed() );


	clock.start();
	int cnt = 0;
	for ( int i = 0; i < N; ++i ) {
		pthread_mutex_lock ( &mutex );
		if ( map->find( (*vec)[i] ) != map->end() ) ++cnt;
		pthread_mutex_unlock ( &mutex );
	}
	clock.stop();
	printf("%d  mutex StdMap(find) used %lld millseconds\n", cnt,  clock.elapsed() );

	delete map;
	delete vec;
	jagmalloc_trim( 0 );
	printf("done delete\n");
	pthread_mutex_destroy( &mutex );
}

void test_jagfixhasharray ( int N )
{
	JagFixHashArray *map = new JagFixHashArray(20, 2);
	Jstr save[N];

	JagClock clock;
	clock.start();
	AbaxString rs;
	char rec[22];
	for ( int i = 0; i < N; ++i ) {
		rs = rs.randomValue(20);
		// map->addKeyValue( rs.c_str(), v );
		memcpy( rec, rs.c_str(), 20 );
		memcpy( rec+20, "aa", 2 );
		map->insert( rec );
		save[i] =  rs.c_str();
	}
	clock.stop();
	printf("%d  fixhasharray(insert)=%lld msec \n", N,  clock.elapsed() );
	sleep(10);

	jagint idx;
	jagint cnt = 0;
	for ( int i = 0; i < N; ++i ) {
		if ( map->exist( save[i].c_str(), &idx ) ) {
			++cnt;
		}
	}
	printf("Found %lld elements\n", cnt );

	rs = rs.randomValue(20);
	if (  map->exist( rs.c_str(), &idx ) ) {
		printf("found rnd str\n");
	} else {
		printf("not found rnd str\n");
	}

	rs = rs.randomValue(20);
	if (  map->exist( rs.c_str(), &idx ) ) {
		printf("found rnd str\n");
	} else {
		printf("not found rnd str\n");
	}

	char buf[23];
	memcpy(buf, save[2].c_str(), 20 );
	buf[22] = '\0';
	if (  map->get( buf ) ) {
		printf("found save2 str [%s]\n", buf );
	} else {
		printf("not found save2 str [%s]\n", buf);
	}

	delete map;
	jagmalloc_trim( 0 );
	printf("done delete\n");
	sleep(10);

}

void test_diskkeychecker( int N )
{
	int klen = 10;
	int vlen = 20;
	JagDiskKeyChecker dh( "test", klen, vlen );

	char kv[klen+JAG_KEYCHECKER_VLEN+1];
	char value[JAG_KEYCHECKER_VLEN+1];

	AbaxString k, v;
	JagFixString  key, val;
	int rc;
	srand( 0 );
	int num = 0;
	for ( int i = 0; i < N; ++i ) {
		k = AbaxString::randomValue( klen );
		v = "12";
		memset( kv, 0, klen+JAG_KEYCHECKER_VLEN+1 );
		sprintf(kv, k.c_str(), klen );
		sprintf(kv+JAG_KEYCHECKER_KLEN, v.c_str(), JAG_KEYCHECKER_VLEN );

		rc = dh.addKeyValue(kv);
		if ( rc ) {
			++num;
		}
	}
	printf("inserted num=%d\n", num );

	srand( 0 );
	num = 0;
	for ( int i = 0; i < N; ++i ) {
		k = AbaxString::randomValue( klen );
		v = "12";
		memset( kv, 0, klen+JAG_KEYCHECKER_VLEN+1 );
		sprintf(kv, k.c_str(), klen );
		sprintf(kv+JAG_KEYCHECKER_KLEN, v.c_str(), JAG_KEYCHECKER_VLEN );
		rc = dh.getValue( k.c_str(), value );
		if ( rc ) {
			++num;
		} else {
			printf("s3848 *** key=[%s] not found\n", k.c_str() );
			// dh.print();
		}
	}
	printf("read num=%d\n", num );

	printf("Start remove ...****\n");
	srand( 0 );
	num = 0;
	for ( int i = 0; i < N; ++i ) {
		k = AbaxString::randomValue( klen );
		rc = dh.removeKey( k.c_str() );
		if ( rc ) {
			++num;
		} else {
			printf("s3848 *** key=[%s] not del\n", k.c_str() );
		}
		// dh.print();
	}
	printf("del num=%d \n", num );

}


void test_sys_getline()
{
	FILE *fp = fopen("getline.txt", "r" );
	if ( ! fp ) {
		printf("error open getline.txt\n" );
		return;
	}

	char *cmdline = NULL;
	size_t  sz=0;

	while ( 1 ) {
        if ( getline( (char**)&cmdline, &sz, fp ) == -1 ) {
            break;
        }
		printf("sz=%ld %s", sz, cmdline );
		if ( cmdline ) free( cmdline );
		cmdline = NULL;
	}

	printf("cmdline=[%s]\n", cmdline );
	if ( cmdline ) {
		printf("free cmdline\n");
		free( cmdline );
	}
	fclose( fp );
}

void test_jag_getline()
{
	FILE *fp = fopen("getline.txt", "r" );
	if ( ! fp ) {
		printf("error open getline.txt\n" );
		return;
	}

	char *cmdline = NULL;
	size_t  sz=0;
	while ( 1 ) {
        if ( jaggetline( (char**)&cmdline, &sz, fp ) == -1 ) {
            break;
        }
		printf("sz=%ld %s", sz, cmdline );
		free( cmdline );
		cmdline = NULL;
	}

	printf("cmdline=[%s]\n", cmdline );

	if ( cmdline ) {
		printf("free cmdline\n");
		free( cmdline );
	}
	fclose( fp );
}

void test_reply ( const char *inputFile )
{
	/***
	JagVector<Jstr> xvec, yvec;
	Jstr xname, yname;
	bool xisvalue;
	readInputFile( inputFile, xname, yname, xvec, yvec, xisvalue );
	for ( int i = 0; i < xvec.length(); ++i ) {
		printf("xvec i=%d  xvec[i]=[%s]\n", i, xvec[i].c_str() );
	}
	***/
}

void test_str( int N ) 
{
    Jstr js1;
    js1 += 236;
    js1 += 200;
    js1 += 100;

    Jstr js2;
    js2 += 246;
    js2 += 200;
    js2 += 100;

    int rcc = memcmp(js1.s(), js2.s(), js1.size() );
    printf("js1 236  cmp? js2 246 rcc=%d\n", rcc );


    /**
    s1.0=[49]
    s1.1=[50]
    s1.2=[235]
    s1.3=[199]
    s1.4=[100]
    **/

	const char *ss1 = "timeseries(30m)   test.ptr (    key:    pickup  datetimesec,    t char(1),    value:    a int ";
	bool ba1 = endWithSQLRightBra( ss1 );
	printf("endWithSQLRightBra ss1=[%s] a1=[%d]\n", ss1, ba1 );

	unsigned char s[4]; s[0] = 23; s[1] = 12; s[2] = 32; s[3] = 99;
	std::string s1 = "0123456";
	size_t f1 = s1.find(s[3]);
	printf("std::string find(2) ==> [%ld]\n", f1 );
	for ( int i=0; i < s1.size(); ++i ) {
		printf("%c ", s1[i] );
	}
	printf("\n" );

	//std::string ns(NULL);
	//printf("null=[%s]", ns.c_str() );
	std::string ns2;
	printf("ns2 null=[%s]\n", ns2.c_str() );


	AbaxCStr s2 = "0123456";
	size_t f2 = s2.find(s[3]);
	printf("AbaxCStr find(2) ==> [%ld]\n", f2 );
	for ( int i=0; i < s2.size(); ++i ) {
		printf("%c ", s2[i] );
	}
	printf("\n" );

	Jstr ss = "hello worldidifjdjdjfjd 0jjfdjfd hahah";
	Jstr enc1 =  abaxEncodeBase64( ss );
	Jstr ss2 =  abaxDecodeBase64( enc1 );
	d("ss=[%s]\n", ss.c_str() );
	d("base64 enc-dec: ss2=[%s]\n", ss2.c_str() );

	char buf[100];
	strcpy(buf, "1.0" );
	printf("buf=[%s]\n", buf );
	stripTailZeros( buf, 3 );
	printf("buf=[%s]\n", buf );

	//jagfwritefloat(buf, 3, stdout );
	writefloat(buf, 3, stdout );
	printf("done\n" );


	const char *str1 = "hello world, welcome to hjhj, ffd 3939 kdkd";
	const char *p = KMPstrstr(str1, "hjhj" );
	d("hjhj=[%s]\n", p );

	p = KMPstrstr(str1, "" );
	d("hjhj=[%s]\n", p );

	p = KMPstrstr(str1, "come" );
	d("come=[%s]\n", p );

	p = KMPstrstr("aa", "come" );
	d("come=[%s]\n", p );

	p = KMPstrstr("come", "come" );
	d("come=[%s]\n", p );

	Jstr h1= "jaaj ";
	Jstr h2= "xxxxx ";
	Jstr h3 = h1 + "|" + h2;
	d("h3=[%s]\n", h3.c_str() );

	JagClock clock;
	clock.start();
	for ( int i=0; i < N; ++i ) {
		Jstr h = "903jjfkdjf kfdjfjd fdjf94jff jjfk djf df fjdjfdjf djf dj  fjd fjdkkfjdjf9rjfkdjfdk fjdkfjdf df d";
		Jstr d = "jdjdjfkd fjdkfjdjfdkfjdkjfd jkfdfjdjfdjfkdjfk djfdkjfkdjkfdjkfjdkjf djfkdjkf dkfjdk fkdjfkdj fdj fd";
		Jstr s = h + d;
		Jstr g;
		g = h;
	}
	clock.stop();
	d("Jstr init + = tool %lld millisec\n", clock.elapsed() );

	clock.start();
	for ( int i=0; i < N; ++i ) {
		std::string h = "903jjfkdjf kfdjfjd fdjf94jff jjfk djf df fjdjfdjf djf dj  fjd fjdkkfjdjf9rjfkdjfdk fjdkfjdf df d";
		std::string d = "jdjdjfkd fjdkfjdjfdkfjdkjfd jkfdfjdjfdjfkdjfk djfdkjfkdjkfdjkfjdkjf djfkdjkf dkfjdk fkdjfkdj fdj fd";
		std::string s = h + d;
		std::string g;
		g = h;
	}
	clock.stop();
	d("std::string init + = tool %lld millisec\n", clock.elapsed() );


	Jstr  c1 = "0000000000003.45000000000";
	Jstr  a2 = "3.0";
	Jstr  a3 = "4";
	d("a1=[%s] a1len=%d a2=[%s] a3=[%s]\n", c1.c_str(), c1.size(), a2.c_str(), a3.c_str() );
	c1.trimEndZeros();
	a2.trimEndZeros();
	a3.trimEndZeros();
	d("after trimend0 a1=[%s] a1len=%d a2=[%s] a3=[%s]\n", c1.c_str(), c1.size(), a2.c_str(), a3.c_str() );

	Jstr q1 = "all(ls )";
	Jstr nm = q1.substrc('(', ')');
	nm = trimChar( nm, ' ' );
	d("q1=[%s]  (*)=[%s]\n", q1.c_str(), nm.c_str() );

	q1 = "all(ls";
	nm = q1.substrc('(', ')');
	nm = trimChar( nm, ' ' );
	d("q1=[%s]  (*)=[%s]\n", q1.c_str(), nm.c_str() );

	q1 = "all( ls)";
	nm = q1.substrc('(', ')');
	nm = trimChar( nm, ' ' );
	d("q1=[%s]  (*)=[%s]\n", q1.c_str(), nm.c_str() );

	q1 = " ls)";
	nm = q1.substrc('(', ')');
	nm = trimChar( nm, ' ' );
	d("q1=[%s]  (*)=[%s]\n", q1.c_str(), nm.c_str() );

	q1 = "(";
	nm = q1.substrc('(', ')');
	nm = trimChar( nm, ' ' );
	d("q1=[%s]  (*)=[%s]\n", q1.c_str(), nm.c_str() );

	q1 = ")";
	nm = q1.substrc('(', ')');
	nm = trimChar( nm, ' ' );
	d("q1=[%s]  (*)=[%s]\n", q1.c_str(), nm.c_str() );

	q1 = "()";
	nm = q1.substrc('(', ')');
	nm = trimChar( nm, ' ' );
	d("q1=[%s]  (*)=[%s]\n", q1.c_str(), nm.c_str() );

	const char *see = "hi there ins it is me tom bay";
	d("see=[%s]\n", see );
	const char *ps = jagstrrstr(see, "me" );
	if ( ps ) {
		d("me=[%s]\n", ps );
	}

	ps = jagstrrstr(see, "mehtere" );
	if ( ps ) {
		d("mehtere=[%s]\n", ps );
	}

	ps = jagstrrstr(see, "hi" );
	if ( ps ) {
		d("hi=[%s]\n", ps );
	}

	Jstr s3[3];
	s3[0] = "OJAG=0=test.pol2.ls=LS 0.0:0.0:500.0:600.0 ";
	d("before s3=[%s]\n", s3[0].c_str() );
	char bf[234];
	sprintf(bf, " 500.0:600.0" );
	//s3 += bf; 
	s3[0]  += Jstr(bf);
	d("after s3=[%s]\n", s3[0].c_str() );

	Jstr a1 = "00000000.2222000000";
	d("old a1=%s\n", a1.c_str() );
	a1.trimEndZeros();
	d("trim0 a1=%s\n", a1.c_str() );

	a1 = "0.000000";
	d("old a1=%s\n", a1.c_str() );
	a1.trimEndZeros();
	d("trim0 a1=%s\n", a1.c_str() );

    d("jagfwritefloat a1:\n");
    jagfwritefloat( a1.s(), a1.size(), stdout );


	a1 = "0000000";
	d("old a1=%s\n", a1.c_str() );
	a1.trimEndZeros();
	d("trim0 a1=%s\n", a1.c_str() );

	a1 = "222.0000000";
	d("old a1=%s\n", a1.c_str() );
	a1.trimEndZeros();
	d("trim0 a1=%s\n", a1.c_str() );

	a1 = ".0000000";
	d("old a1=%s\n", a1.c_str() );
	a1.trimEndZeros();
	d("trim0 a1=%s\n", a1.c_str() );

	a1 = ".00000020";
	d("old a1=%s\n", a1.c_str() );
	a1.trimEndZeros();
	d("trim0 a1=%s\n", a1.c_str() );

	a1 = "2.";
	d("old a1=%s\n", a1.c_str() );
	a1.trimEndZeros();
	d("trim0 a1=%s\n", a1.c_str() );

	a1 = "299";
	d("old a1=%s\n", a1.c_str() );
	a1.trimEndZeros();
	d("trim0 a1=%s\n", a1.c_str() );

	a1 = "0.0000";
	d("old a1=%s\n", a1.c_str() );
	a1.trimEndZeros();
	d("trim0 a1=%s\n", a1.c_str() );

	Jstr a24 = "'fjdkjkdjfkdend'";
	d("a24=[%s]\n", a24.s() );
	a24.trimChar('\'');
	d("a24=[%s]\n", a24.s() );

    int naa = test_skipstrchrnums( "1 2 3 'dddd' 'eeee'", ' ' );
	printf("naa=%d\n", naa );

	AbaxString axs;
	for ( int i=0; i < N; ++i ) {
		axs += "fdkfkdfkdjfdkfdjfkdjkfjdkjfkdjfkdjkfjdkjfkdjkdjkdjfkdjfkdjkfdjkfjdkfjkdjfkdjkfdjkfjdkfjd";
	}
	printf("N=%d AbaxString += done\n", N );
}

#include <GeographicLib/Ellipsoid.hpp>
void test_geo( int N )
{
	const Ellipsoid& wgs84 = Ellipsoid::WGS84();
	printf("s2929 MajorRadius()=%f minor=%f\n", wgs84.MajorRadius(), wgs84.MinorRadius() );

	double lonmaxmeter = JAG_PI * JAG_EARTH_MAJOR_RADIUS/180.0;
	double latmeter = JAG_PI * JAG_EARTH_MINOR_RADIUS/180.0;
	printf("lonmaxmeter=%.10f latmeter=%.10f\n", lonmaxmeter, latmeter );

	JagClock clock;

	clock.start();
	bool rc;
	int cnt = 0;
	for ( int i=0; i < N; ++i ) {
		// 22.2, 232.2, 22.2, 32.3, 0.3
		// 29.2, 230.1, 84.5, 95.3, 0.6
		// rc = JagGeo::ellipseWithinEllipse( 22.2, 232.2, 22.2, 32.3, 0.3, 1.2, 3.1, 34.5, 55.3, 0.6, true );
		rc = JagGeo::ellipseWithinEllipse( 22.2, 232.2, 22.2, 32.3, 0.3,
											29.2, 230.1, 84.5, 95.3, 0.3,
											true );
		if ( rc ) { ++cnt; }
	}
	clock.stop();
	printf("test_geo %d computes ellipseWithinEllipse used %lld millisecs true=%d\n", N, clock.elapsed(), cnt );

	clock.start();
	cnt = 0;
	for ( int i=0; i < N; ++i ) {
		// 22.2, 232.2, 22.2, 32.3, 0.3
		// 29.2, 230.1, 84.5, 95.3, 0.6
		// rc = JagGeo::ellipseWithinEllipse( 22.2, 232.2, 22.2, 32.3, 0.3, 1.2, 3.1, 34.5, 55.3, 0.6, true );
		rc = JagGeo::coneWithinCube( 12.3, 23.1, 32.1, 23.5, 43.1, 0.1, 0.2,
											13.3, 20.3, 30.2, 32.0, 0.3, 0.4, 
											true );
		if ( rc ) { ++cnt; }
	}
	clock.stop();
	printf("test_geo %d computes coneWithinCube used %lld millisecs true=%d\n", N, clock.elapsed(), cnt );


	clock.start();
	cnt = 0;
	JagSortPoint2D *points = new JagSortPoint2D[N];
	for ( int i=0; i < N; ++i ) {
		points[i].x1 = (rand() %10000) * 1.23;
		points[i].y1 = (rand() %10000) * 1.23;
		points[i].x2 = (rand() %100000) * 1.43;
		points[i].y2 = (rand() %100000) * 1.63;
		if ( (i%2) == 0 ) {
			points[i].end = JAG_LEFT;
		} else {
			points[i].end = JAG_RIGHT;
		}
		points[i].color = JAG_RED;
	}

	int nrc = inlineQuickSort( points, N );
	clock.stop();
	printf("test_geo %d computes sortIntersectLinePoints used %lld millisecs nrc=%d\n", N, clock.elapsed(), nrc );
	delete [] points;

	double lon1 = 10.293302838;
	double lat1 = 10.73849879298;
	double alt1 = 15.879298;

	double lon2 = 11.0;
	double lat2 = 11.0;
	int srid = 4326;

	double d1 = JagGeo::distance( lon1, lat1, lon2, lat2, srid );
	printf("geographic dist d1=%f\n", d1);


	double x1, y1, z1, x2, y2;
	JagGeo::lonLatToXY( srid, lon1, lat1, x1, y1 ); 
	JagGeo::lonLatToXY( srid, lon2, lat2, x2, y2 ); 
	printf("point1(%f  %f)    point2(%f %f)\n", x1, y1, x2, y2 );
	printf("(x1-x2)^2=%f  (y1=y2)^2=%f\n", (x1-x2)*(x1-x2), (y1-y2)*(y1-y2) );
	double d2 = JagGeo::distance( x1, y1, x2, y2, 0 );
	printf("jag dist d2=%f\n", d2 );

	double f1= 3.141592653589793/180.0;
	printf("JAG_RADIAN_PER_DEGREE=%.15f\n", f1 );

	double lon1b, lat1b, alt1b;
	JagGeo::XYToLonLat( srid, x1, y1, lon1b, lat1b );
	printf("lon1=%f lat1=%f  lon1b=%f lat1b=%f\n", lon1, lat1, lon1b, lat1b );

	JagGeo::lonLatAltToXYZ( srid, lon1, lat1, alt1, x1, y1, z1 );
	JagGeo::XYZToLonLatAlt( srid, x1,y1,z1, lon1b, lat1b, alt1b );
	printf("lon1=%f lat1=%f alt1=%f  lon1b=%f lat1b=%f alt1b=%f\n", lon1, lat1, alt1, lon1b, lat1b, alt1b );

	//const double lat0 = 48 + 50/60.0, lon0 = 2 + 20/60.0; // Paris
	const double lat0 = 0.0, lon0 = 0.0; 
	const Geocentric& earth = Geocentric::WGS84();
    LocalCartesian proj(lat0, lon0, 0.0, earth);
    {
      // Sample forward calculation
      //double lat = 50.992381, lon = 13.810288, h = 20.2838; // Calais
      double lat = 23.220000, lon = 10.010000, h = 332.400000; // Calais
      double x, y, z;
      proj.Forward(lat, lon, h, x, y, z);
      cout << "lat-lon-alt: " << lat << " " << lon << " " << h << "\n";
      cout << "x-y-z: " << x << " " << y << " " << z << "\n";
	  //-37518.6 229950 -4260.43
      // Sample reverse calculation
   	  LocalCartesian proj2(lat0, lon0, 0.0, earth);
      proj2.Reverse(x, y, z, lat, lon, h);
      cout << "back lat-lon-alt: " << lat << " " << lon << " " << h << "\n";
	  // 50.9003 1.79318 264.915
    }


	std::vector<JagSimplePoint2D> rawPoints;
	rawPoints.push_back( JagSimplePoint2D(32, 31) );
	rawPoints.push_back( JagSimplePoint2D(3, 18) );
	rawPoints.push_back( JagSimplePoint2D(3, 13) );
	rawPoints.push_back( JagSimplePoint2D(32, 1) );
	rawPoints.push_back( JagSimplePoint2D(39, 41) );
	rawPoints.push_back( JagSimplePoint2D(3, 11) );
	rawPoints.push_back( JagSimplePoint2D(8, 21) );
	rawPoints.push_back( JagSimplePoint2D(18, 13) );

	JagSimplePoint2D p(5, 5);
	srid = 4326;
	int K = 3;
	JagMinMaxDistance mm(1, LONG_MAX);
	std::vector<JagSimplePoint2D> neighb;

	JagGeo::kNN2D( srid, rawPoints, p, K, mm, neighb );
	d("neighb.size=%d\n", neighb.size() );
	for ( int i=0; i < neighb.size(); ++i ) {
		d("neigh i=%d neighb.x=%f neighb.y=%f\n", neighb[i].x, neighb[i].y );
	}

}

void test_sqrt( int N )
{
	JagClock clock;
	clock.start();
	//bool rc;
	//int cnt = 0;
	double f;
	for ( int i=0; i < N; ++i ) {
		f = sqrt( (double)N*23.3 );
	}
	clock.stop();
	printf("test_sqrt %d used %lld millisecs \n", N, clock.elapsed() );
}

void writefloat( const char *str, jagint len, FILE *outf )
{
	if ( NULL == str || '\0' == *str || len <1 ) return;

	bool leadzero = false;
	if ( str[0] == '0' && str[1] != '\0' ) leadzero = true;
	d("leadzero=%d\n", leadzero );
	for ( int i = 0; i < len; ++i ) {
		if ( str[i] == '\0' ) continue;

		if ( str[i] != '0' ) leadzero = false;
		d("leadzero=%d\n", leadzero );
		if ( ! leadzero ) {
			fputc( str[i], outf );
		}
	}
}

void test_split( int n )
{
	Jstr s= "1234:93939";
	double x,y;
	char *p;
	const char *str;

	JagClock clock;
	clock.start();
	for ( int i=0; i < n; ++i ) {
		JagStrSplit sp(s, ':');
		x = jagatof(sp[0].c_str() );
		y = jagatof(sp[1].c_str() );
	}
	clock.stop();
	d("JagStrSplit %d used %lld millisecs\n", n, clock.elapsed() );

	clock.start();
	for ( int i=0; i < n; ++i ) {
		str = s.c_str();
		get2double( str, p, ':',  x, y );
		// d("x=%f y=%f\n", x, y );
	}
	clock.stop();
	d("get2double %d used %lld millisecs\n", n, clock.elapsed() );
}

void test_btree( int N )
{
	printf("\ntest_btree ...\n");
	// google btree is slow in search
	std::vector<AbaxString> *vec = new std::vector<AbaxString>(N);
	typedef std::pair<AbaxString, AbaxString> Pair;

	//JagBtree<AbaxString, AbaxString> btree;
	btree::btree_map<AbaxString, AbaxString> btree;

	JagClock clock;
	clock.start();
	int cnt = 0;
	for  ( int i=0; i < N; ++i ) {
		(*vec)[i] = AbaxString::randomValue(32);
		//(*vec)[i] = intToStr(i).c_str();
		btree.insert( Pair( (*vec)[i], (*vec)[i]) );
	}
	clock.stop();
	printf("btree insert %d took %lld millisecs\n", N, clock.elapsed() );

	clock.start();
	cnt = 0;
	for  ( int i=0; i < N; ++i ) {
		if ( btree.find( (*vec)[i] ) != btree.end() ) ++cnt;
	}
	clock.stop();
	printf("btree find found=%d took %lld millisecs\n", cnt, clock.elapsed() );


	clock.start();
	cnt = 0;
	int cp = 0;
	btree::btree_map<AbaxString, AbaxString>::iterator it;

	int tot = 0;
	for ( int i=0; i < 100; ++i ) {
		for ( it =  btree.begin(); it != btree.end(); ++it ) { ++tot; }
	}
	clock.stop();
	printf("btree %d iterator 100 times took %lld millisecs\n", tot, clock.elapsed() );

	#if 0
	clock.start();
	btree::btree_map<AbaxString, AbaxString>::iterator itpred;
	btree::btree_map<AbaxString, AbaxString>::iterator itsucc;

	for  ( int i=0; i < N; ++i ) {
		it = btree.find( (*vec)[i] );
		if ( it == btree.end() ) continue; 
		++cp;
		if ( it->first == (*vec)[i] ) ++cnt;
		//d("i=%d key=[%s] *p=[%s]\n", i, (*vec)[i].c_str(), p->c_str() );
	}
	clock.stop();
	printf("btree getValue pointerOK=%d valuefound=%d tool %lld millisecs\n", cp, cnt, clock.elapsed() );

	clock.start();
	cnt = 0;
	cp = 0;
	AbaxString succ, pred;
	int cnt1 = 0;
	for  ( int i=0; i < N; ++i ) {
		//if ( btree.exist( (*vec)[i] ) ) ++cnt;
		it = btree.lower_bound( (*vec)[i] );
		if ( it == btree.end() ) continue;
		//d(" lowerbound of %s is %s\n", (*vec)[i].c_str(), it->first.c_str() );
		++cp;
		if ( it->first == (*vec)[i] ) {
			--it;
			if ( it == btree.begin() ) continue;
			if ( it == btree.end() ) continue;
			pred = it->first; 
			itpred = it;
			d(" pred of %s is %s\n", (*vec)[i].c_str(), it->first.c_str() );
			++it;
			++it;
			if ( it == btree.end() ) continue;
			succ = it->first;
			itsucc = it;
			d(" succ of %s is %s\n", (*vec)[i].c_str(), it->first.c_str() );

		} else {
			succ = it->first; 
			itsucc = it;
			d(" succ of %s is %s\n", (*vec)[i].c_str(), it->first.c_str() );
			--it;
			if ( it == btree.begin() ) continue;
			if ( it == btree.end() ) continue;
			pred = it->first;
			itpred = it;
			d(" pred of %s is %s\n", (*vec)[i].c_str(), it->first.c_str() );
		}

		if ( itpred != btree.end() && itpred != btree.begin() && itsucc != btree.end() ) {
			++itpred;
			if ( itpred !=  btree.end() && itpred->first == itsucc->first ) {
				++cnt1;
			}

			/***
			--itpred;
			--itsucc;
			if ( itsucc !=  btree.end() && itsucc->first == itpred->first && itpred != btree.begin() ) {
				++cnt2;
			}
			***/
		}

		if ( pred == succ ) ++cnt;
	}
	clock.stop();
	d("btree getPredSucc pointerOK=%d cnt1=%d cnt2=%d tool %lld millisecs\n", cp, cnt1, cnt2, clock.elapsed() );
	#endif


	clock.start();
	cnt = 0;
	cp = 0;
	for ( int i=0; i < N; ++i ) {
		btree.erase( (*vec)[i] );
	}
	clock.stop();
	d("btree remove tool %lld millisecs\n", clock.elapsed() );

	clock.start();
	cnt = 0;
	cp = 0;
	for  ( int i=0; i < N; ++i ) {
		if ( btree.find( (*vec)[i] ) != btree.end() ) ++cnt;
	}
	clock.stop();
	d("btree exist remaining %d tool %lld millisecs\n", cnt, clock.elapsed() );

}

#if 0
void test_btree( int N )
{
	// delete removed do not work
	std::vector<AbaxString> *vec = new std::vector<AbaxString>(N);
	JagBtree<AbaxString, AbaxString> btree;
	JagClock clock;
	clock.start();
	int cnt = 0;
	for  ( int i=0; i < N; ++i ) {
		(*vec)[i] = AbaxString::randomValue(32);
		btree.addValue( (*vec)[i], (*vec)[i] );
	}
	clock.stop();
	d("btree insert %d tool %lld millisecs\n", N, clock.elapsed() );

	clock.start();
	cnt = 0;
	for  ( int i=0; i < N; ++i ) {
		if ( btree.exist( (*vec)[i] ) ) ++cnt;
	}
	clock.stop();
	d("btree exist found=%d tool %lld millisecs\n", cnt, clock.elapsed() );


	clock.start();
	cnt = 0;
	const AbaxString *p;
	const AbaxString *pp;
	int cp = 0;
	for  ( int i=0; i < N; ++i ) {
		//if ( btree.exist( (*vec)[i] ) ) ++cnt;
		p = btree.getValue( (*vec)[i] );
		if ( ! p ) continue;
		if ( p ) ++cp;
		if ( p && *p == (*vec)[i] ) ++cnt;
		//d("i=%d key=[%s] *p=[%s]\n", i, (*vec)[i].c_str(), p->c_str() );
	}
	clock.stop();
	d("btree getValue pointerOK=%d valuefound=%d tool %lld millisecs\n", cp, cnt, clock.elapsed() );

	//btree.print();

	clock.start();
	cnt = 0;
	cp = 0;
	for  ( int i=0; i < N; ++i ) {
		//if ( btree.exist( (*vec)[i] ) ) ++cnt;
		p = btree.getPred( (*vec)[i] );
		if ( ! p ) continue;
		++cp;
		//d("i=%d key=[%s] *p=[%s]\n", i, (*vec)[i].c_str(), p->c_str()  );

		pp = btree.getSucc( *p );
		if ( ! pp ) continue;
		if ( *pp == (*vec)[i] ) ++cnt;
		else {
			d("error i=%d key=[%s] *p=[%s] *pp=[%s]\n", i, (*vec)[i].c_str(), p->c_str(), pp->c_str() );
		}
	}
	clock.stop();
	d("btree getPredSucc pointerOK=%d valuefound=%d tool %lld millisecs\n", cp, cnt, clock.elapsed() );


	clock.start();
	cnt = 0;
	cp = 0;
	for ( int i=0; i < N; ++i ) {
		if ( btree.remove( (*vec)[i] ) ) ++cnt;
	}
	clock.stop();
	d("btree remove %d OK tool %lld millisecs\n", cnt, clock.elapsed() );

	clock.start();
	cnt = 0;
	cp = 0;
	for  ( int i=0; i < N; ++i ) {
		if ( btree.exist( (*vec)[i] ) ) ++cnt;
	}
	clock.stop();
	d("btree exist %d tool %lld millisecs\n", cnt, clock.elapsed() );

}
#endif


void test_json( int N ) 
{
	// write
	using namespace rapidjson;
	StringBuffer s;
	Writer<StringBuffer> writer(s);
	writer.StartObject();   
	writer.Key("key1");  
	writer.String("val1");
	writer.Key("key2");  
	writer.String("val2");
	writer.Key("x1");  
	writer.Double(12.42);
	writer.Key("Null1");  
	writer.Null();
	writer.Key("y1");  
	writer.Double(19.42);
	writer.Double( jagatof("23.434") );
	writer.Key("Null2");  
	writer.Null();

	writer.Key("points");  
	writer.StartArray();   
	for (unsigned i = 0; i < 4; i++) {
		writer.Double(i);
	}
	writer.EndArray();   
	//writer.EndObject();   

    writer.Key("geometry");
	writer.StartObject();   
    writer.Key("type");
    writer.String("point");             // follow by a value.
    writer.Key("cooirdinates");
    writer.StartArray();                // Between StartArray()/EndArray(),
    for (unsigned i = 0; i < 4; i++)
        writer.Uint(i);                 // all values are elements of the array.
    writer.EndArray();
	writer.EndObject();   


	//cout << s.GetString() << endl;
	d("json output string: [%s]\n", s.GetString() );

	rapidjson::Document dom;
	for ( int i=0; i < N; ++i ) {
		if ( dom.Size() > 0 ) {
			dom.RemoveAllMembers( );
		}
		dom.Parse( s.GetString() );
	}

	const Value& n1 = dom["Null1"];
	if ( n1.IsNull() ) {
		d("Null1 is null\n" );
	}

	const Value& n2 = dom["LEN123"];
	if ( n2.IsNull() ) {
		d("LEN123 is null\n" );
	}

	Jstr obj="OJAG=0=test.pol2.po2";
	JagStrSplit sp(obj, '=');

	//const char *sstr = "0.0:0.0:500.0:600.0 0.0:0.0 20.0:0.0 8.0:9.0 0.0:0.0| 1.0:2.0 2.0:3.0 1.0:2.0";
	//char *str = strdup( sstr );

	const char *str = "0.0:0.0:500.0:600.0 0.0:0.0 20.0:0.0 8.0:9.0 0.0:0.0| 1.0:2.0 2.0:3.0 1.0:2.0|2:3 9:0 2:4 20:399";

	//Jstr json= JagGeo::makeJsonPolygon("Polygon", sp, str, true );
	Jstr json= makeJsonPolygon("Polygon", sp, str, false );
	d("json=[%s]\n", json.c_str() );

	const char *js = "{\"type\":\"Polygon\",\"coordinates\":[[[0.0,0.0],[\"20.0\",\"0.0\"],[\"8.0\",\"9.0\"],[\"0.0\",\"0.0\"]],[[\"1.0\",\"2.0\"],[\"2.0\",\"3.0\"],[\"1.0\",\"2.0\"]],[[\"2\",\"3\"],[\"9\",\"0\"],[\"2\",\"4\"],[\"20\",\"399\"]]]}";

	d("js=[%s]\n", js );
	rapidjson::Document d1;
	d1.Parse( js );
	if ( d1.HasParseError() ) {
		d("parse error\n" );
		ParseErrorCode errcode = d1.GetParseError(); 
		const char *err = GetParseError_En( errcode );
		printf("error=[%s]\n", err );
	}



	const Value& tp = d1["type"];
	printf("tp=[%s]\n", tp.GetString() );

	const Value& cd = d1["coordinates"];
	d("cd=[%s]\n", cd.GetString() );
	if ( ! cd.IsArray() ) {
		d("error, coordinates is not array\n" );
	}

	for (SizeType i = 0; i < cd.Size(); i++) {
		if (  cd[i].IsDouble() ) {
			d("i=%d %f\n", i, cd[i].GetDouble() );
		} else if ( cd[i].IsArray() ) {
			const Value &arr = cd[i];
			for (SizeType j = 0; j < arr.Size(); j++) {
				if ( arr[j].IsDouble() ) {
					d("j=%d %f\n", j, arr[j].GetDouble() );
				} else if ( arr[j].IsArray() ) {
					const Value &arr2 = arr[j];
					for (SizeType k = 0; k < arr2.Size(); k++) {
						if ( arr2[k].IsNumber() ) {
							d("number k=%d %f\n", k, arr2[k].GetDouble() );
							//d("number->string k=%s %f\n", k, arr2[k].GetString() );
						} else if ( arr2[k].IsString() ) {
							d("string k=%d %s\n", k, arr2[k].GetString() );
						} else {
							d("no double i=%d j=%d k=%d\n", i, j, k );
						}
					}
				}
			}
		}
	}

}

void test_linefile( int n )
{
	JagLineFile ff;
	ff.append("1111" );
	ff.append("2111" );
	ff.append("3111" );
	ff.append("4111" );
	ff.append("5111" );
	ff.append("6111" );
	ff.append("7111" );

	ff.startRead();

	int cnt = 0;
	Jstr line;
	while ( ff.getLine( line ) ) {
		d("line=[%s]\n", line.c_str() );
		++ cnt;
		if ( cnt > 100 ) break;
	}
}

void test_distance()
{
	double lata1, lona1, lata2, lona2, latb1, lonb1;
	lata1 = 52.10000;
	lona1 = 5.50000;

	lata2 = 52.26;
	lona2 = 5.450000;

	latb1 = 52.2;
	lonb1 = 5.39;

	double dist = JagGeo::pointToLineGeoDistance( lata1, lona1, lata2, lona2, latb1, lonb1 );
	printf("dist=%.3f meters\n", dist );
}

const int DIM = 3;
/***
typedef CGAL::Cartesian_d<double>            CDK;
typedef CGAL::Min_sphere_of_spheres_d_traits_d<CDK,double,DIM> K3Traits;
typedef CGAL::Min_sphere_of_spheres_d<K3Traits> Min_sphere;
typedef CDK::Point_d                      KPoint;
typedef K3Traits::Sphere                  KSphere;
***/

void test_cgal()
{
    // convex_hull_2
    CGALPoint2D points[5] = { CGALPoint2D(0,0), CGALPoint2D(10,0), CGALPoint2D(10,10), CGALPoint2D(6,5), CGALPoint2D(4,1) };
    CGALPoint2D result[5];
    CGALPoint2D *ptr = CGAL::convex_hull_2( points, points+5, result );
    int len  = ptr - result;
    std::cout <<  len << " points on the convex hull:" << std::endl;
    for(int i = 0; i < len; i++){
      std::cout << result[i] << std::endl;
    }

    // convex_hull_2 of vector of points
    std::vector<CGALPoint2D> vpoints, vresult;
    vpoints.push_back(CGALPoint2D(0,0));
    vpoints.push_back(CGALPoint2D(10,0));
    vpoints.push_back(CGALPoint2D(10,10));
    vpoints.push_back(CGALPoint2D(6,5));
    vpoints.push_back(CGALPoint2D(2,1));
    vpoints.push_back(CGALPoint2D(4,8));
    CGAL::convex_hull_2( vpoints.begin(), vpoints.end(), std::back_inserter(vresult) );
    std::cout << vresult.size() << " points on the 2D convex hull:" << std::endl;
    for(int i = 0; i < vresult.size(); i++){
      std::cout << vresult[i] << std::endl;
      //std::cout << "x: " << vresult[i].x() << std::endl;
      //std::cout << "y: " << vresult[i].y() << std::endl;
    }

	d("3D connvex hull:\n" );
	JagLineString lines, hull;
	lines.add( 1.1, 2.3, 3.2);
	lines.add( 1.9, 2.3, 4.9);
	lines.add( 2.2, 2.2, 4.4);
	lines.add( 4.1, 3.3, 2.2);
	lines.add( 9.1, 8.3, 1.2);
	lines.add( 9.0, 8.2, 1.1);
	lines.add( 2.1, 4.3, 1.2);
	lines.add( 5.1, 1.3, 5.2);
	lines.add( 5.4, 1.2, 6.2);
	lines.add( 5.3, 1.1, 6.1);
	lines.add( 4.0, 3.0, 2.1);

	d("t3031 JagCGAL::getConvexHull3D( input lines=%d, hull ); ...\n", lines.size() );
	JagCGAL::getConvexHull3D( lines, hull );
    d("result hull=%d:\n", hull.size() );
	hull.print();

    /////// orientation ///////////////
    CGALPoint2D p(0,2), q(0,5);
    std::cout << "p = " << p << std::endl;
    std::cout << "q = " << q.x() << " " << q.y() << std::endl;
    std::cout << "sqdist(p,q) = " << CGAL::squared_distance(p,q) << std::endl;

    CGALSegment2D s(p,q);
    CGALPoint2D m(1, 0);
    std::cout << "m = " << m << std::endl;
    std::cout << "sqdist(Segment2(p,q), m) = " << CGAL::squared_distance(s,m) << std::endl;

    std::cout << "p, q, and m ";
    switch (CGAL::orientation(p,q,m))
    {
      case CGAL::COLLINEAR:
          std::cout << "are collinear\n";
          break;
      case CGAL::LEFT_TURN:
          std::cout << "make a left turn\n";
          break;
      case CGAL::RIGHT_TURN:
          std::cout << "make a right turn\n";
          break;
    }

	/***
  	std::vector<Point_3> points;
  	CGALPoint3D p;
    points.push_back(p);

  	// define polyhedron to hold convex hull
  	Polyhedron_3 poly;
  
  	// compute convex hull of non-collinear points
  	CGAL::convex_hull_3(points.begin(), points.end(), poly);

  	std::cout << "The convex hull contains " << poly.size_of_vertices() << " vertices" << std::endl;
  
  	Surface_mesh sm;
  	CGAL::convex_hull_3(points.begin(), points.end(), sm);

  	std::cout << "The convex hull contains " << num_vertices(sm) << " vertices" << std::endl;

	boost::geometry::model::multi_polygon<BoostPolygon2D> mbgon;
	double ds = boost::geometry::perimeter( mbgon );
	************/

	std::vector<CGALSphere> S;
	double coord[DIM];
	for ( int i=0; i < 10; ++i ) {
		coord[0] = i+4.5;
		coord[1] = i+5.5;
		coord[2] = i+8.5;
		CGALCartPoint point(DIM, coord, coord+DIM );
		S.push_back( CGALSphere(point, 3.0) );
	}

	CGAL_Min_sphere ms(S.begin(),S.end());
	CGAL_assertion(ms.is_valid());
	int cnt = 0;
	double r = ms.radius();
	d("radius=%f\n", r );
	for ( auto c = ms.center_cartesian_begin(); c != ms.center_cartesian_end(); ++c ) {
		//std::cout << *c << std::endl;
		d("cnt=%d coord=%f\n", cnt, *c );
		++cnt;
	}

	std::vector<CGALCircle> C;
	double cord[2];
	for ( int i=0; i < 10; ++i ) {
		cord[0] = i+4.5;
		cord[1] = i+9.5;
		CGALCartPoint point(2, cord, cord+2 );
		C.push_back( CGALCircle(point, 6.0) );
	}

	CGAL_Min_circle mc(C.begin(),C.end());
	CGAL_assertion(mc.is_valid());
	cnt = 0;
	r = mc.radius();
	d("radius=%f\n", r );
	for ( auto c = mc.center_cartesian_begin(); c != mc.center_cartesian_end(); ++c ) {
		d("cnt=%d coord=%f\n", cnt, *c );
		++cnt;
	}


}

struct Intersection_visitor2 
{
  typedef void result_type;
  Intersection_visitor2( ) { _a = 0; }
  Intersection_visitor2( int a ) { _a = a; }

  void operator()(const CGALKernel::Point_2& p) const
  {
    std::cout << "2D point " <<  p << std::endl;
  }

  void operator()(const CGALKernel::Segment_2& seg) const
  {
     std::cout << "2D segm " << seg << std::endl;
	 CGALKernel::Point_2 s = seg.source();
	 CGALKernel::Point_2 t = seg.target();
     std::cout << "2D s.x " << s.x() << " y " << s.y() <<  std::endl;
     std::cout << "2D t.x " << t.x() << " y " << t.y() <<  std::endl;
  }
  int _a;
};

struct Intersection_visitor3 
{
	Intersection_visitor3( int a ) { _a = a; }
  typedef void result_type;

  void operator()(const CGALKernel::Point_3& p) const
  {
    std::cout << "CGALKernelPoint_3 _a=" << _a << std::endl;
    std::cout << p.x() << ":" << p.y() << std::endl;
  }

  void operator()(const CGALKernel::Segment_3& s) const
  {
    std::cout << "Segment_3 _a=" << _a << std::endl;
    std::cout << s << std::endl;
  }

  int _a;
};


void test_intersection()
{
    CGALKernel::Segment_3 seg(CGALKernel::Point_3(0,0,8), CGALKernel::Point_3(2,2,2));
	CGALKernel::Point_3 p1( 0, 0, 8 );
	CGALKernel::Point_3 p2( 4, 8, 9 );
    CGALKernel::Line_3 lin(p1, p2 );
    CGALKernel::Segment_3 seg2(CGALKernel::Point_3(0,0,8), CGALKernel::Point_3(4,4,2));
    auto result = intersection(seg, lin);
	if ( result ) {
		boost::apply_visitor(Intersection_visitor3(2), *result); 
	} else {
		std::cout << "3D No intersection point" << std::endl;
	}

    result = intersection(seg, seg2);
	if ( result ) {
		boost::apply_visitor(Intersection_visitor3(8), *result); 
	} else {
		std::cout << "3D No intersection point" << std::endl;
	}

	// 2D
    CGALKernel::Segment_2 seg20(CGALKernel::Point_2(0,0), CGALKernel::Point_2(2,2));
    CGALKernel::Segment_2 seg21(CGALKernel::Point_2(0,0), CGALKernel::Point_2(2,2));
    CGALKernel::Segment_2 seg22(CGALKernel::Point_2(0,0), CGALKernel::Point_2(2,4));
    auto result2 = intersection(seg20, seg21);
	if ( result2 ) {
		boost::apply_visitor(Intersection_visitor2(8), *result2); 
	} else {
		std::cout << "2D No intersection point" << std::endl;
	}

    result2 = intersection(seg20, seg22);
	if ( result2 ) {
		boost::apply_visitor(Intersection_visitor2(8), *result2); 
	} else {
		std::cout << "2D No intersection point" << std::endl;
	}

	typedef boost::geometry::model::polygon<boost::geometry::model::d2::point_xy<double> > polygon;
    polygon green, blue;

    boost::geometry::read_wkt(
        "POLYGON((2 1.3,2.4 1.7,2.8 1.8,3.4 1.2,3.7 1.6,3.4 2,4.1 3,5.3 2.6,5.4 1.2,4.9 0.8,2.9 0.7,2 1.3)"
            "(4.0 2.0, 4.2 1.4, 4.8 1.9, 4.4 2.2, 4.0 2.0))", green);

    boost::geometry::read_wkt(
        "POLYGON((4.0 -0.5 , 3.5 1.0 , 2.0 1.5 , 3.5 2.0 , 4.0 3.5 , 4.5 2.0 , 6.0 1.5 , 4.5 1.0 , 4.0 -0.5))", blue);

    //std::deque<polygon> output;
    std::vector<polygon> output;
    boost::geometry::intersection(green, blue, output);
	d("s2029 intersection output #of polygons=%d\n", output.size() );
    int i = 0;
    std::cout << "green && blue:" << std::endl;
    BOOST_FOREACH(polygon const& poly, output)
    {
        std::cout << i++ << ": " << boost::geometry::area(poly) << std::endl;
		//boost::apply_visitor(Intersection_visitor2(8), p); 
		//getting the vertices back
		for(auto it = boost::begin(boost::geometry::exterior_ring(poly)); it != boost::end(boost::geometry::exterior_ring(poly)); ++it)
		{
    		double x = boost::geometry::get<0>(*it);
    		double y = boost::geometry::get<1>(*it);
			printf("s203 x=%0f  y=%f\n", x, y );
		}
    }

	output.clear();
    boost::geometry::intersection( blue, green, output);
	d("s20292 intersection output #of polygons=%d\n", output.size() );
    i = 0;
    std::cout << "green && blue:" << std::endl;
    BOOST_FOREACH(polygon const& poly, output)
    {
        std::cout << i++ << ": " << boost::geometry::area(poly) << std::endl;
		//boost::apply_visitor(Intersection_visitor2(8), p); 
		//getting the vertices back
		for(auto it = boost::begin(boost::geometry::exterior_ring(poly)); it != boost::end(boost::geometry::exterior_ring(poly)); ++it)
		{
    		double x = boost::geometry::get<0>(*it);
    		double y = boost::geometry::get<1>(*it);
			printf("s203 x=%0f  y=%f\n", x, y );
		}
    }


}

class TestN
{
  public:
     TestN() { p = new char[100*1000]; }
     TestN( long n) { p = new char[n*1000]; }
	 ~TestN() { delete p; }
	 char *p;
};

void test_new( long n )
{
	TestN *t = newObject<TestN>( true );
	printf("t=%p n=%ld\n", t, n );
}

void test_equation()
{
	double x, y, z, dist;
	JagGeo::minMaxPointOnNormalEllipse( 0, 20.0, 10.0, 800.0, 900.0, true, x, y, dist );
	d("ellipse min x=%f y=%f  dist=%f\n", x, y, dist );

	JagGeo::minMaxPointOnNormalEllipse( 0, 40.0, 30.0, 80.0, 90.0, true, x, y, dist );
	d("ellipse min x=%f y=%f  dist=%f\n", x, y, dist );

	JagGeo::minMaxPointOnNormalEllipse( 0, 40.0, 30.0, 80.0, 90.0, false, x, y, dist );
	d("ellipse max x=%f y=%f  dist=%f\n", x, y, dist );

	JagGeo::minMaxPoint3DOnNormalEllipsoid( 0,  40.0, 30.0, 20.0, 80.0, 90.0, 93.0, true, x, y, z, dist ); 
	d("ellipsoid min x=%f y=%f z=%f  dist=%f\n", x, y, z, dist );

	JagGeo::minMaxPoint3DOnNormalEllipsoid( 0,  40.0, 30.0, 20.0, 80.0, 90.0, 93.0, false, x, y, z, dist ); 
	d("ellipsoid max x=%f y=%f z=%f  dist=%f\n", x, y, z, dist );

}

void test_hashstr()
{
	JagHashStrStr *hs = new JagHashStrStr();
	hs->addKeyValue("k1", "v1" );
	hs->addKeyValue("k2", "v2" );
	hs->addKeyValue("k3", "v3" );

	hs->reset();

	char *p = hs->getValue("k1");
	if ( p == NULL ) {
		d("reset hash, k1 got NULL\n");
	} else {
		d("reset hash, k1 got [%s]\n", p );
	}

	hs->addKeyValue("k1", "v1" );
	p = hs->getValue("k1");
	if ( p == NULL ) {
		d("reset hash, insert k1,  k1 got NULL\n");
	} else {
		d("reset hash, insert k1, k1 got [%s]\n", p );
	}
	
}

void test_fflush( int N )
{
	FILE *f = fopen("myappend.txt", "ab");

	printf("test fflush %d times\n", N );
	for ( int i = 0; i < N ; i ++ ) {
		fprintf(f, "%d\n", i );
		fflush( f );
	}
	fclose(f );

}

void test_jstr( int N )
{
	Jstr s1 = "aaaaaaaaaaaajkdjkfjdkf djkfdfdfdjkfdkjfkdjkfjdfd";
	Jstr sb = "bbbbbbbbbbbbjkdjkfjdkf djkfdfdfdjkfdkjfkdjkfjdfd";
	printf("%d ...\n", N );
	for ( int i = 0; i < N ; i ++ ) {
		Jstr s("aaaaaaaaaa");
		Jstr s2;
		s2 = s1; // mem leak
		printf("s222293 s2=[%s] s1=[%s]\n", s2.s(), s1.s() );
		//Jstr a = s1 + s2 + s1;

		sb = s;
		printf("s222294 sb=[%s] s=[%s]\n", s1.s(), s.s() );
		//Jstr a = s1 + s2 + s1;

		if ( 0 == ( i%100 ) ) {
			malloc_trim( 0 );
		}
	}

	Jstr a = "hello world hihihi world end";
	printf("a=[%s]\n", a.s() );
	a.replace("world", "usa" );
	printf("a=[%s]\n", a.s() );
	a.replace("hello", 'h');
	printf("a=[%s]\n", a.s() );

}


void test_parseparam( int N ) 
{
	printf("test_parseparam N=%d ...\n", N ); fflush(stdout );
	Jstr cmd = "select a, b, g from t where a='1222' and b like 'sss%'";
	Jstr errmsg;
	JagParser parser( (void*)NULL );
	JagParseParam parseParam(&parser);

	JagParseAttribute jpa( NULL, 0, 0, "test", NULL );

	int rc  = -1;
	for ( int i = 0; i < N ; i ++ ) {
		/***
		JagParseParam parseParam(&parser);
		rc = parser.parseCommand( jpa, cmd, &parseParam, errmsg );
		***/

		rc = parser.parseCommand( jpa, cmd, &parseParam, errmsg );
		//parseParam.clean();
	}
	printf("test_parseparam N=%d done rc=%d\n", N, rc ); fflush(stdout );

}

spsc_queue<int> q;
void test_spsc_queue( int N )
{
  pthread_t thread1;
  pthread_t thread2;
  jagpthread_create(&thread1, NULL, &thrdfunc1, (void *)NULL);
  jagpthread_create(&thread2, NULL, &thrdfunc2, (void *)NULL);
  while ( true );

}

void *thrdfunc1(void *ptr)
{
  //int v;
  int N = 1000000000;
  for ( int i = 0; i < N; i ++ ) {
  	q.enqueue(i);
  }
  d("a939339 done enque N=%d \n", N );
  return NULL;


}

void *thrdfunc2(void *ptr)
{
  int v;
  while ( 1 ) {
  	int cnt = 0;
  	while ( q.dequeue(v) ) {
		++cnt;
	}
    d("batch a939349 done deque cnt=%d sleep 1 sec... \n", cnt);
	sleep(1);
  }
  return NULL;


}

int test_stdmap_threads( int N )
{
	pthread_t  thread[32];
	TPass tp;

	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_init( &mutex, NULL );

	std::map<std::string,std::string> *map = new std::map< std::string,std::string>();
	tp.ptr1 = (void*)map;
	tp.num = N;
	tp.ptr2 = (void*)&mutex;
	
	/***
	pthread_create( &thread[0], NULL, insertMap, (void*)&tp );
	pthread_create( &thread[1], NULL, updateMap, (void*)&tp );
	pthread_create( &thread[2], NULL, readMap, (void*)&tp );
	pthread_create( &thread[3], NULL, deleteMap, (void*)&tp );
	pthread_create( &thread[4], NULL, readMap, (void*)&tp );
	pthread_create( &thread[5], NULL, insertMap, (void*)&tp );
	pthread_create( &thread[6], NULL, readMap, (void*)&tp );

   	pthread_join(thread[0], NULL );
   	pthread_join(thread[2], NULL );
   	pthread_join(thread[3], NULL );
   	pthread_join(thread[1], NULL );
   	pthread_join(thread[4], NULL );
   	pthread_join(thread[5], NULL );
   	pthread_join(thread[6], NULL );
	**/

	JagClock clock;
	clock.start();
	int thrnum = 16;
	for ( int i =0 ; i < thrnum; ++i ) {
		pthread_create( &thread[i], NULL, insertMap, (void*)&tp );
	}

	for ( int i =0 ; i < thrnum; ++i ) {
		pthread_join( thread[i], NULL );
	}
	clock.stop();
	printf("stdmapp threads with mutex N=%d threads=%d usedtim=%lld milliseconds\n", N, thrnum, clock.elapsed() );

	//sleep(10);
	pthread_mutex_destroy( &mutex );
	return 0;

}

void *insertMap( void *ptr )
{
	TPass *pt = (TPass*)ptr;
	std::map<std::string,std::string> *map = (std::map<std::string,std::string>*)(pt->ptr1);
	int N = pt->num;
	pthread_mutex_t *pmutex = (pthread_mutex_t*)pt->ptr2;

	std::string s;
	AbaxString rs;

	try {
    	for ( int i =0; i < N; ++i ) {
    		rs = rs.randomValue(16);
			s = rs.c_str();
    		//map->emplace(s, s);
			pthread_mutex_lock( pmutex );
    		map->insert(std::make_pair(s, s));
    		map->find(s);

    		s = intToStr(i).c_str();
    		//map->emplace(s, s);
    		map->insert(std::make_pair(s, s));
    		map->find(s);
			pthread_mutex_unlock( pmutex );
    	}
	} catch (std::exception &e ) {
		printf("insertMap exception e caught\n");
		fflush(stdout);
	} catch ( ... ) {
		printf("insertMap exception caught\n");
		fflush(stdout);
	}

	//printf("insertMap done\n");
	//fflush(stdout);
	return NULL;
}

void *updateMap( void *ptr )
{
	TPass *pt = (TPass*)ptr;
	std::map<std::string,std::string> *map = (std::map<std::string,std::string> *)(pt->ptr1);
	int N = pt->num;

	std::string s;
	sleep(3);

	try {
		for ( int i=0; i < N; ++i ) {
			s = intToStr(i).c_str();
			(*map)[s] = "111";
		}
	} catch ( ... ) {
		printf("udateMap exception caught\n");
		fflush(stdout);
	}

	printf("updateMap done\n");
	fflush(stdout);

	return NULL;
}

void *readMap( void *ptr )
{
	TPass *pt = (TPass*)ptr;
	std::map<std::string,std::string> *map = (std::map<std::string,std::string> *)pt->ptr1;

	std::map<std::string,std::string>::iterator iter;

	std::string k, v;
	sleep(8);

	try {
		for ( iter = map->begin(); iter != map->end(); ++ iter ) {
			k = iter->first;
			v = iter->second;
		}
	} catch ( ... ) {
		printf("readMap exception caught\n");
		fflush(stdout);
	}

	printf("readMap done\n");
	fflush(stdout);
	return NULL;
}

void *deleteMap( void *ptr )
{
	TPass *pt = (TPass*)ptr;
	std::map<std::string,std::string> *map = (std::map<std::string,std::string> *)pt->ptr1;
	int N = pt->num;

	std::string s;
	sleep(4);

	try {
		for ( int i =N/3; i < N/2; ++i ) {
			s = intToStr(i).c_str();
			map->erase(s);
		}
	} catch ( ... ) {
		printf("deleteMap exception caught\n");
		fflush(stdout);
	}

	printf("deleteMap done\n");
	fflush(stdout);
	return NULL;
}

#if 0
template <typename T>
struct my_traits {
  static const bool value = true;
};

//typedef cds::gc::HP  gc_type;
typedef cds::container::SkipListMap< cds::gc::HP, std::string, std::string > SkipListStrStrMap;
namespace cc = cds::container;

// lock-free data structures: main thread
void test_cds(int N )
{
    // Initialize libcds
    cds::Initialize();
    {
        // Initialize Hazard Pointer singleton
        //cds::gc::HP hpGC;
		//cds::gc::hp::GarbageCollector::Construct( 66, 1, 16 );
        cds::gc::HP hpGC(67);

        // If main thread uses lock-free containers
        // the main thread should be attached to libcds infrastructure
		//cds::gc::hp::GarbageCollector::Construct( cds::gc::hp::c_nHazardPtrCount + 1, 1, 16 );
        cds::threading::Manager::attachThread();
		SkipListStrStrMap *skm = new SkipListStrStrMap;
		TPass pass;
		pass.num = N;
		pass.ptr1 = skm;
        // Now you can use HP-based containers in the main thread
		//skm->c_nHazardPtrCount  = 66;

        //...
		JagClock clock;
		pthread_t ins_thread[32];
		pthread_t find_thread[32];
		pthread_t upd_thread[32];
		pthread_t del_thread[32];
		int thrnum = 30;
		bool dofind = false;
		bool doupdate = false;
		bool dodelete = false;
		bool doiterate = true;

		clock.start();
		for ( int i=0; i < thrnum; ++i ) {
			pthread_create( &ins_thread[i], NULL, cds_insert, (void*)&pass );
		}

		if ( dofind ) {
			for ( int i=0; i < thrnum; ++i ) {
				pthread_create( &find_thread[i], NULL, cds_find, (void*)&pass );
			}
		}

		if ( doupdate ) {
			for ( int i=0; i < thrnum; ++i ) {
				pthread_create( &upd_thread[i], NULL, cds_update, (void*)&pass );
			}
		}

		if ( dodelete ) {
			for ( int i=0; i < thrnum; ++i ) {
				pthread_create( &del_thread[i], NULL, cds_delete, (void*)&pass );
			}
		}

		/***
		Iterator ensures thread-safety even if you delete the item the iterator points to. 
		However, in case of concurrent deleting operations there is no guarantee that you 
		iterate all item in the list. Moreover, a crash is possible when you try to iterate 
		the next element that has been deleted by concurrent thread.
		delete_lock on erase and iterator of N items for copy

		https://kukuruku.co/post/lock-free-data-structures-introduction/
		https://kukuruku.co/post/lock-free-data-structures-basics-atomicity-and-atomic-primitives/
		https://kukuruku.co/post/lock-free-data-structures-memory-model-part-3/
		https://kukuruku.co/post/lock-free-data-structures-the-inside-memory-management-schemes/
		https://kukuruku.co/post/lock-free-data-structures-the-inside-rcu/
		**/
		if ( doiterate ) {
			JagClock clock2;
			clock2.start();
			SkipListStrStrMap::iterator iter;
			int cnt = 0;
			for ( iter = skm->begin(); iter != skm->end(); ++iter ) {
				//printf("first=[%s] second=[%s]\n", iter->first.c_str(), iter->second.c_str() );
				++cnt;
			}
			clock2.stop();
			printf("iterator cnt=%d tool %lld millisecs\n", cnt, clock2.elapsed() );
		}

		for ( int i=0; i < thrnum; ++i ) {
			pthread_join( ins_thread[i], NULL );
			if ( dofind ) pthread_join( find_thread[i], NULL );
			if ( doupdate ) pthread_join( upd_thread[i], NULL );
			if ( dodelete ) pthread_join( del_thread[i], NULL );
		}
		clock.stop();
		printf("skiplistmap N=%d datanum=%d thrnum=%d used %lld milliseconds doinsert=true  dofind=%d doupdate=%d dodelete=%d doiterate=%d\n", 
				N, int(pass.anum1), thrnum, clock.elapsed(), dofind, doupdate, dodelete, doiterate );
    }

    // Terminate libcds
    cds::Terminate();
}

// lock-free data structures: insert thread
void *cds_insert(void * ptr)
{
	TPass *pass = (TPass*)ptr;
	SkipListStrStrMap *skm = (SkipListStrStrMap*)pass->ptr1;
	int N = pass->num; 
	
    // Attach the thread to libcds infrastructure
    cds::threading::Manager::attachThread();
    // Now you can use HP-based containers in the thread
    //...
	AbaxString rs;
	bool rc;
	//JagClock clock;
	//clock.start();
	for ( int i = 0; i < N; i ++ ) {
		rs = rs.randomValue(16);
		//rc = skm->find( std::string(rs.c_str()) );
		rc = skm->insert( std::string(rs.c_str()), "randomva" );
		rc = skm->insert( intToStr(i).c_str(), "sequence" );
		++ pass->anum1 ;
		++ pass->anum1 ;
	}
	//clock.stop();
	//printf("skm N=%d contain cnt=%d items size=%d used %lld milliseconds empty=%d\n", N, cnt, skm->size(), clock.elapsed(), skm->empty() );

    // Detach thread when terminating
    cds::threading::Manager::detachThread();
	return NULL;
}

// lock-free data structures: find thread
void *cds_find(void * ptr)
{
	TPass *pass = (TPass*)ptr;
	SkipListStrStrMap *skm = (SkipListStrStrMap*)pass->ptr1;
	int N = pass->num; 
	
    // Attach the thread to libcds infrastructure
    cds::threading::Manager::attachThread();
    // Now you can use HP-based containers in the thread
    //...
	AbaxString rs;
	bool rc;
	//JagClock clock;
	//clock.start();
	for ( int i = 0; i < N; i ++ ) {
		rs = rs.randomValue(16);
		rc = skm->contains( std::string(rs.c_str()) );
		rc = skm->contains( intToStr(i).c_str() );
	}
    // Detach thread when terminating
    cds::threading::Manager::detachThread();
	return NULL;
}

class my_functor {
  public:
    void operator()( bool bNew, SkipListStrStrMap::value_type &item ) 
	{
		item.second = value;
	} 
	std::string value;
};

// lock-free data structures: find thread
void *cds_update(void * ptr)
{
	TPass *pass = (TPass*)ptr;
	SkipListStrStrMap *skm = (SkipListStrStrMap*)pass->ptr1;
	int N = pass->num; 
	
    // Attach the thread to libcds infrastructure
    cds::threading::Manager::attachThread();
    // Now you can use HP-based containers in the thread
    //...
	my_functor myfunc;
	//bool rc;
	//JagClock clock;
	//clock.start();
	std::string k;
	for ( int i = 0; i < N; i ++ ) {
		k = intToStr(i).c_str();
		myfunc.value = "newval";
		std::pair<bool, bool> res = skm->update(k, myfunc, false );
	}
    // Detach thread when terminating
    cds::threading::Manager::detachThread();
	return NULL;

}

// lock-free data structures: find thread
void *cds_delete(void * ptr)
{
	TPass *pass = (TPass*)ptr;
	SkipListStrStrMap *skm = (SkipListStrStrMap*)pass->ptr1;
	int N = pass->num; 
	
    // Attach the thread to libcds infrastructure
    cds::threading::Manager::attachThread();
    // Now you can use HP-based containers in the thread
    //...
	bool rc;
	//JagClock clock;
	//clock.start();
	std::string k;
	for ( int i = 0; i < N; i ++ ) {
		k = intToStr(i).c_str();
		rc = skm->erase(k);
	}
    // Detach thread when terminating
    cds::threading::Manager::detachThread();
	return NULL;

}

#if 0
void test_lockfreedbmap( int N )
{
	cds::Initialize();
    cds::gc::HP hpGC(67);
	cds::threading::Manager::attachThread();


	printf("test_lockfreedbmap ...\n");
	//JagLockFreeMap<std::string, std::string>  map(128);
	JagLockFreeDBMap  map(128);
	bool rc;
	rc = map.insert("k1", "v1");
	printf("insert k1 rc=%d\n", rc );

	rc = map.update("k1", "v2");
	printf("update k1 rc=%d\n", rc );

	rc = map.update("k2", "v2");
	printf("update k2 rc=%d\n", rc );

	rc = map.exists("k1");
	printf("exists k1 rc=%d\n", rc );

	JagFixString v;
	rc = map.get("k1", v);
	printf("get k1 rc=%d v=[%s]\n", rc, v.c_str() );

	v = "";
	rc = map.get("k2", v);
	printf("get k2 rc=%d v=[%s]\n", rc, v.c_str() );

	rc = map.remove("k1");
	printf("remove k1 rc=%d\n", rc );

	cds::Terminate();
}
#endif

// lock-free data structures: insert thread
void *safemap_insert(void * ptr)
{
	TPass *pass = (TPass*)ptr;
	SafeStrStrMap *skm = (SafeStrStrMap*)pass->ptr1;
	int N = pass->num; 
	
	AbaxString rs;
	for ( int i = 0; i < N; i ++ ) {
		rs = rs.randomValue(16);
		skm->insert( std::make_pair(std::string(rs.c_str()), "randomva" ));
		skm->insert( std::make_pair(intToStr(i).c_str(), "sequence" ));
		++ pass->anum1;
		++ pass->anum1;
	}
	return NULL;
}

// lock-free data structures: find thread
void *safemap_find(void * ptr)
{
	TPass *pass = (TPass*)ptr;
	SafeStrStrMap *skm = (SafeStrStrMap*)pass->ptr1;
	int N = pass->num; 
	
	AbaxString rs;
	for ( int i = 0; i < N; i ++ ) {
		rs = rs.randomValue(16);
		skm->find( std::string(rs.c_str()) );
		skm->find( intToStr(i).c_str() );
	}
	return NULL;

}

// lock-free data structures: find thread
void *safemap_update(void * ptr)
{
	TPass *pass = (TPass*)ptr;
	SafeStrStrMap *skm = (SafeStrStrMap*)pass->ptr1;
	int N = pass->num; 
	
	std::string k;
	for ( int i = 0; i < N; i ++ ) {
		k = intToStr(i).c_str();
		(*skm)[k] = "aaaaaaaaaaa";
	}
	return NULL;

}

// lock-free data structures: find thread
void *safemap_delete(void * ptr)
{
	TPass *pass = (TPass*)ptr;
	SafeStrStrMap *skm = (SafeStrStrMap*)pass->ptr1;
	int N = pass->num; 
	
	std::string k;
	for ( int i = 0; i < N; i ++ ) {
		k = intToStr(i).c_str();
		skm->erase(k);
	}
	return NULL;

}

void test_safemap(int N )
{
		SafeStrStrMap *skm = new SafeStrStrMap;
		TPass pass;
		pass.num = N;
		pass.ptr1 = skm;
        // Now you can use HP-based containers in the main thread
		//skm->c_nHazardPtrCount  = 66;

        //...
		JagClock clock;
		pthread_t ins_thread[32];
		pthread_t find_thread[32];
		pthread_t upd_thread[32];
		pthread_t del_thread[32];
		int thrnum = 30;
		bool dofind = false;
		bool doupdate = false;
		bool dodelete = false;
		bool doiterate = true;
		clock.start();
		for ( int i=0; i < thrnum; ++i ) {
			pthread_create( &ins_thread[i], NULL, safemap_insert, (void*)&pass );
		}

		if ( dofind ) {
			for ( int i=0; i < thrnum; ++i ) {
				pthread_create( &find_thread[i], NULL, safemap_find, (void*)&pass );
			}
		}

		if ( doupdate ) {
			for ( int i=0; i < thrnum; ++i ) {
				pthread_create( &upd_thread[i], NULL, safemap_update, (void*)&pass );
			}
		}

		if ( dodelete ) {
			for ( int i=0; i < thrnum; ++i ) {
				pthread_create( &del_thread[i], NULL, safemap_delete, (void*)&pass );
			}
		}

		if ( doiterate ) {
			JagClock clock2;
			clock2.start();
			int cnt  = 0;
			for ( auto iter = skm->begin(); iter != skm->end(); ++iter ) {
				//printf("fwd first=[%s] second=[%s]\n", iter->first.c_str(), iter->second.c_str() );
				++cnt;
			}
			clock2.stop();
			printf("iterator cnt=%d tool %lld millisecs\n", cnt, clock2.elapsed() );
		}

		for ( int i=0; i < thrnum; ++i ) {
			pthread_join( ins_thread[i], NULL );
			if ( dofind ) pthread_join( find_thread[i], NULL );
			if ( doupdate ) pthread_join( upd_thread[i], NULL );
			if ( dodelete ) pthread_join( del_thread[i], NULL );
		}
		clock.stop();
		printf("safemap N=%d datanum=%d thrnum=%d used %lld milliseconds doinsert=true dofind=%d doupdate=%d dodelete=%d doiterate=%d\n", 
				N, int(pass.anum1), thrnum, clock.elapsed(), dofind, doupdate, dodelete, doiterate );

}
#endif

void test_numinstr()
{
	/***
	int a = -20213391;
	char *p = (char*)malloc(1024);
	memset(p, 0, 1024 );

	long long bignum = 12303019993990;

	memcpy(p, "haha", 4 );
	memcpy(p+4, &a, 4 );
	memcpy(p+8, "12345", 5 );

	printf("p=[%s]\n", p );
	int  b = get4ByteInt( p+4 );
	printf("a=%d   --> b=%d\n", a, b );

	memcpy(p+24, &bignum, 8 );
	long long bignum2 = get8ByteLongLong( p+24 );
	printf("bigum=%lld   --> bignum2=%lld\n", bignum, bignum2 );

	time_t t1 = 30393939339;
	printf("sizeof time_t=%d\n", sizeof(time_t) );
	printf("sizeof time_t=%d\n", sizeof(t1) );
	***/
}


// ULONG_MAX = 18446744073709551615 size=20
// base62 max of ULONG_MAX: 2NVca2g1ST8F  size=12 

void test_base62( char *str )
{
	// str is "84938948394" or "-4383493493"

	if ( str == NULL ) {
        str = strdup("1000");
	}

	JagMath m;
	Jstr res;
	unsigned long n;
	unsigned long back;
	char tbf[64];
	int slen;

	sprintf(tbf, "%lu", ULONG_MAX );
	slen = strlen(tbf);
	printf("ULONG_MAX = %lu size=%d\n", ULONG_MAX, slen );

	sprintf(tbf, "%ld", LONG_MAX );
	slen = strlen(tbf);
	printf("LONG_MAX = %ld size=%d\n", LONG_MAX, slen );

	sprintf(tbf, "%ld", LONG_MIN );
	slen = strlen(tbf);
	printf("LONG_MIN = %ld size=%d\n", LONG_MIN, slen );
	printf("LONG_MIN = %ld size=%d\n", LONG_MIN, slen );
	printf("\n\n");

    // display normal length and b6 length
    for ( int i = 1; i < 25; ++i ) {
        int b62len = m.base62WidthSlow(i);
        printf("i=%d    b62.len=%d\n", i, b62len);
    }

	if ( '-' == *str ) {
    	///////////// negative long /////////////
		printf("negative long conversion ...\n");
    	long sn;
    	sn = strtol( str,  NULL, 10 );
    	sprintf(tbf, "%ld", sn );
    	slen = strlen(tbf);
    	printf("sn = %ld size=%d\n", sn, slen );
    
    	m.base62FromLong( res, sn );
    	printf("sn=%ld   base62: [%s] size=%ld\n", sn, res.s(), res.size() );
    
    	long sback = m.base62ToLong( res.s() );
    	printf("sback=%ld   base62: [%s]\n", sback, res.s() );
    	sprintf(tbf, "%ld", sback );
    	slen = strlen(tbf);
    	printf("sn=%ld ?=? sback = %ld   sback.size=%d\n", sn, sback, slen );
    	assert( sback == sn );
		printf("negative long conversion done\n\n");
	} else {
		// ulong
		printf("unsigned long conversion ...\n");
    	n = strtoul( str,  NULL, 10 );
    	sprintf(tbf, "%lu", n );
    	slen = strlen(tbf);
    	printf("n = %lu size=%d\n", n, slen );
    
    	m.base62FromULong( res, n, 8 );
    	printf("n=%lu   base62: [%s] size=%ld\n", n, res.s(), res.size() );
		assert( res.size() == 8 );

    	m.base62FromULong( res, n, 12 );
    	printf("n=%lu   base62: [%s] size=%ld\n", n, res.s(), res.size() );
		assert( res.size() <= 11 );
    
    	back = m.base62ToULong( res.s() );
    	printf("back=%lu   base62: [%s]\n", back, res.s() );
    	sprintf(tbf, "%lu", back );
    	slen = strlen(tbf);
    	printf("back = %lu size=%d\n", back, slen );
    
    	assert( n == back );
		printf("unsigned long conversion done\n\n");


		// + long
        dnl("t83737");
		printf("positive long conversion ...\n");
    	long sn;
    	sn = strtol( str,  NULL, 10 );
    	sprintf(tbf, "%ld", sn );
    	slen = strlen(tbf);
    	printf("sn = %ld size=%d\n", sn, slen );
    
    	m.base62FromLong( res, sn, 16 );
    	printf("sn=%ld   base62: [%s] size=%ld\n", sn, res.s(), res.size() );
    
    	long sback = m.base62ToLong( res.s() );
    	printf("sback=%ld   base62: [%s]\n", sback, res.s() );
    	sprintf(tbf, "%ld", sback );
    	slen = strlen(tbf);
    	printf("sn=%ld ?=? sback = %ld   sback.size=%d\n", sn, sback, slen );
    	assert( sback == sn );
		printf("positive long conversion done\n\n");
	}

	printf("int %ld bytes\n", sizeof(int) );
	printf("long %ld bytes\n", sizeof(long) );
	printf("long long %ld bytes\n", sizeof(long long) );
	printf("ulong %ld bytes\n", sizeof(unsigned long) );
	printf("float %ld bytes\n", sizeof(float) );
	printf("double %ld bytes\n", sizeof(double) );
	printf("long double %ld bytes\n", sizeof(long double) );

	printf("DBL_MAX = %f\n", DBL_MAX );
	printf("DBL_MIN = %f\n", DBL_MIN );

	printf("FLT_EPSILON = %f\n", FLT_EPSILON );
	printf("DBL_EPSILON = %f\n", DBL_EPSILON );
	printf("LDBL_EPSILON = %Lf\n", LDBL_EPSILON );

	printf("\n");
    dnl("t0003");
    char *pend;

	// + double 
	printf("double conversion ...\n");
	double sd;
	//sd = atof( str );
	sd = strtold( str, &pend);
	sprintf(tbf, "%.9f", sd );
	slen = strlen(tbf);
	printf("str=[%s] sd = %.9f size=%d\n", str, sd, slen );

    /***
	int sig = 8;
    printf("t1220 sig=%d\n", sig );
	m.base62FromDouble( res, sd, -1, sig );
	printf("t0101010 sd=%.9f   base62: [%s] size=%ld sig=%d\n", sd, res.s(), res.size(), sig );

    Jstr origs;
    m.fromBase62( origs, res );
    printf("orig=[%s]  b62str=[%s] origs=[%s]\n", str, res.s(), origs.s() );

	double sback = m.base62ToDouble( res.s() );
	printf("sback=%f   base62: [%s]\n", sback, res.s() );
	sprintf(tbf, "%f", sback );
	slen = strlen(tbf);
	printf("sd=%f ?=? sback = %f   sback.size=%d\n", sd, sback, slen );
	assert( fabs(sd-sback) < 0.0001 );
	printf("double conversion done sig=%d\n\n", sig);
    dnl("t9816");

	// + long double 
	printf("long double conversion [%s] ...\n", str);
	long double sdd = strtold( str, &pend);

	sprintf(tbf, "%.11Lf", sdd );
	slen = strlen(tbf);
	printf("sdd = %.11Lf size=%d\n", sdd, slen );

    int totalB62Len = 18;
	sig = 8;
	m.base62FromLongDouble( res, sdd, totalB62Len, sig );
	printf("sdd=%.11Lf   base62: [%s] size=%ld\n", sdd, res.s(), res.size() );

	long double lsback = m.base62ToLongDouble( res.s() );
	printf("lsback=%.11Lf   base62: [%s]\n", lsback, res.s() );
	sprintf(tbf, "%.11Lf", lsback );
	slen = strlen(tbf);
	printf("sdd=%.11Lf ?=? lsback = %.11Lf   sback.size=%d\n", sdd, lsback, slen );
	assert( fabs(sdd-lsback) < 0.0001 );
	printf("long double conversion done\n\n");
    dnl("t9872016");


	printf("long double conversion [%s] free format of b62 ...\n", str);
    totalB62Len = -1;
	sig = -1;
	m.base62FromLongDouble( res, sdd, totalB62Len, sig );
	printf("sdd=%.11Lf   base62: [%s] size=%ld\n", sdd, res.s(), res.size() );

	lsback = m.base62ToLongDouble( res.s() );
	printf("lsback=%.11Lf   base62: [%s]\n", lsback, res.s() );
	sprintf(tbf, "%.11Lf", lsback );
	slen = strlen(tbf);
	printf("sdd=%.11Lf ?=? lsback = %.11Lf   sback.size=%d\n", sdd, lsback, slen );
	assert( fabs(sdd-lsback) < 0.0000001 );
	printf("long double conversion done\n\n");
    dnl("t907016");


    printf("String conversion ...\n");
	Jstr normal, b62s, backs;
	normal = "434399";
	m.toBase62( b62s, normal );
	printf("normal=[%s] ==>  b62s=[%s]\n", normal.s(), b62s.s() );
	m.fromBase62( backs, b62s );
	printf("b62s=[%s] ==> backs=[%s]\n", b62s.s(), backs.s() );
	assert( normal == backs );
	dnl("t101");

	normal = "-29434399";
	m.toBase62( b62s, normal );
	printf("normal=[%s] ==>  b62s=[%s]\n", normal.s(), b62s.s() );
	m.fromBase62( backs, b62s );
	printf("b62s=[%s] ==> backs=[%s]\n", b62s.s(), backs.s() );
	assert( normal == backs );
	dnl("t201");

	normal = "29434399.883101";
	in("t2723 normal=%s", normal.s() );
	m.toBase62( b62s, normal );
	double nv = m.base62ToDouble( b62s.s());
	printf("normal=[%s] ==>  b62s=[%s][%f]\n", normal.s(), b62s.s(), nv );
	m.fromBase62( backs, b62s );
	printf("b62s=[%s] ==> backs=[%s]\n", b62s.s(), backs.s() );
	assert( normal == backs );
	dnl("t203");

	normal = "-729434399.883101";
	m.toBase62( b62s, normal );
	nv = m.base62ToDouble( b62s.s());
	printf("normal=[%s] ==>  b62s=[%s][%f]\n", normal.s(), b62s.s(), nv );

	m.fromBase62( backs, b62s );
	printf("b62s=[%s] ==> backs=[%s]\n", b62s.s(), backs.s() );
	assert( normal == backs );
	dnl("t204");

	long r1, r2;
	Jstr b1, b2;
	r1 = 100023;
	r2 = 100033;
   	m.base62FromLong( b1, r1, 9 );
   	m.base62FromLong( b2, r2, 9 );
	assert( b2 > b1 );

	printf("r1=%ld  b1=[%s]\n", r1, b1.s() );
	printf("r2=%ld  b2=[%s]\n", r2, b2.s() );
	dnl("t1001");

	r1 = -100023;
	r2 = -100033;
   	m.base62FromLong( b1, r1 );
   	m.base62FromLong( b2, r2 );
	assert( b1 > b2 );
	printf("r1=%ld  b1=[%s]\n", r1, b1.s() );
	printf("r2=%ld  b2=[%s]\n", r2, b2.s() );
	dnl("t208");


	double f1, f2;
	int normalsig = 10;
	sig = m.base62Width(normalsig);
	in("t2330 normalsig=%d b62sig=%d", normalsig, sig );
	f1 = 100023.9383;

   	//m.base62FromDouble( b1, f1, 12, sig );
   	//m.base62FromDouble( b2, f2, 12, sig );
   	m.base62FromDouble( b1, f1);
   	double sback1 = m.base62ToDouble( b1.s() );
	printf("f1=%f  sback1=%f\n", f1, sback1);
	printf("f1=%.18f  b1=[%s] b1.double=[%.18f]\n", f1, b1.s(), sback1 );
	printf("fabs(f1 -sback1)=%.18f DBL_EPSILON=%.18f\n", fabs(f1 -sback1), DBL_EPSILON );
	assert( fabs(f1 -sback1) < 0.0001 );
	dnl("t210-f2");

	f2 = 100023.03938;
   	m.base62FromDouble( b2, f2);
   	double sback2 = m.base62ToDouble( b2.s() );
	printf("f2=%f  sback2=%f\n", f2, sback2);
	printf("f2=%f  b2=[%s] b2.double=[%f]\n", f2, b2.s(), sback2 );
	assert( fabs(f2 -sback2) < 0.00001 );

	dnl("t211-order");

	assert( b1 > b2 );
	dnl("t202929");

	assert( fabs(sback1 - f1) < 0.000001 );
	assert( fabs(sback2 - f2) < 0.000001 );
	dnl("t292929");


	f1 = -100023.9383;
	f2 = -100023.03938;

   	m.base62FromDouble( b1, f1, -1, sig );
   	m.base62FromDouble( b2, f2, -1, sig );
   	sback1 = m.base62ToDouble( b1.s() );
   	sback2 = m.base62ToDouble( b2.s() );
	printf("f1=%f  b1=[%s] b1.double=%f\n", f1, b1.s(), sback1 );
	printf("f2=%f  b2=[%s] b1.double=%f\n", f2, b2.s(), sback2 );
	assert( b1 < b2 );
	dnl("t209");

    in("t3448 b1=[%s] back ..", b1.s() );
   	sback1 = m.base62ToDouble( b1.s() );
	in("f1=%f  sback1=%f", f1, sback1);

	assert( fabs(sback1 - f1) < 0.000001 );
	assert( fabs(sback2 - f2) < 0.000001 );
	dnl("t219");

    Jstr c = "9";
    Jstr b62n;
    JagMath::toBase62( b62n, c, false );
    i("c=[%s] ==> b62n=[%s]\n", c.s(), b62n.s() );
    Jstr bn;
    JagMath::fromBase62( bn, b62n);
    i("b62n=[%s] --> bn=[%s]\n", b62n.s(), bn.s() );
    assert( bn == c ); 


    c = "93";
    JagMath::toBase62( b62n, c, false );
    i("c=[%s] ==> b62n=[%s]\n", c.s(), b62n.s() );
    JagMath::fromBase62( bn, b62n);
    i("b62n=[%s] --> bn=[%s]\n", b62n.s(), bn.s() );
    assert( bn == c ); 

    c = "2398";
    JagMath::toBase62( b62n, c, false );
    i("c=[%s] ==> b62n=[%s]\n", c.s(), b62n.s() );
    JagMath::fromBase62( bn, b62n);
    i("b62n=[%s] --> bn=[%s]\n", b62n.s(), bn.s() );
    assert( bn == c ); 

    //JagMath::base62FromDouble( Jstr &res, double n, int b62TotalWidth, int b62sig )
    Jstr db62;
    double dn = 2769.333;
    JagMath::base62FromDouble( db62, dn, 10, 3);
    in("t031119 base62FromDouble dn=%f  db62=[%s] sz=%d", dn, db62.s(), db62.size() );

    //Jstr norm2("-500.00");
    Jstr norm2("-500.");
    Jstr b622;
    JagMath::toBase62( b622, norm2);
    in("t23308 norm2=[%s]  --> b62 [%s]", norm2.s(), b622.s() );

    Jstr norm3;
    JagMath::fromBase62( norm3, b622 );
    in("t23308 norm3=[%s] <--- b62 [%s]", norm3.s(), b622.s() );


    double fn = -500;
    Jstr b62;
    JagMath::base62FromDouble( b62, fn, 30, 6 );
    in("t56003  fn=%f b62=[%s]", fn, b62.s() );
    JagMath::fromBase62( norm3, b62 );
    in("s9801002 norm=[%s] <--- b62=[%s]", norm3.s(),  b62.s() );


    long double fn2 = -500;
    JagMath::base62FromLongDouble( b62, fn2, 30, 6 );
    in("t9801003 b62=[%s]", b62.s() );

    JagMath::fromBase62( norm3, b62 );
    in("t9801003 norm=[%s] <--- b62=[%s]", norm3.s(),  b62.s() );

    Jstr nbuf("2");
    Jstr clus;
    JagMath::toBase62(clus, nbuf, false);
    in("t2923309 nbuf=[%s]   clus=[%s]", nbuf.s(), clus.s() );
    ***/

}

void test_base254( char *str )
{
	// str is "84938948394" or "-4383493493"

	if ( str == NULL ) {
        str = strdup("1000");
	}

	JagMath m;
	Jstr norm, res;
	unsigned long n;
	unsigned long back;
	char tbf[64];
	int slen;

	sprintf(tbf, "%lu", ULONG_MAX );
	slen = strlen(tbf);
	i("ULONG_MAX = %lu size=%d\n", ULONG_MAX, slen );

	sprintf(tbf, "%ld", LONG_MAX );
	slen = strlen(tbf);
	i("LONG_MAX = %ld size=%d\n", LONG_MAX, slen );

	sprintf(tbf, "%ld", LONG_MIN );
	slen = strlen(tbf);
	i("LONG_MIN = %ld size=%d\n", LONG_MIN, slen );
	i("LONG_MIN = %ld size=%d\n", LONG_MIN, slen );
	i("\n\n");

    // display normal length and b6 length
    in("normal len --> b254 len:");
    for ( int i = 1; i < 25; ++i ) {
        int b254len = m.base254WidthSlow(i);
        in("i=%d    b254.len=%d", i, b254len);
    }

	if ( '-' == *str ) {
    	///////////// negative long /////////////
		printf("negative long conversion ...\n");
    	long sn;
    	sn = strtol( str,  NULL, 10 );
    	sprintf(tbf, "%ld", sn );
    	slen = strlen(tbf);
    	printf("sn = %ld size=%d\n", sn, slen );
    
    	m.base254FromLong( res, sn );
    	printf("sn=%ld   base254: [%s] size=%ld\n", sn, res.s(), res.size() );
    
    	long sback = m.base254ToLong( res.s() );
    	printf("sback=%ld   base254: [%s]\n", sback, res.s() );
    	sprintf(tbf, "%ld", sback );
    	slen = strlen(tbf);
    	printf("sn=%ld ?=? sback = %ld   sback.size=%d\n", sn, sback, slen );
    	assert( sback == sn );
		printf("negative long conversion done\n\n");

    	sback = m.base254ToLongLen( res.s(), res.size() );
    	printf("sback=%ld   base254: [%s]\n", sback, res.s() );
    	sprintf(tbf, "%ld", sback );
    	slen = strlen(tbf);
    	printf("sn=%ld ?=? sback = %ld   sback.size=%d\n", sn, sback, slen );
    	assert( sback == sn );
		printf("negative long conversion done\n\n");


	} else {
		// ulong
		printf("unsigned long conversion ...\n");
    	n = strtoul( str,  NULL, 10 );
    	sprintf(tbf, "%lu", n );
    	slen = strlen(tbf);
    	printf("n = %lu size=%d\n", n, slen );
    
    	m.base254FromULong( res, n, 8 );
    	printf("n=%lu   base254: [%s] size=%ld\n", n, res.s(), res.size() );
		assert( res.size() == 8 );

    	m.base254FromULong( res, n, 12 );
    	printf("n=%lu   base254: [%s] size=%ld\n", n, res.s(), res.size() );
		assert( res.size() <= 11 );
    
    	back = m.base254ToULong( res.s() );
    	printf("back=%lu   base254: [%s]\n", back, res.s() );
    	sprintf(tbf, "%lu", back );
    	slen = strlen(tbf);
    	printf("back = %lu size=%d\n", back, slen );
    
    	assert( n == back );
		printf("unsigned long conversion done\n\n");


		// + long
        dnl("t83737");
		printf("positive long conversion ...\n");
    	long sn;
    	sn = strtol( str,  NULL, 10 );
    	sprintf(tbf, "%ld", sn );
    	slen = strlen(tbf);
    	printf("sn = %ld size=%d\n", sn, slen );
    
    	m.base254FromLong( res, sn, 16 );
    	printf("sn=%ld   base254: [%s] size=%ld\n", sn, res.s(), res.size() );
    
    	long sback = m.base254ToLong( res.s() );
    	printf("sback=%ld   base254: [%s]\n", sback, res.s() );
    	sprintf(tbf, "%ld", sback );
    	slen = strlen(tbf);
    	printf("sn=%ld ?=? sback = %ld   sback.size=%d\n", sn, sback, slen );
    	assert( sback == sn );
		printf("positive long conversion done\n\n");

    	sback = m.base254ToLongLen( res.s(), res.size() );
    	printf("sback=%ld   base254: [%s]\n", sback, res.s() );
    	sprintf(tbf, "%ld", sback );
    	slen = strlen(tbf);
    	printf("sn=%ld ?=? sback = %ld   sback.size=%d\n", sn, sback, slen );
    	assert( sback == sn );
		printf("positive long conversion done\n\n");

	}

	printf("int %ld bytes\n", sizeof(int) );
	printf("long %ld bytes\n", sizeof(long) );
	printf("long long %ld bytes\n", sizeof(long long) );
	printf("ulong %ld bytes\n", sizeof(unsigned long) );
	printf("float %ld bytes\n", sizeof(float) );
	printf("double %ld bytes\n", sizeof(double) );
	printf("long double %ld bytes\n", sizeof(long double) );

	printf("DBL_MAX = %f\n", DBL_MAX );
	printf("DBL_MIN = %f\n", DBL_MIN );

	printf("FLT_EPSILON = %f\n", FLT_EPSILON );
	printf("DBL_EPSILON = %f\n", DBL_EPSILON );
	printf("LDBL_EPSILON = %Lf\n", LDBL_EPSILON );

	printf("\n");
    dnl("t0003");
    char *pend;

	// + double 
	printf("double conversion ...\n");
	double sd;
	//sd = atof( str );
	sd = strtold( str, &pend);
	sprintf(tbf, "%.9f", sd );
	slen = strlen(tbf);
	printf("str=[%s] sd = %.9f size=%d\n", str, sd, slen );

	int sig = 8;
    printf("t1220 sig=%d\n", sig );
	m.base254FromDoubleStr( res, str, -1, sig );
	printf("t0101010 sd=%.9f   base254: [%s] size=%ld sig=%d\n", sd, res.s(), res.size(), sig );

    Jstr origs;
    m.fromBase254( origs, res );
    printf("orig=[%s]  b254str=[%s] origs=[%s]\n", str, res.s(), origs.s() );

	double sback = m.base254ToDouble( res.s() );
	printf("sback=%f   base254: [%s]\n", sback, res.s() );
	sprintf(tbf, "%f", sback );
	slen = strlen(tbf);
	printf("sd=%f ?=? sback = %f   sback.size=%d\n", sd, sback, slen );
	assert( fabs(sd-sback) < 0.0001 );
	printf("double conversion done sig=%d\n\n", sig);
    dnl("t9816");

	// + long double 
	printf("long double conversion [%s] ...\n", str);
	long double sdd = strtold( str, &pend);

	sprintf(tbf, "%.11Lf", sdd );
	slen = strlen(tbf);
	printf("sdd = %.11Lf size=%d\n", sdd, slen );

    int totalB255Len = 18;
	sig = 8;
	m.base254FromLongDoubleStr( res, str, totalB255Len, sig );
	printf("sdd=%.11Lf   base254: [%s] size=%ld\n", sdd, res.s(), res.size() );

	long double lsback = m.base254ToLongDouble( res.s() );
	printf("lsback=%.11Lf   base254: [%s]\n", lsback, res.s() );
	sprintf(tbf, "%.11Lf", lsback );
	slen = strlen(tbf);
	printf("sdd=%.11Lf ?=? lsback = %.11Lf   sback.size=%d\n", sdd, lsback, slen );
	assert( fabs(sdd-lsback) < 0.0001 );
	printf("long double conversion done\n\n");
    dnl("t9872016");


	printf("long double conversion [%s] free format of b254 ...\n", str);
    totalB255Len = -1;
	sig = -1;
	m.base254FromLongDoubleStr( res, str, totalB255Len, sig );
	printf("sdd=%.11Lf   base254: [%s] size=%ld\n", sdd, res.s(), res.size() );

	lsback = m.base254ToLongDouble( res.s() );
	printf("lsback=%.11Lf   base254: [%s]\n", lsback, res.s() );
	sprintf(tbf, "%.11Lf", lsback );
	slen = strlen(tbf);
	printf("sdd=%.11Lf ?=? lsback = %.11Lf   sback.size=%d\n", sdd, lsback, slen );
	assert( fabs(sdd-lsback) < 0.0000001 );
	printf("long double conversion done\n\n");
    dnl("t907016");


    printf("String conversion ...\n");
	Jstr normal, b254s, backs;
	normal = "434399";
	m.toBase254( b254s, normal );
	printf("normal=[%s] ==>  b254s=[%s]\n", normal.s(), b254s.s() );
	m.fromBase254( backs, b254s );
	printf("b254s=[%s] ==> backs=[%s]\n", b254s.s(), backs.s() );
	assert( normal == backs );
	dnl("t101");

	normal = "-29434399";
	m.toBase254( b254s, normal );
	printf("normal=[%s] ==>  b254s=[%s]\n", normal.s(), b254s.s() );
	m.fromBase254( backs, b254s );
	printf("b254s=[%s] ==> backs=[%s]\n", b254s.s(), backs.s() );
	assert( normal == backs );
	dnl("t201");

	normal = "29434399.883101";
	in("t2723 normal=%s m.toBase254()..", normal.s() );
	m.toBase254( b254s, normal );
	double nv = m.base254ToDouble( b254s.s());
	printf("normal=[%s] ==>  b254s=[%s][%f]\n", normal.s(), b254s.s(), nv );
	m.fromBase254( backs, b254s );
	printf("b254s=[%s] ==> backs=[%s]\n", b254s.s(), backs.s() );
	printf("normal=[%s] =?=  backs=[%s]\n", normal.s(), backs.s() );
	assert( normal == backs );
	dnl("t203");

	normal = "-729434399.883101";
	m.toBase254( b254s, normal );
	nv = m.base254ToDouble( b254s.s());
	printf("normal=[%s] ==>  b254s=[%s][%f]\n", normal.s(), b254s.s(), nv );

	m.fromBase254( backs, b254s );
	printf("b254s=[%s] ==> backs=[%s]\n", b254s.s(), backs.s() );
	assert( normal == backs );
	dnl("t204");


	normal = "0.883101";
	m.toBase254( b254s, normal );
	nv = m.base254ToDouble( b254s.s());
	printf("normal=[%s] ==>  b254s=[%s][%f]\n", normal.s(), b254s.s(), nv );

	m.fromBase254( backs, b254s );
	printf("b254s=[%s] ==> backs=[%s]\n", b254s.s(), backs.s() );
	assert( normal == backs );
	dnl("t305");

	normal = "-0.883101";
	m.toBase254( b254s, normal );
	nv = m.base254ToDouble( b254s.s());
	printf("normal=[%s] ==>  b254s=[%s][%f]\n", normal.s(), b254s.s(), nv );

	m.fromBase254( backs, b254s );
	printf("b254s=[%s] ==> backs=[%s]\n", b254s.s(), backs.s() );
	assert( normal == backs );
	dnl("t306");


	long r1, r2;
	Jstr b1, b2;
	r1 = 100023;
	r2 = 100033;
   	m.base254FromLong( b1, r1, 9 );
   	m.base254FromLong( b2, r2, 9 );
	assert( b2 > b1 );

	printf("r1=%ld  b1=[%s]\n", r1, b1.s() );
	printf("r2=%ld  b2=[%s]\n", r2, b2.s() );
	dnl("t1001");

	r1 = -100023;
	r2 = -100033;
   	m.base254FromLong( b1, r1 );
   	m.base254FromLong( b2, r2 );
	assert( b1 > b2 );
	printf("r1=%ld  b1=[%s]\n", r1, b1.s() );
	printf("r2=%ld  b2=[%s]\n", r2, b2.s() );
	dnl("t208");


	int normalsig = 10;
	sig = m.base254Width(normalsig);
	in("t2330 normalsig=%d b254sig=%d", normalsig, sig );

   	//m.base254FromDouble( b1, f1, 12, sig );
   	//m.base254FromDouble( b2, f2, 12, sig );
    char f1[32];
    strcpy(f1, "100023.9383");
   	m.base254FromDoubleStr( b1, f1);
   	double sback1 = m.base254ToDouble( b1.s() );
	printf("f1=%s  sback1=%f\n", f1, sback1);
	assert( fabs(jagatof(f1) -sback1) < 0.0001 );
	dnl("t210-f2");

    strcpy(f1, "100023.03938");
   	m.base254FromDoubleStr( b2, f1);
    printf("b254 wsig=%d\n", m.wSig(b2)); 
   	double sback2 = m.base254ToDouble( b2.s() );
	printf("f1=%s  sback2=%f\n", f1, sback2);
    fflush(stdout);
	assert( fabs(jagatof(f1) -sback2) < 0.0001 );

	dnl("t211-order");

	assert( b1 > b2 );
	dnl("t202929");

    in("t3448 b2=[%s] back ..", b2.s() );
   	sback2 = m.base254ToDouble( b2.s() );
	i("f1=%f  sback2=%f\n", f1, sback2);

	dnl("t219");

    Jstr c = "9";
    Jstr b254n;
    JagMath::toBase254( b254n, c, false );
    i("c=[%s] ==> b254n=[%s]\n", c.s(), b254n.s() );
    Jstr bn;
    JagMath::fromBase254( bn, b254n);
    i("b254n=[%s] --> bn=[%s]\n", b254n.s(), bn.s() );
    assert( bn == c ); 


    c = "93";
    JagMath::toBase254( b254n, c, false );
    i("c=[%s] ==> b254n=[%s]\n", c.s(), b254n.s() );
    JagMath::fromBase254( bn, b254n);
    i("b254n=[%s] --> bn=[%s]\n", b254n.s(), bn.s() );
    assert( bn == c ); 

    c = "2398";
    JagMath::toBase254( b254n, c, false );
    i("c=[%s] ==> b254n=[%s]\n", c.s(), b254n.s() );
    JagMath::fromBase254( bn, b254n);
    i("b254n=[%s] --> bn=[%s]\n", b254n.s(), bn.s() );
    assert( bn == c ); 

    Jstr db254;
    char fn[32];
    strcpy(fn, "2769.333");
    JagMath::base254FromDoubleStr( db254, fn, 10, 3);
    in("t031119 base254FromDouble fn=%s  db254=[%s] sz=%d", fn, db254.s(), db254.size() );

    Jstr norm1;
    Jstr norm2("-500.");
    Jstr b2542;
    JagMath::toBase254( b2542, norm2);
    in("t23308 norm2=[%s]  --> b254 [%s]", norm2.s(), b2542.s() );

    Jstr norm3;
    JagMath::fromBase254( norm3, b2542 );
    in("t23308 norm3=[%s] <--- b254 [%s]", norm3.s(), b2542.s() );
    printf("\n");


    strcpy(fn, "-500");
    Jstr b254;
    JagMath::base254FromDoubleStr( b254, fn, 30, 6 );
    in("t56203 -500  fn=%s b254=[%s]", fn, b254.s() );
    JagMath::fromBase254( norm3, b254 );
    in("t9801002 255 norm=[%s] <--- b254=[%s]", norm3.s(),  b254.s() );
    assert( fabs( atof(norm3.s()) - atof(fn) )  < 0.0000001 );
    printf("\n");

    strcpy(fn, "-0.32500");
    JagMath::base254FromDoubleStr( b254, fn, 30, 6 );
    in("t56213 -0.32500  fn=%s b254=[%s]", fn, b254.s() );
    JagMath::fromBase254( norm3, b254 );
    in("t9801002 255 norm3=[%s] <--- b254=[%s]", norm3.s(),  b254.s() );
    assert( fabs( atof(norm3.s()) - atof(fn) )  < 0.0000001 );
    printf("\n");

    strcpy(fn, "0.32500");
    JagMath::base254FromDoubleStr( b254, fn, 30, 6 );
    in("t56303 0.32500  fn=%f b254=[%s]", fn, b254.s() );
    JagMath::fromBase254( norm3, b254 );
    in("t9801002 255 norm=[%s] <--- b254=[%s]", norm3.s(),  b254.s() );
    assert( fabs( atof(norm3.s()) - atof(fn) )  < 0.0000001 );
    printf("\n");


    char fn2[64];
    strcpy( fn2, "-500");
    JagMath::base254FromLongDoubleStr( b254, fn2, 30, 6 );
    in("t9801003 -500  b254=[%s]", b254.s() );
    JagMath::fromBase254( norm3, b254 );
    in("t9801003 norm=[%s] <--- b254=[%s]", norm3.s(),  b254.s() );
    printf("\n");

    strcpy( fn2, "-0.3334");
    JagMath::base254FromLongDoubleStr( b254, fn2, 30, 6 );
    in("t9801023  -0.3334  b254=[%s]", b254.s() );
    JagMath::fromBase254( norm3, b254 );
    in("t9801023 norm3=[%s] <--- b254=[%s]", norm3.s(),  b254.s() );
    assert( fabs( atof(norm3.s()) - atof(fn2) )  < 0.0000001 );
    printf("\n");

    strcpy( fn2, "0.3334");
    JagMath::base254FromLongDoubleStr( b254, fn2, 30, 6 );
    in("t9801033 0.3334  b254=[%s]", b254.s() );
    JagMath::fromBase254( norm3, b254 );
    in("t9801033 norm=[%s] <--- b254=[%s]", norm3.s(),  b254.s() );
    assert( fabs( atof(norm3.s()) - atof(fn2) )  < 0.0000001 );
    printf("\n");


    norm2 = "-237539";
    JagMath::toBase254( b2542, norm2);
    JagMath::fromBase254( norm3, b2542 );
    assert( fabs( atof(norm3.s()) - atof(norm2.s()) )  < 0.0000001 );

    JagMath::fromBase254Len( norm3, b2542.s(), b2542.size() );
    assert( fabs( atof(norm3.s()) - atof(norm2.s()) )  < 0.0000001 );
    printf("\n");

    norm2 = "27037539";
    JagMath::toBase254( b2542, norm2);
    JagMath::fromBase254( norm3, b2542 );
    assert( fabs( atof(norm3.s()) - atof(norm2.s()) )  < 0.0000001 );

    JagMath::fromBase254Len( norm3, b2542.s(), b2542.size() );
    assert( fabs( atof(norm3.s()) - atof(norm2.s()) )  < 0.0000001 );
    printf("\n");

    norm2 = "-5737539.928289";
    JagMath::toBase254( b2542, norm2);
    JagMath::fromBase254( norm3, b2542 );
    assert( fabs( atof(norm3.s()) - atof(norm2.s()) )  < 0.0000001 );

    JagMath::fromBase254Len( norm3, b2542.s(), b2542.size() );
    in("t223380 norm2=[%s] norm3=[%s]", norm2.s(), norm3.s() );
    assert( fabs( atof(norm3.s()) - atof(norm2.s()) )  < 0.0000001 );


    Jstr umaxs;
    JagMath::base254FromULong( umaxs, ULONG_MAX, 30 );
    printf("ULONG_MAX  b254=[%s]\n", umaxs.s() );
    fflush( stdout );

    unsigned long backu =  JagMath::base254ToULong( umaxs );
    printf("backu=%lu  ULONG_MAX=%lu\n", backu, ULONG_MAX );
    fflush( stdout );
    assert( backu == ULONG_MAX );


    Jstr lmaxs;
    JagMath::base254FromLong( lmaxs, LONG_MAX, 12 );
    printf("LONG_MAX  b254=[%s]\n", lmaxs.s() );
    long backl = JagMath::base254ToLong( lmaxs );
    assert( backl == LONG_MAX );

    Jstr lmins;
    JagMath::base254FromLong( lmins, LONG_MIN, 12 );
    printf("LONG_MIN  b254=[%s]\n", lmins.s() );

    long backm = JagMath::base254ToLong( lmins );
    printf("LONG_MIN=%ld  backm=[%ld]\n", LONG_MIN, backm );

    JagMath::base254FromLong( lmins, LONG_MIN+1, 12 );
    printf("LONG_MIN+1  b254=[%s]\n", lmins.s() );

    long backm2 = JagMath::base254ToLong( lmins );
    printf("LONG_MIN+1=%ld  backm=[%ld]\n\n", LONG_MIN+1, backm2 );


    printf("t500290\n");
    norm = "233706647.353";
    JagMath::base254FromDoubleStr(b254s, norm.s(), 9, 3 );
    JagMath::fromBase254(norm2, b254s);
    printf("norm=[%s] --> b254s=[...][%ld](9.3) --> norm2=[%s]\n", norm.s(), b254s.size(), norm2.s() );
    printf("\n");


    printf("t500294\n");
    JagMath::toBase254(b254s, norm);
    JagMath::fromBase254(norm2, b254s);
    printf("norm=[%s] --> b254s=[...][%ld] --> norm2=[%s]\n", norm.s(),  b254s.size(), norm2.s() );
    printf("\n");


    printf("t500296\n");
    norm = "233706647.353834";
    JagMath::base254FromDoubleStr(b254s, norm.s(), 9, 3 );
    JagMath::fromBase254(norm2, b254s);
    printf("norm=[%s] --> b254s=[...][%ld](9.3) --> norm2=[%s]\n", norm.s(), b254s.size(), norm2.s() );
    printf("\n");


    printf("t500298\n");
    JagMath::toBase254(b254s, norm);
    JagMath::fromBase254(norm2, b254s);
    printf("norm=[%s] --> b254s=[...][%ld] --> norm2=[%s]\n", norm.s(), b254s.size(), norm2.s() );
    printf("\n");


    printf("t500316\n");
    norm = "233706647.38";
    JagMath::base254FromDoubleStr(b254s, norm.s(), 9, 4 );
    JagMath::fromBase254(norm2, b254s);
    printf("norm=[%s] --> b254s=[...][%ld](9.4) --> norm2=[%s]\n", norm.s(), b254s.size(), norm2.s() );
    printf("\n");


    printf("t500318\n");
    JagMath::toBase254(b254s, norm);
    JagMath::fromBase254(norm2, b254s);
    printf("norm=[%s] --> b254s=[...][%ld] --> norm2=[%s]\n", norm.s(), b254s.size(), norm2.s() );
    printf("\n");


    ////////
    printf("t500330\n");
    norm = "233706647";
    JagMath::base254FromDoubleStr(b254s, norm.s(), 11, 3 );
    JagMath::fromBase254(norm2, b254s);
    printf("norm=[%s] --> b254s=[...][%ld](11.3) --> norm2=[%s]\n", norm.s(), b254s.size(), norm2.s() );
    printf("\n");


    printf("t500332\n");
    JagMath::toBase254(b254s, norm);
    JagMath::fromBase254(norm2, b254s);
    printf("norm=[%s] --> b254s=[...][%ld] --> norm2=[%s]\n", norm.s(), b254s.size(), norm2.s() );
    printf("\n");


    /////// 
    printf("t500340\n");
    norm = "878786386";
    JagMath::base254FromDoubleStr(b254s, norm.s(), 11, 3 );
    JagMath::fromBase254(norm2, b254s);
    printf("norm=[%s] --> b254s=[...][%ld](11.3) --> norm2=[%s]\n", norm.s(), b254s.size(), norm2.s() );
    printf("\n");


    printf("t500342\n");
    JagMath::toBase254(b254s, norm);
    JagMath::fromBase254(norm2, b254s);
    printf("norm=[%s] --> b254s=[...][%ld] --> norm2=[%s]\n", norm.s(), b254s.size(), norm2.s() );
    printf("\n");

    // negative
    printf("t500350\n");
    norm = "-878786386";
    JagMath::base254FromDoubleStr(b254s, norm.s(), 11, 3 );
    JagMath::fromBase254(norm2, b254s);
    printf("norm=[%s] --> b254s=[...][%ld](11.3) --> norm2=[%s]\n", norm.s(), b254s.size(), norm2.s() );
    printf("\n");

    printf("t500352\n");
    JagMath::toBase254(b254s, norm);
    JagMath::fromBase254(norm2, b254s);
    printf("norm=[%s] --> b254s=[...][%ld] --> norm2=[%s]\n", norm.s(), b254s.size(), norm2.s() );
    printf("\n");


    // compare positive
    Jstr b254_1, b254_2;

    norm1 = "2000";
    norm2 = "2000.003";

    JagMath::base254FromDoubleStr(b254_1, norm1.s(), 11, 3 );
    JagMath::base254FromDoubleStr(b254_2, norm2.s(), 11, 3 );

    dumpmem(b254_1.s(), b254_1.size() );
    dumpmem(b254_2.s(), b254_2.size() );

    // compare negative

    norm1 = "-2000";
    norm2 = "-2000.003";

    JagMath::base254FromDoubleStr(b254_1, norm1.s(), 11, 3 );
    JagMath::base254FromDoubleStr(b254_2, norm2.s(), 11, 3 );

    dumpmem(b254_1.s(), b254_1.size() );
    dumpmem(b254_2.s(), b254_2.size() );

}

#include <faiss/IndexFlat.h>
#include <faiss/IndexIVFFlat.h>
#include <faiss/index_io.h>
#include <faiss/invlists/OnDiskInvertedLists.h>
#include <faiss/utils/random.h>

struct Tempfilename {
    //static pthread_mutex_t mutex;

    std::string filename = "/tmp/faiss_tmp_XXXXXX";

    Tempfilename() {
        //pthread_mutex_lock(&mutex);
        int fd = mkstemp(&filename[0]);
        close(fd);
        //pthread_mutex_unlock(&mutex);
    }

    ~Tempfilename() {
        if (access(filename.c_str(), F_OK)) {
            unlink(filename.c_str());
        }
    }

    const char* c_str() {
        return filename.c_str();
    }
};


void test_faiss( )
{
    //////////////////////////////////////////////////////////////
    Tempfilename filename;

    int nlist = 100;
    int code_size = 32;
    int nadd = 1000000;
    //int nadd = 10000;
    std::unordered_map<int, int> listnos_map;

    faiss::OnDiskInvertedLists ivf(nlist, code_size, filename.c_str());
    printf("Open [%s] inv file\n",  filename.c_str() );

    {
        std::vector<uint8_t> code_vec(32);
        std::mt19937 rng;
        std::uniform_real_distribution<> distrib;

        for (int i = 0; i < nadd; i++) {
            double d = distrib(rng);
            int list_no = int(nlist * d * d); // skewed distribution

            int* ar = (int*)code_vec.data();
            ar[0] = i;
            ar[1] = list_no;

            ivf.add_entry(list_no, i, code_vec.data());
            listnos_map[i] = list_no;
        }
    }

    printf("ivf.add_entry nadd=%d done\n", nadd );

    int ntot = 0;
    for (int i = 0; i < nlist; i++) {
        int lst_size = ivf.list_size(i);

        const faiss::idx_t* ids = ivf.get_ids(i);
        const uint8_t* codes = ivf.get_codes(i);


        for (int j = 0; j < lst_size; j++) {
            faiss::idx_t id = ids[j];
            const int* ar = (const int*)&codes[code_size * j];
            if ( ar[0] != id ) {
                abort();
            }

            if ( ar[1] != i ) {
                abort();
            }

            if ( listnos_map[id] != i ) {
                abort();
            }

            ntot++;
        }
    }

    printf("ivf.get_ids nlist=%d ntot=%d done\n", nlist, ntot );

    if ( ntot != nadd ) {
        abort();
    }


    ///////////////////////////////////////////////////////////////////////
    int d = 8;
    int  nq = 200, nb = 1500, k = 10;
    nlist = 30;
    faiss::IndexFlatL2 quantizer(d);
    {
        std::vector<float> x(d * nlist);
        faiss::float_rand(x.data(), d * nlist, 12345);
        quantizer.add(nlist, x.data());
    }

    std::vector<float> xb(d * nb); // 8 * 1500 = 16000
    faiss::float_rand(xb.data(), d * nb, 23456);

    // index
    faiss::IndexIVFFlat index(&quantizer, d, nlist);

    index.add(nb, xb.data());

    std::vector<float> xq(d * nb);
    faiss::float_rand(xq.data(), d * nq, 34567);

    std::vector<float> ref_D(nq * k);
    std::vector<faiss::idx_t> ref_I(nq * k);

    index.search(nq, xq.data(), k, ref_D.data(), ref_I.data());

    Tempfilename filename1, filename2;

    // test add + search
    {
        faiss::IndexIVFFlat index2(&quantizer, d, nlist);

        faiss::OnDiskInvertedLists ivf( index.nlist, index.code_size, filename1.c_str());
        printf("OnDiskInvertedLists [%s]\n", filename1.c_str() );

        index2.replace_invlists(&ivf);

        index2.add(nb, xb.data());

        std::vector<float> new_D(nq * k);
        std::vector<faiss::idx_t> new_I(nq * k);

        index2.search(nq, xq.data(), k, new_D.data(), new_I.data());

        //EXPECT_EQ(ref_D, new_D);
        //EXPECT_EQ(ref_I, new_I);

        write_index(&index2, filename2.c_str());
        printf("write_index [%s] done\n", filename2.c_str() );
    }

    // test io
    {
        printf("read_index [%s] ...\n", filename2.c_str() );

        // index3
        faiss::Index* index3 = faiss::read_index(filename2.c_str());

        std::vector<float> new_D(nq * k);
        std::vector<faiss::idx_t> new_I(nq * k);

        index3->search(nq, xq.data(), k, new_D.data(), new_I.data());

        //EXPECT_EQ(ref_D, new_D);
        //EXPECT_EQ(ref_I, new_I);

        delete index3;
    }

}
