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
#include "cxxJaguar.h"
#include "cjaguar.h"

Jaguar JaguarInit() {
	cxxJaguar * ret = new cxxJaguar();
	return (void*)ret;
}

void JaguarFree(Jaguar f) {
	cxxJaguar * jaguar = (cxxJaguar*)f;
	delete jaguar;
}

int JaguarConnect(Jaguar f, const char *ipaddress, unsigned int port, const char *username, const char *passwd, 
					const char *dbname ) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->connect(ipaddress, port, username, passwd, dbname );
}

int JaguarExecute(Jaguar f, const char *query) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->execute(query);
}

int JaguarQuery(Jaguar f, const char *query ) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->query(query );
}

int JaguarReply(Jaguar f ) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return (jaguar->reply());
}

const char* JaguarGetSession(Jaguar f) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->getSession();
}
const char* JaguarGetDatabase(Jaguar f) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->getDatabase();
}
void JaguarPrintRow(Jaguar f) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        jaguar->printRow();
}
void JaguarPrintAll(Jaguar f) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        jaguar->printAll();
}
const char* JaguarError(Jaguar f) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->error();
}
int JaguarHasError(Jaguar f) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->hasError();
}
void JaguarFreeResult(Jaguar f) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        jaguar->freeResult();
}
char* JaguarGetNthValue(Jaguar f, int nth) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->getNthValue(nth);
}
char* JaguarAllByIndex(Jaguar f, int nth) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->getAllByIndex(nth);
}

char* JaguarGetValue(Jaguar f, const char* name) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->getValue(name);
}
char* JaguarGetAllByName(Jaguar f, const char* name) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->getAllByName(name);
}

int JaguarGetInt(Jaguar f, const char* name, int *value) {
	cxxJaguar * jaguar = (cxxJaguar*)f;
	return jaguar->getInt(name, value);
}
int JaguarGetLong(Jaguar f, const char* name, long long *value) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->getLong(name, value);
}
int JaguarGetFloat(Jaguar f, const char* name, float *value) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->getFloat(name, value);
}
int JaguarGetDouble(Jaguar f, const char* name, double *value) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->getDouble(name, value);
}
const char* JaguarGetMessage(Jaguar f) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->getMessage();
}
const char* JaguarGetAll(Jaguar f) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->getAll();
}
const char* JaguarGetLastUuid(Jaguar f) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->getLastUuid();
}
void JaguarClose(Jaguar f) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->close();
}
int JaguarGetColumnCount(Jaguar f) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->getColumnCount();
}
int JaguarGetCluster(Jaguar f) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->getCluster();
}
char* JaguarGetCatalogName(Jaguar f, int col) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->getCatalogName(col);
}
char*  JaguarGetColumnClassName(Jaguar f, int col) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->getColumnClassName(col);
}
int JaguarGetColumnDisplaySize(Jaguar f, int col) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->getColumnDisplaySize(col);
}
char* JaguarGetColumnLabel(Jaguar f, int col) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->getColumnLabel(col);
}
char* JaguarGetColumnName(Jaguar f, int col) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->getColumnName(col);
}
int JaguarGetColumnType(Jaguar f, int col) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->getColumnType(col);
}
char* JaguarGetColumnTypeName(Jaguar f, int col) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->getColumnTypeName(col);
}
int JaguarGetScale(Jaguar f, int col) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->getScale(col);
}
char* JaguarGetSchemaName(Jaguar f, int col) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->getSchemaName(col);
}
char* JaguarGetTableName(Jaguar f, int col) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->getTableName(col);
}
bool JaguarIsAutoIncrement(Jaguar f, int col) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->isAutoIncrement(col);
}
bool JaguarIsCaseSensitive(Jaguar f, int col) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->isCaseSensitive(col);
}
bool JaguarIsCurrency(Jaguar f, int col) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->isCurrency(col);
}
bool JaguarIsDefinitelyWritable(Jaguar f, int col) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->isDefinitelyWritable(col);
}
int JaguarIsNullable(Jaguar f, int col) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->isNullable(col);
}
bool JaguarIsReadOnly(Jaguar f, int col) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->isReadOnly(col);
}
bool JaguarIsSearchable(Jaguar f, int col) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->isSearchable(col);
}
bool JaguarIsSigned(Jaguar f, int col) {
        cxxJaguar * jaguar = (cxxJaguar*)f;
        return jaguar->isSigned(col);
}
 
