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
// https://nodejs.org/api/addons.html#addons_addon_examples myobject.cc
#include "JagAPI.h"
#include <stddef.h>
#include "../JaguarAPI.h"
#include <stdio.h>
#include <string>
#include <node.h>
#include "v8/include/v8.h"

using v8::Context;
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::ObjectTemplate;
using v8::FunctionTemplate;
using v8::Isolate;
using v8::Local;
using v8::Number;
using v8::Object;
using v8::Persistent;
using v8::String;
using v8::Value;

//Persistent<Function> JagAPI::constructor;

JagAPI::JagAPI( ){
  jaguarapi = new JaguarAPI();
}

JagAPI::~JagAPI(){
  delete jaguarapi;
}

void JagAPI::Init(Local<Object> exports){
  Isolate* isolate = exports->GetIsolate();
  Local<Context> context = isolate->GetCurrentContext();

  /** If JagAPI(int n) is used
  Local<ObjectTemplate> addon_data_tpl = ObjectTemplate::New(isolate);
  addon_data_tpl->SetInternalFieldCount(1);  // 1 dummy field for the JagAPI::New()
  Local<Object> addon_data = addon_data_tpl->NewInstance(context).ToLocalChecked();
  **/


  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  //Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New, addon_data); // if JagAPI(int n)
  tpl->SetClassName(String::NewFromUtf8(isolate, "JagAPI").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);
 
  NODE_SET_PROTOTYPE_METHOD(tpl, "connect", connect); 
  NODE_SET_PROTOTYPE_METHOD(tpl, "execute", execute);
  NODE_SET_PROTOTYPE_METHOD(tpl, "query", query);
  NODE_SET_PROTOTYPE_METHOD(tpl, "reply", reply);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getSession", getSession);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getDatabase", getDatabase);
  NODE_SET_PROTOTYPE_METHOD(tpl, "printRow", printRow);
  NODE_SET_PROTOTYPE_METHOD(tpl, "error", error);
  NODE_SET_PROTOTYPE_METHOD(tpl, "hasError", hasError);
  NODE_SET_PROTOTYPE_METHOD(tpl, "freeResult", freeResult);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getNthValue", getNthValue);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getValue", getValue);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getInt", getInt);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getLong", getLong);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getFloat", getDouble);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getMessage", getMessage);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getLastUuid", getLastUuid);
  NODE_SET_PROTOTYPE_METHOD(tpl, "jsonString", jsonString);
  NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getColumnCount", getColumnCount);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getCluster", getCluster);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getCatalogName", getColumnCount);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getColumnClassName", getColumnClassName);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getColumnDisplaySize", getColumnDisplaySize);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getColumnLabel", getColumnLabel);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getColumnName", getColumnName);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getColumnType", getColumnType);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getColumnTypeName", getColumnTypeName);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getScale", getScale);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getSchemaName", getSchemaName);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getTableName", getTableName);
  NODE_SET_PROTOTYPE_METHOD(tpl, "isAutoIncrement", isAutoIncrement);
  NODE_SET_PROTOTYPE_METHOD(tpl, "isCaseSensitive", isCaseSensitive);
  NODE_SET_PROTOTYPE_METHOD(tpl, "isCurrency", isCurrency);
  NODE_SET_PROTOTYPE_METHOD(tpl, "isDefinitelyWritable", isDefinitelyWritable);
  NODE_SET_PROTOTYPE_METHOD(tpl, "isNullable", isNullable);
  NODE_SET_PROTOTYPE_METHOD(tpl, "isReadOnly", isReadOnly);
  NODE_SET_PROTOTYPE_METHOD(tpl, "isSearchable", isSearchable);
  NODE_SET_PROTOTYPE_METHOD(tpl, "isSigned", isSigned);
  NODE_SET_PROTOTYPE_METHOD(tpl, "printAll", printAll);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getAll", getAll);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getAllByName", getAllByName);
  NODE_SET_PROTOTYPE_METHOD(tpl, "getAllByIndex", getAllByIndex);

  Local<Function> constructor = tpl->GetFunction(context).ToLocalChecked();
  //addon_data->SetInternalField(0, constructor); // if JagAPI(int n)
  exports->Set(context, String::NewFromUtf8( isolate, "JagAPI").ToLocalChecked(), constructor).FromJust();
}

void JagAPI::New(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  Local<Context> context = isolate->GetCurrentContext();

  if (args.IsConstructCall()) {
    JagAPI *obj = new JagAPI();
    obj->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
  } else {
	Local<Function> cons = args.Data().As<Object>()->GetInternalField(0).As<Function>();
    Local<Object> instance = cons->NewInstance(context).ToLocalChecked();
    args.GetReturnValue().Set(instance);
  }

}

void JagAPI::connect(const v8::FunctionCallbackInfo<v8::Value>& args){

  Isolate *isolate = args.GetIsolate();
  if(args.Length() < 5){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Needs at least 5 args").ToLocalChecked() ));
    return;
  }
  
  if(!args[0]->IsString() || !args[1]->IsNumber() || !args[2]->IsString() ||
     !args[3]->IsString() || !args[4]->IsString()){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Wrong argument types").ToLocalChecked() ));
    return;
  }
  else if(args.Length() > 5 && !args[5]->IsString()){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Wrong argument types").ToLocalChecked() ));
    return;
  }
  else if(args.Length() > 6 && !args[6]->IsNumber()){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Wrong argument types").ToLocalChecked() ));
    return;
  }

  Local<Context> context = isolate->GetCurrentContext();
  std::string stripaddress = std::string( *(v8::String::Utf8Value( isolate, args[0]->ToString(context).ToLocalChecked() )));

  const char *ipaddress = stripaddress.c_str();
  unsigned int port = args[1]->Uint32Value( context ).FromJust();
  
  std::string strusername = std::string( *(v8::String::Utf8Value( isolate, args[2]->ToString( context ).ToLocalChecked())));
  const char *username = strusername.c_str();
  
  std::string strpasswd = std::string( *(v8::String::Utf8Value( isolate, args[3]->ToString( context ).ToLocalChecked() )));
  const char *passwd = strpasswd.c_str();

  std::string strdbname = std::string( *(v8::String::Utf8Value( isolate, args[4]->ToString( context).ToLocalChecked() )));
  const char *dbname = strdbname.c_str();
  
  const char *unixSocket;
  unsigned long long clientFlag;
  if(args.Length() > 5){
    std::string strunixsocket = std::string( *(v8::String::Utf8Value( isolate, args[5]->ToString( context ).ToLocalChecked() )));
    unixSocket = strunixsocket.c_str();
  } else{
    unixSocket = NULL;
  }

  if(args.Length() > 6){
    clientFlag = (unsigned long)args[6]->Uint32Value( context ).FromJust();
  } else {
    clientFlag = 0;
  }

  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  int result = jag->jaguarapi->JaguarAPI::connect(ipaddress, port, username, 
                                    passwd, dbname, unixSocket, clientFlag);
  args.GetReturnValue().Set(v8::Integer::New(isolate, result) );
}

void JagAPI::error(const v8::FunctionCallbackInfo<v8::Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() > 0){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Doesn't take args").ToLocalChecked() ));
    return;
  }
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  const char *result = jag->jaguarapi->JaguarAPI::error();
  args.GetReturnValue().Set(String::NewFromUtf8(isolate, result).ToLocalChecked() );
}

void JagAPI::execute(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if (args.Length() != 1 || !args[0]->IsString()){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Takes one string only").ToLocalChecked() ));
    return;
  }

  Local<Context> context = isolate->GetCurrentContext();

  std::string strquery = std::string( *(v8::String::Utf8Value(isolate, args[0]->ToString(context).ToLocalChecked() )));
  const char *query = strquery.c_str();
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  int result = jag->jaguarapi->JaguarAPI::execute(query);
  // printf("j222209 execute(%s) result=%d\n", query, result ); fflush(stdout);
  args.GetReturnValue().Set(v8::Integer::New(isolate, result));
}

void JagAPI::query(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if (args.Length() < 1 || !args[0]->IsString()){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Takes a string and (optional) boolean").ToLocalChecked() ));
    return;
  } else if(args.Length() > 2 && !args[1]->IsBoolean()){
    isolate->ThrowException(v8::Exception::TypeError( 
        String::NewFromUtf8(isolate, "Takes a string and (optional) boolean").ToLocalChecked() ));
    return;
  }
  
  Local<Context> context = isolate->GetCurrentContext();
  std::string strquery = std::string( *(v8::String::Utf8Value(isolate, args[0]->ToString( context).ToLocalChecked() )));
  const char *query = strquery.c_str();
  bool reply = true;
  if(args.Length() == 2){
    Local<Value> locreply = args[1];
    reply = locreply->BooleanValue( isolate );
  }
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  int result = jag->jaguarapi->JaguarAPI::query(query, reply);
  args.GetReturnValue().Set(v8::Integer::New(isolate, result));
}

void JagAPI::reply(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  bool headerOnly = false;
  if (args.Length() == 1 && args[0]->IsBoolean()){
    Local<Value> locheaderonly = args[0];
    headerOnly = locheaderonly->BooleanValue( isolate );
  } else if(args.Length() > 1){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Optionally takes a boolean").ToLocalChecked() ));
    return;
  }

  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  int result = jag->jaguarapi->JaguarAPI::reply(headerOnly);
  args.GetReturnValue().Set(v8::Integer::New(isolate, result));
}

void JagAPI::getSession(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() > 0){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Doesn't take parameters").ToLocalChecked() ));
    return;
  }
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  std::string result = jag->jaguarapi->JaguarAPI::getSession();
  args.GetReturnValue().Set(String::NewFromUtf8(isolate, result.c_str()).ToLocalChecked() );
}

void JagAPI::getDatabase(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() > 0){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Doesn't take parameters").ToLocalChecked() ));
    return;
  }
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  std::string result = jag->jaguarapi->JaguarAPI::getDatabase();
  args.GetReturnValue().Set(String::NewFromUtf8(isolate, result.c_str()).ToLocalChecked());
}

void JagAPI::printRow(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() > 0){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Doesn't take parameters").ToLocalChecked() ));
    return;
  }
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  int result = jag->jaguarapi->JaguarAPI::printRow();
  args.GetReturnValue().Set(v8::Integer::New(isolate, result));
}

void JagAPI::hasError(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() > 0){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Doesn't take parameters").ToLocalChecked() ));
    return;
  }
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  int result = jag->jaguarapi->JaguarAPI::hasError();
  args.GetReturnValue().Set(v8::Integer::New(isolate, result));
}

void JagAPI::freeResult(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() > 0){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Doesn't take parameters").ToLocalChecked() ));
    return;
  }
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  int result = jag->jaguarapi->JaguarAPI::freeResult();
  args.GetReturnValue().Set(v8::Integer::New(isolate, result));
}

void JagAPI::getNthValue(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() != 1 || !args[0]->IsNumber()){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Takes one int as parameter").ToLocalChecked() ));
    return;
  }

  Local<Context> context = isolate->GetCurrentContext();
  int nth = args[0]->Int32Value( context ).FromJust();
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  std::string result = jag->jaguarapi->JaguarAPI::getNthValue(nth);
  args.GetReturnValue().Set(String::NewFromUtf8(isolate, result.c_str()).ToLocalChecked());
}

void JagAPI::getValue(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() != 1 || !args[0]->IsString()){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Takes one string as parameter").ToLocalChecked() ));
    return;
  }

  Local<Context> context = isolate->GetCurrentContext();
  std::string strname = std::string(
                            *(v8::String::Utf8Value(isolate, args[0]->ToString(context).ToLocalChecked() )));
  const char *name = strname.c_str();
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  char *p = jag->jaguarapi->JaguarAPI::getValue(name);
  if ( p ) { 
  	args.GetReturnValue().Set(String::NewFromUtf8(isolate, p).ToLocalChecked());
	free( p );
  } else {
  	args.GetReturnValue().Set(String::NewFromUtf8(isolate, "").ToLocalChecked());
  }
}

void JagAPI::getInt(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() != 2 || !args[0]->IsString() || !args[1]->IsNumber()){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Take string and int as parameters").ToLocalChecked() ));
    return;
  }
  Local<Context> context = isolate->GetCurrentContext();
  std::string strname = std::string(
                            *(v8::String::Utf8Value(isolate, args[0]->ToString(context).ToLocalChecked() )));
  const char *name = strname.c_str();
  /***
  int val = args[1]->Int32Value( context ).FromJust();
  int *value = &val;
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  int result = jag->jaguarapi->JaguarAPI::getInt(name, value);
  args.GetReturnValue().Set(v8::Integer::New(isolate, result));
  ***/

  int value = 0;
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  jag->jaguarapi->JaguarAPI::getInt(name, &value);
  args.GetReturnValue().Set(v8::Integer::New(isolate, value));
}

void JagAPI::getLong(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() != 2 || !args[0]->IsString() || !args[1]->IsNumber()){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Take string and int as parameters").ToLocalChecked() ));
    return;
  }
  Local<Context> ctx = isolate->GetCurrentContext();
  std::string strname = std::string(
                            *(v8::String::Utf8Value(isolate, args[0]->ToString(ctx).ToLocalChecked() )));
  const char *name = strname.c_str();
  /***
  long long val = args[1]->Int32Value( ctx ).FromJust();
  long long *value = &val;
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  int result = jag->jaguarapi->JaguarAPI::getLong(name, value);
  args.GetReturnValue().Set(v8::Integer::New(isolate, result));
  ***/
  long long value = 0;
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  jag->jaguarapi->JaguarAPI::getLong(name, &value);
  args.GetReturnValue().Set(v8::Integer::New(isolate, value));
}

void JagAPI::getFloat(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() != 2 || !args[0]->IsString() || !args[1]->IsNumber()){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Take string and int as parameters").ToLocalChecked() ));
    return;
  }
  Local<Context> ctx = isolate->GetCurrentContext();
  std::string strname = std::string(
                            *(v8::String::Utf8Value(isolate, args[0]->ToString(ctx).ToLocalChecked() )));
  const char *name = strname.c_str();
  /**
  float val = args[1]->Int32Value( ctx ).FromJust();
  float *value = &val;
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  int result = jag->jaguarapi->JaguarAPI::getFloat(name, value);
  args.GetReturnValue().Set(v8::Integer::New(isolate, result));
  **/
  float value = 0.0;
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  jag->jaguarapi->JaguarAPI::getFloat(name, &value);
  args.GetReturnValue().Set(v8::Number::New(isolate, value));
}

void JagAPI::getDouble(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() != 2 || !args[0]->IsString() || !args[1]->IsNumber()){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Take string and int as parameters").ToLocalChecked() ));
    return;
  }
  Local<Context> ctx = isolate->GetCurrentContext();
  std::string strname = std::string(
                            *(v8::String::Utf8Value(isolate, args[0]->ToString(ctx).ToLocalChecked() )));
  const char *name = strname.c_str();
  /**
  double val;
  double *value = &val;
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  int result = jag->jaguarapi->JaguarAPI::getDouble(name, value);
  args.GetReturnValue().Set(v8::Integer::New(isolate, result));
  **/
  double value = 0.0;
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  jag->jaguarapi->JaguarAPI::getDouble(name, &value);
  args.GetReturnValue().Set(v8::Number::New(isolate, value));
}

void JagAPI::getMessage(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() > 0){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Doesn't take parameters").ToLocalChecked() ));
    return;
  }
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  std::string result = jag->jaguarapi->JaguarAPI::getMessage();
  args.GetReturnValue().Set(String::NewFromUtf8(isolate, result.c_str()).ToLocalChecked() );
}

void JagAPI::getLastUuid(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() > 0){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Doesn't take parameters").ToLocalChecked() ));
    return;
  }
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  std::string result = jag->jaguarapi->JaguarAPI::getLastUuid();
  args.GetReturnValue().Set(String::NewFromUtf8(isolate, result.c_str()).ToLocalChecked() );
}

void JagAPI::jsonString(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() > 0){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Doesn't take parameters").ToLocalChecked() ));
    return;
  }
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  std::string result = jag->jaguarapi->JaguarAPI::jsonString();
  args.GetReturnValue().Set(String::NewFromUtf8(isolate, result.c_str()).ToLocalChecked());
}

void JagAPI::close(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() > 0){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Doesn't take parameters").ToLocalChecked() ));
    return;
  }
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  jag->jaguarapi->JaguarAPI::close();
  args.GetReturnValue().Set(v8::Null(isolate));
}

void JagAPI::getColumnCount(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() > 0){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Doesn't take parameters").ToLocalChecked() ));
    return;
  }
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  int result = jag->jaguarapi->JaguarAPI::getColumnCount();
  args.GetReturnValue().Set(v8::Integer::New(isolate, result));
}

void JagAPI::getCluster(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() > 0){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Doesn't take parameters").ToLocalChecked() ));
    return;
  }
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  int result = jag->jaguarapi->JaguarAPI::getCluster();
  args.GetReturnValue().Set(v8::Integer::New(isolate, result));
}

void JagAPI::getCatalogName(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() != 0 || !args[0]->IsNumber()){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Takes one int as parameter").ToLocalChecked() ));
    return;
  }
  Local<Context> ctx = isolate->GetCurrentContext();
  int col = args[1]->Int32Value( ctx ).FromJust();
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  std::string result = jag->jaguarapi->JaguarAPI::getCatalogName(col);
  args.GetReturnValue().Set(String::NewFromUtf8(isolate, result.c_str()).ToLocalChecked() );
}

void JagAPI::getColumnClassName(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() != 0 || !args[0]->IsNumber()){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Takes one int as parameter").ToLocalChecked() ));
    return;
  }
  Local<Context> ctx = isolate->GetCurrentContext();
  int col = args[1]->Int32Value( ctx ).FromJust();
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  std::string result = jag->jaguarapi->JaguarAPI::getColumnClassName(col);
  args.GetReturnValue().Set(String::NewFromUtf8(isolate, result.c_str()).ToLocalChecked() );
}

void JagAPI::getColumnDisplaySize(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() != 0 || !args[0]->IsNumber()){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Takes one int as parameter").ToLocalChecked() ));
    return;
  }
  Local<Context> ctx = isolate->GetCurrentContext();
  int col = args[1]->Int32Value( ctx ).FromJust();
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  int result = jag->jaguarapi->JaguarAPI::getColumnDisplaySize(col);
  args.GetReturnValue().Set(v8::Integer::New(isolate, result));
}

void JagAPI::getColumnLabel(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() != 0 || !args[0]->IsNumber()){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Takes one int as parameter").ToLocalChecked() ));
    return;
  }
  Local<Context> ctx = isolate->GetCurrentContext();
  int col = args[1]->Uint32Value( ctx ).FromJust();
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  std::string result = jag->jaguarapi->JaguarAPI::getColumnLabel(col);
  args.GetReturnValue().Set(String::NewFromUtf8(isolate, result.c_str()).ToLocalChecked() );
}

void JagAPI::getColumnName(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() != 0 || !args[0]->IsNumber()){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Takes one int as parameter").ToLocalChecked() ));
    return;
  }
  Local<Context> ctx = isolate->GetCurrentContext();
  int col = args[1]->Uint32Value( ctx ).FromJust();
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  std::string result = jag->jaguarapi->JaguarAPI::getColumnName(col);
  args.GetReturnValue().Set(String::NewFromUtf8(isolate, result.c_str()).ToLocalChecked() );
}

void JagAPI::getColumnType(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() != 0 || !args[0]->IsNumber()){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Takes one int as parameter").ToLocalChecked() ));
    return;
  }
  Local<Context> ctx = isolate->GetCurrentContext();
  int col = args[1]->Uint32Value( ctx ).FromJust();
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  int result = jag->jaguarapi->JaguarAPI::getColumnType(col);
  args.GetReturnValue().Set(v8::Integer::New(isolate, result));
}

void JagAPI::getColumnTypeName(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() != 0 || !args[0]->IsNumber()){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Takes one int as parameter").ToLocalChecked() ));
    return;
  }
  Local<Context> ctx = isolate->GetCurrentContext();
  int col = args[1]->Uint32Value( ctx ).FromJust();
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  std::string result = jag->jaguarapi->JaguarAPI::getColumnTypeName(col);
  args.GetReturnValue().Set(String::NewFromUtf8(isolate, result.c_str()).ToLocalChecked() );
}

void JagAPI::getScale(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() != 0 || !args[0]->IsNumber()){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Takes one int as parameter").ToLocalChecked() ));
    return;
  }
  Local<Context> ctx = isolate->GetCurrentContext();
  int col = args[1]->Uint32Value( ctx ).FromJust();
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  int result = jag->jaguarapi->JaguarAPI::getScale(col);
  args.GetReturnValue().Set(v8::Integer::New(isolate, result));
}

void JagAPI::getSchemaName(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() != 0 || !args[0]->IsNumber()){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Takes one int as parameter").ToLocalChecked() ));
    return;
  }
  Local<Context> ctx = isolate->GetCurrentContext();
  int col = args[1]->Uint32Value( ctx ).FromJust();
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  std::string result = jag->jaguarapi->JaguarAPI::getSchemaName(col);
  args.GetReturnValue().Set(String::NewFromUtf8(isolate, result.c_str()).ToLocalChecked() );
}

void JagAPI::getTableName(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() != 0 || !args[0]->IsNumber()){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Takes one int as parameter").ToLocalChecked() ));
    return;
  }

  Local<Context> ctx = isolate->GetCurrentContext();
  int col = args[1]->Uint32Value( ctx ).FromJust();
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  std::string result = jag->jaguarapi->JaguarAPI::getTableName(col);
  args.GetReturnValue().Set(String::NewFromUtf8(isolate, result.c_str()).ToLocalChecked() );
}

void JagAPI::isAutoIncrement(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() != 0 || !args[0]->IsNumber()){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Takes one int as parameter").ToLocalChecked() ));
    return;
  }

  Local<Context> ctx = isolate->GetCurrentContext();
  int col = args[1]->Uint32Value( ctx ).FromJust();
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  bool result = jag->jaguarapi->JaguarAPI::isAutoIncrement(col);
  args.GetReturnValue().Set(v8::Boolean::New(isolate, result));
}

void JagAPI::isCaseSensitive(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() != 0 || !args[0]->IsNumber()){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Takes one int as parameter").ToLocalChecked() ));
    return;
  }
  Local<Context> ctx = isolate->GetCurrentContext();
  int col = args[1]->Uint32Value( ctx ).FromJust();
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  bool result = jag->jaguarapi->JaguarAPI::isCaseSensitive(col);
  args.GetReturnValue().Set(v8::Boolean::New(isolate, result));
}

void JagAPI::isCurrency(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() != 0 || !args[0]->IsNumber()){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Takes one int as parameter").ToLocalChecked() ));
    return;
  }
  Local<Context> ctx = isolate->GetCurrentContext();
  int col = args[1]->Uint32Value( ctx ).FromJust();
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  bool result = jag->jaguarapi->JaguarAPI::isCurrency(col);
  args.GetReturnValue().Set(v8::Boolean::New(isolate, result));
}

void JagAPI::isDefinitelyWritable(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() != 0 || !args[0]->IsNumber()){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Takes one int as parameter").ToLocalChecked() ));
    return;
  }
  Local<Context> ctx = isolate->GetCurrentContext();
  int col = args[1]->Uint32Value( ctx ).FromJust();
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  bool result = jag->jaguarapi->JaguarAPI::isDefinitelyWritable(col);
  args.GetReturnValue().Set(v8::Boolean::New(isolate, result));
}

void JagAPI::isNullable(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() != 0 || !args[0]->IsNumber()){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Takes one int as parameter").ToLocalChecked() ));
    return;
  }
  Local<Context> ctx = isolate->GetCurrentContext();
  int col = args[1]->Uint32Value( ctx ).FromJust();
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  int result = jag->jaguarapi->JaguarAPI::isNullable(col);
  args.GetReturnValue().Set(v8::Integer::New(isolate, result));
}

void JagAPI::isReadOnly(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() != 0 || !args[0]->IsNumber()){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Takes one int as parameter").ToLocalChecked() ));
    return;
  }
  Local<Context> ctx = isolate->GetCurrentContext();
  int col = args[1]->Uint32Value( ctx ).FromJust();
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  bool result = jag->jaguarapi->JaguarAPI::isReadOnly(col);
  args.GetReturnValue().Set(v8::Boolean::New(isolate, result));
}

void JagAPI::isSearchable(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() != 0 || !args[0]->IsNumber()){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Takes one int as parameter").ToLocalChecked() ));
    return;
  }
  Local<Context> ctx = isolate->GetCurrentContext();
  int col = args[1]->Uint32Value( ctx ).FromJust();
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  bool result = jag->jaguarapi->JaguarAPI::isSearchable(col);
  args.GetReturnValue().Set(v8::Boolean::New(isolate, result));
}

void JagAPI::isSigned(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() != 0 || !args[0]->IsNumber()){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Takes one int as parameter").ToLocalChecked() ));
    return;
  }
  Local<Context> ctx = isolate->GetCurrentContext();
  int col = args[1]->Uint32Value( ctx ).FromJust();
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  bool result = jag->jaguarapi->JaguarAPI::isSigned(col);
  args.GetReturnValue().Set(v8::Boolean::New(isolate, result));
}

void JagAPI::getAllByName(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() != 1 || !args[0]->IsString()){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Takes one string as parameter").ToLocalChecked() ));
    return;
  }
  Local<Context> ctx = isolate->GetCurrentContext();
  std::string strname = std::string(
                            *(v8::String::Utf8Value(isolate, args[0]->ToString(ctx).ToLocalChecked() )));
  const char *name = strname.c_str();
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  char *p = jag->jaguarapi->JaguarAPI::getAllByName(name);
  if ( p ) {
  	args.GetReturnValue().Set(String::NewFromUtf8(isolate, p).ToLocalChecked());
	free(p);
  } else {
  	args.GetReturnValue().Set(String::NewFromUtf8(isolate, "").ToLocalChecked());
  }
}

void JagAPI::getAll(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  char *p = jag->jaguarapi->JaguarAPI::getAll();
  if ( p ) {
  	args.GetReturnValue().Set(String::NewFromUtf8(isolate, p).ToLocalChecked());
	free(p);
  } else {
  	args.GetReturnValue().Set(String::NewFromUtf8(isolate, "").ToLocalChecked());
  }
}

void JagAPI::getAllByIndex(const FunctionCallbackInfo<Value>& args){
  Isolate* isolate = args.GetIsolate();
  if(args.Length() != 1 || !args[0]->IsNumber()){
    isolate->ThrowException(v8::Exception::TypeError(
        String::NewFromUtf8(isolate, "Takes one int as parameter").ToLocalChecked() ));
    return;
  }
  Local<Context> ctx = isolate->GetCurrentContext();
  int nth = args[0]->Uint32Value( ctx ).FromJust();
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  char *p = jag->jaguarapi->JaguarAPI::getAllByIndex(nth);
  if ( p ) {
  	args.GetReturnValue().Set(String::NewFromUtf8(isolate, p).ToLocalChecked());
	free(p);
  } else {
  	args.GetReturnValue().Set(String::NewFromUtf8(isolate, "").ToLocalChecked());
  }
}


void JagAPI::printAll(const FunctionCallbackInfo<Value>& args){
  JagAPI *jag = ObjectWrap::Unwrap<JagAPI>(args.This());
  jag->jaguarapi->JaguarAPI::printAll();
}

