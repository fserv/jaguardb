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

int cxxJaguar::connect( const char *ipaddress, unsigned int port,const char *username, const char *passwd,
					    const char *dbname  )
{
    int rc = jag.connect(ipaddress, port, username, passwd, dbname );
    return rc;
}


int cxxJaguar::execute(const char* query){
	return jag.execute(query);
}

int cxxJaguar::query(const char *query ){
	return jag.query(query );
}

int cxxJaguar::reply(){
	return jag.reply();
}

const char* cxxJaguar::getSession(){
   	return jag.getSession();
}

const char* cxxJaguar::getDatabase(){
    return jag.getDatabase();
}

void cxxJaguar::printRow(){
    jag.printRow();
}
void cxxJaguar::printAll(){
    jag.printAll();
}

const char* cxxJaguar::error(){
    return jag.error();
}

int cxxJaguar::hasError(){
    return jag.hasError();
}

void cxxJaguar::freeResult(){
    jag.freeResult();
}

char* cxxJaguar::getNthValue(int n){
    return jag.getNthValue(n);
}

char* cxxJaguar::getAllByIndex(int n){
    return jag.getAllByIndex(n);
}

char* cxxJaguar::getValue(const char* name){
    return jag.getValue(name);
}
char* cxxJaguar::getAllByName(const char* name){
    return jag.getAllByName(name);
}

int cxxJaguar::getInt(const char* name, int *value){
    return jag.getInt(name, value);
}

int cxxJaguar::getLong(const char* name, long long *value){
    int longNum = jag.getLong(name, value);
    return longNum;
}

int cxxJaguar::getFloat(const char *name, float *value){
    return jag.getFloat(name, value);
}

int cxxJaguar::getDouble(const char* name, double *value){
    return jag.getDouble(name, value);
}

const char* cxxJaguar::getMessage(){
    return jag.getMessage();
}
char* cxxJaguar::getAll(){
    return jag.getAll();
}
char* cxxJaguar::getLastUuid(){
    return jag.getLastUuid();
}

void cxxJaguar::close(){
    jag.close();
}

int cxxJaguar::getColumnCount(){
    return jag.getColumnCount();
}
int cxxJaguar::getCluster(){
    return jag.getCluster();
}

char* cxxJaguar::getCatalogName(int col){
    return jag.getCatalogName(col);
}

char* cxxJaguar::getColumnClassName(int col){
    return jag.getColumnClassName(col);
}

int cxxJaguar::getColumnDisplaySize(int col){
    return jag.getColumnDisplaySize(col);
}

char* cxxJaguar::getColumnLabel(int col){
    return jag.getColumnLabel(col);
}

char* cxxJaguar::getColumnName(int col){
    return jag.getColumnName(col);
}

int cxxJaguar::getColumnType(int col){
    return jag.getColumnType(col);
}

char* cxxJaguar::getColumnTypeName(int col){
    return jag.getColumnTypeName(col);
}

int cxxJaguar::getScale(int col){
    return jag.getScale(col);
}

char* cxxJaguar::getSchemaName(int col){
    return jag.getSchemaName(col);
}

char* cxxJaguar::getTableName(int col){
    return jag.getTableName(col);
}

bool cxxJaguar::isAutoIncrement(int col){
    return jag.isAutoIncrement(col);
}

bool cxxJaguar::isCaseSensitive(int col){
    return jag.isCaseSensitive(col);
}

bool cxxJaguar::isCurrency(int col){
    return jag.isCurrency(col);
}

bool cxxJaguar::isDefinitelyWritable(int col){
    return jag.isDefinitelyWritable(col);
}

int cxxJaguar::isNullable(int col){
    return jag.isNullable(col);
}

bool cxxJaguar::isReadOnly(int col){
    return jag.isReadOnly(col);
}

bool cxxJaguar::isSearchable(int col){
    return jag.isSearchable(col);
}

bool cxxJaguar::isSigned(int col){
    return jag.isSigned(col);
}

