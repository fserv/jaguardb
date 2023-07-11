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

#include <JagFastCompress.h>
#include <JagUtil.h>
#include <snappy.h>

void JagFastCompress::compress(const Jstr& in, Jstr &out )
{
	if ( in.size() < 1 ) { out=""; return; }
	// snappy::Compress( in.data(), in.size(), &out );

	size_t needlen;
	if ( in.size() < 10 ) needlen = 8*in.size();
	else if ( in.size() < 100 ) needlen = 3*in.size();
	else needlen = in.size() + in.size()/2;
	char *buf = (char*)jagmalloc( needlen );

	size_t len;
	snappy::RawCompress( in.c_str(), in.size(), buf, &len );
	//out = Jstr( buf, len );
	out = Jstr( buf, len, len );
	free( buf );
}

void JagFastCompress::compress( const char *instr, jagint inlen, Jstr& outstr )
{
	if ( inlen < 1 || ! instr ) { outstr=""; return; }
	// snappy::Compress( instr, inlen, &outstr );

	size_t needlen;
	if ( inlen  < 10 ) needlen = 8*inlen;
	else if ( inlen < 100 ) needlen = 3*inlen;
	else needlen = inlen + inlen/2;
	char *buf = (char*)jagmalloc( needlen );

	size_t len;
	snappy::RawCompress( instr, inlen, buf, &len );
	//outstr = Jstr( buf, len );
	outstr = Jstr( buf, len, len );
	free( buf );
}


void JagFastCompress::uncompress(const Jstr & in, Jstr &out )
{
	if ( in.size() < 1 ) { out=""; return; }
	// snappy::Uncompress( in.data(), in.size(), &out );
	size_t unlen;
	bool rc = snappy::GetUncompressedLength( in.c_str(), in.size(), &unlen );
	if ( ! rc ) { 
		// printf("s0828 error GetUncompressedLength [%s]\n",  in.c_str() );
		out = ""; 
        return; 
	}

	char *buf = (char*)jagmalloc( unlen + 1 );
	memset( buf, 0, unlen + 1 );
	snappy::RawUncompress( in.c_str(), in.size(), buf );
	//out = Jstr(buf, unlen);
	out = Jstr(buf, unlen, unlen );
	free( buf );
}

void JagFastCompress::uncompress( const char *instr, jagint inlen, Jstr& outstr )
{
	if ( inlen < 1 || ! instr ) { outstr=""; return; }
	// snappy::Uncompress( instr, inlen, &outstr );

	size_t unlen;
	bool rc = snappy::GetUncompressedLength( instr, inlen, &unlen );
	if ( ! rc ) { 
        outstr = ""; 
        return; 
    }

	char *buf = (char*)jagmalloc( unlen + 1 );
	memset( buf, 0, unlen + 1 );
	snappy::RawUncompress( instr, inlen, buf );
	//outstr = Jstr(buf, unlen);
	outstr = Jstr(buf, unlen, unlen );
	free( buf );
}

