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
#ifndef JAGAPI_H
#define JAGAPI_H

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <node.h>
#include <node_object_wrap.h>
#include "../JaguarAPI.h"
#include "v8/include/v8.h"

class JagAPI : public node::ObjectWrap{
  public:
    static void Init(v8::Local<v8::Object> exports);

  private:
    explicit JagAPI();
    ~JagAPI();

    static void New(const v8::FunctionCallbackInfo<v8::Value>& args);

    static void connect(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void execute( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void query( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void reply( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void getSession( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void getDatabase( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void printRow( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void error( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void hasError( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void freeResult( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void getNthValue( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void getValue( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void getInt( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void getLong( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void getFloat( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void getDouble( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void getMessage( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void getLastUuid( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void jsonString( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void close( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void getColumnCount( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void getCluster( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void getCatalogName( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void getColumnClassName( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void getColumnDisplaySize( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void getColumnLabel( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void getColumnName( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void getColumnType( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void getColumnTypeName( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void getScale( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void getSchemaName( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void getTableName( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void isAutoIncrement( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void isCaseSensitive( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void isCurrency( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void isDefinitelyWritable( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void isNullable( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void isReadOnly( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void isSearchable( const v8::FunctionCallbackInfo<v8::Value>& args );
    static void isSigned( const v8::FunctionCallbackInfo<v8::Value>& args);

    static void printAll( const v8::FunctionCallbackInfo<v8::Value>& args);
    static void getAllByName( const v8::FunctionCallbackInfo<v8::Value>& args);
    static void getAllByIndex( const v8::FunctionCallbackInfo<v8::Value>& args);
    static void getAll( const v8::FunctionCallbackInfo<v8::Value>& args);

    JaguarAPI *jaguarapi;
};
#endif
