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

package com.jaguar.jdbc.internal.mysql.packet;

import com.jaguar.jdbc.internal.common.ColumnInformation;
import com.jaguar.jdbc.internal.common.packet.RawPacket;
import com.jaguar.jdbc.internal.common.packet.buffer.Reader;
import com.jaguar.jdbc.internal.common.queryresults.ColumnFlags;
import com.jaguar.jdbc.internal.mysql.MySQLColumnInformation;
import com.jaguar.jdbc.internal.mysql.MySQLType;

import java.io.IOException;
import java.util.EnumSet;
import java.util.Set;

/**
 * Creates column information from field packets.
 */
public class MySQLFieldPacket {
    /*
Bytes                      Name
-----                      ----
n (Length Coded String)    catalog
n (Length Coded String)    db
n (Length Coded String)    table
n (Length Coded String)    org_table
n (Length Coded String)    name
n (Length Coded String)    org_name
1                          (filler)
2                          charsetnr
4                          length
1                          type
2                          flags
1                          decimals
2                          (filler), always 0x00
n (Length Coded Binary)    default

    */

    public static ColumnInformation columnInformationFactory(final RawPacket rawPacket) throws IOException {
        final Reader reader = new Reader(rawPacket);
        return new MySQLColumnInformation.Builder()
                .catalog(reader.getLengthEncodedString())
                .db(reader.getLengthEncodedString())
                .table(reader.getLengthEncodedString())
                .originalTable(reader.getLengthEncodedString())
                .name(reader.getLengthEncodedString())
                .originalName(reader.getLengthEncodedString())
                .skipMe(reader.skipBytes(1))
                .charsetNumber(reader.readShort())
                .length(reader.readInt())
                .type(MySQLType.fromServer(reader.readByte()))
                .flags(parseFlags(reader.readShort()))
                .decimals(reader.readByte())
                .skipMe(reader.skipBytes(2))
                .build();
    }

    private static Set<ColumnFlags> parseFlags(final short i) {
        final Set<ColumnFlags> retFlags = EnumSet.noneOf(ColumnFlags.class);
        for (final ColumnFlags fieldFlag : ColumnFlags.values()) {
            if ((i & fieldFlag.flag()) == fieldFlag.flag()) {
                retFlags.add(fieldFlag);
            }
        }
        return retFlags;
    }
}