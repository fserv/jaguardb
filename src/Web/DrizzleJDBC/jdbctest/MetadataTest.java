import com.jaguar.jdbc.JaguarDataSource;
import java.io.*;
import java.sql.DatabaseMetaData;
import javax.sql.DataSource;
import java.sql.SQLException;
import java.sql.Connection;
import java.sql.Statement;
import java.sql.ResultSet;

public class MetadataTest 
{
	DatabaseMetaData metadata = null;

	public static void main( String [] args ) {
		System.loadLibrary("JaguarClient");
		MetadataTest t = new MetadataTest();
		try {
			t.testMetaData();
		} catch ( SQLException e ) {
		}
	}

    public void testMetaData() throws SQLException {
        DataSource ds = new JaguarDataSource("127.0.0.1", 8900, "test");  // host port database
        Connection connection = ds.getConnection("test", "test");  // username and password
		Statement statement = connection.createStatement();
		
		metadata = connection.getMetaData();

 		System.out.println("Database Product Name: " + metadata.getDatabaseProductName());
        System.out.println("Database Product Version: " + metadata.getDatabaseProductVersion());
        System.out.println("Logged User: " + metadata.getUserName());
        System.out.println("JDBC Driver: " + metadata.getDriverName());
        System.out.println("Driver Version: " + metadata.getDriverVersion());
        System.out.println("\n");

		String   catalog          = null;
		String   schemaPattern    = null;
		String   tableNamePattern = null;
		String[] types            = null;

		System.out.println("metadata.getTables ... ");
		ResultSet result = metadata.getTables( catalog, schemaPattern, tableNamePattern, types );
		while(result.next()) {
    		String tablecat = result.getString("TABLE_CAT");
    		String tableName = result.getString("TABLE_NAME");
    		String tableType = result.getString("TABLE_TYPE");
        	System.out.println("table: " + tableName + " cat: " + tablecat + " type: " + tableType );
		}
		System.out.println("metadata.getTables done ");
		result.close();

		System.out.println("metadata.getColumns ...");
		result = metadata.getColumns( catalog, schemaPattern, "jbench", "" );
		while(result.next()) {
    		String tableName = result.getString("TABLE_NAME");
    		String dtype = result.getString("DATA_TYPE");
    		String colname = result.getString("COLUMN_NAME");
    		String typename = result.getString("TYPE_NAME");
    		String sz = result.getString("COLUMN_SIZE");
    		String dec = result.getString("DECIMAL_DIGITS");

        	System.out.println("getcolumns: table: " + tableName + " col: " + colname );
        	System.out.println("    datatype: " + dtype + " typename: " + typename );
        	System.out.println("    size: " + sz + " decimaldigits: " + dec );
		}
		System.out.println("metadata.getColumns done ");
		result.close();

		System.out.println("metadata.getCatalogs ...");
		result = metadata.getCatalogs ();
		while(result.next()) {
    		String dbName = result.getString("TABLE_CAT");
        	System.out.println("db: " + dbName );
		}
		System.out.println("metadata.getCatalogs done ");
		result.close();

		statement.close();
		connection.close();
    }
}
