/*
 * Drizzle-JDBC
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
import com.jaguar.jdbc.internal.common.GeneratedIdValueObject;
import com.jaguar.jdbc.internal.common.ValueObject;
import com.jaguar.jdbc.internal.common.queryresults.ColumnFlags;
import com.jaguar.jdbc.internal.mysql.MySQLColumnInformation;
import com.jaguar.jdbc.internal.mysql.MySQLType;

import java.util.Arrays;
import java.util.EnumSet;
import java.util.List;
import com.jaguar.jdbc.internal.jaguar.Jaguar;


/**
 * . User: marcuse Date: Mar 9, 2009 Time: 8:34:44 PM
 */
public class DrizzleInsertIdQueryResult implements SelectQueryResult {

    private final long insertId;
    private int rowPointer = 0;
    private final long rows;
    private final String message;

    public DrizzleInsertIdQueryResult(final long insertId, final long rows, final String message) {
        this.insertId = insertId;

        this.message = message;
        this.rows = rows;
    }

    public ValueObject getValueObject(final int index) throws NoSuchColumnException {
        if (index != 0) {
            throw new NoSuchColumnException("No such column: " + index);
        }
        return new GeneratedIdValueObject(insertId);
    }

	public Jaguar getJaguar () {
		return null;
	}

    public ValueObject getValueObject(final String columnName) throws NoSuchColumnException {
        if (!columnName.toLowerCase().equals("insert_id")) {
            throw new NoSuchColumnException("No such column: " + columnName);
        }
        return new GeneratedIdValueObject(insertId + rowPointer - 1);
    }

    public int getRows() {
        return (int) rows;
    }

    public int getColumnId(final String columnLabel) throws NoSuchColumnException {
        if (columnLabel.equals("insert_id")) {
            return 0;
        }
        throw new NoSuchColumnException("No such column");
    }

    public void moveRowPointerTo(final int i) {

    }

    public int getRowPointer() {
        return rowPointer;
    }

    public boolean next() {
        return rowPointer++ < rows;
    }

    public List<ColumnInformation> getColumnInformation() {
        return Arrays.<ColumnInformation>asList(
                new MySQLColumnInformation.Builder()
                .flags(EnumSet.of(
                        ColumnFlags.PRIMARY_KEY, ColumnFlags.AUTO_INCREMENT))
                .name("insert_id")
                .type(new MySQLType(MySQLType.Type.LONG))
                .build());
    }

    public ResultSetType getResultSetType() {
        return ResultSetType.SELECT;
    }

    public void close() {

    }

    public short getWarnings() {
        return 0;
    }

    public String getMessage() {
        return null;
    }
}
