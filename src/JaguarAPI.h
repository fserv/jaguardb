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
#ifndef _jaguar_api_h_
#define _jaguar_api_h_

#include <stdio.h>

#ifdef _WIN32
#ifdef shared_EXPORTS
#define EXP_LIB_API __declspec(dllexport)
#else
#define EXP_LIB_API __declspec(dllimport)
#endif
#else
#define EXP_LIB_API
#endif

class JaguarCPPClient;

class EXP_LIB_API JaguarAPI
{
  public:
  	JaguarAPI( );
	~JaguarAPI();

	int connect( const char *ipaddress, unsigned short port, 
				const char *username, const char *passwd,
				const char *dbname, const char *connectOpt=NULL,
				unsigned long long clientFlag=0 ); 

    // send DML command (not select) to socket to server
    // 1: OK   0: error
    int execute( const char *query ); 

    // send command to socket to server
    // 1: OK   0: error
    int query( const char *query, bool reply=true ); 

    // 0: error or reaching the last record
    // 1: successful and having more records
    int reply( bool headerOnly=false );

	// Get all reply in a loop until reply becomes false
	// It drains all sent data from server.
    int replyAll();

	// get session string
	const char *getSession();

	// get database string
	const char *getDatabase();

    // client receives a row
    int printRow();

	// get string buffer of row, must be freed by caller
	char *getRow(); 


    // get error string from row
    const char *error( ); 
    
    // row hasError?
    // 1: yes  0: no
    int hasError( );
    
	// free memory used by result
	int freeResult();

    // get n-th column in the row  1--N
    // NULL if not found; malloced char* if found, must be freed later
    const char *getNthValue( int nth );

    // returns a pointer to char string as value for name
    // The buffer needs to be freed after use
    char *getValue( const char *name );
    
    // returns a integer
    // 1: if name exists in row; 0: if name does not exists in row
    int getInt(  const char *name, int *value );
    
    // returns a longlong
    // 1: if name exists in row; 0: if name does not exists in row
    int getLong(  const char *name, long long *value );
    
    // returns a float value
    // 1: if name exists in row; 0: if name does not exists in row
    int getFloat(  const char *name, float *value );
    
    // returns a float value
    // 1: if name exists in row; 0: if name does not exists in row
    int getDouble(  const char *name, double *value );
    
    // return data buffer in row  and key/value length
    const char *getMessage( ); 

	// return data string in json format
	const char *jsonString( );

	// return last uuid
	// caller should free the pointer
	const char *getLastUuid();

	// return curtent cluser number
	int getCurrentCluster();
	int getCluster();

    // close and free up memory
    void close();

    // returns a pointer to char string as value for name in GeoJson
    // The buffer needs to be freed after use
    char *getAllByName( const char *name );

    // get n-th value (GeoJson) 1--N in GeoJson
    // NULL if not found; malloced char* if found, must be freed later
    const char *getAllByIndex( int nth );

    // returns a pointer to char string as value for name in GeoJson
    // The buffer needs to be freed after use
    char *getAll();

    // print all geojson data
	void printAll();


	int   getColumnCount();
	char *getCatalogName( int col );
	char *getColumnClassName( int col );
	int  getColumnDisplaySize( int col );
	char *getColumnLabel( int col );
	char *getColumnName( int col );
	int  getColumnType( int col );
	char *getColumnTypeName( int col );
	int  getScale( int col );
	char *getSchemaName( int col );
	char *getTableName( int col );
	bool isAutoIncrement( int col );
	bool isCaseSensitive( int col );
	bool isCurrency( int col );
	bool isDefinitelyWritable( int col );
	int  isNullable( int col );
	bool isReadOnly( int col );
	bool isSearchable( int col );
	bool isSigned( int col );
	const char *getHost();
	bool  allSocketsBad();
	long  getMessageLen();
	void  setDatcType( int srcType, int destType );
	void  setRedirectSock( void *psockarr );
	int   datcDestType();
	int   datcSrcType();
	void  setDebug( bool flag );
	long  getQueryCount();

	// plotting graph 
	int	 makeGraph( const char *type, const char *title, int width, int height,
					const char *inputFile, const char *outputFile, const char *options );

	long sendDirectToSockAll( const char *mesg, long len, bool nohdr=false );
	long sendRawDirectToSockAll( const char *mesg, long len );
	long recvDirectFromSockAll( char *&buf, char *hdr );
	long recvRawDirectFromSockAll( char *&buf, long len );

  protected:
	JaguarCPPClient *_jcli;

};


#endif

