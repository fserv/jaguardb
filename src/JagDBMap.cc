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
#include <JagDBMap.h>
#include <JagRequest.h>
#include <JagDiskArrayBase.h>


// ctor
JagDBMap::JagDBMap( )
{
	init();
}

// dtor
JagDBMap::~JagDBMap( )
{
	destroy();
}

void JagDBMap::destroy( )
{
	if ( _map ) {
		_map->clear();
		delete _map;
		_map = nullptr;
	}
}

void JagDBMap::init()
{
	_map = new JagFixMap();
}

// returns false if item actually exists in array
// otherwise returns true
bool JagDBMap::insert( const JagDBPair &newpair )
{
	_map->emplace( newpair.key, newpair.value);
	return true;

}

bool JagDBMap::remove( const JagDBPair &pair )
{
	jagint sz = _map->erase( pair.key );
	if ( sz > 0 ) return true;
	else return  false;
}

bool JagDBMap::exist( const JagDBPair &search ) const
{
	if ( _map->find( search.key ) == _map->end() ) {
		return false;
	}
	
    return true;
}

bool JagDBMap::get( JagDBPair &pair ) const
{
	JagFixMap::iterator it = _map->find( pair.key );
	if ( it == _map->end() ) {
		return false;
	}

	pair.value = it->second;
	return true;
}

bool JagDBMap::set( const JagDBPair &pair )
{
	JagFixMap::iterator it = _map->find( pair.key );
	if ( it == _map->end() ) {
		return false;
	}

	it->second = pair.value;
	//d("s442773 JagDBMap::set iter ==> [%s][%s]\n", it->first.s(), it->second.s() );
	return true;
}


void JagDBMap::print( ) const
{
    for ( JagFixMap::iterator it=_map->begin(); it!=_map->end(); ++it) {
   		printf("17 key=[%s]  --> value=[%s]\n", it->first.c_str(), it->second.c_str() );
    }

}

// lower_bound of k:  find equal or greater than k. It can be at end() is k is greatest.
// upper_bound of k:  find          greater than k. It can be at end() is k is greatest.

// get strict predecesor of pair; if not found return JagFIxMap::begin() -- changed to below
// get strict predecesor of pair; if not found return JagFIxMap::end()
JagFixMapIterator JagDBMap::getPred( const JagDBPair &pair ) const
{
	if ( _map->size() < 1 ) return _map->end();

	JagFixMapIterator itlow = _map->lower_bound( pair.key );
	if ( itlow == _map->begin() ) {
		return _map->end();
	}
	return --itlow;
}

// get strict predecesor or equal of pair; if not found return JagFIxMap::begin()--- changed to below
// get strict predecesor or equal of pair; if not found return JagFIxMap::end()
JagFixMapIterator JagDBMap::getPredOrEqual( const JagDBPair &pair ) const
{
	if ( _map->size() < 1 ) return _map->end();

	JagFixMapIterator itlow = _map->lower_bound( pair.key );
	if ( itlow == _map->end() ) {
		return --itlow; // last real position
	}

	if ( itlow->first == pair.key ) {
		return itlow;
	}

	if ( itlow == _map->begin() ) {
		return _map->end();
	}

	return --itlow;
}

// get strict successor of pair; if not found return JagFIxMap::end()
JagFixMapIterator JagDBMap::getSucc( const JagDBPair &pair ) const
{
	return _map->upper_bound( pair.key );
}

// get strict successor or equal of pair; if not found return JagFIxMap::end()
JagFixMapIterator JagDBMap::getSuccOrEqual( const JagDBPair &pair ) const
{
	return _map->lower_bound( pair.key );
}


// pair: input pair with keys only; output: keys and values of old record
// retpair: keys and new modified values
bool JagDBMap::setWithRange( const JagDBServer *servobj, const JagRequest &req, JagDBPair &pair, const char *buffers[], 
							 bool uniqueAndHasValueCol, ExprElementNode *root, const JagParseParam *parseParam, int numKeys, 
						const JagSchemaAttribute *schAttr, jagint KLEN, jagint VLEN, jagint setposlist[], JagDBPair &retpair )
{
	return set(pair);
}

bool JagDBMap::isAtEnd( const JagFixMapIterator &iter ) const 
{
	if ( _map->size() < 1 ) { return true; }
	if ( iter == _map->end() ) { return true; } // end() is NULL, has no data
	return false;
}


void JagDBMap::setToEnd( JagFixMapIterator &iter ) const
{
	iter = _map->end();
}


// _map->end() if empty
JagFixMapIterator JagDBMap::getFirst() const
{
	return _map->begin(); // it can be end()
}

// _map->end() if empty
JagFixMapIterator JagDBMap::getLast() const
{
	if ( _map->size() < 1 ) { return _map->end(); }
	JagFixMapIterator iter = _map->end();
	return --iter;
}

// assume iter is not end()
void JagDBMap::iterToPair( const JagFixMapIterator& iter, JagDBPair& pair ) const
{
	if ( iter == _map->end() ) return;
	pair.key = iter->first;
	pair.value = iter->second;
}

// assume iter is not rend()
void JagDBMap::reverseIterToPair( const JagFixMapReverseIterator& iter, JagDBPair& pair ) const
{
	if ( iter == _map->rend() ) return;
	pair.key = iter->first;
	pair.value = iter->second;
}

bool JagDBMap::isAtREnd( const JagFixMapReverseIterator &iter ) const 
{
	if ( _map->size() < 1 ) { return true; }
	if ( iter == _map->rend() ) { return true; } // end() is NULL, has no data
	return false;
}

// returns reverse iterator
// get strict predecesor or equal of pair; if not found return JagFixMap::rend()
JagFixMapReverseIterator JagDBMap::getReversePredOrEqual( const JagDBPair &pair ) const
{
	if ( _map->size() < 1 ) return _map->rend();

	JagFixMapReverseIterator revit( _map->upper_bound( pair.key ) );
	return revit;
}

// returns reverse iterator
// get strict succcesor or equal of pair; if not found return JagFixMap::rend()
JagFixMapReverseIterator JagDBMap::getReverseSuccOrEqual( const JagDBPair &pair ) const
{
	if ( _map->size() < 1 ) return _map->rend();

	JagFixMapIterator it = _map->lower_bound( pair.key );
	if ( it == _map->end() ) {
		return _map->rend();
	}

	JagFixMapReverseIterator revit( it );
	return --revit;
}

