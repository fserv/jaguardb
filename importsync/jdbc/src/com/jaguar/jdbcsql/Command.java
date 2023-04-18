package com.jaguar.jdbcsql;

import java.io.File;
import java.io.FileReader;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.ResultSetMetaData;
import java.sql.DatabaseMetaData;
import java.sql.Statement;
import java.sql.SQLException;
import java.util.Properties;
import java.util.Map;
import java.util.SortedMap;
import java.util.TreeMap;
import java.io.IOException;
import java.io.FileNotFoundException;

public class Command 
{
	// oracle/mysql/postgresql/jaguar/mssql/db2 etc
    private static final String SOURCE_JDBC_URL = "source_jdbcurl";
    private static final String SOURCE_TABLE = "source_table";
    private static final String SOURCE_USER = "source_user";
    private static final String SOURCE_PASSWORD = "source_password";
    private static final String APP_CONF = "appconf";
    private static final String COMMAND = "command";
    private static final String DEBUG = "debug";

    public static void main(String[] args)
	{
    	String appConf = System.getProperty(APP_CONF);
    	if (appConf == null) {
    		System.err.println("Usage: java -cp jar1:jar2:... -Dappconf=<config_file> " + Command.class.getName());
    		return;
    	}
    	
    	Properties appProp = new Properties();
		try {
    		appProp.load(new FileReader(appConf));
		} catch ( IOException e ) {
			// e.printStackTrace();
			return;
		}
    	
        String debug = appProp.getProperty(DEBUG);
        if ( null == debug) {
			debug = "false";
		} else {
			debug = debug.toLowerCase();
		}
        String command = appProp.getProperty(COMMAND).toLowerCase();
        String srcurl = appProp.getProperty(SOURCE_JDBC_URL);
        String user = appProp.getProperty(SOURCE_USER);
        String password = appProp.getProperty(SOURCE_PASSWORD);
        String table = appProp.getProperty(SOURCE_TABLE).toLowerCase();
		String arr[] = srcurl.split(":");
        String dbtype = arr[1].toLowerCase();

        Connection sconn;

		try {
    		try {
				// Class.forName("oracle.jdbc.OracleDriver");
            	sconn = DriverManager.getConnection(srcurl, user, password);
    		} catch ( SQLException e ) {
				if ( debug.equals("true") ) {
    				e.printStackTrace();
				}
    			return;
    		}
    
    		String lead = command.substring(0, 4).toLowerCase();
    		if ( lead.equals("desc") ) {
    			describeTable( dbtype, sconn, table );
    		} else if ( lead.equals("pkey")) {
    			getPrimaryKeys( dbtype, sconn, table );
    		} else if ( lead.equals("sele")) {
    			doSelect( sconn, table, command );
    		} else if ( lead.equals("show")) {
    			doSelect( sconn, table, command );
    		} else if ( lead.equals("upda")) {
    			doExecute( sconn, command );
    		} else if ( lead.equals("inse")) {
    			doExecute( sconn, command );
    		} else if ( lead.equals("dele")) {
    			doExecute( sconn, command );
    		} else {
    			// logit("ERROR Command not supported");
				if ( debug.equals("true") ) {
    				logit("ERROR Command [" + command + "] not supported");
				}
    		}
            
            sconn.close();
		} catch ( Exception e ) {
    		// logit("ERROR exception");
			if ( debug.equals("true") ) {
    			logit("ERROR exception command [" + command + "] not supported");
				e.printStackTrace();
			}
		}

    }

    public static void describeTable( String dbtype, Connection sconn, String table ) throws Exception
	{
        Statement srcst = sconn.createStatement();
		String sql = getSelectOneRowSQL( dbtype, table );
        ResultSet srcrs = srcst.executeQuery( sql );
        ResultSetMetaData srcmeta = srcrs.getMetaData();
        int ncols = srcmeta.getColumnCount();
		String colname, coltypename;
		int  precision, scale;
		// logit("s2773 describeTable ncols=" + ncols );
        for (int i = 1; i <= ncols; i++) {
			colname = srcmeta.getColumnName(i).toLowerCase();
			coltypename = srcmeta.getColumnTypeName(i).toLowerCase();
			precision = srcmeta.getPrecision(i);
			scale = srcmeta.getScale(i);
			logit( colname + "|" + coltypename + "|" + precision + "|" + scale ); 
        }
		srcrs.close();
		srcst.close();
	}

	public static void doSelect( Connection sconn, String table, String sql ) throws Exception
	{
		Statement srcst = sconn.createStatement();
        ResultSet srcrs = srcst.executeQuery( sql );
        ResultSetMetaData srcmeta = srcrs.getMetaData();
        int ncols = srcmeta.getColumnCount();
		String colname;
		StringBuilder sb = new StringBuilder();
		// logit("s2838 doSelect sql=" + sql + "  ncols=" + ncols );
        for (int i = 1; i <= ncols; i++) {
			colname = srcmeta.getColumnName(i);
			if ( 1 == i ) {
				sb.append( colname );
			} else {
				sb.append( "|" + colname );
			}
		}
		logit( sb.toString() );
		logit( "----------------------------------------------------------------------" );

		while ( srcrs.next() ) {
			sb.setLength(0);
        	for (int i = 1; i <= ncols; i++) {
				if ( 1 == i ) {
					sb.append( srcrs.getString(i) );
				} else {
					sb.append( "|" + srcrs.getString(i) );
				}
			}
			logit( sb.toString() );
		}
		srcrs.close();
		srcst.close();
	}

	public static void doExecute( Connection sconn, String sql ) throws Exception
	{
		Statement srcst = sconn.createStatement();
		try {
        	srcst.executeUpdate( sql );
		} catch ( Exception e ) {
			// e.printStackTrace();
		}
		srcst.close();
	}

	public static void logit( String m )
	{
		System.out.println( m );
	}

	public static String  getSelectOneRowSQL( String dbtype, String table )
	{
		String sql = new String();
		if ( dbtype.equals("oracle") ) {
			sql = "select * from " + table + " where ROWNUM<=1";
		} else if ( dbtype.equals("mysql") ) {
			sql = "select * from " + table + " limit 1";
		} else if ( dbtype.equals("postgresql") ) {
			sql = "select * from " + table + " limit 1";
		} else if ( dbtype.equals("postgres") ) {
			sql = "select * from " + table + " limit 1";
		} else if ( dbtype.equals("mssql") ) {
			sql = "select top 1 * from " + table;
		} else if ( dbtype.equals("db2") ) {
			sql = "select * from " + table + " fetch first row only";
		} else if ( dbtype.equals("cassandra") ) {
			sql = "select * from " + table + " limit 1";
		} else {
			sql = "select * from " + table;
		}

		return sql;
	}

   	public static void getPrimaryKeys( String dbtype, Connection sconn, String table ) throws SQLException
	{
		DatabaseMetaData dbmd = sconn.getMetaData();

		// try original name
		ResultSet rs = dbmd.getPrimaryKeys(null, null, table );
		SortedMap<Integer, String> keys = new TreeMap<>();
		int cnt = 0;
		String col;
		int num;
		while ( rs.next() ) {
			col = rs.getString("COLUMN_NAME").toLowerCase();
			num = rs.getInt("KEY_SEQ");
			++cnt;
			keys.put( num, col );
		}
		rs.close();

		// try uppercase as in oralce/db2/etc
		if ( cnt < 1 ) {
			rs = dbmd.getPrimaryKeys(null, null, table.toUpperCase() );
			while ( rs.next() ) {
				col = rs.getString("COLUMN_NAME").toLowerCase();
				num = rs.getInt("KEY_SEQ");
				keys.put( num, col );
			}
			rs.close();
		}

		// print by key sequence order
		for (Integer i : keys.keySet()) {
			logit( keys.get(i));
		}

	}
}
