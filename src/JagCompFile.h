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
#ifndef _jag_disk_compfile_h_
#define _jag_disk_compfile_h_

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <atomic>
#include <vector>

#include <abax.h>
#include <JagArray.h>
#include <JagDBMap.h>
//#include <JagAllBlockIndex.h>

class JagDBPair;
class JagSimpFile;
class JagDiskArrayFamily;

// offset(byte positon) --> simpfile
typedef AbaxPair<AbaxLong, AbaxBuffer> JagOffsetSimpfPair;

// key --> offset
typedef AbaxPair<JagFixString, AbaxLong> JagKeyOffsetPair;

/***
class JagOffsetObj
{
  public:
  	Jstr         fname;
	JagSimpFile  *simpf;
};
***/

class JagCompFile
{
  public:
	JagCompFile( JagDiskArrayFamily *fam, const Jstr &path, jagint KLEN, jagint VLEN );
	~JagCompFile();
	jagint pread(char *buf, jagint len, jagint offset ) const;
	jagint pwrite(const char *buf, jagint len, jagint offset );
	jagint insert(const char *buf, jagint offset, jagint len );
	jagint remove(jagint offset, jagint len );
	jagint size() const { return _length; }
	int     removeFile();

	jagint _writeFirst(const char *buf, jagint offset, jagint len );
	int 	_getOffSet( jagint anyPosition, jagint &partOffset, jagint &offsetIdx ) const;
	float 	computeMergeCost( const JagDBMap *pairmap, jagint seqReadSpeed, jagint seqWriteSpeed, 
							  JagVector<JagMergeSeg> &vec );
	jagint 	mergeBufferToFile( const JagDBMap *pairmap, const JagVector<JagMergeSeg> &vec );
	void 		buildInitIndex( bool force );
	int  		buildInitIndexFromIdxFile();
	void 		flushBlockIndexToDisk();
	void 		removeBlockIndexIndDisk();
	jagint 		flushBufferToNewSimpFile( const JagDBMap *pairmap );
	void 		getMinKOPair( const JagSimpFile *simpf, jagint offset, JagKeyOffsetPair &kopair );
	void 		makeKOPair( const char *buf, jagint offset, JagKeyOffsetPair &kopair );
	int 		removePair( const JagDBPair &pair );
	int 		updatePair( const JagDBPair &pair );
	int 		exist( const JagDBPair &pair, JagDBPair &retpair );
	bool 		findFirstLast( const JagDBPair &pair, jagint &first, jagint &last ) const;
	jagint 	    getPartElements( jagint pos ) const;
	JagSimpFile *getSimpFile(  const JagDBPair &pair );
	void     	_open();
	void 		refreshAllSimpfileOffsets();
	void 		print();

	jagint         _KLEN;
	jagint         _VLEN;
	jagint         _KVLEN;
	Jstr 		   _pathDir;
	jagint         _length;

	JagArray< JagOffsetSimpfPair > *_offsetMap;
	JagArray< JagKeyOffsetPair > *_keyMap;

	JagDiskArrayFamily  *_family;
};

#endif
