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
#ifndef _jag_schema_record_h_
#define _jag_schema_record_h_

#include <abax.h>
#include <JagDef.h>
#include <JagColumn.h>
#include <JagVector.h>
#include <JagHashStrInt.h>

// Timeseries: <n>s   n: 1,2,3,4,5,10,15,20,30,60
// Timeseries: <n>m   n: 1,2,3,4,5,10,15,20,30,60
// Timeseries: <n>h   n: any number
// Timeseries: <n>d   n: any number
// Timeseries: <n>w   n: any number
// Timeseries: <n>M   n: 1,2,3,4,6
// Timeseries: <n>q   n: any number
// Timeseries: <n>y   n: any number
// Timeseries: <n>D   n: any number

class JagParseParam;

class JagSchemaRecord
{
	public:
		JagSchemaRecord( bool newVec = false );
		~JagSchemaRecord();
		void destroy(  AbaxDestroyAction act=ABAX_NOOP);
		JagSchemaRecord( const JagSchemaRecord& other );
		JagSchemaRecord& operator=( const JagSchemaRecord& other );
		bool renameColumn( const AbaxString &oldColName, const AbaxString & newColName );
		bool setColumn( const AbaxString &oldColName, const AbaxString &attr, const AbaxString & value );
		bool addValueColumnFromSpare( const AbaxString &colName, const Jstr &type, 
									  jagint length, jagint sig );
		Jstr getString() const;
		int  parseRecord( const char *str );
		Jstr formatHeadRecord() const;
		Jstr formatColumnRecord( const char *name, const char *type, int offset, int length, 
										   int sig, bool isKey=false ) const;
		Jstr formatTailRecord( ) const;

		int     getKeyMode() const;
		int     getPosition( const AbaxString& colName ) const;
		bool    hasPoly(int &dim ) const;
		void    setLastKeyColumn();
		void    getJoinSchema( long skeylen, long svallen, const JagParseParam &parseParam, const jagint lengths[], Jstr &hstr );
		static Jstr translateTimeSeries( const Jstr &inputTimeSeries );
		static Jstr translateTimeSeriesBack( const Jstr &inputTimeSeries );
		static Jstr translateTimeSeriesToStrs( const Jstr &inputTimeSeries );
		static bool hasTimeSeries( const Jstr &tabProp, Jstr &series );
		static time_t getRetentionSeconds( const Jstr &retention );
		static bool validRetention( char u );
		static Jstr makeTickPair( const Jstr &ser );
		bool    hasTimeSeries( Jstr &series ) const;
		bool    setTimeSeries( const Jstr &normSeries );
		bool    setRetention( const Jstr &retention );
		//bool hasRollupColumn( ) const;
		Jstr    timeSeriesRentention() const;
		static int normalizeTimeSeries( const Jstr &series, Jstr &normalizedSeries );
		bool    isFirstColumnDateTime( Jstr &colType ) const;
		int     getFirstDateTimeKeyCol() const;
		bool    print() const;

		JagVector<JagColumn> *columnVector;
		char        type[2];
		int 		keyLength;
		int 		valueLength;
		int 		ovalueLength;
		Jstr 		tableProperty;
		int		    numKeys;
		int		    numValues;
		bool        hasMute;
		int	        lastKeyColumn;
        bool        keyHasFile;
        bool        keyHasGeo;
		bool        hasRollupColumn;

	protected:
		JagHashStrInt _nameMap;
		void    copyData( const JagSchemaRecord& other ); 
		void    init( bool newVec );

};


#endif
