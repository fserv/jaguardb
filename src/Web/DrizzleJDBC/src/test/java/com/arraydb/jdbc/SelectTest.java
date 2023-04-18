package com.jaguar.jdbc;

import com.jaguar.jdbc.internal.common.Utils;
import org.junit.Test;

import java.sql.Connection;
import java.sql.Date;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.sql.Time;
import java.sql.Timestamp;
import java.util.logging.Level;
import java.util.logging.Logger;

import static junit.framework.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;


public class SelectTest {
    private Connection connection;

    public SelectTest() throws SQLException {
        // connection = DriverManager.getConnection("jdbc:mysql:thin://"+DriverTest.host+":3306/test_units_jdbc?allowMultiQueries=true");
        connection = DriverManager.getConnection("jdbc:jaguar://"+DriverTest.host+":8900/test?allowMultiQueries=true");
    }

    @Test
    public void SelectTest() throws SQLException {
        Statement statement = connection.createStatement();
        // ResultSet rs = statement.executeQuery("select * from t1;select * from t2;");
        ResultSet rs = statement.executeQuery("select * from jon1;");
        int count = 0;
        while(rs.next()) {
            count++;
        }
        assertTrue(count > 0);
        assertTrue(statement.getMoreResults());
        rs = statement.getResultSet();
        count=0;
        while(rs.next()) {
            count++;
        }
        assertTrue(count>0);
        assertFalse(statement.getMoreResults());

    }
}
