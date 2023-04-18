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
#ifndef _jag_key_column_h_
#define _jag_key_column_h_

#include <abax.h>
#include <JagDef.h>

class JagSchemaRecord;

class JagColumn
{
	public:
		JagColumn();
		JagColumn(const JagColumn & other);
		~JagColumn();
		void destroy( AbaxDestroyAction action = ABAX_NOOP );

		JagColumn& operator=( const JagColumn& other );

		AbaxString 	   name;
		Jstr 		   type;
		unsigned int   offset;
		unsigned int   length;
		unsigned int   sig;
		char		   spare[JAG_SCHEMA_SPARE_LEN+1];
		bool           iskey;
		bool           issubcol;
		bool	       isrollup;
		int			   func;

		int			    srid;
		int				begincol;
		int				endcol;
		int				metrics;
		//int				dummy1; replaced by metrics
		//int				dummy2;
		Jstr			rollupWhere;
		int				dummy3;
		int				dummy4;
		int				dummy5;
		int				dummy6;
		int				dummy7;
		int				dummy8;
		int				dummy9;
		int				dummy10;

		static JagColumn NULLVALUE;
		void copyData(const JagColumn & other);

};

#endif
