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
#ifndef _jag_dbmap_class_h_
#define _jag_dbmap_class_h_
#include <map>
#include <JagTime.h>
#include <JagDBPair.h>
#include <functional>
//#include <safemap.h>
//typedef	std::pair<JagFixString,char>  JagFixValuePair;
//typedef   std::pair<JagFixString,JagFixValuePair>  JFixPair;
//typedef	std::map<JagFixString,JagFixValuePair>  JagFixMap;

typedef		std::map<JagFixString,JagFixString>  JagFixMap;
typedef   	JagFixMap::iterator  JagFixMapIterator;
typedef   	JagFixMap::reverse_iterator  JagFixMapReverseIterator;

class JagMergeSeg
{
  public:
    int simpfPos;
    JagFixMapIterator leftIter;
    JagFixMapIterator rightIter;
	void print() {
		printf("simpfPos=%d left=[%s][%s] right=[%s][%s]\n", 
				simpfPos, leftIter->first.c_str(), leftIter->second.c_str(), 
				rightIter->first.c_str(), rightIter->second.c_str() );

	}
};

class JagRequest;
class ExprElementNode;
class JagParseParam;
class JagSchemaAttribute;
class JagDBPair;
class JagDBServer;

class JagDBMap
{
	public:
		JagDBMap( );
		~JagDBMap();

		bool    insert( const JagDBPair &newpair );
		bool    exist( const JagDBPair &pair ) const;
		bool    remove( const JagDBPair &pair );
		bool    get( JagDBPair &pair ) const; 
		bool    set( const JagDBPair &pair ); 
		void    destroy();
		void    clear() { _map->clear(); }
		jagint  size() const { return _map->size(); }
		jagint  elements() const { return _map->size(); }
		void    print() const;
		JagFixMapIterator getPred( const JagDBPair &pair ) const; // _map->end() if not found
		JagFixMapIterator getPredOrEqual( const JagDBPair &pair ) const;// _map->end() if not found
		JagFixMapIterator getSucc( const JagDBPair &pair ) const; // _map->end() if not found
		JagFixMapIterator getSuccOrEqual( const JagDBPair &pair ) const; // _map->end() if not found
		bool    isAtEnd( const JagFixMapIterator &iter ) const;
		void    setToEnd( JagFixMapIterator &iter ) const;
		JagFixMapIterator getFirst() const; // _map->end() if empty
		JagFixMapIterator getLast() const;  // _map->end() if empty
		void    iterToPair( const JagFixMapIterator& iter, JagDBPair& pair ) const;
		bool    setWithRange( const JagDBServer *servobj, const JagRequest &req, JagDBPair &pair, 
						   const char *buffers[], bool uniqueAndHasValueCol,
		                   ExprElementNode *root, const JagParseParam *parseParam, int numKeys,
			               const JagSchemaAttribute *schAttr, jagint KLEN, jagint VLEN, jagint setposlist[], JagDBPair &retpair );

		void reverseIterToPair( const JagFixMapReverseIterator& iter, JagDBPair& pair ) const;
		JagFixMapReverseIterator getReversePredOrEqual( const JagDBPair &pair ) const;// _map->rend() if not found
		JagFixMapReverseIterator getReverseSuccOrEqual( const JagDBPair &pair ) const; // _map->rend() if not found
		bool isAtREnd( const JagFixMapReverseIterator &iter ) const;

		JagFixMap *_map; 

	protected:
		void init();

};
#endif
