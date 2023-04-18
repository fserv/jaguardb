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
#ifndef _JagKeyValProp_h_
#define _JagKeyValProp_h_

#include <abax.h>

// max chars of dbname, tabname. colname
#define JAG_NAME_MAX     64

class JagKeyValProp {
  public:
	char    dbname[JAG_NAME_MAX];
	char    tabname[JAG_NAME_MAX];
	char    colname[JAG_NAME_MAX];
	char    type[4];
	int 	offset;
	int 	length;
	int  	sig;
	bool    iskey;
	bool    isrollup;

	JagKeyValProp(){ memset(type, 0, 4 ); }
	JagKeyValProp( const JagKeyValProp &s2 ) 
    {
		copyData( s2 );
	}

	JagKeyValProp& operator=( const JagKeyValProp &s2 ) 
    {
		if ( colname == s2.colname ) return *this;
		copyData( s2 );
		return * this;
	}

	void copyData( const JagKeyValProp &s2 ) 
    {
		strcpy( dbname, s2.dbname );
		strcpy( tabname, s2.tabname );
		strcpy( colname, s2.colname );

		offset = s2.offset;
		length = s2.length;
		strcpy(type, s2.type);
		sig = s2.sig;
		iskey = s2.iskey;
		isrollup = s2.isrollup;
	}
};

#endif
