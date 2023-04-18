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
#include <JagExprStack.h>
#include <JagParseExpr.h>

JagExprStack::JagExprStack( int initSize )
{
	_arr = new ExprElementNode*[initSize];
	_arrlen = initSize;
	_last = -1;
	_isDestroyed = false;

	numOperators = 0;
}

void JagExprStack::destroy( )
{
	if ( ! _arr || _isDestroyed ) { return; }
	ExprElementNode *p;
	while ( ! empty() ) {
		p = top();
		delete p;
		pop();
	}

	if ( _arr ) delete [] _arr;
	_arr = NULL;
	_arrlen = 0;
	_isDestroyed = true;
}


JagExprStack::~JagExprStack( )
{
	destroy();
}

void JagExprStack::clean()
{
	destroy();
}

void JagExprStack::reAlloc()
{
	jagint i;
	jagint newarrlen  = _GEO*_arrlen; 
	ExprElementNode **newarr;

	newarr = new ExprElementNode *[newarrlen];
	for ( i = 0; i <= _last; ++i) {
		newarr[i] = _arr[i];
	}
	if ( _arr ) delete [] _arr;
	_arr = newarr;
	newarr = NULL;
	_arrlen = newarrlen;
}

void JagExprStack::reAllocShrink()
{
	jagint i;
	ExprElementNode **newarr;

	jagint newarrlen  = _arrlen/_GEO; 
	newarr = new ExprElementNode*[newarrlen];
	for ( i = 0; i <= _last; ++i) {
		newarr[i] = _arr[i];
	}

	if ( _arr ) delete [] _arr;
	_arr = newarr;
	newarr = NULL;
	_arrlen = newarrlen;
}

void JagExprStack::push( ExprElementNode *newnode )
{
	/***
	if ( newnode->_isElement ) {
		d("s9282 ****** pushed %p iselement\n", newnode );
	} else {
		d("s9282 ****** pushed %p isbinopnode op=%d\n", newnode, newnode->getBinaryOp() );
	} 
	***/

	if ( NULL == _arr ) {
		_arr = new ExprElementNode*[4];
		_arrlen = 4;
		_last = -1;
	}

	if ( _last == _arrlen-1 ) { reAlloc(); }

	++ _last;
	_arr[_last] = newnode;

	if ( ! newnode->_isElement ) { ++ numOperators; }
}

ExprElementNode* JagExprStack::top() const
{
	if ( _last < 0 ) {
		throw 2920;
	} 
	return _arr[ _last ];
}


void JagExprStack::pop()
{
	if ( _last < 0 ) { return; } 

	if ( ! _arr[_last]->_isElement ) { 
		-- numOperators; 
	}
	-- _last;
}


void JagExprStack::print()
{
	jagint i;
	printf("c3012 JagExprStack this=%p _arrlen=%lld _last=%lld \n", this, _arrlen, _last );
	for ( i = 0; i  <= _last; ++i) {
		printf("%09lld  %p\n", i, _arr[i] );
	}
	printf("\n");
}

int JagExprStack::lastOp() const
{
	int op = -1;
	for ( int i = _last; i >=0; --i ) {
		if ( ! _arr[i]->_isElement ) {
			op =  _arr[i]->getBinaryOp();
			break;
		}
	}
	return op;
}


const ExprElementNode* JagExprStack::operator[](int i) const 
{ 
	if ( i<0 ) { i=0; }
	else if ( i > _last ) { i = _last; }
	return _arr[i];
}

ExprElementNode*& JagExprStack::operator[](int i) 
{ 
	if ( i<0 ) { i=0; }
	else if ( i > _last ) { i = _last; }
	return _arr[i];
}
