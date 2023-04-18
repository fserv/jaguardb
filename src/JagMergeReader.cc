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

#include <JagMergeReader.h>
#include <JagBuffReader.h>


JagMergeReader::JagMergeReader( const JagDBMap *dbmap, const JagVector<OnefileRange> &fRange,
                                int keylen, int vallen, const char *minbuf, const char *maxbuf )
	:JagMergeReaderBase(dbmap, fRange.size(), keylen, vallen, minbuf, maxbuf )
{
	_isMarkSet = false;
    int veclen = fRange.size();

	findMemBeginPos( minbuf, maxbuf );

	if ( veclen > 0 ) {
		_buffReaderPtr = new JagBuffReaderPtr[veclen];
	} else {
		_buffReaderPtr = NULL;
	}

    dn("s20333810 JagMergeReader ctor keylen=%d vallen=%d veclen=%d", keylen, vallen, veclen );
	for ( int i = 0; i < veclen; ++i ) {
        dn("s870012 new JagBuffReader i=%d startpos=%ld readlen=%ld", i, fRange[i].startpos, fRange[i].readlen );
		_buffReaderPtr[i] = new JagBuffReader( (JagDiskArrayBase*)fRange[i].darr, fRange[i].readlen, 
									 			KEYLEN, VALLEN, fRange[i].startpos, 0, fRange[i].memmax );
	}

	initHeap();
}

JagMergeReader::~JagMergeReader()
{
	if ( _buffReaderPtr ) {
		for ( int i = 0; i < _readerPtrlen; ++i ) {
			if ( _buffReaderPtr[i] ) {
				delete _buffReaderPtr[i];
				_buffReaderPtr[i] = NULL;
			}
		}
        
		delete [] _buffReaderPtr;
		_buffReaderPtr = NULL;
	}

}

void JagMergeReader::initHeap()
{
	if ( _pqueue ) { delete _pqueue; }

    // compare first pair from each file/mem
    _pqueue = new JagPriorityQueue<int,JagDBPair>(256, JAG_MINQUEUE );

	if ( this->_dbmap && this->_dbmap->size() > 0 ) {

        /***
        dn("s002281 JagMergeReader::initHeap beginPair < endPair:");
        dn("s00028 beginPair:");
        beginPair.printkv(true);
        dn("s00028 endPair:");
        endPair.printkv(true);
        ***/

		if ( beginPair <= endPair && ! memReadDone ) {
    		_pqueue->push( -1, beginPair );

            //dn("s8810212 pushed to pqueue beginPair:");
            //beginPair.printkv(true);

    		currentPos = beginPos;

    		if ( currentPos == endPos ) {
    			memReadDone = true;
    		}
		} else {
    		memReadDone = true;
		}
	}  else {
    	memReadDone = true;
    }

	int rc;
	jagint pos;

	for ( int i = 0; i < _readerPtrlen; ++i ) {
		_goNext[i] = 1;
	}

	for ( int i = 0; i < _readerPtrlen; ++i ) {

		rc = _buffReaderPtr[i]->getNext( _buf, pos );
        dn("s760023 initHeap() i=%d _readerPtrlen=%lld _buffReaderPtr[i].getNext->rc=%d", i, _readerPtrlen, rc );

		if ( rc ) {
			JagDBPair pair(_buf, KEYLEN, VALLEN );

            /**
            dn("s020283 initHeap() bufrr.getNext() got a pair from bufrdr[%i] pair:", i );
            pair.printkv(true);
            dn("s02928111 endpair:");
            endPair.printkv(true);
            dumpmem(_buf, KEYLEN+VALLEN, true);
            // _buf: key:11=[-9999999984 val:17=[-0000000250@@@@@@]
            **/

			if ( pair <= endPair ) {
				_pqueue->push( i, pair );
				_goNext[i] = 0;
                dn("s300087 initHeap() bufrdr i=%d pushed pair", i);
			} else {
				_goNext[i] = -1;
				++_endcnt;
                dn("s022062 gonext[i=%d] --> -1  at end", i );
			}

		} else {
            // i at end
            //dn("s022882 gonext[i=%d] --> -1  at end", i );
			_goNext[i] = -1;
			++_endcnt;
		}
	}
}

bool JagMergeReader::getNext( char *buf )
{
	JagDBPair   pair;
	int         rc, filenum;
	jagint      pos;

	if ( ! _pqueue->pop( filenum, pair ) ) {
        dn("s6152002 _pqueue->pop false, return false");
		return false;
	}

    /**
    d("s670034 popped pair: ");
    pair.printkv(true);
    **/

	memcpy(buf, pair.key.c_str(), KEYLEN);
	memcpy(buf+KEYLEN, pair.value.c_str(), VALLEN);

    // filenum == -1 means memory
	if ( filenum == -1 ) {
		if ( ! memReadDone  ) {
            //dn("s17170 memReadDone is false");

			prevPos = currentPos;
			//print("in getNext() prevPos: ", prevPos );
    		++currentPos;

    		JagDBPair nextpair;
    		nextpair.key = currentPos->first;
    		nextpair.value = currentPos->second;

            /**
            dn("s029280 pushed mempair to _pqueue ");
            nextpair.printkv(true);
            **/

    		_pqueue->push( -1, nextpair );

    		if ( currentPos == endPos ) {
                dn("s230074 currentPos=%ld == endPos=%ld memReadDone=true", currentPos, endPos );
    			memReadDone = true;
			}
		}
	} else {

        if ( -1 == _goNext[filenum] ) {
            dn("s3192807 file=%d is -1 return true", filenum);
	        return true;
        }

		_goNext[filenum] = 1; 

		rc = _buffReaderPtr[filenum]->getNext( _buf, pos );
        dn("s0038 filenum=%d _buffReaderPtr.getNext rc=%d", filenum, rc );

		if ( rc ) {
			JagDBPair nextpair(_buf, KEYLEN, VALLEN );

            /**
            dn("s012288 getNext() got buf and nextpair from dbfile: %d", filenum);
            dumpmem(_buf, KEYLEN+VALLEN, true);
            nextpair.printkv(true);
            **/

			if ( nextpair <= endPair ) {
                d("s7811203 getNext() nextpair<endpair diskdata pushed pqueue filenum=%d, nextpair: ", filenum);
                //nextpair.printkv(true);

				_pqueue->push( filenum, nextpair );

			} else {
                dn("s661272 _goNext[%d] = -1 at end", filenum );
				_goNext[filenum] = -1; 
			}
		} else {
			_goNext[filenum] = -1;  // end of file for this filenum
            dn("s601572 _goNext[%d} = -1 ended", filenum );
		}
	}

    dn("s01928 return true");
	return true;
}

#if 0
void JagMergeReader::setRestartPos()
{
	setMemRestartPos();

	for ( int i = 0; i < _readerPtrlen; ++i ) {
        dn("s010102 bufrdr i=%d setRestartPos", i );
		_buffReaderPtr[i]->setRestartPos();
	}

	_isMarkSet = true;
}

void JagMergeReader::moveToRestartPos()
{
    dn("s1092003 moveToRestartPos moveMemToRestartPos()");
	moveMemToRestartPos();

	currentPos = _restartMemPos;
    print("s03038381 currentPos=:", currentPos);

	if ( ! isAtEnd( currentPos ) ) {
		JagDBPair restartPair( currentPos->first.c_str(), KEYLEN, currentPos->second.c_str(), VALLEN );
   		_pqueue->clear();
   		_pqueue->push( -1, restartPair );
        dn("currentPos not at end, pqueue is cleared, pushed restartPair:");
        restartPair.printkv(true);
	} else {
	}

	for ( int i = 0; i < _readerPtrlen; ++i ) {
        dn("s7722801 _buffReaderPtr(%d) moveToRestartPos", i);
		_buffReaderPtr[i]->moveToRestartPos();
	}
}
#endif

void JagMergeReader::findMemBeginPos( const char *minbuf, const char *maxbuf )
{
    dn("s450037 findMemBeginPos: ");

    /**
    dn("s70321 in findMemBeginPos minbuf:");
    dumpmem(minbuf, KEYLEN, true);
    dn("s70322 in findMemBeginPos maxbuf:");
    dumpmem(maxbuf, KEYLEN, true);
    **/

    // Required
	beginPair = JagDBPair( minbuf, KEYLEN ); 
	endPair = JagDBPair( maxbuf, KEYLEN );  

    if ( ! _dbmap ) {
        dn("s020777 findMemBeginPos not _dbmap, memReadDone = true");
		memReadDone = true;
        return;
    }

	beginPos = _dbmap->getSuccOrEqual( beginPair );
	if ( isAtEnd( beginPos) ) {
        dn("s32003 memReadDone");
		memReadDone = true;
		return;
	}

	_dbmap->iterToPair( beginPos, beginPair );

    /***
    dn("s8112265 beginPair printkv()");
    beginPair.printkv(true);
    print("s0038588 beginPos:", beginPos);
    ***/

	endPos = _dbmap->getPredOrEqual( endPair ); 

    /***
    dn("s8812265 endPair printkv()");
    endPair.printkv(true);
    print("s0038288 endPos:", endPos);
    ***/

	if ( isAtEnd( endPos ) ) {
        dn("s32009 endPos is at end, memReadDone");
		memReadDone = true;
	}

    dn("s32049 memReadDone=%d", memReadDone);

}

#if 0
void JagMergeReader::setMemRestartPos()
{
    dn("s450036 setMemRestartPos _restartMemPos = prevPos ");
	_restartMemPos = prevPos; 
    print("s2222088 _restartMemPos to prevPos:", _restartMemPos);

	if ( isAtEnd(prevPos) ) {
		memReadDone = true;
        dn("s017332 prevPos is at end memReadDone true");
	} else {
        dn("s017332 prevPos is not at end memReadDone=%d", memReadDone);
    }
}

void JagMergeReader::moveMemToRestartPos()
{
    dn("s101019 moveMemToRestartPos set memReadDone=false");
	beginPos = _restartMemPos;
	currentPos = _restartMemPos;
	memReadDone = false;

    print("s020299 beginPos=currentPos=_restartMemPos:", beginPos);

	if ( isAtEnd(currentPos) ) {
        dn("s222039 currentPos at end, memReadDone");
		memReadDone = true;
	}
}
#endif


bool JagMergeReader::isAtEnd( JagFixMapIterator iter )
{
	if ( NULL == _dbmap ) return true;

	if ( _dbmap->isAtEnd(iter) ) {
        return true;
    }

	return false;
}

void JagMergeReader::print( const char *hdr, JagFixMapIterator iter )
{
	i("%0x s82369 print %s iter=[%s][%s]\n", this, hdr, iter->first.c_str(), iter->second.c_str() );
}

