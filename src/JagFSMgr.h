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
#ifndef _JDFS_MGR_H_
#define _JDFS_MGR_H_

#include <abax.h>
#include <JagHashMap.h>
#include <JagCompFile.h>

// manages all files
class JagFSMgr
{
	public:
		JagFSMgr();
		~JagFSMgr();

		JagCompFile *openf(  JagDiskArrayFamily *fam, const AbaxString &fpath, jagint klen, jagint vlen, bool force=false );
		int 	     openfd(  const AbaxString &fpath, bool force=false );
		int  		 closef( const AbaxString &fpath );
		int  		 closefd( const AbaxString &fpath );
		JagCompFile *getCompf( const AbaxString &fpath );
		int 		getFileDesc( const AbaxString &fpath );
		int  	rename( const AbaxString &fpath, const AbaxString &newfpath );
		int  	remove( const AbaxString &fpath );
		bool 	exist( const AbaxString &fpath );
		jagint 	getStripeSize( const AbaxString &fpath, size_t kvlen );
		jagint 	getFileSize( const AbaxString &fpath, size_t kvlen );

		jagint  pread( const JagCompFile *compf, void *buf, size_t count, jagint offset);
		jagint  pwrite( JagCompFile *compf, const void *buf, size_t count, jagint offset);

		jagint  pread( int fd, void *buf, size_t count, jagint offset);
		jagint  pwrite( int fd, const void *buf, size_t count, jagint offset);


  protected:

		JagHashMap<AbaxString,AbaxBuffer> *_map;
};

#endif

