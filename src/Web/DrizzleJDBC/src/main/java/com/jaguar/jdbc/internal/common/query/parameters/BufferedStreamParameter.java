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

package com.jaguar.jdbc.internal.common.query.parameters;

import static com.jaguar.jdbc.internal.common.Utils.needsEscaping;
import com.jaguar.jdbc.internal.common.Utils;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * . User: marcuse Date: Feb 19, 2009 Time: 8:56:34 PM
 */
public class BufferedStreamParameter implements ParameterHolder {
    private final byte[] byteRepresentation;
    private final int length;

    public BufferedStreamParameter(final InputStream is) throws IOException {
        int b;
        byte[] tempByteRepresentation = new byte[1000];
        int pos = 0;
        tempByteRepresentation[pos++] = '"';
        while ((b = is.read()) != -1) {
            if (pos > tempByteRepresentation.length - 2) { //need two places in worst case
                tempByteRepresentation = Utils.copyWithLength(tempByteRepresentation, tempByteRepresentation.length * 2);
            }
            if (needsEscaping((byte) (b & 0xff))) {
                tempByteRepresentation[pos++] = '\\';
            }
            tempByteRepresentation[pos++] = (byte) (b & 0xff);
        }
        tempByteRepresentation[pos++] = '"';
        length = pos;
        byteRepresentation = tempByteRepresentation;

    }

    public int writeTo(final OutputStream os, int offset, int maxWriteSize) throws IOException {
        int bytesToWrite = Math.min(length - offset, maxWriteSize);
        os.write(byteRepresentation, offset, bytesToWrite);
        return bytesToWrite;
    }

    public long length() {
        return length;
    }
}