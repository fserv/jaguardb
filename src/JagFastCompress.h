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
#ifndef _snappy_compress_h_
#define _snappy_compress_h_

#include <string>
#include <abax.h>

class JagFastCompress
{
  public:
	static  void compress(const Jstr & instr, Jstr & outstr );
	static  void uncompress(const Jstr &instr, Jstr & outstr );
	static  void compress( const char *instr, jagint inlen, Jstr & outstr );
	static  void uncompress( const char *instr, jagint inlen, Jstr & outstr );
};

#endif
