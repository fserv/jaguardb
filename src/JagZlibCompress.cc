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
#include <string.h>
#include "JagZlibCompress.h"

#ifdef _WINDOWS64_
Jstr JagZlibCompress:: compress(const Jstr& str )
{
	return str;
}
Jstr JagZlibCompress:: uncompress(const Jstr& str)
{
	return str;
}
#else
Jstr JagZlibCompress:: compress(const Jstr& str, int compressionlevel )
{
    z_stream zs; 
    memset(&zs, 0, sizeof(zs));

    if (deflateInit(&zs, compressionlevel) != Z_OK) {
        // throw(std::runtime_error("deflateInit failed while compressing."));
		return "";
	}

    zs.next_in = (Bytef*)str.data();
    zs.avail_in = str.size();           // set the z_stream's input

    int ret;
    char outbuffer[32768];
    Jstr outstring;

    // retrieve the compressed bytes blockwise
    do {
        zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
        zs.avail_out = sizeof(outbuffer);
        ret = deflate(&zs, Z_FINISH);
        if (outstring.size() < zs.total_out) {
            outstring.append(outbuffer, zs.total_out - outstring.size());
        }
    } while (ret == Z_OK);

    deflateEnd(&zs);
    if (ret != Z_STREAM_END) {          // an error occurred that was not EOF
		return "";
    }

    return outstring;
}

/** Decompress an STL string using zlib and return the original data. */
Jstr JagZlibCompress:: uncompress(const Jstr& str)
{
    z_stream zs; 
    memset(&zs, 0, sizeof(zs));

    if (inflateInit(&zs) != Z_OK) {
		return "";
	}

    zs.next_in = (Bytef*)str.data();
    zs.avail_in = str.size();

    int ret;
    char outbuffer[32768];
    Jstr outstring;

    do {
        zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
        zs.avail_out = sizeof(outbuffer);
        ret = inflate(&zs, 0);
        if (outstring.size() < zs.total_out) {
            outstring.append(outbuffer, zs.total_out - outstring.size());
        }

    } while (ret == Z_OK);

    inflateEnd(&zs);
    if (ret != Z_STREAM_END) {          // an error occurred that was not EOF
		return "";
    }

    return outstring;
}
#endif

