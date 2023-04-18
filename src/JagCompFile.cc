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
#include <dirent.h>
#include <sys/sendfile.h>
#include <JagSimpFile.h>
#include <JagCompFile.h>
#include <JagFileMgr.h>
#include <JagFileName.h>
#include <JagSortLinePoints.h>
#include <JagDBMap.h>
#include <JagDiskArrayFamily.h>
#include <JagUtil.h>

JagCompFile::JagCompFile( JagDiskArrayFamily *fam, const Jstr &pathDir, jagint KLEN, jagint VLEN )
{
	_KLEN = KLEN;
	_VLEN = VLEN;
	_KVLEN = KLEN + VLEN;
	_pathDir = pathDir;
	_length = 0;
	_family = fam;

	d("s02839 JagCompFile ctor _pathDir=[%s] KLEN=%d VLEN=%d\n", _pathDir.c_str(), KLEN, VLEN );

	_offsetMap = new JagArray< JagOffsetSimpfPair >();
	_offsetMap->useHash( true );

	_keyMap = new JagArray< JagKeyOffsetPair >();

	_open();
}

void  JagCompFile::_open()
{
	JagFileMgr::makedirPath( _pathDir, 0700 );

	DIR             *dp;
	struct dirent   *dirp;
	if ( NULL == (dp=opendir( _pathDir.c_str())) ) {
		d("s921710 error opendir [%s]\n", _pathDir.c_str() );
		return;
	}

	JagVector<JagFileName> vec;
	while( NULL != (dirp=readdir(dp)) ) {
		if ( 0==strcmp(dirp->d_name, ".") || 0==strcmp(dirp->d_name, "..") ) {
			continue;
		}

		if ( 0==strcmp(dirp->d_name, "files" ) ) { continue; }
		if ( strstr(dirp->d_name, ".bid" ) ) { continue; }
		d("s30247 simpfile vec.append(%s)\n", dirp->d_name );
		vec.push_back( JagFileName(dirp->d_name) );
	}

	const JagFileName *arr = vec.array();
	jagint len = vec.size();
	if ( len > 0 ) {
		inlineQuickSort<JagFileName>( (JagFileName*)arr, len );
	}
	d("s12228 simpf vec.len=%d\n", len );

	Jstr    fpath;
	jagint  offset = 0;
	JagKeyOffsetPair keyOffsetPair;
	char    minkbuf[_KLEN+1];
	int     rc;

	for ( int i=0; i < len; ++i ) {
		fpath = _pathDir + "/" + vec[i].name;
		d("s93110 fpath=[%s] _offset=%ld\n", fpath.c_str(), offset );

		JagSimpFile *simpf = new JagSimpFile( this, fpath, _KLEN, _VLEN );
		JagOffsetSimpfPair pair( offset, AbaxBuffer(simpf) );
		d("s33039 insert JagOffsetSimpfPair offset=%ld simpf=%s\n", offset, fpath.c_str() );
		_offsetMap->insert( pair );

		memset( minkbuf, 0, _KLEN+1 );
		rc = simpf->getMinKeyBuf( minkbuf );
		if ( 1 || rc ) {
			makeKOPair( minkbuf, offset, keyOffsetPair );
			_keyMap->insert( keyOffsetPair );
			d("s22373 _keyMap insert minkbuf=[%s] offset=%ld\n", minkbuf, offset );

			offset += simpf->_length; 
			_length += simpf->_length; 
		}
	}

}

void JagCompFile::getMinKOPair( const JagSimpFile *simpf, jagint offset, JagKeyOffsetPair & kopair )
{
	char minkbuf[_KLEN+1];
	memset( minkbuf, 0, _KLEN+1 );
	simpf->getMinKeyBuf( minkbuf );
	makeKOPair( minkbuf, offset, kopair );
}

void JagCompFile::makeKOPair( const char *buf, jagint offset, JagKeyOffsetPair & kopair )
{
	kopair = AbaxPair<JagFixString, AbaxLong>( JagFixString(buf, _KLEN, _KLEN ), offset );
}

JagCompFile::~JagCompFile()
{
	jagint arrlen =  _offsetMap->size();
	JagSimpFile *simpf;
	for ( int i = 0; i < arrlen; ++i ) {
		if ( _offsetMap->isNull(i) ) { continue; }
		simpf = (JagSimpFile*) (*_offsetMap)[i].value.value();
		if ( simpf) delete simpf;
	}

	delete _offsetMap;
	delete _keyMap;
}

// -1 error  0: no more data;  > 0 got data
jagint JagCompFile::pread(char *buf, jagint len, jagint offset ) const
{
	jagint partOffset;
	jagint offsetIdx;

    dn("s1022988 JagCompFile::pread offset=%ld  len=%ld", offset, len);
	int rc1 = _getOffSet( offset, partOffset, offsetIdx );
	if ( rc1 < 0 ) {
        dn("s8383337 pread error rc1=%d", rc1);
		return rc1;
	}

	jagint      n;
	JagSimpFile *simpf;
	jagint      remaining = len;
	jagint      totalRead =0;
	jagint      arrlen = _offsetMap->size(); // non-null entries point to simpfile
	jagint      localOffset = offset - partOffset; // first simpf start position

    dn("s0007171 len=%ld arrlen=%ld offset=%ld partOffset=%ld localOffset=%ld offsetIdx=%ld",
        len, arrlen, offset, partOffset, localOffset, offsetIdx );

	for ( int i = offsetIdx; i < arrlen; ++i ) {
		if ( _offsetMap->isNull(i) ) {
            dn("s77553 i=%d _offsetMap->isNull skip", i);
            continue;
        }

		simpf = (JagSimpFile*) (*_offsetMap)[i].value.value();

        dn("s85003 simpf.length=%ld bytes,  simpf.records=%ld", simpf->_length, simpf->_elements );
        dn("s85004 simpf.pread  localOffset=%ld  remaining=%ld", localOffset, remaining );

		n = simpf->pread(buf, localOffset, remaining );
        if ( n <= 0 ) {
            dn("s51122 i=%d simpf->pread n=%ld < 0 break", i, n );
            //continue;
            break; // 2/16/2023
        }

		totalRead += n;
		if ( totalRead == len ) { 
			break; 
		}

		remaining -= n;
		localOffset = 0; // from 2nd simpf onward, localoffset=0
		buf += n;
	}

    dn("s22339 totalRead=%ld", totalRead );
	return totalRead;
}

jagint JagCompFile::pwrite(const char *buf, jagint len, jagint offset )
{
	d("s4029 JagCompFile::pwrite buf=[%s] offset=%d len=%d\n", buf, offset, len );
	jagint partOffset;
	jagint offsetIdx;
	int rc1 = _getOffSet( offset, partOffset, offsetIdx );
	if ( rc1 < 0 ) {
		d("s30298 _getOffSet rc1=%d < 0\n", rc1 );
		return rc1;
	}
	d("s71287 _getOffSet rc1=%d partOffset=%d offsetIdx=%d\n", rc1, partOffset, offsetIdx );

	jagint n;
	JagSimpFile *simpf;
	jagint  remaining = len;
	jagint  totalWrite =0;
	jagint arrlen = _offsetMap->size();
	jagint localOffset = offset - partOffset;
	d("s039134 arrlen=%d localOffset=%d\n", arrlen, localOffset );

	for ( int i = offsetIdx; i < arrlen; ++i ) {
		if ( _offsetMap->isNull(i) ) continue;
		simpf = (JagSimpFile*) (*_offsetMap)[i].value.value();
		n = simpf->pwrite(buf, localOffset, remaining );
		totalWrite += n;
		if ( totalWrite == len ) { break; }
		if ( n < remaining ) { break; }

		remaining -= n;
		localOffset = 0;
		buf += n;
	}

	d("s3399 totalWrite=%d bytes\n", totalWrite );
	return totalWrite;
}

jagint JagCompFile::insert(const char *buf, jagint position, jagint len )
{
    dn("s23356 JagCompFile::insert position=%ld len=%ld", position, len);

	jagint partOffset;
	jagint offsetIdx;
	int rc1 = _getOffSet( position, partOffset, offsetIdx );
	if ( rc1 < 0 ) {
        dn("s818081 JagCompFile::insert rc1=%d < 0 return", rc1 );
        return rc1;
    }

	JagSimpFile *simpf;
	jagint localOffset = position - partOffset;
	simpf = (JagSimpFile*) (*_offsetMap)[offsetIdx].value.value();

	JagKeyOffsetPair oldpair;
	getMinKOPair( simpf, 0, oldpair );

	jagint splits = ( simpf->_length + len ) / JAG_SIMPFILE_LIMIT_BYTES;  // e.g.: 3
	jagint remainder = ( simpf->_length + len ) % JAG_SIMPFILE_LIMIT_BYTES;  // bytes
	Jstr fpath, pName;
	pName = longToStr( offsetIdx );
	fpath = _pathDir + "/" + pName;

    dn("s300331 pName=%s fpath=%s new JagSimpFile", pName.s(), fpath.s() );

	JagSimpFile *newfile1 = new JagSimpFile( this, fpath, _KLEN, _VLEN );

	//::sendfile( newfile1->_fd, simpf->_fd, NULL, localOffset );
	jagsendfile( newfile1->_fd, simpf->_fd, localOffset );


	jagint globalOffset = partOffset;

	(*_offsetMap)[offsetIdx] = JagOffsetSimpfPair(globalOffset, AbaxBuffer(newfile1) );  // replace

	jagint n;
	if ( len <= JAG_SIMPFILE_LIMIT_BYTES - localOffset ) {
		n = len; 
	} else {
		n = JAG_SIMPFILE_LIMIT_BYTES - localOffset;
	}
	jagint bufpos = 0;
	newfile1->pwrite( buf+bufpos, localOffset, n );
	bufpos += n;

	JagKeyOffsetPair newpair;
	getMinKOPair( newfile1, globalOffset, newpair );
	_keyMap->remove( oldpair );
	_keyMap->insert( newpair );

	jagint bytesToWrite = JAG_SIMPFILE_LIMIT_BYTES - localOffset;
	jagint startpos = 0;
	JagSimpFile *newf;
	JagKeyOffsetPair kopair;

	for ( int i=1; i < splits-1; ++i ) {
		globalOffset += JAG_SIMPFILE_LIMIT_BYTES;
		pName = longToStr( globalOffset );
		fpath = _pathDir + "/" + pName;

        dn("s53602 globalOffset=%ld pName=%s fpath=%s", globalOffset, pName.s(), fpath.s() );
		newf = new JagSimpFile( this, fpath, _KLEN, _VLEN );

		newf->pwrite( buf+bufpos, startpos, bytesToWrite );
		bufpos += bytesToWrite;
		startpos += bytesToWrite;
		bytesToWrite = JAG_SIMPFILE_LIMIT_BYTES;
		_offsetMap->insert( JagOffsetSimpfPair( globalOffset, AbaxBuffer(newf) ) );

		getMinKOPair( newf, globalOffset, kopair );
		_keyMap->insert( kopair );
	}

	if ( remainder > 0 ) {
		globalOffset += JAG_SIMPFILE_LIMIT_BYTES;
		pName = longToStr( globalOffset );
		fpath = _pathDir + "/" + pName;
        dn("s55602 globalOffset=%ld pName=%s fpath=%s", globalOffset, pName.s(), fpath.s() );

		newf = new JagSimpFile( this, fpath, _KLEN, _VLEN );

		newf->pwrite( buf+bufpos, startpos, remainder );
		bufpos += remainder;
		startpos += remainder;
		_offsetMap->insert( JagOffsetSimpfPair( globalOffset, AbaxBuffer(newf) ) );

		getMinKOPair( newf, globalOffset, kopair );
		_keyMap->insert( kopair );
	}

	//::sendfile( newf->_fd, simpf->_fd, NULL, simpf->_length - localOffset );
	jagsendfile( newf->_fd, simpf->_fd, simpf->_length - localOffset );
	delete simpf;

	jagint startIdx;
	JagOffsetSimpfPair startPair(globalOffset );
	_offsetMap->get( startPair, startIdx );
	JagVector<JagOffsetSimpfPair> vec;

	for ( int i = startIdx+1; i < _offsetMap->size(); ++i ) {
		if ( _offsetMap->isNull(i) ) continue;
		vec.push_back( (*_offsetMap)[i] );
	}

	jagint offset;
	for ( int i=0; i < vec.size(); ++i ) {
		offset =  vec[i].key.value();
		simpf = (JagSimpFile*)vec[i].value.value();
		JagOffsetSimpfPair oldp( offset );
		JagOffsetSimpfPair p( offset + len, vec[i].value );
		_offsetMap->remove( oldp );
		_offsetMap->insert( p );

		getMinKOPair( simpf, offset+len, kopair );
		_keyMap->set( kopair );
	}

	return splits+1;
}

jagint JagCompFile::remove( jagint position, jagint len )
{
	jagint partOffset;
	jagint offsetIdx;
	int rc1 = _getOffSet( position, partOffset, offsetIdx );
	if ( rc1 < 0 ) return rc1;

	JagSimpFile *simpf;
	simpf = (JagSimpFile*) (*_offsetMap)[offsetIdx].value.value();

	jagint endPosition = position + len;
	jagint endPartOffset;
	jagint endOffsetIdx;
	rc1 = _getOffSet( endPosition, endPartOffset, endOffsetIdx );
	if ( rc1 < 0 ) return rc1;

	JagSimpFile *sf;
	JagVector<JagOffsetSimpfPair> vec;
	for ( int i = offsetIdx+1; i <= endOffsetIdx-1; ++i ) {
		if ( _offsetMap->isNull(i) ) continue;
		vec.push_back( (*_offsetMap)[i] );
	}

	JagKeyOffsetPair kopair;
	for ( int i=0; i < vec.size(); ++i ) {
		sf = (JagSimpFile*) vec[i].value.value();
		getMinKOPair( sf, 0, kopair );
		sf->removeFile();
		delete sf;
		_offsetMap->remove( vec[i] );
		_keyMap->remove( kopair );
	}

	jagint endLocalOffset = endPosition - endPartOffset;
	sf = (JagSimpFile*) (*_offsetMap)[endOffsetIdx].value.value();
	getMinKOPair( sf, 0, kopair );
	sf->seekTo( endLocalOffset );

	//::sendfile( simpf->_fd, sf->_fd, NULL, sf->_length - endLocalOffset );
	jagsendfile( simpf->_fd, sf->_fd, sf->_length - endLocalOffset );

	sf->removeFile();
	delete sf;
	_offsetMap->remove( (*_offsetMap)[endOffsetIdx] );
	_keyMap->remove( kopair );

	vec.clear();
	for ( int i = endOffsetIdx+1; i < _offsetMap->size(); ++i ) {
		if ( _offsetMap->isNull(i) ) continue;
		vec.push_back( (*_offsetMap)[i] );
	}

	jagint offset;
	for ( int i=0; i < vec.size(); ++i ) {
		sf = (JagSimpFile*) vec[i].value.value();
		offset = vec[i].key.value();
		JagOffsetSimpfPair oldp( offset );
		JagOffsetSimpfPair p( offset - len, vec[i].value );
		_offsetMap->remove( oldp );
		_offsetMap->insert( p );

		getMinKOPair( sf, offset-len, kopair );
		_keyMap->set( kopair );

		d("s111288 _offsetMap->remove/insert offset=%d\n",  offset - len );
		d("s111288 _keyMap- set kopair=%s offset=%ld\n", kopair.key.c_str(),  offset-len );
	}

	return 0;
}

int JagCompFile::_getOffSet( jagint anyPosition, jagint &partOffset, jagint &offsetIdx )  const
{
	JagOffsetSimpfPair pair( anyPosition);
	d("s55621  _getOffSet() _offsetMap->get(%lld)\n", anyPosition );

	if ( _offsetMap->get( pair, offsetIdx ) ) {
		partOffset = anyPosition;
        dn("s044237 get is ok, partOffset=%ld offsetIdx=%ld", partOffset, offsetIdx );
	} else {
		d("s51346 _offsetMap->getPred() ...\n");
		const JagOffsetSimpfPair *predPair = _offsetMap->getPred( pair, offsetIdx );
		d("s51347 _offsetMap->getPred() done ...\n");
		if ( NULL == predPair ) {
			d("s10214 _offsetMap->getPred(%ld) NULL return -1\n", pair.key.value() );
			return -1;
		} else {
			partOffset = predPair->key.value();
		}
	}

    dn("s01224 _getOffSet() done return 0 (OK)");
	return 0;

}

int JagCompFile::removeFile()
{
	if ( ! _offsetMap ) return 0;

	jagint arrlen =  _offsetMap->size();

	JagSimpFile *simpf;
	for ( int i = 0; i < arrlen; ++i ) {
		if ( _offsetMap->isNull(i) ) { continue; }
		simpf = (JagSimpFile*) (*_offsetMap)[i].value.value();
		if ( ! simpf ) continue;
		simpf->removeFile();
		delete simpf;
	}
	return 0;
}

float JagCompFile::computeMergeCost( const JagDBMap *pairmap, jagint seqReadSpeed, jagint seqWriteSpeed, JagVector<JagMergeSeg> &vec )
{
	if ( pairmap->size() < 1 ) {
        dn("s0833777 JagCompFile::computeMergeCost pairmap-size() < 1, retur -1");
        return -1;
    }

	jagint      arrlen =  _offsetMap->size();
	JagSimpFile *simpf;
	char        kbuf[_KLEN + 1];
	int         rc;
	JagFixMapIterator leftIter = pairmap->_map->begin();
	JagFixMapIterator rightIter;
	size_t      numBufferKeys;
	jagint      fsz;
	JagMergeSeg seg;
	float       rd, wr, tsec;

	tsec = 0.0;

    dn("s0029281 arrlen=%ld", arrlen );

	jagint lastSimpfPos = -1;
	for ( jagint i = 0; i < arrlen; ++i ) {
		if ( _offsetMap->isNull(i) ) { continue; }
		lastSimpfPos = i;

		simpf = (JagSimpFile*) (*_offsetMap)[i].value.value();
		memset( kbuf, 0, _KLEN + 1 );
		rc = simpf->getMaxKeyBuf( kbuf );

        dn("s8801002 getMaxKeyBuf rc=%d", rc );

        // 2/16/2023
		if ( rc < 0 ) {
            dn("s8710023 rc < 0 continue");
			continue; 
		} 

		JagDBPair maxPair(kbuf, _KLEN );
		rightIter = pairmap->getPredOrEqual( maxPair );
		if ( rightIter == pairmap->_map->end() ) {
            dn("s8715023 rightIter not found continue");
			continue;
		}

		if ( rightIter->first < leftIter->first ) {
            dn("s8715003 rightIter->first < leftIter->first  continue");
			continue;
		}

		numBufferKeys = std::distance( leftIter, rightIter ) + 1;
		fsz = simpf->size(); 

		rd = (float)fsz/(float)ONE_MEGA_BYTES;
		wr = (float)(numBufferKeys*_KVLEN + fsz )/(float)ONE_MEGA_BYTES;
		tsec += rd/(float)seqWriteSpeed + wr/(float)seqWriteSpeed;
        dn("s394040 tsec=%f", tsec );

		seg.leftIter = leftIter;
		seg.rightIter = rightIter;
		seg.simpfPos = i;  
		vec.push_back( seg );
        dn("s052004 vec.pushed seg  vec.size=%d", vec.size() );

		++rightIter;
		if ( rightIter == pairmap->_map->end() ) {
			break;
		}

		leftIter = rightIter;
	}

	if ( leftIter !=  pairmap->_map->end() ) {
		int veclen = vec.size();
		if ( veclen > 0 ) {
			seg.leftIter = leftIter;

			rightIter = pairmap->_map->end();
			-- rightIter;
			seg.rightIter = rightIter;

			seg.simpfPos = lastSimpfPos;
			vec.push_back( seg );

			numBufferKeys = std::distance( leftIter, rightIter ) + 1;
			wr = (float)(numBufferKeys*_KVLEN )/(float)ONE_MEGA_BYTES;
			tsec += wr/(float)seqWriteSpeed;
		}
	}

    dn("s03038811 tsec=%f", tsec );
	return tsec;
}

jagint JagCompFile::mergeBufferToFile( const JagDBMap *pairmap, const JagVector<JagMergeSeg> &vec )
{
	int simpfPos;
	JagSimpFile *simpf;
	jagint cnt = 0;
	jagint bytes = 0;

    dn("s3400021 JagCompFile::mergeBufferToFile vec.siz=%d", vec.size() );

	for ( int i=0; i < vec.size(); ++i ) {
		simpfPos = vec[i].simpfPos;  
        dn("s3400041 i=%d simpfPos=%d", i, simpfPos );
		simpf = (JagSimpFile*)(*_offsetMap)[simpfPos].value.value();
		bytes = simpf->mergeSegment( vec[i] );
		cnt += bytes;
	}

	if ( 0 == vec.size() ) {
        dn("s394400 vec.size==0 flushBufferToNewSimpFile ...");
		cnt += flushBufferToNewSimpFile( pairmap )*_KVLEN;
	}

	refreshAllSimpfileOffsets();
	return cnt;
}

void JagCompFile::refreshAllSimpfileOffsets()
{
	JagVector<JagOffsetSimpfPair> vec;

	jagint offset = 0;
	jagint arrlen = _offsetMap->size();
	JagSimpFile *simpf;
	_length = 0;

	for ( int i = 0; i < arrlen; ++i ) {
		if ( _offsetMap->isNull(i) ) { continue; }
		simpf = (JagSimpFile*) (*_offsetMap)[i].value.value();
		if ( simpf) {
			JagOffsetSimpfPair pair(offset, AbaxBuffer(simpf));
			vec.push_back(pair);
			offset += simpf->_length; 
			_length += simpf->_length; 
		}
	}

	delete _offsetMap;
	delete _keyMap;

	_offsetMap = new JagArray< JagOffsetSimpfPair >();
	_offsetMap->useHash( true );
	_keyMap = new JagArray< JagKeyOffsetPair >();

	JagKeyOffsetPair keyOffsetPair;
	char minkbuf[_KLEN+1];
	int rc;
	for ( int i=0; i < vec.size(); ++i ) {
		_offsetMap->insert( vec[i] );

		simpf = (JagSimpFile*)vec[i].value.value();
		offset = vec[i].key.value();

		memset( minkbuf, 0, _KLEN+1);
		rc = simpf->getMinKeyBuf( minkbuf );
		if ( 1 || rc ) {
			makeKOPair( minkbuf, offset, keyOffsetPair );
			_keyMap->insert( keyOffsetPair );
		}
	}
}

void JagCompFile::buildInitIndex( bool force )
{
	jagint arrlen =  _offsetMap->size();
	JagSimpFile *simpf;
	for ( int i = 0; i < arrlen; ++i ) {
		if ( _offsetMap->isNull(i) ) { continue; }
		simpf = (JagSimpFile*) (*_offsetMap)[i].value.value();
		if ( simpf) {
			simpf->buildInitIndex( force );
		}
	}
}

int  JagCompFile::buildInitIndexFromIdxFile()
{
	jagint arrlen =  _offsetMap->size();
	JagSimpFile *simpf;
	for ( int i = 0; i < arrlen; ++i ) {
		if ( _offsetMap->isNull(i) ) { continue; }
		simpf = (JagSimpFile*) (*_offsetMap)[i].value.value();
		if ( simpf) {
			jd(JAG_LOG_LOW, "s318203 i=%d/%d  simpf->buildInitIndexFromIdxFile ...\n", i, arrlen );
			simpf->buildInitIndexFromIdxFile();
			jd(JAG_LOG_LOW, "s318203 i=%d/%d  simpf->buildInitIndexFromIdxFile done\n", i, arrlen );
		} else {
			jd(JAG_LOG_LOW, "s318203 i=%d/%d  simpf==NULL skip buildInitIndexFromIdxFile\n", i, arrlen );
		}
	}
	return 0;
}

void JagCompFile::flushBlockIndexToDisk()
{
	jagint arrlen =  _offsetMap->size();
	JagSimpFile *simpf;
	for ( int i = 0; i < arrlen; ++i ) {
		if ( _offsetMap->isNull(i) ) { continue; }
		simpf = (JagSimpFile*) (*_offsetMap)[i].value.value();
		if ( simpf) {
			jd(JAG_LOG_LOW, "s308103 i=%d flushBlockIndexToDisk ...\n", i );
			simpf->flushBlockIndexToDisk();
			jd(JAG_LOG_LOW, "s308103 i=%d flushBlockIndexToDisk done\n", i );
		} else {
			jd(JAG_LOG_LOW, "s308103 i=%d simpf==NULL\n", i );
		}
	}
}

void JagCompFile::removeBlockIndexIndDisk()
{
	jagint arrlen =  _offsetMap->size();
	JagSimpFile *simpf;
	for ( int i = 0; i < arrlen; ++i ) {
		if ( _offsetMap->isNull(i) ) { continue; }
		simpf = (JagSimpFile*) (*_offsetMap)[i].value.value();
		if ( simpf) {
			simpf->removeBlockIndexIndDisk();
		}
	}
}

jagint JagCompFile::flushBufferToNewSimpFile( const JagDBMap *pairmap )
{
	jagint offset = _length;
	Jstr fpath =  _pathDir + "/" + longToStr( offset ); 

    dn("s222087 flushBufferToNewSimpFile offset=%ld fpath=%s", offset, fpath.s() );

	JagSimpFile *simpf = new JagSimpFile( this, fpath, _KLEN, _VLEN );

	simpf->flushBufferToNewFile( pairmap );
	JagOffsetSimpfPair pair( offset, AbaxBuffer(simpf) );
	_offsetMap->insert( pair );

	JagKeyOffsetPair kopair;
	getMinKOPair( simpf, offset, kopair );
	_keyMap->insert( kopair );

	_length += simpf->_length; 
	return simpf->_length/_KVLEN;
}

int JagCompFile::removePair( const JagDBPair &pair )
{
	JagSimpFile *simpf = getSimpFile( pair );
	if ( nullptr == simpf ) {
		return -1;
	}

	int rc = simpf->removePair( pair );
	return rc;
}

int JagCompFile::updatePair( const JagDBPair &pair )
{
	JagSimpFile *simpf = getSimpFile( pair );
	if ( nullptr == simpf ) {
		return -1;
	}

	int rc = simpf->updatePair( pair );
	return rc;
}

int JagCompFile::exist( const JagDBPair &pair, JagDBPair &retpair )
{
	JagSimpFile *simpf = getSimpFile( pair );
	if ( nullptr == simpf ) {
		return -1;
	}

	int rc = simpf->exist( pair, retpair );
	return rc;
}


bool JagCompFile::findFirstLast( const JagDBPair &pair, jagint &first, jagint &last ) const
{
    dn("s9100023 JagCompFile::findFirstLast pair:");
    //pair.printkv( true );

	JagKeyOffsetPair tpair(pair.key);

	const JagKeyOffsetPair *tp = _keyMap->getPredOrEqual( tpair );
	if ( ! tp ) {
        dn("s804001 JagCompFile::findFirstLast return false !!!!");
        return false;
    }

	AbaxLong goffset =  tp->value;

	JagOffsetSimpfPair ofpair( goffset );
	const JagOffsetSimpfPair *op = _offsetMap->getPredOrEqual( ofpair );
	if ( ! op ) {  
        dn("s887760 getPredOrEqual op==NULL return false !!!!");
        return false;
    }

	jagint goffsetval = goffset.value();

	JagSimpFile *simpf = ( JagSimpFile*) op->value.value();
	bool rc = simpf->getFirstLast( pair, first, last );
	first = first + goffsetval/_KVLEN;
	last = last + goffsetval/_KVLEN;
    dn("s870041 simpf->getFirstLast rc=%d  first=%ld  last=%ld", rc, first, last);
	return rc;
}

jagint JagCompFile::getPartElements( jagint pos ) const
{
	JagOffsetSimpfPair ofpair( pos );
	const JagOffsetSimpfPair *op = _offsetMap->getPredOrEqual( ofpair );
	if ( ! op ) return 0;

	JagSimpFile *simpf = ( JagSimpFile*) op->value.value();
	jagint rc = simpf->getPartElements( pos - op->key.value() );
	return rc;
}


JagSimpFile *JagCompFile::getSimpFile(  const JagDBPair &pair )
{
	JagKeyOffsetPair tpair(pair.key);
	const JagKeyOffsetPair *tp = _keyMap->getPredOrEqual( tpair );
	if ( ! tp ) return nullptr;

	AbaxLong offset =  tp->value;
	JagOffsetSimpfPair ofpair( offset );

	const JagOffsetSimpfPair *op = _offsetMap->getPredOrEqual( ofpair );
	if ( ! op ) return nullptr;

	JagSimpFile *simpf = ( JagSimpFile*) op->value.value();
	return simpf;
}

void JagCompFile::print()
{
	jagint arrlen = _offsetMap->size();
	i("s4444829 JagCompFile::print() arrlen=%d\n", arrlen );
	JagSimpFile *simpf;
	for ( jagint j = 0; j < arrlen; ++j ) {
		if ( _offsetMap->isNull(j) ) { continue; }
		i("    i=%d simpf:\n", j );
		simpf = (JagSimpFile*) (*_offsetMap)[j].value.value();
		simpf->print();
	}
}
