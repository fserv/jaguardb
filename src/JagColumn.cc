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
#include <JagColumn.h>

JagColumn::JagColumn()
{
	type = JAG_C_COL_TYPE_STR;
	offset = 0;
	length = 0;
	sig = 0;
	func = 0;
	spare[JAG_SCHEMA_SPARE_LEN] = '\0';
	memset(spare, JAG_S_COL_SPARE_DEFAULT, JAG_SCHEMA_SPARE_LEN );
	iskey = 0;
	issubcol = 0;
    isrollup = false;

	srid = begincol = endcol = metrics = 0; dummy3 = dummy4 = dummy5 = 0;
	dummy6 = dummy7 = dummy8 = dummy9 = dummy10 = 0;
}

JagColumn::~JagColumn()
{
}

void JagColumn::destroy( AbaxDestroyAction action )
{
}


JagColumn& JagColumn::operator=( const JagColumn& other )
{
	if ( this == &other ) return *this;
	copyData( other );
	return *this;
}


JagColumn::JagColumn(const JagColumn & other)
{
	copyData( other );
}

void JagColumn::copyData( const JagColumn& other )
{
	name = other.name;
	type = other.type;
	offset = other.offset;
	length = other.length;
	sig = other.sig;
	func = other.func;
	strncpy( spare, other.spare, JAG_SCHEMA_SPARE_LEN );
	if ( spare[0] == JAG_C_COL_KEY ) {
		iskey = 1;
	} else {
		iskey = 0;
	}

	if ( spare[6] == JAG_SUB_COL ) {
		issubcol = 1;
	} else {
		issubcol = 0;
	}

	if ( spare[7] == JAG_ROLL_UP ) {
		isrollup = true;
	} else {
		isrollup = false;
	}

	srid = other.srid;
	begincol = other.begincol;
	endcol = other.endcol;
	metrics = other.metrics;
	rollupWhere = other.rollupWhere;
	dummy3 = other.dummy3;
	dummy4 = other.dummy4;
	dummy5 = other.dummy5;
	dummy6 = other.dummy6;
	dummy7 = other.dummy7;
	dummy8 = other.dummy8;
	dummy9 = other.dummy9;
	dummy10 = other.dummy10;
}

