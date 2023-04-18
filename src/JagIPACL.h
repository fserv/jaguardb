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
#ifndef _jag_ipacl_h_
#define _jag_ipacl_h_
#include <stdio.h>
#include <abax.h>
#include <JagHashMap.h>

class JagIPACL 
{
    public: 
        JagIPACL( const Jstr &fpath );
        ~JagIPACL() { destroy(); }
		bool match( const Jstr &ip );
		int  size();
		Jstr _data;
		void refresh( const Jstr &newdata );

    protected:
		JagHashMap<AbaxString, AbaxString> *_map;
		bool readFile( const Jstr &fpath );
		void  destroy();
};

#endif
