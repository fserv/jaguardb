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

/* Note: you may need to add your own -L path for location of libJaguarClient.so */

package jaguargo

//#cgo LDFLAGS: -L/home/jaguar/jaguar/lib -L/mypath/to/libdir -lJaguarClient -ldl
//#include <stdbool.h>
//#include <cjaguar.h>
import "C"
import "unsafe"

type JaguarGo struct {
	jaguar C.Jaguar
}

func New() JaguarGo {
	var ret JaguarGo
	ret.jaguar = C.JaguarInit()
	return ret
}

func (f JaguarGo) Free() {
	C.JaguarFree(f.jaguar)
}

func (f JaguarGo) Connect( gipaddress string, gport uint, gusername string, gpasswd string, gdbname string ) int {
	var ipaddress *C.char = C.CString(gipaddress)
	var port C.uint = C.uint(gport)
	var username *C.char = C.CString(gusername)
	var passwd *C.char = C.CString(gpasswd)
	var dbname *C.char = C.CString(gdbname)
    return int(C.JaguarConnect(f.jaguar, ipaddress, port, username, passwd, dbname ))
}

func (f JaguarGo) Execute(gquery string) int {
	var query *C.char = C.CString(gquery)
	return int(C.JaguarExecute(f.jaguar, query))
}

func (f JaguarGo) Query(gquery string ) int {
	var query *C.char = C.CString(gquery)
	return int(C.JaguarQuery(f.jaguar, query ))
}

func (f JaguarGo) Reply() int {
	return int(C.JaguarReply(f.jaguar ))
}

func (f JaguarGo) GetSession() string {
    return C.GoString(C.JaguarGetSession(f.jaguar))
}

func (f JaguarGo) GetDatabase() string {
    return C.GoString(C.JaguarGetDatabase(f.jaguar))
}

func (f JaguarGo) PrintRow() {
    C.JaguarPrintRow(f.jaguar)
}

func (f JaguarGo) Error() string {
    return C.GoString(C.JaguarError(f.jaguar))
}

func (f JaguarGo) HasError() int {
    return int(C.JaguarHasError(f.jaguar))
}

func (f JaguarGo) FreeResult() {
    C.JaguarFreeResult(f.jaguar)
}

func (f JaguarGo) GetNthValue(gnth int) string {
	var nth C.int = C.int(gnth)
    return C.GoString(C.JaguarGetNthValue(f.jaguar, nth))
}

func (f JaguarGo) GetValue(gname string) string {
	var name *C.char = C.CString(gname)
    return C.GoString(C.JaguarGetValue(f.jaguar, name))
}

func (f JaguarGo) GetInt(gname string, gvalue *int) int {
	var name *C.char = C.CString(gname)
	var value *C.int = (*C.int)(unsafe.Pointer(gvalue))
	return int(C.JaguarGetInt(f.jaguar, name, value))
}

func (f JaguarGo) GetLong(gname string, gvalue *int64) int {
    var name *C.char = C.CString(gname)
    var value *C.longlong = (*C.longlong)(unsafe.Pointer(gvalue))
    return int(C.JaguarGetLong(f.jaguar, name, value))
}

func (f JaguarGo) GetFloat(gname string, gvalue *float32) int {
    var name *C.char = C.CString(gname)
    var value *C.float = (*C.float)(unsafe.Pointer(gvalue))
    return int(C.JaguarGetFloat(f.jaguar, name, value))
}

func (f JaguarGo) GetDouble(gname string, gvalue *float64) int {
    var name *C.char = C.CString(gname)
    var value *C.double = (*C.double)(unsafe.Pointer(gvalue))
    return int(C.JaguarGetDouble(f.jaguar, name, value))
}

func (f JaguarGo) GetMessage() string {
	return C.GoString(C.JaguarGetMessage(f.jaguar))
}

func (f JaguarGo) Close(){
    C.JaguarClose(f.jaguar)
}

func (f JaguarGo) GetColumnCount() int {
    return int(C.JaguarGetColumnCount(f.jaguar))
}

func (f JaguarGo) GetCatalogName(gcol int) string {
    var col C.int = C.int(gcol)
	return C.GoString(C.JaguarGetCatalogName(f.jaguar, col))
}

func (f JaguarGo) GetColumnClassName(gcol int) string {
    var col C.int = C.int(gcol)
    return C.GoString(C.JaguarGetColumnClassName(f.jaguar, col))
}

func (f JaguarGo) GetColumnDisplaySize(gcol int) int {
    var col C.int = C.int(gcol)
    return int(C.JaguarGetColumnDisplaySize(f.jaguar, col))
}

func (f JaguarGo) GetColumnLabel(gcol int) string {
    var col C.int = C.int(gcol)
    return C.GoString(C.JaguarGetColumnLabel(f.jaguar, col))
}

func (f JaguarGo) GetColumnName(gcol int) string {
    var col C.int = C.int(gcol)
    return C.GoString(C.JaguarGetColumnName(f.jaguar, col))
}

func (f JaguarGo) GetColumnType(gcol int) int {
    var col C.int = C.int(gcol)
    return int(C.JaguarGetColumnType(f.jaguar, col))
}

func (f JaguarGo) GetColumnTypeName(gcol int) string {
    var col C.int = C.int(gcol)
    return C.GoString(C.JaguarGetColumnTypeName(f.jaguar, col))
}

func (f JaguarGo) GetScale(gcol int) int {
    var col C.int = C.int(gcol)
    return int(C.JaguarGetScale(f.jaguar, col))
}

func (f JaguarGo) GetSchemaName(gcol int) string {
    var col C.int = C.int(gcol)
    return C.GoString(C.JaguarGetSchemaName(f.jaguar, col))
}

func (f JaguarGo) GetTableName(gcol int) string {
    var col C.int = C.int(gcol)
    return C.GoString(C.JaguarGetTableName(f.jaguar, col))
}

func (f JaguarGo) IsAutoIncrement(gcol int) bool {
    var col C.int = C.int(gcol)
    return bool(C.JaguarIsAutoIncrement(f.jaguar, col))
}

func (f JaguarGo) IsCaseSensitive(gcol int) bool {
    var col C.int = C.int(gcol)
    return bool(C.JaguarIsCaseSensitive(f.jaguar, col))
}

func (f JaguarGo) IsCurrency(gcol int) bool {
    var col C.int = C.int(gcol)
    return bool(C.JaguarIsCurrency(f.jaguar, col))
}

func (f JaguarGo) IsDefinitelyWritable(gcol int) bool {
    var col C.int = C.int(gcol)
    return bool(C.JaguarIsDefinitelyWritable(f.jaguar, col))
}

func (f JaguarGo) IsNullable(gcol int) int {
    var col C.int = C.int(gcol)
    return int(C.JaguarIsNullable(f.jaguar, col))
}

func (f JaguarGo) IsReadOnly(gcol int) bool {
    var col C.int = C.int(gcol)
    return bool(C.JaguarIsReadOnly(f.jaguar, col))
}

func (f JaguarGo) IsSearchable(gcol int) bool {
    var col C.int = C.int(gcol)
    return bool(C.JaguarIsSearchable(f.jaguar, col))
}

func (f JaguarGo) IsSigned(gcol int) bool {
    var col C.int = C.int(gcol)
    return bool(C.JaguarIsSigned(f.jaguar, col))
}

