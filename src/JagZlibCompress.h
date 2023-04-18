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
#ifndef _zlib_compress_h_
#define _zlib_compress_h_

#include <string>
#include <abax.h>

#ifdef _WINDOWS64_
class JagZlibCompress
{
  public:
	static  Jstr compress(const Jstr& str );
	static  Jstr uncompress(const Jstr& str);
};
#else
#include <zlib.h>
class JagZlibCompress
{
  public:
	static  Jstr compress(const Jstr& str, 
			int compressionlevel = Z_BEST_COMPRESSION );

	static  Jstr uncompress(const Jstr& str);

};
#endif


#endif
