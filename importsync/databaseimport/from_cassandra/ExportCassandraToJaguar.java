import java.text.SimpleDateFormat;
import java.util.Iterator;
import java.lang.Integer;
import javax.sql.*;
import java.sql.*;
import com.jaguar.jdbc.*;

import com.datastax.driver.core.DataType;
import com.datastax.driver.core.Cluster;
import com.datastax.driver.core.ColumnDefinitions.Definition;
import com.datastax.driver.core.DataType;
import com.datastax.driver.core.ResultSet;
import com.datastax.driver.core.Row;
import com.datastax.driver.core.Session;
import com.datastax.driver.core.SimpleStatement;
import com.datastax.driver.core.Statement;
import com.datastax.driver.core.querybuilder.QueryBuilder;
import io.netty.util.Timer; 

// Before running this program, please create a table in Jaguar database
// with the SAME table name, SAME column names and SAME number of columns
// as the table in Cassandra.

public class ExportCassandraToJaguar
{
    final static String KEYSPACE         = "mykeyspace";    // name of Cassandra database
	final static String TABLE            = "users";         // name of Cassandra table
	final static String USERNAME         = "username";      // user name to login Cassandra if any
	final static String PASSWORD         = "password";      // password to login Cassandra if any
	final static String IP_CASSANDRA     = "127.0.0.1";     // IP address of Cassandra server
	final static String IP_JAGUAR        = "192.168.7.120"; // IP address of Jaguar server
	final static int    PORT             = 5555;            // listening port for Jaguar
	final static String JAGUARDBNAME     = "test";          // Jaguar database name
	final static String USERNAME_JAGUAR  = "admin";         // user name to login Jaguar
	final static String PASSWORD_JAGUAR  = "jaguar";        // password to login Jaguar
    public static void main( String[] args )
    {
		Cluster.Builder clusterBuilder = Cluster.builder().addContactPoints(IP_CASSANDRA);
		// Uncomment the following line if the Cassandra cluster has been configured to use the PasswordAuthenticator. 
		// Cluster.Builder clusterBuilder = Cluster.builder().addContactPoints(IP_CASSANDRA).withCredentials(USERNAME, PASSWORD);
		Cluster cluster = clusterBuilder.build();
		Session session = cluster.connect(KEYSPACE);
		ResultSet rs = session.execute(QueryBuilder.select().from(KEYSPACE, TABLE));
		Iterator<Row> iter = rs.iterator();
        
        try {
		    
            DataSource ds = new JaguarDataSource(IP_JAGUAR, PORT, JAGUARDBNAME);
            java.sql.Connection connectionJaguar = ds.getConnection(USERNAME_JAGUAR, PASSWORD_JAGUAR);
            java.sql.Statement statementJaguar = connectionJaguar.createStatement();
            for (Row row : rs ) {
                if ( rs.getAvailableWithoutFetching() == 0) {
                    break;
                }
                else {
                    String insertCmd1 = "insert into " + TABLE + " ( ";;
                    String insertCmd2 = " values ( " ;
                    for ( Definition columnName : row.getColumnDefinitions().asList() ) {
                        Object obj = row.getObject(columnName.getName());
                        if ( obj != null ) {
                            insertCmd1 += columnName.getName() + ", ";
                            insertCmd2 += "'" + obj.toString()       + "', ";
                        }
                        else {
                            break;
                        }
                    }
                    insertCmd1 = insertCmd1.substring(0, insertCmd1.length()-2 );
                    insertCmd1 += " )";
                    insertCmd2 = insertCmd2.substring(0, insertCmd2.length()-2 );
                    insertCmd2 += " );";
                    String insertCmd = insertCmd1 + insertCmd2;
                    statementJaguar.executeUpdate( insertCmd );
                    System.out.println( insertCmd );
                }
            }
			System.out.println("Finsh exporting data of table " + TABLE + " from Cassandra to Jaguar." );
            statementJaguar.close();
            connectionJaguar.close();
			System.out.println("Closing connection with Cassandra ...");
			session.close();
		    cluster.close();
        }
        catch (Exception ex) {
            ex.printStackTrace();
        }
	}
}
