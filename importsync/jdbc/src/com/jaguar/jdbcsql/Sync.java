package com.jaguar.jdbcsql;

import java.io.FileReader;
import java.io.File;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.ResultSetMetaData;
import java.sql.Statement;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Properties;
import java.util.Date;
import com.jaguar.jdbcsql.JagUtil;


public class Sync 
{
    private static final String SOURCE_JDBCURL = "source_jdbcurl";
    private static final String SOURCE_TABLE = "source_table";
    private static final String SOURCE_PASSWORD = "source_password";
    private static final String SOURCE_USER = "source_user";
    private static final String COM_JAGUAR_JDBC_JAGUAR_DRIVER = "com.jaguar.jdbc.JaguarDriver";
    private static final String DEST_JDBCURL = "dest_jdbcurl";
    private static final String APP_CONF = "appconf";
    private static final String DEST_PASSWORD = "dest_password";
    private static final String DEST_USER = "dest_user";
    private static final String SLEEP_IN_MILLIS = "sleep_in_millis";
    private static final String STOP = "stop";
    private static final String KEEP_ROWS = "keep_rows";
    private static final String D = "D";
    private static final String U = "U";
    private static final String I = "I";
    private static boolean DEBUG = false;
    
    public static void main(String[] args) throws Exception 
	{
        String appConf = System.getProperty(APP_CONF);
        if (appConf == null) {
            logit("Usage: java -cp jar1:jar2:... -Dappconf=<config_file> " + Sync.class.getName());
			File file = new File("java.lock"); file.delete();
            return;
        }

        
        // load Jaguar driver
        try {
            // Class.forName(COM_JAGUAR_JDBC_JAGUAR_DRIVER).newInstance();
            Class.forName(COM_JAGUAR_JDBC_JAGUAR_DRIVER);
        } catch (Exception e) {
            e.printStackTrace();
			logit("Jaguar SyncServer stopped");
			File file = new File("java.lock"); file.delete();
			System.exit(1);
        }

        Properties appProp = new Properties();
        appProp.load(new FileReader(appConf));
        DEBUG = Boolean.parseBoolean(appProp.getProperty("debug"));
        String srcurl = appProp.getProperty(SOURCE_JDBCURL);
        logit("sourceurl " + srcurl);
        String arr[] = srcurl.split(":");
        String source_dbtype = arr[1].toLowerCase();
        String sleepms = appProp.getProperty(SLEEP_IN_MILLIS, "1000" );
		logit("Sleep interval is " + sleepms + " milliseconds" );
        String user = appProp.getProperty(SOURCE_USER);
        String password = appProp.getProperty(SOURCE_PASSWORD);
		logit("Source database user is " + user );
        Connection srcconn = null;
		try {
        	srcconn = DriverManager.getConnection( srcurl, user, password);
		} catch ( Exception e ) {
			logit("Jaguar SyncServer stopped");
			File file = new File("java.lock"); file.delete();
            e.printStackTrace();
			return;
		}

        Statement srcst = srcconn.createStatement();
        // dest database
        String desturl = appProp.getProperty(DEST_JDBCURL).toLowerCase();
        logit("desturl " + desturl);
        String keep_rows = appProp.getProperty(KEEP_ROWS, "10000");
		long keeprows = Long.parseLong( keep_rows );
        String destuser = appProp.getProperty(DEST_USER);
        String destpassword = appProp.getProperty(DEST_PASSWORD);


        // String table = appProp.getProperty(SOURCE_TABLE).toLowerCase(); 
        String tables = appProp.getProperty(SOURCE_TABLE).toLowerCase(); 
		logit("Jaguar SyncServer started for table " + tables + " ...");
		String tabarr[] = JagUtil.splitString( tables, "[|]" );
		int tablen = tabarr.length;
		String lastID[] = new String[tablen];
		String lastTS[] = new String[tablen];
		PreparedStatement updateLogPSArr[]  = new PreparedStatement[tablen];

		// prepare init objects
		String columnNameStr[] = new String[tablen];
		DBAccess destdbarr[] = new DBAccess[tablen];
		for ( int i = 0; i < tablen; ++i ) {
			String table = tabarr[i];
        	String changeLog = table + "_jagchangelog";

            String srcsql = Command.getSelectOneRowSQL( source_dbtype, table );
            ResultSet metars = srcst.executeQuery( srcsql);
            ResultSetMetaData srcmeta = metars.getMetaData();
            String[] columnNames = new String[ srcmeta.getColumnCount()];
            for(int j = 1; j <= srcmeta.getColumnCount(); j++) {
                columnNames[j - 1] = srcmeta.getColumnName(j).toLowerCase();
            }
			columnNameStr[i] = String.join(",", columnNames );
            metars.close();

			updateLogPSArr[i] = srcconn.prepareStatement("update " + changeLog + " set status_='D' where id_=?");

			try {
				destdbarr[i] = new DBAccess( desturl, destuser, destpassword, table, columnNames, DEBUG );
				destdbarr[i].init();
			} catch ( Exception e ) {
				logit("Jaguar SyncServer stopped");
				File file = new File("java.lock"); file.delete();
            	e.printStackTrace();
				return;
			}
		}

        
        long total = 0;
		for ( int i = 0; i < tablen; ++i ) {
			lastID[i] = "0"; lastTS[i] = "0";
		}
		String action, status, ts, id;
		long changenum = 0;
        while ( true ) {
			changenum = 0;
            Properties appPropNew = new Properties();
            appPropNew.load(new FileReader(appConf));
            if (Boolean.parseBoolean(appPropNew.getProperty(STOP))) { break; }
            DEBUG = Boolean.parseBoolean(appPropNew.getProperty("debug"));

			for ( int ti = 0; ti < tablen; ++ ti ) {
				String table = tabarr[ti];
        		String changeLog = table + "_jagchangelog";
                DBAccess destdb = destdbarr[ti];
                // srcst = srcconn.createStatement( ResultSet.TYPE_SCROLL_SENSITIVE );
                String srcsql = "select * from " + changeLog + " where status_='N'";
               	if (DEBUG) { logit("srcsql  " + srcsql ); }
                ResultSet changers = srcst.executeQuery( srcsql);
				PreparedStatement updateLogPS = updateLogPSArr[ti];
                while ( changers.next()) {
               		if (DEBUG) { logit("inside changers.next() " + changers.toString() ); }
                    action = changers.getString("action_");
                    status = changers.getString("status_");
                    ts = changers.getString("ts_");
                    id = changers.getString("id_");
    				if ( DEBUG ) {
                    	logit(" id=" + id + " action=" + action + " status=" + status + " ts=" + ts );
    				}
    				lastID[ti] = id.toString();
    				lastTS[ti] = ts;
    
                    if (I.equals(action)) {
                		if (DEBUG) { logit("Insert " ); }
                        try {
                			if (DEBUG) { logit("destdb.doInsert " ); }
                            destdb.doInsert( changers);
    						++ changenum;
                        } catch (Exception e) {
                            if (DEBUG) {
                                e.printStackTrace();
                            }
                        }
                    } else if (D.equals(action)) {
                		if (DEBUG) { logit("destdb.doDelete " ); }
                        destdb.doDelete( changers);
    					++ changenum;
                    } else {
                		if (DEBUG) { logit("Unknown action " + action ); }
    				}
                    
                    //update log status
                    updateLogPS.clearParameters();
                    // updateLogPS.setObject(1, id);
                    updateLogPS.setLong(1, Long.parseLong(id) );
                    if (DEBUG) { logit("updateLogPS.executeUpdate status_ " + updateLogPS.toString() ); }
                    updateLogPS.executeUpdate();
                    
                    total++;
                }
                
                changers.close();
                if (DEBUG) {
                    logit("changenum=" + changenum + " lastID=" + lastID[ti] + " lastTS=" + lastTS[ti] );
                    logit("Sleep " + sleepms + " millisecs ...");
                }
                Thread.sleep(Long.parseLong( sleepms ) );
    
    			// periodically cleanup changelog table
    			long lastid = Long.parseLong( lastID[ti] );
    			if ( ( (lastid + 1) % keeprows ) == 0 ) {
    				long pastid = lastid - keeprows;
            		Statement chst = srcconn.createStatement();
            		String sql = "delete from " + changeLog + " where id_ < " + pastid;
					if ( DEBUG ) logit( sql );
            		chst.executeUpdate( sql);
    				chst.close();
    			}


			}  // next table
        }

        srcconn.close();
            
        logit( "Total rows synched: " + total);
		for ( int i = 0; i < tablen; ++i ) {
        	logit( "Table " + tabarr[i] + " changelog lastID " + lastID[i] + " lastTS " + lastTS[i] );
			destdbarr[i].close();
			updateLogPSArr[i].close();
		}
		File file = new File("java.lock");
		file.delete();

    }

	public static String nowTime()
	{
		Date now = new Date();
		return now.toString();
	}
	public static void logit ( String msg )
	{
        System.out.println( nowTime() + " " + msg );
	}
}
