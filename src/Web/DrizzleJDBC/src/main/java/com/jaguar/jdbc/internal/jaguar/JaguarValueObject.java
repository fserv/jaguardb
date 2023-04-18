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

package com.jaguar.jdbc.internal.jaguar;

import com.jaguar.jdbc.JaguarBlob;
import com.jaguar.jdbc.internal.common.AbstractValueObject;
import com.jaguar.jdbc.internal.common.DataType;

import java.sql.Time;
import java.text.ParseException;
import java.text.SimpleDateFormat;

/**
 * Contains the raw value returned from the server
 * <p/>
 * Is immutable
 * <p/>
 * User: marcuse Date: Feb 16, 2009 Time: 9:18:26 PM
 */
public class JaguarValueObject extends AbstractValueObject {

    public JaguarValueObject(final byte[] rawBytes, final DataType dataType) {
        super(rawBytes, dataType);
    }

    public Object getObject() throws ParseException {
        if (this.getBytes() == null) {
            return null;
        }
        switch (dataType.getType()) {
            case TINY:
                return getShort();
            case LONG:
                return getLong();
            case DOUBLE:
                return getDouble();
            case TIMESTAMP:
				// System.out.println("jdbc3322 getObject TIMESTAMP getTimestamp() ...");
                return getTimestamp();
            case LONGLONG:
                return getBigInteger();
            case DATETIME:
                return getTimestamp();
            case DATE:
                return getDate();
            case VARCHAR:
                return getString();
            case NEWDECIMAL:
				// System.out.println("jdbc3312 getObject getBigDecimal() ...");
                return getBigDecimal();
            case ENUM:
                return getString();
            case BLOB:
                return new JaguarBlob(getBytes());
            case YEAR:
                return getString();
            case BIT:
                if(getBytes().length == 1) {
                    return getBytes()[0] == 1;
                }
                return null;
            case SHORT:
            case INT24:
                return getInt();
            case FLOAT:
                return getFloat();
            case TIME:
                return getTime();
            case CLOB:
                return getString();
        }
        return null;
    }

    @Override
    public Time getTime() throws ParseException {
        if (getBytes() == null) {
            return null;
        }
        final String rawValue = getString();
        final SimpleDateFormat sdf;
        sdf = new SimpleDateFormat("HH:mm:ss");
        final java.util.Date utilTime = sdf.parse(rawValue);
        return new Time(utilTime.getTime());
    }

}
