import java.io.IOException;
import java.util.*;
import javax.sql.*;
import java.sql.*;
import com.jaguar.jdbc.*;

import org.apache.hadoop.hbase.*;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.client.Table;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.util.Bytes;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.TableName;
import org.apache.hadoop.hbase.client.Admin;
import org.apache.hadoop.hbase.client.Connection;
import org.apache.hadoop.hbase.client.ConnectionFactory;
import org.apache.hadoop.hbase.io.compress.Compression.Algorithm;

// Real example exporting HBase to Jaguar

public class TestHBaseToJaguar {
    public static void main (String[] args) throws IOException {
        Configuration config = HBaseConfiguration.create();
        Connection connection = ConnectionFactory.createConnection(config);
        
        // Please add the name of HBase table in the line below.
		// Delete *** and type the table name.
        String tableName = "test"; 
        Table table = connection.getTable(TableName.valueOf(tableName));
        
        // Jaugar Database needs names of columns to create table.
        // Please provide the names of columns used to create Jaguar table.
        // 
        // Before '_' is the name of one of column families of HBase table.
        // After '_' is the name of one of qualifiers in the column family.
		// More lines can be added below if needed.
		// For example, 
		// jaguarColNames.add("addr_city");
		// addr is the name of one column family and city is the name of 
		// qualifiers in addr family in HBase table.
		// addr_city will be one of columns of the table of Jaguar Database.
        HashSet<String> jaguarColNames = new HashSet<String>();
		jaguarColNames.add("addr_state");
		jaguarColNames.add("addr_city");
		jaguarColNames.add("addr_street");
		jaguarColNames.add("contact_tel");
		jaguarColNames.add("contact_email");

		// Please provide the name of key column when creating Jaguar table.
		String keyJaguar = "name";
		
        // Please provide the names of column families below:
		// More lines can be added below if needed.
        ArrayList<String> familyNamesHBase = new ArrayList<String>();
        familyNamesHBase.add("addr");
        familyNamesHBase.add("contact");
        
        try {
		    // Please provide IP address, listening port, database name in the following line:
			// In JaguarDataSource("***.***.***.***", ****, "***"), the 1st parameter is IP 
			// address connected to Jaguar Database. The 2nd one is port number, which should be
			// an interger and the last one is the name of the database of Jaguar.
            DataSource ds = new JaguarDataSource("192.168.7.120", 5555, "test");

			// Please provide user name and password used to login Jaguar Database below.
			// In getConnection("***", "***"), the 1st parameter is user name and the 2nd one is
			// password.
            java.sql.Connection connectionJ = ds.getConnection("admin", "jaguar");
            Statement statement = connectionJ.createStatement();
            
            //Create a table in Jaguar Database
            String createTable = "create table " + tableName + "( key : " + keyJaguar +  " char(32) , value : ";
            for (String oneCol : jaguarColNames ) {
                createTable += oneCol + " char(32) , ";
            }
            createTable = createTable.substring(0, createTable.length()-2 );
            createTable += ");";
			System.out.println("Create a table in Jaguar DB called" + tableName);
            statement.executeUpdate( createTable );
            
            // Scan HBase table and copy data to Jaguar database
            Scan s = new Scan();
            ResultScanner scanner = table.getScanner(s);
            
			System.out.print("Begin to scan HBase table, " + tableName + ". ");
			System.out.println("And copy data to Jaguar DB.");
            for (Result rnext = scanner.next(); rnext != null; rnext = scanner.next()) {
                String insertCmd1 = "insert into " + tableName + " ( " + keyJaguar  + ", ";
                String insertCmd2 = " values ( '" + Bytes.toString(rnext.getRow()) + "', ";
                for ( String oneFamily : familyNamesHBase ) {
                    NavigableMap<byte[], byte[]> columnValueMap = rnext.getFamilyMap(Bytes.toBytes(oneFamily));
                    for( Map.Entry<byte[], byte[]> entry : columnValueMap.entrySet()){
                        if ( jaguarColNames.contains( oneFamily + "_" + Bytes.toString(entry.getKey()) ) ) {
                            insertCmd1 += oneFamily + "_" + Bytes.toString(entry.getKey())    + ", ";
                            insertCmd2 += "'" + Bytes.toString(entry.getValue()) + "'"  + ", ";
                        }
                    }
                }
                insertCmd1 = insertCmd1.substring(0, insertCmd1.length()-2 );
                insertCmd1 += " )";
                insertCmd2 = insertCmd2.substring(0, insertCmd2.length()-2 );
                insertCmd2 += " );";
                String insertCmd = insertCmd1 + insertCmd2;
                System.out.println( insertCmd );
                statement.executeUpdate( insertCmd );
            }
			statement.close();
			connectionJ.close();
        }
        catch (Exception ex) {
            ex.printStackTrace();
        }
    }
}
