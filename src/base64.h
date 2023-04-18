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
#ifndef _base64_encode_decode_h_
#define _base64_encode_decode_h_

#include <abax.h>

Jstr abaxEncodeBase64( const Jstr &src );
Jstr abaxDecodeBase64(Jstr const& encstr);

Jstr abax_base64_encode(unsigned char const* rawstr, unsigned int inlen) ;

int base64EncodeFile2String( FILE *infile, Jstr &outStr );
int base64DecodeFile2String( FILE *infile, Jstr &outStr );

#endif

