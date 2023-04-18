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
#ifndef _jag_single_buff_writer_h_
#define _jag_single_buff_writer_h_

#include <abax.h>
#include <JagCfg.h>

class JagCompFile;

class JagSingleBuffWriter
{
	public:

		JagSingleBuffWriter( JagCompFile *compf, int keyvallen, jagint bufferSize=-1 ); 
		JagSingleBuffWriter( int fd, int keyvallen, jagint bufferSize=-1 );

		~JagSingleBuffWriter();
		
		// void resetKVLEN( int newkvlen );
		void writeit( jagint pos, const char *keyvalbuf, jagint KEYVALLEN );
		void flushBuffer();

	protected:
		void init( int keyvallen, jagint bufferSize );

		int  _fd;
		JagCompFile *_compf;
		char *_superbuf; 
		jagint  KVLEN;
		jagint _lastSuperBlock;
		jagint _relpos;
		jagint SUPERBLOCKLEN;
		jagint SUPERBLOCK;
};


#endif
