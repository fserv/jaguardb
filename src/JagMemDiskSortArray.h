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
#ifndef _jag_memdisk_sort_array_h_
#define _jag_memdisk_sort_array_h_

#include <abax.h>
#include <JagDef.h>
#include <JagDBPair.h>

class JagBuffReader;
class JagBuffBackReader;
class JagSchemaRecord;
//class JagDiskArrayClient;
class JagDataAggregate;
class JagReadWriteLock;
template <class Pair> class JagArray;

// if _rwtype == 0, no read and write allowed, but allowed to call beginWrite;
// call begingWrite set _rwtype == 1, and write but no read allowed, also allowed to call endWrite;
// call endWrite set _rwtype == 2, no read and write allowed, but allowed to call beginRead;
// call beginRead set _rwtype == 3, and read but no write allowed, also allowed to call endRead;
// call endRead set _rwtype == 0, back to the first condition above
class JagMemDiskSortArray
{
  public:
	JagMemDiskSortArray();
	~JagMemDiskSortArray();
	
	void init( int memlimit=0, const char *diskhdr=NULL, const char *opstr=NULL );
	void setJDA( JagDataAggregate *jda, bool isback=0 );
	void setClientUseStr( const Jstr &selcnt, const Jstr &selhdr, const JagFixString &aggstr );
	void clean();
	int beginWrite();
	int insert( JagDBPair &pair );
	int endWrite();
	int beginRead( bool isback=0 );
	int setDataLimit( jagint num );
	int ignoreNumData( jagint num );
	// int get( JagFixString &str );
	int get( char *buf );
	int endRead();

	int groupByUpdate( const JagDBPair &pair );
	void groupByValueCalculation( const JagDBPair &pair, JagDBPair &oldpair );
	
	int _keylen;
	int _vallen;
	int _kvlen;
	JagArray<JagDBPair> *_memarr;
	
  protected:

	//JagDiskArrayClient *_diskarr;

	JagDataAggregate *_jda;
	
	Jstr _diskhdr;
	Jstr _opstr;
	
	jagint _mempos;
	JagSchemaRecord *_srecord;
	JagBuffReader *_ntr;
	JagBuffBackReader *_bntr;

	pthread_mutex_t _umutex;

	jagint _memlimit; // in bytes
	int _rwtype;
	bool _usedisk;
	bool _isback;
	bool _hassort;
	bool _rend;

	jagint _cnt;
	jagint _cntlimit;

	// data members for client reply use
	Jstr _cntstr;
	Jstr _selhdr;
	JagFixString _aggstr;

};

#endif
