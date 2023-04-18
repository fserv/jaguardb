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
#ifndef _jag_clock_h_
#define _jag_clock_h_

// Clock Class
class JagClock
{

    public:
    JagClock()
    {
		start();
    }

    void start()
    {
        struct timeval now;
        gettimeofday( &now, NULL ); 
        endsec = beginsec = now.tv_sec;
        endmsec = beginmsec = now.tv_usec/1000;
        endusec = beginusec = now.tv_usec;
    }

    void stop()
    {
        struct timeval now;
        gettimeofday( &now, NULL ); 
        endsec = now.tv_sec;
        endmsec = now.tv_usec/1000;
        endusec = now.tv_usec;
    }

    // return elapsed time in milliseconds
    jagint elapsed() const
    {
        return ( 1000*(endsec - beginsec) + ( endmsec - beginmsec ) );
    }
    jagint elapsedusec() const
    {
        return ( 1000000*(endsec-beginsec) + ( endusec - beginusec ) );
    }
	void print( const char *hdr ) {
		printf("%s elapsed %lld usecs\n", hdr, elapsedusec() );
		fflush( stdout );
	}
	void flash( const char *hdr ) {
		stop();
		printf("%s elapsed %lld usecs\n", hdr, elapsedusec() );
		fflush( stdout );
		start();
	}

    private:
       	jagint  beginsec, beginmsec, beginusec;
        jagint  endsec, endmsec, endusec;

};

#endif
