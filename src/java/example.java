import com.jaguar.jdbc.internal.jaguar.Jaguar;
import java.io.*;
import java.util.concurrent.TimeUnit;

// usage: java example <serverIP> <port>
public class example
{
    public static void main(String[] args)
    {
		if ( args.length < 2 ) {
           	System.out.println( "Usage: java example server  port");
			System.exit(1);
		}

        System.out.println( "Server: " + args[0] );
        System.out.println( "Port: " + args[1] );

        System.loadLibrary("JaguarClient");  // need -Djava.library.path in Linux
        Jaguar client = new Jaguar();
        boolean rc = client.connect( args[0], Integer.parseInt( args[1] ), "admin", "jaguarjaguarjaguar", "test", "dummy", 0 );
		if ( ! rc ) {
           	System.out.println( "Connection error");
			System.exit(1);
		}
		
        rc = client.execute( "drop table if exists testjava");
        rc = client.execute( "create table testjava (key: id uuid, value: ssn char(32), model char(32) )  ");

        rc = client.execute( "insert into testjava values (  '3000000', 'm222')" );
		String zuid = client.getLastUuid();
        System.out.println( "lastuuid=[" + zuid + "]" );

        rc = client.execute( "insert into testjava values (  '123000', 'm223' )" );
		//id = client.getLastUuid();
        //System.out.println( "id=" + id );

        rc = client.execute( "insert into testjava values (  '32100000', 'm301' )" );
		//id = client.getLastUuid();
		String js = client.jsonString();
        System.out.println( "jsonString=" + js );

		try { TimeUnit.SECONDS.sleep(1); } catch ( Exception e) { System.out.println( "sleep error"); }
        rc = client.query( "select id, ssn, model from testjava limit 10;" );
        String val;

        while ( client.reply() ) {
            val = client.getValue(  "id" );
            System.out.println( "id=" + val );

            val = client.getValue( "ssn" );
            System.out.println( "ssn=" + val );

            val = client.getValue( "model" );
            System.out.println( "model=" + val );

            val = client.getNthValue( 1 );
            System.out.println( "1-th="+val );

            val = client.getValue( "v1" );
            System.out.println( "v1="+val );

            val = client.getNthValue( 2 );
            System.out.println( "2-nth="+val );

            val = client.getValue( "v2" );
            System.out.println( "v2="+val );

            val = client.getNthValue( 3 );
            System.out.println( "3-th="+val );

            // invalid
            val = client.getNthValue( 5 );
            System.out.println( "5-th=[" + val + "]" );

            val = client.getNthValue( 0 );
            System.out.println( "0-th=[" + val + "]" );

            val = client.getNthValue( -1 );
            System.out.println( "col -1=[" + val + "]" );

            val = client.jsonString();
            System.out.println( "json=[" + val + "]" );
        }

        if ( client.hasError( ) ) {
             String e = client.error( );
             System.out.println( e );
        }

        client.close();
    }
}
