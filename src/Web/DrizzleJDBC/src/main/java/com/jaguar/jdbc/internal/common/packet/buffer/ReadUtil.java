/*
 * Drizzle-JDBC
 *
 * Copyright (c) 2009-2011, Marcus Eriksson, Trond Norbye
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

package com.jaguar.jdbc.internal.common.packet.buffer;

import com.jaguar.jdbc.internal.common.packet.RawPacket;

import java.io.IOException;
import java.io.InputStream;
import java.io.InterruptedIOException;
import java.nio.ByteBuffer;

/**
 * . User: marcuse Date: Jan 16, 2009 Time: 8:27:38 PM
 */
public final class ReadUtil {
    private ReadUtil() {

    }

    /**
     * Read a number of bytes from the stream and store it in the buffer, and fix the problem with "incomplete" reads by
     * doing another read if we don't have all of the data yet.
     *
     * @param inputStream the input stream to read from
     * @param buffer      where to store the data
     * @return the number of bytes read (should be == length if we didn't hit EOF)
     * @throws java.io.IOException if an error occurs while reading the stream
     */
    public static int safeRead(final InputStream inputStream, final byte[] buffer) throws IOException {
        int readBytes = inputStream.read(buffer);
        if(readBytes == -1) {
            return -1;
        }
        if(readBytes < buffer.length) {
            int offset = readBytes;
            int left = buffer.length;
            left = left - readBytes;
            do {
                try {
                    final int nr = inputStream.read(buffer, offset, left);
                    if (nr == -1) {
                        return nr;
                    }
                    offset += nr;
                    left -= nr;
                } catch (InterruptedIOException exp) {
                    /* Ignore, just retry */
                }
            } while (left > 0);
        }
        return buffer.length;
    }

    /**
     * Checks whether the next packet is EOF. 
     * @param rawPacket the raw packet
     * @return true if the packet is an EOF packet
     */
    public static boolean eofIsNext(final RawPacket rawPacket) {
        final ByteBuffer buf = rawPacket.getByteBuffer();
        return (buf.get(0) == (byte)0xfe && buf.capacity() < 9);

    }

    public static boolean isErrorPacket(RawPacket rawPacket) {
        return rawPacket.getByteBuffer().get(0) == (byte)0xff;
    }
}
