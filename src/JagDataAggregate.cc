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

#include <JagDataAggregate.h>
#include <JagHashMap.h>
#include <JagMutex.h>
#include <JagFSMgr.h>
#include <JagUtil.h>
#include <JagCfg.h>
#include <JagSingleMergeReader.h>
#include <JagSchemaRecord.h>
#include <JagFileMgr.h>
#include <JagTable.h>
#include <JagRequest.h>
#include <JagMath.h>

JagDataAggregate::JagDataAggregate( bool isserv ) 
{
	_useDisk = 0;
	_keepFile = 0;
	_isSetWriteDone = 0;
	_isFlushWriteDone = 0;
	_numHosts = 0;
	_numIdx = 0;
	_datalen = 0;
	_readpos = 0;
	_readlen = 0;
	_readmaxlen = 0;
	_maxLimitBytes = 0;
	_totalwritelen = 0;
	_numwrites = 0;
	initWriteBuf();
	_readbuf = NULL;
	_sqlarr = NULL;
	_hostToIdx = new JagHashMap<AbaxString, jagint>();

	if ( isserv ) {
		_lock = NULL; 
	} else {
		_lock = newJagReadWriteLock();
	}

	_jfsMgr = new JagFSMgr();
	if ( isserv ) {
		_cfg = new JagCfg( JAG_SERVER );
	} else {
		_cfg = new JagCfg( JAG_CLIENT );
	}
	_isServ = isserv;
	_defpath =  jaguarHome() + "/tmp/";
	_mergeReader = NULL;
}

JagDataAggregate::~JagDataAggregate()
{
	clean();
	if ( _hostToIdx ) {
		delete _hostToIdx;
		_hostToIdx = NULL;
	}

	if ( _lock ) {
		deleteJagReadWriteLock( _lock );
	}

	if ( _jfsMgr ) {
		delete _jfsMgr;
		_jfsMgr = NULL;
	}

	if ( _cfg ) {
		delete _cfg;
		_cfg = NULL;
	}
}

void JagDataAggregate::clean()
{
    dn("sa300088 JagDataAggregate::clean() is called");

	cleanWriteBuf();
	
	if ( _readbuf ) {
		free( _readbuf );
		_readbuf = NULL;
	}

	if ( _sqlarr ) {
		delete [] _sqlarr;
		_sqlarr = NULL;
	}
	
	for ( int i = 0; i < _dbPairFileVec.size(); ++i ) {
		_jfsMgr->closefd( _dbPairFileVec[i].fpath );
		if ( _keepFile != 1 && _keepFile != 3 ) {
			_jfsMgr->remove( _dbPairFileVec[i].fpath );
		}
	}

	_dbPairFileVec.clean();
	_pallreadpos.clean();
	_pallreadlen.clean();
	_hostToIdx->removeAllKey();
	
	_dbobj = "";
	_useDisk = 0;
	_keepFile = 0;
	_isSetWriteDone = _isFlushWriteDone = 0;
	_numHosts = _numIdx = _datalen = _readpos = _readlen = _readmaxlen = _totalwritelen = _numwrites = 0;
	_maxLimitBytes = 0;
    dn("da03445 in clean() _useDisk set to 0");

	if ( _mergeReader ) {
		delete _mergeReader;
		_mergeReader = NULL;
	}

}

void JagDataAggregate::setwrite( const JagVector<Jstr> &hostlist )
{
	clean();
	JagDBPairFile dbpfile;
	_numHosts = hostlist.size();

	for ( jagint i = 0; i < _numHosts; ++i ) {
		dbpfile.fpath = _defpath + longToStr( THID ) + "_" + hostlist[i];
        dn("a0933819 in setwrite openfd [%s]", dbpfile.fpath.s() );
		dbpfile.fd = _jfsMgr->openfd( dbpfile.fpath, true );
		if ( dbpfile.fd < 0 ) {
		}

		_dbPairFileVec.append( dbpfile );
		_pallreadpos.append( 0 );
		_pallreadlen.append( 0 );
		_hostToIdx->addKeyValue( hostlist[i], i );
	}

	_isSetWriteDone = 1;	

}

void JagDataAggregate::setwrite( jagint numHosts )
{
	clean();
	JagDBPairFile dbpfile;
	_numHosts = numHosts;

	for ( jagint i = 0; i < _numHosts; ++i ) {
		dbpfile.fpath = _defpath + longToStr( THID ) + "_" + longToStr( i );
		_dbPairFileVec.append( dbpfile );
		_pallreadpos.append( 0 );
		_pallreadlen.append( 0 );
		_hostToIdx->addKeyValue( longToStr( i ), i );
	}
	_isSetWriteDone = 1;
}

void JagDataAggregate::setwrite( const Jstr &mapstr, const Jstr &filestr, int keepFile )
{

	clean();
	JagDBPairFile dbpfile;
	_numHosts = 1; // mapstr acts like a host

	if ( keepFile == 1 ) {
		_dirpath = jaguarHome() + "/export/" + filestr + "/";
		dbpfile.fpath = _dirpath + filestr + ".sql";
		_keepFile = 1;
		_dbobj = filestr;
		JagFileMgr::rmdir( _dirpath );
		JagFileMgr::makedirPath( _dirpath, 0755 );
	} else if ( keepFile == 2 || keepFile == 3 ) {
		dbpfile.fpath = filestr;
		_keepFile = keepFile;
	} else {
		dbpfile.fpath = _defpath + longToStr( THID ) + "_" + filestr;
        dn("a450028 setwrite dbpfile.fpath=[%s]", dbpfile.fpath.s() );
	}

	_dbPairFileVec.append( dbpfile );
	_pallreadpos.append( 0 );
	_pallreadlen.append( 0 );
	_hostToIdx->addKeyValue( AbaxString(mapstr), 0 );  
	_isSetWriteDone = 1;

}

void JagDataAggregate::setMemoryLimit( jagint maxLimitBytes )
{
	_maxLimitBytes = maxLimitBytes;
}

// 0: ok; <0: error
int JagDataAggregate::writeit( int hosti, const char *buf, jagint len, 
								const JagSchemaRecord *rec, bool noUnlock, jagint membytes )
{
	d("s202930 writeit hosti=[%d] buf=[%s] len=%d noUnlock=%d membytes=%d _keepFile=%d\n", 
			 hosti, buf, len, noUnlock, membytes, _keepFile );
    //dumpmem(buf, len);

    /***
    dn("s70900023 writeit dumpmem:");
    dumpmem( buf, len, true );
    ***/

	if ( !_isSetWriteDone ) {
		return -10 ;
	}

	if ( !_writebuf[hosti] && _keepFile != 1 ) {
		if ( _datalen <= 0 ) _datalen = len;
		jagint maxbytes = 10*1024*1024;

		if ( membytes > 0 && membytes < maxbytes ) maxbytes = membytes;
		if ( _datalen < 1 ) {
			return -20;
		}

		maxbytes /= _datalen;
		if ( 0 == maxbytes ) ++maxbytes;
		maxbytes *= _datalen;  

		_writebuf[hosti] = (char*)jagmalloc( maxbytes );
		memset( _writebuf[hosti], 0, maxbytes );
		_writebufHasData = true;
		
		_dbPairFileVec[hosti].memstart = 0;
		_dbPairFileVec[hosti].memoff = _dbPairFileVec[hosti].mempos = 0;
		_dbPairFileVec[hosti].memlen = maxbytes;
	}
	
	jagint clen, wlen;

	if ( _keepFile == 1 ) {
		cleanWriteBuf();

		if ( !_sqlarr ) {
			if ( _datalen <= 0 ) _datalen = len;
			jagint maxlens = jagatoll(_cfg->getValue("SHUFFLE_MEM_SIZE_MB", "10").s())*1024*1024/_datalen;
			_sqlarr = new Jstr[maxlens];
			_dbPairFileVec[0].memlen = maxlens-1;
			_dbPairFileVec[0].mempos = 0;
			_dbPairFileVec[0].memstart = 0;
			_dbPairFileVec[0].memoff = 0;
		}

		if ( !rec ) {
			return -30;
		}

		jagint offset, length;
		Jstr type;
		Jstr outstr, cmd = Jstr("insert into ") + _dbobj + "(";
		bool isLast = 0;
		for ( int i = 0; i < rec->columnVector->size(); ++i ) {
			if ( i == rec->columnVector->size()-2 ) {
				if ( (*(rec->columnVector))[i+1].name == "spare_" ) { isLast = 1; }
			} else if ( i==rec->columnVector->size()-1 ) { 
				isLast = 1; 
			}

			if ( ! isLast ) {
				cmd += Jstr(" ") + (*(rec->columnVector))[i].name.c_str() + ",";
			} else {
				cmd += Jstr(" ") + (*(rec->columnVector))[i].name.c_str() + " ) values (";
			}
			if ( isLast ) break;
		}

		isLast = 0;
		for ( int i = 0; i < rec->columnVector->size(); ++i ) {
			if ( i == rec->columnVector->size()-2 ) {
				if ( (*(rec->columnVector))[i+1].name == "spare_" ) { isLast = 1; }
			} else if ( i==rec->columnVector->size()-1 ) { 
				isLast = 1; 
			}
			
			offset = (*(rec->columnVector))[i].offset;
			length = (*(rec->columnVector))[i].length;
			type = (*(rec->columnVector))[i].type;

			outstr = formOneColumnNaturalData( buf, offset, length, type );

			if ( ! isLast ) {
				cmd += Jstr(" '") + outstr + "',";
			} else {
				cmd += Jstr(" '") + outstr + "' );\n";
			}
			if ( isLast ) break;
		}

		if ( _dbPairFileVec[0].mempos > _dbPairFileVec[0].memlen ) {
			shuffleSQLMemAndFlush();
			_dbPairFileVec[0].mempos = 0;
		}
		_sqlarr[_dbPairFileVec[0].mempos++] = cmd;	
		_totalwritelen += len;
		++_numwrites;

        dn("a30303381 _keepFile == 1 return 0 here");
		return 0;
	}

	if ( _dbPairFileVec[hosti].mempos + len > _dbPairFileVec[hosti].memlen ) { 
		wlen = _dbPairFileVec[hosti].mempos - _dbPairFileVec[hosti].memoff;
		if ( _dbPairFileVec[hosti].fd < 0 ) {
            dn("a2203389 in writeit() openfd [%s]", _dbPairFileVec[hosti].fpath.s() );
			_dbPairFileVec[hosti].fd = _jfsMgr->openfd( _dbPairFileVec[hosti].fpath, true );
			_useDisk = 1;
            dn("da4000293 _useDisk set to 1");
		}

		clen = _jfsMgr->pwrite( _dbPairFileVec[hosti].fd, _writebuf[hosti]+_dbPairFileVec[hosti].memoff, wlen, _dbPairFileVec[hosti].diskpos );
		if ( clen < wlen ) {
			clean();
			return -40;
		}
		_useDisk = 1;
        dn("da4000233 _useDisk set to 1");

		memset( _writebuf[hosti]+_dbPairFileVec[hosti].memoff, 0, wlen );
		_dbPairFileVec[hosti].mempos = _dbPairFileVec[hosti].memoff;
		_dbPairFileVec[hosti].disklen += clen;
		_dbPairFileVec[hosti].diskpos = _dbPairFileVec[hosti].disklen;
	}
	
	if ( len > _dbPairFileVec[hosti].memlen - _dbPairFileVec[hosti].memoff ) { 
		if ( _dbPairFileVec[hosti].fd < 0 ) {
            dn("a102909 in writeit() openfd [%s]", _dbPairFileVec[hosti].fpath.s() );
			_dbPairFileVec[hosti].fd = _jfsMgr->openfd( _dbPairFileVec[hosti].fpath, true );
			_useDisk = 1;
            dn("da4300233 _useDisk set to 1");
		}

		clen = _jfsMgr->pwrite( _dbPairFileVec[hosti].fd, buf, len, _dbPairFileVec[hosti].diskpos );
		if ( clen < len ) {
			clean();
			return -50;
		}
		_useDisk = 1;
        dn("da4305233 _useDisk set to 1");
		_dbPairFileVec[hosti].disklen += clen;
		_dbPairFileVec[hosti].diskpos = _dbPairFileVec[hosti].disklen;		
	} else { 
		memcpy( _writebuf[hosti]+_dbPairFileVec[hosti].mempos, buf, len );
		_dbPairFileVec[hosti].mempos += len;
	}

	_totalwritelen += len;
	++_numwrites;

    dn("sa344409 writeit return 0 at end");
	return 0;
}

bool JagDataAggregate::flushwrite()
{
	jagint clen, wlen;

	if ( _keepFile == 1 ) {
        dn("a2233088 flushwrite _keepFile == 1");
		if ( _dbPairFileVec[0].mempos != _dbPairFileVec[0].memoff ) {
			shuffleSQLMemAndFlush();
			_dbPairFileVec[0].mempos = 0;
		}
		cleanWriteBuf();
		if ( _sqlarr ) delete [] _sqlarr;
		_sqlarr = NULL;
	} else if ( _useDisk || _keepFile == 3 ) {
        dn("a2233083 _useDisk || _keepFile == 3 ");
		for ( int i = 0; i < _numHosts; ++i ) {
			if ( _dbPairFileVec[i].mempos != _dbPairFileVec[i].memoff ) {
				wlen = _dbPairFileVec[i].mempos - _dbPairFileVec[i].memoff;
				if ( _dbPairFileVec[i].fd < 0 ) {
                    dn("a17220 in flushwrite() openfd [%s]", _dbPairFileVec[i].fpath.s() );
					_dbPairFileVec[i].fd = _jfsMgr->openfd( _dbPairFileVec[i].fpath, true );
				}
				clen = _jfsMgr->pwrite( _dbPairFileVec[i].fd, _writebuf[i]+_dbPairFileVec[i].memoff, 
									  wlen, _dbPairFileVec[i].diskpos );
				if ( clen < wlen ) {
					clean();
					return false;
				}
				_dbPairFileVec[i].mempos = _dbPairFileVec[i].memoff;
				_dbPairFileVec[i].disklen += clen;
			}
			_dbPairFileVec[i].diskpos = 0;
		}
		cleanWriteBuf();
	} else {
        dn("a55504 else");
    }

	_isFlushWriteDone = 1;
	return true;
}

bool JagDataAggregate::readit( JagFixString &res )
{
    dn("da00120 JagDataAggregate::readit _datalen=%d _useDisk=%d", _datalen, _useDisk );
	res = "";
	if ( 0 == _datalen || !_isFlushWriteDone ) {
		clean();
        dn("sa12208 JagDataAggregate::readit 0 == _datalen || !_isFlushWriteDone return false");
		return false;
	}

	if ( !_useDisk ) {
		while ( true ) {
			if ( _numIdx >= _numHosts ) {
				break;
			}

			if ( _dbPairFileVec[_numIdx].memoff + _datalen > _dbPairFileVec[_numIdx].mempos ) {
				++_numIdx;
			} else {
				res = JagFixString( _writebuf[_numIdx]+_dbPairFileVec[_numIdx].memoff, _datalen, _datalen );
                dn("da0020304 readit() !_useDisk got res=[%s] memoff=%d datalen=%d", res.s(), _dbPairFileVec[_numIdx].memoff,  _datalen );
                //dumpmem( _writebuf[_numIdx]+_dbPairFileVec[_numIdx].memoff, _datalen );
				_dbPairFileVec[_numIdx].memoff += _datalen;
				break;
			}
		}
		
		if ( _numIdx >= _numHosts ) {
			clean();
            dn("sa51004002 readit() _numIdx >= _numHosts return false");
			return false;
		}

        dn("s1222098 readit return true");
		return true;
	} else {
		jagint rc;
		if ( !_readbuf ) {
			jagint maxbytes = 10 * ONE_MEGA_BYTES;
			maxbytes /= _datalen;
			if ( 0 == maxbytes ) ++maxbytes;
			maxbytes *= _datalen;
			_readmaxlen = maxbytes;
			_readbuf = (char*)jagmalloc( maxbytes );
			memset( _readbuf, 0, maxbytes );

			rc = readNextBlock();

			if ( rc < 0 ) {
				clean();
                dn("s11220 readit readNextBlock rc=%d return false", rc );
				return false;
			}
		}
		
		if ( _readpos + _datalen > _readlen ) { 
            dn("da222208 readNextBlock ...");
			rc = readNextBlock();
			if ( rc < 0 ) {
				clean();
                dn("s15220 readit readNextBlock rc=%d return false", rc );
				return false;
			}
		}

		res = JagFixString( _readbuf+_readpos, _datalen, _datalen );
        dn("da0020354 readit() _useDisk got res=[%s]", res.s() );
		_readpos += _datalen;
        dn("s122088 return true");
		return true;
	}
}

bool JagDataAggregate::backreadit( JagFixString &res )
{
	res = "";
	if ( 0 == _datalen || !_isFlushWriteDone ) {
		clean();
		return false;
	}

	if ( !_useDisk ) {
		int i;
        jagint mempos, memoff, memstart;

		while ( _numIdx < _numHosts ) {
			i = _numHosts-_numIdx-1;

			if ( _dbPairFileVec[i].memoff + _datalen > _dbPairFileVec[i].mempos ) {
				++_numIdx;
			} else {
                mempos = _dbPairFileVec[i].mempos;
                memstart = _dbPairFileVec[i].memstart;
                memoff = _dbPairFileVec[i].memoff;

				res = JagFixString( _writebuf[_numIdx] + (mempos-memoff-_datalen+memstart), _datalen, _datalen );

				_dbPairFileVec[_numHosts-_numIdx-1].memoff += _datalen;
				break;
			}
		}
		
		if ( _numIdx >= _numHosts ) {
			clean();
			return false;
		}
		return true;
	} else {
		jagint rc;
		if ( !_readbuf ) {
			jagint maxbytes = getUsableMemory();
			maxbytes /= _datalen;
			if ( 0 == maxbytes ) ++maxbytes;
			maxbytes *= _datalen;
			_readmaxlen = maxbytes;
			_readbuf = (char*)jagmalloc( maxbytes );
			memset( _readbuf, 0, maxbytes );
			rc = backreadNextBlock();
			if ( rc < 0 ) {
				clean();
				return false;
			}
		}
		
		if ( _readpos + _datalen > _readlen ) { 
			rc = backreadNextBlock();
			if ( rc < 0 ) {
				clean();
				return false;
			}
		}

		res = JagFixString( _readbuf+(_readlen-_readpos-_datalen), _datalen, _datalen );
		_readpos += _datalen;
		return true;
	}
	
}

void JagDataAggregate::setread( jagint start, jagint readcnt )
{
    dn("da20019 setread _useDisk=%d", _useDisk );
	if ( _useDisk ) {
		_dbPairFileVec[_numIdx].diskpos = start * _datalen;
		_dbPairFileVec[_numIdx].disklen = _dbPairFileVec[_numIdx].diskpos + readcnt * _datalen;
	} else {
		_dbPairFileVec[_numIdx].memoff = start * _datalen;
		_dbPairFileVec[_numIdx].mempos = _dbPairFileVec[_numIdx].memoff + readcnt * _datalen;
	}
}

char *JagDataAggregate::readBlock( jagint &len )
{
	if ( 0 == _datalen || !_isFlushWriteDone ) {
		clean();
		len = -1;
		return NULL;
	}

	if ( !_useDisk ) {
		if ( _numIdx >= _dbPairFileVec.size() || _dbPairFileVec[_numIdx].memoff >= _dbPairFileVec[_numIdx].mempos ) {
			clean();
			len = -1;
			return NULL;
		}

		char *ptr = _writebuf[_numIdx]+_dbPairFileVec[_numIdx].memoff;
		len = _dbPairFileVec[_numIdx].mempos - _dbPairFileVec[_numIdx].memoff;
		_dbPairFileVec[_numIdx].memoff += len;
		++_numIdx;
		return ptr;
	} else {
		jagint rc;
		if ( !_readbuf ) {
			jagint maxbytes = 10*ONE_MEGA_BYTES;
			maxbytes /= _datalen;
			if ( 0 == maxbytes ) ++maxbytes;
			maxbytes *= _datalen;
			_readmaxlen = maxbytes;
			_readbuf = (char*)jagmalloc( maxbytes );
			memset( _readbuf, 0, maxbytes );
			rc = readNextBlock();
			if ( rc < 0 ) {
				clean();
				len = -1;
				return NULL;
			}
		}
	
		if ( _readpos + _datalen > _readlen ) { 
			rc = readNextBlock();
			if ( rc < 0 ) {
				clean();
				len = -1;
				return NULL;
			}
		}
	
		_readpos = _readlen;
		len = _readlen;
		return _readbuf;
	}
}

jagint JagDataAggregate::readNextBlock()
{
	memset( _readbuf, 0, _readlen );
	_readpos = _readlen = 0;
	
	if ( _numIdx >= _numHosts ) { 
		return -1;
	}
	
	jagint mrest, drest, clen;
	while( true ) {
		if ( _numIdx >= _numHosts || _readmaxlen == _readlen ) break;
		mrest = _readmaxlen - _readlen;
		drest = _dbPairFileVec[_numIdx].disklen - _dbPairFileVec[_numIdx].diskpos;
		
		if ( 0 == _dbPairFileVec[_numIdx].disklen ) ++_numIdx;
		else {
			if ( drest > mrest ) {
				clen = _jfsMgr->pread( _dbPairFileVec[_numIdx].fd, _readbuf+_readpos, 
									 mrest, _dbPairFileVec[_numIdx].diskpos );
				if ( clen < mrest ) {
					return -1;
				}
				_dbPairFileVec[_numIdx].diskpos += clen;
				_readlen += clen;
				_readpos += clen;
			} else {
				clen = _jfsMgr->pread( _dbPairFileVec[_numIdx].fd, _readbuf+_readpos, 
									drest, _dbPairFileVec[_numIdx].diskpos );
				if ( clen < drest ) {
					return -1;
				}
				++_numIdx;
				_readlen += clen;
				_readpos += clen;
			}
		}
	}
	
	_readpos = 0;
	return _readlen;
}

jagint JagDataAggregate::backreadNextBlock()
{
	memset( _readbuf, 0, _readlen );
	_readpos = _readlen = 0;
	
	if ( _numIdx >= _numHosts ) { 
		return -1;
	}
	
	jagint mrest, drest, clen;
	int idx;

	while( true ) {
		if ( _numIdx >= _numHosts || _readmaxlen == _readlen ) break;
		mrest = _readmaxlen - _readlen;

		idx = _numHosts-_numIdx-1;

		//drest = _dbPairFileVec[_numHosts-_numIdx-1].disklen - _dbPairFileVec[_numHosts-_numIdx-1].diskpos;
		drest = _dbPairFileVec[idx].disklen - _dbPairFileVec[idx].diskpos;
		
		if ( 0 == _dbPairFileVec[_numHosts-_numIdx-1].disklen ) ++_numIdx;
		else {
			if ( drest > mrest ) {
				clen = _jfsMgr->pread( _dbPairFileVec[idx].fd, _readbuf+_readpos, mrest, drest-mrest );
				if ( clen < mrest ) {
					return -1;
				}
				_dbPairFileVec[idx].diskpos += clen;
				_readlen += clen;
				_readpos += clen;
			} else {
				clen = _jfsMgr->pread( _dbPairFileVec[idx].fd, _readbuf+_readpos, drest, 0 );
				if ( clen < drest ) {
					return -1;
				}
				++_numIdx;
				_readlen += clen;
				_readpos += clen;
				break;
			}
		}
	}
	
	_readpos = 0;
	return _readlen;
}

jagint JagDataAggregate::getUsableMemory()
{
	jagint maxbytes, callCounts = -1, lastBytes = 0;
	int mempect = atoi(_cfg->getValue("DBPAIR_MEM_PERCENT", "5").c_str());

	if ( mempect < 2 ) mempect = 2;
	else if ( mempect > 50 ) mempect = 50;

	maxbytes = availableMemory( callCounts, lastBytes )*mempect/100;

	if ( maxbytes < 100000000 ) {
		maxbytes = 100000000;
	}

	if ( _isServ && maxbytes > _maxLimitBytes ) {
		maxbytes = _maxLimitBytes;
	}

	if ( !_isServ ) {
		_maxLimitBytes = 10*ONE_MEGA_BYTES;
		if ( maxbytes > _maxLimitBytes ) {
			maxbytes = _maxLimitBytes;
		}
	}

	return maxbytes;
}

int JagDataAggregate::joinReadNext( JagVector<JagFixString> &vec )
{
	if ( 0 == _datalen || ! _isFlushWriteDone ) {
		clean();
		return 0;
	}

	if ( ! _useDisk ) {
		return _joinReadFromMem( vec );
	} else {
		return _joinReadFromDisk( vec );
	}
}

void JagDataAggregate::beginJoinRead( int klen )
{
	_keylen = klen;
	_vallen = _datalen - klen;

	for ( int i = 0; i < _numHosts; ++i ) {
		_goNext[i] = 1;
	}

	if ( NULL == _mergeReader ) {
		JagVector<OnefileRangeFD> vec;
		jagint memmax = 1024/_numHosts;
		memmax = getBuffReaderWriterMemorySize( memmax );
		for ( int i = 0; i < _numHosts; ++i ) {
			OnefileRangeFD fr;
			fr.fd = _dbPairFileVec[i].fd;
			fr.startpos = 0;
			fr.readlen = _dbPairFileVec[i].disklen/_datalen;
			fr.memmax = memmax;
			vec.append( fr );
		}
		_mergeReader = new JagSingleMergeReader( vec, _numHosts, _keylen, _vallen );
	}

	_endcnt = 0;
}

void JagDataAggregate::endJoinRead()
{
	clean();
}


int JagDataAggregate::_joinReadFromMem( JagVector<JagFixString> &vec )
{
	int rc, cnt = 0, minpos = -1;
	
	for ( int i = 0; i < _numHosts; ++i ) {
		if ( 1 == _goNext[i] ) {
			rc = _getNextDataOfHostFromMem( i ); 
			if ( rc > 0 ) {
				_goNext[i] = 0;
				if ( minpos < 0 ) minpos = i; 
			} else {
				_goNext[i] = -1;
				++_endcnt;
			}
		} else if ( 0 == _goNext[i] ) {
			if ( minpos < 0 ) minpos = i;
		}
	}
	
	if ( _endcnt == _numHosts || minpos < 0 ) {
		return 0;
	}
	
	for ( int i = 0; i < _numHosts; ++i ) {
		if ( _goNext[i] != -1 ) {
			rc = memcmp( _dbPairFileVec[i].kv.c_str(), _dbPairFileVec[minpos].kv.c_str(), _keylen );
			if ( rc < 0 ) {
				minpos = i;
			}
		}
	}

	for ( int i = 0; i < _numHosts; ++i ) {
		if ( _goNext[i] != -1 ) {
			rc = memcmp( _dbPairFileVec[i].kv.c_str(), _dbPairFileVec[minpos].kv.c_str(), _keylen );
			if ( 0 == rc ) {
				vec.append( _dbPairFileVec[i].kv ); 
				++cnt;
				_goNext[i] = 1;
			}
		}
	}
	
	return cnt;	
}

int JagDataAggregate::_getNextDataOfHostFromMem( int hosti  )
{
	jagint memoff = _dbPairFileVec[hosti].memoff;
	if ( memoff >= _dbPairFileVec[hosti].mempos ) {
		return 0; 
	}
	
	_dbPairFileVec[hosti].kv  = JagFixString( _writebuf[hosti]+memoff, _datalen, _datalen );
	_dbPairFileVec[hosti].memoff += _datalen;

	return 1;
}

int JagDataAggregate::_joinReadFromDisk( JagVector<JagFixString> &vec )
{
	int cnt = _mergeReader->getNext( vec );
	return cnt;
}

void JagDataAggregate::shuffleSQLMemAndFlush()
{
	srand(time(NULL));
	int radidx;
	while ( true ) {
		radidx = rand() % JAG_RANDOM_FILE_LIMIT;
		_dbPairFileVec[0].fpath = _dirpath + _dbobj + "." + intToStr( radidx ) + ".sql";
		if ( _jfsMgr->exist( _dbPairFileVec[0].fpath ) ) {
			continue;
		} 

        dn("a200819 openfd [%s]", _dbPairFileVec[0].fpath.s() );
		_dbPairFileVec[0].fd = _jfsMgr->openfd( _dbPairFileVec[0].fpath, true );
		break;
	}

	for ( int i = 0; i < _dbPairFileVec[0].mempos; ++i ) {
		raysafewrite( _dbPairFileVec[0].fd, (char*)_sqlarr[i].c_str(), _sqlarr[i].size() );
		_sqlarr[i] = "";
	}
	
	_jfsMgr->closefd( _dbPairFileVec[0].fpath );
}

jagint JagDataAggregate::sendDataToClient( jagint cnt, JagRequest &req )
{
	// disallow schem, host updates to client
	//req.session->noInterrupt = true;
    dn("s0038873 JagDataAggregate::sendDataToClient()  cnt=%ld ...", cnt );

	jagint len;
	char *ptr = NULL;
	char buf[SELECT_DATA_REQUEST_LEN+1];
	memset(buf, 0, SELECT_DATA_REQUEST_LEN+1);

	sprintf( buf, "_datanum|%lld|%lld", cnt, this->getdatalen() );
	//sendMessage( req, buf, "OK" );
	dn("sa12341 sendDataMore [%s] ...", buf);
	sendOKMore( req, buf );  // 1/12/2023 added
	dn("sa12341 sendDataMore [%s] done", buf);

 	char hdr[JAG_SOCK_TOTAL_HDR_LEN+1];
	char *newbuf = NULL;

	dn("sa09921 in sendDataToClient() recvMessage()...");
	jagint clen = recvMessage( req.session->sock, hdr, newbuf );
	dn("sa09921 in sendDataToClient() recvMessage() done clen=%ld...", clen);
	dn("sa09921 in sendDataToClient() recvMessage() done hdr=[%s] newbuf=[%s]", hdr, newbuf);

	jagint dcnt = 0; 

	if ( clen > 0 && 0 == strncmp( newbuf, "_send", 5 ) ) {
		if ( 0 == strncmp( newbuf, "_senddata|", 10 ) ) { 

			JagStrSplit sp( newbuf, '|', true );
			setread( jagatoll(sp[1].c_str()), jagatoll(sp[2].c_str()) );
            dn("sa4500288 setread (%s  %s)", sp[1].s(), sp[2].s() );
		} 
		
		jagint slen;
		while ( true ) {
			dn("sa602210 readBlock() ...");
			ptr = readBlock( len );
			dn("sa602210 readBlock() done ptr=%p len=%d ...", ptr, len);
			if ( !ptr || len < 0 ) {
				dn("sa320081 readBlock len=%ld break", len);
				break;
			}

            //dn("sa39338 from readBlock dumpmem len=%d:", len);
            //dumpmem(ptr, len);

			dn("sa4008127 inloop sendDataMore(len=%d) ...", len );
			slen = sendDataMore( req, ptr, len );
			dn("sa4008127 inloop sendDataMore(len=%d) done slen=%d", len, slen );

			if ( slen < 0 ) {
				dn("sa322081 sendMessageLength < 0 break slen=%lld", slen);
				break;
			}
			++dcnt;
		}
		dn("sa400887 send datablocks dcnt=%lld", dcnt);
	} else {
		dn("sa033300 recvMessage from client, error newbuf=[%s] clen=%lld no sendDataMore !!!!!!!!!!!!!!!!", newbuf, clen);
	}

	if ( newbuf ) free( newbuf );
	
	// allow schem, host updates to client
	//req.session->noInterrupt = false;
    dn("s0038875 JagDataAggregate::sendDataToClient() done  dcnt=%ld ...", dcnt );

	return dcnt;
}

void JagDataAggregate::initWriteBuf()
{
	for ( int i=0; i < JAG_MAX_HOSTS; ++i ) {
		_writebuf[i] = NULL;
	}
	_writebufHasData = false;
}

void JagDataAggregate::cleanWriteBuf()
{
	if ( ! _writebufHasData ) return;
	for ( int i=0; i < JAG_MAX_HOSTS; ++i ) {
		if ( _writebuf[i] ) {
			free( _writebuf[i] );
			_writebuf[i] = NULL;
		}
	}

	_writebufHasData = false;
}

