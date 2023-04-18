/*
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following
 * conditions are met:
 *
 *  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
 *   disclaimer in the documentation and/or other materials provided with the distribution.
 *  Neither the name of the driver nor the names of its contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

package com.jaguar.jdbc;

import com.jaguar.jdbc.internal.SQLExceptionMapper;
import java.sql.DatabaseMetaData;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;

public class JaguarDataBaseMetaData extends CommonDatabaseMetaData {
    public JaguarDataBaseMetaData(Builder builder) {
        super(builder);
    }

  /**
     * Retrieves a description of the given table's primary key columns.  They are ordered by COLUMN_NAME.
     * <p/>
     * <P>Each primary key column description has the following columns: <OL> <LI><B>TABLE_CAT</B> String => table
     * catalog (may be <code>null</code>) <LI><B>TABLE_SCHEM</B> String => table schema (may be <code>null</code>)
     * <LI><B>TABLE_NAME</B> String => table name <LI><B>COLUMN_NAME</B> String => column name <LI><B>KEY_SEQ</B> short
     * => sequence number within primary key( a value of 1 represents the first column of the primary key, a value of 2
     * would represent the second column within the primary key). <LI><B>PK_NAME</B> String => primary key name (may be
     * <code>null</code>) </OL>
     *
     * @param catalog a catalog name; must match the catalog name as it is stored in the database; "" retrieves those
     *                without a catalog; <code>null</code> means that the catalog name should not be used to narrow the
     *                search
     * @param schema  a schema name; must match the schema name as it is stored in the database; "" retrieves those
     *                without a schema; <code>null</code> means that the schema name should not be used to narrow the
     *                search
     * @param table   a table name; must match the table name as it is stored in the database
     * @return <code>ResultSet</code> - each row is a primary key column description
     * @throws java.sql.SQLException if a database access error occurs
     */
    @Override
    public ResultSet getPrimaryKeys(final String catalog, final String schema, final String table) throws SQLException {
        final String query; 
		String lowtable = table;
		query = "_pkey " + lowtable.toLowerCase();
        final Statement stmt = getConnection().createStatement();
        return stmt.executeQuery(query);
    }

        /**
     * Maps standard table types to mysql ones - helper since table type is never "table" in mysql, it is "base table"
     * @param tableType the table type defined by user
     * @return the internal table type.
     */
    private String mapTableTypes(String tableType) {
        if(tableType.equals("TABLE")) {
            return "BASE TABLE";
        }
        if(tableType.equals("SYSTEM VIEW")) {
            return "VIEW";
        }
        return tableType;
    }

    @Override
    public ResultSet getTables( final String catalog, final String schemaPattern, final String tableNamePattern,
								final String[] types) throws SQLException {

        String query = "_show tables;";
        final Statement stmt = getConnection().createStatement();
        return stmt.executeQuery(query);
    }

    public ResultSet getColumns(final String catalog, final String schemaPattern, final String tableNamePattern,
								final String columnNamePattern) throws SQLException 
	{
        final String query; 
		query = "_describe " + tableNamePattern;
        final Statement stmt = getConnection().createStatement();
        return stmt.executeQuery(query);
    }

    @Override
 	public ResultSet getCatalogs() throws SQLException {
        final String query; 
		query = "_show databases";
        final Statement stmt = getConnection().createStatement();
        return stmt.executeQuery(query);
    }
   


    public ResultSet getExportedKeys(final String catalog, final String schema, final String table) throws SQLException {
        String query = "SELECT null PKTABLE_CAT,\n" +
                "       fk.constraint_schema PKTABLE_SCHEM,\n" +
                "       fk.referenced_table_name PKTABLE_NAME,\n" +
                "       replace(fk.referenced_table_columns,'`','') PKCOLUMN_NAME,\n" +
                "       null FKTABLE_CAT,\n" +
                "       fk.constraint_schema FKTABLE_SCHEM,\n" +
                "       fk.constraint_table FKTABLE_NAME,\n" +
                "       replace(fk.constraint_columns,'`','') FKCOLUMN_NAME,\n" +
                "       1 KEY_SEQ,\n" +
                "       CASE update_rule\n" +
                "            WHEN 'RESTRICT' THEN 1\n" +
                "            WHEN 'NO ACTION' THEN 3\n" +
                "            WHEN 'CASCADE' THEN 0\n" +
                "            WHEN 'SET NULL' THEN 2\n" +
                "            WHEN 'SET DEFAULT' THEN 4\n" +
                "       END UPDATE_RULE,\n" +
                "       CASE delete_rule\n" +
                "            WHEN 'RESTRICT' THEN 1\n" +
                "            WHEN 'NO ACTION' THEN 3\n" +
                "            WHEN 'CASCADE' THEN 0\n" +
                "            WHEN 'SET NULL' THEN 2\n" +
                "            WHEN 'SET DEFAULT' THEN 4\n" +
                "       END DELETE_RULE,\n" +
                "       fk.constraint_name FK_NAME,\n" +
                "       null PK_NAME,\n" +
                "       6 DEFERRABILITY\n" +
                "FROM data_dictionary.foreign_keys fk "+
                "WHERE " +
                (schema != null ? "fk.constraint_schema='" + schema + "' AND " : "") +
                "fk.referenced_table_name='" +
                table +
                "' " +
                "ORDER BY FKTABLE_CAT, FKTABLE_SCHEM, FKTABLE_NAME, KEY_SEQ";
        final Statement stmt = getConnection().createStatement();
        return stmt.executeQuery(query);
    }

    public ResultSet getImportedKeys(final String catalog, final String schema, final String table) throws SQLException {
        final String query = "SELECT null PKTABLE_CAT,\n" +
                "fk.constraint_schema PKTABLE_SCHEM,\n" +
                "fk.referenced_table_name PKTABLE_NAME,\n" +
                "replace(fk.referenced_table_columns,'`','') PKCOLUMN_NAME,\n" +
                "null FKTABLE_CAT,\n" +
                "fk.constraint_schema FKTABLE_SCHEM,\n" +
                "fk.constraint_table FKTABLE_NAME,\n" +
                "replace(fk.constraint_columns,'`','') FKCOLUMN_NAME,\n" +
                "1 KEY_SEQ,\n" +
                "CASE update_rule\n" +
                "   WHEN 'RESTRICT' THEN 1\n" +
                "   WHEN 'NO ACTION' THEN 3\n" +
                "   WHEN 'CASCADE' THEN 0\n" +
                "   WHEN 'SET NULL' THEN 2\n" +
                "   WHEN 'SET DEFAULT' THEN 4\n" +
                "END UPDATE_RULE,\n" +
                "CASE delete_rule\n" +
                "   WHEN 'RESTRICT' THEN 1\n" +
                "   WHEN 'NO ACTION' THEN 3\n" +
                "   WHEN 'CASCADE' THEN 0\n" +
                "   WHEN 'SET NULL' THEN 2\n" +
                "   WHEN 'SET DEFAULT' THEN 4\n" +
                "END DELETE_RULE,\n" +
                "fk.constraint_name FK_NAME,\n" +
                "null PK_NAME,\n" +
                "6 DEFERRABILITY\n" +
                "FROM data_dictionary.foreign_keys fk "+
                "WHERE " +
                (schema != null ? "fk.constraint_schema='" + schema + "' AND " : "") +
                "fk.constraint_table='" +
                table +
                "'" +
                "ORDER BY FKTABLE_CAT, FKTABLE_SCHEM, FKTABLE_NAME, KEY_SEQ";
        final Statement stmt = getConnection().createStatement();
        return stmt.executeQuery(query);
    }
    public ResultSet getBestRowIdentifier(final String catalog, final String schema, final String table, final int scope, final boolean nullable)
            throws SQLException {
        final String query = "SELECT " + DatabaseMetaData.bestRowSession + " scope," +
                "column_name," +
                dataTypeClause + " data_type," +
                "data_type type_name," +
                "if(numeric_precision is null, character_maximum_length, numeric_precision) column_size," +
                "0 buffer_length," +
                "numeric_scale decimal_digits," +
                DatabaseMetaData.bestRowNotPseudo + " pseudo_column" +
                " FROM data_dictionary.columns" +
                " WHERE is_indexed = 'YES' OR is_used_in_primary = 'YES' OR is_unique = 'YES'" +
                " AND table_schema like " + (schema != null ? "'%'" : "'" + schema + "'") +
                " AND table_name='" + table + "' ORDER BY scope";
        final Statement stmt = getConnection().createStatement();
        return stmt.executeQuery(query);
    }

   /**
     * Retrieve a description of the psuedo or hidden columns in a table.
     * @param catalog a catalog name
     * @param schemaPattern a schema name pattern
     * @param tableNamePattern a table name pattern
     * @param columnNamePattern a column name pattern
     * @return Each row in the result set is a column description.
     */
    public ResultSet getPseudoColumns(String catalog,
                         String schemaPattern,
                         String tableNamePattern,
                         String columnNamePattern)
                           throws SQLException {
        throw SQLExceptionMapper.getFeatureNotSupportedException("getPseudoColumns");
    }

    public boolean generatedKeyAlwaysReturned() throws SQLException {
        return false;
    }

}
