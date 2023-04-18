/*
 * Jaguar-JDBC
 *
 * Copyright (c) 2009-2011, Marcus Eriksson
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following
 * conditions are met:
 *
 *  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
 *   disclaimer in the documentation and/or other materials provided with the distribution.
 *  Neither the name of the driver nor the names of its contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

package com.jaguar.jdbc.internal.common.queryresults;

import com.jaguar.jdbc.internal.common.ColumnInformation;
import com.jaguar.jdbc.internal.common.ValueObject;

import com.jaguar.jdbc.internal.jaguar.Jaguar;
// import com.jaguar.jdbc.internal.jaguar.JaguarType;
import com.jaguar.jdbc.internal.mysql.MySQLType;
import com.jaguar.jdbc.internal.jaguar.JaguarValueObject;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Arrays;
import java.util.Vector;


/**
 * <p/>
 * User: marcuse Date: Jan 23, 2009 Time: 8:15:55 PM
 */
public final class JaguarQueryResult implements SelectQueryResult 
{
    private final List<ColumnInformation> columnInformation;
    private final List<List<ValueObject>> resultSet;
    private final Map<String, Integer> columnNameMap;
    private final short warningCount;
    private int rowPointer;
    private int _columnCount;
	private Jaguar  jaguarClient;

	/********
    public JaguarQueryResult(final List<ColumnInformation> columnInformation,
                              final List<List<ValueObject>> valueObjects,
                              final short warningCount) {
        this.columnInformation = columnInformation;
        this.resultSet = valueObjects;
        this.warningCount = warningCount;
        columnNameMap = new HashMap<String, Integer>();
        rowPointer = -1;
        int i = 0;
        for (final ColumnInformation ci : columnInformation) {
            columnNameMap.put(ci.getTable().toLowerCase()+"."+ci.getName().toLowerCase(), i);
            columnNameMap.put(ci.getName().toLowerCase(), i++);
        }
    }
	*******/

    // public JaguarQueryResult(final Jaguar injaguarClient, final long row ) {
    public JaguarQueryResult(final Jaguar injaguarClient ) {
		// System.out.println("j0022 JaguarQueryResult ctor called");
		
        this.columnInformation = null;
        this.resultSet = null;
        this.warningCount = 0;
		rowPointer = -1;
		jaguarClient = injaguarClient;

        columnNameMap = new HashMap<String, Integer>();

		// get reply header first
		// System.out.println("j0229 begin jaguarClient.replyHeader ...");
		// qwer jan-12-2017
		jaguarClient.replyHeader();
		// System.out.println("j0229 begin jaguarClient.replyHeader done");
		int columnCount = jaguarClient.getColumnCount();
		// System.out.println("j0229 columnCount=" + columnCount );
		if ( columnCount < 1 ) {
			_columnCount = 0;
			// return;
		}
        for ( int i = 1; i <= columnCount; ++i ) {
            columnNameMap.put(jaguarClient.getColumnName(i).toLowerCase(), i );
			// System.out.println("j0249 colname=" + jaguarClient.getColumnName(i) );
		}
	}

	// jyue added
	public Jaguar getJaguar() {
		return jaguarClient;
	}

	/***
    public boolean next() {
        rowPointer++;
        return rowPointer < resultSet.size();
    }
	***/
    public boolean next() {
        rowPointer++;
		// System.out.println("j2830 JaguarQueryResult.java next() calling jaguarClient.reply( ) ...");
		// if ( _columnCount < 1 ) { return false; }
		// boolean rc = jaguarClient.reply( false );
		boolean rc = jaguarClient.reply( );
		return rc;
    }

	/***
    public void close() {
        columnInformation.clear();
        resultSet.clear();
        columnNameMap.clear();
    }
	***/

    public void close() {
		// System.out.println("JaguarQueryResult.close()  jaguarClient.freeRow( ) ");
		jaguarClient.freeResult( );
	}


    public short getWarnings() {
        return warningCount;
    }

    public String getMessage() {
        return null;
    }

    public List<ColumnInformation> getColumnInformation() {
        return columnInformation;
    }

    /**
     * gets the value at position i in the result set. i starts at zero!
     *
     * @param i index, starts at 0
     * @return
     */
	 /*****
    public ValueObject getValueObject(final int i) throws NoSuchColumnException {
        if (i < 0 || i > resultSet.get(rowPointer).size()) {
            throw new NoSuchColumnException("No such column: " + i);
        }
        return resultSet.get(rowPointer).get(i);
    }
	*****/

	/*******
    public ValueObject getValueObject(final String column) throws NoSuchColumnException {
        if (columnNameMap.get(column.toLowerCase()) == null) {
            throw new NoSuchColumnException("No such column: " + column);
        }
        return getValueObject(columnNameMap.get(column.toLowerCase()));
    }
	******/
    public ValueObject getValueObject(final String column) throws NoSuchColumnException {
		String str;
		if ( column == null || column.length() < 1 ) {
			str = jaguarClient.getMessage( );
		} else {
			str = jaguarClient.getValue(  column );
			// System.out.println("jaguarClient.getValue(  column ) str=[" + str + "]" );
			if ( str == null || str.length() < 1  ) {
				str = "";
			}
		}

		// System.out.println("JaguarQueryResult.java: getValueObject (final String column) getValue " + column + " value=" + str  );
		MySQLType dataType = new MySQLType( MySQLType.Type.VARCHAR );
		JaguarValueObject obj = new JaguarValueObject( str.getBytes(), dataType  );
		return obj;
	}

    public ValueObject getValueObject(final int column) throws NoSuchColumnException {
		String str;
		// System.out.println("getValueObject j2930 column=" + column );
		if ( column < 1 ) {
			str = jaguarClient.getMessage( );
		} else {
			str = jaguarClient.getNthValue(  column );
			// System.out.println("j0385 jaguarClient.getNthValue col=" + column + "  str="+ str );
			/***
			String name = jaguarClient.getColumnName( column );
			str = jaguarClient.getValue( name );
			System.out.println("j0385 jaguarClient.getNthValue col=" + column + " name=" + name + "  value="+ str );
			***/
			if ( str == null || str.length() < 1  ) {
				// System.out.println("j0382 error jaguarClient.getNthValue str= NULL" );
				str = "";
			}
		}

		// System.out.println("JaguarQueryResult.java: getValueObject (final String column) getValue " + column + " value=" + str  );
		String cn = jaguarClient.getColumnName( column );
		String ctn = jaguarClient.getColumnTypeName( column );
		// System.out.println("*********** getValueObject() getColumnName=" + cn + "  getColumnTypeName=" + ctn );

		MySQLType dataType; 
		// todo, to complete types
		if ( ctn.equals ( "NUMERIC" ) ) {
			dataType = new MySQLType( MySQLType.Type.LONG );
			str = str.trim();
		} else if ( ctn.equals( "FLOAT" ) ) {
			dataType = new MySQLType( MySQLType.Type.FLOAT );
			str = str.trim();
		} else if ( ctn.equals( "DOUBLE" ) ) {
			dataType = new MySQLType( MySQLType.Type.DOUBLE );
			str = str.trim();
		} else if ( ctn.equals( "TIME" ) ) {
			dataType = new MySQLType( MySQLType.Type.LONG );
			str = str.trim();
		} else {
			dataType = new MySQLType( MySQLType.Type.VARCHAR );
		}

		// System.out.println("j3820 before new JaguarValueObject() str=" + str + " rawbytes=" + str.getBytes() );
		JaguarValueObject obj = new JaguarValueObject( str.getBytes(), dataType  );
		// System.out.println("j3821 getValueObject str=" + str );
		return obj;
	}

    public ValueObject getValueObject() throws NoSuchColumnException {
		String str;
		str = jaguarClient.getMessage( );
		// System.out.println("JaguarQueryResult.java: getValueObject (final ) getValue value=" + str  );
		MySQLType dataType = new MySQLType( MySQLType.Type.VARCHAR );
		JaguarValueObject obj = new JaguarValueObject( str.getBytes(), dataType  );
		return obj;
	}


    public int getRows() {
        return resultSet.size();
    }

    public int getColumnId(final String columnLabel) throws NoSuchColumnException {
        if (columnNameMap.get(columnLabel.toLowerCase()) == null) {
            throw new NoSuchColumnException("No such column: " + columnLabel);
        }
        return columnNameMap.get(columnLabel.toLowerCase());
    }

    public void moveRowPointerTo(final int i) {
        this.rowPointer = i;
    }

    public int getRowPointer() {
        return rowPointer;
    }


    public ResultSetType getResultSetType() {
        return ResultSetType.SELECT;
    }
}
