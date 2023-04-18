import com.jaguar.jdbc.JaguarDriver;
// import com.jaguar.jdbc.internal.common.*;

import java.math.BigInteger;
import java.sql.*;
import java.util.List;
import java.io.*;
import java.math.BigDecimal;
import java.net.URL;

public class DriverTest {
    public static String host = "127.0.0.1";
    private Connection connection;

    public DriverTest() throws SQLException {
       connection = DriverManager.getConnection("jdbc:jaguar://127.0.0.1:8900/test", "admin", "jaguar" );
    }
    public void close() throws SQLException {
        connection.close();
    }
    public Connection getConnection() {
        return connection;
    }

    public void doQuery() throws SQLException{
        Statement stmt = connection.createStatement();

		System.out.println("drop table t12 ... ");
        try { stmt.executeUpdate("drop table t12"); } catch (Exception e) 
		{ System.out.println( e.getMessage() ); }

        stmt.executeUpdate("create table t12 ( key: id char(10), value: test char(20));");
		System.out.println("Done create table ");

        stmt.executeUpdate("insert into t12 (id, test) values ( '111', 'fdfdfd1' ); ");
        stmt.executeUpdate("insert into t12 (id, test) values ( '112', 'ccccc' ); ");
		System.out.println("Done, insert. executeQuery ...");

        ResultSet rs = stmt.executeQuery("select * from t12;");
		String k;
		String v;
        while ( rs.next() ) {
			k = rs.getString("id");
			v = rs.getString("test");
			System.out.println( "key: "+k + " value: " + v );
        }
		rs.close();

		System.out.println("desc t12 ...");
        rs = stmt.executeQuery("desc t12;");
		String line;
        while ( rs.next() ) {
			line = rs.getString("");
			// line = rs.getString(null);
			System.out.println( line );
        }
		rs.close();
		stmt.close();
    }


 	public static void main( String [] args ) {
    	System.loadLibrary("JaguarClient");

		try {
			Class.forName("com.jaguar.jdbc.JaguarDriver");
		} catch ( ClassNotFoundException e ) {
			System.out.println( e.getMessage() );
		}

    	DriverTest t; 
		try {
    		t = new DriverTest();
			t.doQuery();
			t.close();
		} catch ( SQLException e ) {
			System.out.println( e.getMessage() );
		}
	}
}
