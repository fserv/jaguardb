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

#include <JaguarAPI.h>
#include <JaguarCPPClient.h>
#include <JagUtil.h>
#include <JagHashMap.h>

static void readInputFile( const char *inputFile, int maxLines, Jstr &xname, Jstr &yname, 
			JagVector<Jstr> &xvec, JagVector<Jstr> &yvec, bool &xisvalue, bool &yisvalue );
static void readInputLine( const char *buf, Jstr &xname, Jstr &yname, 
			JagVector<Jstr> &xvec, JagVector<Jstr> &yvec, bool &xisvaliue, bool &yisvalue );

static int make2DBarChart( const char *title, int width, int height,
                    const char *inputFile, const char *outputFile, const char *options );
static int make3DBarChart( const char *title, int width, int height,
                    const char *inputFile, const char *outputFile, const char *options );

static void  makeErrorPage( const char *title, const char *outfile );

JaguarAPI::JaguarAPI()
{
	_jcli = new JaguarCPPClient();
}

JaguarAPI::~JaguarAPI()
{
 	delete _jcli;
}

int JaguarAPI::connect( const char *ipaddress, unsigned short port, 
				const char *username, const char *passwd,
				const char *dbname, const char *unixSocket,
				unsigned long long clientFlag )
{
	/**
	if ( strlen(ipaddress) < 128 ) {
		strcpy(_host, ipaddress );
	}
	**/

	return _jcli->connect( ipaddress, port, username, passwd, dbname, unixSocket, clientFlag );
}

int JaguarAPI::execute( const char *query )
{
	return _jcli->execute( query );
}

int JaguarAPI:: query( const char *query, bool reply )
{
	return _jcli->query( query, reply );
}

int JaguarAPI::reply( bool headerOnly )
{
	return _jcli->reply( headerOnly, true );
}

int JaguarAPI::replyAll()
{
	// while (_jcli->reply() ) {}
	// return 1;
	return _jcli->replyAll( false, true);
}

// get session string
const char * JaguarAPI::getSession()
{
	return _jcli->getSession();
}

// get session string
const char * JaguarAPI::getDatabase()
{
	return _jcli->getDatabase();
}

// client receives a row
int JaguarAPI::printRow()
{
	return _jcli->printRow();
}

// client receives a row
char *JaguarAPI::getRow()
{
	return _jcli->getRow();
}

// get n-th column in the row
// NULL if not found; malloced char* if found, must be freed later
char* JaguarAPI::getNthValue( int nth )
{
	return _jcli->getNthValue( nth );
}


// get error string from row
// NULL if no error; Must be freeed after use
const char* JaguarAPI::error( )
{
	return _jcli->error();
}
    
// row hasError?
// 1: yes  0: no
int JaguarAPI:: hasError( )
{
	return _jcli->hasError();
}
    
// free all memory of row fields, if true, free _row->data only
int JaguarAPI::freeResult()
{
	return _jcli->freeResult();
}

// returns a pointer to char string as value for name
// The buffer needs to be freed after use
char * JaguarAPI::getValue( const char *name )
{
	return _jcli->getValue( name );
}

// caller should free the pointer if not NULL
char *JaguarAPI::getLastUuid()
{
	Jstr uuid = _jcli->getLastUuid();
	if ( uuid.size() < 0 ) return NULL;
	return strdup( uuid.c_str() );
}

// return curtent cluser number
int JaguarAPI::getCurrentCluster()
{
	return _jcli->getCurrentCluster();
}
int JaguarAPI::getCluster()
{
	return _jcli->getCurrentCluster();
}

// geojson all data
char * JaguarAPI::getAll( )
{
	return _jcli->getAll();
}

char * JaguarAPI::getAllByName( const char *name )
{
	return _jcli->getAllByName( name );
}

char *JaguarAPI::getAllByIndex( int nth )
{
	return _jcli->getAllByIndex( nth );
}

void JaguarAPI::printAll()
{
	_jcli->printAll();
}
    
// returns a integer
// 1: if name exists in row; 0: if name does not exists in row
int JaguarAPI::getInt(  const char *name, int *value )
{
	return _jcli->getInt( name, value );
}
    
// returns a long long
// 1: if name exists in row; 0: if name does not exists in row
int JaguarAPI::getLong(  const char *name, long long *value )
{
	return _jcli->getLong( name, value );
}
    
// returns a float value
// 1: if name exists in row; 0: if name does not exists in row
int JaguarAPI::getFloat(  const char *name, float *value )
{
	return _jcli->getFloat( name, value );
}
    
// returns a float value
// 1: if name exists in row; 0: if name does not exists in row
int JaguarAPI::getDouble(  const char *name, double *value )
{
	return _jcli->getDouble( name, value );
}

    
// return data buffer in row  and key/value length
const char *JaguarAPI::getMessage( )
{
	return _jcli->getMessage();
}

// return data string in json format
const char *JaguarAPI::jsonString( )
{
	return _jcli->jsonString();
}
    
// close and free up memory
void JaguarAPI::close()
{
	return _jcli->close();
}

int JaguarAPI::getColumnCount()
{
	return  _jcli->getColumnCount();
}

char *JaguarAPI::getCatalogName( int col )
{
	return  _jcli->getCatalogName( col );
}

char *JaguarAPI::getColumnClassName( int col )
{
	return  _jcli->getColumnClassName( col );
}

int JaguarAPI::getColumnDisplaySize( int col )
{
	return  _jcli->getColumnDisplaySize( col );
}

char *JaguarAPI::getColumnLabel( int col )
{
	return  _jcli->getColumnLabel( col );
}

char *JaguarAPI::getColumnName( int col )
{
	return  _jcli->getColumnName( col );
}

int JaguarAPI::getColumnType( int col )
{
	return  _jcli->getColumnType( col );
}

char *JaguarAPI::getColumnTypeName( int col )
{
	return  _jcli->getColumnTypeName( col );
}

int JaguarAPI::getScale( int col )
{
	return  _jcli->getScale( col );
}

char *JaguarAPI::getSchemaName( int col )
{
	return  _jcli->getSchemaName( col );
}

char *JaguarAPI::getTableName( int col )
{
	return  _jcli->getTableName( col );
}

bool JaguarAPI::isAutoIncrement( int col )
{
	return  _jcli->isAutoIncrement( col );
}

bool JaguarAPI::isCaseSensitive( int col )
{
	return  _jcli->isCaseSensitive( col );
}

bool JaguarAPI::isCurrency( int col )
{
	return  _jcli->isCurrency( col );
}

bool JaguarAPI::isDefinitelyWritable( int col )
{
	return  _jcli->isDefinitelyWritable( col );
}

int  JaguarAPI::isNullable( int col )
{
	return  _jcli->isNullable( col );
}

bool JaguarAPI::isReadOnly( int col )
{
	return  _jcli->isReadOnly( col );
}

bool JaguarAPI::isSearchable( int col )
{
	return  _jcli->isSearchable( col );
}

bool JaguarAPI::isSigned( int col )
{
	return  _jcli->isSigned( col );
}

const char* JaguarAPI::getHost()
{
	if ( _jcli->getHost().size() > 0 ) {
		return _jcli->getHost().c_str();
	}
	return "";
}

bool JaguarAPI::allSocketsBad()
{
	return  _jcli->_allSocketsBad;
}

long JaguarAPI::getMessageLen()
{
	return _jcli->getMessageLen();
}

void JaguarAPI::setDatcType( int srcType, int destType )
{
	_jcli->_datcSrcType = srcType;
	_jcli->_datcDestType = destType;
}

void JaguarAPI::setRedirectSock( void *psock )
{
	JAGSOCK *p = (JAGSOCK*) psock;
	_jcli->_redirectSock = *p;
}

int JaguarAPI::datcDestType() {
	return _jcli->_datcDestType;
}

int JaguarAPI::datcSrcType() {
	return _jcli->_datcSrcType;
}

void JaguarAPI::setDebug( bool flag )
{
	_jcli->setDebug( flag );
}

long JaguarAPI::getQueryCount()
{
	return _jcli->_seq;
}

long JaguarAPI::sendDirectToSockAll( const char *mesg, long len, bool nohdr )
{
	return _jcli->sendDirectToSockAll( mesg, len, nohdr );
}

long JaguarAPI::sendRawDirectToSockAll( const char *mesg, long len )
{
	return _jcli->sendDirectToSockAll( mesg, len, true );
}

long JaguarAPI::recvDirectFromSockAll( char *&buf, char *hdr )
{
	return _jcli->recvDirectFromSockAll( buf, hdr );
}

long JaguarAPI::recvRawDirectFromSockAll( char *&buf, long len )
{
	return _jcli->recvRawDirectFromSockAll( buf, len );
}

/***
<!doctype html>
    <html>
    <head>
    	<title>ECharts Sample</title>
    	<script src="http://echarts.baidu.com/dist/echarts.min.js"></script>
    </head>
    <body>
    	<div id="chart" style="width: 500px; height: 350px;"></div>
    	<script>
    		var chart = document.getElementById('chart');
    		var myChart = echarts.init(chart);
    		var option = {
    			title: { text: 'ECharts Sample' },
    			tooltip: { },
    			legend: { data: [ 'Sales' ] },
    			xAxis: { data: [ "shirt", "cardign", "chiffon shirt", "pants", "heels", "socks" ] },
    			yAxis: { },
    			series: [{
    				name: 'Sales',
    				type: 'bar',
    				data: [5, 20, 36, 10, 10, 20]
    			}]
    		};
    		myChart.setOption(option);
    	</script>
    </body>
    </html>

***/
// plotting graph
// 0: error  1: success
// inputFile has  abc:[ssss] xde:[fffff] ddd:[eeee] format in each line
// outputFile is a html file using baidu echart js
// options "x2=rrr arg2=rrr arg4=eee"
int  JaguarAPI::makeGraph( const char *type, const char *title, int width, int height,
                const char *inputFile, const char *outputFile, const char *options )
{
	if ( 0 == strcasecmp(type, "barchart-2d" ) ) {
		return make2DBarChart( title, width, height, inputFile, outputFile, options );
	} else if ( 0 == strcasecmp(type, "barchart-3d" ) ) {
		return make3DBarChart( title, width, height, inputFile, outputFile, options );
	}

	return 0;
}

int make2DBarChart( const char *title, int width, int height,
			    const char *inputFile, const char *outputFile, const char *options )
{
	Jstr page, str;
	/***
	JagHashMap<AbaxString, AbaxString> omap;
	makeMapFromOpt( options, omap );
	AbaxString xtype;
	omap.getValue("xtype", xtype);
	***/
	JagVector<Jstr> xvec, yvec;
	Jstr xname, yname, res;
	bool isxvalue, isyvalue;
	readInputFile( inputFile, 10000, xname, yname, xvec, yvec, isxvalue, isyvalue );
	// inputFile a:[ddd]  b:[eee]   
	// xname: "a"  yname: "b"
	// xvec: "ddd3" "ccc2"     yvec: "aaa1"  "ddd3"
	if ( ! isyvalue ) {
		makeErrorPage( "Second column has no numbers", outputFile );
		return 0;
	}

	int debug = 0;
	FILE *dbg = NULL;
	if ( debug ) {
		dbg = fopen("/tmp/JaguarAPI.log", "w");
	}

	page = "<!doctype html>\n";
	page += "<html><head><title>Jaguar Chart</title>\n";
	// page += "<script src=\"http://echarts.baidu.com/dist/echarts.min.js\"></script>\n";
	page += "<script src=\"/jagjs/echarts.min.js\"></script>\n";
	page += "</head>\n";
	page += "<body leftmargin=20><br><form name=fm>\n";
   	page += "<div id=chart style='width: "+intToStr(width)+ "px; height: " + intToStr(height) + "px;'></div>";
    page += "<script>\n";
    page += " var chart = document.getElementById('chart');\n";
    page += " var myChart = echarts.init(chart);\n";
    page += " var option = {\n";
    page += " 	title: { text: '" + Jstr(title) + "' },\n";
    page += "  	tooltip: { },\n";
    page += Jstr("  		legend: { data: [ '") + xname + "' ] },\n";
    // page += "  		xAxis: { data: [ "shirt", "cardign", "chiffon shirt", "pants", "heels", "socks" ] },\n";
    page += "  	xAxis: [\n";
    page += "  	         {\n";
	if ( isxvalue ) {
    	page += "  		    type: 'value'\n";
	} else {
    	page += "  		    type: 'category',\n";
    	page += "  		    data: [\n";
		page += makeStringFromOneVec( xvec, 1 );
    	page += "  		    ],\n";
	}
    page += "  		  }\n";
    page += "  		],\n";
    page += "  		yAxis: { type: 'value' },\n";
    page += "  		series: [{\n";
    page += Jstr("  			name: '") + yname + "',\n";
    page += "  			type: 'bar',\n";
    page += "  			data: [ ";
	if ( isxvalue ) {
		res = makeStringFromTwoVec( xvec, yvec );
		page += res;
    	// page += "  			data: [ [5, 20], [36, 10], [10, 20] ]\n";
	} else {
		res = makeStringFromOneVec( yvec, 0 );
		page += res;
    	// page += "  			data: [5, 20, 36, 10, 10, 20]\n";
	}

    page += "  			]\n";
    page += "  		}]\n";
    page += "  	};\n";
    page += "  	myChart.setOption(option);\n";
    page += " </script>\n";
    page += " <input type=button value=' << Back ' onclick='window.history.back()' style='width: 200px;height:30px'></form>\n";
    page += "</body>\n";
    page += "</html>\n";

	if ( debug && dbg ) {
		fprintf( dbg, "2d: res=[%s]\n", res.c_str() ); 
	}

	if ( debug && dbg ) {
		fclose( dbg );
	}

	FILE *outf = fopen( outputFile, "w" );
	if ( outf ) {
		fprintf( outf, "%s", page.c_str() );
		fclose( outf );
	} else {
		return 0;
	}

	return 1;
}

int make3DBarChart( const char *title, int width, int height,
			    const char *inputFile, const char *outputFile, const char *options )
{
	return 1;
}

// static
// inputFile a:[ddd]  b:[eee]   
// xname: "a"  yname: "b"
// xvec: "ddd3" "ccc2"     yvec: "aaa1"  "ddd3"
void readInputFile( const char *inputFile, int maxLines, Jstr &xname, Jstr &yname, 
			JagVector<Jstr> &xvec, JagVector<Jstr> &yvec, bool &xisvalue, bool &yisvalue )
{
	xisvalue = false;
	FILE *fp = fopen( inputFile, "r" );
	if ( ! fp ) return;

	char buf[256];
	//char c, *p, *q;
	bool xvaldigit, yvaldigit;
	long tot = 0;
	long xval = 0;
	long yval = 0;
	int lines = 0;
	while ( NULL != fgets( buf, 256, fp ) ) {
		if ( maxLines > 0 && lines > maxLines ) break;
		readInputLine( buf, xname, yname, xvec, yvec, xvaldigit, yvaldigit );
		++tot;
		if ( xvaldigit ) ++xval;
		if ( yvaldigit ) ++yval;
		++lines;
	}
	fclose( fp );

	if ( xval*100 > tot*80 ) { xisvalue  = true; }
	if ( yval*100 > tot*80 ) { yisvalue  = true; }

}

// static
// input buf a:[ddd]  b:[eee]   
// xname: "a"  yname: "b"
// xvec: "ddd3" "ccc2"     yvec: "aaa1"  "ddd3"
void readInputLine( const char *buf, Jstr &xname, Jstr &yname, 
			JagVector<Jstr> &xvec, JagVector<Jstr> &yvec, bool &xisvalue, bool &yisvalue )
{
	if ( buf == NULL ) return;
	if ( *buf == '\0' ) return;
	if ( *buf == '\n' ) return;
	xisvalue = false;
	yisvalue = false;

	char c, *p, *q;
	p = (char*)buf;

	while ( *p == ' ' ) ++p;
	q = p+1;
	while ( *q != ':' ) ++q;
	if ( *q == '\0' || *q == '\n' ) return;

	// x-name part
	c = *q;
	*q = '\0';
	xname = p;
	*q = c;

	// x-value part inside [xxx]
	p = q;
	while ( *p != '[' ) ++p;
	if ( *p == '\0' || *p == '\n' ) return;
	++p;
	q=p+1;
	while ( *q != ']' ) ++q;
	c = *q;
	*q = '\0';
	xvec.append(p);
	if ( isdigit( *p ) ) xisvalue = true;
	*q = c;

	// 2-nd column
	p = q+1; // past], ie. ]p
	while ( *p == ' ' ) ++p;
	if ( *p == '\0' || *p == '\n' ) return;

	// repeat x-name value part
	q = p+1;
	while ( *q != ':' ) ++q;
	if ( *q == '\0' || *q == '\n' ) return;

	// y-name part
	c = *q;
	*q = '\0';
	yname = p;
	*q = c;

	// x-value part inside [xxx]
	p = q;
	while ( *p != '[' ) ++p;
	if ( *p == '\0' || *p == '\n' ) return;
	++p;
	q=p+1;
	while ( *q != ']' ) ++q;
	c = *q;
	*q = '\0';
	yvec.append(p);
	if ( isdigit( *p ) ) yisvalue = true;
	*q = c;
}

// static 
void  makeErrorPage( const char *title, const char *outputFile )
{
	Jstr page;
	page = "<!doctype html>\n";
	page += "<html><head><title>Jaguar Chart</title>\n";
	page += "</head>\n";
	page += Jstr("<body leftmargin=20><h3>") + title + "</h3>\n";
    page += " <input type=button value=' << Back ' onclick='window.history.back()' style='width: 200px;height:30px'></form>\n";
    page += "</body>\n";
    page += "</html>\n";

	FILE *outf = fopen( outputFile, "w" );
	if ( outf ) {
		fprintf( outf, "%s", page.c_str() );
		fclose( outf );
	} 
}
