#!/usr/local/bin/ruby

require 'jaguarrb'

a = Jaguar.new()
port = ARGV[0]

a.connect("127.0.0.1", port.to_i(), "admin", "jaguarjaguarjaguar", "test" );
db = a.getDatabase();
print "database is #{db} \n";

a.query("select * from jbench limit 10" );
while  a.reply() 
	a.printRow();
end


a.execute("create table testrb ( key: uid char(32), value: addr char(32) )");
a.execute("insert into testrb values ( 'k1', 'v1' )");
a.execute("insert into testrb values ( 'k2', 'v2' )");
a.execute("insert into testrb values ( 'k3', 'v3' )");
a.query("select * from testrb" );
while  a.reply() 
	a.printRow();
end

a.execute("drop table testrb");

a.close();
