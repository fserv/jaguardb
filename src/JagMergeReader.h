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
#ifndef _jag_merge_navig_h_
#define _jag_merge_navig_h_

#include <abax.h>
#include <JagVector.h>
#include <JagTableUtil.h>
#include <JagMergeReaderBase.h>
#include <JagDBMap.h>


class JagMergeReader : public JagMergeReaderBase
{
  public:
	JagMergeReader( const JagDBMap *dbmap,  const JagVector<OnefileRange> &fRange, 
					int keylen, int vallen, const char *minbuf, const char *maxbuf );
  	virtual ~JagMergeReader(); 	

  	virtual bool getNext( char *buf );

	void findMemBeginPos( const char *minbuf, const char *maxbuf );
	void setRestartPos();
	void moveToRestartPos();
	void initHeap();

    void    setMemRestartPos();
    void    moveMemToRestartPos();
    bool    isAtEnd( JagFixMapIterator iter );
    void    print( const char *hdr,  JagFixMapIterator iter );

    JagFixMapIterator   beginPos;
    JagFixMapIterator   endPos;
    JagFixMapIterator   prevPos;
    JagFixMapIterator   currentPos;

  protected:
	JagBuffReaderPtr *_buffReaderPtr;
	JagFixMapIterator  _restartMemPos;
};


#endif

