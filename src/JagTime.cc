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

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <assert.h>
#include <JagTime.h>
#include <JagUtil.h>
#include <JagMath.h>

int JagTime::getTimeZoneDiff()
{
    char buf[8];
    memset( buf, 0, 8 );
    time_t now = time(NULL);
    struct tm result;

	jag_localtime_r ( &now, &result );

    strftime( buf, 8,  "%z", &result );  // -0500
	int hrmin =  atoi( buf );
	int hour = hrmin/100;
	int min = hrmin % 100; 
	if ( hour < 0 ) {
		min = 60 * hour - min;
	} else {
		min = 60 * hour + min;
	}

	if ( result.tm_isdst > 0 ) {
		min -= 60;
	}

	return min;
}

jaguint JagTime::mtime()
{
	struct timeval now;
	gettimeofday( &now, NULL );
	return (1000*(jaguint)now.tv_sec + now.tv_usec/1000);
}

jaguint JagTime::utime()
{
	struct timeval now;
	gettimeofday( &now, NULL );
	return (1000000*(jaguint)now.tv_sec + now.tv_usec);
}

Jstr JagTime::makeRandDateTimeString( int N )
{
	time_t rawtime;
  	struct tm * timeinfo;
  	struct tm  result;
  	char buffer [80];
	jagint yearSecs = 365 * 86400;
	jagint rnd = rand() % (N*yearSecs);

  	time (&rawtime);
	rawtime -=  rnd;
	timeinfo = jag_localtime_r ( &rawtime, &result );
  	strftime (buffer,80,"%Y-%m-%d %H:%M:%S", timeinfo);
	return buffer;
}

Jstr JagTime::makeRandTimeString()
{
	time_t rawtime;
  	struct tm * timeinfo;
  	struct tm  result;
  	char buffer [80];

  	time (&rawtime);
	timeinfo = jag_localtime_r ( &rawtime, &result );
  	strftime (buffer,80,"%H:%M:%S", timeinfo);
	return buffer;
}

Jstr JagTime::YYYYMMDDHHMM()
{
	time_t rawtime;
  	struct tm * timeinfo;
  	struct tm  result;
	char  buffer[64];

  	time (&rawtime);
	timeinfo = jag_localtime_r ( &rawtime, &result );
  	strftime( buffer, 64, "%Y-%m-%d-%H-%M", timeinfo);
	return buffer;
}

Jstr JagTime::YYYYMMDDHHMMSS()
{
	time_t rawtime;
  	struct tm * timeinfo;
  	struct tm  result;
	char  buffer[64];

  	time (&rawtime);
	timeinfo = jag_localtime_r ( &rawtime, &result );
  	strftime( buffer, 64, "%Y-%m-%d-%H-%M-%S", timeinfo);
	return buffer;
}

Jstr JagTime::YYYYMMDD()
{
	time_t rawtime;
  	struct tm * timeinfo;
  	struct tm result;
	char  buffer[64];

  	time (&rawtime);
	timeinfo = jag_localtime_r ( &rawtime, &result );
  	strftime( buffer, 64, "%Y%m%d", timeinfo);
	return buffer;
}

Jstr JagTime::makeNowTimeStringSeconds()
{
	time_t rawtime;
  	struct tm * timeinfo;
  	struct tm result;
	char  buffer[80];

  	time (&rawtime);
	timeinfo = jag_localtime_r ( &rawtime, &result );
  	strftime (buffer, 80, "%Y-%m-%d %H:%M:%S", timeinfo);
	return buffer;
}

Jstr JagTime::makeNowTimeStringMilliSeconds()
{
	char  buffer[80];
	char  endbuf[12];
    struct timeval now;
    gettimeofday( &now, NULL );

    int usec = now.tv_usec;
	time_t rawtime;
  	struct tm * timeinfo;
  	struct tm result;

  	time (&rawtime);
	timeinfo = jag_localtime_r ( &rawtime, &result );
  	strftime (buffer, 80, "%Y-%m-%d %H:%M:%S", timeinfo);
	sprintf( endbuf, ".%d", usec/1000 );
	strcat( buffer, endbuf );
	return buffer;
}

Jstr JagTime::makeNowTimeStringMicroSeconds()
{
	char  buffer[80];
	char  endbuf[8];
    struct timeval now;
    gettimeofday( &now, NULL );
    int usec = now.tv_usec;
	time_t rawtime;
  	struct tm * timeinfo;
  	struct tm result;
  	time (&rawtime);
	timeinfo = jag_localtime_r ( &rawtime, &result );
  	strftime (buffer, 80, "%Y-%m-%d %H:%M:%S", timeinfo);
	sprintf( endbuf, ".%d", usec );
	strcat( buffer, endbuf );
	return buffer;
}

// str: "yyyy-mm-dd hh:mm:ss.cccc"  "hh:mm:ss"  "yyyy-mm-dd"
bool JagTime::setTimeInfo( const JagParseAttribute &jpa , const char *str, struct tm &timeinfo, int isTime ) 
{
	const char *ttime, *tdate;
	jagint timeval = -1;

	dn("s202928 setTimeInfo str=[%s] isTime=%d", str, isTime );
	
	ttime = strchr(str, ':'); 
	tdate = strchr(str, '-');

	if ( ttime ) {
		if ( strchrnum(str, ':') != 2 ) {
			return false;
		}
		if ( strlen(str) < 8 ) return false;
	}

	if ( tdate ) {
		if ( strchrnum(str, '-') != 2 ) {
			return false;
		}
		if ( strlen(str) < 10 ) return false;
	}

	if ( ttime && tdate ) {
		// "2016-02-13 12:32:22"
		timeval = getDateTimeFromStr( jpa, str, isTime );
		dn("s202238 ttime && tdate str=[%s] timeval=[%lld]", str, timeval );
	} else if ( ttime ) {
		// "12:23:00"
		timeval = getTimeFromStr( str, isTime );
		dn("s202238 ttime timeval=[%lld]", timeval );
	} else if ( tdate ) {
		// "yyyy-mm-dd" only
        int year = rayatoi(str, 4) - 1900;;
        int mon = rayatoi(str+5, 2) - 1;
        int mday = rayatoi(str+8, 2);
		dn("s233038001 tdate str=[%s] year=%d mon=%d mday=%d", str, year, mon, mday );

		timeinfo.tm_year = year;
		timeinfo.tm_mon = mon;
		timeinfo.tm_mday = mday;
		timeinfo.tm_hour = 0;
		timeinfo.tm_min = 0;
		timeinfo.tm_sec = 0;
		timeinfo.tm_isdst = -1;

        //debug only
        /**
        time_t  tm = mktime( &timeinfo );
        dn("s692004 tm=%ld maketime got seconds from timeinfo", tm );
        **/

	} else {
		if ( isTime ) {
			// "19293944"  microsec, nanosec, or secs, or milliseconds
			timeval = jagatoll( str );
		} else {
			// "20130212"
			timeinfo.tm_year = rayatoi(str, 4) - 1900;
			timeinfo.tm_mon = rayatoi(str+4, 2) - 1;
			timeinfo.tm_mday = rayatoi(str+6, 2);
			timeinfo.tm_hour = 0;
			timeinfo.tm_min = 0;
			timeinfo.tm_sec = 0;			
			timeinfo.tm_isdst = -1;
		}
	}

	if ( timeval >= 0 ) {
		dn("s20118 timeval=%lld isTime=%d", timeval, isTime );
		time_t secs = timeval;

		if ( JAG_TIME_SECOND_MICRO == isTime ) secs = timeval / 1000000;
		else if ( JAG_TIME_SECOND == isTime ) secs = timeval;
		else if ( JAG_TIME_SECOND_NANO == isTime ) secs = timeval / 1000000000;
		else if ( JAG_TIME_SECOND_MILLI == isTime ) secs = timeval / 1000;

		jagint  defTZDiffMin = jpa.timediff;
		jagint servTZDiffMin = jpa.servtimediff;
		jagint minSrvCli = servTZDiffMin - defTZDiffMin;
		d("s23941 secs=%lld\n", secs );

		secs -=  minSrvCli * 60;
		d("s23942 secs=%lld\n", secs );
		jag_localtime_r( &secs, &timeinfo );
	}
    // else: timeinfo is already set

	return true;
}

JagFixString 
JagTime::getValueFromTimeOrDate( const JagParseAttribute &jpa, const JagFixString &str, const Jstr &type,
								const JagFixString &str2, const Jstr &type2, int op, const Jstr& ddiff ) 
{
	dn("s28336 getValueFromTimeOrDate str=[%s] str2=[%s] op=%d ddiff=%s", str.c_str(), str2.c_str(), op, ddiff.s() );
	dn("s28336 getValueFromTimeOrDate str.len=[%d] str2.leng=%d", str.length(), str2.length() );

	char *p1, *p2;
	char savebyte;
	int timelen, isTime, isTime2;
	struct tm timeinfo;

	//timelen = JAG_DATETIMENANO_FIELD_LEN-6;
	// 20 - 6 = 14  // microseconds space

	timelen = 16;  // roughly, big enough

	char buf[timelen+1];
	memset(buf, 0, timelen+1);

    int slen = str.length();
	isTime = checkTimeType( type, slen );
    dn("t20233301 slen=%d checkTimeType type=[%s] isTime=%d", slen, type.s(), isTime );

	p1 = (char*)str.c_str();
	savebyte = *(p1+slen);
	*(p1+slen) = '\0';

	dn("s4120348 isTime=%d p1=[%s]", isTime, p1 );

	bool rc1 = setTimeInfo( jpa, p1, timeinfo, isTime );
	if ( ! rc1 ) { return ""; }

	*(p1+slen) = savebyte;	

	if ( op == JAG_FUNC_SECOND ) {
		sprintf(buf, "%d", timeinfo.tm_sec);
	} else if ( op == JAG_FUNC_MINUTE ) {
		sprintf(buf, "%d", timeinfo.tm_min);
	} else if ( op == JAG_FUNC_HOUR ) {
		sprintf(buf, "%d", timeinfo.tm_hour);
	} else if ( op == JAG_FUNC_DAY ) {
		sprintf(buf, "%d", timeinfo.tm_mday);
	} else if ( op == JAG_FUNC_MONTH ) {
		sprintf(buf, "%d", timeinfo.tm_mon + 1);
	} else if ( op == JAG_FUNC_YEAR ) {
		sprintf(buf, "%d", timeinfo.tm_year + 1900);
	} else if ( op == JAG_FUNC_DATE ) {
        dn("c82277023");
		sprintf(buf, "%4d-%02d-%02d", timeinfo.tm_year + 1900, timeinfo.tm_mon+1, timeinfo.tm_mday );
	} else if ( op == JAG_FUNC_DAYOFMONTH ) {
		if ( timeinfo.tm_mday <= 1 || timeinfo.tm_mday > 31 ) {
			buf[0] = '0';
		} else {
			sprintf(buf, "%d", timeinfo.tm_mday);
		}
	} else if ( op == JAG_FUNC_DAYOFWEEK ) {
		sprintf(buf, "%d", timeinfo.tm_wday);
	} else if ( op == JAG_FUNC_DAYOFYEAR ) {
		sprintf(buf, "%d", timeinfo.tm_yday + 1);
	} else if ( op == JAG_FUNC_DATEDIFF ) {
		struct tm timeinfo2;
		isTime2 = checkTimeType( type2, str2.length() );

		p2 = (char*)str2.c_str();
		savebyte = *(p2+str2.length());
		*(p2+str2.length()) = '\0';

	    dn("s4120358  isTime2=%d p=[%s]", isTime2, p2 );

        if ( type2.size() < 1 ) {
            isTime2 = isTime; 
            dn("s20239 set isTime2 to isTime %d", isTime );
        }

		bool rc2 = setTimeInfo( jpa, p2, timeinfo2, isTime2 );		
		if ( ! rc2 ) { return ""; }

		*(p2+str2.length()) = savebyte;

		time_t begint = mktime( &timeinfo );
		time_t endt = mktime( &timeinfo2 );

		dn("s300183 begint=%lu endt=%lu  diff=%lld", begint, endt, endt - begint );

		jagint diff = 0; // assuming seconds
		if ( endt > -1 && begint > -1 ) {
			diff = endt - begint;

        		if ( ddiff == "m" ) {
        			diff = diff / 60;
        		} else if ( ddiff == "h" ) {
        			diff = diff / 3600;
        		} else if ( ddiff == "D" ) {
        			diff = diff / 86400;
        		} else if ( ddiff == "M" ) {
        			diff = (timeinfo2.tm_year-timeinfo.tm_year)*12 + (timeinfo2.tm_mon-timeinfo.tm_mon);
        		} else if ( ddiff == "Y" ) {
        			diff = timeinfo2.tm_year - timeinfo.tm_year;
        		}
		} 

		snprintf(buf, timelen+1, "%lld", diff);
	}

	d("s400124 buf=[%s] timelen=%d\n", buf, timelen );
	
	JagFixString res(buf);

	d("s400125 res=[%s] timelen=%d\n", res.s(), timelen );
	return res;	
}

// instr: "1882738383399"  outstr: "yyyy-mm-dd hh:mm:ss.mmmmmmm"
//void JagTime::convertDateTimeToLocalStr( const Jstr& instr, Jstr& outstr, int isnano )
void JagTime::convertDateTimeToStr( const Jstr& instr, Jstr& outstr, bool local, int isnano )
{
	char tmstr[48];
	char utime[12];
	jaguint timestamp;
	jaguint  sec;
	jaguint  usec;
	struct tm *tmptr;
	struct tm result;

	timestamp = jagatoll( instr.c_str() );
	if ( 0 == isnano || JAG_TIME_SECOND_MICRO == isnano ) {
        // JAG_TIME_SECOND_MICRO
		sec = timestamp/1000000; 
		usec = timestamp%1000000;
	} else if ( JAG_TIME_SECOND_NANO == isnano ) {
        // JAG_TIME_SECOND_NANO
		sec = timestamp/1000000000; 
		usec = timestamp%1000000000;
	} else if ( JAG_TIME_SECOND == isnano ) {
        // JAG_TIME_SECOND
		sec = timestamp;
		usec = 0;
	} else if ( JAG_TIME_SECOND_MILLI == isnano ) {
        // JAG_TIME_SECOND_MILLI
		sec = timestamp/1000; 
		usec = timestamp%1000;
	}

	time_t tsec = sec;
    if ( local ) {
	    tmptr = jag_localtime_r( &tsec, &result );  // client calls this
    } else {
	    tmptr = gmtime_r( &tsec, &result );  // server calls this
    }

	strftime( tmstr, sizeof(tmstr), "%Y-%m-%d %H:%M:%S", tmptr ); 

	if ( 0 == isnano || JAG_TIME_SECOND_MICRO == isnano ) {
		sprintf( utime, ".%06lld", usec );
		strcat( tmstr, utime );
		outstr = tmstr;
	} else if ( JAG_TIME_SECOND_NANO == isnano ) {
		sprintf( utime, ".%09lld", usec );
		strcat( tmstr, utime );
		outstr = tmstr;
	} else if ( JAG_TIME_SECOND_MILLI == isnano ) {
		sprintf( utime, ".%03lld", usec );
		strcat( tmstr, utime );
		outstr = tmstr;
	} else if ( JAG_TIME_SECOND == isnano ) {
		outstr = tmstr;
	}

}

// instr: "1838383399"  outstr: "hh:mm:ss.mmmmmmm"
void JagTime::convertTimeToStr( const Jstr& instr, Jstr& outstr, int tmtype )
{
	char    tmstr[24];
	char    utime[12];
	jagint  timestamp;
	time_t  sec;
	jagint  usec;
	struct  tm *tmptr;

	timestamp = jagatoll( instr.c_str() );
	if ( JAG_TIME_SECOND_MICRO == tmtype ) {
		sec = timestamp/1000000; 
		usec = timestamp%1000000;
	} else if ( JAG_TIME_SECOND_NANO == tmtype ) {
		sec = timestamp/1000000000; 
		usec = timestamp%1000000000;
	} else if ( JAG_TIME_SECOND == tmtype ) {
		sec = timestamp;
		usec = 0;
	} else if ( JAG_TIME_SECOND_MILLI == tmtype ) {
		sec = timestamp/1000; 
		usec = timestamp%1000;
	} else { 
        // micro seconds
		sec = timestamp/1000000; 
		usec = timestamp%1000000;
	}

	struct tm result;

	tmptr = gmtime_r( &sec, &result );

	strftime( tmstr, sizeof(tmstr), "%H:%M:%S", tmptr ); 

	if ( JAG_TIME_SECOND_MICRO == tmtype ) {
	    sprintf( utime, ".%06lld", usec );
		strcat( tmstr, utime );
	} else if ( JAG_TIME_SECOND_NANO == tmtype ) {
		sprintf( utime, ".%09lld", usec );
		strcat( tmstr, utime );
	} else if ( JAG_TIME_SECOND_MILLI == tmtype ) {
		sprintf( utime, ".%03lld", usec );
		strcat( tmstr, utime );
	} else if ( JAG_TIME_SECOND == tmtype ) {
	} 

	outstr = tmstr;
}

// instr "yyyymmdd"  outstr "yyyy-mm-dd"
void JagTime::convertDateToStr( const Jstr& instr, Jstr& outstr )
{
	char  buf[11];
	memset( buf, 0, 11 );
	buf[0] = instr[0];
	buf[1] = instr[1];
	buf[2] = instr[2];
	buf[3] = instr[3];
	buf[4] = '-';
	buf[5] = instr[4];
	buf[6] = instr[5];
	buf[7] = '-';
	buf[8] = instr[6];
	buf[9] = instr[7];
	outstr = buf;
}

// inbuf: "12828220000"  or   "2028-21-21 12:13:21...." 
jagint JagTime::getNumDateTime( const JagParseAttribute &jpa, const char *inbuf, int isnano )
{
	d("s31661 getNumDateTime inbuf=[%s] isnano=%d\n", inbuf, isnano );
    jagint lonnum = 0;

	if ( ( NULL == strchr(inbuf, '-') && NULL == strchr(inbuf, ':') ) ) {
        dn("s400028 no - nor : in inbuf");
		// inbuf:  "16263838380"
		if ( JAG_TIME_SECOND_NANO == isnano ) {
			lonnum = jagatoll( inbuf ); 
			d("s400123 2 == isnano lonnum=%lld\n", lonnum );
		} else if ( JAG_TIME_SECOND == isnano ) {
			lonnum = jagatoll( inbuf );
			if ( lonnum > jagint(1000000000) * JAG_BILLION ) {
			   lonnum = lonnum/JAG_BILLION;
			} else if ( lonnum > jagint(1000000000) * JAG_MILLION ) {
			   lonnum = lonnum/JAG_MILLION;
			}

		} else if ( JAG_TIME_SECOND_MILLI == isnano ) {
			// to milliseconds
			lonnum = jagatoll( inbuf );
			if ( lonnum > jagint(1000000000) * JAG_BILLION ) {
			   lonnum = lonnum/JAG_MILLION;
			} else if ( lonnum > jagint(1000000000) * JAG_MILLION ) {
			   lonnum = lonnum/1000;
			}

			d("s401124 4 == isnano lonnum=%lld\n", lonnum );
		} else {
			// to microsecs
			lonnum = jagatoll( inbuf );
			if ( lonnum > jagint(1000000000) * JAG_BILLION ) {
			   lonnum = lonnum/1000;
			}
			d("s401125 microsecs 1 == isnano lonnum=%lld\n", lonnum );
		}
		// d("s2883 pure num in datestr inbuf lonnum=%lld\n", lonnum  );
	} else {
		// inbuf:  "2020-21-21 12:13:21.000000" or datetimesec or datatimenano format
		lonnum = getDateTimeFromStr( jpa, inbuf, isnano);
        dn("s300039 inbuf has - or :  got lonnum=%ld", lonnum );
	}

    return lonnum;
}

int JagTime::convertDateTimeFormat( const JagParseAttribute &jpa, char *outbuf, const char *inbuf, 
								    int offset, int length, int isnano )
{
	d("s31661 convertDateTimeFormat inbuf=[%s] isnano=%d\n", inbuf, isnano );
    jagint lonnum = getNumDateTime( jpa, inbuf, isnano );
    if ( lonnum < 0 ) return 1;

    if ( JAG_TIME_SECOND_MICRO == isnano ) {
        Jstr b;
        JagMath::base254FromULong( b, lonnum, JAG_TIMESTAMPMICRO_FIELD_LEN );
        memcpy(outbuf+offset, b.s(), JAG_TIMESTAMPMICRO_FIELD_LEN);
        assert( length == JAG_TIMESTAMPMICRO_FIELD_LEN ); 
        dn("s303029 converted long=%ld to base254=[%s]", lonnum, b.s() );
    } else if ( JAG_TIME_SECOND_NANO == isnano ) {
        Jstr b;
        JagMath::base254FromULong( b, lonnum, JAG_TIMESTAMPNANO_FIELD_LEN );
        memcpy(outbuf+offset, b.s(), JAG_TIMESTAMPNANO_FIELD_LEN);
        assert( length == JAG_TIMESTAMPNANO_FIELD_LEN ); 
        dn("s303029 converted long=%ld to base254=[%s]", lonnum, b.s() );
    } else if ( JAG_TIME_SECOND_MILLI == isnano ) {
        Jstr b;
        JagMath::base254FromULong( b, lonnum, JAG_TIMESTAMPMILLI_FIELD_LEN );
        memcpy(outbuf+offset, b.s(), JAG_TIMESTAMPMILLI_FIELD_LEN);
        assert( length == JAG_TIMESTAMPMILLI_FIELD_LEN ); 
        dn("s303029 converted long=%ld to base254=[%s]", lonnum, b.s() );
    } else if ( JAG_TIME_SECOND == isnano ) {
        Jstr b;
        JagMath::base254FromULong( b, lonnum, JAG_DATETIMESEC_FIELD_LEN );
        memcpy(outbuf+offset, b.s(), JAG_DATETIMESEC_FIELD_LEN);
        assert( length == JAG_DATETIMESEC_FIELD_LEN ); 
        dn("s303029 converted long=%ld to base254=[%s]", lonnum, b.s() );
    } else {
        int rlen = snprintf(outbuf+offset, length+1, "%0*lld", length, lonnum);
        if ( rlen > length ) {
		    d("s1884 convertDateTimeFormat return 2 length=%d rlen=%d lonnum=%ld\n", length, rlen, lonnum );
		    return 2; // error
	    }
    }

	return 0; // OK
}

// inbuf: "8282299"  "17:34:28"
int JagTime::convertTimeFormat( char *outbuf, const char *inbuf, int offset, int length, int isnano )
{
    jagint lonnum = getNumTime(inbuf, isnano) ;
    if ( lonnum < 0 ) {
        return 1;
    }

    /***
	if ( strchr((char*)inbuf, ':') ) {
		lonnum = getTimeFromStr(inbuf, isnano);
		if ( -1 == lonnum ) return 1;
	} else {
		lonnum = jagatoll( inbuf );
	}

    if ( snprintf(outbuf+offset, length+1, "%0*lld", length, lonnum) > length ) {
		return 2;
	}
    ***/

    if ( JAG_TIME_SECOND_MICRO == isnano ) {
        Jstr b;
        JagMath::base254FromULong( b, lonnum, JAG_TIMEMICRO_FIELD_LEN );
        memcpy(outbuf+offset, b.s(), JAG_TIMEMICRO_FIELD_LEN);
        assert( length == JAG_TIMEMICRO_FIELD_LEN ); 
        dn("s303049 converted long=%ld to base254=[%s]", lonnum, b.s() );
    } else if ( JAG_TIME_SECOND_NANO == isnano ) {
        Jstr b;
        JagMath::base254FromULong( b, lonnum, JAG_TIMENANO_FIELD_LEN );
        memcpy(outbuf+offset, b.s(), JAG_TIMENANO_FIELD_LEN);
        assert( length == JAG_TIMENANO_FIELD_LEN ); 
        dn("s303529 converted long=%ld to base254=[%s]", lonnum, b.s() );
    } else {
        int rlen = snprintf(outbuf+offset, length+1, "%0*lld", length, lonnum);
        if ( rlen > length ) {
		    d("s420884 convertTimeFormat return 2 length=%d rlen=%d lonnum=%ld\n", length, rlen, lonnum );
		    return 2; // error
	    }
    }

	return 0;
}

jagint JagTime::getNumTime( const char *inbuf, int isnano )
{
    jagint lonnum;
	if ( strchr((char*)inbuf, ':') ) {
		lonnum = getTimeFromStr(inbuf, isnano);
		if ( -1 == lonnum ) return -1;
	} else {
		lonnum = jagatoll( inbuf );
	}

    return lonnum;
}

jagint JagTime::getNumDate( const char *inbuf )
{
	char buf[9];
	const char *ubuf;
	memset(buf, 0, 9);

	if ( NULL != strchr((char*)inbuf, '-') && !getDateFromStr(inbuf, buf) ) {
		return -1;
	}

	if ( *buf != '\0' ) ubuf = buf;
	else ubuf = inbuf;

    jagint n8 = jagatol(ubuf);
    return n8;
}

int JagTime::convertDateFormat( char *outbuf, const char *inbuf, int offset, int length )
{
    /***
	char buf[JAG_DATE_FIELD_LEN+1];
	const char *ubuf;
	memset(buf, 0, JAG_DATE_FIELD_LEN+1);
		
	if ( NULL != strchr((char*)inbuf, '-') && !getDateFromStr(inbuf, buf) ) {
		return 1;
	}

	if ( *buf != '\0' ) ubuf = buf;
	else ubuf = inbuf;

	if ( snprintf(outbuf+offset, length+1, "%s", ubuf) > length ) return 2;

	return 0;
    ***/

    long n8 = getNumDate( inbuf );
    if ( n8 < 0 ) return 1;

    Jstr b;
    JagMath::base254FromULong( b, n8, JAG_DATE_FIELD_LEN );
    memcpy(outbuf+offset, b.s(), JAG_DATE_FIELD_LEN);

    return 0;
}

jaguint JagTime::getDateTimeFromStr( const JagParseAttribute &jpa, const char *str, int isnano )
{
	jagint  defTZDiffMin = jpa.timediff;
	jagint  servTZDiffMin = jpa.servtimediff;
	jagint  minSrvCli = servTZDiffMin - defTZDiffMin;

	int c;
	char buf10[10];
    memset(buf10, 0, 10);
	struct tm  ttime;
	char *s = (char*) str;
	char *p;
	char v;
	jaguint res;

	ttime.tm_isdst = -1;

	// get yyyy
	p = s;
	while ( *p != '-' && *p != '\0' ) ++p;
	if ( *p == '\0' ) return 0;

	*p = '\0';
	ttime.tm_year = atoi(s) - 1900;
	if ( ttime.tm_year >= 10000 ) return 0;
	*p = '-';
	s = p+1;
	if ( *s == '\0' ) return 0;

	// get mm
	p = s;
	while ( *p != '-' && *p != '\0' ) ++p;
	if ( *p == '\0' ) return 0;
	*p = '\0';
	ttime.tm_mon = atoi(s) - 1;
	// if ( ttime.tm_mon < 0 || ttime.tm_mon > 11 ) return -1;
	*p = '-';

	// get dd
	s = p+1;
	if ( *s == '\0' ) return 0;

	p = s;
	while ( *p != ' ' && *p != '\0' ) ++p;
	if ( *p == '\0' ) return 0;
	*p = '\0';
	ttime.tm_mday = atoi(s);
	// if ( ttime.tm_mday < 1 || ttime.tm_mday > 31 ) return -1;
	*p = ' ';

	while ( *p == ' ' && *p != '\0' ) ++p;
	if ( *p == '\0' ) return 0;

	// hh:mm:ss[.ffffff]
	// get hh
	s = p;
	while ( *p != ':' && *p != '\0' ) ++p;
	if ( *s == '\0' ) return 0;
	*p = '\0';
	ttime.tm_hour = atoi(s);
	// if ( ttime.tm_hour > 23 ) return -1;
	*p = ':';

	// get mm
	++p;
	s = p;
	while ( *p != ':' && *p != '\0' ) ++p;
	if ( *s == '\0' ) return 0;
	*p = '\0';
	ttime.tm_min = atoi(s);
	// if ( ttime.tm_min > 59 ) return -1;
	*p = ':';

	// get ss
	++p;
	s = p;
	while ( *p != '.' && *p != ' ' && *p != '\0' ) ++p;
	v = *p;
	*p = '\0';
	ttime.tm_sec = atoi(s);
	// if ( ttime.tm_sec > 59 ) return -1;
	*p = v;


   	// no .ffffff section
   	// res = 1000000* mktime( &ttime );
   	res = mktime( &ttime );
   	// adjust server
   	res += minSrvCli * 60;

	if ( JAG_TIME_SECOND_NANO == isnano ) {
		res *= (jaguint)1000000000;
	} else if ( JAG_TIME_SECOND_MICRO == isnano ) {
		res *= (jaguint)1000000;
	} else if ( JAG_TIME_SECOND == isnano || 0 == isnano ) {
		// seconds
	} else if ( JAG_TIME_SECOND_MILLI == isnano ) {
		res *= (jaguint)1000;
	} else {
		res *= (jaguint)1000000;
	}

   	jagint zsecdiff;
	jagint tzhour;

	while ( 1 ) {
    	if ( *p == '\0' ) { break; }
    
    	// saw .fff or .ffffff or .fffffffff
    	if ( *p == '.' ) {
    		if ( JAG_TIME_SECOND_NANO == isnano ) {
        		memset(buf10, '0', 9);
			} else if ( JAG_TIME_SECOND_MICRO == isnano ) {
    			memset(buf10, '0', 6);
			} else if ( JAG_TIME_SECOND == isnano ) {
    			memset(buf10, 0, 10);  // seconds
			} else if ( JAG_TIME_SECOND_MILLI == isnano ) {
    			memset(buf10, '0', 3);
    		} else {
                // seconds
    			memset(buf10, 0, 10);
    		}

        	++p;
        	s = p;
        	c = 0;
    		while ( *p != '\0' && *p != '+' && *p != '-' && *p != ' ' ) {
    			if ( (JAG_TIME_SECOND_MICRO == isnano) && c < 6 ) {
    				buf10[c] = *p;
					++c;
    			} else if ( ( JAG_TIME_SECOND_NANO == isnano ) && c < 9 ) {
    				buf10[c] = *p;
					++c;
    			} else if ( ( JAG_TIME_SECOND_MILLI == isnano ) && c < 3 ) {
    				buf10[c] = *p;
					++c;
    			} else {
    				while ( *p != '\0' && *p != ' ' ) ++p;
    				break;
    			}

    			++p;
    		}
    
        	res += jagatoll(buf10);
    	}
    
    	if ( *p == '\0' ) { break; }
    
    	while ( *p == ' ' ) ++p;  // skip blanks
    	if ( *p == '\0' ) {
			break;
    	}
    
		// -08:00 or +09:30 part
    	if ( *p == '-' ) {
    		c = -1; ++p;
    	} else if ( *p == '+' )  {
    		c = 1; ++p;
    	} else {
    		c = 1;
    	}
    
    	s = p;
    	// printf("c100 s=[%s]\n", s );
    	if ( *p == '\0' ) {
    		// no time zone info is given
			break;
    	}
    
    	while ( *p != ':' && *p != '\0' ) { ++p; } // goto : in 08:30
    
    	v = *p;
    	*p = '\0';
    	// -06:30
    	// printf("c110 s=[%s] defTZDiffMin=%d atoll(s)*60=%d\n", s, defTZDiffMin, atoll(s)*60 );
    	zsecdiff;
		tzhour = jagatoll(s);
		// d("s6612 before hour adjust tzhour=%d res=%lld\n", tzhour, res );
    	if ( c > 0 ) {
    		zsecdiff =  tzhour *3600 - defTZDiffMin*60;  // adjust to original client time zone
			if ( JAG_TIME_SECOND_MICRO == isnano ) {
				// micro
				res -=  zsecdiff * (jaguint)1000000;
			} else if ( JAG_TIME_SECOND_NANO == isnano ) {
				// nano
				res -=  zsecdiff * (jaguint)1000000000;
			} else if ( JAG_TIME_SECOND_MILLI == isnano ) {
				// milli
				res -=  zsecdiff * (jaguint)1000;
			} else if ( JAG_TIME_SECOND == isnano ) {
				res -=  zsecdiff;
			} else {
			}

    	} else {
    		zsecdiff =  defTZDiffMin*60 + tzhour*3600;
			if ( JAG_TIME_SECOND_MICRO == isnano ) {
				// micro
				res +=  zsecdiff * (jaguint)1000000;
			} else if ( JAG_TIME_SECOND_NANO == isnano ) {
				// nano
				res +=  zsecdiff * (jaguint)1000000000;
			} else if ( JAG_TIME_SECOND_MILLI == isnano ) {
				// milli
				res +=  zsecdiff * (jaguint)1000;
			} else if ( JAG_TIME_SECOND == isnano ) {
				res +=  zsecdiff;
			} else {
			}
    	}

    	*p = v;
    	if ( *p == '\0' ) { break; }

    	++p; // past :   :30 minute part
    	if ( *p == '\0' ) { break; }

    	s = p;
		// d("s8309 s=[%s] res=%lld\n", s, res );
    	if ( c > 0 ) {
			if ( JAG_TIME_SECOND_MICRO == isnano ) {
				// micro
				res = res - jagatoll(s) * 60 * (jaguint)1000000;  // res is GMT time
			} else if ( JAG_TIME_SECOND_NANO == isnano ) {
				// nano
				res = res - jagatoll(s) * 60 * (jaguint)1000000000;  // res is GMT time
			} else if ( JAG_TIME_SECOND_MILLI == isnano ) {
				// milli
				res = res - jagatoll(s) * 60 * (jaguint)1000;  // res is GMT time
			} else if ( JAG_TIME_SECOND == isnano ) {
				// secs
				res = res - jagatoll(s) * 60;
			} else {
			}
    	} else {
			if ( JAG_TIME_SECOND_MICRO == isnano ) {
				// micro
				res = res + jagatoll(s) * 60 * (jaguint)1000000;  // res is GMT time
			} else if ( JAG_TIME_SECOND_NANO == isnano ) {
				// nano
				res = res + jagatoll(s) * 60 * (jaguint)1000000000;  // res is GMT time
			} else if ( JAG_TIME_SECOND_MILLI == isnano ) {
				// milli
				res = res + jagatoll(s) * 60 * (jaguint)1000;  // res is GMT time
			} else if ( JAG_TIME_SECOND == isnano ) {
				// secs
				res = res + jagatoll(s) * 60;
			} else {
			}
    	}

		break;  // must be here to exit the loop
	}

	// d("s4309 res=%lld\n", res );
	return res;
}

// str:  hh:mm:ss[.ffffff]
jagint JagTime::getTimeFromStr( const char *str, int isnano )
{
	int c;
	char buf10[10];
    memset(buf10, 0, 10);
	struct tm  ttime;
	char *s = (char*) str;
	char *p;
	char v;
	jagint res;

	ttime.tm_isdst = -1;
	ttime.tm_year = 0;
	ttime.tm_mon = 0;
	ttime.tm_mday = 1;

	p = s;
	// hh:mm:ss[.ffffff]
	// get hh
	while ( *p != ':' && *p != '\0' ) ++p;
	if ( *s == '\0' ) return -1;
	*p = '\0';
	ttime.tm_hour = atoi(s);
	if ( ttime.tm_hour > 23 ) return -1;
	*p = ':';

	// get mm
	++p;
	s = p;
	while ( *p != ':' && *p != '\0' ) ++p;
	if ( *s == '\0' ) return -1;
	*p = '\0';
	ttime.tm_min = atoi(s);
	if ( ttime.tm_min > 59 ) return -1;
	*p = ':';

	// get ss
	++p;
	s = p;
	while ( *p != '.' && *p != ' ' && *p != '\0' ) ++p;
	v = *p;
	*p = '\0';
	ttime.tm_sec = atoi(s);
	if ( ttime.tm_sec > 59 ) return -1;
	*p = v;

	// no .ffffff section
	// res = 1000000* mktime( &ttime );
	// res = 1000000* timegm( &ttime );
	res = 3600*ttime.tm_hour + 60*ttime.tm_min + ttime.tm_sec;

	if ( JAG_TIME_SECOND_MICRO == isnano ) {
		res *= (jagint)1000000;
	} else if ( JAG_TIME_SECOND_NANO == isnano ) {
		res *= (jagint)1000000000;
	} else if ( JAG_TIME_SECOND_MILLI == isnano ) {
		res *= (jagint)1000;
	} else if ( JAG_TIME_SECOND == isnano ) {
	} else {
	}

	// d("s8907 time str=[%s] ===> res=[%lld]\n", str, res);
	if ( *p == '\0' ) {
		// printf("c111\n");
		return res;
	}

	// saw .fffffff or .fffffffff
	if ( *p == '.' ) {
    	if ( JAG_TIME_SECOND_NANO == isnano ) {
       		memset(buf10, '0', 9);
		} else if ( JAG_TIME_SECOND_MICRO == isnano ) {
    		memset(buf10, '0', 6);
		} else if ( JAG_TIME_SECOND_MILLI == isnano ) {
    		memset(buf10, '0', 3);
		} else if ( JAG_TIME_SECOND == isnano ) {
    		memset(buf10, 0, 10);
    	} else {
    		memset(buf10, 0, 10);
    	}

    	++p;
    	s = p;
    	c = 0;
    	while ( *p != '\0' && *p != ' ' ) {
    		if ( (JAG_TIME_SECOND_MICRO == isnano) && c < 6 ) {
    			buf10[c] = *p;
				++c;
    		} else if ( ( JAG_TIME_SECOND_NANO == isnano ) && c < 9 ) {
    			buf10[c] = *p;
				++c;
    		} else if ( ( JAG_TIME_SECOND_MILLI == isnano ) && c < 3 ) {
    			buf10[c] = *p;
				++c;
    		} else {
    			while ( *p != '\0' && *p != ' ' ) ++p;
    			break;
    		}
    
    		++p;
    	}
    
    	res += jagatoll(buf10);
		// printf("c112 buf10=%d  s=[%s] res=%lld\n", atoll(buf10), buf10, res );
	}

	return res;
}

// instr: "yyyy-mm-dd"  outstr: "yyyymmdd"
bool JagTime::getDateFromStr( const char *instr, char *outstr )
{
	int yyyy, mm, dd;
	char *p = (char*) instr;
	bool res = false;

	// get yyyy
	yyyy =  atoi(p);
	if ( yyyy >= 10000 ) return res;

	// get mm
	while ( *p != '-' && *p != '\0' ) ++p;
	if ( *p == '\0' ) return res;
	++p;
	if ( *p == '\0' ) return res;
	mm =  atoi(p);
	if ( mm < 0 || mm > 12 ) return res;

	// get dd
	while ( *p != '-' && *p != '\0' ) ++p;
	if ( *p == '\0' ) return res;
	++p;
	if ( *p == '\0' ) return res;
	dd =  atoi(p);
	if ( dd < 1 || dd > 31 ) return res;

	// get result
	sprintf( outstr, "%04d%02d%02d", yyyy, mm, dd );

	// d("s7383 getdatestr outstr=[%s]\n", outstr );

	res = true;
	return res;
}

// cycle: 1,2,3,4,6,8,12
jagint JagTime::getStartTimeSecOfSecond( time_t tsec, int cycle )
{
	struct tm result;
	gmtime_r( &tsec, &result );
	int startsec = (result.tm_sec/cycle) * cycle;

	result.tm_sec = startsec;
	time_t  t = mktime( &result );
	return t;
}

// cycle: 1,2,3,4,6,8,12
jagint JagTime::getStartTimeSecOfMinute( time_t tsec, int cycle )
{
	struct tm result;
	
	gmtime_r( &tsec, &result );
	int startmin = (result.tm_min/cycle) * cycle; 
	dn("s22220 tm_isdst=%d  tm_min=%d cycle=%d startmin=%d", result.tm_isdst, result.tm_min, cycle, startmin );

	result.tm_sec = 0;
	result.tm_min = startmin;

	time_t  t = mktime( &result );
	return t;
}

jagint JagTime::getStartTimeSecOfHour( time_t tsec, int cycle )
{
	struct tm result;
	
	gmtime_r( &tsec, &result );
	int starthour = (result.tm_hour/cycle) * cycle;  // 0 --30
	d("s22220 tm_isdst=%d\n", result.tm_isdst );

	result.tm_sec = 0;
	result.tm_min = 0;
	result.tm_hour = starthour;

	time_t  t = mktime( &result );
	return t;
}

jagint JagTime::getStartTimeSecOfDay( time_t tsec, int cycle )
{
	struct tm result;
	gmtime_r( &tsec, &result );

	int startday = ((result.tm_mday-1)/cycle) * cycle;  // 0 --30

	result.tm_sec = 0;
	result.tm_min = 0;
	result.tm_hour = 0;
	result.tm_mday = startday + 1;  // 1--31

	time_t  t = mktime( &result );
	return t;
}

// cycle: 1 only
jagint JagTime::getStartTimeSecOfWeek( time_t tsec )
{
	struct tm result;
	
	gmtime_r( &tsec, &result );
	tsec = tsec - jagint(result.tm_wday) * 86400;  // back to sunday

	gmtime_r( &tsec, &result );
	result.tm_sec = 0;
	result.tm_min = 0;
	result.tm_hour = 0;

	time_t  t = mktime( &result );
	return t;
}

// cycle: 1,2,3,4,6
jagint JagTime::getStartTimeSecOfMonth( time_t tsec, int cycle )
{
	struct tm result;
	
	gmtime_r( &tsec, &result );
	result.tm_sec = 0;
	result.tm_min = 0;
	result.tm_hour = 0;
	result.tm_mday = 1;
	// result.tm_mon  0 --11
	result.tm_mon = (result.tm_mon/cycle) * cycle; 

	time_t  t = mktime( &result ); // ignores tm_wday and tm_yday
	return t;
}


// cycle: 1, 2
jagint JagTime::getStartTimeSecOfQuarter( time_t tsec, int cycle )
{
	struct tm result;
	gmtime_r( &tsec, &result );
	result.tm_sec = 0;
	result.tm_min = 0;
	result.tm_hour = 0;
	result.tm_mday = 1;
	// result.tm_mon  0-11
	result.tm_mon = (result.tm_mon/(3*cycle)) * 3*cycle;
	time_t  t = mktime( &result );
	return t;
}

// cycle: any
jagint JagTime::getStartTimeSecOfYear( time_t tsec, int cycle )
{
	struct tm result;
	
	gmtime_r( &tsec, &result );
	result.tm_sec = 0;
	result.tm_min = 0;
	result.tm_hour = 0;
	result.tm_mday = 1;
	result.tm_mon = 0;
	result.tm_year = (result.tm_year/cycle) * cycle;
	time_t  t = mktime( &result );
	return t;
}

// cycle: any
jagint JagTime::getStartTimeSecOfDecade( time_t tsec, int cycle )
{
	struct tm result;
	gmtime_r( &tsec, &result );
	result.tm_sec = 0;
	result.tm_min = 0;
	result.tm_hour = 0;
	result.tm_mday = 1;
	result.tm_mon = 0;
	result.tm_year = (result.tm_year/(cycle*10)) * cycle*10;
	time_t  t = mktime( &result );
	return t;
}

void JagTime::print( struct tm &t )
{
	printf("JagTime::print:\n");
	printf(" tm.tm_year=%d\n", t.tm_year );
	printf(" tm.tm_mon=%d\n", t.tm_mon );
	printf(" tm.tm_mday=%d\n", t.tm_mday );
	printf(" tm.tm_hour=%d\n", t.tm_hour );
	printf(" tm.tm_min=%d\n", t.tm_min );
	printf(" tm.tm_sec=%d\n", t.tm_sec );
	printf(" tm.tm_wday=%d\n", t.tm_wday );
	printf(" tm.tm_yday=%d\n", t.tm_yday );
	printf(" tm.tm_isdst=%d\n", t.tm_isdst );
}

// return buf
// return 1: for success;   0 for error
int JagTime::fillTimeBuffer ( time_t tsec, const Jstr &colType, char *buf )
{
	if ( colType == JAG_C_COL_TYPE_DATETIMENANO || colType == JAG_C_COL_TYPE_TIMESTAMPNANO ) {
		sprintf( buf, "%lu", tsec * 1000000000 );
	} else if ( colType == JAG_C_COL_TYPE_DATETIMEMICRO || colType == JAG_C_COL_TYPE_TIMESTAMPMICRO ) {
		sprintf( buf, "%lu", tsec * 1000000 );
	} else if ( colType == JAG_C_COL_TYPE_DATETIMEMILLI || colType == JAG_C_COL_TYPE_TIMESTAMPMILLI ) {
		sprintf( buf, "%lu", tsec * 1000 );
	} else if ( colType == JAG_C_COL_TYPE_DATETIMESEC || colType == JAG_C_COL_TYPE_TIMESTAMPSEC ) {
		sprintf( buf, "%lu", tsec );
	} else {
		return 0;
	}

	return 1;
}

// returns seconds, microseconds, or nanoseconds
// return 0 for error
time_t JagTime::getTypeTime( time_t tsec, const Jstr &colType )
{
	if ( colType == JAG_C_COL_TYPE_DATETIMENANO || colType == JAG_C_COL_TYPE_TIMESTAMPNANO ) {
		return (tsec * 1000000000 );
	} else if ( colType == JAG_C_COL_TYPE_DATETIMEMICRO || colType == JAG_C_COL_TYPE_TIMESTAMPMICRO ) {
		return (tsec * 1000000 );
	} else if ( colType == JAG_C_COL_TYPE_DATETIMEMILLI || colType == JAG_C_COL_TYPE_TIMESTAMPMILLI ) {
		return (tsec * 1000 );
	} else if ( colType == JAG_C_COL_TYPE_DATETIMESEC || colType == JAG_C_COL_TYPE_TIMESTAMPSEC ) {
		return tsec;
	} else {
		return 0;
	}
}


Jstr JagTime::getLocalTime( time_t  tsec )
{
	struct tm result;
	char tmstr[48];
	jag_localtime_r( &tsec, &result );  // client calls this
	strftime( tmstr, sizeof(tmstr), "%Y-%m-%d %H:%M:%S", &result ); 
	return tmstr;
}

jagint JagTime::nowMilliSeconds()
{
    struct timeval now;
    gettimeofday( &now, NULL );
    return (jagint)now.tv_usec / 1000 + now.tv_sec*1000;
}

jaguint JagTime::nowMicroSeconds()
{
    struct timeval now;
    gettimeofday( &now, NULL );
    return (jaguint)now.tv_usec + jaguint(now.tv_sec)*1000000;
}


void JagTime::getLocalNowBuf( const Jstr &colType, char *timebuf )
{
	if ( colType == JAG_C_COL_TYPE_TIMESTAMPSEC ) {
		sprintf( timebuf, "%ld", time(NULL) );
	} else if ( colType == JAG_C_COL_TYPE_TIMESTAMPMICRO ) {
		struct timeval now; 
		gettimeofday( &now, NULL );
		time_t snow = now.tv_sec * 1000000 + now.tv_usec;
		sprintf( timebuf, "%ld", snow );
	} else if ( colType == JAG_C_COL_TYPE_TIMESTAMPMILLI ) {
		struct timeval now; 
		gettimeofday( &now, NULL );
		time_t snow = now.tv_sec * 1000 + now.tv_usec/1000;
		sprintf( timebuf, "%ld", snow );
	} else if ( colType == JAG_C_COL_TYPE_TIMESTAMPNANO ) {
		struct timespec tp;
		clock_gettime( CLOCK_REALTIME , &tp);
		time_t snow = tp.tv_sec * 1000000000 + tp.tv_nsec;
		sprintf( timebuf, "%ld", snow );
	}
}


void JagTime::getNowTimeBuf( char spare4, char *timebuf )
{
	if ( JAG_CREATE_DEFUPDATE_DATETIME == spare4 || JAG_CREATE_DEFDATETIME == spare4 ) {
		struct timeval now; 
		gettimeofday( &now, NULL );
		time_t snow = now.tv_sec * 1000000 + now.tv_usec;
		sprintf( timebuf, "%ld", snow );
	} else if ( JAG_CREATE_DEFUPDATE_DATETIMESEC == spare4 || JAG_CREATE_DEFDATETIMESEC == spare4 ) {
		struct timeval now; 
		gettimeofday( &now, NULL );
		time_t snow = now.tv_sec;
		sprintf( timebuf, "%ld", snow );
	} else if ( JAG_CREATE_DEFUPDATE_DATE == spare4 || JAG_CREATE_DEFDATE == spare4 ) {
		struct tm res; struct timeval now; 
		gettimeofday( &now, NULL );
		time_t snow = now.tv_sec;
		jag_localtime_r( &snow, &res );
		memset( timebuf, 0, 40);
		sprintf( timebuf, "%4d-%02d-%02d", res.tm_year+1900, res.tm_mon+1, res.tm_mday );
	} else if ( JAG_CREATE_DEFUPDATE_DATETIMENANO == spare4 || JAG_CREATE_DEFDATETIMENANO == spare4 ) {
		struct timespec tp;
		clock_gettime( CLOCK_REALTIME , &tp);
		time_t snow = tp.tv_sec * 1000000000 + tp.tv_nsec;
		sprintf( timebuf, "%ld", snow );
	} else if ( JAG_CREATE_DEFUPDATE_DATETIMEMILL == spare4 || JAG_CREATE_DEFDATETIMEMILL == spare4 ) {
		struct timeval now; 
		gettimeofday( &now, NULL );
		time_t snow = now.tv_sec * 1000 + now.tv_usec/1000;
		sprintf( timebuf, "%ld", snow );
	} else {
		d("c333881 getNowTimeBuf spare4 is unknown\n");
		timebuf[0] = '\0';
	}
}

// 0: if not in right time measure (seconds, milliseconds, micro..., nano...)
int JagTime::checkTimeType( const Jstr &type, int tlen )
{
	int isTime = 0;
    dn("t02288 checkTimeType type=[%s] tlen=%d", type.s(), tlen );

    if ( type.size() > 0 ) {
        if ( type == JAG_C_COL_TYPE_TIMEMICRO ) {
            isTime = JAG_TIME_SECOND_MICRO;
        } else if ( type == JAG_C_COL_TYPE_TIMENANO ) {
            isTime = JAG_TIME_SECOND_NANO;
        } else if ( type == JAG_C_COL_TYPE_TIMESTAMPNANO || type == JAG_C_COL_TYPE_DATETIMENANO ) {
            isTime = JAG_TIME_SECOND_NANO;
        } else if ( type == JAG_C_COL_TYPE_TIMESTAMPMICRO || type == JAG_C_COL_TYPE_DATETIMEMICRO ) {
            isTime = JAG_TIME_SECOND_MICRO;
        } else if ( type == JAG_C_COL_TYPE_TIMESTAMPMILLI || type == JAG_C_COL_TYPE_DATETIMEMILLI ) {
            isTime = JAG_TIME_SECOND_MILLI;
        } else if ( type == JAG_C_COL_TYPE_TIMESTAMPSEC || type == JAG_C_COL_TYPE_DATETIMESEC ) {
            isTime = JAG_TIME_SECOND;
        } else {
            /***
            dn("s202394005 uss tlen");
        	if ( 16 == tlen || 16+1 == tlen ) {
                isTime = JAG_TIME_SECOND_MICRO; // microseconds
        	} else if ( 19 == tlen || 19+1 == tlen ) {
                isTime = JAG_TIME_SECOND_NANO; // nanosecs
        	} else if ( 10 == tlen || 10+1 == tlen ) {
                isTime = JAG_TIME_SECOND; // seconds
        	} else if ( 13 == tlen || 13+1 == tlen ) {
                isTime = JAG_TIME_SECOND_MILLI; // milliseconds
            } 
            **/
            dn("s10239 type invalid");
            abort();
        }
    } else {
        /***
        dn("s22223038 no type");
      	if ( 16 == tlen || 16+1 == tlen ) {
            isTime = JAG_TIME_SECOND_MICRO; // microseconds
       	} else if ( 19 == tlen || 19+1 == tlen ) {
            isTime = JAG_TIME_SECOND_NANO; // nanosecs
       	} else if ( 10 == tlen || 10+1 == tlen ) {
            isTime = JAG_TIME_SECOND; // seconds
       	} else if ( 13 == tlen || 13+1 == tlen ) {
            isTime = JAG_TIME_SECOND_MILLI; // milliseconds
        }
        **/
        dn("s20239 no type");
        //abort();
    }

    return isTime;
}

// instr "93838822800919"   outstr "yyyy-mm-dd hh:mm:ss.cccc" "hh:mm:ss.cccc" "yyyy-mm-dd"
void JagTime::getTimeOrDateStr( const Jstr &colType, const Jstr &instr, Jstr &outstr )
{
	if ( colType == JAG_C_COL_TYPE_DATETIMENANO || colType == JAG_C_COL_TYPE_TIMESTAMPNANO ) {
        convertDateTimeToStr( instr, outstr, true, JAG_TIME_SECOND_NANO );
	} else if ( colType == JAG_C_COL_TYPE_DATETIMEMICRO || colType == JAG_C_COL_TYPE_TIMESTAMPMICRO ) {
        convertDateTimeToStr( instr, outstr, true, JAG_TIME_SECOND_MICRO );
	} else if ( colType == JAG_C_COL_TYPE_DATETIMEMILLI || colType == JAG_C_COL_TYPE_TIMESTAMPMILLI ) {
        convertDateTimeToStr( instr, outstr, true, JAG_TIME_SECOND_MILLI );
	} else if ( colType == JAG_C_COL_TYPE_DATETIMESEC || colType == JAG_C_COL_TYPE_TIMESTAMPSEC ) {
        convertDateTimeToStr( instr, outstr, true, JAG_TIME_SECOND );
	} else if ( colType == JAG_C_COL_TYPE_TIMEMICRO ) {
        convertTimeToStr( instr, outstr, JAG_TIME_SECOND_MICRO );
	} else if ( colType == JAG_C_COL_TYPE_TIMENANO ) {
        convertTimeToStr( instr, outstr, JAG_TIME_SECOND_NANO );
	} else if ( colType == JAG_C_COL_TYPE_DATE ) {
        convertDateToStr( instr, outstr );
	} else {
        outstr = instr;
	}
}

// n is microseconds time since epoch, outstr is string adjusted to client time zone
void JagTime::getStrFromMicroSec( const JagParseAttribute &jpa, unsigned long n, Jstr &outstr)
{
	jagint defTZDiffMin = jpa.timediff;
	jagint servTZDiffMin = jpa.servtimediff;
	jagint minSrvCli = servTZDiffMin - defTZDiffMin;

    time_t secs = n / 1000000; // convert microseconds to seconds
	secs -=  minSrvCli * 60;

    struct tm  timeinfo;

	jag_localtime_r( &secs, &timeinfo );

    char tbuf[32];
    strftime(tbuf, 32, "%Y-%m-%d %H:%M:%S", &timeinfo);
    sprintf(tbuf + 19, ".%06lu", n % 1000000); // add microseconds and timezone offset
    outstr = tbuf;
}

// n is nanoseconds time since epoch, outstr is string adjusted to client time zone
void JagTime::getStrFromNanoSec( const JagParseAttribute &jpa, unsigned long n, Jstr &outstr)
{
	jagint defTZDiffMin = jpa.timediff;
	jagint servTZDiffMin = jpa.servtimediff;
	jagint minSrvCli = servTZDiffMin - defTZDiffMin;

    time_t secs = n / 1000000000; // convert nanoseconds to seconds
	secs -=  minSrvCli * 60;

    struct tm  timeinfo;

	jag_localtime_r( &secs, &timeinfo );

    char tbuf[32];
    strftime(tbuf, 32, "%Y-%m-%d %H:%M:%S", &timeinfo);
    sprintf(tbuf + 19, ".%06lu", n % 1000000000); // add nanoseconds 
    outstr = tbuf;
}

// n is milliseconds time since epoch, outstr is string adjusted to client time zone
void JagTime::getStrFromMilliSec( const JagParseAttribute &jpa, unsigned long n, Jstr &outstr)
{
	jagint defTZDiffMin = jpa.timediff;
	jagint servTZDiffMin = jpa.servtimediff;
	jagint minSrvCli = servTZDiffMin - defTZDiffMin;

    time_t secs = n / 1000; // convert milliseconds to seconds
	secs -=  minSrvCli * 60;

    struct tm  timeinfo;

	jag_localtime_r( &secs, &timeinfo );

    char tbuf[32];
    strftime(tbuf, 32, "%Y-%m-%d %H:%M:%S", &timeinfo);
    sprintf(tbuf + 19, ".%06lu", n % 1000); // add milliseconds 
    outstr = tbuf;
}

// n is seconds time since epoch, outstr is string adjusted to client time zone
void JagTime::getStrFromSec( const JagParseAttribute &jpa, unsigned long secs, Jstr &outstr)
{
	jagint defTZDiffMin = jpa.timediff;
	jagint servTZDiffMin = jpa.servtimediff;
	jagint minSrvCli = servTZDiffMin - defTZDiffMin;

	secs -=  minSrvCli * 60;

    struct tm  timeinfo;

	jag_localtime_r( (time_t*)&secs, &timeinfo );

    char tbuf[32];
    strftime(tbuf, 32, "%Y-%m-%d %H:%M:%S", &timeinfo);
    outstr = tbuf;
}

void JagTime::getStrFromEpochTime(const JagParseAttribute &jpa, unsigned long n, const Jstr &type, Jstr &outstr )
{
    if ( type == JAG_C_COL_TYPE_DATETIMEMICRO || type == JAG_C_COL_TYPE_TIMESTAMPMICRO ) {
        getStrFromMicroSec( jpa, n, outstr);
    } else if ( type == JAG_C_COL_TYPE_DATETIMENANO || type == JAG_C_COL_TYPE_TIMESTAMPNANO ) {
        getStrFromNanoSec( jpa, n, outstr);
    } else if ( type == JAG_C_COL_TYPE_DATETIMESEC || type == JAG_C_COL_TYPE_TIMESTAMPSEC ) {
        getStrFromSec( jpa, n, outstr);
    } else if ( type == JAG_C_COL_TYPE_DATETIMEMILLI || type == JAG_C_COL_TYPE_TIMESTAMPMILLI ) {
        getStrFromMilliSec( jpa, n, outstr);
    }
}

// 1: "yyyy-mm-dd hh:mm:ss.cccc" or 2: "yyyy-mm-ddThh:mm:ss.cccc"
//     0   4  7
// 0: not in above format
int JagTime::isDateTimeFormat( const Jstr &s )
{
    if ( s.size() < 19 ) return 0;

    if (s[4] == '-' && s[7] == '-' && s[13] == ':' && s[16] == ':' ) {
        if ( s[10] == ' ' ) {
            return 1;
        } else if ( s[10] == 'T' ) {
            return 2;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}

// 1: "yyyy-mm-dd *" or 2: "hh:mm:ss.cccc"
//     0   4  7
// 0: not in above format
int JagTime::isDateOrTimeFormat( const Jstr &s )
{
    if ( s.size() < 8 ) return 0;

    if (s[4] == '-' && s[7] == '-' ) return 1;
    if (s[2] == ':' && s[5] == ':' ) return 2;
    return 0;
}
