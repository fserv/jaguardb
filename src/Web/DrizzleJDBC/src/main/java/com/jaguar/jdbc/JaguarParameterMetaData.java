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

package com.jaguar.jdbc;

import com.jaguar.jdbc.internal.common.query.ParameterizedQuery;

import java.sql.ParameterMetaData;
import java.sql.SQLException;

/**
 * Very basic info about the parameterized query, only reliable method is getParameterCount();
 */
public class JaguarParameterMetaData implements ParameterMetaData {
    private final ParameterizedQuery query;

    public JaguarParameterMetaData(ParameterizedQuery dQuery) {
        this.query = dQuery;
    }

    public int getParameterCount() throws SQLException {
        return query.getParamCount();
    }

    public int isNullable(int i) throws SQLException {
        return ParameterMetaData.parameterNullableUnknown;
    }

    public boolean isSigned(int i) throws SQLException {
        return true;
    }

    //TODO: fix
    public int getPrecision(int i) throws SQLException {
        return 1;
    }

    //TODO: fix
    public int getScale(int i) throws SQLException {
        return 0;
    }

    //TODO: fix
    public int getParameterType(int i) throws SQLException {
        return java.sql.Types.VARCHAR;
    }

    //TODO: fix
    public String getParameterTypeName(int i) throws SQLException {
        return "String";
    }

    //TODO: fix
    public String getParameterClassName(int i) throws SQLException {
        return "String.class";
    }

    public int getParameterMode(int i) throws SQLException {
        return parameterModeInOut;
    }

    public <T> T unwrap(Class<T> tClass) throws SQLException {
        return null;  //To change body of implemented methods use File | Settings | File Templates.
    }

    public boolean isWrapperFor(Class<?> aClass) throws SQLException {
        return false;  //To change body of implemented methods use File | Settings | File Templates.
    }
}
