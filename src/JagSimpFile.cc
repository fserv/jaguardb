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
#include <JagGlobalDef.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <abax.h>
#include <JagUtil.h>
#include <JagSimpFile.h>
#include <JagCompFile.h>
#include <JagMutex.h>
#include <JagFileMgr.h>
#include <JagDBMap.h>
#include <JagSingleBuffReader.h>
#include <JagSingleBuffWriter.h>
#include <JagDiskArrayFamily.h>


JagSimpFile::JagSimpFile( JagCompFile *compf,  const Jstr &path, jagint KLEN, jagint VLEN )
{
    dn("s7700033 JagSimpFile ctor path=[%s]", path.s() );

	_compf = compf;
	_KLEN = KLEN;
	_VLEN = VLEN;
	_KVLEN = KLEN + VLEN;
	_fpath = path;
	_fname = JagFileMgr::baseName( _fpath );

	_length = 0;
	_fd = -1;
	_elements = 0;

	_maxindex = 0;
	_minindex = -1;

	_open();

	_doneIndex = false;
	_blockIndex = new JagFixBlock( _KLEN );
	_nullbuf = (char*)jagmalloc(_KVLEN+1);

}

void JagSimpFile::_open()
{
    dn("s717277  JagSimpFile::_open( %s )", _fpath.c_str() );

	_fd = jagopen( _fpath.c_str(), O_CREAT|O_RDWR|JAG_NOATIME, S_IRWXU );
	if ( _fd < 0 ) {
        char t[256];
        char *pe = strerror_r( errno, t, 256);
        if ( pe ) {
            jd(JAG_LOG_LOW, "E9383 Error open file [%s] error=[%s]\n",  _fpath.c_str(), t );
        }
        return;
    }

	struct stat sbuf;
	int rc = fstat( _fd, &sbuf);
	if ( 0 == rc ) {
		_length = sbuf.st_size;
	} else {
		_length = 0;
	}
	
}

void JagSimpFile::close()
{
	::close( _fd );
}

JagSimpFile::~JagSimpFile() 
{
	free( _nullbuf );
}

jagint JagSimpFile::pread( char *buf, jagint localOffset, jagint nbytes ) const
{
	return raysafepread( _fd, buf, nbytes, localOffset );
}

jagint JagSimpFile::pwrite( const char *buf, jagint localOffset, jagint nbytes )
{
	return raysafepwrite( _fd, buf, nbytes, localOffset );
}

void JagSimpFile::removeFile()
{
	::close( _fd );
	jagunlink( _fpath.c_str() );
}


void JagSimpFile::renameTo( const Jstr &newName )
{
	::close( _fd );
	Jstr dirname = JagFileMgr::dirName( _fpath );
	Jstr newfpath = dirname + "/" + newName;
	jagrename( _fpath.c_str(), newfpath.c_str() );
    dn("s0298011 renameTo _fpath=[%s]  --> newfpath=[%s]", _fpath.s(), newfpath.s() );
	_fpath = newfpath;
	_open();
}

int JagSimpFile::seekTo( jagint pos)
{
	if ( ::lseek( _fd, pos, SEEK_SET ) < 0 ) return -1;
	return 0;
}

int JagSimpFile::getMinKeyBuf( char *buf ) const
{
	jagint rc;
	jagint localOffset = 0;
	bool hasData = false;

	while ( true ) {
		rc = JagSimpFile::pread( buf, localOffset, _KLEN );
		if ( rc <= 0 ) {
			break;
		}

		if ( '\0' != buf[0] ) {
			hasData = true;
			break;
		}

		localOffset += _KVLEN;
	}

	if ( hasData ) {
		return 1;
	} else {
		return 0;
	}
}

int JagSimpFile::getMaxKeyBuf( char *buf ) const
{
	jagint rc = 0;
	jagint localOffset = _length - _KVLEN ;  
    // position to get record

	while ( true ) {
		rc = JagSimpFile::pread( buf, localOffset, _KLEN );
		if ( rc <= 0 ) {
            dn("s082001 JagSimpFile::pread rc=%d <= 0 break", rc );
			break;
		}

		if ( '\0' != buf[0] ) {
			rc = 0;
            dn("s082002 buf[0] not null byte rc=0 buf=[%s]", buf);
			break;
		}

		localOffset -= _KVLEN;
		if ( localOffset < 0 ) {
			rc = -10;  
			break;
		}
	}

    dn("s5020034 rc=%d", rc );
	return rc;
}

jagint JagSimpFile::mergeSegment( const JagMergeSeg& seg )
{
    JagFixMapIterator leftIter = seg.leftIter;
    JagFixMapIterator rightIter = seg.rightIter;
    JagFixMapIterator endIter = ++ rightIter;  
    JagFixMapIterator iter;

	Jstr fpath = _fpath + ".merging";
    dn("s8278111 mergeSegment jagopen(%s)", fpath.s() );

	int fd = jagopen( fpath.c_str(), O_CREAT|O_RDWR|JAG_NOATIME, S_IRWXU );
	if ( fd < 0 ) {
	    jd(JAG_LOG_LOW, "E22208 open(%s) to write error\n", fpath.c_str() );
		return -1;
	}

	if ( _blockIndex ) {
		delete _blockIndex;
	}
	_blockIndex = new JagFixBlock( _KLEN );

    JagSingleBuffWriter *sbw = NULL;
    jagint dblimit = 64;
	_elements = 0;
    sbw = new JagSingleBuffWriter( fd, _KVLEN, dblimit );

	char *kvbuf = (char*)jagmalloc(_KVLEN+1);
	char *dbuf = (char*)jagmalloc(_KVLEN+1);
	memset( kvbuf, 0, _KVLEN+1 );
	memset( dbuf, 0, _KVLEN+1 );

	jagint rlimit = getBuffReaderWriterMemorySize( _length/1024/1024 );
	JagSingleBuffReader dbr( (int)_fd, _length/_KVLEN, _KLEN, _VLEN, 0, 0, rlimit );
	JagDBPair tmppair;
	jagint length = 0;
	JagDBPair mpair;
	bool memDone = false;
	bool diskDone = false;

	iter = leftIter;
	int rc;
	jagint wpos = 0;
	jagint lastBlock = -1;
	int drc;

	drc = dbr.getNext( dbuf );
	while ( true ) {
		if ( iter == endIter ) { 
			memDone = true; 
			break; 
		}

		if ( 0 == drc ) { 
			diskDone = true; 
			break; 
		}

		mpair.point( iter->first, iter->second);

		rc = memcmp(dbuf, mpair.key.c_str(), _KLEN );
		if ( rc > 0 ) {
			memcpy( kvbuf, mpair.key.c_str(), _KLEN  );
			memcpy( kvbuf+ _KLEN, mpair.value.c_str(), _VLEN );
			++ iter;
		}  else if ( rc == 0 ) {
			memcpy( kvbuf, mpair.key.c_str(), _KLEN );
			memcpy( kvbuf+ _KLEN, mpair.value.c_str(), _VLEN );
			++ iter;
			drc = dbr.getNext( dbuf );
		} else {
			memcpy( kvbuf, dbuf, _KVLEN );
			drc = dbr.getNext( dbuf );
		}
		
		sbw->writeit( wpos, kvbuf, _KVLEN );
		++ _elements;
		insertMergeUpdateBlockIndex( kvbuf, wpos, lastBlock );
		++ wpos;
		length += _KVLEN;
	}

	if (  memDone && diskDone ) {
	} else if ( memDone ) {
		while ( true ) {
			memcpy( kvbuf, dbuf, _KVLEN );
			sbw->writeit( wpos, kvbuf, _KVLEN );
			++ _elements;
			insertMergeUpdateBlockIndex( kvbuf, wpos, lastBlock );
			wpos += 1;
			length += _KVLEN;
			drc = dbr.getNext( dbuf );
			if ( 0 == drc ) {
				break;
			}
		}
	} else {
		while ( true ) {
			memcpy( kvbuf, mpair.key.c_str(), _KLEN );
			memcpy( kvbuf+ _KLEN, mpair.value.c_str(), _VLEN );
			sbw->writeit( wpos, kvbuf, _KVLEN );
			++ _elements;
			insertMergeUpdateBlockIndex( kvbuf, wpos, lastBlock );
			wpos += 1;
			length += _KVLEN;
			++ iter;
			if ( iter == endIter ) {
				break;
			}
			mpair.point( iter->first, iter->second);
		}
	}

	if ( sbw ) sbw->flushBuffer();

	::close( _fd );
	::close( fd );
	jagrename( fpath.c_str(), _fpath.c_str() );

    dn("s717273 mergeSegment  JagSimpFile::_open( %s )", _fpath.c_str() );
	_fd = jagopen( _fpath.c_str(), O_CREAT|O_RDWR|JAG_NOATIME, S_IRWXU );
    if ( _fd < 0 ) {
	    jd(JAG_LOG_LOW, "E20201 open(%s) to write error\n", _fpath.c_str() );
    }

	_length = length;

	delete sbw;
	free( kvbuf );
	free( dbuf );
    return length;
}

void JagSimpFile::insertMergeUpdateBlockIndex( char *kvbuf, jagint ipos, jagint &lastBlock )
{
    JagDBPair tpair;
    if ( ipos > _maxindex ) _maxindex = ipos;
    if ( ipos < _minindex ) _minindex = ipos;

    if ( ( ipos/JagCfg::_BLOCK) != lastBlock ) {
        _getPair( kvbuf, _KLEN, _VLEN, tpair, true );
        _blockIndex->updateIndex( tpair, ipos, true );
        _blockIndex->updateCounter( ipos, 0, true, false );
    }

    _blockIndex->updateCounter( ipos, 1, false, false );
    lastBlock = ipos/JagCfg::_BLOCK;
}

void JagSimpFile::_getPair( char buffer[], int KLENgth, int VLENgth, JagDBPair &pair, bool keyonly ) const
{
    if (buffer[0] != '\0') {
        if ( keyonly ) {
            pair = JagDBPair( buffer, KLENgth );
        } else {
            pair = JagDBPair( buffer, KLENgth, buffer+KLENgth, VLENgth );
        }
    } else {
        JagDBPair temp_pair;
        pair = temp_pair;
    }
}

void JagSimpFile::buildInitIndex( bool force )
{
	if ( ! force ) {
		int rc = buildInitIndexFromIdxFile();
		if ( rc ) {
			jagmalloc_trim( 0 );
			return;
		}
	}

	if ( !force && _doneIndex ){
		d("s3820 in buildInitIndex return here\n");
		return;
	}

	JagFixBlock *newblockIndex = new JagFixBlock( _KLEN );
	
	_elements = 0;
	JagDBPair tpair;
	jagint ipos, lastBlock = -1;
   	char *keyvalbuf = (char*)jagmalloc( _KVLEN+1);
	memset( keyvalbuf, 0,  _KVLEN + 1 );
	
	jagint rlimit = getBuffReaderWriterMemorySize( _length/1024/1024 );

    dn("s503737 JagSimpFile::buildInitIndex buildInitIndexFromIdxFile error, not read self _fd ...");
	JagSingleBuffReader nav( _fd, _length/_KVLEN, _KLEN, _VLEN, 0, 0, rlimit );
	_minindex = -1;

	while ( nav.getNext( keyvalbuf, _KVLEN, ipos ) ) { 
		++ _elements;
		_maxindex = ipos;
		if ( _minindex < 0 ) _minindex = ipos;
		_getPair( keyvalbuf, _KLEN, _VLEN, tpair, true );
		newblockIndex->updateMaxKey( tpair, false );

		if ( ( ipos/JagCfg::_BLOCK) != lastBlock ) {
			newblockIndex->updateIndex( tpair, ipos, false, false );
		}
		newblockIndex->updateCounter( ipos, 1, false, false );
		
		lastBlock = ipos/JagCfg::_BLOCK;
	}

    dn("s029111 JagSingleBuffReader read _elements=[%d] items. setup _blockIndex", _elements );

	if ( _blockIndex ) delete _blockIndex;
	_blockIndex = newblockIndex;
	newblockIndex = NULL;
	_doneIndex = 1;
	free( keyvalbuf );
	jagmalloc_trim( 0 );
}

int JagSimpFile::buildInitIndexFromIdxFile()
{
	if ( _doneIndex ){
		d("s3822 in buildInitIndexFromIdxFile return here\n");
		return 1;
	}

	Jstr idxPath = _fpath + ".bid";
    dn("s766399 buildInitIndexFromIdxFile() jagopen=[%s]", idxPath.c_str() );

	struct stat sbuf;
	stat(idxPath.c_str(), &sbuf);
	if ( sbuf.st_size < 1 ) {
		jagunlink( idxPath.c_str() );
		return 0;
	}

	int fd = jagopen((char *)idxPath.c_str(), O_RDONLY|JAG_NOATIME );
	if ( fd < 0 ) {
        dn("s200281 JagSimpFile::buildInitIndexFromIdxFile .bid file [%s] not found, return 0", idxPath.c_str() );
		return 0;
	}

	char *buf = (char*)jagmalloc(JAG_BID_FILE_HEADER_BYTES+2*_KLEN+1);
	char c;
	memset( buf, 0, JAG_BID_FILE_HEADER_BYTES+2*_KLEN+1 );

	raysaferead( fd, buf, 1 ); // .bid file
	if ( buf[0] != '0' ) {
		jagclose( fd );
		free( buf );
		jagunlink( idxPath.c_str() );
		return 0;
	}

	raysaferead( fd, buf, JAG_BID_FILE_HEADER_BYTES+2*_KLEN );
	c = buf[16];
	buf[16] = '\0';
	_elements = atoll(buf);
	buf[16] = c;

	c = buf[32];
	buf[32] = '\0';
	buf[32] = c; 

	c = buf[48];
	buf[48] = '\0';
	_minindex = atoll( buf +32 );
	buf[48] = c; 

	c = buf[64];
	buf[64] = '\0';
	_maxindex = atoll( buf +48 );
	buf[64] = c; 

	JagFixBlock *newblockIndex = new JagFixBlock( _KLEN );

   	char maxKeyBuf[ _KLEN+1 ];
	memset( maxKeyBuf, 0,  _KLEN + 1 );
	memcpy( maxKeyBuf, buf+JAG_BID_FILE_HEADER_BYTES+_KLEN, _KLEN ); 
	JagDBPair maxPair;
	_getPair( maxKeyBuf, _KLEN, 0, maxPair, true );
	newblockIndex->updateMaxKey( maxPair, false );
	
	JagDBPair tpair;
   	char keybuf[ _KLEN+2];
	memset( keybuf, 0,  _KLEN + 2 );
	
	jagint rlimit = getBuffReaderWriterMemorySize( _length/1024/1024 );
	JagSingleBuffReader br( fd, (sbuf.st_size-1-JAG_BID_FILE_HEADER_BYTES-2*_KLEN)/(_KLEN+1), _KLEN, 1, 0, 
							1+JAG_BID_FILE_HEADER_BYTES+2*_KLEN, rlimit );
	jagint cnt = 0; 
	jd(JAG_LOG_LOW, "begin reading bid file ...\n" );
	jagint i;

	while( br.getNext( keybuf, i ) ) {	
		++cnt;
		_getPair( keybuf, _KLEN, 0, tpair, true );
		newblockIndex->updateIndex( tpair, i*JagCfg::_BLOCK, false, false );
		newblockIndex->updateCounter( i*JagCfg::_BLOCK, keybuf[_KLEN], true, false );
	}

	jd(JAG_LOG_LOW, "done reading bid file %ld records rlimit=%ld\n", cnt, rlimit );
	jagclose( fd );

	if ( _blockIndex ) delete _blockIndex;
	_blockIndex = newblockIndex;
	newblockIndex = NULL;
	_doneIndex = 1;
	free( buf );

	jagunlink( idxPath.c_str() );
	return 1;
}

void JagSimpFile::flushBlockIndexToDisk()
{
    Jstr idxPath = _fpath + ".bid";
    if ( _blockIndex ) {
		jd(JAG_LOG_LOW, "s308123 flushBottomLevel idxPath=%s _elements=%d ...\n", idxPath.s(), _elements );
        _blockIndex->flushBottomLevel( idxPath, _elements, _length/_KVLEN, _minindex, _maxindex );
		jd(JAG_LOG_LOW, "s308123 flushBottomLevel idxPath=%s _elements=%d done\n", idxPath.s(), _elements );
    } else {
		jd(JAG_LOG_LOW, "s308123  _blockIndex is NULL, no flushBottomLevel\n");
	}

}

void JagSimpFile::removeBlockIndexIndDisk()
{
    Jstr idxPath = _fpath + ".bid";
    jagunlink( idxPath.c_str() );
}

void JagSimpFile::flushBufferToNewFile( const JagDBMap *pairmap )
{
	if (  pairmap->elements() < 1 ) { return; }
	jagint cnt = 0;

	JagSingleBuffWriter *sbw = NULL;
	jagint dblimit = 64; 

	char *kvbuf = (char*)jagmalloc( _KVLEN+1);
	memset(kvbuf, 0, _KVLEN+1);

	sbw = new JagSingleBuffWriter( _fd, _KVLEN, dblimit );
	jagint wpos = 0;
	jagint lastBlock = -1;
	JagFixMapIterator it;
	char vbuf[3]; vbuf[2] = '\0';
	int activepos = -1;
	if ( _compf->_family ) {
		activepos = _compf->_family->_darrlist.size();
	}

	for ( it = pairmap->_map->begin(); it != pairmap->_map->end(); ++it ) {
		memcpy( kvbuf, it->first.c_str(), _KLEN );
		memcpy( kvbuf+ _KLEN, it->second.c_str(), _VLEN );
		sbw->writeit( wpos, kvbuf, _KVLEN );
		insertMergeUpdateBlockIndex( kvbuf, wpos, lastBlock );
		++wpos;
		++cnt;

		++_elements;
		_length += _KVLEN;
	}

	if ( sbw ) sbw->flushBuffer();

	free( kvbuf );		
	delete sbw;

	::fsync( _fd );
}

int JagSimpFile::removePair( const JagDBPair &pair )
{
	bool rc;
    jagint first, last;
    JagDBPair retpair;
	rc = getFirstLast( pair, first, last );
	if ( ! rc ) {
		return -99;
	}

	char *diskbuf = (char*)jagmalloc(JAG_BLOCK_SIZE*_KVLEN+1);
	memset( diskbuf, 0, JAG_BLOCK_SIZE* _KVLEN+1 );
	jagint retindex;
	rc = findPred( pair, &retindex, first, last, retpair, diskbuf );
	if ( !rc ) {
		free( diskbuf );
		return 0;
	}

	jagint i, j = retindex;
	JagDBPair npair;
	for ( i = j+1; i <= last; ++i ) {
		if ( *(diskbuf+(i-first)* _KVLEN) != '\0' ) break;
	}

	if ( i <= last ) {
		npair = JagDBPair( diskbuf+(i-first)* _KVLEN, _KLEN, true );
	} else {
		i = j;
	}

	i /= JAG_BLOCK_SIZE;
	_blockIndex->deleteIndex( pair, npair, i, false );
	_blockIndex->updateCounter( j, -1, false );
	if ( _elements > 0 ) --_elements;

	memset( _nullbuf, 0, _KVLEN+1 );
	raysafepwrite( _fd, _nullbuf, _KVLEN, retindex*_KVLEN);
	free( diskbuf );
	return 0;
}

int JagSimpFile::updatePair( const JagDBPair &pair )
{
	bool rc;
    jagint first, last;
    JagDBPair retpair;
	rc = getFirstLast( pair, first, last );
	if ( ! rc ) {
		return -99;
	}

	char *diskbuf = (char*)jagmalloc(JAG_BLOCK_SIZE*_KVLEN+1);
	memset( diskbuf, 0, JAG_BLOCK_SIZE* _KVLEN+1 );
	jagint retindex;
	rc = findPred( pair, &retindex, first, last, retpair, diskbuf );
	if ( !rc ) {
		free( diskbuf );
		return 0;
	}

	raysafepwrite( _fd, pair.value.c_str(), _VLEN, retindex*_KVLEN+_KLEN);
	free( diskbuf );
	return 0;
}

int JagSimpFile::exist( const JagDBPair &pair, JagDBPair &retpair )
{
	bool rc;
    jagint first, last;
	rc = getFirstLast( pair, first, last );
	if ( ! rc ) {
		return -99;
	}

	char *diskbuf = (char*)jagmalloc(JAG_BLOCK_SIZE*_KVLEN+1);
	memset( diskbuf, 0, JAG_BLOCK_SIZE* _KVLEN+1 );
	jagint retindex;
	rc = findPred( pair, &retindex, first, last, retpair, diskbuf );
	if ( !rc ) {
		free( diskbuf );
		return 0;
	}

	retpair.key = pair.key;
	retpair.value = JagFixString( diskbuf+retindex*_KVLEN +_KLEN, _VLEN );
	
	free( diskbuf );
	return 0;
}

bool JagSimpFile::getFirstLast( const JagDBPair &pair, jagint &first, jagint &last )
{
    bool rc;
    rc = _blockIndex->findFirstLast( pair, &first, &last );
    dn("s190123  _blockIndex->findFirstLast rc=%d", rc );

    if ( ! rc ) {
        dn("s70032 JagSimpFile::getFirstLast not rc, return false !!!!!");
        return false;
    }

    last = first + JagCfg::_BLOCK - 1;
    dn("s88772200 JagSimpFile::getFirstLast first=%ld last=%ld return true", first, last );
    return true;
}

jagint JagSimpFile::getPartElements(jagint pos) const
{
	return _blockIndex->getPartElements( pos );
}


bool JagSimpFile::findPred( const JagDBPair &pair, jagint *index, jagint first, jagint last, JagDBPair &retpair, char *diskbuf )
{
	bool found = 0;

	*index = -1;
    JagDBPair arr[JagCfg::_BLOCK];
    JagFixString key, val;

   	raysafepread( _fd, diskbuf, (JagCfg::_BLOCK)*_KVLEN, first*_KVLEN);

   	for (int i = 0; i < JagCfg::_BLOCK; ++i ) {
   		key.point( diskbuf+i*_KVLEN, _KLEN );
   		val.point( diskbuf+i*_KVLEN+_KLEN, _VLEN );
   		arr[i].point( key, val );
   	}

	found = binSearchPred( pair, index, arr, JagCfg::_BLOCK, 0, JagCfg::_BLOCK-1 );
	if ( *index != -1 ) retpair = arr[*index];
	else retpair = arr[0];

	*index += first;
	return found;        
}

void JagSimpFile::print()
{
	i("s2032929 JagSimpFile:\n");
	i("_fpath=[%s]\n", _fpath.s() );
	i("_length=[%d]\n", _length );
	i("_elements=[%d]\n", _elements);
	i("_KVLEN=[%d]\n", _KVLEN);
	i("_length/_KVEN=[%d]\n", _length/_KVLEN);

   	char *keyvalbuf = (char*)jagmalloc( _KVLEN+1);
	memset( keyvalbuf, 0,  _KVLEN + 1 );
	
	jagint ipos;
	jagint elements = 0;
	jagint rlimit = 1;
	JagSingleBuffReader nav( _fd, _length/_KVLEN, _KLEN, _VLEN, 0, 0, rlimit );
	_minindex = -1;

	while ( nav.getNext( keyvalbuf, _KVLEN, ipos ) ) { 
		i("i=%04d pos=%04d [%s][%s]\n", elements, ipos, keyvalbuf, keyvalbuf+_KLEN );
		++ elements;
	}

	free( keyvalbuf );
	i("\n");
}
