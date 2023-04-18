import com.jaguar.jdbc.JaguarDataSource;
import java.io.*;
import java.sql.DatabaseMetaData;
import javax.sql.DataSource;
import java.sql.SQLException;
import java.sql.Connection;
import java.sql.Statement;
import java.sql.ResultSet;
import com.jaguar.jdbc.JaguarResultSetMetaData;
import java.sql.ResultSetMetaData;

public class DataSourceTest 
{
	public static void main( String [] args ) {
		System.loadLibrary("JaguarClient");
		DataSourceTest t = new DataSourceTest();
		try {
			t.testJaguarDataSource( args[0] );
			// t.testJaguarDataSourcePrepare();
		} catch ( SQLException e ) {
		}
	}
    public void testJaguarDataSource( String ports ) throws SQLException 
	{
        DataSource ds = new JaguarDataSource("127.0.0.1", Integer.parseInt( ports ), "test");  // host port database
        Connection connection = ds.getConnection("admin", "jaguarjaguarjaguar");  // username and password
		Statement statement = connection.createStatement();

		ResultSet rs = statement.executeQuery("select k2, addr, k1 from int10k limit 10;");

		System.out.println( "test ResultSetMetaData before next ...");
		ResultSetMetaData resmeta1 = rs.getMetaData();
		int ct = resmeta1.getColumnType( 1 );
		String n1 = resmeta1.getColumnName(1);
		System.out.println( "colcount =" + ct );
		System.out.println( "colname n1=" + n1 );

		String val;
		String m1;
		while(rs.next()) {
			val = rs.getString("uid");
			m1 = rs.getString("k2");
			System.out.println( "uid: " + val + " m1: " + m1  );
		}

		System.out.println( "test ResultSetMetaData after next ...");
		ResultSetMetaData resmeta = rs.getMetaData();
		ct = resmeta.getColumnType( 1 );
		n1 = resmeta.getColumnName(1);
		System.out.println( "colcount =" + ct );
		System.out.println( "colname n1=" + n1 );

		rs.close();
		statement.close();
		connection.close();
    }

    public void testJaguarDataSourcePrepare() throws SQLException 
	{
        DataSource ds = new JaguarDataSource("127.0.0.1", 8900, "test");  // host port database
        Connection connection = ds.getConnection("test", "test");  // username and password
		ResultSet rs = connection.prepareStatement("select * from jbench limit 10").executeQuery();

		String val;
		String m1;
		while(rs.next()) {
			val = rs.getString("uid");
			m1 = rs.getString("m1");
			System.out.println( "uid: " + val + " m1: " + m1  );
		}

		System.out.println( "test ResultSetMetaData after next ...");
		ResultSetMetaData resmeta = rs.getMetaData();
		int ct = resmeta.getColumnType( 1 );
		String n1 = resmeta.getColumnName(1);
		System.out.println( "coltype=" + ct );
		System.out.println( "colname=" + n1 );

		ct = resmeta.getColumnType( 2 );
		n1 = resmeta.getColumnName(2);
		System.out.println( "coltype=" + ct );
		System.out.println( "colname=" + n1 );

		rs.close();
		connection.close();
    }

}
