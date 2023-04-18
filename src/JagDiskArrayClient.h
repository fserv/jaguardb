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
#ifndef _jag_disk_arr_client_h_
#define _jag_disk_arr_client_h_

#include <JagDiskArrayBase.h>

class JagDiskArrayClient : public JagDiskArrayBase
{
	public:
	
		JagDiskArrayClient( const Jstr &fpathname, const JagSchemaRecord *record, bool dropClean=false, jagint length=32 );
		virtual ~JagDiskArrayClient();

		virtual void drop();
		virtual void buildInitIndex( bool force=false );	
		virtual void init( jagint length, bool buildBlockIndex );
		// virtual int _insertData( JagDBPair &pair, jagint *retindex, int &insertCode, bool doFirstRedist, JagDBPair &retpair );
		// virtual int _insertData( JagDBPair &pair, int &insertCode, bool doFirstRedist, JagDBPair &retpair );
		//virtual int reSize( bool force=false, jagint newarrlen=-1 );
		//virtual void reSizeLocal( );
		//int needResize();
		jagint flushInsertBufferSync();
		int _dropClean;

};

#endif
