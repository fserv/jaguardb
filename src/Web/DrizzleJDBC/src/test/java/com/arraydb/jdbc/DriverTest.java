package com.jaguar.jdbc;

import org.junit.Assert;
import org.junit.Test;
import org.junit.After;
import com.jaguar.jdbc.internal.common.packet.buffer.WriteBuffer;
import com.jaguar.jdbc.internal.common.packet.RawPacket;
import com.jaguar.jdbc.internal.common.*;

import static org.junit.Assert.assertFalse;
import static org.mockito.Mockito.mock;

import java.math.BigInteger;
import java.sql.*;
import java.util.List;
import java.util.logging.Logger;
import java.util.logging.Level;
import java.io.*;
import java.math.BigDecimal;
import java.net.URL;

import static junit.framework.Assert.assertEquals;
import static junit.framework.Assert.assertTrue;

/**
 * User: Jon Yue
 * Date: May 22, 2015
 * Time: 12:44 PM
 */
public class DriverTest {
    public static String host = "127.0.0.1";
    private Connection connection;
    static { Logger.getLogger("").setLevel(Level.OFF); }

    public DriverTest() throws SQLException {
       connection = DriverManager.getConnection("jdbc:jaguar://test@"+host+":8900/test");

       // main: connection = DriverManager.getConnection("jdbc:mysql:thin://10.100.100.50:3306/test_units_jdbc");
       //connection = DriverManager.getConnection("jdbc:drizzle://root@"+host+":3307/test_units_jdbc");
       //connection = DriverManager.getConnection("jdbc:mysql://10.100.100.50:3306/test_units_jdbc");
    }
    @After
    public void close() throws SQLException {
        connection.close();
    }
    public Connection getConnection() {
        return connection;
    }
    @Test
    public void doQuery() throws SQLException{
        Statement stmt = getConnection().createStatement();
        try { stmt.execute("drop table t1"); } catch (Exception e) {}
        stmt.execute("create table t1 ( key: id int, value: test char(20))");
        stmt.execute("insert into t1 [ id=1, test='fdfdfd1' ] ");
        stmt.execute("insert into t1 [ id=2, test='fdfdfd2' ] ");
        ResultSet rs = stmt.executeQuery("select * from t1");
        for(int i=1;i<4;i++) {
            rs.next();
        }
    }


    @Test(expected = SQLException.class)
    public void badQuery() throws SQLException {
        Statement stmt = getConnection().createStatement();
        stmt.executeQuery("whraoaooa");
    }

}
