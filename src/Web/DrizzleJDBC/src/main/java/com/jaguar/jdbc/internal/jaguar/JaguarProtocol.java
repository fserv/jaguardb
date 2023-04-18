/*
 * Jaguar-JDBC
 *
 * Copyright (c) 2009-2011, Marcus Eriksson, Stephane Giron, Marc Isambart, Trond Norbye
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

import com.jaguar.jdbc.internal.SQLExceptionMapper;
import com.jaguar.jdbc.internal.jaguar.Jaguar;

import com.jaguar.jdbc.internal.common.*;
import com.jaguar.jdbc.internal.common.packet.EOFPacket;
import com.jaguar.jdbc.internal.common.packet.ErrorPacket;
import com.jaguar.jdbc.internal.common.packet.OKPacket;
import com.jaguar.jdbc.internal.common.packet.RawPacket;
import com.jaguar.jdbc.internal.common.packet.ResultPacket;
import com.jaguar.jdbc.internal.common.packet.ResultPacketFactory;
import com.jaguar.jdbc.internal.common.packet.ResultSetPacket;
import com.jaguar.jdbc.internal.common.packet.SyncPacketFetcher;
import com.jaguar.jdbc.internal.common.packet.buffer.ReadUtil;
import com.jaguar.jdbc.internal.common.packet.commands.ClosePacket;
import com.jaguar.jdbc.internal.common.packet.commands.SelectDBPacket;
import com.jaguar.jdbc.internal.common.packet.commands.StreamedQueryPacket;
import com.jaguar.jdbc.internal.mysql.packet.commands.MySQLBinlogDumpPacket;

import com.jaguar.jdbc.internal.common.query.JaguarQuery;
import com.jaguar.jdbc.internal.common.query.Query;
import com.jaguar.jdbc.internal.common.queryresults.JaguarQueryResult;
import com.jaguar.jdbc.internal.common.queryresults.JaguarUpdateResult;
import com.jaguar.jdbc.internal.common.queryresults.NoSuchColumnException;
import com.jaguar.jdbc.internal.common.queryresults.QueryResult;


import javax.net.SocketFactory;
import javax.net.ssl.SSLSocket;
import javax.net.ssl.SSLSocketFactory;
import java.io.*;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.EnumSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Properties;
import java.util.Set;
import java.util.logging.Logger;

// import static com.jaguar.jdbc.internal.common.packet.buffer.WriteBuffer.intToByteArray;

/**
 * TODO: refactor, clean up TODO: when should i read up the resultset? TODO: thread safety? TODO: exception handling
 * User: marcuse Date: Jan 14, 2009 Time: 4:06:26 PM
 */
public class JaguarProtocol implements Protocol 
{
    private final static Logger log = Logger.getLogger(JaguarProtocol.class.getName());
    private boolean connected = false;
	private BufferedOutputStream writer;
	private PacketFetcher packetFetcher;
    private final String version;
    private boolean readOnly = false;
    private final String host;
    private final int port;
    private String database;
    private final String username;
    private final String password;
	private final List<Query> batchList;
    private final Properties info;
    private final long serverThreadId;
    private volatile boolean queryWasCancelled = false;
    private volatile boolean queryTimedOut = false;

	private final  Jaguar jaguarclient;

    /**
     * Get a protocol instance
     *
     * @param host     the host to connect to
     * @param port     the port to connect to
     * @param database the initial database
     * @param username the username
     * @param password the password
     * @param info
     * @throws com.jaguar.jdbc.internal.common.QueryException
     *          if there is a problem reading / sending the packets
     */
    public JaguarProtocol(final String host,
                         final int port,
                         final String database,
                         final String username,
                         final String password,
                         Properties info) throws QueryException 
	{
        this.info = info;
        this.host = host;
        this.port = port;
        this.database = (database == null ? "" : database);
        this.username = (username == null ? "" : username);
        this.password = (password == null ? "" : password);

		String tmo = info.getProperty("timeout", "5");
		String unixStr = "/timeout=" + tmo;

		jaguarclient = new Jaguar();
		// boolean rc = jaguarclient.connect( host, port, username, password, database, null, 0 );
		boolean rc = jaguarclient.connect( host, port, username, password, database, unixStr, 0 );
		if ( ! rc ) {
            throw new QueryException("jaguarclient.connect() failed", (short) -1, "JZ0001");
		}

		// System.out.println("j2280 jaguarclient.connect() done OK\n");
		this.connected = true;

		version = "2.0";
		serverThreadId = 1;

		batchList = new ArrayList<Query>();
    }

    /**
     * @return row pointer
     */
    /**
     * @return Jaguar object
     */
    public Jaguar getJaguar() {
        return jaguarclient;
    }


    /**
     * Closes socket and stream readers/writers
     *
     * @throws com.jaguar.jdbc.internal.common.QueryException
     *          if the socket or readers/writes cannot be closed
     */
    public void close() throws QueryException {

		// free row and close connection
		if ( this.connected ) {
			jaguarclient.close();
		}
		// System.out.println("jaguarprotocol close() jaguarclient.freeRow( ) jaguarclient.close(); j3380");
		// System.out.println("j3380 jaguarprotocol close() jaguarclient.close(); j3380");

        this.connected = false;
    }

    /**
     * @return true if the connection is closed
     */
    public boolean isClosed() {
        return !this.connected;
    }

    public String getServerVersion() {
        return version;
    }

    public void setReadonly(final boolean readOnly) {
        this.readOnly = readOnly;
    }

    public boolean getReadonly() {
        return readOnly;
    }

    public void commit() throws QueryException {
        log.finest("commiting transaction");
        executeQuery(new JaguarQuery("COMMIT"));
    }

    public void rollback() throws QueryException {
        log.finest("rolling transaction back");
        executeQuery(new JaguarQuery("ROLLBACK"));
    }

    public void rollback(final String savepoint) throws QueryException {
        log.finest("rolling back to savepoint " + savepoint);
        executeQuery(new JaguarQuery("ROLLBACK TO SAVEPOINT " + savepoint));
    }

    public void setSavepoint(final String savepoint) throws QueryException {
        executeQuery(new JaguarQuery("SAVEPOINT " + savepoint));
    }

    public void releaseSavepoint(final String savepoint) throws QueryException {
        executeQuery(new JaguarQuery("RELEASE SAVEPOINT " + savepoint));
    }

    public String getHost() {
        return host;
    }

    public int getPort() {
        return port;
    }

    public String getDatabase() {
        return database;
    }

    public String getUsername() {
        return username;
    }

    public String getPassword() {
        return password;
    }

	public QueryResult executeQuery(Query dQuery, InputStream inputStream) throws QueryException {
		// System.out.println("j1909 executeQuery return null\n");
		return executeQuery( dQuery );
	}
	
	public QueryResult executeQuery(final Query dQuery) throws QueryException {
		// System.out.println("j1919 executeQuery [" + dQuery.getQuery() + "]" );
		return executeQuery( dQuery.getQuery() );
	}

    public QueryResult executeQuery(final String dQuery) throws QueryException 
	{
		boolean rc = jaguarclient.query( dQuery );
		if ( ! rc ) {
			String err = jaguarclient.error();
            throw new QueryException("executeQuery failed: " + dQuery + " (" + err + ")" );
		}

		// System.out.println("j3909 new JaguarQueryResult ...");
		JaguarQueryResult res = new JaguarQueryResult( jaguarclient );
		// System.out.println("j3909 new JaguarQueryResult done, return res");
		return res;
    }

	// new
    public QueryResult executeUpdate(final String dQuery) throws QueryException 
	{
		boolean rc = jaguarclient.query( dQuery );
		if ( ! rc ) {
			String err = jaguarclient.error();
			/***
			boolean rc2 = jaguarclient.hasError();
			if ( rc2 ) {
				System.out.println("j3949 hasError true");
			} else {
				System.out.println("j3949 hasError false");
			}
			***/
            throw new QueryException("executeUpdate failed: " + dQuery + " (" + err + ")" );
		}

		//System.out.println("j3909 new JaguarQueryResult ...");
		JaguarUpdateResult res = new JaguarUpdateResult( jaguarclient );
		//System.out.println("j3909 new JaguarQueryResult done, return res");
		return res;
    }

    public void addToBatch(final Query dQuery) {
		 batchList.add(dQuery);
    }

    public List<QueryResult> executeBatch() throws QueryException {
		// return null;
		final List<QueryResult> retList = new ArrayList<QueryResult>(batchList.size());
		for (final Query query : batchList) {
			retList.add(executeQuery( query.asString() ));
		}
		clearBatch();
		return retList;
    }

    public void clearBatch() {
		batchList.clear();
    }

    public SupportedDatabases getDatabaseType() {
        return SupportedDatabases.fromVersionString(version);
    }

    public String getServerVariable(String variable) throws QueryException {
        JaguarQueryResult qr = (JaguarQueryResult) executeQuery(new JaguarQuery("select @@" + variable));
        if (!qr.next()) {
            throw new QueryException("Could not get variable: " + variable);
        }

        try {
            String value = qr.getValueObject(0).getString();
            return value;
        } catch (NoSuchColumnException e) {
            throw new QueryException("Could not get variable: " + variable);
        }
    }


    /**
     * cancels the current query - clones the current protocol and executes a query using the new connection
     * <p/>
     * thread safe
     *
     * @throws QueryException
     */
    public void cancelCurrentQuery() throws QueryException {
        Protocol copiedProtocol = new JaguarProtocol(host, port, database, username, password, info);
        queryWasCancelled = true;
        copiedProtocol.executeQuery(new JaguarQuery("KILL QUERY " + serverThreadId));
        copiedProtocol.close();
    }

    public void timeOut() throws QueryException {
        Protocol copiedProtocol = new JaguarProtocol(host, port, database, username, password, info);
        queryTimedOut = true;
        copiedProtocol.executeQuery(new JaguarQuery("KILL QUERY " + serverThreadId));
        copiedProtocol.close();

    }


    /**
     * Catalogs are not supported in Jaguar so this is a no-op with a Jaguar
     * connection<br>
     * Jaguar treats catalogs as databases. The only difference with
     * {@link JaguarProtocol#selectDB(String)} is that the catalog is switched
     * inside the connection using SQL 'use' command
     */
    public void setCatalog(String catalog) throws QueryException
    {
        if (getDatabaseType() == SupportedDatabases.JAGUAR)
        {
            executeQuery(new JaguarQuery("use `" + catalog + "`"));
            this.database = catalog;
        }
        // else (Jaguar protocol): silently ignored since Jaguar does not
        // support catalogs
    }

    /**
     * Catalogs are not supported in Jaguar so this will always return null
     * with a Jaguar connection<br>
     * Jaguar treats catalogs as databases. This function thus returns the
     * currently selected database
     */
    public String getCatalog() throws QueryException
    {
        if (getDatabaseType() == SupportedDatabases.JAGUAR )
        {
            return getDatabase();
        }
        // else (Jaguar protocol): retrun null since Jaguar does not
        // support catalogs
        return null;
    }

	public QueryResult getMoreResults() throws QueryException {
		return null;

	}

	public boolean createDB() {
		return true;
	}

	public boolean supportsPBMS() {
		return false;
	}


    public List<RawPacket> startBinlogDump(final int startPos, final String filename) throws BinlogDumpException {
        final MySQLBinlogDumpPacket mbdp = new MySQLBinlogDumpPacket(startPos, filename);
        try {
            mbdp.send(writer);
            final List<RawPacket> rpList = new LinkedList<RawPacket>();
            while (true) {
                final RawPacket rp = this.packetFetcher.getRawPacket();
                if (ReadUtil.eofIsNext(rp)) {
                    return rpList;
                }
                rpList.add(rp);
            }
        } catch (IOException e) {
            throw new BinlogDumpException("Could not read binlog", e);
        }
    }

 	public boolean ping() throws QueryException {
		return true;
 	}

	public void selectDB(final String database) throws QueryException {
	}

}

