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
#ifndef _jag_db_logger_h_
#define _jag_db_logger_h_
#include <abax.h>
#include <JagRequest.h>
#include <JagHashMap.h>
class JagDBLogger
{
   public:
   	 JagDBLogger( int dologmsg, int dologerr, int historyDay = 3 );
   	 ~JagDBLogger( );

	 void logmsg( const JagRequest &req, const Jstr &msg,  const Jstr &cmd );
	 void logerr( const JagRequest &req, const Jstr &errmsg, const Jstr &cmd );

  protected:
  	 int _historyDay; 
	 int _dologmsg;
	 int _dologerr;
	 void logit( const JagRequest &req, const Jstr &fpath, const Jstr &hdr, const Jstr &cmd );
	 Jstr  _logDir;
  	 JagHashMap<AbaxString, AbaxBuffer> *_filemap;
  	 JagHashMap<AbaxString, AbaxLong> *_timemap;
};

#endif
