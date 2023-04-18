/*
 * Copyright (C) 2023 DataJaguar, Inc.
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
#include <float.h>
#include <math.h>
#include <assert.h>
#include <JagVector.h>
#include <JagDef.h>
#include <JagMath.h>
#include <JagUtil.h>
#include <JagLog.h>


// ULONG_MAX = 18446744073709551615 size=20
// LONG_MAX = 9223372036854775807 size=19
// LONG_MIN = -9223372036854775808 size=20


////////////////////// Unsigned long ////////////////////////////
void JagMath::base62FromULong(Jstr &res, unsigned long n, int width)
{
    res = "";
    if ( n == 0 ) {
        res.appendChars(width, '0');
        return;
    }

    JagVector<char> vec;
    unsigned long i;
    while ( n > 0 ) {
        i = n % 62;
        vec.push_back(jag_b62_set[i]);
        n = n/62;
    }

    vec.reverse();

    if ( width < 0 || width > JAG_B62_ULONG_SIZE ) width = JAG_B62_ULONG_SIZE;

    int prepend = width - vec.size();
    for ( int i = 0; i < prepend; ++i ) {
        res += '0';
    }

    for ( int i=0; i < vec.size() && i < width; ++i ) {
        res += vec[i];
    }
}

unsigned long JagMath::base62ToULong( const char *b62 )
{
    return base62ToULongLen( b62, strlen(b62) );
}

unsigned long JagMath::base62ToULongLen( const char *b62, size_t len )
{
    if ( b62 == NULL || *b62 == '\0' ) return 0;

    if ( strcmp(b62, JAG_ULONG_B62_MAX) > 0 ) {
        b62 = JAG_ULONG_B62_MAX;
    }

    unsigned long sum = 0;
    for ( size_t i=0; i < len; ++i ) {
        if ( '\0' ==  b62[i] ) {
            break;
        }

        sum = sum * 62 + base62Value( b62[i]);
    }

    return sum;
}

unsigned long JagMath::base62ToULong( const Jstr &b62 )
{
    return base62ToULongLen( b62.s(), b62.size() );
}


///////////// Long methods //////////////////////////////////
//#define JAG_B62_PREPEND_SIGN_ZERO_FRONT  1
//#define JAG_B62_NO_PREPEND_SIGN  2

// width is total width (including the beginning sign if possible)
void JagMath::base62FromLong( Jstr &res, long n, int width, int prepend)
{
    if ( JAG_B62_PREPEND_SIGN_ZERO_FRONT != prepend && n == 0 ) {
        res = "0"; 
        return;
    }

    if ( width < 0 ) {
        width = JAG_B62_LONG_SIZE;
    }

    res = "";
    if ( 0 == n ) {
        res.appendChars( 1, JAG_B62_POS );
        res.appendChars( width-1, '0');
        return;
    }

    bool isNegative = false;
    if ( n < 0 ) isNegative = true;


    JagVector<char> vec;
    prepare62Vec_( n, isNegative, vec);
    int  digitWidth;

    if ( JAG_B62_PREPEND_SIGN_ZERO_FRONT == prepend ) {
        if ( isNegative ) {
            res = JAG_B62_NEG;
        } else {
            res = JAG_B62_POS;
        }

        if ( width < 0 || width > JAG_B62_LONG_SIZE ) width = JAG_B62_LONG_SIZE;

        digitWidth = width - 1;
        int prependLen = digitWidth - vec.size();
        for ( int i = 0; i < prependLen; ++i ) {
            if ( isNegative ) {
                res += 'z';
            } else {
                res += '0';
            }
        }
    } else {
        digitWidth = width;
    }

    for ( int i=0; i < vec.size() && i < digitWidth; ++i ) {
        if ( isNegative ) {
            res += compliment62(vec[i]) ;
        } else {
            res += vec[i];
        }
    }

}

// base62 is long, has no double
void JagMath::fromBase62(Jstr &normal, const Jstr &base62 )
{
    fromBase62(normal, base62.s() );
}

// base62 is long, has no double
void JagMath::fromBase62(Jstr &normal, const char *base62 )
{
    char *pdot = (char*)strchr(base62, '.');
    if ( ! pdot ) {
        long n = base62ToLong( base62 );
        normal = longToStr(n);
        return;
    }

}

// a list of 62-based digit-symbols
void JagMath::prepare62Vec_( long n, bool isNegative, JagVector<char> &vec)
{
    unsigned long un;
    if ( isNegative ) {
        if ( n < 0 ) {
            un =  (unsigned long)(-n);
        } else {
            un =  (unsigned long)(n);
        }
    } else {
        un = (unsigned long)n;
    }
    //dn("prepareVec_ n=%ld un=%lu isNegative=%d", n, un, isNegative); 

    unsigned long i;
    while ( un > 0 ) {
        i = un % 62;
        vec.push_back(jag_b62_set[i]);
        //dn("prepareVec_ i=%lu jag_b62_set[i]=%c", i, jag_b62_set[i] ); 
        un = un/62;
    }

    vec.reverse();
}

long JagMath::base62ToLong( const char *b62 )
{
    return base62ToLongLen( b62, strlen(b62) );
}

// b62 a base62 encoded string
// len = strlen(b62)
// return: >=0 or < 0
long JagMath::base62ToLongLen( const char *b62, size_t len, int inType )
{
    if ( b62 == NULL || *b62 == '\0' ) return 0;
    
    if ( *b62 == JAG_B62_NEG || *b62 == JAG_B62_POS ) {
        if ( strcmp(b62, JAG_LONG_B62_MAX) > 0 ) {
            b62 = JAG_LONG_B62_MAX;
            //dn("m0998 b62 = JAG_LONG_B62_MAX");
        } else if (  strcmp(b62, JAG_LONG_B62_MIN) < 0 ) {
            b62 = JAG_LONG_B62_MIN;
            //dn("m0948 b62 = JAG_LONG_B62_MIN");
        }
    }

    bool isNeg;
    size_t start = 0;

    if ( 0 == inType ) {
        /// not designated sign type
        if ( JAG_B62_NEG == *b62 ) {
            isNeg = true;
            start = 1;
        } else if ( JAG_B62_POS == *b62 ) {
            isNeg = false;
            start = 1;
        } else {
            isNeg = false;
        }
    } else if ( 1 == inType ) {
        // b62: "...." which is negative but no sign
        isNeg = true;
    } else if ( 2 == inType ) {
        // b2: "...." which is positive but no sign
        isNeg = false;
    } else {
        return 0;
    }

    //d("base62ToLongLen b62=[%s] len=%ld start=%ld isneg=%d\n", b62, len, start, isNeg);

    unsigned long sum = 0;
    char c;
    for ( size_t i=start; i < len; ++i ) {
        if ( '\0' == b62[i] ) {
            break;
        }

        if ( isNeg ) {
            c = compliment62(b62[i]);
        } else {
            c = b62[i];
        }
        sum = sum * 62 + base62Value( c );
        /***
        dn("m0127 base62ToLongLen() b62=[%s] len=%d i=%ld c=%c base62Value(c)=%d sum=%lu",
            b62, len, i, c, base62Value( c ), sum );
            ***/
    }

    if ( isNeg ) {
        char tbf[64];
        sprintf(tbf, "%lu", sum);
        //d("m0174 b62=[%s]  -  tbf=[%s] sum=%ld\n", b62, tbf, -atol(tbf) );
        return -atol(tbf);
    } else {
        //d("m0177 b62=[%s]  +  sum=%lu\n", b62, sum );
        return sum;
    }
}

long JagMath::base62ToLong( const Jstr &b62 )
{
    return base62ToLongLen( b62.s(), b62.size() );
}


int JagMath::base62WidthSlow( int normalWidth )
{
    unsigned long pw = ulpow(10, normalWidth);
    int cnt = 0;
    unsigned long mx = pw -1;
    while ( mx > 0 ) {
        ++cnt;
        mx = mx / 62;
    }
    return cnt;
}

// length b62 from a normal number
int JagMath::base62Width( int normalWidth )
{
    static int base62_lookup_tab[] = {
      //   1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16  17  18  19
        0, 1, 2, 2, 3, 3, 4, 4, 5, 6, 6,  7, 7, 8, 8, 9, 9, 10, 11, 11 
    };

    if ( normalWidth < 0 ) return 0;
    if ( normalWidth > 19 ) return 11;

    return base62_lookup_tab[normalWidth];
}

// required length of normal to get desired b62Width
int JagMath::normalWidth62( int b62Width )
{
    static int normal_lookup_tab[] = {
      //   1  2  3  4  5  6   7   8   9   10
        0, 1, 3, 5, 7, 8, 10, 12, 14, 16, 17 
    };

    if ( b62Width < 0 ) return 0;
    if ( b62Width >= 10 ) return 17;

    return normal_lookup_tab[b62Width];
}


int JagMath::ipow(int base, int exp)
{
    int result = 1;
    while (true) {
        if (exp & 1) result *= base;
        exp >>= 1;
        if (!exp) break;
        base *= base;
    }

    return result;
}

long JagMath::lpow(long base, long exp)
{
    long result = 1;
    while (true) {
        if (exp & 1) result *= base;
        exp >>= 1;
        if (!exp) break;
        base *= base;
    }

    return result;
}

unsigned long JagMath::ulpow(unsigned long base, unsigned long exp)
{
    unsigned long result = 1;
    while (true) {
        if (exp & 1) result *= base;
        exp >>= 1;
        if (!exp) break;
        base *= base;
    }

    return result;
}



////////////// protected methods ///////////////////////
unsigned char JagMath::base62Value( char b62 )
{
    if ( '0' <= b62 && b62 <= '9' ) {
        return b62 - '0';
    } else if ( 'A' <= b62 && b62 <= 'Z' ) {
        return b62 - 'A' + 10;
    } else if ( 'a' <= b62 && b62 <= 'z' ) {
        return b62 - 'a' + 36;
    } else {
        return 0;
    }

}

char JagMath::compliment62( char c )
{
    unsigned char v = base62Value(c);
    return jag_b62_set[61 - v];
}

// front: "+3344"  "-3455"  "3445"  "+0" "-0"
//  len==strlen(front)
// return total widt including the sign
int JagMath::frontBase62Width( const char *front, int len)
{
    if ( front == NULL || *front == '\0' ) {
        return 0;
    }

    int width;
    if ( front[0] == '-' ) {
        width = 1 + base62Width( len -1) ;
    } else {
        if ( front[0] == '-' ) {
            width = 1 + base62Width( len -1) ;
        } else {
            width = 1 + base62Width( len ) ;
        }
    }

    return width;

}

//////////////////// base254 ////////////////////////
void JagMath::base254FromULong(Jstr &res, unsigned long n, int width)
{
    //dn("m0277180 base254FromULong n=%lu width=%d", n, width );

    res = "";
    if ( n == 0 ) {
        res.appendChars(width, JAG_B254_POS_ZERO);
        return;
    }

    JagVector<unsigned char> vec;
    unsigned long i;
    while ( n > 0 ) {
        i = n % JAG_B254_BASE;
        vec.push_back(base254Symbol(i));
        //dn("m761120 i=%lu pushed byte=%d", i, base254Symbol(i) );
        n = n/JAG_B254_BASE;
    }

    vec.reverse();

    if ( width < 0 || width > JAG_B254_ULONG_SIZE ) {
        //dn("m2771 width setto = JAG_B254_ULONG_SIZE");
        width = JAG_B254_ULONG_SIZE;
    }

    int prepend = width - vec.size();
    // prepend zeros for ordering
    //dn("m40051 prependZeros=%d ", prepend );
    for ( int i = 0; i < prepend; ++i ) {
        res += JAG_B254_POS_ZERO;
    }

    //dn("m333029 vec.size=%ld  width=%d", vec.size(), width );
    for ( int i=0; i < vec.size() && i < width; ++i ) {
        res += vec[i];
    }

    // debug
    /**
    printf("print base254FromULong vec.size=%lld:\n", vec.size() );
    for ( int i=0; i < vec.size(); ++i ) {
        printf("%d:[%d] ", i, vec[i] );
    }
    printf("\n");
    **/
}

unsigned long JagMath::base254ToULong( const char *b254 )
{
    return base254ToULongLen( b254, strlen(b254) );
}

unsigned long JagMath::base254ToULongLen( const char *b254, size_t len )
{
    if ( b254 == NULL || *b254 == '\0' ) return 0;

    unsigned long sum = 0;
    for ( size_t i=0; i < len; ++i ) {
        if ( '\0' == b254[i] ) {
            break;
        }
        
        sum = sum * JAG_B254_BASE + valueOfBase254( c2uc(b254[i]) );
    }

    //dn("m892003 base254ToULongLen sum=%lu", sum );
    return sum;
}

unsigned long JagMath::base254ToULong( const Jstr &b )
{
    return base254ToULongLen( b.s(), b.size() );
}


// width is the width of the base254 string should have
void JagMath::base254FromLong( Jstr &res, long n, int width, int prepend)
{
    dn("m80281 base254FromLong n=%ld width=%d prependflag=%d", n, width, prepend );

    res = "";
    if ( width < 0 ) {
        width = JAG_B254_LONG_SIZE;
        //dn("m282289 width ====> JAG_B254_LONG_SIZE=%d", width );
    }

    if ( 0 == n ) {
        if ( prepend == JAG_B254_NO_PREPEND_SIGN ) {
            res.appendChars( width, JAG_B254_POS_ZERO  );
        } else {
            res.appendChars( 1, JAG_B254_POS );
            res.appendChars( width-1, JAG_B254_POS_ZERO );
        }
        return;
    }

    bool isNegative = false;
    if ( n < 0 ) isNegative = true;

    JagVector<unsigned char> vec;
    prepare254Vec_( n, isNegative, vec);
    int  digitWidth;
    dn("m30019901 prepare254Vec_ n=%ld vec.size=%d", n, vec.size() );

    if ( JAG_B254_PREPEND_SIGN_ZERO_FRONT == prepend ) {
        if ( isNegative ) {
            res = JAG_B254_NEG;
        } else {
            res = JAG_B254_POS;
        }

        if ( width < 0 || width > JAG_B254_LONG_SIZE ) {
            width = JAG_B254_LONG_SIZE;
            dn("m3110028 width adjusted <== %d", width );
        }

        digitWidth = width - 1;
        int prependLen = digitWidth - vec.size();
        dn("m300281 digitWidth=%d prependLen=%d vec.size=%ld", digitWidth, prependLen,  vec.size() );
        // prepend zeros for ordering
        if ( prependLen > 0 ) {
            if ( isNegative ) {
                res.appendChars( prependLen, JAG_B254_NEG_ZERO);
            } else {
                res.appendChars( prependLen, JAG_B254_POS_ZERO);
            }
        }

    } else {
        digitWidth = width;
    }

    //dn("m701115 vec.size=%ld digitWidth=%d isNegative=%d", vec.size(), digitWidth, isNegative );

    for ( int i=0; i < vec.size() && i < digitWidth; ++i ) {
        if ( isNegative ) {
            res += compliment254( (unsigned char)vec[i]) ;
        } else {
            res += (int)vec[i];
            dn("m292020 res += [%d: %d]", i, vec[i] );
            //in("m292020 res += [%d: %d]", i, res[res.size()-1] );
        }
    }

    /***
    dn("m2220818 dump res: res.size=%ld", res.size() );
    dumpmem( res.s(), res.size() );
    Jstr norm;
    fromBase254(norm, res.s() );
    dn("m4106301 long n=%ld --> b254=[] --> norm=%s", n, norm.s() );
    ***/

    // debug
    /**
    in("m2939300 debug res.size=%ld:", res.size() );
    for ( int j=0; j < res.size(); ++j ) {
        in("j=%d res[j]=[%d]", j, res[j] );
    }
    in("m2939300 debug res done");
    **/

}

// a list of 254-based digit-symbols
void JagMath::prepare254Vec_( long n, bool isNegative, JagVector<unsigned char> &vec)
{
    unsigned long un;
    if ( isNegative ) {
        if ( n < 0 ) {
            un =  (unsigned long)(-n);
        } else {
            un =  (unsigned long)(n);
        }
    } else {
        un = (unsigned long)n;
    }
    //dn("prepare254Vec_ n=%ld un=%lu isNegative=%d", n, un, isNegative); 

    unsigned long i;
    while ( un > 0 ) {
        i = un % JAG_B254_BASE;
        vec.push_back(base254Symbol(i));
        //dn("prepare254Vec_ i=%lu base254Symbol[i]=%d", i, base254Symbol(i) ); 
        un = un/JAG_B254_BASE;
    }

    vec.reverse();
}

long JagMath::base254ToLong( const char *b )
{
    return base254ToLongLen( b, strlen(b) );
}

long JagMath::base254ToLongLen( const char *b254, size_t len, int inType )
{
    if ( b254 == NULL || *b254 == '\0' ) return 0;
    
    bool isNeg;
    size_t start = 0;

    if ( 0 == inType ) {
        /// not designated sign type
        if ( JAG_B254_NEG == *b254 ) {
            isNeg = true;
            start = 1;
        } else if ( JAG_B254_POS == *b254 ) {
            isNeg = false;
            start = 1;
        } else {
            isNeg = false;
        }
    } else if ( 1 == inType ) {
        // b254: "...." which is negative but no sign
        isNeg = true;
    } else if ( 2 == inType ) {
        // b2: "...." which is positive but no sign
        isNeg = false;
    } else {
        return 0;
    }

    //dn("base254ToLongLen b254=[%s] len=%ld start=%ld isneg=%d", b254, len, start, isNeg);

    unsigned long sum = 0;
    unsigned char c;
    for ( size_t i=start; i < len; ++i ) {
        if ( '\0' == b254[i] ) {
            break;
            // ignore NULLs
        }

        if ( isNeg ) {
            c = compliment254(c2uc(b254[i]));
        } else {
            c = c2uc(b254[i]);
        }
        sum = sum * JAG_B254_BASE + valueOfBase254( c );

        /**
        dn("m30127 base254ToLongLen() b254=[%s] len=%d i=%ld c=%d valueOfBase254(c)=%d sum=%lu",
            b254, len, i, c, valueOfBase254( c ), sum );
            **/
    }

    if ( isNeg ) {
        char tbf[64];
        sprintf(tbf, "%lu", sum);
        //d("m0174 b254=[%s]  -  tbf=[%s] sum=%ld\n", b254, tbf, -atol(tbf) );
        return -atol(tbf);
    } else {
        //d("m0177 b254=[%s]  +  sum=%lu\n", b254, sum );
        return sum;
    }
}

long JagMath::base254ToLong( const Jstr &b254 )
{
    return base254ToLongLen( b254.s(), b254.size() );
}

//////////////// double ////////////////////////////
void JagMath::base254FromStr( Jstr &res, const char *str, int b254TotalWidth, int b254sig )
{
    if ( str == NULL || *str == '\0' ) {
        res = "";
        return;
    }

    long first = long(jagatof( str ));

    bool isNeg = false;
    if ( first < 0 ) isNeg = true;
    else if ( '-' == *str ) isNeg = true;

    dn("m2496148 base254FromStr str=%s isNeg=%d first=%ld  b254TotalWidth=%d  b254sig=%d", 
        str, isNeg, first, b254TotalWidth, b254sig  );

    int  normlen = normalWidth254( b254sig );
    unsigned char b254len = base254Symbol(normlen);
    dn("m230881  b254sig=%d  normlen=%d b254len=%d", b254sig, normlen, b254len);

    const char *pdot = strchr(str, '.' );
    if ( ! pdot || ( *pdot == '.' && *(pdot+1) == '\0') ) {
        // int intb254width = b254TotalWidth -1 -1;
        // keept 223334  and 223334.320 same with in front
        int intb254width = b254TotalWidth -1 -1 - b254sig;
        dn("m16403110 nofraction first=%ld b254TotalWidth=%d --> intb254width=%d", first, b254TotalWidth, intb254width );
        base254FromLong(res, first, intb254width);
        res += '.' ;
        res += b254len ;

        //Jstr s2;
        //base254FromLong(s2, 0, b254sig, JAG_B254_NO_PREPEND_SIGN);
        // res += s2;

        if ( isNeg ) {
            res.appendChars( b254sig, JAG_B254_NEG_ZERO  );
        } else {
            res.appendChars( b254sig, JAG_B254_POS_ZERO  );
        }
        dn("m5100901 nofraction return res");

        /**
        dn("m122029 dump res: res.size=%ld", res.size() );
        dumpmem( res.s(), res.size() );
        **/
        return;
    }

    Jstr s1, s2;
    if ( 0 == first ) {
        // "0.023"
        if ( isNeg ) {
            s1 = JAG_B254_NEG_STR;
        } else {
            s1 = JAG_B254_POS_STR;
        }
    } else {
        // front may or may not have sign. Here just take off . and sigwidth parts
        int intb254width = b254TotalWidth -1 -1 - b254sig;
        dn("m46003110 b254TotalWidth=%d b254sig=%d --> intb254width=%d", b254TotalWidth, b254sig, intb254width );
        base254FromLong(s1, first, intb254width); // #reversed...
    }
    //dn("m3819200 base254FromLong first=%ld  intwidth=%d front-s1=[%s]", first, intwidth, s1.s() );

    ++ pdot; // passing the decimal point

    long s2n;

    dn("m3221108 pdot=[%s] b254sig=%d normlen=%d", pdot, b254sig, normlen );

    if ( isNeg ) {
        s2n = convertNegStrToNum( pdot, normlen );
    } else {
        s2n = convertPosStrToNum( pdot, normlen );
    }

    base254FromLong(s2, s2n, b254sig, JAG_B254_NO_PREPEND_SIGN);
    dn("m111098 s2=[%s] s2.size=%d", s2.s(), s2.size() );

    if ( s2.size() < b254sig ) {
        if ( isNeg ) {
            s2.appendChars( b254sig - s2.size(), JAG_B254_NEG_ZERO );
        } else {
            s2.appendChars( b254sig - s2.size(), JAG_B254_POS_ZERO );
        }
    }

    res = s1 + "." + b254len + s2;
    dn("m3440881 wsig=[%u]=%d", b254len, valueOfBase254(b254len) );

    dn("m35701209 res=[%s]", res.s() );
}

void JagMath::base254FromDoubleStr( Jstr &res, const char *str, int b254TotalWidth, int b254sig )
{
    long first = long(jagatof( str ));

    bool isNeg = false;
    if ( first < 0 ) isNeg = true;
    else if ( '-' == *str ) isNeg = true;

    dn("m2726001 base254FromDoubleStr str=%s isNeg=%d first=%ld", str, isNeg, first );

    if ( b254TotalWidth < 0 || b254TotalWidth > JAG_B254_DBL_TOTAL_MAX_SIZE ) {
        b254TotalWidth = JAG_B254_DBL_TOTAL_MAX_SIZE;
        //dn("m028334 b254TotalWidth = JAG_B254_DBL_TOTAL_MAX_SIZE");
    }

    if ( b254sig < 0 ) {
        b254sig = 2;
        // b254sig = 5; ok
        dn("m33039 use b254sig=%d", b254sig);
    }

    base254FromStr( res, str, b254TotalWidth, b254sig );
}

void JagMath::base254FromLongDoubleStr( Jstr &res, const char *str, int b254TotalWidth, int b254sig )
{
    long first = long(jagatof( str ));

    bool isNeg = false;
    if ( first < 0 ) isNeg = true;
    else if ( '-' == *str ) isNeg = true;

    dn("m2756001 base254FromLongDoubleStr str=%s isNeg=%d first=%ld", str, isNeg, first );

    if ( b254TotalWidth < 0 || b254TotalWidth > JAG_B254_LDBL_TOTAL_MAX_SIZE ) {
        b254TotalWidth = JAG_B254_LDBL_TOTAL_MAX_SIZE;
    }

    if ( b254sig < 0 ) {
        b254sig = 3; 
        // b254sig = 7; OK
        dn("m440829 use b254sig = %d", b254sig);
    }

    base254FromStr( res, str, b254TotalWidth, b254sig );
}

double JagMath::base254ToDouble( const Jstr &b254 )
{
    return base254ToDouble( b254.s() );
}

// b254: "+8Sgtef.0skY"  output: 9283399.02980
// b254: "+8Sgtef.@@s8kY"  output: 9283399.0029807
// b254: "#8Sgtef.0skY"  output: -9283399.02980
// b254: "+8SgtefY"  output: 928339930
// b254: "#8SgtefY"  output: -928339930
double JagMath::base254ToDouble( const char *b254 )
{
    if ( b254 == NULL || *b254 == '\0' ) return 0;

    bool isNeg;
    if ( JAG_B254_NEG == *b254 ) {
        isNeg = true;
    } else if ( JAG_B254_POS == *b254 ) {
        isNeg = false;
    } else {
        isNeg = false;
    }

    //d("m2028 base254ToDoubleLen b254=[%s] len=%ld  isNeg=%d\n", b254, len, isNeg );

    char *pdot = (char*)strchr(b254, '.');
    if ( ! pdot ) {
        long n = base254ToLong( b254 );
        return (double)(n);
    } 

    *pdot = '\0';

    long n1 = base254ToLong( b254 );
    *pdot = '.';

    ++pdot; // passed .
    unsigned char nlen = valueOfBase254( *pdot );
    int   b254len = base254Width( nlen );
    ++pdot;

    dn("m321009 nlen=%d  pd=[%s]", nlen, pdot );

    double secondv;

    if ( *pdot == '\0' ) {
        secondv = 0.0;
    } else {
        long n2;
        if ( isNeg ) {
            n2 =  base254ToLongLen( pdot, b254len, 1 );
        } else {
            n2 =  base254ToLongLen( pdot, b254len, 2 );
        }

        secondv = longToFraction( nlen, n2 ); 
        dn("m301911 n2=%ld secondv=%f nlen=%d", n2, secondv, nlen );
    }

    return (double)n1 + secondv;
}

long double JagMath::base254ToLongDouble( const Jstr &b254 )
{
    return base254ToLongDouble( b254.s() );
}

long double JagMath::base254ToLongDouble( const char *b254 )
{
    if ( b254 == NULL || *b254 == '\0' ) return 0;

    bool isNeg;

    if ( JAG_B254_NEG == *b254 ) {
        isNeg = true;
    } else if ( JAG_B254_POS == *b254 ) {
        isNeg = false;
    } else {
        isNeg = false;
    }

    //d("m2028 base254ToDoubleLen b254=[%s] len=%ld  isNeg=%d\n", b254, len, isNeg );

    char *pdot = (char*)strchr(b254, '.');

    if ( ! pdot ) {
        long n = base254ToLong( b254 );
        return (long double)(n);
    } 

    *pdot = '\0';
    long n1 = base254ToLong( b254 );
    *pdot = '.';

    ++pdot; // point to nlen byte

    unsigned char nlen = valueOfBase254(*pdot);
    int   b254len = base254Width( nlen );
    ++pdot;

    dn("m580020 nlen=%d  pdot=[%s]", nlen, pdot );

    long double secondv;

    if ( *pdot == '\0' ) {
        secondv = 0.0;
    } else {
        long n2;
        dn("m333010 pdot=[%s] isNeg=%d", pdot, isNeg );
        if ( isNeg ) {
            n2 = base254ToLongLen( pdot, b254len, 1 );
        } else {
            n2 = base254ToLongLen( pdot, b254len, 2 );
        }

        secondv = longToFraction( nlen, n2 ); 
        dn("m301914 n2=%ld secondv=%f nlen=%d =?= strlen(pdot)=%d", n2, secondv, nlen, strlen(pdot) );
    }

    return (long double)n1 + secondv;
}

void JagMath::toBase254( Jstr &base254, const Jstr &normal, bool withSign )
{
    toBase254Len(base254, normal.s(), normal.size(), withSign );
}

void JagMath::toBase254Len( Jstr &base254, const char *normal, int normlen, bool withSign)
{
    if ( normlen < 1 ) {
        base254="";
        return;
    }

    char *pdot = (char*)strchr(normal, '.');
    if ( ! pdot ) {
        long n = atol(normal);
        int width = frontBase254Width( normal, normlen );

        if ( withSign ) {
            base254FromLong( base254, n, width );
        } else {
            base254FromLong( base254, n, width, JAG_B254_NO_PREPEND_SIGN );
        }
        return;
    }

    // front integral part
    *pdot = '\0';
    long n1 = atol(normal);
    *pdot = '.';
    Jstr s1;

    bool isNeg = false;
    if ( normal[0] == '-' ) isNeg = true;

    dn("m202298 withSign=%d  isNeg=%d", withSign, isNeg );

    if ( 0 == n1 ) {
        if ( withSign ) {
            if ( isNeg ) {
                s1 = JAG_B254_NEG_STR;
                dn("m100287 added s1=[%s]", s1.s() );
            } else {
                s1 = JAG_B254_POS_STR;
            } 
        } else {
            if ( isNeg ) {
                s1 = JAG_B254_NEG_STR;
            } else {
                s1 = "";
            }
        }
    } else {
        int width = frontBase254Width( normal, pdot - normal );
        if ( withSign ) {
            base254FromLong( s1, n1, width);
        } else {
            base254FromLong( s1, n1, width, JAG_B254_NO_PREPEND_SIGN);
        }
    }

    ++pdot;

    int b254sig;
    int pdlen = strlen(pdot);

    if ( *pdot == '\0' ) {
        pdot = (char*)"0";
        b254sig = 1;
        pdlen = 1;
    }  else {
        b254sig = base254Width(pdlen);
    }

    Jstr s2;
    long s2n;

    if ( isNeg ) {
        s2n = convertNegStrToNum( pdot, pdlen );
    } else {
        s2n = convertPosStrToNum( pdot, pdlen );
    }

    base254FromLong(s2, s2n, b254sig, JAG_B254_NO_PREPEND_SIGN);
    unsigned char b254len = base254Symbol(pdlen);

    dn("m2020211 s2=[%s] s2.size=%d widthofsig=[%u]", s2.s(), s2.size(), b254len );

    if ( s2.size() < b254sig ) {
        if ( isNeg ) {
            s2.appendChars( b254sig - s2.size(), JAG_B254_NEG_ZERO );
        } else {
            s2.appendChars( b254sig - s2.size(), JAG_B254_POS_ZERO );
        }
    }

    base254 = s1 + "." + b254len + s2;

    dn("m20039 fromBase254 pdlen=%d  normal=[%s] ==> base254=[%s]", pdlen, normal, base254.s() );
}

void JagMath::fromBase254(Jstr &normal, const Jstr &base254 )
{
    dn("m322206 fromBase254 base254=[%s] len=%d", base254.s(), base254.size() );
    fromBase254(normal, base254.s() );
}

void JagMath::fromBase254(Jstr &normal, const char *base254 )
{
    dn("m322208 fromBase254 base254=[%s] len=%d", base254, strlen(base254) );
    // dumpmem( base254, strlen(base254) );

    if ( base254 && '*' == *base254 ) {
        normal = "*";
        return;
    }

    char *pdot = (char*)strchr(base254, '.');
    if ( ! pdot ) {
        long n = base254ToLong( base254 );
        normal = longToStr(n);
        dn("m38018 no . return normal=%s", normal.s() );
        return;
    }

    *pdot = '\0';
    long n1 = base254ToLong( base254 );
    *pdot = '.';
    Jstr s1 = longToStr(n1);

    bool isNeg = false;
    if ( n1 < 0 ) {
        isNeg = true;
    } else if ( 0 == n1 && *base254 == JAG_B254_NEG ) {
        isNeg = true;
        s1 = "-0";
    }

    dn("m33302828 n1=[%ld]  isNeg=%d s1=[%s]", n1, isNeg, s1.s() );

    ++pdot;
    dn("m32782208 pdot=[%s] dump:", pdot );
    //dumpmem(pdot, strlen(pdot));

    unsigned char nlen = valueOfBase254( *pdot );
    int   b254len = base254Width( nlen );
    ++pdot;

    dn("m42008733 decoded nlen=%d b254len=%d", nlen, b254len );

    //long JagMath::base254ToLongLen( const char *b254, size_t len, int inType )
    long n2;
    Jstr s2;

    if ( isNeg ) {
        n2 = base254ToLongLen( pdot, b254len, 1 );
        s2 = longToStr(-n2);
        dn("neg m33309 n2=%ld", n2 );
    } else {
        n2 = base254ToLongLen( pdot, b254len, 2 );
        s2 = longToStr(n2);
        dn("pos m33329 n2=%ld  nlen=%d =?= strlen(pdot)==b254len=%d", n2, nlen, strlen(pdot), b254len );
    }

    //dn("m602229 s1=[%s] . s2=[%s]", s1.s(), s2.s() );
    normal = s1 + "." + s2;
    //dn("m24039 fromBase254 base254=[%s] ==> normal=[%s]", base254, normal.s() );
}

void JagMath::fromBase254Len(Jstr &normal, const char *base254, int len )
{
    dn("m6500241 fromBase254Len base254=[%s] len=%d", base254, len );
    if ( base254 && '*' == *base254 ) {
        normal = "*";
        return;
    }

    char *pdot = (char*)strnchr(base254, '.', len);
    if ( ! pdot ) {
        long n = base254ToLongLen( base254, len );

        normal = longToStr(n);
        return;
    }

    *pdot = '\0';
    long n1 = base254ToLongLen( base254, pdot-base254 );

    *pdot = '.';
    Jstr s1 = longToStr(n1);
    dn("m23005 base254ToLongLen() n1=%ld s1=[%s]", n1, s1.s() );


    bool isNeg = false;
    if ( n1 < 0 ) { 
        isNeg = true;
    } else if ( 0 == n1 && *base254 == JAG_B254_NEG ) {
        isNeg = true;
        s1 = "-0";
    }

    ++pdot;
    int nlen = valueOfBase254(*pdot);
    int   b254len = base254Width( nlen );
    ++pdot;
    dn("m3333019 pdot=[%s] nlen=%d", pdot, nlen );

    long n2;
    Jstr s2;
    if ( isNeg ) {
        n2 = base254ToLongLen( pdot, b254len, 1 );
        s2 = longToStr(-n2);
    } else {
        n2 = base254ToLongLen( pdot, b254len, 2 );
        s2 = longToStr(n2);
        dn("m30111378 in fromBase254Len() pdot=[%s] strlen(pdot)=%d=?=nlen=%d n2=%ld len=%d", pdot, strlen(pdot), nlen, n2, len);
    }

    dn("m602701229 fromBase254Len() s1=[%s] . s2=[%s]", s1.s(), s2.s() );

    normal = s1 + "." + s2;
    //dn("m24039 fromBase254 base254=[%s] ==> normal=[%s]", base254, normal.s() );
}

int JagMath::base254WidthSlow( int normalWidth )
{
    unsigned long pw = ulpow(10, normalWidth);
    int cnt = 0;
    unsigned long mx = pw -1;
    while ( mx > 0 ) {
        ++cnt;
        mx = mx / 254;
    }
    return cnt;
}

// length b254 from a normal number
int JagMath::base254Width( int normalWidth )
{
    static int base254_lookup_tab[] = {
      //   1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16  17  18  19
        0, 1, 1, 2, 2, 3, 3, 3, 4, 4, 5,  5, 5, 6, 6, 7, 7, 8,  8,  8 
    };

    if ( normalWidth < 0 ) return 0;
    if ( normalWidth > 19 ) return 8;

    return base254_lookup_tab[normalWidth];
}

// required length of normal to get desired b254Width
int JagMath::normalWidth254( int b254Width )
{
    static int normal_lookup_tab[] = {
      //   1  2  3  4  5  6   7   8   9   10
        0, 2, 4, 7, 9, 12, 14, 16, 18, 19, 19 
    };

    if ( b254Width < 0 ) return 0;
    if ( b254Width >= 10 ) return 19;

    return normal_lookup_tab[b254Width];
}


////////////// protected methods ///////////////////////
// return total widt including the sign
int JagMath::frontBase254Width( const char *front, int len)
{
    if ( front == NULL || *front == '\0' ) {
        return 0;
    }

    int width;
    if ( front[0] == '-' ) {
        width = 1 + base254Width( len -1) ;
    } else {
        if ( front[0] == '-' ) {
            width = 1 + base254Width( len -1) ;
        } else {
            width = 1 + base254Width( len ) ;
        }
    }

    return width;
}

// natural byte:        [0(x), 1, 2, 3, ... 45(-), 46(.), 47(/), ......., 255]
//                             |  |  |       |             |               |
// base254 sequence inex:: [   0, 1, 2,     44,           45,             253]

// b254[1, 255] --> returns reguar value [0,255]
unsigned char JagMath::valueOfBase254( unsigned char b254 )
{
    if ( b254 < '.' ) {
        return b254 -1; // index position
    } else {
        return b254 -2; // index position 
    }
}

// c255: [1,255]
unsigned char JagMath::compliment254( unsigned char c254 )
{
    unsigned char v = valueOfBase254(c254);
    int opposite = 253 - v;
    if ( opposite <= 44 ) {
        return opposite + 1;
    } else {
        return opposite + 2;
    }
}

// natural byte:        [0(x), 1, 2, 3, ... 45(-), 46(.), 47(/), ......., 255]
//                             |  |  |       |             |               |
// base254 sequence inex:: [   0, 1, 2,     44,           45,             253]
// returns symbol in base254 set from a regular number(character)

unsigned char JagMath::base254Symbol(unsigned char b)
{
    if ( b <= 44 ) {
        return b + 1;
    } else {
        return b + 2;
    }
}


/**
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
**/

// size is number of digits in result
// returns positvie long number
long JagMath::convertPosStrToNum(const char *instr, int size)
{
    if ( NULL == instr ) return 0;
    if ( '\0' == *instr ) return 0;

    if ( size > 16 ) {
        size = 16;
    }

    if ( size < 1 ) {
        size = 1;
    }

    int frontzeros = 0;
    char *newstr = (char*)malloc(size+1);
    memset( newstr, 0 , size+1);

    int  slen = strlen(instr);
    int  newlen = 0;
    int  i = 0;

    bool tog = false;
    while (i < slen && i < size ) {
        if (tog) {
            newstr[newlen++] = instr[i];
        } else if ( instr[i] != '0' ) {
            newstr[newlen++] = instr[i];
            tog = true;
        } else {
            ++frontzeros;
        }

        if ( newlen >= size ) {
            break;
        }
  
        ++i;
    }

  
    int remainzeros = size - newlen - frontzeros;
    dn("m51000291 slen = strlen(instr=%s)=%d newlen=%d size=%d frontzeros=%d remainzeros=%d newstr=[%s]", 
        instr, slen, newlen, size, frontzeros, remainzeros, newstr );

    for ( i=0; i < remainzeros; ++i ) { 
        newstr[newlen++] = '0';
    }
  
    long res = strtol(newstr, NULL, 10);
    free( newstr );
    return res;
}

long JagMath::convertNegStrToNum(const char *instr, int size)
{
    long pn = convertPosStrToNum( instr, size );
    return -pn;
}


// 2300  or 3490299 or -21108  against nlen=8 99999999  pow(10, nlen+1)
double JagMath::longToFraction( int nlen, long n )
{
    double frac = double(n)/pow(10.0, double(nlen) );
    dn("m50009812 in longToFraction nlen=%d n=%ld frac=%f pow=%f", nlen, n, frac, pow(10.0, double(nlen)) );
    return frac;
}

int JagMath::wSig( const Jstr &b254 )
{
    JagStrSplit  sp(b254, '.');
    if ( sp.size() < 2 ) return 0;
    Jstr sig = sp[1];

    unsigned char w = sig[0];
    return valueOfBase254(w);
}

