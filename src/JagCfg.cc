/*
 * Copyright JaguarDB  www.jaguardb.com
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

#include <JagCfg.h>
#include <JagDBServer.h>
#include <JagTableSchema.h>

JagCfg::JagCfg( int type )
{
	_map = NULL;
	init( type );
}

JagCfg::~JagCfg()
{
	destroy();
}

int JagCfg::init( int type )
{
    char  line[1024];
	char  fpath[256];
	char  fn[32];
    char *p;
    char *pline;
	int rc;

	_type = type;
	memset( line, 0, 1024 );
	memset( fpath, 0, 256 );
	if ( ! _map ) {
		_map = new JagHashMap<AbaxString, AbaxString>();
	} 

	Jstr  home = jaguarHome();
	if ( type == JAG_SERVER ) {
		strcpy(fn, "server.conf" );
	} else {
		strcpy(fn, "client.conf" );
		return 0; // client does not need conf file
	}
	sprintf(line, "%s/conf/%s", home.c_str(), fn );

	strcpy( fpath, line );
    FILE * fp = jagfopen( fpath, "r" );
    if ( ! fp ) {
		return 0;
    }

	AbaxString name;
	AbaxString value;
    while ( NULL != (fgets( line, 1024, fp ) ) ) {
        pline = line;
        while ( *pline == ' ' || *pline == '\t' ) ++pline;
        if ( pline[0] == '#' ) {
            continue;
        }

		if ( strlen(line) < 3 ) continue;
		p = line;
		while ( *p == ' ' ) ++p;
		pline = p;
		while ( *p != ' ' && *p != '=' && *p != '\0' && *p !='\r' && *p != '\n' ) ++p;
		*p = '\0';
		name = pline;

		++p;
		pline = p;
		while ( (*p == ' ' || *p == '=' ) && *p != '\0' ) ++p;
		if ( *p != '\0' ) {
			pline = p;
			++p;
			while ( *p != '\r' && *p != '\n' && *p != '\0' ) ++p;
			*p = '\0';
			value = pline;
		}

		rc = _map->addKeyValue( name, value );
    }

    jagfclose( fp );
	return 1;
}

int JagCfg::destroy()
{
	if ( _map ) {
		delete _map;
		_map = NULL;
	}
	return 0;
}

int JagCfg::refresh()
{
	destroy();
	init( _type );
	return 1;
}

Jstr  JagCfg::getValue( const AbaxString &name, const Jstr &defValue ) const
{
	AbaxString  value;
	if ( _map->getValue( name, value ) ) {
		return value.c_str();
	} else {
		return defValue;
	}
}

int JagCfg::getIntValue( const AbaxString &name, int defaultValue ) const
{
	AbaxString  value;
	if ( _map->getValue( name, value ) ) {
		return jagatoi(value.c_str());
	} else {
		return defaultValue;
	}
}

jagint JagCfg::getLongValue( const AbaxString &name, jagint defaultValue ) const
{
	AbaxString  value;
	if ( _map->getValue( name, value ) ) {
		return jagatol(value.c_str());
	} else {
		return defaultValue;
	}
}

float JagCfg::getFloatValue( const AbaxString &name, float defaultValue ) const
{
	AbaxString  value;
	if ( _map->getValue( name, value ) ) {
		return jagatof(value.c_str());
	} else {
		return defaultValue;
	}
}

Jstr JagCfg::getConfHOME() 
{
	Jstr value = jaguarHome() +  "/conf";
	return value;
}

Jstr JagCfg::getJDBDataHOME( int objType ) 
{
	Jstr  value;
	if ( 0 == objType ) {
		value = jaguarHome() + "/data";
	} else if ( 1 == objType ) {
		value = jaguarHome() + "/pdata";
	} else if ( 2 == objType ) {
		value = jaguarHome() + "/ndata";
	}
	return value;
}

Jstr JagCfg::getTEMPDataHOME( int objType ) 
{
	Jstr  value;
	if ( 0 == objType ) {
		value = jaguarHome() + "/tmp/data";
	} else if ( 1 == objType ) {
		value = jaguarHome() + "/tmp/pdata";
	} else if ( 2 == objType ) {
		value = jaguarHome() + "/tmp/ndata";
	}
	return value;
}

Jstr JagCfg::getTEMPJoinHOME() 
{
	Jstr value = jaguarHome() + "/tmp/join";
	return value;
}

Jstr JagCfg::getWalLogHOME() 
{
	Jstr value = jaguarHome() + "/log/cmd";
	return value;
}

