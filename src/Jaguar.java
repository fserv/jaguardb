/*
 * Copyright (C) 2018,2019,2020,2021 DataJaguar, Inc.
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
package com.jaguar.jdbc.internal.jaguar;

import java.io.*;
import java.lang.*;

public class Jaguar
{
	static {
		System.loadLibrary("JaguarClient");
	}

    public Jaguar()
    {
       	adbMakeObject();
		closed = false;
    }

    public void freeResult() 
    {
		if ( closed ) return;
		freeRow();
    }

    public void close() 
    {
        adbClose( _adb );
		closed = true;
    }

    public float getFloat( String name )
	{
    	String v =  getValue(name);
		return Float.parseFloat( v.trim() );
	}

    public double getDouble( String name )
	{
    	String v =  getValue(name);
		return Double.parseDouble( v.trim() );
	}

    public int getInt( int n )
	{
		return getIntByCol( n );
	}

    public int getInt( String name  )
	{
		return getIntByName( name );
	}

    public long getLong( int n )
	{
		return getLongByCol( n );
	}

    public long getLong( String name  )
	{
		return getLongByName( name );
	}


    public native boolean connect( String ipaddress, int port, String username,
                 				   String passwd, String dbname, String unixSocket, long clientFlag );
    public native boolean execute( String query );
    public native boolean query( String query );
    public native boolean reply( ); 
    public native boolean replyHeader(); 
    public native boolean printRow( );
    public native boolean printAll( );
    public native boolean hasError( );
    public native String error( );
    public native boolean freeRow( );
    public native String getValue( String name);
    public native String getAllByName( String name);
    public native String getNthValue(  int nth );
    public native String getAllByIndex(  int nth );
    public native String getMessage( );
    public native String getAll( );
    public native String jsonString( );
    public native String getLastUuid( );

    public native int getIntByCol( int nth );
    public native int getIntByName( String name  );

    public native long getLongByCol( int nth );
    public native long getLongByName( String name );

    public native float getFloat( int nth );
    public native double getDouble( int nth );


    public native int getColumnCount();
    public native String getCatalogName( int col );
    public native String getColumnClassName( int col );
    public native int getColumnDisplaySize( int col );
    public native String getColumnLabel( int col );
    public native String getColumnName( int col );
    public native int getColumnType( int col );
    public native String getColumnTypeName( int col );
    public native int getScale( int col );
    public native String getSchemaName( int col );
    public native String getTableName( int col );
    public native boolean isAutoIncrement( int col );
    public native boolean isCaseSensitive( int col );
    public native boolean isCurrency( int col );
    public native boolean isDefinitelyWritable( int col );
    public native int isNullable( int col );
    public native boolean isReadOnly( int col );
    public native boolean isSearchable( int col );
    public native boolean isSigned( int col );



    ///////// protected methods
    protected long _adb;

    protected native void adbMakeObject();
    protected native void adbClose( long adb );
    protected boolean closed;

    public static void main(String[] args)
    {
        System.loadLibrary("JaguarClient");
        Jaguar client = new Jaguar();
        client.connect( "127.0.0.1", 8888, "test", "test", "test", "dummy", 0 );
        client.query( "select * from demo1 limit 10;" );
        String val;
        while ( true ) {
            if ( ! client.reply() ) {
                break;
            }
        	client.printRow( ); 
            val = client.getValue( "fname" );
            System.out.println( val );

            val = client.getNthValue( 2 );
            System.out.println( val );
            val = client.getNthValue( 3 );
            System.out.println( val );
        }
        if ( client.hasError( ) ) {
             String e = client.error( );
             System.out.println( e );
        }
        client.freeRow( ); 
        client.close();
    }
 }


