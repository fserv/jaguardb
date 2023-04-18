package com.jaguar.jdbcsql;

import java.io.FileReader;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.ResultSetMetaData;
import java.sql.Statement;
import java.util.Properties;
import com.jaguar.jdbcsql.Command;
import com.jaguar.jdbcsql.JagUtil;

public class Importer 
{
    private static final String SOURCE_TABLE = "source_table";
    private static final String SOURCE_PASSWORD = "source_password";
    private static final String SOURCE_USER = "source_user";
    private static final String SOURCE_JDBCURL = "source_jdbcurl";
    private static final String DEST_PASSWORD = "dest_password";
    private static final String DEST_USER = "dest_user";
    private static final String DEST_JDBCURL = "dest_jdbcurl";
    private static final String COM_JAGUAR_JDBC_JAGUAR_DRIVER = "com.jaguar.jdbc.JaguarDriver";
    private static final String APP_CONF = "appconf";
    private static final String IMPORT_ROWS = "import_rows";

    public static void main(String[] args) throws Exception 
	{
    	String appConf = System.getProperty(APP_CONF);
    	if (appConf == null) {
    		System.err.println("Usage: java -cp jar1:jar2:... -Dappconf=<config_file> " + Importer.class.getName());
    		return;
    	}
    	
    	Properties appProp = new Properties();
    	appProp.load(new FileReader(appConf));
    	
    	// load Jaguar driver
    	try {
    		Class.forName(COM_JAGUAR_JDBC_JAGUAR_DRIVER);
    	} catch (Exception e) {
    		e.printStackTrace();
			System.exit(1);
    	}
 
        // source database
        String import_rows = appProp.getProperty(IMPORT_ROWS);
		if ( null == import_rows ) import_rows = "0";
		long importrows = Long.parseLong(import_rows, 10);
        String srcurl = appProp.getProperty(SOURCE_JDBCURL);  // has /dbname or /servicename at end
        String arr[] = srcurl.split(":");
        String source_dbtype = arr[1].toLowerCase();
        System.out.println("sourceurl " + srcurl);
        String user = appProp.getProperty(SOURCE_USER);
        String password = appProp.getProperty(SOURCE_PASSWORD);
        // String table = appProp.getProperty(SOURCE_TABLE).toLowerCase();
        String tables = appProp.getProperty(SOURCE_TABLE).toLowerCase();
        Connection sconn = DriverManager.getConnection(srcurl, user, password);
        Statement srcst = sconn.createStatement();
		String tabs[] = JagUtil.splitString( tables, "[|]" );
		int tablen = tabs.length;

        // dest database
        String desturl = appProp.getProperty(DEST_JDBCURL).toLowerCase();
        System.out.println("desturl " + desturl);
        user = appProp.getProperty(DEST_USER);
        password = appProp.getProperty(DEST_PASSWORD);
        Connection tconn = DriverManager.getConnection( desturl, user, password);
        Statement tst = tconn.createStatement();

		for ( int ti = 0; ti < tablen; ++ti ) {
			String table = tabs[ti];
    		String allsql = "select * from " + table;
            ResultSet srcrs = srcst.executeQuery( allsql );
            ResultSetMetaData srcmeta = srcrs.getMetaData();
            int ncols = srcmeta.getColumnCount();
    		String colnames[] = new String[ncols];
            System.out.println("column count=" + srcmeta.getColumnCount());
    		String colname, coltypename;
            for (int i = 1; i <= ncols; i++) {
    			colname = srcmeta.getColumnName(i);
    			coltypename = srcmeta.getColumnTypeName(i);
                System.out.println( colname + " has type=" + srcmeta.getColumnType(i) + " typename=" + coltypename );
    			colnames[i-1] = colname.toLowerCase();
            }
            
            
            // insert statement
            long goodrows = 0, badrows = 0;
    		String val0, val;
            while ( srcrs.next()) {
            	StringBuilder sb = new StringBuilder("insert into " + table + " (");
                for (int i = 0; i < ncols; i++) {
    				if ( 0 == i ) {
    					sb.append( colnames[0] );
    				} else {
    					sb.append( ", " + colnames[i] );
    				}
    			}
    			sb.append( ") values (" );
    
                for (int i = 1; i <= ncols; i++) {
    				val0 = srcrs.getString(i);
    				val = val0.replaceAll("'", "\\\\'");
    				if ( i > 1 ) { sb.append( "," ); }
    				sb.append( "'"+ val + "'" );
                }
    
    			// add extra columns if needed
    			// sb.append( ",'extra1', 'extra2'" );
    
    			sb.append( ")");
    
                if ( tst.executeUpdate( sb.toString() ) > 0 ) {
            		System.out.println( "OK " + sb.toString() );
    				++ goodrows;
    			} else {
            		System.out.println( "ER " + sb.toString() );
    				++ badrows;
    			}
    
    			if ( importrows > 0 ) {
    				if ( ( goodrows+badrows ) >= importrows ) {
    					break;
    				}
    			}
            }

        	System.out.println("Table=" + table + " total goodrows=" + goodrows + " imported. badrows=" + badrows );

		}
        
        sconn.close();
        tconn.close();
        
    }
}
