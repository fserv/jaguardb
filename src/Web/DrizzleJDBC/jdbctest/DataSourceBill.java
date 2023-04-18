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
import java.sql.Timestamp;


public class DataSourceBill 
{
	public static void main( String [] args ) {
		System.loadLibrary("JaguarClient");
		DataSourceBill t = new DataSourceBill();
		try {
			t.testJaguarDataSource();
		} catch ( SQLException e ) {
		}
	}
    public void testJaguarDataSource() throws SQLException 
	{
        DataSource ds = new JaguarDataSource("127.0.0.1", 8900, "test");  // host port database
        Connection connection = ds.getConnection("test", "test");  // username and password
		Statement statement = connection.createStatement();

		// ResultSet rs = statement.executeQuery("select uid, daytime from tm3 limit 10;");
		System.out.println("execute query ...");
		ResultSet rs = statement.executeQuery("select uid, amt, unit from tm3 where daytime >'2014-02-01 14:00:00' and daytime <'2014-02-01 20:00:00' and utype='H' ;");
		System.out.println("execute query done ");

		String uid;
		String m1;
		String amt;
		String utype;
		Timestamp ts;
		double avg = 0.5;
		String lastuid = "";
		while(rs.next()) {
			uid = rs.getString("uid");
			// m1 = rs.getString("daytime");
			// ts = rs.getString("daytime").getTimestamp();
			amt = rs.getString("amt");
			utype = rs.getString("utype");
			// System.out.println( "uid: " + uid + " daytime: " + m1 + " amt: " + amt + " utype: " + utype  );
			System.out.println( "uid: " + uid + " amt: " + amt + " utype: " + utype  );
			// System.out.println( "ts: " + ts );
			/***
			if ( uid.equals(lastuid) || lastuid.equals("")  ) {
			}
			***/

			lastuid = uid;
		}

		rs.close();
		statement.close();
		connection.close();
    }

}
