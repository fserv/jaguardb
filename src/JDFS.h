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
#ifndef _JDFS_H_
#define _JDFS_H_

#include <abax.h>

class JagDBServer;
class JagFSMgr;
class JagCompFile;
class JagDiskArrayFamily;

class JDFS
{
	public:
		//JDFS( JagDBServer *servobj, JagDiskArrayFamily *fam, const Jstr &fpath, int klen, int vlen, jagint arrlen=0 );
		JDFS( JagDBServer *servobj, JagDiskArrayFamily *fam, const Jstr &fpath, int klen, int vlen );
		~JDFS();
		JagCompFile *getCompf();
		//jagint getFileSize() const;
		int exist() const;
		JagCompFile *open();
		int close();
		int rename( const Jstr &newpath );
		int remove();
		int fallocate( jagint offset, jagint len );
		jagint getArrayLength() const;
		jagint pread( char *buf, size_t len, size_t offset ) const; 
		jagint pwrite( const char *buf, size_t len, size_t offset ); 
		Jstr  _fpath;

  protected:
  		int          _klen;  
  		int          _vlen;  
  		int          _kvlen;
		jagint      _stripeSize;
		JagDBServer  *_servobj;
		JagDiskArrayFamily *_family;
		JagFSMgr      *_jdfsMgr;
};

#endif
