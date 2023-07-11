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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <malloc.h>
#include <sstream>
#include "JagRecord.h"
#include "JagUtil.h"

JagRecord::JagRecord()
{
	_record = NULL;
	_readOnly = 0;
}

JagRecord::JagRecord( const char *srcrec )
{
	_record = NULL;
	_readOnly = 0;
	setSource( srcrec );
}

void JagRecord::destroy ( ) 
{
	if ( _readOnly ) {
		return;
	}

	if ( _record ) {
		free( _record );
	}
	_record = NULL;
}

JagRecord::~JagRecord ( ) 
{
	destroy();
}

int JagRecord::makeNewRecLength( const char *name, int n1,  const char *value, int n2 )
{
	char *pnew;
	int len;
	int hdrsize;
	Jstr buf256str;
	char buf[32];

	if ( _record ) {
		free ( _record );
		_record = NULL;
	}

	len = 10 + n1 + 32 + n2; 
	pnew = (char*)jagmalloc( len );
	memset(pnew, 0, len );

	sprintf(buf, "%06d", n2 );
	buf256str = Jstr(name) + ":0+" + buf;
	hdrsize = buf256str.size();

	int len2 = 6+1+6+1 + hdrsize + 1 + n2; 
	sprintf(pnew, "  %08d%06d%c%06d%c%s%c%s", len2, hdrsize, FREC_VAL_SEP, n2, FREC_COMMA, buf256str.c_str(), FREC_HDR_END, value );
	_record = pnew;
	return 0;
}

int JagRecord::getNameStartLen( const char *name, int namelen, int *colstart, int *collen )
{
	int index;
	char *p;
	char *start;
	Jstr startposstr;
	Jstr poslenstr;;
	if ( name == NULL || *name == '\0' ) {
		*colstart = *collen = 0;
		return 0;
	}
	Jstr buf256;

	buf256 = FREC_COMMA_STR;
	buf256 += Jstr(name, namelen );
	buf256 += ":";

	index = str_str_ch( _record+10, FREC_HDR_END, buf256.c_str() );
	if ( index < 0 ) {
		return -2;
	}

	start = (char*)( _record + 10 + index + 1 + namelen + 1); // 1st:1 is to pass; 2nd 1 is to point to once byte after :

	for( p= start; *p != '+' && *p!='\0'; p++ ) {
		startposstr += *p;
	}

	if ( *p != '+' ) {
		return -4; 
	}
	*colstart = jagatoi( startposstr.c_str() );
	p++;  

	while ( (*p != FREC_COMMA ) && ( *p != FREC_HDR_END ) && ( *p != '\0' ) ) {
		poslenstr += *p;
		p++;
	}

	*collen = jagatoi( poslenstr.c_str() );
	return 0;
}

int JagRecord::getSize( int *hdrsize, int *valsize )
{
	char *p;
	Jstr lenbufstr;
	p = _record+10;
	while ( (*p !=  FREC_VAL_SEP ) && *p != '\0' ) {
		lenbufstr += *p;
		p++;
	}

	if ( *p != FREC_VAL_SEP ) {
		*hdrsize = 0;
		*valsize = 0;
		return -2;
	}

	*hdrsize = jagatoi(lenbufstr.c_str());

	lenbufstr = "";
	p++;
	while ( (*p != FREC_COMMA ) && *p != '\0' ) {
		lenbufstr += *p;
		p++;
	}

	if ( *p != FREC_COMMA ) {
		*hdrsize = 0;
		*valsize = 0;
		return -5;
	}

	*valsize = jagatoi(lenbufstr.c_str() );
	return 0;
}

size_t JagRecord::getLength()
{

	char *p = _record;
	if ( p == NULL || *p == '\0' ) {
		return 0;
	}

	char buf[9];
	for ( int i = 2; i < 10; ++i ) {
		buf[i-2] = _record[i];
	}
	buf[8] = '\0';
	return ( 10 + atoi( buf ) );
}

int JagRecord::getAllNameValues( char *names[],  char *values[], int *len )
{
	char *p;
	int  startposi[FREC_MAX_NAMES];
	int  posleni[FREC_MAX_NAMES];
	int  donenames = 0;
	int  total = 0;
	int  hdrsize, valsize;

	total = 0;
	*len = 0;

	Jstr valbufstr;
	for ( p = _record+10;  (*p != FREC_VAL_SEP ) && *p != '\0'; p++ ) {
		valbufstr += *p;
	}

	if ( *p != FREC_VAL_SEP ) {
		return -1;
	}
	hdrsize = jagatoi( valbufstr.c_str() );

	p++;
	valbufstr = "";
	while ( (*p != FREC_COMMA ) && *p != '\0' ) {
		valbufstr += *p;
		p++;
	}

	if ( *p != FREC_COMMA ) {
		return -2;
	}

	valsize = jagatoi( valbufstr.c_str() );

	p++;

	Jstr bufstr;
	valbufstr = "";
	while ( ( *p != FREC_HDR_END ) && (*p != '\0')  ) {
		donenames = 0;

		bufstr = "";
		while ( *p != ':' && *p != '\0' ) {
			bufstr += *p;
			p++;
		}

		if ( *p != ':' ) {
			return -4;
		}

		if ( total  >= FREC_MAX_NAMES  ) {
			return -5;
		}

		names[total] = strdup(bufstr.c_str());

		p++; 

		valbufstr = "";
		while ( *p != '+' && *p != '\0' ) {
			valbufstr += *p;
			p++;
		}

		if ( *p != '+' ) {
			return -7;
		}

		startposi[total] = atoi(valbufstr.c_str() );

		p++; 

		valbufstr = "";
		while ( *p != FREC_COMMA && *p != FREC_HDR_END && *p != '\0' ) {
			valbufstr += *p;
			p++;
		}

		if ( *p == FREC_HDR_END ) {
			donenames = 1;
		}

		if ( *p == '\0' ) {
			return -8;
		}

		posleni[total] = atoi(valbufstr.c_str());

		total ++;
		p++;

		if ( donenames ) {
			break;
		}


		if ( total >= FREC_MAX_NAMES-1 ) {
			break;
		}
	}

	for (int i = 0; i < total; i++) {
		values[i] = getValueFromPosition( startposi[i], posleni[i], hdrsize, valsize );
		if ( ! values[i] ) {
			return -12;
		}
	}

	*len = total;
	return 0;
}

char * JagRecord::getValueFromPosition( int start, int len, int hdrsize, int valsize )
{
	char 	*pd; 
	char 	*p; 
	int 	i;
	char    *pstart;

	if ( ( start + len ) > valsize ) {
		return NULL; 
	}

	pstart = _record+10;
	while ( *pstart != FREC_VAL_SEP && *pstart != '\0' ) pstart++;
	if ( '\0' == *pstart ) return 0;
	pstart ++;  
	while ( *pstart != FREC_COMMA  && *pstart != '\0' ) pstart++;
	if ( '\0' == *pstart ) return 0;
	pstart ++; 

	pd = (char*)jagmalloc(len+1);
	memset(pd, 0, len+1);
	p = pd;

	pstart += hdrsize + 1;

	for ( i = start; i< start + len; i++) {
		*p = pstart[i]; 
		p++;
	}

	return pd;
}


bool JagRecord::remove ( const char *name )
{
	char 	*names[ FREC_MAX_NAMES ];
	char 	*vals[ FREC_MAX_NAMES ];
	int 	rc;
	int 	len; 
	char  	*pnew;
	int 	oldlen;
	int		i;
	char 	*phdr;
	char 	*pval;
	int   	relpos = 0;
	int  	vallen;
	int  	namelen;
	int  	hdrsize, valsize;
	char 	sizebuf[32];

	len = 0;
	rc = getAllNameValues( names,  vals, &len ); 

	if ( rc < 0 ) {
		printf("c9394 rc=%d < 0 \n", rc );
		return false;
	}

	char buf[9];
	for ( int i = 2; i < 10; ++i ) {
		buf[i-2] = _record[i];
	}
	buf[8] = '\0';
	oldlen = 10 + atoi( buf );

	pnew = (char*)jagmalloc(oldlen + 1 );
	memset(pnew, 0, oldlen + 1 );

	phdr = (char*)jagmalloc(oldlen + 1 );
	memset( phdr, 0, oldlen + 1 );

	pval = (char*)jagmalloc(oldlen + 1 );
	memset( pval, 0, oldlen + 1 );

	relpos = 0;
	for (i=0; i<len; i++ ) {
		if ( 0 == strcmp( name, names[i] ) ) {
			continue;
		}

		memset( sizebuf, 0, 32 );
		namelen = strlen( names[i] );
		vallen = strlen( vals[i] );

		strcat( phdr, names[i] );
		sprintf( sizebuf, ":%06d+%06d%c", relpos, vallen, FREC_COMMA ); 
		strcat( phdr, sizebuf); 

		strcat( pval, vals[i] );
		strcat( pval, FREC_VAL_SEP_STR );

		relpos += vallen + 1;
	}

	hdrsize = strlen(phdr);
	valsize = strlen(pval);

	memset(pnew, 0, oldlen + 1 );
	int len2 = 6+1+6+1 +hdrsize + 1 + valsize; 
	sprintf( pnew, "  %08d%06d%c%06d%c%s%c%s", len2, hdrsize, FREC_VAL_SEP, valsize, FREC_COMMA, phdr, FREC_HDR_END, pval );

	if ( phdr ) free(phdr);
	if ( pval ) free(pval);
	phdr = pval = NULL;
	for (i=0; i<len; i++ ) {
		if ( names[i] ) free( names[i] );
		if ( vals[i] ) free( vals[i] );
		names[i] = vals[i] = NULL;
	}

	if ( _record ) free ( _record );
	_record = pnew;

	return true;
}

int JagRecord::addNameValue ( const char *name, const char *value )
{
	int namelen;
	int vallen;
	if ( _readOnly ) { return -1; }
	namelen = strlen(name);
	vallen = strlen(value);
	return addNameValueLength ( name, namelen, value, vallen );
}

int JagRecord::addNameValueLength ( const char *name, int n1, const char *value, int n2 )
{
	int newlen;
	char *pnew;
	char *psep;
	char *phdr;
	int   rc;
	int   hdrsize, valsize;
	int   new_hdrsize, new_valsize;
	int   comma;
	int   pound;
	char buf[32];
	int oldlen;

	if ( NULL == _record ) {
		oldlen = 0;
	} else {
		oldlen = getLength();
	}

	if ( oldlen <1 || NULL == _record ) {
		rc = makeNewRecLength( name, n1, value, n2 );
		return rc;
	}

	rc = getSize( &hdrsize, &valsize ); 
	if ( rc < 0 ) {
		return -2903;
	}

	newlen = oldlen + n1 + 32 + n2; 

	phdr = _record + 10;
	while ( *phdr != FREC_COMMA && *phdr != '\0' ) phdr++;
	if ( *phdr == '\0' ) { return -2829; }
	phdr++;


	psep = phdr;
	while ( *psep != FREC_HDR_END && *psep != '\0' ) psep++;
	if ( *psep == '\0' ) { return -2818; }

	Jstr buf256str;
	pound = 0;
	if ( _record[oldlen-1] == FREC_VAL_SEP ) {
		pound = 1;
		buf256str = name;
		sprintf( buf, ":%06d+%06d", valsize, n2 );
		buf256str += buf;
		new_valsize = valsize + n2;
	} else {
		buf256str = name;
		sprintf( buf, ":%06d+%06d", valsize+1, n2 );
		buf256str += buf;
		new_valsize = valsize + 1 + n2;  
	}

	comma = 0;
	if ( *(psep-1) == FREC_COMMA ) {
		new_hdrsize = hdrsize + buf256str.size();
		comma = 1;
	} else {
		new_hdrsize = hdrsize + 1 + buf256str.size();  
	}

	pnew = (char*)jagmalloc( newlen );
	memset( pnew, 0, newlen );

	int len2 = 6+1+6+1 + new_hdrsize + 1 + new_valsize; 
	sprintf( pnew, "  %08d%06d%c%06d%c", len2, new_hdrsize, FREC_VAL_SEP, new_valsize, FREC_COMMA );
	strncat( pnew, phdr, psep - phdr );
	if ( comma ) {
		strcat(pnew, buf256str.c_str());
	} else {
		strcat(pnew, FREC_COMMA_STR);
		strcat(pnew, buf256str.c_str() );
	}

	strcat(pnew, FREC_HDR_END_STR);
	strcat(pnew, psep+1);

	if ( pound ) {
		strcat(pnew, value);
	} else {
		strcat(pnew, FREC_VAL_SEP_STR);
		strcat(pnew, value);
	}

	if ( _record ) free ( _record );
	_record = pnew;

	return 0;
}

char *JagRecord::getValue( const char *name )
{
	int namelen;
	namelen = strlen(name);
	return getValueLength( name, namelen );
}

char * JagRecord::getValueLength( const char *name, int namelen )
{
	int rc;
	int colstart, collen;
	int   hdrsize, valsize;
	char *res;

	rc = getSize( &hdrsize, &valsize );
	if ( rc < 0 ) {
		return NULL;
	}

	rc = getNameStartLen( name, namelen, &colstart, &collen );
	if ( rc < 0 ) {
		return NULL;
	}

	res = getValueFromPosition( colstart, collen, hdrsize, valsize );
	return res;
}

int JagRecord::nameExists( const char *name )
{
	int namelen = strlen(name);
	return nameLengthExists( name, namelen );
}

int JagRecord::nameLengthExists( const char *name, int namelen )
{
	int    index;
	Jstr buf256;

	buf256 = FREC_COMMA_STR;
	buf256 += Jstr(name, namelen );
	buf256 += ":";

    index = str_str_ch( _record+10, FREC_HDR_END, buf256.c_str() );
    if ( index < 0 ) {
        return 0; 
    }

	return 1;
}

bool JagRecord::setValue ( const char *name, const char *value )
{
	int rc;
	bool pdel;
	int namelen;

	if ( _readOnly ) { return false; }

	namelen = strlen(name);
	rc = nameLengthExists( name, namelen );
	if ( rc ) {
		pdel = remove ( name );
		if ( 0 == pdel ) {
			return 0;
		}
		addNameValue ( name, value );
	} else {
		addNameValue ( name, value );
	}

	return true;
}

void JagRecord::setSource( const char *srcrec )
{
	char buf[11];
	int i;

	_readOnly = 0;

	if ( _record ) {
		free ( _record );
	}
	_record = NULL;

	if ( NULL == srcrec ) {
		_record = (char*)jagmalloc( 1 );
		*_record = '\0';
		return;
	}

	for ( i = 0; i <10; ++i ) {
		buf[i ] = srcrec[i];
		if ( srcrec[i] == '\0' ) {
			_record = (char*)jagmalloc( 1 );
			*_record = '\0';
			return;
		}
	}
	buf[10] = '\0';

	int len = 10 + atoi( buf+2 ); // CSnnnnnnnn

	_record = (char*)jagmalloc( len +1 );
	memcpy( _record, srcrec, len );
	_record[len] = '\0';
}


void JagRecord::readSource( const char *srcrec )
{
	_record = (char*)srcrec;
	_readOnly = 1;
}

void JagRecord::referSource( const char *srcrec )
{
	_record = (char*)srcrec;
}

void JagRecord::print()
{
	printf("%s\n", _record );
	fflush( stdout );
}

bool JagRecord::addNameValueArray ( const char *name[], const char *value[], int len )
{
	if ( _readOnly ) { return false; }
	int  i;
	for ( i = 0; i < len; i++ ) {
		addNameValue (  name[i], value[i] );
	}

	return true;
}
