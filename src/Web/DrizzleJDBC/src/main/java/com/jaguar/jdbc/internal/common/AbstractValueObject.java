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


import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.UnsupportedEncodingException;
import java.math.BigDecimal;
import java.math.BigInteger;
import java.sql.Date;
import java.sql.Time;
import java.sql.Timestamp;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Calendar;

/**
 * Created by IntelliJ IDEA. User: marcuse Date: Jun 10, 2009 Time: 4:13:03 PM To change this template use File |
 * Settings | File Templates.
 */
public abstract class AbstractValueObject implements ValueObject {
    private final byte[] rawBytes;
    protected final DataType dataType;

    protected AbstractValueObject(final byte[] rawBytes, final DataType dataType) {
		// System.out.println("j0384 AbstractValueObject ctor  rawBytes="+rawBytes );
        this.dataType = dataType;
        this.rawBytes = rawBytes;
    }

    public String getString() {
        if (rawBytes == null) {
            return null;
        }
        try {
           return new String(rawBytes, "UTF-8");
        } catch (UnsupportedEncodingException e) {
           throw new RuntimeException("Unsupported encoding: " + e.getMessage(), e);
       }
    }

    public long getLong() {
        if (rawBytes == null) {
            return 0;
        }
        return Utils.byteArrayToLong(rawBytes);
    }

    public int getInt() {
        if (rawBytes == null) {
            return 0;
        }
        return Utils.byteArrayToInt(rawBytes);
    }

    public short getShort() {
        if (rawBytes == null) {
            return 0;
        }
        return Utils.byteArrayToShort(rawBytes);
    }

    public byte getByte() {
        if (rawBytes == null) {
            return 0;
        }
        switch(dataType.getType()) {
            case BIT:
                return rawBytes[0];
        }
        return Utils.byteArrayToByte(rawBytes);
    }

    public byte[] getBytes() {
        return rawBytes;
    }

    public float getFloat() {
        if (rawBytes == null) {
            return 0;
        }
		String s = getString();
		if ( s == null || s.length() < 1 ) return 0.0f;
        return Float.valueOf( s );
    }

    public double getDouble() {
        if (rawBytes == null) {
            return 0;
        }
		String s = getString();
		if ( s == null || s.length() < 1 ) return 0.0;
        return Double.valueOf( s );
    }

    public BigDecimal getBigDecimal() {
        if (rawBytes == null) {
			String bs = "0.0";
        	return new BigDecimal( bs );
        }

		String s = getString();
		if ( s == null || s.length() < 1 ) {
			String bs = "0.0";
        	return new BigDecimal( bs );
		}
		String str = s.trim();
		// System.out.println("j3803 str=["+str + "]");
        // return new BigDecimal(getString());
        return new BigDecimal( str );
    }
    public BigInteger getBigInteger() {
        if (rawBytes == null) {
			String bs = "0";
        	return new BigInteger( bs );
        }
		String s = getString();
		if ( s == null || s.length() < 1 ) {
			String bs = "0";
        	return new BigInteger( bs );
		}
        return new BigInteger( s );
    }
    public Date getDate() throws ParseException {
        if (rawBytes == null) {
            return null;
        }
        String rawValue = getString();
		if ( rawValue == null ) {
			rawValue = "1970-01-01 00:00:00";
		}

        final SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd");
        final java.util.Date utilDate = sdf.parse(rawValue);
        return new Date(utilDate.getTime());
    }

    /**
     * Since drizzle has no TIME datatype, JDBC Time is stored in a packed integer
     *
     * @return the time
     * @throws java.text.ParseException
     * @see Utils#packTime(long)
     * @see Utils#unpackTime(int)
     */
    public Time getTime() throws ParseException {
        if (rawBytes == null) {
            return null;
        }
        String rawValue = getString();
		if ( rawValue == null ) { rawValue = "00:00:00"; }
        SimpleDateFormat sdf = new SimpleDateFormat("HH:mm:ss");

        sdf.setLenient(false);
        final java.util.Date utilTime = sdf.parse(rawValue);
        return new Time(utilTime.getTime());
    }

    public Timestamp getTimestamp() throws ParseException {
        if (rawBytes == null) {
            return null;
        }
        String rawValue = getString();
		if ( rawValue == null ) { rawValue = "1970-01-01"; }
        SimpleDateFormat sdf;

        if (rawValue.length() > 11) {
            sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
        } else {
            sdf = new SimpleDateFormat("yyyy-MM-dd");
        }
        sdf.setLenient(false);
        final java.util.Date utilTime = sdf.parse(rawValue);
        return new Timestamp(utilTime.getTime());
    }

    public InputStream getInputStream() {
        if (rawBytes == null) {
            return null;
        }
        return new ByteArrayInputStream(getString().getBytes());
    }

    public InputStream getBinaryInputStream() {
        if (rawBytes == null) {
            return null;
        }
        return new ByteArrayInputStream(rawBytes);
    }
    
    public abstract Object getObject() throws ParseException;

    public Date getDate(final Calendar cal) throws ParseException {
        if (rawBytes == null) {
            return null;
        }
        //final String rawValue = getString();
        String rawValue = getString();
		if ( rawValue == null ) { rawValue = "1970-01-01"; }
        final SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd");
        sdf.setCalendar(cal);
        final java.util.Date utilDate = sdf.parse(rawValue);
        return new Date(utilDate.getTime());
    }

    public Time getTime(final Calendar cal) {
        // TODO: FIX! USE CAL!

        if (rawBytes == null) {
            return null;
        }
        final int packedTime = getInt();
        final long millis = Utils.unpackTime(packedTime);
        final SimpleDateFormat sdf = new SimpleDateFormat("HH:mm:ss");
        sdf.setCalendar(cal);
        return new Time(millis);
    }

    public Timestamp getTimestamp(final Calendar cal) throws ParseException {
        if (rawBytes == null) {
            return null;
        }
        //final String rawValue = getString();
        String rawValue = getString();
		if ( rawValue == null ) { rawValue = "1970-01-01 00:00:00"; }
        final SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
        sdf.setCalendar(cal);
        final java.util.Date utilTime = sdf.parse(rawValue);
        return new Timestamp(utilTime.getTime());
    }

    public boolean getBoolean() {
        if (rawBytes == null) {
            return false;
        }
        //final String rawVal = getString();
        String rawVal = getString();
		if ( rawVal == null ) { rawVal = "0"; }
        return rawVal.equalsIgnoreCase("true") || rawVal.equalsIgnoreCase("1") || (rawBytes[0] & 0x1)==1;
    }

    public boolean isNull() {
        return rawBytes == null;
    }


    public int getDisplayLength() {
        if (rawBytes != null) {
            return rawBytes.length;
        }
        return 4; //NULL
    }

    public InputStream getPBMSStream(Protocol protocol) throws QueryException, IOException {
        if(rawBytes == null) {
            return null;
        }
        if(rawBytes[0] == '~' && rawBytes[1] == '*') { //TODO: better check if it actually is a pbms column
            String port = protocol.getServerVariable("pbms_port");
            HttpClient httpClient = new HttpClient("http://"+protocol.getHost()+":"+port+"/"+getString());
            return httpClient.get();
        }
        return getBinaryInputStream();         // if it is not a pbms-column
    }

}
