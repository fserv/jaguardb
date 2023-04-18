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
package main

import(
  "jaguargo/jaguargo"
  "strconv"
  "flag"
  "fmt"
  "time"
  "os"
)

func main() {
	flag.Parse()
	ports := flag.Arg(0)
    jdb := jaguargo.New()
	fmt.Printf("connecting to jaguardb 127.0.0.1 port=%s\n", ports )
	port, err := strconv.ParseUint(ports, 0, 64 )
	if err != nil {
		fmt.Printf("error\n" )
		os.Exit(1)
	} 

    jdb.Connect("127.0.0.1", uint(port), "admin", "jaguarjaguarjaguar", "test" )
    jdb.Execute("drop table if exists gotab123")
    jdb.Execute("create table gotab123 (key: uid char(32), value: addr char(128) )" )
    jdb.Execute("insert into gotab123 values ( 'id1001', '123 W. Washington Blvd' )")
    jdb.Execute("insert into gotab123 values ( 'id1002', '225 E. Sunshine St' )")

    jdb.Query("show databases" )
	fmt.Printf("List of databases:\n")
    for {
            rc := jdb.Reply()
            if rc > 0 {
                jdb.PrintRow()
            } else {
                break
            }
    }

    jdb.Query("show tables" )
	fmt.Printf("List of tables:\n")
    for {
            rc := jdb.Reply()
            if rc > 0 {
                jdb.PrintRow()
            } else {
                break
            }
    }

	time.Sleep(1*time.Second)
    jdb.Query("select * from gotab123" )
	fmt.Printf("Data in table gotab123:\n")
    for {
            rc := jdb.Reply()
            if rc > 0 {
                jdb.PrintRow()
            } else {
                break
            }
    }
	// fmt.Printf("select done\n" )

    jdb.Close()
}
