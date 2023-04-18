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
#include <stdio.h>
#include <string.h>
#include <JagStrSplit.h>

JagStrSplit::JagStrSplit()
{
	list_ = NULL;
	length_ = 0;
	start_ = 0;
	_NULL = "";
	pdata_ = NULL;
}

JagStrSplit::JagStrSplit(const Jstr& str, char sep, bool ignoreregion )
{
	list_ = NULL;
	length_ = 0;
	start_ = 0;
	_NULL = "";
	init( str.c_str(), -1, sep, ignoreregion );
}

JagStrSplit::JagStrSplit(const Jstr& str, int fields, char sep, bool ignoreregion )
{
	list_ = NULL;
	length_ = 0;
	start_ = 0;
	_NULL = "";
	init( str.c_str(), fields, sep, ignoreregion );
}

JagStrSplit::JagStrSplit(const char *str, char sep, bool ignoreregion )
{
	list_ = NULL;
	length_ = 0;
	start_ = 0;
	_NULL = "";
	init( str, -1, sep, ignoreregion );
}

void JagStrSplit::init(const char *str, int fields, char sep, bool ignoreregion )
{
	destroy();
	char *p;

	pdata_ = str;

	list_ = NULL;
	length_ = 0;

	sep_ = sep;
	if ( str == NULL  || *str == '\0' ) return;

	char *start, *end, *ps;
	int len;
	int tokens=1;

	p = (char*) str;
	if ( ignoreregion ) { while ( *p == sep_ ) { ++p; } }

    if ( fields < 0 ) {
    	while ( *p != '\0' ) {
    		if ( *p == sep_ ) {
    			if ( ignoreregion ) {
    				while( *p == sep_ ) ++p;
    				if ( *p == '\0' ) break;
    			} 
    			tokens ++;
    		}
    		++p;
    	}
    } else {
        tokens = fields;
    }

	list_ = new Jstr[tokens];
	length_ = tokens;

	start = ps = (char*) str;
	if ( ignoreregion ) {
		while ( *ps == sep_ ) { ++start; ++ps; }
	}

	end = start;
	int i = 0;
	while(  i <= tokens -1 )
	{
		for( end=start; *end != sep_ && *end != '\0'; end++ ) { ; }
		
		len= end-start;
		if ( len == 0 ) {
			list_[i] = "";
		} else {
			list_[i] = Jstr(start, len);
		}

		i++;
		if ( *end == '\0' ) {
			break;
		}

		end++;
		if ( ignoreregion ) {
			while ( *end != '\0' && *end == sep_ ) ++end;
		}
		start = end;
	}

}

JagStrSplit::~JagStrSplit()
{
	destroy();
}

void JagStrSplit::destroy()
{
	if ( list_ ) {
		delete [] list_;
	}
	list_ = NULL;
	length_=0;
}

const Jstr& JagStrSplit::operator[](int i ) const
{
	if ( i+start_ < 0 ) return _NULL;

	if ( i < length_ - start_ )
	{
		return list_[start_+i];
	}
	else
	{
		return _NULL; 
	}
}

Jstr& JagStrSplit::operator[](int i ) 
{
	if ( i+start_ < 0 ) return _NULL;

	if ( i < length_ - start_ )
	{
		return list_[start_+i];
	}
	else
	{
		return _NULL; 
	}
}

jagint JagStrSplit::length() const
{
	return length_ - start_;
}
jagint JagStrSplit::size() const
{
	return length_ - start_;
}

jagint JagStrSplit::slength() const
{
	return length_ - start_;
}

bool JagStrSplit::exists(const Jstr &token) const
{
	for (int i=0; i < length_; i++) {
		if ( 0==strcmp( token.c_str(), list_[i].c_str() ) ) {
		    return true;
		}
	}

	return false;
}

bool JagStrSplit::contains(const Jstr &token, Jstr &rec) const
{
	const char *tok;
	for (int i=0; i < length_; i++) {
		tok = list_[i].c_str(); 
		if ( strstr( tok, token.c_str() ) ) {
		  	rec = tok;
		  	return 1;
		 }
	}

	rec = "";
	return 0;
}

void JagStrSplit::print() const
{
	printf("s3008 JagStrSplit::print():\n" );
	for (int i=0; i < length_; i++)
	{
		printf("i=%d [%s]\n", i, list_[i].c_str() );
	}
	printf("\n"); 
	fflush(stdout);
}

void JagStrSplit::printStr() const
{
	printf("s3008 JagStrSplit::printStr(): [%s]\n", pdata_ );
}

Jstr& JagStrSplit::last()
{
	return list_[ length_ -1];
}

const Jstr& JagStrSplit::last() const
{
	return list_[ length_ -1];
}

void JagStrSplit::shift()
{
	if ( start_ <= length_-2 ) {
		++ start_;
	}
}

void JagStrSplit::back()
{
	if ( start_ <= 1 ) {
		-- start_;
	}
}

const char* JagStrSplit::c_str() const
{
	return pdata_;
}

void JagStrSplit::pointTo( const char *str )
{
	pdata_ = str;
}

