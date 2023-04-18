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
#ifndef _jag_math_h_
#define _jag_math_h_
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <AbaxCStr.h>
#include <JagVector.h>

#define JAG_B62_PREPEND_SIGN_ZERO_FRONT  1
#define JAG_B62_NO_PREPEND_SIGN          2

#define JAG_ULONG_B62_MAX    "LygHa16AHYF"
#define JAG_LONG_B62_MAX     "+AzL8n0Y58m7"
#define JAG_LONG_B62_MIN     "#p0erCzRurDs"

#define JAG_B62_ULONG_SIZE   11 // "......(11)
#define JAG_B62_LONG_SIZE    12 // "#...." or "+...."

#define JAG_B62_DBL_TOTAL_MAX_SIZE    20
#define JAG_B62_LDBL_TOTAL_MAX_SIZE   30

#define JAG_B62_POS          '+'
#define JAG_B62_NEG          '#'
#define JAG_B62_POS_STR      "+"
#define JAG_B62_NEG_STR      "#"
#define jag_b62_set         "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"


#define JAG_B254_PREPEND_SIGN_ZERO_FRONT  1
#define JAG_B254_NO_PREPEND_SIGN          2

#define JAG_ULONG_B254_MAX    "LygHa16AHYF"
#define JAG_LONG_B254_MAX     "+AzL8n0Y58m7"
#define JAG_LONG_B254_MIN     "#p0erCzRurDs"

#define JAG_B254_ULONG_SIZE   9 // "......(11)
#define JAG_B254_LONG_SIZE    9 // "#...." or "+...."

//#define JAG_B254_DBL_TOTAL_MAX_SIZE    20
#define JAG_B254_DBL_TOTAL_MAX_SIZE    16
#define JAG_B254_LDBL_TOTAL_MAX_SIZE   18

#define JAG_B254_POS          '+'
#define JAG_B254_NEG          '#'
#define JAG_B254_POS_STR      "+"
#define JAG_B254_NEG_STR      "#"

#define JAG_B254_POS_ZERO     1
#define JAG_B254_NEG_ZERO     255

#define JAG_B254_BASE         254

class JagMath
{
  public:   

    // unsigned long
    static void base62FromULong(Jstr &res, unsigned long n, int width);
    static unsigned long base62ToULong( const Jstr &b62 );
    static unsigned long base62ToULong( const char *b62 );
    static unsigned long base62ToULongLen( const char *b62, size_t len );

    // long
    static void base62FromLong( Jstr &res, long n, int width=-1, int prepend = JAG_B62_PREPEND_SIGN_ZERO_FRONT );
    static long base62ToLong( const char *b62 );
    static long base62ToLongLen( const char *b62, size_t len, int inType=0 );
    static long base62ToLong( const Jstr &b62 );

    static void fromBase62( Jstr &normal, const Jstr &base62 );
    static void fromBase62( Jstr &normal, const char *base62 );

    // double
    /***
    static void   base62FromDouble( Jstr &res, double n, int b62TotalWidth=-1, int b62sig=-1);
    static double base62ToDouble( const char *b62 );
    static double base62ToDoubleLen( const char *b62, size_t len );
    static double base62ToDouble( const Jstr &b62 );

    // long double
    static void   base62FromLongDouble( Jstr &res, long double n, int b62TotalWidth=-1, int b62sig=-1);
    static long double base62ToLongDouble( const char *b62 );
    static long double base62ToLongDoubleLen( const char *b62, size_t len );
    static long double base62ToLongDouble( const Jstr &b62 );

    // non zero-prepended
    static void toBase62( Jstr &base62, const Jstr &normal, bool withSign=true);
    ***/

    // utils
    static int  base62Width( int normalWidth );
    static int  normalWidth62( int b62Width );
    static int  base62WidthSlow( int normalWidth );

    static unsigned char base62Value( char b62 );
    static char compliment62( char c );
    static int numDigits(long n );

    //static int leadingZeros(double dec);


    // unsigned long
    static void base254FromULong(Jstr &res, unsigned long n, int width);
    static unsigned long base254ToULong( const Jstr &b254 );
    static unsigned long base254ToULong( const char *b254 );
    static unsigned long base254ToULongLen( const char *b254, size_t len );

    // long
    static void base254FromLong( Jstr &res, long n, int width=-1, int prepend = JAG_B254_PREPEND_SIGN_ZERO_FRONT );
    static long base254ToLong( const char *b254 );
    static long base254ToLongLen( const char *b254, size_t len, int inType=0 );
    static long base254ToLong( const Jstr &b254 );

    static void   base254FromStr( Jstr &res, const char *str, int b254TotalWidth=-1, int b254sig=-1);

    // double
    //static void   base254FromDouble( Jstr &res, double n, int b254TotalWidth=-1, int b254sig=-1);
    static void   base254FromDoubleStr( Jstr &res, const char *str, int b254TotalWidth=-1, int b254sig=-1);
    static double base254ToDouble( const char *b254 );
    static double base254ToDoubleLen( const char *b254, size_t len );
    static double base254ToDouble( const Jstr &b254 );

    // long double
    //static void   base254FromLongDouble( Jstr &res, long double n, int b254TotalWidth=-1, int b254sig=-1);
    static void   base254FromLongDoubleStr( Jstr &res, const char *str, int b254TotalWidth=-1, int b254sig=-1);
    static long double base254ToLongDouble( const char *b254 );
    static long double base254ToLongDoubleLen( const char *b254, size_t len );
    static long double base254ToLongDouble( const Jstr &b254 );

    // non zero-prepended
    static void toBase254( Jstr &base254, const Jstr &normal, bool withSign=true);
    static void toBase254Len( Jstr &base254, const char *norm, int normlen, bool withSign=true);
    static void fromBase254( Jstr &normal, const Jstr &base254 );
    static void fromBase254( Jstr &normal, const char *base254 );
    static void fromBase254Len( Jstr &normal, const char *base254, int len );

    // utils
    static int  base254Width( int normalWidth );
    static int  normalWidth254( int b254Width );
    static int  base254WidthSlow( int normalWidth );

    static unsigned char valueOfBase254( unsigned char b254 );
    static unsigned char base254Symbol( unsigned char n );
    static unsigned char compliment254( unsigned char c );


    // + or -
    static int   ipow(int base, int exp);

    // + or -
    static long  lpow(long base, long exp);

    // + only
    static unsigned long ulpow(unsigned long base, unsigned long exp);

    //  20304 --> 0.20304    -34 --> -0.34
    static double longToFraction(int nlen, long n );

    static int wSig( const Jstr &b254 );


  protected:
    static void prepare62Vec_( long n, bool isNegative, JagVector<char> &vec);
    static int  frontBase62Width( const char *front, int len);

    static void prepare254Vec_( long n, bool isNegative, JagVector<unsigned char> &vec);
    static int  frontBase254Width( const char *front, int len);

    static long convertPosStrToNum(const char *str, int size);
    static long convertNegStrToNum(const char *str, int size);

};

#endif
