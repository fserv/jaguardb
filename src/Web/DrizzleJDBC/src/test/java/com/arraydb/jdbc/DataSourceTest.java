package com.jaguar.jdbc;

import org.junit.Test;
import static org.junit.Assert.assertEquals;

import javax.sql.DataSource;
import java.sql.SQLException;
import java.sql.Connection;

/**
 * Created by IntelliJ IDEA.
 * User: Jon Yue
 * Date: May 22, 2015
 * Time: 12:4PM
 * To change this template use File | Settings | File Templates.
 */
public class DataSourceTest {
    @Test
    public void testArrayDBDataSource() throws SQLException {
        DataSource ds = new JaguarDataSource(DriverTest.host,8900,"test");
        Connection connection = ds.getConnection("test", "test");
        assertEquals(connection.isValid(0),true);
    }
    @Test
    public void testArrayDBDataSource2() throws SQLException {
        DataSource ds = new JaguarDataSource(DriverTest.host,8900,"test");
        Connection connection = ds.getConnection("test","test");
        assertEquals(connection.isValid(0),true);
    }
}
