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

#include <JagMergeBackReader.h>
#include <JagBuffBackReader.h>
#include <JagUtil.h>


JagMergeBackReader::JagMergeBackReader( const JagDBMap *dbmap, const JagVector<OnefileRange> &fRange, 
										int keylen, int vallen, const char *minbuf, const char *maxbuf )
	: JagMergeReaderBase( dbmap, fRange.size(), keylen, vallen, minbuf, maxbuf )
{
	_isMarkSet = false;
	findMemBeginPos( minbuf, maxbuf );

	if ( fRange.size() > 0 ) {
		_buffBackReaderPtr = new JagBuffBackReaderPtr[fRange.size()];
		//jd(JAG_LOG_HIGH, "s1829 JagBuffBackReader[%d] ...\n", veclen );
	} else {
		_buffBackReaderPtr = NULL;
	}
	
	for ( int i = 0; i < fRange.size(); ++i ) {
		_buffBackReaderPtr[i] = new JagBuffBackReader( (JagDiskArrayBase*)fRange[i].darr, fRange[i].readlen, 
														KEYLEN, VALLEN, fRange[i].startpos, 0, fRange[i].memmax );
		//_goNext[i] = 1;
	}

	initHeap();
}

JagMergeBackReader::~JagMergeBackReader()
{
	if ( _buffBackReaderPtr ) {
		for ( int i = 0; i < _readerPtrlen; ++i ) {
			if ( _buffBackReaderPtr[i] ) {
				delete _buffBackReaderPtr[i];
				_buffBackReaderPtr[i] = NULL;
			}
		}
		delete [] _buffBackReaderPtr;
		_buffBackReaderPtr = NULL;
	}
}

/**********
bool JagMergeBackReader::getNext( char *buf )
{
	jagint pos;
	return getNext( buf, pos );
}

bool JagMergeBackReader::getNext( char *buf, jagint &pos )
{

	if ( _cacheBuf ) {
		pos = 0;
		memcpy(buf, _cacheBuf, KEYVALLEN);
		free( _cacheBuf ); _cacheBuf = NULL;
		return true;
	}

	int beginpos = 0;
	int rc;
	for ( int i = 0; i < _readerPtrlen; ++i ) {
		if ( _goNext[i] == 1 ) {
			rc = _buffBackReaderPtr[i]->getNext( _buf+i*KEYVALLEN, pos );
			if ( !rc ) {
				_goNext[i] = -1;
				++_endcnt;
			} else {
				_goNext[i] = 0;
			}
		}
	}
	
	if ( _endcnt == _readerPtrlen ) {
		return false;
	}
	
	// get initial maxbuf as first avail buf
	for ( int i = beginpos; i < _readerPtrlen; ++i ) {
		if ( _goNext[i] != -1 ) {
			memcpy(buf, _buf+i*KEYVALLEN, KEYVALLEN);
			beginpos = i;
			break;
		}
	}
	
	// find the max buf ( while oldest file first )
	for ( int i = beginpos; i < _readerPtrlen; ++i ) {
		if ( _goNext[i] != -1 ) {
			rc = memcmp(buf, _buf+i*KEYVALLEN, KEYLEN);
			if ( rc <= 0 ) {
				memcpy(buf, _buf+i*KEYVALLEN, KEYVALLEN);
			}
		}
	}
	
	// check and set all other equal buf to getNext
	for ( int i = beginpos; i < _readerPtrlen; ++i ) {
		if ( _goNext[i] != -1 ) {
			rc = memcmp(buf, _buf+i*KEYVALLEN, KEYLEN);
			if ( rc <= 0 ) {			
				if ( _goNext[i] != -1 ) {
					_goNext[i] = 1;
				}
			}
		}
	}
	return true;
}
***/

// read first data from inertbuffer, from each of N files
void JagMergeBackReader::initHeap()
{
	if ( _pqueue ) { delete _pqueue; }
    _pqueue = new JagPriorityQueue<int,JagDBPair>(256, JAG_MAXQUEUE );

    if ( this->_dbmap && this->_dbmap->size() > 0 && ! memReadDone ) {

        if ( beginPair >= endPair ) {
            _pqueue->push( -1, beginPair );

            currentPos = beginPos;
            if ( currentPos == endPos ) {
                memReadDone = true;
            }
        } else {
            memReadDone = true;
        }
    } else {
        memReadDone = true;
    }

    int rc;
    jagint pos;
    for ( int i = 0; i < _readerPtrlen; ++i ) {
        _goNext[i] = 1;
    }

    for ( int i = 0; i < _readerPtrlen; ++i ) {
        rc = _buffBackReaderPtr[i]->getNext( _buf, pos );
        if ( rc ) {
            JagDBPair pair(_buf, KEYLEN, VALLEN );
            if ( pair >= endPair ) {
                _pqueue->push( i, pair );

                _goNext[i] = 0;
            } else {
                _goNext[i] = -1;  // end of data for this filenum
                ++_endcnt;
            }

        } else {
            _goNext[i] = -1;  // end of file for this filenum
            ++_endcnt;
        }
    }


}

bool JagMergeBackReader::getNext( char *buf )
{
	JagDBPair   pair;
	int         rc, filenum;
	jagint      pos;

	if ( ! _pqueue->pop( filenum, pair ) ) {
        dn("s77771 JagMergeBackReader pop done");
		return false;
	}

	//use pair
	memcpy(buf, pair.key.c_str(), KEYLEN);
	memcpy(buf+KEYLEN, pair.value.c_str(), VALLEN);

	// push next item
	if ( filenum == -1 ) {
		// get next item from memory
		if ( ! memReadDone ) {
			prevPos = currentPos;
			++currentPos;
			JagDBPair nextpair; // backward next
			nextpair.key = currentPos->first;
			nextpair.value = currentPos->second;

			_pqueue->push( -1, nextpair );

			if ( currentPos == endPos ) {
				memReadDone = true;
			}
		}
	} else {
		// get next item from filenum
		_goNext[filenum] = 1; 
		rc = _buffBackReaderPtr[filenum]->getNext( _buf, pos );
		if ( rc ) {
			JagDBPair nextpair(_buf, KEYLEN, VALLEN );
			if ( nextpair >= endPair ) {
				_pqueue->push( filenum, nextpair );

			} else {
				_goNext[filenum] = -1;  // end of file for this filenum
			}
		} else {
			_goNext[filenum] = -1;  // end of file for this filenum
		}
	}
	return true;
}


void JagMergeBackReader::findMemBeginPos( const char *minbuf, const char *maxbuf )
{
	beginPair = JagDBPair( maxbuf, KEYLEN ); 
	endPair = JagDBPair( minbuf, KEYLEN ); 

    if ( ! _dbmap ) {
		memReadDone = true;
		return;
    }

	beginPos = _dbmap->getReversePredOrEqual( beginPair );
	if (  isAtREnd( endPos ) ) {
		memReadDone = true;
		return;
	}

	_dbmap->reverseIterToPair( beginPos, beginPair );

	endPos = _dbmap->getReverseSuccOrEqual( endPair );   
	if ( isAtREnd( endPos ) ) {
		memReadDone = true;
	}

}

#if 0
// method to setup restart position for each buffreader ( as current pos when calling this method )
void JagMergeBackReader::setRestartPos()
{
	setMemRestartPos();
	for ( int i = 0; i < _readerPtrlen; ++i ) {
		_buffBackReaderPtr[i]->setRestartPos();
	}
	_isMarkSet = true;	
}

// method to reset pos to restart position set by the above method 
void JagMergeBackReader::moveToRestartPos()
{
	moveMemToRestartPos();

    currentPos = _restartMemPos;
    if ( ! isAtREnd( currentPos ) ) {
        JagDBPair restartPair( currentPos->first.c_str(), KEYLEN, currentPos->second.c_str(), VALLEN );
        _pqueue->clear();
        _pqueue->push( -1, restartPair );
    } else {
    }

	for ( int i = 0; i < _readerPtrlen; ++i ) {
		_buffBackReaderPtr[i]->moveToRestartPos();
	}
}

void JagMergeBackReader::setMemRestartPos()
{
	_restartMemPos = prevPos; 
	if ( isAtREnd(prevPos) ) {
		memReadDone = true;
	}
}

void JagMergeBackReader::moveMemToRestartPos()
{
	beginPos = _restartMemPos;
	currentPos = _restartMemPos;
	memReadDone = false;
	if ( isAtREnd(currentPos) ) {
		memReadDone = true;
	}
}
#endif

bool JagMergeBackReader::isAtREnd( JagFixMapReverseIterator iter )
{
	if ( NULL == _dbmap ) return true;
	if ( _dbmap->isAtREnd(iter) ) return true;
	return false;
}

void JagMergeBackReader::print( const char *hdr, JagFixMapReverseIterator iter )
{
	i("%0x s8827339 print %s iter=[%s][%s]\n", this, hdr, iter->first.c_str(), iter->second.c_str() );
}
