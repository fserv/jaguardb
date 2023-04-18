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

public class JagUtil
{
	public static void logit( String m )
	{
		System.out.println( m );
	}

	public static String[] splitString( String instr, String sep )
	{
		String arr[] = instr.split( sep );
		String newarr[] = new String[arr.length];
		for ( int i = 0; i < arr.length; ++i ) {
			newarr[i] = arr[i].trim();
		}
		return newarr;
	}

}
