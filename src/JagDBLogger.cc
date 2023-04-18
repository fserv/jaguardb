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

#include  <JagDBLogger.h>
#include  <JagUtil.h>

JagDBLogger::JagDBLogger( int dologmsg, int dologerr, int historyDay )
{
	_dologmsg = dologmsg;
	_dologerr = dologerr;
	if ( _dologmsg || dologerr ) {
		_historyDay = historyDay;
		_filemap = new JagHashMap<AbaxString, AbaxBuffer> ();
		_timemap = new JagHashMap<AbaxString, AbaxLong> ();
		_logDir = jaguarHome() + "/log/";
	}
}

JagDBLogger::~JagDBLogger()
{
	if ( ! _dologmsg && ! _dologerr ) return;

	FILE * fp = NULL;
	const AbaxPair<AbaxString, AbaxBuffer> * arr = _filemap->array();
	jagint len = _filemap->arrayLength();
	for ( int i = 0; i < len; ++i ) {
		if ( _filemap->isNull(i)  ) continue;
		const AbaxPair<AbaxString, AbaxBuffer> &pair = arr[i];
		fp = (FILE*)pair.value.value();
		if ( fp ) jagfclose( fp );
	}

	delete _filemap;
	delete _timemap;
}

void JagDBLogger::logmsg( const JagRequest &req, const Jstr &hdr, const Jstr &cmd )
{
	if ( ! _dologmsg ) return;
	Jstr fpath = _logDir + req.session->dbname + ".log";
	logit( req, fpath, hdr, cmd );
}

void JagDBLogger::logerr( const JagRequest &req, const Jstr &hdr, const Jstr &cmd )
{
	if ( ! _dologerr ) return;
	Jstr fpath = _logDir + req.session->dbname + ".err";
	logit( req, fpath, hdr, cmd );
}

void JagDBLogger::logit( const JagRequest &req, const Jstr &fpath, 
						 const Jstr &hdr, const Jstr &cmd )
{
	FILE *fp = NULL;
	AbaxBuffer bfr;
	time_t nowt = time(NULL);
	if ( ! _filemap->getValue ( fpath, bfr ) ) {
		fp = jagfopen( fpath.c_str(), "a" );
		if ( fp ) {
			_filemap->addKeyValue( fpath, fp );
			_timemap->addKeyValue( fpath, nowt );
		}
	} else {
		fp = (FILE*)bfr.value();
		AbaxLong st;
		_timemap->getValue ( fpath, st );
		if ( ( nowt - st.value() ) > _historyDay * 86400 ) {
			jagunlink( fpath.c_str() );
			_filemap->removeKey( fpath );
			_timemap->removeKey( fpath );
		}
	}

	if ( fp ) {
		jdf( fp, JAG_LOG_LOW, "[%s] %s\n",  hdr.c_str(), cmd.c_str() );
	}
}
