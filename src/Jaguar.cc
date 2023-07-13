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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "com_jaguar_jdbc_internal_jaguar_Jaguar.h"
#include "JaguarCPPClient.h"

 JNIEXPORT void JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_adbMakeObject
        (JNIEnv *env, jobject obj )
{

	JaguarCPPClient *padb = new JaguarCPPClient( );

	jclass objClass = env->GetObjectClass(obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);

	env->SetLongField( obj, adbFieldID, (jlong)padb );
}


JNIEXPORT void JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_adbClose
  (JNIEnv *env, jobject obj, jlong adbhandle )
{
	/****
	jclass objClass = env->GetObjectClass(obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);

	// JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;
	***/
	JaguarCPPClient *padb = (JaguarCPPClient*)adbhandle;

	// same
	// printf("jni adbClose  adbhandle=%ld     fieldVal=%ld\n", adbhandle, fieldVal );
	// fflush(stdout );

	// padb->close();
	// padb->destroy();
	delete padb;
}


JNIEXPORT jboolean JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_connect(
			JNIEnv *env, jobject obj, jstring host, jint port, 
			jstring username, jstring passwd, jstring dbname, jstring unixSocket, jlong clientFlag)
{
	
	const char *c_host;
	c_host = env->GetStringUTFChars(host,0);

	const char *c_username;
	c_username = env->GetStringUTFChars(username,0);
	// printf("c5333 adbConnect c_user=[%s]\n", c_username );
	// fflush( stdout );
    
	const char *c_passwd;
	c_passwd = env->GetStringUTFChars(passwd,0);
	// printf("c5333 adbConnect c_passwd=[%s]\n", c_passwd );
	// fflush( stdout );
      
	const char *c_dbname;
	c_dbname = env->GetStringUTFChars(dbname,0);
	// printf("c5333 adbConnect c_dbname=[%s]\n", c_dbname );
	// fflush( stdout );
      
	const char *c_unixSocket = NULL;
	if ( unixSocket ) {
		c_unixSocket = env->GetStringUTFChars( unixSocket, 0);
		// printf("c5333 adbConnect c_unixSocket=[%s]\n", c_unixSocket );
		// fflush( stdout );
	}

	jclass objClass = env->GetObjectClass(obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	// printf("connect c++ padb=%0x jlong=%ld\n", padb , (jlong)padb );
	// printf("c5633 adbConnect padb=%0x c_host=[%s] c_user=[%s] c_pass=[%s]\n", padb, c_host, c_username, c_passwd );
      
	jint rc;
 	rc = padb->connect( c_host, port, c_username, c_passwd, c_dbname, c_unixSocket, clientFlag );      

	env->ReleaseStringUTFChars( host, c_host);
	env->ReleaseStringUTFChars( username, c_username );
	env->ReleaseStringUTFChars( passwd, c_passwd );
	env->ReleaseStringUTFChars( dbname, c_dbname );
	env->ReleaseStringUTFChars( unixSocket, c_unixSocket);

	if ( ! rc ) {
		return 0;
	} else {
		return 1;
	}
}


JNIEXPORT jboolean JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_query
  (JNIEnv *env, jobject obj, jstring query){

	const char *c_query;
	c_query = env->GetStringUTFChars(query,0);

	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;
	
	int rc = padb->query( c_query );

	env->ReleaseStringUTFChars( query, c_query );

	if ( ! rc ) {
		return 0;
	} else {
		return 1;
	}
}

JNIEXPORT jboolean JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_execute
  (JNIEnv *env, jobject obj, jstring query){

	const char *c_query;
	c_query = env->GetStringUTFChars(query,0);

	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;
	
	int rc = padb->execute( c_query );

	env->ReleaseStringUTFChars( query, c_query );

	if ( ! rc ) {
		return 0;
	} else {
		return 1;
	}
}

JNIEXPORT jboolean JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_reply
  (JNIEnv *env, jobject obj )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	int rc = padb->reply( false );
	if ( ! rc ) {
		return 0;
	} else {
		return 1;
	}
}

JNIEXPORT jboolean JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_replyHeader
  (JNIEnv *env, jobject obj )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	int rc = padb->reply( true );
	if ( ! rc ) {
		return 0;
	} else {
		return 1;
	}
}


JNIEXPORT jboolean JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_printRow
  (JNIEnv *env, jobject obj )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	int rc = padb->printRow(); 

	if ( ! rc ) {
		return 0;
	} else {
		return 1;
	}
}

JNIEXPORT jboolean JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_printAll
  (JNIEnv *env, jobject obj )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	padb->printAll(); 
	return 1;
}

/***********
JNIEXPORT jboolean JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_initRow
  (JNIEnv *env, jobject obj )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	int rc = padb->initRow( ); 

	if ( ! rc ) {
		return 0;
	} else {
		return 1;
	}
}
*******/


JNIEXPORT jboolean JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_hasError
  (JNIEnv *env, jobject obj )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	int rc =  padb->hasError( ); 
	if ( rc ) return 1;
	else return 0;
}

JNIEXPORT jstring JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_error
  (JNIEnv *env, jobject obj ) 
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	const char *estr = padb->error( );	
	return env->NewStringUTF( estr);
} 

JNIEXPORT jboolean JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_freeRow
  (JNIEnv *env, jobject obj )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	// printf("c8484 jni adbFreeRow()\n");
	int rc = padb->freeRow( );
	if ( rc ) return 1;
	else return 0;
}

JNIEXPORT jstring JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getValue
  (JNIEnv *env, jobject obj, jstring longName)
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	jsize len = env->GetStringUTFLength( longName );
	// printf("getValue len=%d\n", len );
	if ( len < 1 ) {
		const char *msg = padb->getMessage( );
		return env->NewStringUTF( msg);
	}

	const char *c_name = env->GetStringUTFChars( longName, 0);
	char *str = padb->getValue(  c_name );
	// printf("Jaguar_getVakue c_name=[%s] valuestr=[%s]\n", c_name, str );
	env->ReleaseStringUTFChars( longName, c_name);
	
	jstring res =  env->NewStringUTF( str);
	if ( str ) free( str );
	return res;
}
JNIEXPORT jstring JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getAllByName
  (JNIEnv *env, jobject obj, jstring longName)
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	jsize len = env->GetStringUTFLength( longName );
	if ( len < 1 ) {
		const char *msg = "";
		return env->NewStringUTF( msg);
	}

	const char *c_name = env->GetStringUTFChars( longName, 0);
	char *str = padb->getAllByName(  c_name );
	env->ReleaseStringUTFChars( longName, c_name);
	
	jstring res =  env->NewStringUTF( str);
	if ( str ) free( str );
	return res;
}


JNIEXPORT jstring JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getNthValue
  (JNIEnv *env, jobject obj,  jint nth )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	const char *str = padb->getNthValue( nth);
	jstring res =  env->NewStringUTF( str);
	//if ( str ) free( str );
	return res;
}

JNIEXPORT jstring JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getAllByIndex
  (JNIEnv *env, jobject obj,  jint nth )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	const char *str = padb->getAllByIndex( nth);
	jstring res =  env->NewStringUTF( str);
	//if ( str ) free( str );
	return res;
}


JNIEXPORT jint JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getIntByCol
  (JNIEnv *env, jobject obj,  jint nth )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	const char *str = padb->getNthValue( nth);
	if ( ! str ) return 0;
	jint res =  atoi(str);
	//if ( str ) free( str );
	return res;
}

JNIEXPORT jint JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getIntByName
  (JNIEnv *env, jobject obj,   jstring name  )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	const char *c_name = env->GetStringUTFChars( name, 0);
	char *str = padb->getValue( c_name);
	if ( ! str ) return 0;
	jint res =  atoi(str);
	if ( str ) free( str );
	return res;
}


JNIEXPORT jlong JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getLongByCol
  (JNIEnv *env, jobject obj,  jint nth )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	const char *str = padb->getNthValue( nth);
	if ( ! str ) return 0;
	jlong res =  atol(str);
	// if ( str ) free( str );
	return res;
}

JNIEXPORT jlong JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getLongByName
  (JNIEnv *env, jobject obj,   jstring name  )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	const char *c_name = env->GetStringUTFChars( name, 0);
	char *str = padb->getValue( c_name);
	if ( ! str ) return 0;
	jlong res =  atol(str);
	if ( str ) free( str );
	return res;
}

JNIEXPORT jfloat JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getFloat
  (JNIEnv *env, jobject obj,  jint nth )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	const char *str = padb->getNthValue( nth);
	if ( ! str ) return 0.0;
	jfloat res =  atof(str);
	// if ( str ) free( str );
	return res;
}

JNIEXPORT jdouble JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getDouble
  (JNIEnv *env, jobject obj,  jint nth )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	const char *str = padb->getNthValue( nth);
	if ( ! str ) return 0.0;
	jdouble res =  atof(str);
	// if ( str ) free( str );
	return res;
}


JNIEXPORT jstring JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getMessage
  (JNIEnv *env, jobject obj )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	const char *str = padb->getMessage( );
	return env->NewStringUTF( str);
}
JNIEXPORT jstring JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getAll
  (JNIEnv *env, jobject obj )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	const char *str = padb->getAll( );
	return env->NewStringUTF( str);
}

JNIEXPORT jstring JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_jsonString
  (JNIEnv *env, jobject obj )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	const char *str = padb->jsonString( );
	return env->NewStringUTF( str);
}

JNIEXPORT jstring JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getLastUuid
  (JNIEnv *env, jobject obj )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	const char *str = padb->getLastUuid();
	return env->NewStringUTF( str);
}


/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    getColumnCount
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getColumnCount
  (JNIEnv * env, jobject obj)
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	jint cc = padb->getColumnCount( );
	return cc;
}


/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    getCatalogName
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getCatalogName
  (JNIEnv *env, jobject obj, jint col )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	char *str = padb->getCatalogName( col );
	jstring res =  env->NewStringUTF( str);
	if ( str ) free( str );
	return res;
}

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    getColumnClassName
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getColumnClassName
  (JNIEnv *env, jobject obj, jint col )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	char *str = padb->getColumnClassName( col );
	jstring res =  env->NewStringUTF( str);
	if ( str ) free( str );
	return res;
}

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    getColumnDisplaySize
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getColumnDisplaySize
  (JNIEnv *env, jobject obj, jint col )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	jint cds = padb->getColumnDisplaySize( col );
	return cds;
}

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    getColumnLabel
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getColumnLabel
  (JNIEnv *env, jobject obj, jint col )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	char *str = padb->getColumnLabel( col );
	jstring res =  env->NewStringUTF( str);
	if ( str ) free( str );
	return res;
}

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    getColumnName
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getColumnName
  (JNIEnv *env, jobject obj, jint col )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	char *str = padb->getColumnName( col );
	jstring res =  env->NewStringUTF( str);
	if ( str ) free( str );
	return res;
}

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    getColumnType
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getColumnType
  (JNIEnv *env, jobject obj, jint col )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	jint rc = padb->getColumnType( col );
	return rc;
}

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    getColumnTypeName
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getColumnTypeName
  (JNIEnv *env, jobject obj, jint col )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	char *str = padb->getColumnTypeName( col );
	jstring res =  env->NewStringUTF( str);
	if ( str ) free( str );
	return res;
}


/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    getScale
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getScale
  (JNIEnv *env, jobject obj, jint col )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	jint rc = padb->getScale( col );
	return rc;
}

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    getSchemaName
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getSchemaName
  (JNIEnv *env, jobject obj, jint col )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	char *str = padb->getSchemaName( col );
	jstring res =  env->NewStringUTF( str);
	if ( str ) free( str );
	return res;
}

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    getTableName
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getTableName
  (JNIEnv *env, jobject obj, jint col )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	char *str = padb->getTableName( col );
	jstring res =  env->NewStringUTF( str);
	if ( str ) free( str );
	return res;
}

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    isAutoIncrement
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_isAutoIncrement
  (JNIEnv *env, jobject obj, jint col )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	return padb->isAutoIncrement( col );
}

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    isCaseSensitive
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_isCaseSensitive
  (JNIEnv *env, jobject obj, jint col )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	return padb->isCaseSensitive( col );
}

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    isCurrency
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_isCurrency
  (JNIEnv *env, jobject obj, jint col )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	return padb->isCurrency( col );
}

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    isDefinitelyWritable
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_isDefinitelyWritable
  (JNIEnv *env, jobject obj, jint col )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	return padb->isDefinitelyWritable( col );
}

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    isNullable
 * Signature: (I)Z
 */
JNIEXPORT jint JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_isNullable
  (JNIEnv *env, jobject obj, jint col )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	return padb->isNullable( col );
}

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    isReadOnly
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_isReadOnly
  (JNIEnv *env, jobject obj, jint col )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	return padb->isReadOnly( col );
}

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    isSearchable
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_isSearchable
  (JNIEnv *env, jobject obj, jint col )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	return padb->isSearchable( col );
}

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    isSigned
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_isSigned
  (JNIEnv *env, jobject obj, jint col )
{
	jclass objClass = env->GetObjectClass( obj);
	jfieldID adbFieldID = env->GetFieldID( objClass, "_adb", "J");
	jlong fieldVal = env->GetLongField( obj, adbFieldID);
	JaguarCPPClient *padb = (JaguarCPPClient*)fieldVal;

	return padb->isSigned( col );
}

// int main(){ return 0;}
