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

package com.jaguar.jdbc.internal.mysql.packet.commands;

import com.jaguar.jdbc.internal.common.packet.CommandPacket;
import com.jaguar.jdbc.internal.common.packet.buffer.WriteBuffer;

import java.io.IOException;
import java.io.OutputStream;

public class MySQLBinlogDumpPacket implements CommandPacket {
    private final WriteBuffer writeBuffer;

    public MySQLBinlogDumpPacket(final int startPos, final String filename) {
        writeBuffer = new WriteBuffer();
        writeBuffer.writeByte((byte) 18);
        writeBuffer.writeInt(startPos);
        writeBuffer.writeShort((short) 0);
        writeBuffer.writeInt(0);
        writeBuffer.writeString(filename);
    }

    public int send(final OutputStream os) throws IOException {
        os.write(writeBuffer.getLengthWithPacketSeq((byte) 0));
        os.write(writeBuffer.getBuffer(),0,writeBuffer.getLength());
        os.flush();
        return 0;
    }
}
