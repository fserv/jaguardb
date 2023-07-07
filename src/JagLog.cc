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
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdarg.h>

#include <JagLog.h>

int JAG_LOG_LEVEL;

void log(bool line, const char * format, va_list args )
{
    char tmstr[48];
    struct tm result;
    struct timeval now;
    gettimeofday( &now, NULL );
    time_t tsec = now.tv_sec;
    int ms = now.tv_usec / 1000;
    gmtime_r( &tsec, &result );
    strftime( tmstr, sizeof(tmstr), "%Y-%m-%d %H:%M:%S", &result );
    char msb[5];
    sprintf(msb, ".%03d", ms);
    strcat( tmstr, msb );

    //g_log_mutex.lock();
    fprintf(stdout, "%s %d %ld: ", tmstr, getpid(), pthread_self()%10000 );
    vfprintf(stdout, format, args);
	if ( line ) {
    	fprintf(stdout, "\n");
	}
    fflush(stdout);
    //g_log_mutex.unlock();
}

void i(const char * format, ...)
{
    va_list args;
    va_start(args, format);
    log(false, format, args);
    va_end(args);
}

void in(const char * format, ...)
{
    va_list args;
    va_start(args, format);
    log(true, format, args);
    va_end(args);
}

void d(const char * format, ...)
{
	#ifdef DEBUG_PRINT
    va_list args;
    va_start(args, format);
    log(false, format, args);
    va_end(args);
	#endif
}

void dn(const char * format, ...)
{
	#ifdef DEBUG_PRINT
    va_list args;
    va_start(args, format);
    log(true, format, args);
    va_end(args);
	#endif
}

void dnlock(const char * format, ...)
{
	#ifdef DEBUG_PRINT
	#ifdef DEBUG_LOCK
    va_list args;
    va_start(args, format);
    log(true, format, args);
    va_end(args);
	#endif
	#endif
}

void dnl(const char *m)
{
	#ifdef DEBUG_PRINT
	dn("%s\n", m);
	#endif
}

void jd(int level, const char * format, ...)
{
    if ( JAG_LOG_LEVEL < 1 ) JAG_LOG_LEVEL = 1;
    if ( level > JAG_LOG_LEVEL ) { return; }

    va_list args;
    va_start(args, format);
    jdflog(stdout, level, format, args);
    va_end(args);
}

void jdf(FILE *outf, int level, const char * format, ...)
{
    if ( JAG_LOG_LEVEL < 1 ) JAG_LOG_LEVEL = 1;
    if ( level > JAG_LOG_LEVEL ) { return; }

    va_list args;
    va_start(args, format);
    jdflog(outf, level, format, args);
    va_end(args);
}

void jdflog( FILE *outf, int level, const char *fmt, va_list args )
{
    char        tstr[22];
    memset(tstr, 0, 22);
    time_t  now = time(NULL);
    struct tm  *tmp;
    struct tm  result;
    tmp = localtime_r( &now, &result );
    strftime( tstr, 22, "%Y-%m-%d %H:%M:%S", tmp );
    pthread_t tid = THID;
    //fprintf( outf, "%s 0x%0lx=%lu ", tstr, tid, tid );
    fprintf( outf, "%s %d %ld ", tstr, getpid(), tid%10000 );

    vfprintf(outf, fmt, args);
    fflush( outf );
}
