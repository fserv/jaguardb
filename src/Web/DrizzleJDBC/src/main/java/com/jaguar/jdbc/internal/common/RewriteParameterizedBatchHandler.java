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

package com.jaguar.jdbc.internal.common;

// import com.jaguar.jdbc.internal.common.query.DrizzleQuery;
import com.jaguar.jdbc.internal.common.query.JaguarQuery;
import com.jaguar.jdbc.internal.common.query.ParameterizedQuery;
import com.jaguar.jdbc.internal.common.query.parameters.ParameterHolder;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.sql.Statement;
import java.util.Arrays;
import java.util.LinkedList;
import java.util.List;
import java.util.logging.Logger;

/**
 * Rewrites queries on the form (INSERT INTO xyz (a,b,c) VALUES (?,?,?))* to INSERT INTO xyz (a,b,c) VALUES ((?,?,?),)*
 */
public class RewriteParameterizedBatchHandler implements ParameterizedBatchHandler {
    private final static Logger log = Logger.getLogger(RewriteParameterizedBatchHandler.class.getName());
    private final int MAX_QUERY_LENGTH = 1000000;
    private final String baseQuery;
    private final String queryValuePart;
    private final Protocol protocol;
    private final List<String> queriesToSend = new LinkedList<String>();
    private StringBuilder queryBuilder = new StringBuilder();
    private boolean firstWritten = false;
    private int queryCount = 0;
    private final String onDupKeyPart;

    /**
     * Constructs a new handler
     *
     * @param protocol       the protocol to use to send the query.
     * @param baseQuery      the base of the query, i.e. everything including .*VALUES
     * @param queryValuePart the value part (?,?..)
     * @param onDupKeyPart   the duplicate key part of the query
     */
    public RewriteParameterizedBatchHandler(final Protocol protocol,
                                            final String baseQuery,
                                            final String queryValuePart,
                                            final String onDupKeyPart) {
        this.baseQuery = baseQuery;
        this.queryValuePart = queryValuePart;
        this.onDupKeyPart = (onDupKeyPart == null ? "" : onDupKeyPart);
        queryBuilder.append(baseQuery);
        this.protocol = protocol;
    }

    public void addToBatch(final ParameterizedQuery query) {
        final ParameterHolder [] parameters = query.getParameters();
        final StringBuilder replacedValuePart = new StringBuilder();
        int questionMarkPosition = 0;

        for (int i = 0; i < queryValuePart.length(); i++) {
            final char ch = queryValuePart.charAt(i);
            if (ch == '?') {
                final ByteArrayOutputStream baos = new ByteArrayOutputStream();
                final ParameterHolder parameterHolder = parameters[questionMarkPosition++];
                try {
                    parameterHolder.writeTo(baos, 0, Integer.MAX_VALUE); // writeTo escapes and adds quotes etc
                } catch (IOException e) {
                    throw new RuntimeException("Could not write to byte array: " + e.getMessage(), e);
                }
                replacedValuePart.append(baos.toString());
            } else {
                replacedValuePart.append(ch);
            }
        }
        final String replaced = replacedValuePart.toString();

        if (queryBuilder.length() + replaced.length() + onDupKeyPart.length() > MAX_QUERY_LENGTH) {
            queryBuilder.append(onDupKeyPart);
            queriesToSend.add(queryBuilder.toString());
            queryBuilder = new StringBuilder();
            queryBuilder.append(baseQuery);

            firstWritten = false;
        }

        if (firstWritten) {
            queryBuilder.append(",");
        }

        queryBuilder.append(replaced);
        firstWritten = true;
        queryCount++;
    }

    public int[] executeBatch() throws QueryException {
		// System.out.println("j2209 executeBatch  RewriteParameterizedBatchHandler.java ");

        queryBuilder.append(onDupKeyPart);
        final String lastQuery = queryBuilder.toString();

        if (firstWritten) {
            queriesToSend.add(lastQuery);
        }

        for (final String query : queriesToSend) {
            protocol.executeQuery(new JaguarQuery(query));
        }

        log.finest("Rewrote " + queryCount + " queries to " + queriesToSend.size() + " queries");
        final int[] returnArray = new int[queryCount];
        Arrays.fill(returnArray, Statement.SUCCESS_NO_INFO);
        // reset stuff
        queriesToSend.clear();
        queryBuilder = new StringBuilder();
        queryBuilder.append(baseQuery);
        firstWritten = false;
        queryCount = 0;        
        return returnArray;
    }
}
