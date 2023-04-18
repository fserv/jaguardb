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
#ifndef _jag_hotspot_h_
#define _jag_hotspot_h_

#include <abax.h>
#include <JagSimpleBoundedArray.h>

typedef AbaxPair<AbaxLong, AbaxDouble> HotSpotPair;
//class JagReadWriteLock;

template <class Pair>
class JagHotSpot
{
  public:
  	JagHotSpot(int bound=1000);
	~JagHotSpot();

	int insert( jagint usec, double spot, const Pair &pair );
	int updateUpsAndDowns( const Pair &dbpair );
	int goingRight() const;
	int getResizeMode() const;
	void setMaxPair( const Pair &dbpair );
	void clean();

  protected:
  	int 		_bound;
  	JagSimpleBoundedArray<HotSpotPair> *_barr;
	pthread_rwlock_t    *_lock;
	Pair  				_oldPair;
	Pair  				_prevResizeMaxPair;
	jaguint  _ups;
	jaguint  _downs;
	jaguint  _mups;
	jaguint  _mdowns;

	jagint  _uds;
	jagint  _locs;
};


template <class Pair>
JagHotSpot<Pair>::JagHotSpot(int bound)
{
	_barr = new JagSimpleBoundedArray<HotSpotPair>( bound );
	//_lock = newJagReadWriteLock();
	_lock = NULL;
	_ups = _downs = _mups = _mdowns = 1;

	_uds = 0;
	_locs = 0;
}

template <class Pair>
JagHotSpot<Pair>::~JagHotSpot()
{
	delete _barr;
	//delete _lock;
	deleteJagReadWriteLock( _lock );
}

template <class Pair>
int JagHotSpot<Pair>::updateUpsAndDowns( const Pair &dbpair )
{
	JagReadWriteMutex mutex( _lock, JagReadWriteMutex::WRITE_LOCK );
	// d("s3834 updateUpsAndDowns  this=%0x  thrd=%lld\n", this, THREADID );
	if ( dbpair > _oldPair ) {
		++ _ups;
	}  else if ( dbpair < _oldPair ) {
		++_downs;
	}
	_oldPair = dbpair;

	if ( dbpair > _prevResizeMaxPair ) {
		++ _mups;
		// _maxPair = dbpair;
	}  else if ( dbpair < _prevResizeMaxPair ) {
		++_mdowns;
	}
	+_uds;
	// d("s3831 _uds=%lld\n", _uds );
	return 1;
}


template <class Pair>
int JagHotSpot<Pair>::insert( jagint usec, double spot, const Pair &dbpair )
{
	HotSpotPair hpair;
	hpair.key = usec;
	hpair.value = spot;
	// d("s8381 hotspot insert %.2f  this=%0x THREAD=%lld\n", spot, this, THREADID );

	JagReadWriteMutex mutex( _lock, JagReadWriteMutex::WRITE_LOCK );
	_barr->insert( hpair );
	++ _locs;
	//d("s3832 _locs=%lld\n", _locs );

	/***
	if ( dbpair > _oldPair ) {
		++ _ups;
	}  else if ( dbpair < _oldPair ) {
		++_downs;
	}
	_oldPair = dbpair;
	***/
	return 1;
}

// hot spot says keys are ascending
// return 1: if keys are on the right side or are purely ascending 
// return 0: keys are randomly distributed
template <class Pair>
int JagHotSpot<Pair>::goingRight() const
{
	JagReadWriteMutex mutex( _lock, JagReadWriteMutex::READ_LOCK );

	double res = 0.0;
	int cnt = 1;

	_barr->begin();
	HotSpotPair pair;
	while ( _barr->next( pair ) ) {
		res += pair.value.value();
		++cnt;
	}

	res = res /(double)cnt;

	if ( res >= 0.7 ) {
		// d("s3822 goingRight  ups=%lld  downs=%lld this=%0x thrd=%lld res >=0.7\n", _ups, _downs, this, THREADID );
		return 1;
	} else {
		// d("s3823 goingRight  ups=%lld  downs=%lld this=%0x thrd=%lld\n", _ups, _downs, this, THREADID );
		/***
		if ( _ups/_downs >= 7 ) {
			return 1;
		}
		***/
	}

	return 0;
}

// get resize mode based on hotspot, up/down value and max up/down value
// return STRICT_ASCENDING if hotspot is large, and up/down value and max up/down value both large
// return STRICT_RANDOM if hotspot is small, and max up/down alue is small
// return CYCLE_RIGHT for other conditions
template <class Pair>
int JagHotSpot<Pair>::getResizeMode() const
{
	JagReadWriteMutex mutex( _lock, JagReadWriteMutex::READ_LOCK );

	double res = 0.0;
	int cnt = 1;
	int udp = 0, mudp = 0;

	_barr->begin();
	HotSpotPair pair;
	while ( _barr->next( pair ) ) {
		res += pair.value.value();
		++cnt;
	}

	res = res /(double)cnt;
	udp = _ups/_downs;
	mudp = _mups/_mdowns;

	if ( res >= JAG_HOTSPOT_LIMIT && udp >= JAG_UPDOWN_LIMIT && mudp >= JAG_UPDOWN_LIMIT ) {
		return JAG_STRICT_ASCENDING;
	} else if ( res < JAG_HOTSPOT_MINLIMIT && mudp < JAG_UPDOWN_LIMIT ) {
		return JAG_STRICT_ASCENDING_LEFT;
	} else if ( res < JAG_HOTSPOT_LIMIT && mudp < JAG_UPDOWN_LIMIT ) {
		return JAG_STRICT_RANDOM;
	} else {
		if ( mudp < 1 ) { // see as random
			return JAG_STRICT_RANDOM;
		}

		if ( mudp < JAG_UPDOWN_LIMIT/2 ) {
			return JAG_CYCLE_RIGHT_SMALL;
		} else {
			return JAG_CYCLE_RIGHT_LARGE;
		}
	}
	return 0;
}

template <class Pair>
void JagHotSpot<Pair>::setMaxPair( const Pair &dbpair )
{
	JagReadWriteMutex mutex( _lock, JagReadWriteMutex::WRITE_LOCK );
	// _maxPair = dbpair;
	_prevResizeMaxPair = dbpair;
}

template <class Pair>
void JagHotSpot<Pair>::clean()
{
	JagReadWriteMutex mutex( _lock, JagReadWriteMutex::WRITE_LOCK );
	_ups = _downs = _mups = _mdowns = 1;
	_oldPair = Pair::NULLVALUE;
	_prevResizeMaxPair = Pair::NULLVALUE;
	_barr->clean();
}

#endif
