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
#ifndef _jag_exprstack_h_
#define _jag_exprstack_h_

#include <abax.h>
class ExprElementNode;

class JagExprStack
{
	public:
		JagExprStack( int initSize = 4 );
		~JagExprStack();

		void	clean();
		void 	push( ExprElementNode *pair );
		void 	pop();
		ExprElementNode* top() const;
		const   ExprElementNode* operator[](int i) const;
		ExprElementNode*& operator[](int i);
		inline  jagint size() const { return _last+1; }
		inline  jagint capacity() const { return _arrlen; }
		void 	destroy();
		void 	print();
		void 	reAlloc();
		void 	reAllocShrink();
		void    concurrent( bool flag = true );
		inline bool    empty() { if ( _last >=0 ) return false; else return true; }
		int    numOperators;
		int	   lastOp() const;

	protected:
		ExprElementNode **_arr;
		jagint  	_arrlen;
		jagint  	_last;
		static const int _GEO  = 2;
		bool        _isDestroyed;
};


#endif
