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
#ifndef _jag_log_h_ 
#define _jag_log_h_ 

#include <pthread.h>
#include <stdarg.h>
#include <abax.h>
#include <JagDef.h>
#include <JagClock.h>
#include <JagLogLevel.h>
#include <time.h>

void i(const char * format, ...);
void in(const char * format, ...);
void d(const char * format, ...);
void dn(const char * format, ...);
void dnl(const char *m = "");
void log(bool line, const char * format, va_list args);
void jd(int level, const char *fmt, ... );
void jdf(FILE *outf, int level, const char *fmt, ... );
void jdflog( FILE *outf, int level, const char *fmt, va_list args );

#endif
