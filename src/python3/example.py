#!/usr/bin/python3

###########################################################################
##  Example program of Python for Jaguar
##
##  Copyright  DataJaguar Inc
##
###########################################################################
import jaguarpy, sys

jag = jaguarpy.Jaguar()
rc = jag.connect( "127.0.0.1", sys.argv[1], "admin", "jaguarjaguarjaguar", "test" )
print ("Connected to Jaguar server rc=%d " % ( rc ) )

rc = jag.query( "show tables" );
while  jag.reply():
	jag.printRow();

rc = jag.query( "select * from jbench limit 10" );
while jag.reply():
	jag.printRow();
	u=jag.getValue("uid")
	a=jag.getValue("addr")
	s = 'uid is ' + repr(u) + '  addr is ' + repr(a)
	js=jag.jsonString()
	#print (s)
	print (js)


rc = jag.execute( "drop table t123;");
rc = jag.execute( "create table t123 (key: uid char(32), value: dept char(32) ); " );
rc = jag.execute( "insert into t123 values ( k1, v1 )");
rc = jag.execute( "insert into t123 values ( k2, v1 )");
rc = jag.execute( "insert into t123 values ( k3, v3 )");

rc = jag.query( "select * from t123 limit 10" );
while jag.reply():
	jag.printRow();

rc = jag.execute( "drop table t123;", 0 );

jag.close()
jag = None
