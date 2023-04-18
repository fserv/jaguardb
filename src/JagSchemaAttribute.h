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
#ifndef _jag_schmattr_h_
#define _jag_schmattr_h_
#include <JagSchemaRecord.h>

//for one column attributes from schema
class JagSchemaAttribute
{
  public:
	Jstr dbcol;
	Jstr objcol;
	Jstr colname;
	Jstr type;
	unsigned int offset;
	unsigned int length;
	unsigned int sig;
	unsigned int begincol;
	unsigned int endcol;
	unsigned int srid;
	unsigned int metrics;
	bool isUUID;
	bool isFILE;
	bool isKey;
	bool isAscending;
	Jstr defValue;
	JagVector<Jstr> enumList;
	JagSchemaRecord record;
	
	JagSchemaAttribute() {
		offset = length = sig = begincol = endcol = srid = metrics = 0;
		type = JAG_C_COL_TYPE_STR;
		isAscending = isUUID = isFILE = isKey = false;
	}
};

#endif
