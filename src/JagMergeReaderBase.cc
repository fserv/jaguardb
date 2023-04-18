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

#include <JagMergeReaderBase.h>
#include <JagUtil.h>
#include <JagDBPair.h>
#include <JagDBMap.h>

JagMergeReaderBase::JagMergeReaderBase( const JagDBMap *dbmap, int veclen, int keylen, int vallen, 
										const char *minbuf, const char *maxbuf )
    :_dbmap( dbmap )
{
	_setRestartPos = 0;
	_endcnt = 0;
	_readerPtrlen = veclen;
	KEYLEN = keylen;
	VALLEN = vallen;
	KEYVALLEN = keylen + vallen;
	
	_goNext = NULL; 
	if ( _readerPtrlen > 0 ) {
		_goNext =(int*)calloc( _readerPtrlen, sizeof(int));
	}

	_buf = (char*)jagmalloc(KEYVALLEN+1);
	_cacheBuf = NULL;

	memReadDone = false;
	_pqueue = NULL;
}

JagMergeReaderBase::~JagMergeReaderBase()
{
	if ( _goNext ) {
		free ( _goNext );
	}
	
	if ( _buf ) {
		free ( _buf );
	}

	if ( _cacheBuf ) {
		free( _cacheBuf );
	}

	if ( _pqueue ) {
		delete _pqueue;
	}
	
}

void JagMergeReaderBase::putBack( const char *buf )
{
	if ( _cacheBuf ) {
		free( _cacheBuf );
	}

	_cacheBuf = (char*)jagmalloc(KEYVALLEN + 1 );
	memcpy( _cacheBuf, buf, KEYVALLEN );
}


void JagMergeReaderBase::unsetMark()
{
    _isMarkSet = false;
}

bool JagMergeReaderBase::isMarked() const
{
    return _isMarkSet;
}

