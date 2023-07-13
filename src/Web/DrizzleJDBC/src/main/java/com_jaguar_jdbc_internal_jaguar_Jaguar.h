/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_jaguar_jdbc_internal_jaguar_Jaguar */

#ifndef _Included_com_jaguar_jdbc_internal_jaguar_Jaguar
#define _Included_com_jaguar_jdbc_internal_jaguar_Jaguar
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    connect
 * Signature: (Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;J)Z
 */
JNIEXPORT jboolean JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_connect
  (JNIEnv *, jobject, jstring, jint, jstring, jstring, jstring, jstring, jlong);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    execute
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_execute
  (JNIEnv *, jobject, jstring);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    query
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_query
  (JNIEnv *, jobject, jstring);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    reply
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_reply
  (JNIEnv *, jobject);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    replyHeader
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_replyHeader
  (JNIEnv *, jobject);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    printRow
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_printRow
  (JNIEnv *, jobject);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    printAll
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_printAll
  (JNIEnv *, jobject);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    hasError
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_hasError
  (JNIEnv *, jobject);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    error
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_error
  (JNIEnv *, jobject);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    freeRow
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_freeRow
  (JNIEnv *, jobject);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    getValue
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getValue
  (JNIEnv *, jobject, jstring);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    getAllByName
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getAllByName
  (JNIEnv *, jobject, jstring);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    getNthValue
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getNthValue
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    getAllByIndex
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getAllByIndex
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    getMessage
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getMessage
  (JNIEnv *, jobject);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    getAll
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getAll
  (JNIEnv *, jobject);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    jsonString
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_jsonString
  (JNIEnv *, jobject);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    jsonString
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getLastUuid
  (JNIEnv *, jobject);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    getIntByCol
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getIntByCol
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    getIntByName
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getIntByName
  (JNIEnv *, jobject, jstring);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    getLongByCol
 * Signature: (I)J
 */
JNIEXPORT jlong JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getLongByCol
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    getLongByName
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getLongByName
  (JNIEnv *, jobject, jstring);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    getFloat
 * Signature: (I)F
 */
JNIEXPORT jfloat JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getFloat
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    getDouble
 * Signature: (I)D
 */
JNIEXPORT jdouble JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getDouble
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    getColumnCount
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getColumnCount
  (JNIEnv *, jobject);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    getCatalogName
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getCatalogName
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    getColumnClassName
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getColumnClassName
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    getColumnDisplaySize
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getColumnDisplaySize
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    getColumnLabel
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getColumnLabel
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    getColumnName
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getColumnName
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    getColumnType
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getColumnType
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    getColumnTypeName
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getColumnTypeName
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    getScale
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getScale
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    getSchemaName
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getSchemaName
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    getTableName
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_getTableName
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    isAutoIncrement
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_isAutoIncrement
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    isCaseSensitive
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_isCaseSensitive
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    isCurrency
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_isCurrency
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    isDefinitelyWritable
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_isDefinitelyWritable
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    isNullable
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_isNullable
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    isReadOnly
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_isReadOnly
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    isSearchable
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_isSearchable
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    isSigned
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_isSigned
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    adbMakeObject
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_adbMakeObject
  (JNIEnv *, jobject);

/*
 * Class:     com_jaguar_jdbc_internal_jaguar_Jaguar
 * Method:    adbClose
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_jaguar_jdbc_internal_jaguar_Jaguar_adbClose
  (JNIEnv *, jobject, jlong);

#ifdef __cplusplus
}
#endif
#endif
