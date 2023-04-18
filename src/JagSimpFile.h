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
#ifndef _jag_disk_simpfile_h_
#define _jag_disk_simpfile_h_

#include <abax.h>
#include <JagDBMap.h>
#include <JagGapVector.h>
#include <JagFixBlock.h>

class JagReadWriteLock;
class JagCompFile;
class JagDBPair;

class JagSimpFile
{
  public:
	JagSimpFile( JagCompFile *compf,  const Jstr &path, jagint KLEN, jagint VLEN );
	~JagSimpFile();
	jagint  pread( char *buf, jagint localOffset, jagint nbytes ) const; 
	jagint  pwrite( const char *buf, jagint localOffset, jagint nbytes ); 
	int     seekTo( jagint pos);
	void    removeFile();
	void    renameTo( const Jstr &newName );
	jagint  size() const { return _length; }
	jagint  mergeSegment( const JagMergeSeg& seg );
	int     getNextMemDarrMergePair( const JagDBMap *pairmap, JagDBPair &ppair, char *kvbuf, char *dbuf,
	                             JagSingleBuffReader &dbr, JagFixMapIterator &iter,
		                         int &dgoNext, int &mgoNext );

	int     getMinKeyBuf( char *buf ) const;
	int     getMaxKeyBuf( char *buf ) const;
	void    insertMergeUpdateBlockIndex( char *kvbuf, jagint ipos, jagint &lastBlock );
	void    _getPair( char buffer[], int keylength, int vallength, JagDBPair &pair, bool keyonly ) const;
	void    buildInitIndex( bool force );
	int     buildInitIndexFromIdxFile();
	void    flushBlockIndexToDisk();
	void    removeBlockIndexIndDisk();
	void    flushBufferToNewFile( const JagDBMap *pairmap );
	int     removePair( const JagDBPair &pair );
	int     updatePair( const JagDBPair &pair );
	int     exist( const JagDBPair &pair, JagDBPair &retpair );
	bool    getFirstLast( const JagDBPair &pair, jagint &first, jagint &last );
	bool    findPred( const JagDBPair &pair, jagint *index, jagint first, jagint last, JagDBPair &retpair, char *diskbuf );
	jagint  getPartElements(jagint) const;
	void    print();


	jagint      _KLEN;
	jagint      _VLEN;
	jagint      _KVLEN;
	Jstr 		_fpath;
	Jstr 		_fname;
	jagint      _length;  // size of file, in bytes
	int			_fd;
	jagint      _elements; // number of records
	jagint      _minindex;
	jagint      _maxindex;

	void        _open();
	void        close();
	JagFixBlock *_blockIndex;
	bool        _doneIndex;
	JagCompFile *_compf;
	char        *_nullbuf;

};

#endif
