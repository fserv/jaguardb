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
#ifndef _jag_index_string_h_
#define _jag_index_string_h_

#include <abax.h>

class JagIndexString
{
  public:
	JagIndexString();
	~JagIndexString();

	jagint length() const { return _length; }
	jagint size() const { return _length; }
	jagint capacity() const { return _capacity; }
	jagint tokens() const { return _tokens; }
	const char  *c_str() const { return _buf; }
	void destroy();
	void init();
	void reset();

	void add( const JagFixString &key, jagint i, int isnew, int force );
  	
  protected:
	jagint  _length;
	jagint  _capacity;
	jagint  _tokens;
	char  *_buf;
	jagint  _lasti;

	// temp vars
	char  _tmp[32];
	char  _tmp2[32];
	jagint   _tmplen;
	jagint  _tokenLen;

};

#endif

