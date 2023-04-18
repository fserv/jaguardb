#ifndef _jag_file_name_h_
#define _jag_file_name_h_

#include <stdlib.h>

class JagFileName
{
    public:
		static  JagFileName  NULLVALUE;

        Jstr name;

        JagFileName(){}
        JagFileName( const Jstr &n) : name(n) {}
		JagFileName( const JagFileName& other ) 
		{
			name = other.name;
		}

		JagFileName& operator= ( const JagFileName& other )
		{
			if ( this == &other ) return *this;
			name = other.name;
			return *this;
		}

        bool operator<= ( const JagFileName &n2 ) const {
			if ( atoll( name.c_str() ) <= atoll( n2.name.c_str()) ) return true;
			else return false;
		}
		
        int operator>= ( const JagFileName &n2 ) const {
			if ( atoll( name.c_str() ) >= atoll( n2.name.c_str()) ) return true;
			else return false;
		}
};

#endif
