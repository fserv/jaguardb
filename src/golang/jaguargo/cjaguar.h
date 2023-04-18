/*
 * Copyright (C) 2018,2019,2020,2021 DataJaguar, Inc.
 *
 * This file is part of JaguarDB.
 *
 * JaguarDB is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * JaguarDB is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with JaguarDB (LICENSE.txt). If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _MY_PACKAGE_CJAG_H_
#define _MY_PACKAGE_CJAG_H_

typedef void* Jaguar;

#ifdef __cplusplus
extern "C" {
#endif

	Jaguar JaguarInit(void);
	void JaguarFree(Jaguar);
	int JaguarConnect(Jaguar j, const char *ip, unsigned int port, const char *username, const char *passwd, const char *dbname );
	int JaguarExecute(Jaguar, const char *query);
	int JaguarQuery(Jaguar, const char *query );
	int JaguarReply(Jaguar );
	const char* JaguarGetSession(Jaguar);
	const char* JaguarGetDatabase(Jaguar);
	void JaguarPrintRow(Jaguar);
	const char* JaguarError(Jaguar);
	int JaguarHasError(Jaguar);
	void JaguarFreeResult(Jaguar);
	char* JaguarGetNthValue(Jaguar, int nth );
	char* JaguarGetValue(Jaguar, const char *name );
	int JaguarGetInt(Jaguar, const char *name, int *value );
	int JaguarGetLong(Jaguar, const char *name, long long *value );
	int JaguarGetFloat(Jaguar, const char *name, float *value );
	int JaguarGetDouble(Jaguar, const char *name, double *value );
	const char* JaguarGetMessage(Jaguar);
	void JaguarClose(Jaguar);
	int JaguarGetColumnCount(Jaguar);
	char* JaguarGetCatalogName(Jaguar, int col );
	char* JaguarGetColumnClassName(Jaguar, int col );
	int JaguarGetColumnDisplaySize(Jaguar, int col );
	char* JaguarGetColumnLabel(Jaguar, int col );
	char* JaguarGetColumnName(Jaguar, int col );
	int JaguarGetColumnType(Jaguar, int col );
	char* JaguarGetColumnTypeName(Jaguar, int col );
	int JaguarGetScale(Jaguar, int col );
	char* JaguarGetSchemaName(Jaguar, int col );
	char* JaguarGetTableName(Jaguar, int col );
	bool JaguarIsAutoIncrement(Jaguar, int col );
	bool JaguarIsCaseSensitive(Jaguar, int col );
	bool JaguarIsCurrency(Jaguar, int col );
	bool JaguarIsDefinitelyWritable(Jaguar, int col );
	int  JaguarIsNullable(Jaguar, int col );
	bool JaguarIsReadOnly(Jaguar, int col );
	bool JaguarIsSearchable(Jaguar, int col );
	bool JaguarIsSigned(Jaguar, int col );

#ifdef __cplusplus
}
#endif

#endif
