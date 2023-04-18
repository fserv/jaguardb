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

#ifndef _jag_base254_h_
#define _jag_base254_h_

#include <stdlib.h>
#include <JagMath.h>
#include <AbaxCStr.h>

class JagBase254
{
  public:
    JagBase254(const char *p, int len)
    {
        _offset = p;
        _length = len;
    }

    long tol() const
    {
        Jstr norm;
        JagMath::fromBase254Len( norm, _offset, _length );
        return norm.toLong();
    }

    double tod() const
    {
        Jstr norm;
        JagMath::fromBase254Len( norm, _offset, _length );
        return norm.tof();
    }

    long double told() const
    {
        Jstr norm;
        JagMath::fromBase254Len( norm, _offset, _length );
        return norm.toLongDouble();
    }

    const char *_offset;
    int        _length;
};


#endif
