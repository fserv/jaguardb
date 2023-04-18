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
#ifndef _jag_lang_h_
#define _jag_lang_h_

#define JAG_ASCII  2
#define JAG_UTF8   10
#define JAG_GB2312 12
#define JAG_GBK    14

#include <abax.h>
#include <JagVector.h>

class JagLang 
{
  public:
  	JagLang ();
  	~JagLang ();

	jagint parse( const char *instr, const char *ENCODE );
	jagint length();
	jagint size();
	Jstr  at( int i);
	void rangeFixString( int buflen, int start, int len, JagFixString &res );

  protected:
  	JagVector<Jstr> *_vec;

	jagint _parseUTF8( const char *instr );
	jagint _parseGB2312( const char *instr );
	jagint _parseGB18030( const char *instr );
};

#endif
