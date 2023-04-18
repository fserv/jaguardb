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
#ifndef _jag_merge_back_navig_h_
#define _jag_merge_back_navig_h_

#include <abax.h>
#include <JagMergeReaderBase.h>
#include <JagDBMap.h>

class JagMergeBackReader : public JagMergeReaderBase
{
  public:
	JagMergeBackReader( const JagDBMap *dbmap, const JagVector<OnefileRange> &fRange, 
					    int keylen, int vallen, const char *minbuf, const char *maxbuf );
  	virtual ~JagMergeBackReader(); 	

  	virtual bool getNext( char *buf );

	void findMemBeginPos( const char *minbuf, const char *maxbuf );
	void setRestartPos();
	void moveToRestartPos();
	void  initHeap();

	/////// new: similiar to map begin means rbegin() direction and end means rend() direction
    void    setMemRestartPos();
    void    moveMemToRestartPos();
    bool    isAtREnd( JagFixMapReverseIterator iter );  // rend(): left en
    void    print( const char *hdr,  JagFixMapReverseIterator iter );

    JagFixMapReverseIterator   beginPos;
    JagFixMapReverseIterator   endPos;
    JagFixMapReverseIterator   prevPos;
    JagFixMapReverseIterator   currentPos;

  protected:
	JagBuffBackReaderPtr *_buffBackReaderPtr;
	JagFixMapReverseIterator  _restartMemPos;

};


#endif

