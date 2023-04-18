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
import com.jaguar.jdbc.internal.jaguar.Jaguar;

import java.util.List;

/**
 * . User: marcuse Date: Mar 9, 2009 Time: 8:20:04 PM
 */
public class JaguarUpdateResult implements ModifyQueryResult {
    private final long updateCount;
    private final short warnings;
    private final String message;
    private final long insertId;
    // private final QueryResult generatedKeysResult;
    private final Jaguar  jaguarClient;

	/*******
    public JaguarUpdateResult(final long updateCount, final short warnings, final String message, final long insertId) {
        this.updateCount = updateCount;
        this.warnings = warnings;
        this.message = message;
        this.insertId = insertId;
        generatedKeysResult = new JaguarInsertIdQueryResult(insertId, updateCount, message);
    }
	*****/


    public JaguarUpdateResult(final Jaguar injaguarClient ) {
        // System.out.println("j0022 JaguarQueryResult ctor called");

        jaguarClient = injaguarClient;
        this.updateCount = 0;
        this.warnings = 0;
        this.message = "";
        this.insertId = 0;

        // jaguarClient.replyHeader();
        // int columnCount = jaguarClient.getColumnCount();
    }

    // jyue added
    public Jaguar getJaguar() {
        return jaguarClient;
    }



    public long getUpdateCount() {
        return updateCount;
    }

    public ResultSetType getResultSetType() {
        return ResultSetType.MODIFY;
    }

    public void close() {
        // generatedKeysResult.close();
        jaguarClient.freeResult( );
    }

    public boolean next() {
        // generatedKeysResult.close();
        // return jaguarClient.reply( false );
        return jaguarClient.reply( );
    }

    public short getWarnings() {
        return warnings;
    }

    public String getMessage() {
        return message;
    }

    public List<ColumnInformation> getColumnInformation() {
        return null;
    }

    public int getRows() {
        return 0;
    }

    public long getInsertId() {
        return insertId;
    }

    public QueryResult getGeneratedKeysResult() {
        // return generatedKeysResult;
        return null;
    }
}
