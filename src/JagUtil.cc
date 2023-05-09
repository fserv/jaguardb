/*
 * Copyright JaguarDB  www.jaguardb.com
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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>
#include <malloc.h>
#include <libgen.h>


#ifndef _WINDOWS64_
#include <termios.h>
#else
#include <direct.h>
#include <io.h>
#include <conio.h>
#endif

#include <abax.h>
#include <JagLogLevel.h>
#include <JagStrSplit.h>
#include <JagParser.h>
#include <JagParseParam.h>
#include <JagVector.h>
#include <JagHashMap.h>
#include <JagTime.h>
#include <JagNet.h>
#include <JagSchemaRecord.h>
#include <JagFastCompress.h>
#include <JagUtil.h>
#include <JagMath.h>


/********************************************************************************
** Internal helper function
**
** str_match function matches string bigstr with smallstr up to the length of 
** smallstr from the beginning of the two strings
********************************************************************************/
static int str_match_ch( const char *s1, char ch, const char *s2, size_t *step);

static void _rwnegConvertOneCol( char *outbuf, int offset, int length, const Jstr &type );
static void _rwnegConvertAllCols( char *outbuf, int numCols, const JagSchemaAttribute *schAttr );


/*******************************************************
same as above except terminator in str1 is ch
*******************************************************/
int str_str_ch(const char *str1, char ch, const char *str2 )
{
    const char  *p1 = NULL; 
    size_t      index = -1, step =1;
    int         diff = 1;

    if ( NULL == str2 || *str2 == '\0' ) { return 0; }  
	if ( *str1 == ch ) { return -1; }


    const char *start = str1;
    while(1)
    {
        for ( p1=start; (*p1 != '\0' && *p1 != ch ) && *p1 != *str2; p1++ );
        if ( *p1 == '\0') break;
        if ( *p1 == ch ) break;

        if ( ( diff = str_match_ch (p1, ch, str2, &step ) ) > 0 ) 
        {
            start = p1 + step; 
        }
        else
        {
            if ( diff == 0 ) { index = p1 - str1; }
            break;
        }
    }

    return index;
}


/********************************************************************************
Same as str_match except terminater char in s1 is ch
********************************************************************************/
int str_match_ch( const char *s1, char ch, const char *s2, size_t *step)
{
    const char *p, *q;

    for (p = s1+1, q=s2+1; (*p != ch && *p != '\0') && *q != '\0' && *p == *q; p++, q++ );

    if ( *q == '\0' ) { return 0; }
    if ( *p == '\0' || *p == ch ) { return  -1; }

    *step = q - s2;
    return 1;
}

int str_print( const char *ptr, int len )
{
	for(int i = 0; i < len; i++) {
		if (ptr[i] == '\0') {
			printf("@");
		}
		else {
			printf("%c", ptr[i]);
		}
	}
	printf("\n");
	fflush(stdout);
	return 0;
}

// jag_strtok_r, get next token ending with one of delim; ignore strings in '', "" and ``
char * jag_strtok_r(char *s, const char *delim, char **lasts)
{
	const char *spanp; char *tok, *r;
	int c, sc;

	// if input is NULL, change input position to lasts
	if ( s == NULL ) {
		s = *lasts;
		// if lasts is NULL, return NULL
		if ( s == NULL ) {
			return NULL;
		}
	}
	
	// first, skip ( span ) any leading delimiters(s += strspn(s, delim), sort of)
	cont:
	c = *s++;   // c in "jkfdjkfjd,fjdkfjdk='fjdkfjkdfd'  fjdkf d
	for (spanp = delim; (sc = *spanp++) != 0;) {   // sc: "delimieter , "
		if (c == sc && sc != '\'' && sc != '"' && sc != '`' ) {
			goto cont;
		}
	}	
	if (c == 0) {           /* no non-delimiter characters */
		*lasts = NULL;
		return NULL;
	}
	// then, put back s to the starting position
	tok = --s;
		
	// Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
	// Note that delim must have one NULL; we stop if we see that, too.
	// printf("scan token s=[%s]\n", s );
	for (;;) {
		r = s++;
		if ( *r == '\0' ) break;
		// see any quote, scan to enclosing single quote or till end
		if ( *r == '\'' || *r == '"' || *r == '`' ) {
			r = jumptoEndQuote( r );
			if ( *r == '\0' ) {
				*lasts = r;
				return tok;
			} else {
				s = ++r;
			}
			continue;
		}
		c = *r;
		
		for (spanp = delim; (sc = *spanp++) != 0;) {
			if (c == sc && sc != '\'' && sc != '"' && sc != '`' ) {
				if (c == 0) {
					s = NULL;
				} else {
					s[-1] = 0;
				}
				*lasts = s;
				// printf("reg sep tok=[%s]  s=[%s]\n", tok, s );
				return tok;
			}
		}
	}
	*lasts = r;
	return tok;
}


// jag_strtok_r, get next token ending with one of delim; ignore strings in '', "" and ``
char * jag_strtok_r_bracket(char *s, const char *delim, char **lasts )
{
	if ( s == NULL ) {
		s = *lasts;
		if ( *s == '\0' ) {
			return NULL;
		}
	} else {
		*lasts = NULL;
	}

	while ( strchr(delim, *s) ) { ++s; }
	if ( *s == '\0' ) {
		*lasts = s;
		return NULL;
	}

	char *r = s;
	if (*s == '(' ) {
		while ( *s != ')' ) ++s;
		if ( *s == '\0' ) {
			*lasts = s;
			return r;
		}
		++s;
	}


	while ( *s ) {
		if ( *s == '(' ) {
			while ( *s != ')' ) ++s;
			if ( *s == '\0' ) {
				*lasts = s;
				return r;
			}
		}
		
		if ( strchr(delim, *s) ) {
				*s = '\0';
				*lasts = s+1;
				return r;
		} else if ( *s == '"' || *s == '\'' ) {
			s = jumptoEndQuote( s );
			if ( *s == '\0' ) {
				*lasts = s;
				return r;
			} 
		}

		++s;
	}
	
	if ( *s == '\0' ) {
		*lasts = s;
	} else {
		*lasts = s+1;
	}
	return r;
}


short memreversecmp( const char *buf1, const char *buf2, int len )
{
	for ( int i = len-1; i >= 0; --i ) {
		if ( *(buf1+i) != *(buf2+i) ) return *(buf1+i) - *(buf2+i);	
	}
	return 0;
}

int reversestrlen( const char *str, int maxlen )
{
	if ( 1 == maxlen ) return 1;

	const char *p = str+maxlen-1;
	while( *p == '\0' && p-str > 0 ) --p;
	if ( p-str == 0 ) return 0;
	else return p-str+1;
}

const char *strrchrWithQuote(const char *str, int c, bool processParenthese) 
{
	const char *result = NULL;
	const char *p = str;
	int parencnt = 0;

	while ( 1 ) {
		if ( *p == '\0' ) break;
		else if ( *p == '\'' && (p = jumptoEndQuote(p)) && *p != '\0' ) ++p;
		else if ( *p == '"' && (p = jumptoEndQuote(p)) && *p != '\0' ) ++p;
		else if ( *p == '(' && processParenthese ) {
			++parencnt;
			++p;
			while ( 1 ) {
				if ( *p == '\0' ) break;
				else if ( *p == ')' ) {
					--parencnt;
					if ( !parencnt ) break;
					else ++p;
				} else if ( *p == '\'' && (p = jumptoEndQuote(p)) && *p != '\0' ) ++p;
				else if ( *p == '"' && (p = jumptoEndQuote(p)) && *p != '\0' ) ++p;
				else if ( *p == '(' ) {
					++parencnt;
					++p;
				} else if ( *p != ')' && *p != '\0' ) ++p;
			}
			if ( *p == ')' ) ++p;			
		} else if ( *p == c ) { 
			result = p; 
			++p; 
		} else ++p;
	}
	return result;
}

char *jumptoEndQuote(const char *p) 
{
	char *q = (char*)p + 1;
	while( 1 ) {
		if ( *q == '\0' || (*q == *p && *(q-1) != '\\') ) break;
		++q;
	}
	return q;
}

Jstr strRemoveQuote( const char *p )
{
	Jstr str = p;
	if ( p == NULL || *p == '\0' ) {
	} else if ( *p != '\'' && *p != '"' && *p != '`' ) {
		str = p;
	} else {
		char *q = jumptoEndQuote( p ); ++p;
		if ( *p == '\0' || *q == '\0' || q-p <= 0 ) {
		} else {
			str = Jstr( p, q-p, q-p );
			//str = Jstr( p, q-p );
		}
	}
	return str;
}	

char *itostr( int i, char *buf )
{
	sprintf( buf, "%d", i );
	return buf;
}

char *ltostr( jagint i, char *buf )
{
	sprintf( buf, "%lld", i );
	return buf;
}

// safe pread data from a file
// 0: no more data   -1: error
ssize_t raysafepread( int fd, char *buf, jagint length, jagint startpos )
{
    ssize_t bytes = 0;
    ssize_t len;

   	len = jagpread( fd, buf, length, startpos );
	if ( len < 0 && errno == EINTR ) {
    	len = jagpread( fd, buf, length, startpos );
	}

	if ( len == length ) {
		return len;
	}

	if ( len == 0 ) {
		d("E62816 raysafepread error len==0  fd=%d length=%lld startpos=%lld [%s] return 0\n",
				 fd, length, startpos, strerror(errno) ); 
		//return 0;
		return -2;
	} else if ( len < 0 ) {
		d("E628117 raysafepread error len=%d  fd=%d length=%lld startpos=%lld [%s] return -1\n",
				len, fd, length, startpos, strerror(errno) ); 
                abort();
		return -1;
	}

    bytes += len;
    while ( bytes < length ) {
       	len = jagpread( fd, buf+bytes, length-bytes, startpos+bytes );
		if ( len < 0 && errno == EINTR ) {
        	len = jagpread( fd, buf+bytes, length-bytes, startpos+bytes );
		}

		if ( len < 0 ) {
			d("E62808 raysafepread error fd=%d length-bytes=%lld startpos+bytes=%lld [%s]\n",
				     fd, length-bytes, startpos+bytes, strerror(errno) ); 
		}

        if ( len <= 0 ) {
            return bytes;
        }

        bytes += len;
    }

    return bytes;
}

// safe pwrite data to a file
ssize_t raysafepwrite( int fd, const char *buf, jagint len, jagint startpos )
{
    size_t      nleft;
    ssize_t     nwritten;
    const char *ptr;

    ptr = buf;
    nleft = len;

    while (nleft > 0) {
        nwritten = jagpwrite(fd, ptr, nleft, startpos );
        if ( nwritten <= 0 ) {
            if (nwritten < 0 && errno == EINTR) {
                nwritten = 0;   /* and call write() again */
			} else {
				d("E290260 raysafepwrite error fd=%d len=%lld startpos=%lld [%s]\n", fd, len, startpos, strerror(errno) );
                return (-1);    /* error */
			}
        }

        nleft -= nwritten;
        ptr += nwritten;
		startpos += nwritten;
    }
    return len;
}

int rayatoi( const char *buf, int length )
{
	char *p = (char*)buf;
	char savebyte = *(p+length);
	*(p+length) = '\0';
	int value = atoi(p);
	*(p+length) = savebyte;
	return value;
}

jagint rayatol( const char *buf, int length )
{
	char *p = (char*)buf;
	char savebyte = *(p+length);
	*(p+length) = '\0';
	jagint value = jagatoll(p);
	*(p+length) = savebyte;
	return value;
}

double rayatof( const char *buf, int length )
{
	char *p = (char*)buf;
	char savebyte = *(p+length);
	*(p+length) = '\0';
	double value = jagatof(p);
	*(p+length) = savebyte;
	return value;	
}

abaxdouble raystrtold( const char *buf, int length )
{
	char *p = (char*)buf;
	char savebyte = *(p+length);
	*(p+length) = '\0';
	abaxdouble value = jagstrtold(p, NULL);
	*(p+length) = savebyte;
	return value;
}

bool formatOneCol( int tzdiff, int servtzdiff, char *outbuf, const char *inbuf, 
				   Jstr &errmsg, const Jstr &name, 
				   int offset, int length, int sig, const Jstr &type )
{
	if ( length < 1 ) {
		return 1;
	}

    dn("u800199 formatOneCol name=[%s] type=[%s] inbuf=[%s] offset=%d length=%d sig=%d", name.s(), type.s(), inbuf, offset, length, sig );
    // debug
    // if ( 0 == strcmp(inbuf, "2023-03-08") ) { abort(); }

	if ( *inbuf == '\0' ) return 1;
	int errcode = 0;
	char savebyte = *(outbuf+offset+length);
	jagint actwlen = 0;
	int writelen = 0;

	if ( type == JAG_C_COL_TYPE_DATETIMEMICRO || type == JAG_C_COL_TYPE_TIMESTAMPMICRO ) { 
		JagParseAttribute jpa( NULL, tzdiff, servtzdiff );
		errcode = JagTime::convertDateTimeFormat( jpa, outbuf, inbuf, offset, length, JAG_TIME_SECOND_MICRO );
        dn("c203938 JAG_C_COL_TYPE_DATETIMEMICRO outbuf=[%s]", outbuf );
	} else if ( type == JAG_C_COL_TYPE_DATETIMENANO || type == JAG_C_COL_TYPE_TIMESTAMPNANO ) { 
		JagParseAttribute jpa( NULL, tzdiff, servtzdiff );
		errcode = JagTime::convertDateTimeFormat( jpa, outbuf, inbuf, offset, length, JAG_TIME_SECOND_NANO );
        dn("c203338 JAG_C_COL_TYPE_DATETIMEMICRO outbuf=[%s]", outbuf );
	} else if ( type == JAG_C_COL_TYPE_DATETIMESEC || type == JAG_C_COL_TYPE_TIMESTAMPSEC ) { 
		JagParseAttribute jpa( NULL, tzdiff, servtzdiff );
		errcode = JagTime::convertDateTimeFormat( jpa, outbuf, inbuf, offset, length, JAG_TIME_SECOND );
	} else if ( type == JAG_C_COL_TYPE_DATETIMEMILLI || type == JAG_C_COL_TYPE_TIMESTAMPMILLI ) {
		JagParseAttribute jpa( NULL, tzdiff, servtzdiff );
		errcode = JagTime::convertDateTimeFormat( jpa, outbuf, inbuf, offset, length, JAG_TIME_SECOND_MILLI );
	} else if ( type == JAG_C_COL_TYPE_DATE ) { 
		errcode = JagTime::convertDateFormat( outbuf, inbuf, offset, length );
	} else if ( type == JAG_C_COL_TYPE_TIMEMICRO ) { 
		errcode = JagTime::convertTimeFormat( outbuf, inbuf, offset, length, JAG_TIME_SECOND_MICRO );
	} else if ( type == JAG_C_COL_TYPE_TIMENANO ) {
		errcode = JagTime::convertTimeFormat( outbuf, inbuf, offset, length, JAG_TIME_SECOND_NANO );
	} else if ( type == JAG_C_COL_TYPE_DBOOLEAN || type == JAG_C_COL_TYPE_DBIT ) {
		if ( atoi(inbuf) == 0 ) { 
            outbuf[offset] = '0'; 
        } else { 
            outbuf[offset] = '1'; 
        }
	} else if ( type == JAG_C_COL_TYPE_STR ) { 
		actwlen = snprintf(outbuf+offset, length+1, "%s", inbuf);
        dn("u22223001 JAG_C_COL_TYPE_STR outbuf+offset=[%s] length=%d actwlen=%d", outbuf+offset, length, actwlen );
	} else { 
		if ( inbuf[0] == '*' ) {
			memset( outbuf+offset, 0, length );
			*(outbuf+offset) = '*';
			return 1;
		}

        if ( type == JAG_C_COL_TYPE_DINT ) {
            dn("u0128289 JAG_C_COL_TYPE_DINT name=[%s] inbuf=[%s] strlen(buf)=%ld ", name.s(), inbuf, strlen(inbuf) );
            // dumpmem( inbuf, JAG_DINT_FIELD_LEN);

            if ( inbuf[0] != '\0' ) {
                long n = jagatol(inbuf);
                Jstr b;
                JagMath::base254FromLong( b, n, JAG_DINT_FIELD_LEN, JAG_B254_PREPEND_SIGN_ZERO_FRONT );
                dn("u10029 b=[%s]", b.s() );
                dn("u0029228 length=%d JAG_DINT_FIELD_LEN=%d", length, JAG_DINT_FIELD_LEN );
                memcpy(outbuf+offset, b.s(), length);
            }

            return 1;
        }

        if ( type == JAG_C_COL_TYPE_DBIGINT ) {
            dn("u0128409 JAG_C_COL_TYPE_DBIGINT inbuf=[%s] ", inbuf );

            if ( inbuf[0] != '\0' ) {
                long n = jagatol(inbuf);
                dn("u0128239 JAG_C_COL_TYPE_DINT n=%ld", n );
                Jstr b;
                JagMath::base254FromLong( b, n, length, JAG_B254_PREPEND_SIGN_ZERO_FRONT );
                dn("u10429 b=[%s]", b.s() );
                dn("u0329228 length=%d JAG_DBIGINT_FIELD_LEN=%d", length, JAG_DBIGINT_FIELD_LEN );
                memcpy(outbuf+offset, b.s(), length);
            }
            return 1;
        }

        if ( type == JAG_C_COL_TYPE_DMEDINT ) {
            if ( inbuf[0] != '\0' ) {
                long n = jagatol(inbuf);
                Jstr b;
                JagMath::base254FromLong( b, n, length, JAG_B254_PREPEND_SIGN_ZERO_FRONT );
                memcpy(outbuf+offset, b.s(), length);
            }
            return 1;
        }

        if ( type == JAG_C_COL_TYPE_DSMALLINT ) {
            if ( inbuf[0] != '\0' ) {
                long n = jagatol(inbuf);
                Jstr b;
                JagMath::base254FromLong( b, n, length, JAG_B254_PREPEND_SIGN_ZERO_FRONT );
                memcpy(outbuf+offset, b.s(), length);
            }
            return 1;
        }

        if ( type == JAG_C_COL_TYPE_DTINYINT ) {
            if ( inbuf[0] != '\0' ) {
                long n = jagatol(inbuf);
                Jstr b;
                JagMath::base254FromLong( b, n, length, JAG_B254_PREPEND_SIGN_ZERO_FRONT );
                memcpy(outbuf+offset, b.s(), length);
            }
            return 1;
        }

        if ( type == JAG_C_COL_TYPE_FLOAT ) {
            if ( inbuf[0] != '\0' ) {
                Jstr b;
                JagMath::base254FromDoubleStr( b, inbuf, length, sig );
                memcpy(outbuf+offset, b.s(), length);
            }
            return 1;
        }

        if ( type == JAG_C_COL_TYPE_DOUBLE ) {
            if ( inbuf[0] != '\0' ) {
                dn("u1112097 dump inbuf: [%s]", inbuf);
                // dumpmem( inbuf, length);
    
                Jstr b;
                JagMath::base254FromDoubleStr( b, inbuf, length, sig );
                memcpy(outbuf+offset, b.s(), length);
    
                /***
                dn("u1110288 b254 dump: b254.size=%d length=%d", b.size(), length );
                dn("u1110288 b254 dump: b254=[%s]", b.s() );
                dumpmem(b.s(), length);
                dn( "u3330339 formatOneCol DOUBLE inbuf=[%s] b254.size=[%d] length=%d offset=%d sig=%d", 
                    inbuf, b.size(), length, offset, sig );
    
                // debug
                Jstr norm;
                JagMath::fromBase254(norm, b);
                dn("u202011301 fromBase254()   inbuf=[%s] --> b254 --> norm=[%s]", inbuf, norm.s() );
    
                dn("u233098 dump outbuf offset=%d length=%d", offset, length );
                dumpmem(outbuf+offset, length);
    
                JagMath::fromBase254Len(norm, outbuf+offset, length);
                dn("u202011302 fromBase254Len  inbuf=[%s] --> b254 --> norm=[%s]", inbuf, norm.s() );
                ***/
            }

            return 1;
        }

        if ( type == JAG_C_COL_TYPE_LONGDOUBLE ) {
            if ( inbuf[0] != '\0' ) {
                Jstr b;
                JagMath::base254FromLongDoubleStr( b, inbuf, length, sig );
                memcpy(outbuf+offset, b.s(), length);
            }

            return 1;
        }

        dn("u0802005 type=[%s]", type.s() );
	}
	
    dn("u03039499 errcode=%d", errcode );
	if ( errcode == 0 ) {
		*(outbuf+offset+length) = savebyte;
		return 1;
	} 
	
	if ( errcode == 1 ) {
		if ( type == JAG_C_COL_TYPE_DATE ) {
			errmsg = "E6200 Error date format. Please correct your input.";
		} else if ( type == JAG_C_COL_TYPE_DATETIMEMICRO ) {
			errmsg = "E6202 Error datetime format. Please correct your input.";
		} else if ( type == JAG_C_COL_TYPE_DATETIMESEC ) {
			errmsg = "E6203 Error datetimesec format. Please correct your input.";
		} else if ( type == JAG_C_COL_TYPE_DATETIMENANO ) {
			errmsg = "E6204 Error datetimenano format. Please correct your input.";
		} else if ( type == JAG_C_COL_TYPE_TIMESTAMPSEC ) {
			errmsg = "E6208 Error timestampsec format. Please correct your input.";
		} else if ( type == JAG_C_COL_TYPE_TIMESTAMPMICRO ) {
			errmsg = "E6210 Error timestamp format. Please correct your input.";
		} else if ( type == JAG_C_COL_TYPE_TIMESTAMPSEC ) {
			errmsg = "E6212 Error timestampsec format. Please correct your input.";
		} else if ( type == JAG_C_COL_TYPE_DATETIMESEC ) {
			errmsg = "E6212 Error datetimesec format. Please correct your input.";
		} else if ( type == JAG_C_COL_TYPE_TIMESTAMPNANO ) {
			errmsg = "E6214 Error timestampnano format. Please correct your input.";
		} else if ( type == JAG_C_COL_TYPE_TIMESTAMPMILLI ) {
			errmsg = "E6218 Error timestampmill format. Please correct your input.";
		} else if ( type == JAG_C_COL_TYPE_DATETIMEMILLI ) {
			errmsg = "E6220 Error datetimemill format. Please correct your input.";
		} else if ( type == JAG_C_COL_TYPE_TIMEMICRO ) {
			errmsg = "E6223 Error time format. Please correct your input.";
		} else if ( type == JAG_C_COL_TYPE_STR ) {
			errmsg = "E6227 Length of string " + longToStr(actwlen) + " exceeded limit " + 
					intToStr(length) + " for column " + name + ". Please correct your input.";
		} else {
			errmsg = "E6232 Error input. Please correct your input.";
		}
	} else if ( errcode == 2 ) {
		errmsg = "E16208 Length of input " + longToStr(actwlen) + " exceeded limit " + 
				intToStr(writelen-1) + " for column " + name + ". Please correct your input.";
	} else {
		errmsg = "E16210 Error error code";
	}
	
	return 0; // error
}

void MultiDbNaturalFormatExchange( char *buffers[], int num, int numKeys[], const JagSchemaAttribute *attrs[] )
{
	for ( int i = 0; i < num; ++i ) {
		_rwnegConvertAllCols( buffers[i], numKeys[i], attrs[i] );
	}
}

void dbNaturalFormatExchange( char *buffer, int numKeys, const JagSchemaAttribute *schAttr, int offset, int length, const Jstr &type )
{
	if ( numKeys == 0 ) {
		_rwnegConvertOneCol( buffer, offset, length, type );
	} else {
		_rwnegConvertAllCols( buffer, numKeys, schAttr );
	}
}

void _rwnegConvertOneCol( char *buffer, int offset, int length, const Jstr &type )
{
    return;

    /**
    if ( buffer[offset] == JAG_C_NEG_SIGN && ( isInteger(type) || isFloat(type) ) ) {
        for ( int j = offset+1; j < offset+length; ++j ) {
            if ( buffer[j] != '.' ) {
				buffer[j] = '9' - buffer[j] + '0';
			}
        }
   }
   **/
}

void _rwnegConvertAllCols( char *buffer, int numKeys, const JagSchemaAttribute *schAttr )
{
    return;

    /**
    int offset, len;

	for ( int i = 0; i < numKeys; ++i ) {
        offset = schAttr[i].offset;
        len = schAttr[i].length;

		if ( buffer[offset] == JAG_C_NEG_SIGN 
			 && ( isInteger(schAttr[i].type) || isFloat( schAttr[i].type ) ) ) {

			for ( int j = offset+1; j < offset+len; ++j ) {
				if ( buffer[j] != '.' ) {
					buffer[j] = '9' - buffer[j] + '0';
				}
			}
		}
	}
    **/
}

Jstr makeUpperString( const Jstr &str )
{
	if ( str.length()<1) return "";

	char *p = (char*)jagmalloc( str.length() + 1 );
	p[str.length()] = '\0';
	for ( int i = 0; i < str.length(); ++i ) {
		p[i] = toupper( *(str.c_str()+i) );
	}
	Jstr res( p );
	if ( p ) free( p );
	return res;
}

Jstr makeLowerString( const Jstr &str )
{
	if ( str.length()<1) return "";

	char *p = (char*)jagmalloc( str.length() + 1 );
	p[str.length()] = '\0';
	for ( int i = 0; i < str.length(); ++i ) {
		p[i] = tolower( *(str.c_str()+i) );
	}
	Jstr res( p );
	if ( p ) free( p );
	return res;
}

JagFixString makeUpperOrLowerFixString( const JagFixString &str, bool isUpper )
{
	JagFixString res;
	if ( str.length() < 1 ) return res;
	char * p = (char*)jagmalloc( str.length() + 1 );
	p[str.length()] = '\0';
	for ( int i = 0; i < str.length(); ++i ) {
		if ( isUpper ) {
			p[i] = toupper( *(str.c_str()+i) );
		} else {
			p[i] = tolower( *(str.c_str()+i) );
		}
	}
	res = JagFixString( p, str.length(), str.length() );
	if ( p ) free( p );
	return res;
}

Jstr trimChar( const Jstr &str, char c )
{
	Jstr s1 = trimHeadChar( str, c);
	Jstr s2 = trimTailChar( s1, c);
	return s2;
}

Jstr trimHeadChar( const Jstr &str, char c )
{
	if ( str.size() < 1 ) return str;
	char *p = (char*)str.c_str();
	if ( *p != c ) return str; 
	while ( *p == c ) ++p;
	if ( *p == '\0' ) return "";
	return Jstr(p);
}

Jstr trimTailChar( const Jstr &str, char c )
{
	char v;
	if ( str.size() < 1 ) return str;
	char *p = (char*)str.c_str() + str.size()-1;
	if ( *p != c ) return str; 
	while ( *p == c && p >= str.c_str() ) --p;
    if ( p < str.c_str() ) return ""; 

	v = *(p+1);
	*(p+1) = '\0';
	Jstr newstr = Jstr( str.c_str() );
	*(p+1) = v;
	return newstr;
}

Jstr trimTailLF( const Jstr &str )
{
	char v;
	if ( str.size() < 1 ) return str;
	char *p = (char*)str.c_str() + str.size()-1;
	if ( *p != '\r' && *p != '\n' ) return str; 
	while ( (*p =='\r' || *p=='\n' ) && p >= str.c_str() ) --p;
    if ( p < str.c_str() ) return ""; 

	v = *(p+1);
	*(p+1) = '\0';
	Jstr newstr = Jstr( str.c_str() );
	*(p+1) = v;
	return newstr;
}

bool endWith( const Jstr &str, char c )
{
	if ( str.size() < 1 ) return false;
	char *p = (char*)str.c_str() + str.size()-1;
	if ( *p == c ) return true; 
	return false;
}

bool endWithStr( const Jstr &str, const Jstr &end )
{
	const char *p = jagstrrstr( str.c_str(), end.c_str() );
	if ( ! p ) return false;
	if ( strlen(p) != end.size() ) {
		return false;
	}

	return true;
}

bool endWith( const AbaxString &str, char c )
{
	if ( str.size() < 1 ) return false;
	char *p = (char*)str.c_str() + str.size()-1;
	if ( *p == c ) return true; 
	return false;
}

// skip white spaces at end and check
bool endWhiteWith( const AbaxString &str, char c )
{
	if ( str.size() < 1 ) return false;
	char *p = (char*)str.c_str() + str.size()-1;
	while ( p != (char*)str.c_str() ) {
		if ( *p == c ) return true; 

		if ( isspace(*p) ) { --p; }
		else break;
	}
	return false;
}

// skip white spaces at end and check
bool endWhiteWith( const Jstr &str, char c )
{
	if ( str.size() < 1 ) return false;
	char *p = (char*)str.c_str() + str.size()-1;
	while ( p != (char*)str.c_str() ) {
		if ( *p == c ) return true; 

		if ( isspace(*p) ) { --p; }
		else break;
	}
	return false;
}


#ifdef _WINDOWS64_
const char *strcasestr(const char *s1, const char *s2)
{
    if (s1 == 0 || s2 == 0) return 0;
    size_t n = strlen(s2);
   
    while(*s1) {
    	if(!strncasecmp(s1++,s2,n))
		{
     		return (s1-1);
		}
    }
    return 0;
}
#endif

Jstr intToStr( int i )
{
	char buf[16];
	memset(buf, 0, 16 );
	sprintf(buf, "%d", i );
	return Jstr(buf);
}

Jstr longToStr( jagint i )
{
	char buf[32];
	memset(buf, 0, 32 );
	sprintf(buf, "%lld", i );
	return Jstr(buf);
}

Jstr doubleToStr( double f )
{
	char buf[48];
	memset(buf, 0, 48 );
	sprintf(buf, "%.10f", f );

	/***
	// debug
	printf("u038277 doubleToStr buf=[%s]\n", buf);
	Jstr a = Jstr(buf).trim0();
	printf("u038277 doubleToStr a.trim0=[%s]\n", a.s());
	// end debug
	***/

	return Jstr(buf).trim0();
}

Jstr d2s( double f )
{
	char buf[48];
	memset(buf, 0, 48 );
	sprintf(buf, "%.10f", f );
	return Jstr(buf).trim0();
}

Jstr doubleToStr( double f, int maxlen, int sig )
{
	char buf[64];
	memset(buf, 0, 64 );
	snprintf( buf, 64, "%0*.*f", maxlen, sig, f);

	// debug
	/***
	printf("u038278 doubleToStr maxlen=%d sig=%d buf=[%s]\n", maxlen, sig, buf);
	Jstr a = Jstr(buf).trim0();
	printf("u038278 doubleToStr a.trim0=[%s]\n", a.s());
	// end debug
	***/

	return Jstr(buf).trim0();
}


Jstr longDoubleToStr( abaxdouble f )
{
	char buf[64];
	memset(buf, 0, 64 );

	#ifdef _WINDOWS64_
	sprintf(buf, "%f", f );
	#else
	sprintf(buf, "%Lf", f );
	#endif

	// debug
	/***
	printf("u038288 longDoubleToStr buf=[%s]\n", buf);
	Jstr a = Jstr(buf).trim0();
	printf("u038288 longDoubleToStr a.trim0=[%s]\n", a.s());
	***/
	// end debug

	return Jstr(buf).trim0();
}

/***
int jagsprintfLongDouble( int mode, bool fill, char *buf, abaxdouble i, jagint maxlen )
{
	int rc;
	#ifdef _WINDOWS64_
	if ( 0 == mode ) {
		if ( !fill ) rc = sprintf( buf, "%f", i );
		else rc = sprintf( buf, "%0*.*f", JAG_MAX_INT_LEN, JAG_MAX_SIG_LEN, i);
	} else {
		if ( !fill ) rc = snprintf( buf, maxlen+1, "%f", i );
		else rc = snprintf( buf, maxlen+1, "%0*.*f", JAG_MAX_INT_LEN, JAG_MAX_SIG_LEN, i);
	}
	#else
	if ( 0 == mode ) {
		if ( !fill ) rc = sprintf( buf, "%Lf", i );
		else rc = sprintf( buf, "%0*.*Lf", JAG_MAX_INT_LEN, JAG_MAX_SIG_LEN, i);
	} else {
		if ( !fill ) rc = snprintf( buf, maxlen+1, "%Lf", i );
		else rc = snprintf( buf, maxlen+1, "%0*.*Lf", JAG_MAX_INT_LEN, JAG_MAX_SIG_LEN, i);
	}
	#endif
    // JAG_MAX_INT_LEN is used by groupby aggregate funcs
	return rc;
}
**/

bool lastStrEqual( const char *bigstr, const char *smallstr, int lenbig, int lensmall )
{
	lensmall = reversestrlen( smallstr, lensmall );
	if ( lensmall < 1 ) return false;

	lenbig = reversestrlen( bigstr, lenbig );
	if ( lensmall > lenbig ) {
		return false;
	}

	if ( 0 == memcmp(bigstr+lenbig-lensmall, smallstr, lensmall) ) {
		return true;
	} else {
		return false;
	}
}

bool isInteger( const Jstr &dtype )
{
	if ( dtype == JAG_C_COL_TYPE_DINT  ||
		dtype == JAG_C_COL_TYPE_DBIGINT   ||
		dtype == JAG_C_COL_TYPE_DBOOLEAN  ||
		dtype == JAG_C_COL_TYPE_DBIT  ||
		dtype == JAG_C_COL_TYPE_DTINYINT  ||
		dtype == JAG_C_COL_TYPE_DSMALLINT  ||
		dtype == JAG_C_COL_TYPE_DMEDINT )
	 {
	 	return true;
	 }
	 return false;
}

bool isFloat( const Jstr &colType )
{
	 if ( colType == JAG_C_COL_TYPE_DOUBLE || colType == JAG_C_COL_TYPE_FLOAT || colType == JAG_C_COL_TYPE_LONGDOUBLE ) {
	 	return true;
	 }

	 return false;
}

bool isTime( const Jstr &dtype ) 
{
	if ( dtype == JAG_C_COL_TYPE_TIMENANO ||
		dtype == JAG_C_COL_TYPE_TIMEMICRO ) 
	{
		return true;
	}
	return false;
}

bool isDateTime( const Jstr &dtype ) 
{
	if ( isDateAndTime( dtype ) ) return true;
	if ( isTime( dtype ) ) return true;
	if ( dtype == JAG_C_COL_TYPE_DATE ) return true;

	return false;
}

bool isDateAndTime( const Jstr &dtype ) 
{
	if ( dtype == JAG_C_COL_TYPE_DATETIMEMICRO  ||
		 dtype == JAG_C_COL_TYPE_TIMESTAMPMICRO  ||
		 dtype == JAG_C_COL_TYPE_TIMESTAMPSEC  ||
		 dtype == JAG_C_COL_TYPE_DATETIMESEC  ||
		 dtype == JAG_C_COL_TYPE_DATETIMEMILLI  ||
		 dtype == JAG_C_COL_TYPE_TIMESTAMPMILLI  ||
		 dtype == JAG_C_COL_TYPE_DATETIMENANO  ||
		 dtype == JAG_C_COL_TYPE_TIMESTAMPNANO )
	{
		return true;
	}
	return false;
}

short getSimpleEscapeSequenceIndex( char p )
{
	if ( p == 'a' ) return 7;
	else if ( p == 'b' ) return 8;
	else if ( p == 't' ) return 9;
	else if ( p == 'n' ) return 10;
	else if ( p == 'v' ) return 11;
	else if ( p == 'f' ) return 12;
	else if ( p == 'r' ) return 13;
	else if ( p == '"' ) return 34;
	else if ( p == '\'' ) return 39;
	else if ( p == '?' ) return 63;
	else if ( p == '\\' ) return 92;
	else return 0;
}

// strip ending \n and \r
int stripStrEnd( char *msg, int len )
{
    int i;
	int t = len;
    for ( i=len-1; i>=0; --i ) {
        if ( msg[i] == '\n' ) {  msg[i] = '\0'; --t; }
        else if ( msg[i] == '\r' ) { msg[i] = '\0'; --t; }
    }

    return t;
}

// replace ending \n and \r  --> ' '
void replaceStrEnd( char *msg, int len )
{
    int i;
    for ( i=len-1; i>=0; --i ) {
        if ( msg[i] == '\n' ) {  msg[i] = ' '; }
        else if ( msg[i] == '\r' ) { msg[i] = ' '; }
    }

	// leave only one end ' '
    for ( i=len-1; i>=1; --i ) {
		if ( msg[i-1] == ' ' && msg[i] == ' ' ) {
			 msg[i] = '\0';
		} else {
			break;
		}
	}
}

int trimEndChar ( char *msg, char c )
{
	if ( ! msg ) return 0;
    if ( *msg == '\0' ) return 0;

    int len = strlen(msg);
	char *p = msg + len-1;
	while ( p != msg ) {
		if ( isspace(*p) || *p == c ) { *p = '\0'; --p; }
		else break;
	} 

	return 1;
}

int trimEndWithChar ( char *msg, int len, char c )
{
	if ( ! msg ) return 0;
	char *p = msg + len-1;
	while ( p != msg ) {
		if ( isspace(*p) ) { *p = '\0'; --p; }
		else break;
	} 

	if ( *p == c ) {
		return 1;
	} else {
		return 0;
	}
}

int trimEndToChar ( char *msg, int len, char stopc )
{
	if ( ! msg ) return 0;
	if ( *msg == '\0' ) return 0;

	char *p = msg + len-1;
	while ( p != msg ) {
		if ( *p == stopc ) break;
		*p = '\0'; --p; 
	} 

	return 1;
}

int trimEndWithCharKeepNewline ( char *msg, int len, char c )
{
	if ( ! msg ) return 0;
	char *p = msg + len-1;
	int hasEndC = 0;
	while ( p != msg ) {
		if ( isspace(*p) ) { --p; }
		else {
			if ( *p == c ) { hasEndC = 1; }
			break;
		}
	} 

	p = msg + len-1;
	if ( hasEndC ) {
		while ( p != msg ) {
			if ( isspace(*p) ) { *p = '\0'; --p; }
			else break;
		}
	}

	return hasEndC;
}

Jstr intToString( int i ) 
{
	char buf[16];
	sprintf(buf, "%d", i );
	return Jstr( buf );
}
Jstr longToString( jagint i ) 
{
	char buf[32];
	sprintf(buf, "%lld", i );
	return Jstr( buf );
}
Jstr ulongToString( jaguint i ) 
{
	char buf[32];
	sprintf(buf, "%llu", i );
	return Jstr( buf );
}

jagint strchrnum( const char *str, char ch )
{
    if ( ! str || *str == '\0' ) return 0;

    jagint cnt = 0;
    const char *q;
    while ( 1 ) {
        q=strchr(str, ch);
        if ( ! q ) break;
        ++cnt;
        str = q+1;
    }

    return cnt;
}

jagint strchrnumskip( const char *str, char ch )
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

void escapeNewline( const Jstr &instr, Jstr &outstr )
{
	if ( instr.size() < 1 ) return;

	jagint n = strchrnum( instr.c_str(), '\n' );
	char *str = (char*)jagmalloc( instr.size() + 2*n + 1 );
	const char *p = instr.c_str();
	char *q = str;
	while ( *p != '\0' ) {
		if ( *p == '\n' ) {
			*q++ = '\\';
			*q++ = 'n';
		} else {
			*q++ = *p;
		}
		++p;
	}
	*q++ = '\0';
	outstr = Jstr(str);
	jagfree( str );
}

int strInStr( const char *str, int len, const char *str2 )
{
    const char *p = str2;
    if ( !p ) return 0;
    if ( *p == '\0' ) return 0;
    if ( !str ) return 0;
    if ( *str == '\0' ) return 0;

    int i;
    int found = 0;
    const char *q;
    while ( *p != '\0' ) {
        found = 1;
        q = p;
        for ( i = 0; i < len; ++i ) {
            if ( str[i] != *q++ ) {
                found = 0;
                break;
            }
        }

        if ( found ) { break; }
        ++p;
    }

    return found;
}

void splitFilePath( const char *fpath, Jstr &first, Jstr &last )
{
	char *p = (char*)strrchr( fpath, '.' );
	if ( !p ) {
		first = fpath;
		return;
	}

	*p = '\0';
	first = fpath;
	last = p+1;
	*p = '.';
}

Jstr makeDBObjName( JAGSOCK sock, const Jstr &dbname, const Jstr &objname )
{
	Jstr dbobj;
	dbobj = dbname + "." + objname;
	return dbobj;
}

Jstr makeDBObjName( JAGSOCK sock, const Jstr &dbdotobj )
{
	JagStrSplit split( dbdotobj, '.' );
	return makeDBObjName( sock, split[0], split[1] );
}

Jstr jaguarHome()
{
	Jstr jaghm;
	const char *p = getenv("JAGUAR_HOME");
	if (  ! p ) {
		p = getenv("HOME");
		if ( p ) {
			jaghm = Jstr(p) + "/jaguar";
		} else {
			#ifdef _WINDOWS64_
		       p = "/c/jaguar";
		       _mkdir( p );
			#else
		  	  p = "/tmp/jaguar";
		      ::mkdir( p, 0777 );
		    #endif
			jaghm = Jstr(p);
		}
	} else {
		jaghm = Jstr(p);
	}
	
	return jaghm;
}

// Used for  yes or nor tests.  a should be lowercase
bool startWith( const Jstr &str, char a )
{
	if ( str.size() < 1 ) return false;
	if ( *(str.c_str()) == a ) return true;
	if ( *(str.c_str()) == toupper(a) ) return true;
	return false;

}

// safe read data from a file
ssize_t raysaferead( int fd, char *buf, jagint len )
{
     size_t  nleft;
     ssize_t nread;
     char   *ptr;

     ptr = buf;
     nleft = len;
     while (nleft > 0) {
         if ( (nread = ::read(fd, ptr, nleft)) < 0) {
             if (errno == EINTR)
                 nread = 0;      /* and call read() again */
             else
                 return (-1);
         } else if (nread == 0)
             break;              /* EOF */

         nleft -= nread;
         ptr += nread;
     }
     return (len - nleft);         /* return >= 0 */
}

ssize_t raysaferead( int fd, unsigned char *buf, jagint len )
{
     size_t  nleft;
     ssize_t nread;
     unsigned char   *ptr;

     ptr = buf;
     nleft = len;
     while (nleft > 0) {
         if ( (nread = ::read(fd, ptr, nleft)) < 0) {
             if (errno == EINTR)
                 nread = 0;      /* and call read() again */
             else
                 return (-1);
         } else if (nread == 0)
             break;              /* EOF */

         nleft -= nread;
         ptr += nread;
     }
     return (len - nleft);         /* return >= 0 */
}


// safe write data to a file
ssize_t raysafewrite( int fd, const char *buf, jagint len )
{
    size_t nleft;
    ssize_t nwritten;
    const char *ptr;

    ptr = buf;
    nleft = len;
    while (nleft > 0) {
        if ( (nwritten = ::write(fd, ptr, nleft)) <= 0) {
            if (nwritten < 0 && errno == EINTR)
                nwritten = 0;   /* and call write() again */
            else
                return (-1);    /* error */
        }

        nleft -= nwritten;
        ptr += nwritten;
    }
    return len;
}

int selectServer( const JagFixString &min, const JagFixString &max, const JagFixString &inkey )
{
	const char *p1, *p2, *t;
	int i, mid;

	p1 = min.c_str();
	p2 = max.c_str();
	t = inkey.c_str();

	for ( i = 0; i < min.size(); ++i ) {
		if ( *p1 == *p2 ) {
			if ( *t > *p1 ) {
				return 1;
			} else { 
				return 0;
			}
			++p1; ++p2; ++t;
			continue;
		}

		mid = ( *p1 + *p2 )/2;
		if ( *t > mid ) {
			return 1;
		} else {
			return 0;
		}
	}

	return 0;
}


// Available Memory to use in bytes
jagint availableMemory( jagint &callCount, jagint lastBytes )
{
	jagint bytes;
	#ifndef _WINDOWS64_
		bytes = sysconf( _SC_PAGESIZE ) * sysconf( _SC_AVPHYS_PAGES );
    #else
        MEMORYSTATUSEX memInfo;
		memInfo.dwLength = sizeof( memInfo );
        GlobalMemoryStatusEx (&memInfo);
        bytes = memInfo.ullAvailPhys;
	#endif

	if ( callCount < 0 ) {
		return bytes;
	} else if ( callCount == 0 ) {
		++callCount;
		return bytes;
	} else if ( callCount >= 100000 ) {
		callCount = 0;
		return bytes;
	} else {
		if ( lastBytes <= 0 ) {
			callCount = 0;
			return bytes;
		} else {
			++callCount;
			return lastBytes;
		}
	}
}

int checkReadOrWriteCommand( const char *pmesg )
{
	if ( 0 == strncasecmp( pmesg, "insert", 6 ) || 0 == strncasecmp( pmesg, "finsert", 7 ) ||
		 0 == strncasecmp( pmesg, "create", 6 ) || 0 == strncasecmp( pmesg, "alter", 5 ) ||
		 0 == strncasecmp( pmesg, "update", 6 ) || 0 == strncasecmp( pmesg, "delete", 6 ) ||
		 0 == strncasecmp( pmesg, "use ", 4 ) || 0 == strncmp( pmesg, "import", 6 ) ||
		 0 == strncasecmp( pmesg, "drop", 4 ) || 0 == strncasecmp( pmesg, "truncate", 8 ) ||
		 0 == strncasecmp( pmesg, "createdb", 8 ) || 0 == strncasecmp( pmesg, "dropdb", 6 ) ||
		 0 == strncasecmp( pmesg, "createuser", 10 ) || 0 == strncasecmp( pmesg, "dropuser", 8 ) ||
		 0 == strncasecmp( pmesg, "grant", 5 ) || 0 == strncasecmp( pmesg, "revoke", 6 ) ||
		 0 == strncasecmp( pmesg, "changepass", 10 ) || 0 == strncasecmp( pmesg, "changedb", 8 ) ) {
		 return JAG_WRITE_SQL;
	}
	return JAG_READ_SQL;
}

int checkReadOrWriteCommand( int qmode )
{
	if ( 1 == qmode || 7 == qmode || 3 == qmode ) {
		 return JAG_WRITE_SQL;
	} else {
		return JAG_READ_SQL;
	}
}

int checkColumnTypeMode( const Jstr &type )
{
	if ( type == JAG_C_COL_TYPE_STR ) return 1;
	else if ( type == JAG_C_COL_TYPE_DBOOLEAN || type == JAG_C_COL_TYPE_DBIT ) return 2;
	else if ( type == JAG_C_COL_TYPE_DINT  || type == JAG_C_COL_TYPE_DTINYINT  || 
				type == JAG_C_COL_TYPE_DSMALLINT  || type == JAG_C_COL_TYPE_DMEDINT  ) return 3;
	else if ( type == JAG_C_COL_TYPE_DBIGINT  ) return 4;
	else if ( type == JAG_C_COL_TYPE_FLOAT  || type == JAG_C_COL_TYPE_DOUBLE || type == JAG_C_COL_TYPE_LONGDOUBLE  ) return 5;
	else if ( isDateTime(type) ) return 6;
	return 0;
}

Jstr formOneColumnNaturalData( const char *buf, jagint offset, jagint length, const Jstr &type )
{
	Jstr instr, outstr;
	int rc = checkColumnTypeMode( type );

    // no need for base254

	if ( 6 == rc ) {
		if ( type == JAG_C_COL_TYPE_DATETIMEMICRO || type == JAG_C_COL_TYPE_TIMESTAMPMICRO ) {
			instr = Jstr( buf+offset, length, length );
			JagTime::convertDateTimeToStr( instr, outstr, false, JAG_TIME_SECOND_MICRO );
		} else if ( type == JAG_C_COL_TYPE_DATETIMENANO || type == JAG_C_COL_TYPE_TIMESTAMPNANO ) {
			instr = Jstr( buf+offset, length, length );
			JagTime::convertDateTimeToStr( instr, outstr, false, JAG_TIME_SECOND_NANO );
		} else if ( type == JAG_C_COL_TYPE_DATETIMESEC || type == JAG_C_COL_TYPE_TIMESTAMPSEC ) {
			instr = Jstr( buf+offset, length, length );
			JagTime::convertDateTimeToStr( instr, outstr, false, JAG_TIME_SECOND );
		} else if ( type == JAG_C_COL_TYPE_DATETIMEMILLI || type == JAG_C_COL_TYPE_TIMESTAMPMILLI ) {
			instr = Jstr( buf+offset, length, length );
			JagTime::convertDateTimeToStr( instr, outstr, false, JAG_TIME_SECOND_MILLI );
		} else if ( type == JAG_C_COL_TYPE_TIMEMICRO ) {
			instr = Jstr( buf+offset, length, length );
			JagTime::convertTimeToStr( instr, outstr, JAG_TIME_SECOND_MICRO );
		} else if ( type == JAG_C_COL_TYPE_TIMENANO ) {
			instr = Jstr( buf+offset, length, length );
			JagTime::convertTimeToStr( instr, outstr, JAG_TIME_SECOND_NANO );
		} else if ( type == JAG_C_COL_TYPE_DATE ) {
			instr = Jstr( buf+offset, length, length );
			JagTime::convertDateToStr( instr, outstr );
		}
	} else if ( 5 == rc ) {
		outstr = longDoubleToStr( raystrtold(buf+offset, length) );
	} else if ( 4 == rc ) {
		outstr = longToStr( rayatol(buf+offset, length) );
	} else if ( 3 == rc ) {
		outstr = intToStr( rayatoi(buf+offset, length) );
	} else if ( 2 == rc ) {
		outstr = intToStr( rayatoi(buf+offset, length) != 0 );
	} else if ( 1 == rc ) {
		char *pbuf = (char*)buf;
		char v = *(pbuf+offset+length);
		*(pbuf+offset+length) = NBT;
		outstr = Jstr(pbuf+offset);
		*(pbuf+offset+length) = v;
		d("u222208 outstr=[%s]\n", outstr.s() );
	}

	return outstr;
}

int rearrangeHdr( int num, const JagHashStrInt *maps[], const JagSchemaAttribute *attrs[], 
					JagParseParam *parseParam, const JagVector<SetHdrAttr> &spa, Jstr &newhdr, Jstr &gbvhdr,
					jagint &finalsendlen, jagint &gbvsendlen, bool needGbvs )
{
	int 		rc, collen, siglen, constMode = 0, typeMode = 0;
	bool 		isAggregate;
	jagint 		offset = 0;
	const 		JagSchemaRecord *records[num];
	Jstr 		type, hdr, tname;
	int 		groupnum = parseParam->groupVec.size();
	ExprElementNode *root;

    dn("s9393011 rearrangeHdr num=%d selColVec.size=%d", num, parseParam->selColVec.size() );

	gbvsendlen = 0;

	if ( !parseParam->hasColumn && num == 1 ) {
        dn("s542003");
		newhdr = spa[0].sstring;
		finalsendlen = spa[0].record->keyLength + spa[0].record->valueLength;

		if ( parseParam->opcode == JAG_GETFILE_OP ) {
			JagSchemaRecord record;
			newhdr = record.formatHeadRecord();
			for ( int i = 0; i < parseParam->selColVec.size(); ++i ) {
				type = JAG_C_COL_TYPE_STR;
				collen = JAG_FILE_FIELD_LEN;
				siglen = 0;
				tname = parseParam->selColVec[i].getfileCol;
				if ( JAG_GETFILE_SIZE == parseParam->selColVec[i].getfileType ) {
					tname += "_size";
				} else if ( JAG_GETFILE_TIME == parseParam->selColVec[i].getfileType ) {
					tname += "_time";
				} else if ( JAG_GETFILE_MD5SUM == parseParam->selColVec[i].getfileType ) {
					tname += "_md5";
				} else if ( JAG_GETFILE_FPATH == parseParam->selColVec[i].getfileType ) {
					tname += "_fpath";
				}

				newhdr += record.formatColumnRecord( tname.c_str(), type.c_str(), offset, collen, siglen );

				parseParam->selColVec[i].offset = offset;
				parseParam->selColVec[i].length = collen;
				parseParam->selColVec[i].sig = siglen;
				parseParam->selColVec[i].type = type;
				offset += collen;
                dn("u89023 JAG_GETFILE_OP parseParam->selColVec[%d].offset=%d length=%d parseParam=%p", i, offset, collen, parseParam ); 
			}
			newhdr += record.formatTailRecord();
			finalsendlen = offset;
		}
	} else {
        dn("s542003 num=%d", num);
		for ( int i = 0; i < num; ++i ) {
			records[i] = spa[i].record;
		}	

		newhdr = records[0]->formatHeadRecord();
		if ( !parseParam->hasColumn ) {	

			for ( int i = 0; i < num; ++i ) {
				const JagVector<JagColumn> &cv = *(records[i]->columnVector);

				for ( int j = 0; j < cv.size(); ++j ) {
					if ( num == 1 ) {
						hdr = cv[j].name.c_str();
					} else {	
						hdr = spa[i].dbobj + "." + Jstr(cv[j].name.c_str());
					}

					newhdr += records[i]->formatColumnRecord( hdr.c_str(), cv[j].type.c_str(), offset, cv[j].length, cv[j].sig );
					offset += cv[j].length;
				}
			}	
		} else {
            dn("s2640012 has column parseParam->selColVec.size=%d", parseParam->selColVec.size() );

			for ( int i = 0; i < parseParam->selColVec.size(); ++i ) {
				isAggregate = false;
				root = parseParam->selColVec[i].tree->getRoot();

                dn("s3454001 setFuncAttribute ...");
				rc = root->setFuncAttribute( maps, attrs, constMode, typeMode, isAggregate, type, collen, siglen );
				if ( 0 == rc ) {
                    in("s39004 Error setFuncAttribute");
					return 0;
				}

				dn("u2046 i=%d name=[%s] type=[%s] collen=%d siglen=%d", i, parseParam->selColVec[i].name.s(), type.s(), collen, siglen );

				newhdr += records[0]->formatColumnRecord( parseParam->selColVec[i].asName.c_str(), 
														  type.c_str(), offset, collen, siglen );

				dn("u2046 i=%d name=[%s] asName=[%s] type=[%s] offset=%d collen=%d isAggregate=%d",
					i, parseParam->selColVec[i].name.c_str(), parseParam->selColVec[i].asName.c_str(), 
					type.c_str(), offset, collen, isAggregate );

				parseParam->selColVec[i].offset = offset;
				parseParam->selColVec[i].length = collen;
				parseParam->selColVec[i].sig = siglen;
				parseParam->selColVec[i].type = type;
				parseParam->selColVec[i].isAggregate = isAggregate;
				offset += collen;

                dn("u211009 i=%d offset=%d length=%d", i, offset, collen );
			}
		}
		newhdr += records[0]->formatTailRecord();
		finalsendlen = offset;
	}
	
	if ( !parseParam->hasGroup || !parseParam->hasColumn || !needGbvs ) {
        dn("c0338290 newhdr=[%s]", newhdr.s() );
		return 1;
	}

	for ( int i = 0; i < num; ++i ) {
		records[i] = spa[i].record;
	}	

	offset = 0;
	gbvhdr = records[0]->formatHeadRecord( );
	for ( int i = 0; i < parseParam->selColVec.size(); ++i ) {
		isAggregate = false;
		root = parseParam->selColVec[i].tree->getRoot();
		rc = root->setFuncAttribute( maps, attrs, constMode, typeMode, isAggregate, type, collen, siglen );
		if ( 0 == rc ) {
			in("s371005 Error setFuncAttribute\n");
			return 0;
		}

		if ( i < groupnum ) {
			gbvhdr += records[0]->formatColumnRecord( parseParam->selColVec[i].asName.c_str(), 
													  type.c_str(), offset, collen, siglen, true );
		} else {
			gbvhdr += records[0]->formatColumnRecord( parseParam->selColVec[i].asName.c_str(), 
													  type.c_str(), offset, collen, siglen, false );
		}
		offset += collen;

		d("u12342 gbvhdr=[%s] ...\n", gbvhdr.s() );
	}

	gbvhdr += records[0]->formatTailRecord( );
	gbvsendlen = offset;

    dn("c392222010 gbvhdr=[%s]", gbvhdr.s() );

	return 2;
}

int checkGroupByValidation( const JagParseParam *parseParam )
{
	int grouplen = 0;
	if ( parseParam->selColVec.size() < parseParam->groupVec.size() ) {
		return 0;
	}

	for ( int i = 0; i < parseParam->groupVec.size(); ++i ) {
		if ( parseParam->groupVec[i].name != parseParam->selColVec[i].asName ) {
			return 0;
		}
		grouplen += parseParam->selColVec[i].length;
	}
	return grouplen;
}


int checkAndReplaceGroupByAlias( JagParseParam *parseParam )
{
	bool found;
	Jstr gstr;
	for ( int i = 0; i < parseParam->groupVec.size(); ++i ) { 
		found = false;
		for ( int j = 0; j < parseParam->selColVec.size(); ++j ) {
			if ( parseParam->groupVec[i].name == parseParam->selColVec[j].asName ) {
				if ( parseParam->selColVec[j].givenAsName ) {
					gstr += parseParam->selColVec[j].origFuncStr;
					parseParam->groupVec[i].name = parseParam->selColVec[j].origFuncStr;
				} else {
					gstr += parseParam->selColVec[j].asName;
				}
				found = true;
				break;
			}
		}
		if ( !found ) {
			return 0;
		}
		if ( i != parseParam->groupVec.size()-1 ) {
			gstr += ",";
		}
	}
	parseParam->selectGroupClause = gstr;
	return 1;
}

void convertToHashMap( const Jstr &kvstr, char sep,  JagHashMap<AbaxString, AbaxString> &hashmap )
{
	if ( kvstr.size()<1) return;
	JagStrSplit sp( kvstr, sep, true );
	if ( sp.length()<1) return;
	Jstr kv;
	for ( int i=0; i < sp.length(); ++i ) {
		kv = sp[i];
		JagStrSplit p( kv, '=');
		if ( p.length()>1 ) {
			hashmap.addKeyValue( p[0], p[1] );
		}
	}
}

void changeHome( Jstr &fpath )
{
	JagStrSplit oldsp( fpath, '/', true );
	char *home = getenv("JAGUAR_HOME");
	if ( ! home ) home = getenv("HOME");
	int idx = 0;
	for ( int i = oldsp.length()-1; i>=0; --i ) {
		if ( oldsp[i] == "jaguar" ) {
			idx = i;
			break; // found last "jaguar"
		}
	}

	if ( 0 == idx ) {
		printf("Fatal error: %s path is wrong\n", fpath.c_str() );
		return;
	}

	fpath = home;
	for ( int i = idx; i < oldsp.length(); ++i ) {
		fpath += Jstr("/") + oldsp[i];
	}
	//d("s5881 changeHome out=[%s]\n", fpath.c_str() );

}

int jaguar_mutex_lock(pthread_mutex_t *mutex)
{
	int rc = pthread_mutex_lock( mutex );
	if ( 0 != rc ) {
		d("s6803 error pthread_mutex_lock(%0x) [%s]\n", mutex, strerror( rc ) );
	}
	return rc;
}

int jaguar_mutex_unlock(pthread_mutex_t *mutex)
{
	int rc = pthread_mutex_unlock( mutex );
	if ( 0 != rc ) {
		d("s6804 error pthread_mutex_unlock(%0x) [%s]\n", mutex, strerror( rc ) );
	}
	return rc;
}

int jaguar_cond_broadcast( pthread_cond_t *cond)
{
	int rc = pthread_cond_broadcast( cond );
	if ( 0 != rc ) {
		d("s6805 error pthread_cond_broadcast(%0x) [%s]\n", cond, strerror( rc ) );
	}
	return rc;
}

int jaguar_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
	int rc = pthread_cond_wait(cond, mutex);
	if ( 0 != rc ) {
		d("s6807 error pthread_cond_wait(%0x %0x) [%s]\n", cond, mutex, strerror( rc ) );
	}
	return rc;
}

int getPassword( Jstr &outPassword )
{
	outPassword = "";
    char password[128];
	#ifndef _WINDOWS64_
    struct termios oflags, nflags;
    tcgetattr(fileno(stdin), &oflags);
    nflags = oflags;
    nflags.c_lflag &= ~ECHO;
    nflags.c_lflag |= ECHONL;
    if (tcsetattr(fileno(stdin), TCSANOW, &nflags) != 0) {
        perror("tcsetattr");
        return 0;;
    }

    char *pw = fgets(password, sizeof(password), stdin);
    if ( ! pw ) {
        return 0;
    }

    tcsetattr(fileno(stdin), TCSANOW, &oflags);
	#else
	memset( password, 0, 12 );
	getWinPass( password );
	#endif

    password[strlen(password) - 1] = 0;
	outPassword = password;

    return 1;
}

int jagmkdir(const char *path, mode_t mode)
{
	#ifdef _WINDOWS64_
		::mkdir( path );
	#else
		::mkdir( path, mode );
	#endif
	return 1;
}


ssize_t jagpread( int fd, char *buf, jagint length, jagint startpos )
{
	ssize_t len;
	#ifdef _WINDOWS64_
		lseek(fd, startpos, SEEK_SET );
    	len = ::read( fd, buf, length );
	#else
    	len = ::pread( fd, buf, length, startpos );
	#endif

	return len;
}

ssize_t jagpwrite( int fd, const char *buf, jagint length, jagint startpos )
{
	ssize_t len;
	#ifdef _WINDOWS64_
		lseek(fd, startpos, SEEK_SET );
    	len = ::write( fd, buf, length );
	#else
    	len = ::pwrite( fd, buf, length, startpos );
	#endif

	return len;
}

const char *strcasestrskipquote( const char *str, const char *token )
{
    int toklen = strlen( token );
    while ( *str != '\0' ) {
        if ( *str == '\'' || *str == '\"' ) {
            str = jumptoEndQuote( str );
            if ( ! str ) return NULL;
            ++str;
            if ( *str == '\0' ) break;
        }

        if ( 0 == strncasecmp( str, token, toklen) ) {
            return str;
        }
        ++str;
    }

    return NULL;
}

const char *strcasestrskipspacequote( const char *str, const char *token )
{
	while ( isspace(*str) ) ++str;
    int toklen = strlen( token );
    while ( *str != '\0' ) {
        if ( *str == '\'' || *str == '\"' ) {
            str = jumptoEndQuote( str );
            if ( ! str ) return NULL;
            ++str;
			while ( isspace(*str) ) ++str;
            if ( *str == '\0' ) break;
        }

		while ( isspace(*str) ) ++str;
        if ( 0 == strncasecmp( str, token, toklen) ) {
            return str;
        }
        ++str;
    }

    return NULL;
}

int jagsync( )
{
	#ifndef _WINDOWS64_
		sync();
		return 1;
	#else
		return 1;
	#endif
}

int jagfdatasync( int fd )
{
	#ifndef _WINDOWS64_
		return fdatasync( fd );
	#else
		return 1;
	#endif
}

int jagfsync( int fd )
{
	#ifndef _WINDOWS64_
		return fsync( fd );
	#else
		return 1;
	#endif
}

void trimLenEndColonWhite( char *str, int len )
{
	char *p = str + len -1;
	while ( p >= str && isspace(*p) ) { *p = '\0'; --p; }
	if ( p >= str && *p == ';' ) { *p = '\0'; }
}

void trimEndColonWhite( char *str )
{
	int len = strlen( str );
	trimLenEndColonWhite( str, len );
}


void randDataStringSort( Jstr *vec, int maxlen )
{
	struct timeval now;
	gettimeofday( &now, NULL );
	jagint microsec = now.tv_sec * (jagint)1000000 + now.tv_usec;
	srand( microsec );
	Jstr tmp;
	int i = maxlen-1, pivot = maxlen-1, radidx;
	while ( i > 0 ) {
		tmp = vec[pivot];
		radidx = rand() % i;
		vec[pivot] = vec[radidx];
		vec[radidx] = tmp;
		--i;
		--pivot;
	}
}

jagint getNearestBlockMultiple( jagint value )
{
	if ( value < JAG_BLOCK_SIZE ) value = JAG_BLOCK_SIZE;
	jagint a, b;
	double c, d;
	d = (double)value;
	c = log2 ( d );
	b = (jagint) c;
	++b;
	a = exp2( b );
	return a;
}

jagint getBuffReaderWriterMemorySize( jagint value ) 
{
	if ( value <= 0 ) value = 0;
	value = getNearestBlockMultiple( value ) / 2;
	if ( value > 1024 ) value = 1024;
	return value;
}

char *jagmalloc( jagint sz )
{
	char *p = NULL;
	while ( NULL == p ) {
		p = (char*)malloc(sz);
		if ( NULL == p ) {
			d("ERR0300 Error malloc %lld bytes memory, not enough memory. Please free up some memory. Retry in 10 seconds...\n", sz );
			sleep(10);
		}
	}

	return p;
}

int jagpthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg)
{
	int rc = 1;
	while ( rc != 0 ) {
		rc = pthread_create( thread, attr, (*start_routine), arg );
		if ( rc != 0 ) {
			d("ERR0400 Error pthread_create, errno=%d. Please clean up some unused processes. Retry in 10 seconds...\n", rc );
			sleep(10);
		}
	}

	return 0;
}

int jagpthread_join(pthread_t thread, void **retval)
{
	int rc = pthread_join( thread, retval );
	if ( rc != 0 ) {
		d("Error pthread_join , errno=%d\n", rc );
	}
	return rc;
}

jagint sendRawData( JAGSOCK sock, const char *buf, jagint msglen )
{
	jagint slen = _raysend( sock, buf, msglen );
    dn("u280019 sendRawData (%s) want: msglen=%ld   got slen=%ld", buf, msglen, slen );
	if ( slen < msglen ) { 
		return -1; 
	} else {
		return slen;
	}
}

// return -1: net error; >0 got data, slen is length data for buf
// should not return 0
jagint recvMessage( JAGSOCK sock, char *hdr, char *&buf )
{
	jagint slen, len;
	memset( hdr, 0, JAG_SOCK_TOTAL_HDR_LEN+1);

	len = _rayrecv( sock, hdr, JAG_SOCK_TOTAL_HDR_LEN); 
	if ( len < JAG_SOCK_TOTAL_HDR_LEN) { 
		// network error
		d("u2220221 in recvMessage() _rayrecv() got len=%d < JAG_SOCK_TOTAL_HDR_LEN=%d return -1\n", len, JAG_SOCK_TOTAL_HDR_LEN );
		return -1; 
	}

    dn("u450092 in recvMessage received hdr=[%s]", hdr );

	len = getXmitMsgLen( hdr );
	if ( len <= 0 ) { 
		// this should not happen
		d("u82038 thrd=%lu getXmitMsgLen hdr=[%s] len=%d return 0\n", THID, hdr, len );
		//abort();
		jagfree( buf );
		buf = (char*)jagmalloc(1);
		memset( buf, 0, 1 );
		return 0; 
		//return slen;
	} 

	d("u82038 thrd=%lu getXmitMsgLen hdr=[%s] len=%d\n", THID, hdr, len );

	jagfree( buf );
	buf = (char*)jagmalloc( len+1 );
	memset( buf, 0, len+1 );

	slen = _rayrecv( sock, buf, len );

	if ( slen < len ) { 
		d("recvMessage error sock=%d %d %d\n", sock, slen, len);
		free( buf ); buf = NULL; 
		return -1; 
	}

    //dumpmem(buf, slen);

	// check if mesg is compressed
    if ( hdr[JAG_SOCK_TOTAL_HDR_LEN-4] == 'Z' ) {
        dn("u30930012 uncompress data ...");
        Jstr compressed( buf, slen, slen );
        Jstr unCompressed;
        JagFastCompress::uncompress( compressed, unCompressed );
        jagfree( buf );
        buf = (char*)jagmalloc( unCompressed.size()+1 );
        memcpy( buf, unCompressed.c_str(), unCompressed.size() );
        buf[unCompressed.size()] = '\0';
		slen = unCompressed.size(); // updates to plain data
    }

	//d("u387440 thrd=%lu recvMessage() got hdr=[%s] slen=%lld buf=[%s]\n", THID, hdr, slen, buf );

	return slen;
}


// >0 OK; <=0: error
jagint recvMessageInBuf( JAGSOCK sock, char *hdr, char *&buf, char *sbuf, int sbuflen )
{
	dn("u008317 recvMessageInBuf thrd=%lu ...", THID);

	jagint slen, len;
	memset( hdr, 0, JAG_SOCK_TOTAL_HDR_LEN+1);

	slen = _rayrecv( sock, hdr, JAG_SOCK_TOTAL_HDR_LEN); 
	if ( slen < JAG_SOCK_TOTAL_HDR_LEN) { 
		d("u80041 in recvMessageInBuf() slen=%lld < JAG_SOCK_TOTAL_HDR_LEN return -1 thrd=%lu hdr=[%s]\n", slen, THID, hdr );
		return -1; 
	}

	len = getXmitMsgLen( hdr );
	if ( len <= 0 ) { 
		d("u890024 in recvMessageInBuf() len=%lld <=0 return 0\n", len );
		return 0; 
	}

	if ( len < sbuflen ) {
    	if ( buf ) { free( buf ); buf=NULL; }
		memset( sbuf, 0, sbuflen+1);
    	slen = _rayrecv( sock, sbuf, len );
    	if ( slen < len ) { 
			dn("u882220 slen=%lld < len=%lld return -10", slen, len );
			return -10;
		}
		sbuf[ slen ] = '\0' ;
	} else {
    	if ( buf ) { free( buf ); }
    	buf = (char*)jagmalloc( len+1 );
		memset( buf, 0,  len+1 );
    	slen = _rayrecv( sock, buf, len );
    	if ( slen < len ) { 
    		free( buf ); buf = NULL; 
			dn("u882225 slen=%lld < len=%lld return -10", slen, len );
    		return -20; 
    	}
		buf[ slen ] = '\0' ;
		sbuf[0] = '\0';
	}

	dn("u233308 recvMessageInBuf() return slen=%lld", slen);
	return slen;
}

jagint recvRawData( JAGSOCK sock, char *buf, jagint len )
{
	jagint slen = _rayrecv( sock, buf, len );
	if ( slen < len ) { 
		return -1; 
	}
	return slen;
}

#ifdef _WINDOWS64_
// windows code
jagint _raysend( JAGSOCK sock, const char *hdr, jagint N )
{
    register jagint bytes = 0;
    register jagint len;
	int errcnt = 0;
	
	if ( N <= 0 ) { return 0; }
	if ( socket_bad( sock ) ) { return -1; } 
	len = ::send( sock, hdr, N, 0 );
	if ( len < 0 && WSAGetLastError() == WSAEINTR ) {
    	len = ::send( sock, hdr, N, 0 );
	}
	if ( len < 0 && WSAGetLastError() == WSAEWOULDBLOCK ) {
		++errcnt;
		while ( 1 ) {
    		len = ::send( sock, hdr, N, 0 );
			if ( len < 0 && WSAGetLastError() == WSAEWOULDBLOCK ) {
				++errcnt;
			} else break;
			if ( errcnt >= 5 ) break;
		}
		if ( errcnt >= 5 ) {
			rayclose( sock );
			len = -1;
		}
	}

	if ( len < 0 ) {
		if ( WSAGetLastError() == WSAENETRESET || WSAGetLastError() == WSAECONNRESET ) {
			rayclose( sock );
		}
		return -1;
	}

    bytes += len;
    while ( bytes < N ) {
        len = ::send( sock, hdr+bytes, N-bytes, 0 );
		if ( len < 0 && WSAGetLastError() == WSAEINTR ) {
        	len = ::send( sock, hdr+bytes, N-bytes, 0 );
		}

		errcnt = 0;
		if ( len < 0 && WSAGetLastError() == WSAEWOULDBLOCK ) { // send timeout
			++errcnt;
			while ( 1 ) {
        		len = ::send( sock, hdr+bytes, N-bytes, 0 );
				if ( len < 0 && WSAGetLastError() == WSAEWOULDBLOCK ) {
					++errcnt;
				} else break;
				if ( errcnt >= 5 ) break;
			}
			if ( errcnt >= 5 ) {
				rayclose( sock );
				len = -1;
			}
		}	

        if ( len < 0 ) {
            if ( WSAGetLastError() == WSAENETRESET || WSAGetLastError() == WSAECONNRESET ) {
                rayclose( sock );
            }
            return bytes;
        }
        bytes += len;
    }

    return bytes;
}

// windows code
jagint _rayrecv( JAGSOCK sock, char *hdr, jagint N )
{
    register jagint bytes = 0;
    register jagint len;
	int errcnt = 0;

	if ( N <= 0 ) { 
		d("s4429 _rayrecv  N <= 0 return 0\n" );
		return 0; 
	}

	if ( socket_bad( sock ) ) { 
		return -1; 
	}

	len = ::recv( sock, hdr, N, 0 );
	if ( len < 0 && WSAGetLastError() == WSAEINTR ) {
    	len = ::recv( sock, hdr, N, 0 );
	}
	if ( len < 0 && WSAGetLastError() == WSAEWOULDBLOCK ) { 
		++errcnt;
		while ( 1 ) {
    		len = ::recv( sock, hdr, N, 0 );
			if ( len < 0 && WSAGetLastError() == WSAEWOULDBLOCK ) {
				++errcnt;
			} else break;
			if ( errcnt >= 5 ) break;
		}
		if ( errcnt >= 5 ) {
			rayclose( sock );
			len = -1;
		}
	}	

	if ( len == 0 ) {
		return 0;
	} else if ( len < 0 ) {
		if ( WSAGetLastError() == WSAENETRESET || WSAGetLastError() == WSAECONNRESET ) {
			rayclose( sock );
		}
		return -1;
	}

    bytes += len;
    while ( bytes < N ) {
        len = ::recv( sock, hdr+bytes, N-bytes, 0 );
		if ( len < 0 && WSAGetLastError() == WSAEINTR ) {
        	len = ::recv( sock, hdr+bytes, N-bytes, 0 );
		}
	
		errcnt = 0;
		if ( len < 0 && WSAGetLastError() == WSAEWOULDBLOCK ) { 
			++errcnt;
			while ( 1 ) {
        		len = ::recv( sock, hdr+bytes, N-bytes, 0 );
				if ( len < 0 && WSAGetLastError() == WSAEWOULDBLOCK ) {
					++errcnt;
				} else break;
				if ( errcnt >= 5 ) break;
			}
			if ( errcnt >= 5 ) {
				rayclose( sock );
				len = -1;
			}
		}	

        if ( len <= 0 ) {
            if ( WSAGetLastError() == WSAENETRESET || WSAGetLastError() == WSAECONNRESET ) {
                rayclose( sock );
            }
            return bytes;
        }
        bytes += len;
    }

    return bytes;
}
#else
// linux code
jagint _raysend( JAGSOCK sock, const char *buf, jagint N )
{
    jagint  bytes = 0;
    jagint  len;
	int     errcnt;

	if ( N <= 0 ) { 
		abort();
		return 0;
	}

	if ( socket_bad( sock ) ) {
		d("u29373734 in _raysend, bad sock=%d return -1\n", sock );
		return -1; 
	}

	len = ::send( sock, buf, N, MSG_NOSIGNAL );
	if ( len < 0 && errno == EINTR ) {
    	len = ::send( sock, buf, N, MSG_NOSIGNAL );
	}

    dn("u00229903 in _raysend first :send buf=[%s] got len=%ld", buf, len);
    // If server closes the socket or server has died, the ::send() will still be OK
    // without reporting error here (len<0). Only in recv() client can tell socket is closed.

    if ( len == N ) {
        return len;
    }

	errcnt = 0;
	if ( len < 0 && ( errno == EAGAIN || errno == EWOULDBLOCK ) ) { 
		++errcnt;
		while ( 1 ) {
    		len = ::send( sock, buf, N, MSG_NOSIGNAL );
			if ( len < 0 && ( errno == EAGAIN || errno == EWOULDBLOCK ) ) {
				++errcnt;
			} else break;
			if ( errcnt >= 5 ) break;
		}

		if ( errcnt >= 5 ) {
			rayclose( sock );
			len = -1;
		}
	}	

	if ( len < 0 ) {
		if ( errno == ENETRESET || errno == ECONNRESET ) {
			rayclose( sock );
		}
		dn("u20922 raysend hdr=[%s] N=%lld  len < 0 ret -1\n", buf, N );
		return -1;
	}

    //bytes += len;
    bytes = len;
    dn("u28010 len=%ld  update bytes=%ld", len, bytes );

    while ( bytes < N ) {
        len = ::send( sock, buf+bytes, N-bytes, MSG_NOSIGNAL );
		if ( len < 0 && errno == EINTR ) {
        	len = ::send( sock, buf+bytes, N-bytes, MSG_NOSIGNAL );
		}

		errcnt = 0;
		if ( len < 0 && ( errno == EAGAIN || errno == EWOULDBLOCK ) ) { // send timeout
			++errcnt;
			while ( 1 ) {
        		len = ::send( sock, buf+bytes, N-bytes, MSG_NOSIGNAL );
				if ( len < 0 && ( errno == EAGAIN || errno == EWOULDBLOCK ) ) {
					++errcnt;
				} else break;
				if ( errcnt >= 5 ) break;
			}

			if ( errcnt >= 5 ) {
				rayclose( sock );
				len = -1;
			}
		}	

        if ( len < 0 ) {
            if ( errno == ENETRESET || errno == ECONNRESET ) {
                rayclose( sock );
            }
			d("    u20922 raysend len < 0 ret bytes=%lld\n", bytes);
            return bytes;
        }

        bytes += len;
        dn("u2290301 updated bytes=%ld again len=%ld", bytes, len );
    }

	//d("u0223712 _raysend bytes=%lld N=%lld buf=[%s] thrd=%lu\n", bytes, N, buf, THID);
    dn("_raysend bytes=%ld/N=%ld", bytes, N );
    return bytes;
}

// linux code
jagint _rayrecv( JAGSOCK sock, char *buf, jagint N )
{
    register jagint bytes = 0;
    register jagint len;
	int errcnt = 0;

	if ( N <= 0 ) { 
		return 0;
	}

	if ( socket_bad( sock ) ) { 
		d("u833302 _rayrecv bad sock=%d return -1\n", sock );
		return -1; 
	}

	len = ::recv( sock, buf, N, 0 );
	if ( len < 0 && errno == EINTR ) {
    	len = ::recv( sock, buf, N, 0 );
	}

	if ( len < 0 && ( errno == EAGAIN || errno == EWOULDBLOCK ) ) { 
		++errcnt;
		while ( 1 ) {
    		len = ::recv( sock, buf, N, 0 );
			if ( len < 0 && ( errno == EAGAIN || errno == EWOULDBLOCK ) ) {
				++errcnt;
			} else break;
			if ( errcnt >= 5 ) break;
		}
		if ( errcnt >= 5 ) {
			rayclose( sock );
			len = -1;
		}
	}	

	if ( len == 0 ) {
		return 0;
	} else if ( len < 0 ) {
		if ( errno == ENETRESET || errno == ECONNRESET ) {
			rayclose( sock );
		}
		return -1;
	}

    bytes += len;
    while ( bytes < N ) {
        len = ::recv( sock, buf+bytes, N-bytes, 0 );
		if ( len < 0 && errno == EINTR ) {
        	len = ::recv( sock, buf+bytes, N-bytes, 0 );
		}
	
		errcnt = 0;
		if ( len < 0 && ( errno == EAGAIN || errno == EWOULDBLOCK ) ) { 
			++errcnt;
			while ( 1 ) {
        		len = ::recv( sock, buf+bytes, N-bytes, 0 );
				if ( len < 0 && ( errno == EAGAIN || errno == EWOULDBLOCK ) ) {
					++errcnt;
				} else break;
				if ( errcnt >= 5 ) break;
			}
			if ( errcnt >= 5 ) {
				rayclose( sock );
				len = -1;
			}
		}	

        if ( len <= 0 ) {
            if ( errno == ENETRESET || errno == ECONNRESET ) {
                rayclose( sock );
            }
            return bytes;
        }
        bytes += len;
    }

	// debug
	/***
	d("u8909384 thrd=%lu _rayrecved data:", THID);
	printf("[");
	for (int i=0; i<bytes; ++i) {
		printf("%c", buf[i]);
	}
	printf("]");
	printf("\n");
	***/

    return bytes;
}
#endif


int jagmalloc_trim( jagint n ) 
{
	#ifdef _WINDOWS64_
    return 1;
	#else
	int rc = malloc_trim( n );
    return rc; 
	#endif
}

FILE *loopOpen( const char *path, const char *mode )
{
	FILE *fp = NULL;
	while ( 1 ) {
		fp = ::fopen( path, mode );
		if ( fp ) {
			return fp;
		}
		jagsleep(10, JAG_SEC);
	}

	return fp;
}

FILE *jagfopen( const char *path, const char *mode )
{
	FILE *fp = ::fopen( path, mode );
	if ( !fp ) {
	} else {
	}
	return fp;
}

int jagfclose( FILE *fp )
{
	int rc = ::fclose( fp );
	if ( rc != 0 ) {
		// d("E0112 fclose error=[%s]\n", strerror(errno));
	} else {
		// d("s3370 fclose()\n");
	}
	return rc;
}

int jagopen( const char *path, int flags )
{
	#ifdef _WINDOWS64_
	flags = flags | O_BINARY;
	#endif
	int rc = ::open( path, flags );
	if ( rc < 0 ) {
		d("s3371 open(%s) error [%s]\n", path, strerror( errno ));
	} else {
		// d("s3372 open(%s)\n", path);
	}
	
	return rc;
}

int jagopen( const char *path, int flags, mode_t mode )
{
	int rc = ::open( path, flags, mode );
	if ( rc < 0 ) {
		d("s3371 open(%s) error [%s]\n", path, strerror( errno ));
	} else {
		// d("s3372 open(%s)\n", path);
	}
	
	return rc;
}


int jagclose( int fd )
{
	int rc = ::close( fd );
	if ( rc < 0 ) {
		// d("s3373 close(%d) error [%s]\n", fd, strerror( errno ));
	} else {
		// d("s3374 close(%d)\n", fd);
	}
	return rc;
}

int jagaccess( const char *path, int mode )
{
	int rc = ::access( path, mode );
	if ( rc < 0 ) {
		// d("s3375 access(%s) error [%s]\n", path, strerror( errno ));
	} else {
		// d("s3376 access(%s)\n", path);
	}
	return rc;
}

// 0: ok
int jagunlink( const char *path )
{
    if ( ::access(path, F_OK) != 0 ) {
        dn("u2006502 jagunlink(%s) not exist", path );
        return 0;
    }

	int rc = ::unlink( path );
	if ( rc < 0 ) {
		d("s3377 ::unlink(%s) error [%s]\n", path, strerror( errno ));
	} else {
		d("s3378 ::unlink(%s) OK\n", path);
	}

	return rc;
}

int jagrename( const char *path, const char *newpath )
{
#ifdef _WINDOWS64_
	jagunlink( newpath );
#endif
	int rc = ::rename( path, newpath );
	if ( rc < 0 ) {
		d("s3379 rename(%s   ->   %s) error [%s]\n", path, newpath, strerror( errno ));
	} else {
		//d("s3380 rename(%s   ->   %s)\n", path, newpath);
	}
	return rc;
}

#ifdef _WINDOWS64_
int jagftruncate( int fd, __int64 size )
{
	errno_t rc = _chsize( fd, size );
	if ( rc != 0 ) {
		d("s3381 _chsize(%d) error [%s]\n", fd, strerror( rc ));
	} else {
		// d("s3382 _chsize(%d)\n", fd);
	}
	return rc;
}
#else 
int jagftruncate( int fd, off_t length )
{
	int rc = ::ftruncate( fd, length );
	if ( rc < 0 ) {
		d("s3383 ftruncate(%d) error [%s]\n", fd, strerror( errno ));
	} else {
		// d("s3384 ftruncate(%d)\n", fd);
	}
	return rc;
}
#endif

void jagsleep( useconds_t time, int mode )
{
	#ifdef _WINDOWS64_
	if ( JAG_SEC == mode ) Sleep( time*1000 );
	else if ( JAG_MSEC == mode ) Sleep( time );
	else if ( JAG_USEC == mode ) {
		jagint t = time/1000;
		if ( t < 1 ) t = 1;
		Sleep( t );
	}
	#else
	if ( JAG_SEC == mode ) sleep( time );
	else if ( JAG_MSEC == mode ) usleep( time*1000 );
	else if ( JAG_USEC == mode ) usleep( time );
	#endif
}

const char *jagstrrstr(const char *haystack, const char *needle)
{
	if ( ! needle ) return haystack;

	const char *p;
	const char *r = NULL;
	if (!needle[0]) return (char*)haystack + strlen(haystack);

	while (1) {
		p = strstr(haystack, needle);
		if (!p) return r;
		r = p;
		haystack = p + 1;
	}
	return NULL;
}

#ifdef _WINDOWS64_
jagint jagsendfile( JAGSOCK sock, int fd, jagint size )
{
	char buf[1024*1024];
	lseek(fd, 0, SEEK_SET );
	jagint sz = 0;
	jagint n = 0;
	jagint snd;
	while (  ( n= ::read(fd, buf, 1024*1024 ) ) > 0 ) {
		snd = raysafewrite( sock, buf, n );
		if ( snd < n ) {
			lseek(fd, 0, SEEK_SET );
			return -1;
		}

		sz += snd;
	}
	return sz;
}
#else
#include <sys/sendfile.h>
jagint jagsendfile( JAGSOCK sock, int fd, jagint size )
{
    jagint BATCHSIZE = 100000000;
    jagint batches = size/BATCHSIZE;
    jagint remain = size % BATCHSIZE;
    jagint tot, rc;

    tot = 0;

    // batches can be 0
    for ( int i = 0; i < batches; ++i ) {
        rc = sendOneBatch( sock, fd, BATCHSIZE );
        if ( rc < 0 ) {
            break;
        }
        tot += rc;
    }

    if ( remain > 0 ) {
        rc = sendOneBatch( sock, fd, remain );
    }

    if ( rc > 0 ) {
        tot += rc;
    }

    return tot;

}
#endif

// -1: error; or actual sentBytes
jagint sendOneBatch( int sock, int fd, jagint size )
{
    dn("u873110 sendfile sock=%d fd=%d size=%ld", sock, fd, size );

	jagint remain = size;
	jagint onesendbytes;
    jagint  sentBytes = 0;
    bool    first = true;

	while ( remain > 0 ) {
		d("u022294 try sendfile remain=%lld ...\n", remain );
		onesendbytes = ::sendfile( sock, fd, NULL, remain );
		d("u022294 sendfile returns onesendbytes=%ld\n", onesendbytes );
		if ( onesendbytes < 0 ) {
            if ( first ) {
			    d("u022294 sendfile got %d return -1\n", onesendbytes );
			    return -1;
            } else {
			    d("u022295 sendfile got %d return %ld\n", sentBytes );
	            return sentBytes;
            }
		}

        first = false;
		d("u022295 sendfile onesendbytes=%lld\n", onesendbytes );
		remain = remain - onesendbytes;
        sentBytes += onesendbytes;
	}
	d("u022298 remain=%lld done\n", remain );
	return sentBytes;
}

Jstr psystem( const char *command )
{
	Jstr res;
	FILE *fp = popen(command, "r" );
	if ( ! fp ) {
		res = Jstr("Error execute ") + command; 
		return res;
	}

	char buf[1024];
	while ( NULL != fgets(buf, 1024, fp ) ) {
		res += buf;
	}

	pclose( fp );
	return res;
}

bool checkCmdTimeout( jagint startTime, jagint timeoutLimit )
{
	if ( timeoutLimit < 0 ) return false;
	struct timeval now;
	gettimeofday( &now, NULL ); 
	if ( now.tv_sec - startTime >= timeoutLimit ) return true;
	return false;
}

char *getNameValueFromStr( const char *content, const char *name )
{
	if ( ! content ) return NULL;
	if ( *content == '\0' ) return NULL;
	const char *p = strstr( content, name );
	if ( ! p ) return NULL;
	while ( *p != '\0' && *p != '=' ) ++p;
	if ( *p == '\0' ) return NULL;
	++p; // pass '='
	const char *start = p;
	while ( *p != '\0' && *p != '/' ) ++p;
	#ifndef _WINDOWS64_
	return strndup( start, p-start );
	#else
		int len =  p-start;
		char *p2 = jagmalloc( len+1);
		memcpy(p2, start, len );
		p2[len] = '\0';
		return p2;
	#endif
}

ssize_t jaggetline(char **lineptr, size_t *n, FILE *stream)
{
	#ifndef _WINDOWS64_
		return getline( lineptr, n, stream );
	#else
    ssize_t count = 0;
    char c;
	Jstr out;
    while ( 1) {
    	c = (char)getc(stream);
    	// if ( '\n' == c || EOF == c ) break;
    	if ( EOF == c ) break;
		out += c;
		++count;
		if ( '\n' == c ) { break;}
    }

	*n = count;
	if ( count < 1 && EOF == c ) {
		*lineptr = strdup("");
		return -1;
	}

	*lineptr = strdup( out.c_str() );
    return  count;
	#endif
}

Jstr expandEnvPath( const Jstr &path )
{
    if ( '$' != *(path.c_str()) ) return path;

    JagStrSplit sp( path, '/' );
    Jstr fs = sp[0];
	const char *p = fs.c_str() + 1;
	const char *penv = getenv(p);
	if ( penv ) {
		return Jstr(penv) + Jstr(path.s()+fs.size());
	} else {
		return Jstr(".") + Jstr(path.s()+fs.size());
	}
}

#ifdef _WINDOWS64_
struct tm *jag_localtime_r(const time_t *timep, struct tm *result)
{
	localtime_s (result, timep);
	return result;
}
char *jag_ctime_r(const time_t *timep, char *result)
{
	ctime_s(result, JAG_CTIME_LEN, timep);
	return result;
}

#else
struct tm *jag_localtime_r(const time_t *timep, struct tm *result)
{
	return localtime_r( timep, result );
}
char *jag_ctime_r(const time_t *timep, char *result)
{
	return ctime_r( timep, result );
}

#endif

int formatInsertSelectCmdHeader( const JagParseParam *parseParam, Jstr &str )
{
	if ( JAG_INSERTSELECT_OP == parseParam->opcode ) {
		str = "insert into " + parseParam->objectVec[0].dbName + "." + parseParam->objectVec[0].tableName;
		if ( parseParam->valueVec.size() > 0 ) {
			str += " (";
			for ( int i = 0; i < parseParam->valueVec.size(); ++i ) {
				if ( 0 == i ) {
					str += parseParam->valueVec[i].objName.colName;
				} else {
					str += ", " + parseParam->valueVec[i].objName.colName;
				}
			}
			str += " ) ";
		} else {
			str += " ";
		}
		return 1;
	}
	return 0;
}

bool isValidVar( const char *name )
{
	while ( *name ) {
		if ( ! isValidNameChar( *name ) ) {
			return false;
		}
		++name;
	}
	return true;
}

bool isValidCol( const char *name )
{
	while ( *name ) {
		if ( ! isValidColChar( *name ) ) {
			return false;
		}
		++name;
	}
	return true;
}

void stripEndSpace( char *qstr, char endch )
{
	if ( NULL == qstr || *qstr == '\0' ) return;
	char *start = qstr;
	while ( *qstr ) ++qstr;
	--qstr;

	while ( qstr != start ) {
		if ( isspace(*qstr) || *qstr == endch ) {
			*qstr = '\0';
		} else {
			break;
		}
		--qstr;
	}
}

#ifdef _WINDOWS64_
void getWinPass( char *pass )
{
    char c = 0;
    int i = 0;
    while ( c != 13 && i < 20 ) {
        c = _getch();
        pass[i++] = c;
    }
}
#endif

jagint _getFieldInt( const char * rowstr, char fieldToken )
{
	char newbuf[30];
	const char *p; 
	const char *start, *end;
	int len;
	char  feq[3];

	sprintf(feq, "%c=", fieldToken );  // fieldToken: E   "E=" in _row
	p = strstr( rowstr, feq );
	if ( ! p ) {
		return 0;
	}

	start = p +2;
	end = strchr( start, '|' );
	if ( ! end ) {
		end = strchr( start, ']' );
	}

	if ( ! end ) return 0;

	len = end - start;
	if ( len < 1 ) {
		return 0;
	}

	memcpy( newbuf, start, len );
	newbuf[len] = '\0';

	return jagatoll(newbuf);
}

void makeMapFromOpt( const char *options, JagHashMap<AbaxString, AbaxString> &omap )
{
	if ( NULL == options ) return;
	if ( '\0' == *options ) return;

	JagStrSplit sp( options, ' ', true );
	for ( int i = 0; i < sp.length(); ++i ) {
		JagStrSplit sp2( sp[i], '=' );
		if ( sp2.length() < 2 ) continue;
		omap.addKeyValue ( sp2[0], sp2[1] );
	}
}

Jstr makeStringFromOneVec( const JagVector<Jstr> &vec, int dquote )
{
	Jstr res;
	int len = vec.length();
	for ( int i =0; i < len; ++i ) {
		if ( dquote ) {
			res += Jstr("\"") + vec[i] + "\"";
		} else {
			res += vec[i];
		}

		if ( i < len-1 ) {
			res += ",";
		} 
	}
	return res;
}

Jstr makeStringFromTwoVec( const JagVector<Jstr> &xvec, const JagVector<Jstr> &yvec )
{
	Jstr res;
	int len = xvec.length();
	if ( len != yvec.length() ) return "";
	for ( int i =0; i < len; ++i ) {
		res += Jstr("[") + xvec[i] + "," + yvec[i] + "]";
		if ( i < len-1 ) { res += ","; } 
	}
	return res;
}

//int oneFileSender( JAGSOCK sock, const Jstr &inpath, const Jstr &hashDir, int &actualSent )
int oneFileSender( JAGSOCK sock, const Jstr &inpath,  const Jstr &dbName, const Jstr &tableName, const Jstr &hashDir, int &actualSent )
{
	d("s4009 oneFileSender sock=%d THRD=%lld inpath=[%s] ...\n", sock, THID, inpath.c_str() );
	Jstr    cmd, filename;
	char    *newbuf = NULL; 
	jagint  rlen = 0; 
    struct  stat sbuf; 
    int     fd = -1;
	jagint  filelen;

    actualSent = 0;

	d("s2838 oneFileSender inpath=[%s]\n", inpath.c_str() );
    const char *p = strrchr( inpath.c_str(), '/' );
    if ( p == NULL ) {
		p = inpath.c_str();
		filename = p;
    } else {
		filename = p+1;
	}

	rlen = 0;
	if ( inpath == "." || inpath.size() < 1 || filename.size() < 1 ) {
		rlen = -1;
	} else {
    	if ( (0 == stat(inpath.c_str(), &sbuf)) && ( sbuf.st_size > 0 ) ) {
            dn("u76520 jagopen inpath=[%s] ...", inpath.s() );
    		fd = jagopen( inpath.s(), O_RDONLY, S_IRWXU);
    		if ( fd < 0 ) { 
                // try other files in pdata/ndata
                rlen = -2; 
            }
    	} else { rlen = -3; }
	}

	int sendFakeData;
	if ( rlen < 0 ) {
		sendFakeData = 1;
		filelen = 0;
		d("u02813 sendFakeData = 1\n" );
		//filename = ".";  // means no data
		filename = JAG_FAKE_FILE;
	} else {
		sendFakeData = 0;
		filelen = sbuf.st_size;
		d("u02863 sendFakeData = 0\n" );
	}

	cmd = "_onefile|" + filename + "|" + longToStr( filelen ) + "|" + longToStr(THID);

    if ( dbName.size() > 0 ) {
        cmd += Jstr("|") + dbName;
    } else {
        cmd += Jstr("|_"); 
    }

    if ( tableName.size() > 0 ) {
        cmd += Jstr("|") + tableName;
    } else {
        cmd += Jstr("|_"); 
    }

    if ( hashDir.size() > 0 ) {
        cmd += Jstr("|") + hashDir;
    } else {
        cmd += Jstr("|_"); 
    }
    // "_onefile|fname|size|thrid|db|table|hashdir"

	char cmdbuf[JAG_SOCK_TOTAL_HDR_LEN+cmd.size()+1]; 
	memset( cmdbuf, ' ', JAG_SOCK_TOTAL_HDR_LEN+cmd.size()+1);

	char sqlhdr[8]; makeSQLHeader( sqlhdr );
	putXmitHdrAndData( cmdbuf, sqlhdr, cmd.c_str(), cmd.size(), "ATFC" );

	//d("s2309 sendRawData cmdbuf=[%s]", cmdbuf );
	rlen = sendRawData( sock, cmdbuf, JAG_SOCK_TOTAL_HDR_LEN+cmd.size() ); // 016ABCDmessage client query mode
    dn("u70017 sendRawData rlen=%ld  JAG_SOCK_TOTAL_HDR_LEN+cmd.size()=%ld", rlen, JAG_SOCK_TOTAL_HDR_LEN+cmd.size() );

	if ( rlen < JAG_SOCK_TOTAL_HDR_LEN+cmd.size() ) {
		dn("u0822 sendRawData error rlen=%ld", rlen );
		sendFakeData = 1;
	} else {
		dn("u00271 sendRawData rlen=%ld OK  cmdbuf=[%s]", rlen, cmdbuf );
	}

	if ( ! sendFakeData ) {
		dn("u229302 jagsendfile fd=%d sbuf.st_size=%lld ...", fd, sbuf.st_size );
		beginBulkSend( sock );
		rlen = jagsendfile( sock, fd, sbuf.st_size );
		endBulkSend( sock );
		dn("u229302 jagsendfile done rlen=%lld\n", rlen );
        if ( rlen > 0 ) {
            actualSent = 1;
        }
	} else {
        dn("u711108 sendFakeData true, filename was .");
    }

	if ( fd >= 0 ) jagclose( fd );
	if ( newbuf ) free( newbuf );
	return 1;
}

// < 0 error; 1 OK
//int oneFileReceiver( JAGSOCK sock, const Jstr &outpath, bool isDirPath )
int oneFileReceiver( JAGSOCK sock, const Jstr &filesPath, const Jstr &hashDir, bool isDirPath )
{	
	if ( 0 == sock ) {
		return 0;
	}

    // filesPath=/home/testuser/jaguar/pdata/test/media/files
    // hashDir=23/17
    Jstr outpath = filesPath + "/" + hashDir;

	//jagint fsize = 0, totlen = 0, recvlen = 0, memsize = 64*JAG_MEGABYTEUNIT;
	jagint  fsize = 0;
	Jstr    filename, senderID, recvpath;
	char    *newbuf = NULL; 
	char    hdr[JAG_SOCK_TOTAL_HDR_LEN+1];

	jagint rlen = 0;

	dn("u38830001 in oneFileReceiver()...");

	while ( 1 ) {
    	rlen = recvMessage( sock, hdr, newbuf ); 
		if ( 0 == rlen ) {
			continue;
		}

		if ( rlen < 0 ) {
			return 0;
		}

		if ( hdr[JAG_SOCK_TOTAL_HDR_LEN-3] == JAG_MSG_HB ) {
			continue;
		}

    	if ( 0 != strncmp( newbuf, "_onefile|", 9 ) ) {
    		dn("u0293 _onefile error newbuf=[%s]", newbuf );
    		if ( newbuf ) free(newbuf );
    		return -1;
    	} 

		break; // got _onefile command
	}

	JagStrSplit sp( newbuf, '|', true );
	if ( newbuf ) free( newbuf );

	if ( sp.length() <  4 ) return -100;

    // "_onefile|fname|size|thrid|db|table|hashdir(13/52)" : size=7

	filename = sp[1];
	// if ( filename == "." )
	if ( filename == JAG_FAKE_FILE ) {
        // fake file name
    	if ( newbuf ) free(newbuf );
		return -3;
	}

	fsize = jagatoll(sp[2].c_str());
	senderID = sp[3];

    if ( sp[6] != "_" ) {
        outpath = filesPath + "/" + sp[4];
    }

	if ( isDirPath ) recvpath = outpath + "/" + filename;
	else recvpath = filesPath;

    /***
	fd = jagopen( recvpath.c_str(), O_CREAT|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR );

	if ( fd < 0 ) {
		dn("u0394 jagopen(recvpath=[%s]) error ",  recvpath.c_str() );
		if ( newbuf ) free(newbuf );
	}

	char *mbuf =(char*)jagmalloc(memsize);
	while( 1 ) {
		if ( totlen >= fsize ) break;
		if ( fsize-totlen < memsize ) {
			recvlen = fsize-totlen;
		} else {
			recvlen = memsize;
		}
		rlen = recvRawData( sock, mbuf, recvlen );
		if ( rlen < recvlen ) {
			rlen = -1;
			break;
		}
		raysafewrite( fd, mbuf, rlen );
		totlen += rlen;
	}


	if ( mbuf ) free( mbuf );
	if ( newbuf ) free( newbuf );

    if ( fd < 0 ) {
        return -10;
    }

	jagfdatasync( fd ); 
    jagclose( fd );

	return 1;
    ****/

    jagint totlen = readSockAndSave( sock, recvpath, fsize );
    if ( totlen == fsize ) {
        return 1;
    } else {
        return 0;
    }

}

// return < 0: error;  else bytes received
jagint readSockAndSave( int sock, const Jstr &recvpath, jagint fsize )
{
    int fd;
	jagint totlen = 0, recvlen = 0, rlen, memsize = 64*JAG_MEGABYTEUNIT;

	fd = jagopen( recvpath.c_str(), O_CREAT|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR );

	if ( fd < 0 ) {
		dn("u0394 jagopen(recvpath=[%s]) error ",  recvpath.c_str() );
	}

	char *mbuf =(char*)jagmalloc(memsize);

	while( true ) {
		if ( totlen >= fsize ) break;
		if ( fsize-totlen < memsize ) {
			recvlen = fsize-totlen;
		} else {
			recvlen = memsize;
		}

		rlen = recvRawData( sock, mbuf, recvlen );
		if ( rlen < recvlen ) {
			rlen = -1;
			break;
		}

        if ( fd > 0 ) {
		    raysafewrite( fd, mbuf, rlen );
        }

		totlen += rlen;
	}

	if ( mbuf ) free( mbuf );

    if ( fd < 0 ) {
        return -10;
    }

	jagfsync( fd ); 
    jagclose( fd );

    dn("u820128 read and save file [%s] done  totlen=%ld", recvpath.s(), totlen );
	return totlen;
}

// method to send data directly to sock
jagint sendDirectToSock( JAGSOCK sock, const char *mesg, jagint len, bool nohdr )
{
	jagint rlen;
	if ( nohdr ) {
		rlen = sendRawData( sock, mesg, len ); // 016ABCDmessage client query mode
	} else {
		char *buf =(char*)jagmalloc(len+JAG_SOCK_TOTAL_HDR_LEN+1);
		char sqlhdr[8];  makeSQLHeader( sqlhdr );
		putXmitHdrAndData( buf, sqlhdr, mesg, len, "ATFC" );
		rlen = sendRawData( sock, buf, len+JAG_SOCK_TOTAL_HDR_LEN ); // 016ABCDmessage client query mode
		if ( buf ) free( buf );
	}
	return rlen;
}

// method to recv data directly from sock
jagint recvDirectFromSock( JAGSOCK sock, char *&buf, char *hdr )
{
	dn("u61322501 enter recvDirectFromSock() sock=%d  thrd=%lu...", sock, THID);
	jagint rlen = 0;

	int hbcnt = 0;
	while ( 1 ) {
		rlen = recvMessage( sock, hdr, buf );
		if ( rlen > 0 && hdr[JAG_SOCK_TOTAL_HDR_LEN-3] == JAG_MSG_HB ) {
			dn("u60333201 in recvDirectFromSock() got %d HB, skip", hbcnt );
			++hbcnt;
			if ( hbcnt > 10 ) {
				rlen = 0;
				break;
			}

			continue;
		}

		//dn("u0393380 rlen=%d not HB hdr=[%s] buf=[%s] break done", rlen, hdr, buf );
        // dumpmem( buf, rlen );

		break;
	}

	dn("u83333012 in recvDirectFromSock() while(1) is done, rlen=%d sock=%d  thrd=%lu ", rlen, sock, THID );


	return rlen;
}

jagint sendDirectToSockWithHdr( JAGSOCK sock, const Jstr &shdr, const Jstr &mesg )
{
	jagint len = mesg.size();
	const char *p = mesg.c_str();
	const char *hdr = shdr.c_str();
	jagint rlen;
	Jstr comp;
	if ( hdr[JAG_SOCK_TOTAL_HDR_LEN-4] == 'Z' ) {
		JagFastCompress::compress( mesg.c_str(), len, comp );
		p = comp.c_str();
		len = comp.size();
    }

	rlen = sendRawData( sock, hdr, JAG_SOCK_TOTAL_HDR_LEN );
	rlen = sendRawData( sock, p, len );
	return rlen;
}

int isValidSciNotation(const char *str )
{
	int rc = 1;
	while ( *str != '\0' ) {
		if ( isdigit(*str) || '.' == *str || 'e' == *str || 'E' == *str 
		     || '-' == *str || '+' == *str ) {
		 	++str;
			if ( 'e' == *str || 'E' == *str ) { rc = 2; }
		 } else {
		 	return 0;
		 }
	}
	return rc;
}

Jstr fileHashDir( const JagFixString &fstr )
{
	jagint hcode = fstr.hashCode();
	char buf[32];
	sprintf( buf, "%lld/%lld", hcode % 1000, (hcode/1000) % 1000 ); 
	return buf;
}

char lastChar( const JagFixString &str )
{
	return *( str.c_str() + str.size()-1);
}

void jagfwrite( const char *str, jagint len, FILE *outf )
{
	for ( int i = 0; i < len; ++i ) {
		if ( str[i] != '\0' ) {
			fputc( str[i], outf );
		}
	}
}

void jagfwritefloat( const char *str, jagint len, FILE *outf )
{
	if ( NULL == str || '\0' == *str || len <1 ) return;
	bool leadzero = false;
	int start = 0;
	if ( str[0] == '+' || str[0] == '-' ) {
		start = 1;
		fputc( str[0], outf );
	}

	if ( str[start] == '0' ) {
        leadzero = true;
    }

	bool allzeros = true;
	char c;

	for ( int i = start; i < len; ++i ) {
		c = str[i];
		if ( c  == '\0' ) continue;
		if ( c != '0' && c != '.' ) allzeros = false;
		if ( c != '0' || ( c == '0' && i+1 < len && str[i+1] == '.' ) ) leadzero = false;

		if ( ! leadzero && (isdigit(c) || '.' == c ) ) {
			fputc( c, outf );
		}
	}

	if ( allzeros ) fputc( '0', outf );
}

void charFromStr( char *dest, const Jstr &src )
{
	strcpy( dest, src.c_str() );
}

bool streq( const char *s1, const char *s2 )
{
	if ( 0 == strcmp( s1, s2 ) ) {
		return true;
	}

	return false;
}

bool isValidNameChar( char c )
{
    if ( c == '+' ) return false;
    if ( c == '-' ) return false;
    if ( c == '*' ) return false;
    if ( c == ' ' ) return false;
    if ( c == '\t' ) return false;
    if ( c == '/' ) return false;
    if ( c == '\\' ) return false;
    if ( c == '\n' ) return false;
    if ( c == '\0' ) return false;
    if ( c == '=' ) return false;
    if ( c == '!' ) return false;
    if ( c == '(' ) return false;
    if ( c == ')' ) return false;
    if ( c == '|' ) return false;
    if ( c == '[' ) return false;
    if ( c == ']' ) return false;
    if ( c == '{' ) return false;
    if ( c == '}' ) return false;
    if ( c == ';' ) return false;
    if ( c == ',' ) return false;
    if ( c == ':' ) return false;
    if ( c == '<' ) return false;
    if ( c == '>' ) return false;
    if ( c == '`' ) return false;
    if ( c == '~' ) return false;
    if ( c == '&' ) return false;

	// supports UTF chars
    return true;
}

bool isValidColChar( char c )
{
    if ( c == ':' ) return true;
	return isValidNameChar( c );
}

unsigned long jagatoul(const char *nptr)
{
	if ( NULL == nptr || '\0' == *nptr ) return 0;
	return strtoul( nptr, NULL, 10 );
}

long jagatol(const char *nptr)
{
	if ( NULL == nptr || '\0' == *nptr ) return 0;
	return atol( nptr );
}

long long jagatoll(const char *nptr)
{
	if ( NULL == nptr || '\0' == *nptr ) return 0;
	return atoll( nptr );
}

long long jagatoll(const Jstr &str )
{
	return jagatoll( str.c_str() );
}

long double jagstrtold(const char *nptr, char **endptr)
{
	if ( NULL == nptr || '\0' == *nptr ) return 0.0;
	return strtold( nptr, endptr );
}

long double jagatold(const char *nptr, char **endptr)
{
	if ( NULL == nptr || '\0' == *nptr ) return 0.0;
	return strtold( nptr, endptr );
}

double jagatof(const char *nptr)
{
	if ( NULL == nptr || '\0' == *nptr ) return 0.0;
	return atof( nptr);
}

double jagatof(const Jstr &str )
{
	return jagatof( str.c_str() );
}


int jagatoi(const char *nptr)
{
	if ( NULL == nptr || '\0' == *nptr ) return 0;
	return atoi( nptr);
}

int jagatoi(char *nptr, int len)
{
	if ( NULL == nptr || '\0' == *nptr ) return 0;
	if ( len < 0 ) return 0;
	char save = nptr[len];
	nptr[len] = 0;
	int n = atoi( nptr);
	nptr[len] = save;
	return n;
}

// float:   120.12300000  --> 120.123
void stripTailZeros( char *buf, int len )
{
	//printf("u882038 stripTailZeros buf=[%s] len=%d begin\n", buf, len);

    if ( NULL == buf || *buf == '\0' ) return;
    if ( len < 2 ) return;

	//if ( ! strchr(buf, '.') ) { return; }

    char *p = buf+len-1;
    while ( p >= buf+1 ) {
        if ( *p == '0' || *p == '.' || !isdigit(*p) ) {
            *p = '\0';
            --p;
			if ( *p == '.' ) { break; }  // added
            continue;
        } else {
            break;
        }
    }

    if ( buf[1] == '\0' ) {
        if ( buf[0] == '.' || buf[0] == '+' || buf[0] == '-' ) {
            buf[0] = '0';
        }
    } else {
        if ( *p == '.' ) {
            *p = '\0';
        }
	}

	//printf("u882038 stripTailZeros buf=[%s] len=%d done\n", buf, len);
}


bool jagisspace( char c)
{
	if ( ' ' == c  || '\t' == c  || '\r' == c  ) return true;
	return false;
}

Jstr trimEndZeros( const Jstr& str )
{
	if ( ! strchr( str.c_str(), '.') ) return str;
	Jstr res;

	// skip leading zeros
	int len = str.size();
	if ( len < 1 ) return "";
	int start=0;
	bool leadzero = false;
	if ( str[0] == '+' || str[0] == '-' ) {
		res += str[0];
		start = 1;
	}

	if ( str[start] == '0' && str[start+1] != '\0' ) leadzero = true;
	for ( int i = start; i < len; ++i ) {
		if ( str[i] != '0' || ( str[i] == '0' && str[i+1] == '.' ) ) leadzero = false;
		if ( ! leadzero ) {
			res += str[i];
		}
	}
	//d("u1200 trimEndZeros res=[%s]\n", res.c_str() );

	//  trim tail zeros
	len = res.size();
	if ( len < 1 ) return "";
	char *buf = strndup( res.c_str(), len );
	//d("u12001 trimEndZeros buf=[%s]\n", buf );
	char *p = buf+len-1;
	while ( p >= buf+1 ) {
		if ( *(p-1) != '.' && *p == '0' ) {
			*p = '\0';
		} else {
			break;
		}
		--p;
	}
	if ( buf[0] == '.' && buf[1] == '\0' ) buf[0] = '0';
	//d("u2231 trimEndZeros buf=[%s]\n", buf );
	res = buf;
	free( buf );
	return res;
}

void dumpmem( const char *buf, int len, bool newline )
{
	printf("{");
	for ( int i=0; i < len; ++i ) {
		if ( buf[i] == '\0' ) {
			printf("@");
		} else {
			printf("%c", buf[i] );
		}
	}
	printf("}");

    if ( newline ) {
	    printf("\n");
    }
	fflush(stdout);

    dumpmemi( buf, len, newline );
}

void dumpmemi( const char *buf, int len, bool newline )
{
	printf("{");
	for ( int i=0; i < len; ++i ) {
		if ( buf[i] == '\0' ) {
			printf("@ ");
		} else {
			printf("%d ", c2uc(buf[i]) );
		}
	}
	printf("}");

    if ( newline ) {
	    printf("\n");
    }
	fflush(stdout);
}

void prepareKMP(const char *pat, int M, int *lps)
{
    int len = 0;
    lps[0] = 0; 
    int i = 1;
    while (i < M) {
        if (pat[i] == pat[len]) {
            len++;
            lps[i] = len;
            i++;
        } else {
            if (len != 0) {
                len = lps[len-1];
            } else {
                lps[i] = 0;
                i++;
            }
        }
    }
}

const char *KMPstrstr(const char *txt, const char *pat)
{
	if ( ! txt || ! pat ) return NULL;
	if ( 0 == *txt ) return NULL;
	if ( 0 == *pat  ) return txt;
    int N = strlen(txt);
    int M = strlen(pat);
	if ( M > N ) return NULL;
    int lps[M];
    prepareKMP(pat, M, lps);
    int i = 0, j=0;
    while (i < N) {
        if (pat[j] == txt[i]) {
            j++; i++;
        }
 
        if (j == M) {
			return &(txt[i-j]);
        }
 
        if (i < N  &&  pat[j] != txt[i] ) {
            if (j != 0) j = lps[j-1];
            else i = i+1;
        }
    }

	return NULL;
}

Jstr replaceChar( const Jstr& str, char oldc, char newc )
{
	Jstr res;
	for ( int i=0; i < str.size(); ++i ) {
		if ( str[i] == oldc ) {
			res += newc;
		} else {
			res += str[i];
		}
	}
	return res;
}

char *secondTokenStart( const char *str, char sep )
{
	if ( NULL == str || *str == 0 ) return NULL;
	char *p = (char*) str;
	while ( *p == sep ) ++p;  // "  abc  mdef"  p is at a
	while ( *p != sep && *p != '\0' ) ++p; // p is at pos after c
	if ( *p == '\0' ) return NULL;
	while ( *p == sep ) ++p;  // p is at m
	return p;
}

char *thirdTokenStart( const char *str, char sep )
{
	if ( NULL == str || *str == 0 ) return NULL;
	char *p = (char*) str;
	while ( *p == sep ) ++p;  // "  abc  mdef nfe"  p is at a
	while ( *p != sep && *p != '\0' ) ++p; // p is at pos after c
	if ( *p == '\0' ) return NULL;
	while ( *p == sep ) ++p;  // p is at m

	while ( *p != sep && *p != '\0' ) ++p; // p is at pos after f
	if ( *p == '\0' ) return NULL;
	while ( *p == sep ) ++p;  // p is at n

	return p;
}

char *secondTokenStartEnd( const char *str, char *&pend, char sep )
{
	if ( NULL == str || *str == 0 ) return NULL;
	char *p = (char*) str;
	while ( *p == sep ) ++p;  // "  abc  mdef"  p is at a
	while ( *p != sep && *p != '\0' ) ++p; // p is at pos after c
	if ( *p == '\0' ) return NULL;
	while ( *p == sep ) ++p;  // p is at m
	pend = p; 
	while ( *pend != sep && *pend != '\0' ) ++pend;
	return p;
}


//  xm, xh, xd, xw
jagint convertToSecond( const char *p )
{
	jagint num, n = jagatol(p);
    if ( strchr(p, 'm') || strchr(p, 'M')  ) {
          num = n * 60; // tosecond
    } else if (  strchr(p, 'h') || strchr(p, 'H') ) {
          num = n * 360; // tosecond
    } else if (  strchr(p, 'd') || strchr(p, 'D') ) {
          num = n * 86400; // tosecond
    } else if (  strchr(p, 'w') || strchr(p, 'W') ) {
          num = n * 86400 *7; // tosecond
    } else {
          num = -1;
    }
	return num;
}

//  xm, xh, xd, xs, xw
jagint convertToMicroSecond( const char *p )
{
	jagint num, n = jagatol(p);
    if ( strchr(p, 'm') || strchr(p, 'M')  ) {
          num = n * 60000000; 
    } else if (  strchr(p, 'h') || strchr(p, 'H') ) {
          num = n * 360000000; 
    } else if (  strchr(p, 'd') || strchr(p, 'D') ) {
          num = n * 86400000000; 
    } else if (  strchr(p, 's') || strchr(p, 'S') ) {
          num = n * 1000000; 
    } else if (  strchr(p, 'w') || strchr(p, 'W') ) {
          num = n * 86400000000 *7; 
    } else {
          num = -1;
    }
	return num;
}

bool likeMatch( const Jstr& str, const Jstr& like )
{
	if ( like.size() < 1 ) {
		return false;
	}

	char firstc = *(like.c_str());
	char endc = *(like.c_str()+ like.size()-1);
	if ( firstc == '%' && endc == '%' ) {
		char *p =  (char*)(like.c_str() + like.size() - 1);
		*p = '\0';
		if ( strstr( str.c_str(), like.c_str()+1 ) ) {
			*p = endc;
			return true;
		}
		*p = endc;
	} else if ( firstc == '%' ) {
		if ( lastStrEqual(str.c_str(), like.c_str()+1, str.size(), like.size()-1 ) ) {
			return true;
		} 
	} else if ( endc == '%' ) {
		if ( 0 == strncmp( str.c_str(), like.c_str(), like.size()-1 ) ) {
			return true;
		}
	} else {
		if ( str == like ) return true;
	}

	return false;
}


#define MINVAL3(a, b, c) ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))
int levenshtein(const char *s1, const char *s2) 
{
    unsigned int s1len, s2len, x, y, lastdiag, olddiag;
    s1len = strlen(s1);
    s2len = strlen(s2);
    unsigned int column[s1len+1];
    for (y = 1; y <= s1len; y++) column[y] = y;
    for (x = 1; x <= s2len; x++) {
        column[0] = x;
        for (y = 1, lastdiag = x-1; y <= s1len; y++) {
            olddiag = column[y];
            column[y] = MINVAL3(column[y] + 1, column[y-1] + 1, lastdiag + (s1[y-1] == s2[x-1] ? 0 : 1));
            lastdiag = olddiag;
        }
    }
    return(column[s1len]);
}

// check first n chars in s for c
char *strnchr(const char *s, int c, int n)
{
	if ( ! s || *s=='\0' ) return 0;
	char *p = (char*)s;
	while ( *p != '\0' ) {
		if ( p-s >= n ) break;
		if ( *p == c ) return p;
		++p;
	}
	return NULL;
}


bool isNumeric( const char *str) 
{
	while ( *str != '\0' ) {
        if ( ! isdigit(*str) && *str != '.' ) {
            return false;
        } else {
			++str;
		}
    }
    return true;
}

// rotate counter-clock-wise of point oldx,oldy around point x0,y0
// alpha is radians
void rotateat( double oldx, double oldy, double alpha, double x0, double y0, double &x, double &y )
{
	x = x0 + (oldx-x0)*cos(alpha) - (oldy-y0)*sin(alpha);
	y = y0 + (oldy-y0)*cos(alpha) + (oldx-x0)*sin(alpha);
}

void rotatenx( double oldnx, double alpha, double &nx )
{
	if ( fabs(oldnx) > 1.0 || fabs( fabs(oldnx) - 1.0 ) < 0.00000001 ) {
		nx = 1.0;
		return;
	}
	double oldny = sqrt(1.0 - oldnx*oldnx);
	nx = oldnx*cos(alpha) - oldny*sin(alpha);
}

void affine2d( double x1, double y1, double a, double b, double d, double e,
                double dx, double dy, double &x, double &y )
{
	x = a*x1 + b*y1 + dx;
	y = d*x1 + e*y1 + dy;
}

void affine3d( double x1, double y1, double z1, double a, double b, double c, double d, double e,
                double f, double g, double h, double i, double dx, double dy, double dz,
                double &x, double &y, double &z )
{
	x = a*x1 + b*y1 + c*z1 + dx;
	y = d*x1 + e*y1 + f*z1 + dy;
	z = g*x1 + h*y1 + i*z1 + dz;
}

void ellipseBoundBox( double x0, double y0, double a, double b, double nx, 
					  double &xmin, double &xmax, double &ymin, double &ymax )
{
	if ( nx > 1.0 ) nx = 1.0;
	if ( nx < -1.0 ) nx = -1.0;

	if ( jagEQ(nx, 0.0) ) {
		xmin = x0-a;
		xmax = x0+a;
		ymin = y0-b;
		ymax = y0+b;
		return;
	}

	if ( jagEQ(fabs(nx), 1.0) ) {
		xmin = x0-b;
		xmax = x0+b;
		ymin = y0-a;
		ymax = y0+a;
		return;
	}

	double nx2 = nx * nx;
	double ny2 = 1.0-nx2;
	double sqrt1 = sqrt( a*a*ny2 + b*b*nx2 );
	double sqrt2 = sqrt( a*a*nx2 + b*b*ny2 );

	xmin = x0 - sqrt1;
	xmax = x0 + sqrt1;
	ymin = y0 - sqrt2;
	ymax = y0 + sqrt2;
}


void ellipseMinMax(int op, double x0, double y0, double a, double b, double nx, 
					  double &xmin, double &xmax, double &ymin, double &ymax )
{
	// nx is sin(theta) -- theta is angle of rotating shape clock-wise
	// ny=sqrt(1.0-nx*nx) and == cos(theta)=ny
	// newx = xcos(theta) + ysin(theta)
	// newy = ycos(theta) - xsin(theta)
	// xmin = - sqrt( a^2 * ny^2 + b^2 * nx^2 )
	// xmax = + sqrt( a^2 * ny^2 + b^2 * nx^2 )
	// ymin = - sqrt( a^2 * nx^2 + b^2 * ny^2 )
	// ymax = + sqrt( a^2 * nx^2 + b^2 * ny^2 )
	xmin = ymin = xmax = ymax = 0.0;
	if ( jagEQ(a, 0.0) && jagEQ( b, 0.0) ) {
		return;
	}

	if ( nx > 1.0 ) nx = 1.0;
	if ( nx < -1.0 ) nx = -1.0;

	if ( jagEQ(nx, 0.0) ) {
		if ( JAG_FUNC_XMINPOINT == op ) {
			xmin = x0 - a;
			ymin = y0;
		} else if ( JAG_FUNC_YMINPOINT == op ) {
			xmin = x0;
			ymin = y0 - b;
		} else if ( JAG_FUNC_XMAXPOINT == op ) {
			xmax = x0 + a;
			ymax = y0;
		} else if ( JAG_FUNC_YMAXPOINT == op ) {
			xmax = x0;
			ymax = y0 + b;
		}
		return;
	}

	double nx2 = nx * nx;
	double ny2 = 1.0-nx2;
	double ny = sqrt(ny2);

	double A, B;
	if ( JAG_FUNC_XMINPOINT == op ) {
		double sqrt1 = sqrt( a*a*ny2 + b*b*nx2 );
		A = b*b*nx2 + a*a * ny2;
		B = 2.0*(b*b - a*a)*sqrt1*nx*ny;
		xmin = x0 - sqrt1;
		ymin = y0 - 0.5*B/A;
	} else if ( JAG_FUNC_XMAXPOINT == op ) {
		double sqrt1 = sqrt( a*a*ny2 + b*b*nx2 );
		A = b*b*nx2 + a*a * ny2;
		B = 2.0*(a*a - b*b)*sqrt1*nx*ny;
		xmax = x0 + sqrt1;
		ymax = y0 - 0.5*B/A;
	} else if ( JAG_FUNC_YMINPOINT == op ) {
		double sqrt2 = sqrt( a*a*nx2 + b*b*ny2 );
		A = b*b*ny2 + a*a * nx2;
		B = 2.0*(b*b-a*a)*sqrt2*nx*ny;
		ymin = y0 - sqrt2;
		xmin = x0 - 0.5*B/A;
	} else if ( JAG_FUNC_YMAXPOINT == op ) {
		double sqrt2 = sqrt( a*a*nx2 + b*b*ny2 );
		A = b*b*ny2 + a*a * nx2;
		B = 2.0*(a*a - b*b)*sqrt2*nx*ny;
		ymax = y0 + sqrt2;
		xmax = x0 - 0.5*B/A;
	}
}

double dotProduct( double x1, double y1, double x2, double y2 )
{
    return ( x1*x2 + y1*y2 );
}
double dotProduct( double x1, double y1, double z1, double x2, double y2, double z2 )
{
    return ( x1*x2 + y1*y2 + z1*z2 );
}

void crossProduct( double x1, double y1, double z1, double x2, double y2, double z2,
                           double &x, double &y, double &z )
{
    x = y1*z2 - z1*y2;
    y = z1*x2 - x1*z2;
    z = x1*y2 - y1*x2;
}


bool jagLE (double f1, double f2 )
{
    if ( f1 < f2 ) return true;
    if ( fabs(f1-f2) < JAG_ZERO ) return true;
    return false;
}

bool jagGE (double f1, double f2 )
{
    if ( f1 > f2 ) return true;
    if ( fabs(f1-f2) < JAG_ZERO ) return true;
    return false;
}

bool jagEQ (double f1, double f2 )
{
    if ( fabs(f1-f2) < JAG_ZERO ) return true;
    return false;
}

void putXmitHdr( char *outbuf, const char *sqlhdr, int msglen, const char *code )
{
	int blanksz = JAG_SOCK_SQL_HDR_LEN-strlen(sqlhdr);
	dn("u33301 putXmitHdr() blanksz=%d", blanksz );
    blanksz && memset(outbuf, '#', blanksz );
    sprintf( outbuf+blanksz, "%s", sqlhdr );
    sprintf( outbuf+JAG_SOCK_SQL_HDR_LEN, "%0*d%s", JAG_SOCK_MSG_HDR_LEN-4, msglen, code );
	//dn("u222208 putXmitHdr() final buf=[%s]", buf);
}

void putXmitHdrAndData( char *outbuf, const char *sqlhdr, const char *msg, int msglen, const char *code )
{
	int blanksz = JAG_SOCK_SQL_HDR_LEN-strlen(sqlhdr);
	dn("u22272 in putXmitHdrAndData() blanksz=%d", blanksz);
    blanksz && memset(outbuf, '#', blanksz );
    sprintf( outbuf+blanksz, "%s", sqlhdr );
    sprintf( outbuf+JAG_SOCK_SQL_HDR_LEN, "%0*d%s", JAG_SOCK_MSG_HDR_LEN-4, msglen, code );
    memcpy( outbuf+JAG_SOCK_TOTAL_HDR_LEN, msg, msglen );
    outbuf[JAG_SOCK_TOTAL_HDR_LEN+msglen] = '\0';
	//dn("u242208 putXmitHdrAndData() final buf=[%s]", buf);
}

void getXmitSQLHdr( const char *buf, char *sqlhdr )
{
	//dn("u111029 getXmitSQLHdr()  buf=[%s]", buf );
	const char *p = buf;
	int blanklen;
	while ( *p == '#' ) ++p;
	blanklen = p-buf;
	for ( int i=0; i < JAG_SOCK_SQL_HDR_LEN-blanklen; ++i ) {
		*sqlhdr = *p;
		++p;
		++sqlhdr;
	}
	*sqlhdr = '\0';
	dn("u2228281 getXmitSQLHdr() sqlhdr=[%s]", sqlhdr);
}

/**
void getXmitCode( const char *buf, char *code )
{
	memcpy( code, buf+JAG_SOCK_TOTAL_HDR_LEN-4, 4 );
}
**/

long long getXmitMsgLen( char *buf )
{
	char c = buf[JAG_SOCK_TOTAL_HDR_LEN-4];
	buf[JAG_SOCK_TOTAL_HDR_LEN-4]='\0';
	long long n = atoll( buf+JAG_SOCK_SQL_HDR_LEN );
	buf[JAG_SOCK_TOTAL_HDR_LEN-4]=c;
	return n;
}

void makeSQLHeader( char *sqlhdr )
{
	for ( int i=0; i < JAG_SOCK_SQL_HDR_LEN; ++i ) {
		sqlhdr[i] = '9';
	}
	sqlhdr[JAG_SOCK_SQL_HDR_LEN] = '\0';
}

/***
void makeTotalHeader( char *tothdr, jagint payloadLen, const char *code4 )
{
    char sqlhdr[8]; makeSQLHeader( sqlhdr );
    int blanksz = JAG_SOCK_SQL_HDR_LEN-strlen(sqlhdr);
	dn("u233088 in makeTotalHeader blanksz=%d", blanksz);
    memset(tothdr, '#', blanksz );
    sprintf( tothdr+blanksz, "%s", sqlhdr );
    sprintf( tothdr+JAG_SOCK_SQL_HDR_LEN, "%0*lld%s", JAG_SOCK_MSG_HDR_LEN-4, payloadLen, code4 );
}

jagint sendShortMessageToSock( JAGSOCK sock, const char *msg, jagint msglen, const char *code4 )
{
	char buf[JAG_SOCK_TOTAL_HDR_LEN+msglen+1];
	makeTotalHeader( buf, msglen, code4 );
	memcpy( buf + JAG_SOCK_TOTAL_HDR_LEN, msg, msglen );
	return sendRawData( sock, buf, JAG_SOCK_TOTAL_HDR_LEN + msglen );
}
***/

Jstr makeGeoJson( const JagStrSplit &sp, const char *str )
{
    dn("u8903005 makeGeoJson str=[%s]", str );

	if ( sp[3] == JAG_C_COL_TYPE_LINESTRING ) {
		return makeJsonLineString("LineString", sp, str );
	} else if ( sp[3] == JAG_C_COL_TYPE_VECTOR ) {
		return makeJsonVector( "Vector", str );
	} else if ( sp[3] == JAG_C_COL_TYPE_LINESTRING3D ) {
		return makeJsonLineString3D( "LineString", sp, str );
	} else if ( sp[3] == JAG_C_COL_TYPE_MULTIPOINT ) {
		return makeJsonLineString("MultiPoint", sp, str );
	} else if ( sp[3] == JAG_C_COL_TYPE_MULTIPOINT3D ) {
		return makeJsonLineString3D("MultiPoint", sp, str );
	} else if ( sp[3] == JAG_C_COL_TYPE_POLYGON ) {
		return makeJsonPolygon( "Polygon", sp, str, false );
	} else if ( sp[3] == JAG_C_COL_TYPE_POLYGON3D ) {
		return makeJsonPolygon( "Polygon", sp, str, true );
	} else if ( sp[3] == JAG_C_COL_TYPE_MULTILINESTRING ) {
		return makeJsonPolygon("MultiLineString", sp, str, false );
	} else if ( sp[3] == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
		return makeJsonPolygon( "MultiLineString", sp, str, true );
	} else if ( sp[3] == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		return makeJsonMultiPolygon( "MultiPolygon", sp, str, false );
	} else if ( sp[3] == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
		return makeJsonMultiPolygon( "MultiPolygon", sp, str, true );
	} else {
		return makeJsonDefault( sp, str) ;
	}
}

/******************************************************************
** GeoJSON supports the following geometry types: 
** Point, LineString, Polygon, MultiPoint, MultiLineString, and MultiPolygon. 
** Geometric objects with additional properties are Feature objects. 
** Sets of features are contained by FeatureCollection objects.
** https://tools.ietf.org/html/rfc7946
*******************************************************************/

// sp: OJAG=0=test.lstr.ls=LS guarantee 3 '=' signs
// str: "xmin:ymin:xmax:ymax x:y x:y x:y ..." 
/*********************
    {
       "type": "Feature",
       //"bbox": [-10.0, -10.0, 10.0, 10.0],
       "geometry": {
           "type": "LineString",
           "coordinates": [
                   [-10.0, -10.0],
                   [10.0, -10.0],
                   [10.0, 10.0],
                   [-10.0, -10.0]
           ]
       }
       //...
    }

    {
       "type": "Feature",
       //"bbox": [-10.0, -10.0, 10.0, 10.0],
       "geometry": {
           "type": "Polygon",
           "coordinates": [
               [
                   [-10.0, -10.0],
                   [10.0, -10.0],
                   [10.0, 10.0],
                   [-10.0, -10.0]
               ]
           ]
       }
       //...
    }
****************/
Jstr makeJsonVector( const Jstr &title, const char *str )
{
    dn("u23404025 makeJsonLineString str=[%s]", str );
    // makeJsonLineString str=[2.0:2.0:77.0:88.0 33.0:44.0 55.0:66.0 8.0:9.0]
    // makeJsonLineString str=[2.0:2.0:77.0:88.0 33.0 55.0 8.0]
	const char *p = skipBBox( str );
	if ( p == NULL ) return "";

	Jstr s;
	rapidjson::StringBuffer bs;
	rapidjson::Writer<rapidjson::StringBuffer> writer(bs);
	writer.StartObject();
	writer.Key("type");
	writer.String("Feature");

	while ( isspace(*p) ) ++p; //  "x:y x:y x:y ..."
	char *q = (char*)p;

	writer.Key("data");
	writer.StartObject();
	    writer.Key("type");
	    writer.String( title.c_str() );
		writer.Key("coordinates");
		writer.StartArray(); 
        int num = 0;
		while( *q != '\0' ) {
			writer.StartArray(); 

			while ( *q != ' ' && *q != ':' && *q != '\0' ) ++q;
			if ( *q == '\0' ) {
				writer.Double( jagatof(p) );
				writer.EndArray(); 
				break;
			}

			s = Jstr(p, q-p, q-p);
			writer.Double( jagatof(s.c_str()) );
			writer.EndArray(); 
            ++num;

			while (*q != ' ' && *q != '\0' ) ++q;
			while (*q == ' ' ) ++q;
			p = q;
		}
		writer.EndArray(); 
	writer.EndObject();

	writer.Key("properties");
	writer.StartObject();
	    writer.Key("points");
	    writer.String( intToStr(num).c_str() );

	    writer.Key("dimension");
	    writer.String( "1" );
	writer.EndObject();

	writer.EndObject();

	return (char*)bs.GetString();
}

Jstr makeJsonLineString( const Jstr &title, const JagStrSplit &sp, const char *str )
{
    dn("u23404003 makeJsonLineString str=[%s]", str );
    // makeJsonLineString str=[2.0:2.0:77.0:88.0 33.0:44.0 55.0:66.0 8.0:9.0]
    // makeJsonLineString str=[33.0:44.0 55.0:66.0 8.0:9.0]

	const char *p = skipBBox( str );
	if ( p == NULL ) return "";

	Jstr s;
	rapidjson::StringBuffer bs;
	rapidjson::Writer<rapidjson::StringBuffer> writer(bs);
	writer.StartObject();
	writer.Key("type");
	writer.String("Feature");

    /***
    Jstr  ss(str, p-str, p-str);
    dn("u87602223 in makeJsonLineString() ss=[%s]", ss.s() );
	JagStrSplit bsp( ss, ':' );
    dn("u61120 bsp.size=%d", bsp.length() );

	if ( bsp.length() == 4 ) {
		writer.Key("bbox");
		writer.StartArray();
		writer.Double( jagatof(bsp[0].c_str()) );
		writer.Double( jagatof(bsp[1].c_str()) );
		writer.Double( jagatof(bsp[2].c_str()) );
		writer.Double( jagatof(bsp[3].c_str()) );
		writer.EndArray();
	} else if ( bsp.length() == 6 ) {
		writer.Key("bbox");
		writer.StartArray();
		writer.Double( jagatof(bsp[0].c_str()) );
		writer.Double( jagatof(bsp[1].c_str()) );
		writer.Double( jagatof(bsp[2].c_str()) );
		writer.Double( jagatof(bsp[3].c_str()) );
		writer.Double( jagatof(bsp[4].c_str()) );
		writer.Double( jagatof(bsp[5].c_str()) );
		writer.EndArray();
	}
    ***/

    dn("u22271 p=[%s]", p );

	while ( isspace(*p) ) ++p; //  "x:y x:y x:y ..."
	char *q = (char*)p;

    dn("u22281 p=[%s]", p );

	writer.Key("geometry");
	writer.StartObject();

	    writer.Key("type");
	    writer.String( title.c_str() );
		writer.Key("coordinates");

		writer.StartArray(); 
		while( *q != '\0' ) {
			writer.StartArray(); 
			while (*q != ':' && *q != '\0' ) ++q;
			if ( *q == '\0' ) {
				writer.EndArray(); 
				break;
			}

			s = Jstr(p, q-p, q-p);
			dn("u0123941 s=[%s]", s.s() );
			writer.Double( jagatof(s.c_str()) );

			++q;
			p = q;
			while ( *q != ' ' && *q != ':' && *q != '\0' ) ++q;
			if ( *q == '\0' ) {
				writer.Double( jagatof(p) );
				writer.EndArray(); 
                dn("u8712220 break here");
				break;
			}

			s = Jstr(p, q-p, q-p);
			writer.Double( jagatof(s.c_str()) );
			writer.EndArray(); 

			while (*q != ' ' && *q != '\0' ) ++q;
			while (*q == ' ' ) ++q;
			p = q;
		}
		writer.EndArray(); 
	writer.EndObject();

	writer.Key("properties");
	writer.StartObject();
	    writer.Key("column");
	    writer.String( sp[2].c_str() );
	    writer.Key("srid");
	    writer.String( sp[1].c_str() );
	    writer.Key("dimension");
	    writer.String( "2" );
	writer.EndObject();

	writer.EndObject();

	return (char*)bs.GetString();
}

Jstr makeJsonLineString3D( const Jstr &title, const JagStrSplit &sp, const char *str )
{
	const char *p = skipBBox( str );
	if ( p == NULL ) return "";

	Jstr s;
	rapidjson::StringBuffer bs;
	rapidjson::Writer<rapidjson::StringBuffer> writer(bs);
	writer.StartObject();
	writer.Key("type");
	writer.String("Feature");

    /***
	JagStrSplit bsp( Jstr(str, p-str, p-str), ':' );
	if ( bsp.length() >= 6 ) {
		writer.Key("bbox");
		writer.StartArray();
		writer.Double( jagatof(bsp[0].c_str()) );
		writer.Double( jagatof(bsp[1].c_str()) );
		writer.Double( jagatof(bsp[2].c_str()) );
		writer.Double( jagatof(bsp[3].c_str()) );
		writer.Double( jagatof(bsp[4].c_str()) );
		writer.Double( jagatof(bsp[5].c_str()) );
		writer.EndArray();
	}
    ***/

	while ( isspace(*p) ) ++p;  
	char *q = (char*)p;

	writer.Key("geometry");
	writer.StartObject();
	    writer.Key("type");
	    writer.String( title.c_str() );
		writer.Key("coordinates");
		writer.StartArray(); 
		while( *q != '\0' ) {
			writer.StartArray(); 

			while (*q != ':' && *q != '\0' ) ++q;
			if ( *q == '\0' ) {
				writer.EndArray(); 
				break;
			}
			s = Jstr(p, q-p, q-p);
			writer.Double( jagatof(s.c_str()) );

			++q;
			p = q;
			while (*q != ':' && *q != '\0' ) ++q;
			if ( *q == '\0' ) {
				writer.EndArray(); 
				break;
			}
			s = Jstr(p, q-p, q-p);
			writer.Double( jagatof(s.c_str()) );

			++q;
			p = q;
			while ( *q != ' ' && *q != ':' && *q != '\0' ) ++q;
			if ( *q == '\0' ) {
				writer.Double( jagatof(p) );
				writer.EndArray(); 
				break;
			}

			s = Jstr(p, q-p, q-p);
			writer.Double( jagatof(s.c_str()) );
			writer.EndArray(); 

			while (*q != ' ' && *q != '\0' ) ++q;
			while (*q == ' ' ) ++q;
			p = q;
		}
		writer.EndArray(); 
	writer.EndObject();

	writer.Key("properties");
	writer.StartObject();
	    writer.Key("column");
	    writer.String( sp[2].c_str() );
	    writer.Key("srid");
	    writer.String( sp[1].c_str() );
	    writer.Key("dimension");
	    writer.String( "3" );
	writer.EndObject();

	writer.EndObject();

	return (char*)bs.GetString();
}

/********************************************************
{
   "type": "Polygon",
   "coordinates": [
       [ [100.0, 0.0], [101.0, 0.0], [101.0, 1.0], [100.0, 1.0], [100.0, 0.0] ],
       [ [100.2, 0.2], [100.8, 0.2], [100.8, 0.8], [100.2, 0.8], [100.2, 0.2] ]
   ]
}
********************************************************/
Jstr makeJsonPolygon( const Jstr &title,  const JagStrSplit &sp, const char *str, bool is3D )
{
	//d("s7081 makeJsonPolygon str=[%s] is3D=%d", str, is3D );
	const char *p = skipBBox( str );
	if ( p == NULL ) return "";

	rapidjson::StringBuffer bs;
	rapidjson::Writer<rapidjson::StringBuffer> writer(bs);
	writer.StartObject();
	writer.Key("type");
	writer.String("Feature");

    /***
	Jstr bbox(str, p-str, p-str);
	JagStrSplit bsp( bbox, ':' );
	if ( bsp.length() == 4 ) {
		writer.Key("bbox");
		writer.StartArray();
		writer.Double( jagatof(bsp[0].c_str()) );
		writer.Double( jagatof(bsp[1].c_str()) );
		writer.Double( jagatof(bsp[2].c_str()) );
		writer.Double( jagatof(bsp[3].c_str()) );
		writer.EndArray();
	} else if ( bsp.length() == 6 ) {
		writer.Key("bbox");
		writer.StartArray();
		writer.Double( jagatof(bsp[0].c_str()) );
		writer.Double( jagatof(bsp[1].c_str()) );
		writer.Double( jagatof(bsp[2].c_str()) );
		writer.Double( jagatof(bsp[3].c_str()) );
		writer.Double( jagatof(bsp[4].c_str()) );
		writer.Double( jagatof(bsp[5].c_str()) );
		writer.EndArray();
	}
    ***/

	while ( isspace(*p) ) ++p;
	char *q = (char*)p;
	Jstr s;

	writer.Key("geometry");
	writer.StartObject();
	    writer.Key("type");
	    writer.String( title.c_str() );
		writer.Key("coordinates");
		writer.StartArray(); 
		bool startRing = true;
		while( *q != '\0' ) {
			while (*q == ' ' ) ++q; 
			p = q;
			if ( startRing ) {
				writer.StartArray(); 
				startRing = false;
			}

			while (*q != ':' && *q != '\0' && *q != '|' ) ++q;
			if ( *q == '\0' ) {
				writer.EndArray(); // outeraray
				break;
			}

			if ( *q == '|' ) {
				writer.EndArray(); 
				startRing = true;
				++q;
				p = q;
				continue;
			}

			s = Jstr(p, q-p, q-p);

			writer.StartArray(); 
			writer.Double( jagatof(s.c_str()) );

			++q;
			p = q;
			if ( is3D ) {
				while ( *q != ':' && *q != '\0' && *q != '|' ) ++q;
			} else {
				while ( *q != ' ' && *q != ':' && *q != '\0' && *q != '|' ) ++q;
			}

			s = Jstr(p, q-p, q-p);
			writer.Double( jagatof(s.c_str()) );

			if ( is3D && *q != '\0' ) {
				++q;
				p = q;
				while ( *q != ' ' && *q != ':' && *q != '\0' && *q != '|' ) ++q;
				s = Jstr(p, q-p, q-p);
				writer.Double( jagatof(s.c_str()) );
			}

			writer.EndArray(); // inner raray

			if ( *q == '\0' ) {
				writer.EndArray(); // outeraray
				break;
			}

			if ( *q == '|' ) {
				writer.EndArray(); // outer raray
				startRing = true;
				++q;
				p = q;
				continue;
			}

			while (*q != ' ' && *q != '\0' ) ++q;  // goto next x:y coord
			while (*q == ' ' ) ++q;  // goto next x:y coord
			if ( *q == '\0' ) {
				writer.EndArray(); // outeraray
				break;
			}

			p = q;
		}

		writer.EndArray(); 
	writer.EndObject();

	writer.Key("properties");
	writer.StartObject();
	    writer.Key("column");
	    writer.String( sp[2].c_str() );
	    writer.Key("srid");
	    writer.String( sp[1].c_str() );
	    writer.Key("dimension");
		if ( is3D ) {
	    	writer.String( "3" );
		} else {
	    	writer.String( "2" );
		}
	writer.EndObject();

	writer.EndObject();

	return (char*)bs.GetString();
}

/***********************************************************************************
{
   "type": "MultiPolygon",
   "coordinates": [
       [
           [ [102.0, 2.0], [103.0, 2.0], [103.0, 3.0], [102.0, 3.0], [102.0, 2.0] ]
       ],
       [
           [ [100.0, 0.0], [101.0, 0.0], [101.0, 1.0], [100.0, 1.0], [100.0, 0.0] ],
           [ [100.2, 0.2], [100.8, 0.2], [100.8, 0.8], [100.2, 0.8], [100.2, 0.2] ]
       ]
   ]
}
***********************************************************************************/
Jstr makeJsonMultiPolygon( const Jstr &title,  const JagStrSplit &sp, const char *str, bool is3D )
{
	const char *p = skipBBox( str );
	if ( p == NULL ) return "";

	rapidjson::StringBuffer bs;
	rapidjson::Writer<rapidjson::StringBuffer> writer(bs);
	writer.StartObject();
	writer.Key("type");
	writer.String("Feature");

    /***
	Jstr bbox(str, p-str, p-str);
	JagStrSplit bsp( bbox, ':' );
	if ( bsp.length() == 4 ) {
		writer.Key("bbox");
		writer.StartArray();
		writer.Double( jagatof(bsp[0].c_str()) );
		writer.Double( jagatof(bsp[1].c_str()) );
		writer.Double( jagatof(bsp[2].c_str()) );
		writer.Double( jagatof(bsp[3].c_str()) );
		writer.EndArray();
	} else if ( bsp.length() == 6 ) {
		writer.Key("bbox");
		writer.StartArray();
		writer.Double( jagatof(bsp[0].c_str()) );
		writer.Double( jagatof(bsp[1].c_str()) );
		writer.Double( jagatof(bsp[2].c_str()) );
		writer.Double( jagatof(bsp[3].c_str()) );
		writer.Double( jagatof(bsp[4].c_str()) );
		writer.Double( jagatof(bsp[5].c_str()) );
		writer.EndArray();
	}
    ***/

	while ( isspace(*p) ) ++p; //  "x:y x:y x:y ..."
	char *q = (char*)p;
	Jstr s;

	writer.Key("geometry");
	writer.StartObject();
	    writer.Key("type");
	    writer.String( title.c_str() );
		writer.Key("coordinates");
		writer.StartArray(); 
		bool startPolygon = true;
		bool startRing = true;
		while( *q != '\0' ) {
			while (*q == ' ' ) ++q; 
			p = q;

			if ( startPolygon ) {
				writer.StartArray(); 
				startPolygon = false;
			}

			if ( startRing ) {
				writer.StartArray(); 
				//++level;
				startRing = false;
			}

			while (*q != ':' && *q != '\0' && *q != '|' && *q != '!' ) ++q;
			if ( *q == '\0' ) {
				writer.EndArray(); // outeraray
				writer.EndArray(); // outeraray
				break;
			}

			if ( *q == '|' ) {
				writer.EndArray(); // outeraray
				startRing = true;
				++q;
				p = q;
				continue;
			}

			if ( *q == '!' ) {
				writer.EndArray(); // outeraray
				writer.EndArray(); // outeraray
				startPolygon = true;
				startRing = true;
				++q;
				p = q;
				continue;
			}

			s = Jstr(p, q-p, q-p);

			writer.StartArray(); 
			writer.Double( jagatof(s.c_str()) );

			++q;
			p = q;
			if ( is3D ) {
				while ( *q != ':' && *q != '\0' && *q != '|' && *q != '!' ) ++q;
			} else {
				while ( *q != ' ' && *q != '\0' && *q != '|' && *q != '!' ) ++q;
			}

			s = Jstr(p, q-p, q-p);
			writer.Double( jagatof(s.c_str()) );

			if ( is3D && *q != '\0' ) {
				++q;
				p = q;
				//dn("s2039 q=[%s] p=[%s]n", q, p );
				while ( *q != ' ' && *q != '\0' && *q != '|' ) ++q;
				s = Jstr(p, q-p, q-p);
				//writer.String( s.c_str(), s.size() );   // z-coord
				writer.Double( jagatof(s.c_str()) );
			}

			writer.EndArray(); // inner raray

			if ( *q == '\0' ) {
				writer.EndArray(); // outeraray
				writer.EndArray(); // outeraray
				break;
			}

			if ( *q == '|' ) {
				writer.EndArray(); // outer raray
				startRing = true;
				++q;
				p = q;
				continue;
			}

			if ( *q == '!' ) {
				writer.EndArray(); // outer raray
				writer.EndArray(); // outer raray
				startPolygon = true;
				startRing = true;
				//d("s1162 level=%d continue p=[%s]\n", level, p );
				++q;
				p = q;
				continue;
			}

			while (*q == ' ' ) ++q;  // goto next x:y coord
			//d("s2339 q=[%s]\n", q );
			if ( *q == '\0' ) {
				writer.EndArray(); // outeraray
				writer.EndArray(); // outeraray
				break;
			}

			p = q;
		}

		writer.EndArray(); 
	writer.EndObject();

	writer.Key("properties");
	writer.StartObject();
	    writer.Key("column");
	    writer.String( sp[2].c_str() );
	    writer.Key("srid");
	    writer.String( sp[1].c_str() );
	    writer.Key("dimension");
		if ( is3D ) {
	    	writer.String( "3" );
		} else {
	    	writer.String( "2" );
		}
	writer.EndObject();

	writer.EndObject();

	return (char*)bs.GetString();
}

Jstr makeJsonDefault( const JagStrSplit &sp, const char *str )
{
	return "";
}

int getDimension( const Jstr& colType )
{
	if ( colType == JAG_C_COL_TYPE_VECTOR || colType ==  JAG_C_COL_TYPE_DATA ) {
        return 1;
    }

	if ( colType == JAG_C_COL_TYPE_POINT 
		|| colType == JAG_C_COL_TYPE_LINE 
		|| colType == JAG_C_COL_TYPE_LINESTRING
		|| colType == JAG_C_COL_TYPE_MULTILINESTRING
		|| colType == JAG_C_COL_TYPE_MULTIPOLYGON
		|| colType == JAG_C_COL_TYPE_MULTIPOINT
		|| colType == JAG_C_COL_TYPE_POLYGON
		|| colType == JAG_C_COL_TYPE_CIRCLE
		|| colType == JAG_C_COL_TYPE_SQUARE
		|| colType == JAG_C_COL_TYPE_RECTANGLE
		|| colType == JAG_C_COL_TYPE_TRIANGLE
		|| colType == JAG_C_COL_TYPE_ELLIPSE
		 ) {
		 return 2;
	 } else if (  colType == JAG_C_COL_TYPE_POINT3D
	 			 ||  colType == JAG_C_COL_TYPE_LINE3D
	 			 ||  colType == JAG_C_COL_TYPE_LINESTRING3D
	 			 ||  colType == JAG_C_COL_TYPE_MULTILINESTRING3D
				 || colType == JAG_C_COL_TYPE_MULTIPOINT3D
	 			 ||  colType == JAG_C_COL_TYPE_POLYGON3D
	 			 ||  colType == JAG_C_COL_TYPE_MULTIPOLYGON3D
	 			 ||  colType == JAG_C_COL_TYPE_CIRCLE3D
	 			 ||  colType == JAG_C_COL_TYPE_SPHERE
	 			 ||  colType == JAG_C_COL_TYPE_SQUARE3D
	 			 ||  colType == JAG_C_COL_TYPE_CUBE
	 			 ||  colType == JAG_C_COL_TYPE_RECTANGLE3D
	 			 ||  colType == JAG_C_COL_TYPE_BOX
	 			 ||  colType == JAG_C_COL_TYPE_TRIANGLE3D
	 			 ||  colType == JAG_C_COL_TYPE_CYLINDER
	 			 ||  colType == JAG_C_COL_TYPE_CONE
	 			 ||  colType == JAG_C_COL_TYPE_ELLIPSE3D
	 			 ||  colType == JAG_C_COL_TYPE_ELLIPSOID
				 ) {
		 return 3;
	 } else {
	 	return 0;
	 }
}


Jstr getTypeStr( const Jstr& colType )
{
	Jstr t;

	if ( colType == JAG_C_COL_TYPE_DATA ) {
		t = "Data";
	} else if ( colType == JAG_C_COL_TYPE_POINT ) {
		t = "Point";
	} else if ( colType == JAG_C_COL_TYPE_LINE ) {
		t = "Line";
	} else if ( colType == JAG_C_COL_TYPE_VECTOR ) {
		t = "Vector";
	} else if ( colType == JAG_C_COL_TYPE_LINESTRING ) {
		t = "LineString";
	} else if ( colType == JAG_C_COL_TYPE_MULTILINESTRING ) {
		t = "MultiLineString";
	} else if ( colType == JAG_C_COL_TYPE_MULTIPOLYGON ) {
		t = "MultiPolygon";
	} else if ( colType == JAG_C_COL_TYPE_MULTIPOINT ) {
		t = "MultiPoint";
	} else if ( colType == JAG_C_COL_TYPE_POLYGON ) {
		t = "Polygon";
	} else if ( colType == JAG_C_COL_TYPE_CIRCLE ) {
		t = "Circle";
	} else if ( colType == JAG_C_COL_TYPE_SQUARE ) {
		t = "Square";
	} else if ( colType == JAG_C_COL_TYPE_RECTANGLE ) {
		t = "Rectangle";
	} else if ( colType == JAG_C_COL_TYPE_TRIANGLE ) {
		t = "Triangle";
	} else if ( colType == JAG_C_COL_TYPE_ELLIPSE ) {
		t = "Ellipse";
	} else if ( colType == JAG_C_COL_TYPE_POINT3D ) {
		t = "Point3D";
	} else if ( colType == JAG_C_COL_TYPE_LINE3D ) {
		t = "Line3D";
	} else if ( colType == JAG_C_COL_TYPE_LINESTRING3D ) {
		t = "LineString3D";
	} else if ( colType == JAG_C_COL_TYPE_MULTILINESTRING3D ) {
		t = "MultiLineString3D";
	} else if ( colType == JAG_C_COL_TYPE_MULTIPOINT3D ) {
		t = "MultiPoint3D";
	} else if ( colType == JAG_C_COL_TYPE_POLYGON3D ) {
		t = "Polygon3D";
	} else if ( colType == JAG_C_COL_TYPE_MULTIPOLYGON3D ) {
		t = "MultiPolygon3D";
	} else if ( colType == JAG_C_COL_TYPE_CIRCLE3D ) {
		t = "Circle3D";
	} else if ( colType == JAG_C_COL_TYPE_SPHERE ) {
		t = "Sphere";
	} else if ( colType == JAG_C_COL_TYPE_SQUARE3D ) {
		t = "Square3D";
	} else if ( colType == JAG_C_COL_TYPE_CUBE ) {
		t = "Cube";
	} else if ( colType == JAG_C_COL_TYPE_RECTANGLE3D ) {
		t = "Rectangle3D";
	} else if ( colType == JAG_C_COL_TYPE_BOX  ) {
		t = "Box";
	} else if ( colType == JAG_C_COL_TYPE_TRIANGLE3D  ) {
		t = "Triangle3D";
	} else if ( colType == JAG_C_COL_TYPE_CYLINDER  ) {
		t = "Cylinder";
	} else if ( colType == JAG_C_COL_TYPE_CONE   ) {
		t = "Cone";
	} else if ( colType == JAG_C_COL_TYPE_ELLIPSE3D   ) {
		t = "Ellipse3D";
	} else if ( colType == JAG_C_COL_TYPE_ELLIPSOID ) {
		t = "Ellipsoid";
	} else if ( colType == JAG_C_COL_TYPE_DBIGINT ) {
		t = "bigint";
	} else if ( colType == JAG_C_COL_TYPE_DINT ) {
		t = "int";
	} else if ( colType == JAG_C_COL_TYPE_DSMALLINT ) {
		t = "smallint";
	} else if ( colType == JAG_C_COL_TYPE_DMEDINT ) {
		t = "mediumint";
	} else if ( colType == JAG_C_COL_TYPE_FLOAT ) {
		t = "float";
	} else if ( colType == JAG_C_COL_TYPE_LONGDOUBLE ) {
		t = "longdouble";
	} else if ( colType == JAG_C_COL_TYPE_DOUBLE ) {
		t = "double";
	} else if ( colType == JAG_C_COL_TYPE_UUID ) {
		t = "uuid";
	} else if ( colType == JAG_C_COL_TYPE_FILE ) {
		t = "file";
	} else if ( colType == JAG_C_COL_TYPE_ENUM ) {
		t = "enum";
	} else {
		t = "Unknown";
	}
	return t;
}

int getPolyDimension( const Jstr& colType )
{
    if ( colType == JAG_C_COL_TYPE_VECTOR ) {
        return 1;
    }

	if ( colType == JAG_C_COL_TYPE_LINESTRING
	     || colType == JAG_C_COL_TYPE_MULTILINESTRING
		 || colType == JAG_C_COL_TYPE_POLYGON
		 || colType == JAG_C_COL_TYPE_MULTIPOLYGON
		 || colType == JAG_C_COL_TYPE_MULTIPOINT
		 ) {
		 return 2;
	 } else if (  colType == JAG_C_COL_TYPE_LINESTRING3D
	     		 || colType == JAG_C_COL_TYPE_MULTILINESTRING3D
	 			 ||  colType == JAG_C_COL_TYPE_POLYGON3D
	 			 ||  colType == JAG_C_COL_TYPE_MULTIPOLYGON3D
	 			 ||  colType == JAG_C_COL_TYPE_MULTIPOINT3D
				 ) {
		 return 3;
	 } else {
	 	return 0;
	 }
}

jagint sendMessage( const JagRequest &req, const char *mesg, char msgtype, char endtype)
{
	if ( ! req.hasReply ) return 1;
    jagint len = strlen( mesg );
	return sendMessageLength2( req.session, mesg, len, msgtype, endtype );
}

jagint sendMessageLength( const JagRequest &req, const char *mesg, jagint msglen, char msgtype, char endtype )
{
	if ( ! req.hasReply ) return 1;
    return sendMessageLength2( req.session, mesg, msglen, msgtype, endtype );
}

jagint sendMessageLength2( JagSession *session, const char *mesg, jagint msglen, char msgtype, char endtype )
{
	d("u3020217 thrd=%lu receiver_IP=%s msgtype=[%c] endtype=[%c]\n", THID, session->ip.c_str(), msgtype, endtype );
    //dn("u2202287 in sendMessageLength2 dump mesg: [%s] msglen=%d", mesg, msglen);
    // dumpmem(mesg, msglen);

	char *buf = NULL;
	jagint rc = 0;

	char sqlhdr[8]; makeSQLHeader( sqlhdr );
	if ( msgtype == JAG_MSG_HB ) {
		sqlhdr[0] = 'H'; sqlhdr[1] = 'B'; 
		sqlhdr[2] = 'B'; // "HBB"
	}

	char code4[5];

	if ( msglen >= JAG_SOCK_COMPRSS_MIN ) {
        dn("u300329 msglen >= JAG_SOCK_COMPRSS_MIN compress send ...");
		Jstr comp;
		JagFastCompress::compress( mesg, msglen, comp );
		msglen = comp.size();
		buf = (char*)jagmalloc( JAG_SOCK_TOTAL_HDR_LEN + msglen + 1 + 64 );
		sprintf( code4, "Z%c%cC", msgtype, endtype );
		putXmitHdrAndData( buf, sqlhdr, comp.c_str(), msglen, code4 );
	} else {
        dn("u20229804 no compress send");
 		buf = (char*)jagmalloc( JAG_SOCK_TOTAL_HDR_LEN + msglen + 1 + 64 );
		sprintf( code4, "C%c%cC", msgtype, endtype );
		putXmitHdrAndData( buf, sqlhdr, mesg, msglen, code4 );
	}

	rc = sendRawData( session->sock, buf, JAG_SOCK_TOTAL_HDR_LEN+msglen );
    dn("u23330501 sendRawData rc=%d", rc );

	if ( rc < 0 ) {
		session->sessionBroken = 1; 
		d("s444448 session broken\n");
	}

	if ( rc < JAG_SOCK_TOTAL_HDR_LEN+msglen ) {
		d("s20298 error sendMLen rc = %d < %d(HLN=%d) mesg=[%s] msglen=%d -1\n", 
		  rc, JAG_SOCK_TOTAL_HDR_LEN+msglen , JAG_SOCK_TOTAL_HDR_LEN, mesg, msglen );
		rc = -1;
	}

	if ( buf ) free( buf );
	d("s4455083 sendMessageLength2() rc=%d thrd=%lu receiver_IP=%s\n", rc, THID, session->ip.c_str() );
    return rc;
}

jagint sendEOM( const JagRequest &req, const char *dbgcode )
{
	if ( ! req.hasReply ) return 1;
	return sendMessage( req, dbgcode, JAG_MSG_EMPTY, JAG_MSG_NEXT_END);
}

jagint sendER( const JagRequest &req, const char *err )
{
	if ( ! req.hasReply ) return 1;

    Jstr errmsg(err);
    if ( req.session->servobj ) {
        errmsg += Jstr(" fromserver: ") + req.session->serverIP;
    }

    return sendMessageLength( req, errmsg.s(), errmsg.size(), JAG_MSG_ERR, JAG_MSG_NEXT_END);
}

jagint sendER( const JagRequest &req, const Jstr &err )
{
	if ( ! req.hasReply ) return 1;

    Jstr errmsg = err;
    if ( req.session->servobj ) {
        errmsg += Jstr(" fromserver: ") + req.session->serverIP;
    }

	return sendMessageLength(req, errmsg.s(), errmsg.size(), JAG_MSG_ERR, JAG_MSG_NEXT_END);
}

jagint sendDataMore( const JagRequest &req, const char *data )
{
	if ( ! req.hasReply ) return 1;
	return sendMessage(req, data, JAG_MSG_DATA, JAG_MSG_NEXT_MORE);
}

jagint sendDataMore( const JagRequest &req, const char *data, jagint len )
{
	if ( ! req.hasReply ) return 1;
	return sendMessageLength(req, data, len, JAG_MSG_DATA, JAG_MSG_NEXT_MORE);
}

jagint sendDataMore( const JagRequest &req, const Jstr &data )
{
	if ( ! req.hasReply ) return 1;
	return sendMessageLength(req, data.s(), data.size(), JAG_MSG_DATA, JAG_MSG_NEXT_MORE);
}

jagint sendDataEnd( const JagRequest &req, const char *data )
{
	if ( ! req.hasReply ) return 1;
	return sendMessage(req, data, JAG_MSG_DATA, JAG_MSG_NEXT_END);
}

jagint sendDataEnd( const JagRequest &req, const Jstr &data )
{
	if ( ! req.hasReply ) return 1;
	return sendMessageLength(req, data.s(), data.size(), JAG_MSG_DATA, JAG_MSG_NEXT_END);
}

jagint sendOKEnd( const JagRequest &req, const char *data )
{
	if ( ! req.hasReply ) return 1;
	return sendMessage(req, data, JAG_MSG_OK, JAG_MSG_NEXT_END);
}

jagint sendOKEnd( const JagRequest &req, const Jstr &data )
{
	if ( ! req.hasReply ) return 1;
	return sendMessageLength(req, data.s(), data.size(), JAG_MSG_OK, JAG_MSG_NEXT_END);
}

jagint sendOKMore( const JagRequest &req, const char *data )
{
	if ( ! req.hasReply ) return 1;
	return sendMessage(req, data, JAG_MSG_OK, JAG_MSG_NEXT_MORE);
}

jagint sendOKMore( const JagRequest &req, const char *data, jagint len )
{
	if ( ! req.hasReply ) return 1;
	return sendMessageLength(req, data, len, JAG_MSG_OK, JAG_MSG_NEXT_MORE);
}

jagint sendOKMore( const JagRequest &req, const Jstr &data )
{
	if ( ! req.hasReply ) return 1;
	return sendMessageLength(req, data.s(), data.size(), JAG_MSG_OK, JAG_MSG_NEXT_MORE);
}

Jstr convertToStr( const Jstr  &pm )
{
    Jstr str;
    if ( pm == JAG_ROLE_SELECT  ) {
        str = "select";
    } else if ( pm == JAG_ROLE_INSERT ) {
        str = "insert";
    } else if ( pm == JAG_ROLE_UPDATE ) {
        str = "update";
    } else if ( pm == JAG_ROLE_DELETE ) {
        str = "delete";
    } else if ( pm == JAG_ROLE_CREATE ) {
        str = "create";
    } else if ( pm == JAG_ROLE_DROP ) {
        str = "drop";
    } else if ( pm == JAG_ROLE_ALTER ) {
        str = "alter";
    } else if ( pm == JAG_ROLE_ALL  ) {
        str = "all";
    }

    return str;
}

// pms "A,U,D"
Jstr convertManyToStr( const Jstr &pms )
{
    Jstr str;
    JagStrSplit sp( pms, ',', true );
    for ( int i=0; i < sp.length(); ++i ) {
        str += convertToStr( sp[i] ) + ",";
    }
    return trimTailChar(str, ',' );
}

Jstr convertType2Short( const Jstr &geotypeLong )
{
	const char *p = geotypeLong.c_str();
    if ( 0==strcasecmp(p, "point" ) ) {
		return JAG_C_COL_TYPE_POINT;
	} else if ( 0==strcasecmp(p, "point3d" ) ) {
		return JAG_C_COL_TYPE_POINT3D;
	} else if ( 0==strcasecmp(p, "line" ) ) {
		return JAG_C_COL_TYPE_LINE;
	} else if ( 0==strcasecmp(p, "line3d" ) ) {
		return JAG_C_COL_TYPE_LINE3D;
	} else if ( 0==strcasecmp(p, "circle" ) ) {
		return JAG_C_COL_TYPE_CIRCLE;
	} else if ( 0==strcasecmp(p, "circle3d" ) ) {
		return JAG_C_COL_TYPE_CIRCLE3D;
	} else if ( 0==strcasecmp(p, "sphere" ) ) {
		return JAG_C_COL_TYPE_SPHERE;
	} else if ( 0==strcasecmp(p, "square" ) ) {
		return JAG_C_COL_TYPE_SQUARE;
	} else if ( 0==strcasecmp(p, "square3d" ) ) {
		return JAG_C_COL_TYPE_SQUARE3D;
	} else if ( 0==strcasecmp(p, "cube" ) ) {
		return JAG_C_COL_TYPE_CUBE;
	} else if ( 0==strcasecmp(p, "rectangle" ) ) {
		return JAG_C_COL_TYPE_RECTANGLE;
	} else if ( 0==strcasecmp(p, "rectangle3d" ) ) {
		return JAG_C_COL_TYPE_RECTANGLE3D;
	} else if ( 0==strcasecmp(p, "bbox" ) ) {
		return JAG_C_COL_TYPE_BBOX;
	} else if ( 0==strcasecmp(p, "box" ) ) {
		return JAG_C_COL_TYPE_BOX;
	} else if ( 0==strcasecmp(p, "cone" ) ) {
		return JAG_C_COL_TYPE_CONE;
	} else if ( 0==strcasecmp(p, "triangle" ) ) {
		return JAG_C_COL_TYPE_TRIANGLE;
	} else if ( 0==strcasecmp(p, "triangle3d" ) ) {
		return JAG_C_COL_TYPE_TRIANGLE3D;
	} else if ( 0==strcasecmp(p, "cylinder" ) ) {
		return JAG_C_COL_TYPE_CYLINDER;
	} else if ( 0==strcasecmp(p, "ellipse" ) ) {
		return JAG_C_COL_TYPE_ELLIPSE;
	} else if ( 0==strcasecmp(p, "ellipse3d" ) ) {
		return JAG_C_COL_TYPE_ELLIPSE3D;
	} else if ( 0==strcasecmp(p, "ellipsoid" ) ) {
		return JAG_C_COL_TYPE_ELLIPSOID;
	} else if ( 0==strcasecmp(p, "polygon" ) ) {
		return JAG_C_COL_TYPE_POLYGON;
	} else if ( 0==strcasecmp(p, "polygon3d" ) ) {
		return JAG_C_COL_TYPE_POLYGON3D;
	} else if ( 0==strcasecmp(p, "vector" ) ) {
		return JAG_C_COL_TYPE_VECTOR;
	} else if ( 0==strcasecmp(p, "linestring" ) ) {
		return JAG_C_COL_TYPE_LINESTRING;
	} else if ( 0==strcasecmp(p, "linestring3d" ) ) {
		return JAG_C_COL_TYPE_LINESTRING3D;
	} else if ( 0==strcasecmp(p, "multipoint" ) ) {
		return JAG_C_COL_TYPE_MULTIPOINT;
	} else if ( 0==strcasecmp(p, "multipoint3d" ) ) {
		return JAG_C_COL_TYPE_MULTIPOINT3D;
	} else if ( 0==strcasecmp(p, "multilinestring" ) ) {
		return JAG_C_COL_TYPE_MULTILINESTRING;
	} else if ( 0==strcasecmp(p, "multilinestring3d" ) ) {
		return JAG_C_COL_TYPE_MULTILINESTRING3D;
	} else if ( 0==strcasecmp(p, "multipolygon" ) ) {
		return JAG_C_COL_TYPE_MULTIPOLYGON;
	} else if ( 0==strcasecmp(p, "multipolygon3d" ) ) {
		return JAG_C_COL_TYPE_MULTIPOLYGON3D;
	} else if ( 0==strcasecmp(p, "range" ) ) {
		return JAG_C_COL_TYPE_RANGE;
	} else {
		return "UNKNOWN";
	}
}

Jstr firstToken( const char *str, char sep )
{
    if ( NULL == str || *str == NBT ) {
		d("u127608 firstToken str NULL or NBT\n" );
		return "";
	}
	char *p = (char*)str;
	if ( sep != NBT ) {
    	while ( ! isspace(*p) && *p != sep && *p != '\0' ) ++p;
		// str points to first non-space char
	} else {
    	while ( *p != sep && *p != '\0' ) ++p;
		// p points to sep or end NBT
	}
	return Jstr(str, p-str);
}

Jstr jagerr( int errcode )
{
	Jstr err;
	if ( -30144 == errcode ) {
		err = "Key column must not be a roll-up column";
	} else if ( -30145 == errcode )  {
		err = "Spare column must be a char column";
	} else if ( -10511 == errcode )  {
		err = "dropdb force must have a database name";
	} else if ( -12823 == errcode )  {
		err = "timeseries(TIMESERIES|RETAINPERIOD) is required";
	} else if ( -12820 == errcode || -12821 == errcode )  {
		err = "timeseries syntax is incorrect";
	} else if ( -15315 == errcode )  {
		err = "A value column must be given";
	} else if ( -13052 == errcode )  {
		err = "A timeseries table must have a timestamp(nano) or datetime(nano) KEY column";
	} else if ( -13050 == errcode )  {
		err = "A table cannot have duplicated columns";
	} else if ( -19000 == errcode )  {
		err = "Column name too long";
	} else if ( -19001 == errcode )  {
		err = "Column name cannot have . character";
	} else if ( -19013 == errcode )  {
		err = "Syntax error near rollup";
	} else if ( -90030 == errcode )  {
		err = "Column type error";
	} else if ( -18000 == errcode || -18010 == errcode || -18030 == errcode )  {
		err = "enum syntax is incorrect";
	} else if ( -19042 == errcode )  {
		err = "srid is too large ( must be <= 2000000000 )";
	} else if ( -19060 == errcode )  {
		err = "Syntax error near default";
	} else if ( -19150 == errcode )  {
		err = "Default value not enclosed with single-quotes or double-quotes";
	} else if ( -12800 == errcode )  {
		err = "Inserting duplicate columns";
	} else if ( -11010 == errcode )  {
		err = "Empty command";
	} else if ( -13822 == errcode )  {
		err = "Wrong rention unit";
	} else if ( -19004 == errcode )  {
		err = "Column type is missing";
	} else if ( -19003 == errcode )  {
		err = "Column name is invalid";
	} else if ( -13821 == errcode )  {
		err = "TimeSeries clause is invalid";
	} else if ( -14001 == errcode )  {
		err = "Parsing exception";
	} else if ( -14002 == errcode )  {
		err = "Parsing unknown exception";
	} else if ( -14003 == errcode )  {
		err = "Parsing error";
	}

	return err;
}

bool hasDefaultValue( char spare4 )
{
    if ( spare4 == JAG_CREATE_DEFINSERTVALUE ) return true;
    return hasDefaultDateTimeValue( spare4 );
}

bool hasDefaultDateTimeValue( char spare4 )
{
    if ( spare4 == JAG_CREATE_DEFDATE || spare4 == JAG_CREATE_DEFUPDATE_DATE ) return true;
    if ( spare4 == JAG_CREATE_DEFDATETIMESEC || spare4 == JAG_CREATE_DEFUPDATE_DATETIMESEC ) return true;
    if ( spare4 == JAG_CREATE_DEFDATETIME || spare4 == JAG_CREATE_DEFUPDATE_DATETIME ) return true;
    if ( spare4 == JAG_CREATE_DEFDATETIMENANO || spare4 == JAG_CREATE_DEFUPDATE_DATETIMENANO ) return true;
    if ( spare4 == JAG_CREATE_DEFDATETIMEMILL || spare4 == JAG_CREATE_DEFUPDATE_DATETIMEMILL ) return true;
    return false;
}

bool hasDefaultUpdateDateTime( char spare4 )
{
    if ( spare4 == JAG_CREATE_UPDATE_DATE || spare4 == JAG_CREATE_DEFUPDATE_DATE ) return true;
    if ( spare4 == JAG_CREATE_UPDATE_DATETIMESEC || spare4 == JAG_CREATE_DEFUPDATE_DATETIMESEC ) return true;
    if ( spare4 == JAG_CREATE_UPDATE_DATETIME || spare4 == JAG_CREATE_DEFUPDATE_DATETIME ) return true;
    if ( spare4 == JAG_CREATE_UPDATE_DATETIMENANO || spare4 == JAG_CREATE_DEFUPDATE_DATETIMENANO ) return true;
    if ( spare4 == JAG_CREATE_UPDATE_DATETIMEMILL || spare4 == JAG_CREATE_DEFUPDATE_DATETIMEMILL ) return true;
    return false;
}

Jstr charToStr(char c)
{
	char p[2];
	p[0] = c;
	p[1] = '\0';
	return p;
}

bool endWithSQLRightBra( const char *sql )
{
	if ( !sql ) return false;
	if ( *sql == '\0' ) return false;
	int len  = strlen(sql);
	const char *r = sql + len-1;
	bool seebra = false;
	while ( r != sql ) {
		if ( isspace(*r ) || *r == ';' ) {
			--r;
		} else {
			if ( *r == ')' ) {
				seebra = true;
			}
			break;
		}
	}

	if ( seebra ) {
		return true;
	} else {
		return false;
	}

}

bool isAutoUpdateTime(char spare4)
{
	if ( JAG_CREATE_DEFUPDATE_DATE == spare4 
		 || JAG_CREATE_DEFUPDATE_DATETIMESEC == spare4
         || JAG_CREATE_DEFUPDATE_DATETIMEMILL == spare4
         || JAG_CREATE_DEFUPDATE_DATETIME == spare4 
         || JAG_CREATE_DEFUPDATE_DATETIMENANO == spare4 
         || JAG_CREATE_UPDATE_DATE == spare4 
         || JAG_CREATE_UPDATE_DATETIMESEC == spare4 
         || JAG_CREATE_UPDATE_DATETIMEMILL == spare4
         || JAG_CREATE_UPDATE_DATETIME == spare4 
         || JAG_CREATE_UPDATE_DATETIMENANO == spare4 ) {
            return true;
	}
	return false;
}

void reverseStr(char *s)
{
    if ( s == NULL || *s == '\0' ) return;
    int len = strlen(s);
    char t;
    for ( int i=0; i < len/2; ++i ) {
        t = s[i];
        s[i] = s[len-1-i];
        s[len-1-i] = t;
    }
}

void maskKey(const JagSchemaRecord &record, JagFixString &key )
{
    if ( ! record.keyHasGeo ) {
        return;
    }

    char *buf = (char*)key.s();
    const auto &cv = *(record.columnVector);
    dn("s031531 record.size=%d", cv.size() );

    for ( int i = 0; i < cv.size(); ++i ) {
        if ( ! cv[i].iskey ) {
            break;
        }

        if ( cv[i].issubcol ) {
            memset(buf + cv[i].offset, 0, cv[i].length );
            dn("u823951 memset offset - length to 0", cv[i].offset, cv[i].length );
        }
    }

}

void getMaskedKey(const JagSchemaRecord &record, const JagFixString &key, JagFixString &maskedKey )
{
    assert( record.keyLength == key.size() );

    char *buf = jagmalloc( record.keyLength );
    memcpy( buf, key.s(), key.size() );

    const auto &cv = *(record.columnVector);
    dn("s035031 record.size=%d", cv.size() );

    for ( int i = 0; i < cv.size(); ++i ) {
        if ( ! cv[i].iskey ) {
            break;
        }

        if ( cv[i].issubcol ) {
            memset(buf + cv[i].offset, 0, cv[i].length );
            dn("u803911 memset offset - length to 0", cv[i].offset, cv[i].length );

        }
    }

    maskedKey = JagFixString(buf, key.size(), key.size() );

    free( buf );
}

Jstr getFileHashDir( const JagSchemaRecord &record, const JagFixString &kstr )
{
    Jstr hdir;

    if ( record.keyHasGeo ) {
        JagFixString maskedKey;
        getMaskedKey(record, kstr, maskedKey );
        hdir = fileHashDir( maskedKey );

        /**
        dn("tab34001 fileHashDir kstr:");
        dumpmem( maskedKey.s(), maskedKey.size() );
        **/

    } else {
        hdir = fileHashDir( kstr );

        /**
        dn("tab34002 fileHashDir kstr:");
        dumpmem( kstr.s(), kstr.size() );
        **/
    }

    return hdir;
}

int leadZeros( const char *str )
{
    if ( str == NULL ) return 0;
    if ( *str == '\0' ) return 0;
    if ( *str == '0' && *(str+1) == '\0' ) return 0;

    int cnt = 0;
    while ( *str == '0' ) {
        ++cnt;
        ++str;
    }
    return cnt;
}

const char *skipBBox(const char *str)
{
	const char *p = str;
    int ncolon = 0;
    while ( *p != ' ' && *p != '\0' ) {
        ++p;
        if ( *p == ':' ) {
            ++ncolon;
        }
    }

    if ( *p == '\0' ) return NULL;;
    if ( ncolon < 3 ) { p = str; }
    return p;
}


