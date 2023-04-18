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

#include <JagIndexString.h>
#include <JagUtil.h>

JagIndexString::JagIndexString()
{
	init();
}

void JagIndexString::init()
{
	_length = 0;
	_capacity = 32;
	_tokens = 0;
	_lasti = -1;
	_buf = (char*)jagmalloc( _capacity );
	memset( _buf, 0, _capacity );
}

JagIndexString::~JagIndexString()
{
	destroy();
}

void JagIndexString::reset()
{
	destroy();
	init();
}

void JagIndexString::destroy()
{
	if ( _buf ) {
		free (_buf);
	}
	_buf = NULL;
	_length = 0;
	_capacity = 0;
	_tokens = 0;
	_lasti = -1;
}

//  "[10digit length of key]kkkkkkk000kkkkkk000N0F0dddddddd|...|...|"
//  "[10digit length of key]kkkkkkk000kkkkkk000N1F1dddddddd|...|...|"
void JagIndexString::add( const JagFixString &key, jagint i, int isnew, int force )
{
	if ( i != -1 && i == _lasti ) {
		return;
	}

	// key is fixed string and might have \0 in it
	sprintf( _tmp, "N%1dF%1d%lld|", isnew, force, i );
	_tmplen = strlen( _tmp );
	_tokenLen = 10 + key.length() + _tmplen;

	if ( _capacity <=  (_length + _tokenLen + 1) ) {
		_capacity = 2*(_capacity+_tokenLen);
		_buf = (char*)realloc( (void*)_buf, _capacity );
		memset(_buf+_length, 0, _capacity-_length);
	}

	sprintf( _tmp2, "%010lld", key.length() );
	if ( _length < 1 ) {
		memcpy( _buf, _tmp2, 10);
		memcpy( _buf+10, key.c_str(), key.length() );
		memcpy( _buf+10+key.length(), _tmp, _tmplen );
		_length = _tokenLen;
	} else {
		// for now, use i, isnew force = 0 to represent table part
		if ( i == 0 && isnew == 0 && force == 0 ) {
			*(_buf+_length-1) = '~';
		}
		memcpy( _buf+_length, _tmp2, 10);
		memcpy( _buf+_length+10, key.c_str(), key.length() );
		memcpy( _buf+_length+10+key.length(), _tmp, _tmplen );
		_length += _tokenLen;
	}
	++ _tokens;
	_lasti = i;

}
