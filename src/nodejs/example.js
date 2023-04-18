// DataJaguar, Inc.  Copyright
// Example javscript program to connect/query jaguardb

var homedir=process.env.HOME;
var libname = homedir + "/jaguar/lib/jaguarnode";
//var libname = './build/Release/jaguarnode'  // using local lib is OK too

const jaguarnode = require( libname )
var jaguar = new jaguarnode.JagAPI();

var port =  process.argv[2];
process.stdout.write("port: " + port + "\n" );
jaguar.connect("127.0.0.1", parseInt( port ), "admin", "jaguarjaguarjaguar", "test");
console.log('connected to jaguardb')

jaguar.execute("create table if not exists nodejs ( key: uid char(16), value: addr char(32) )");
process.stdout.write("create table nodejs done\n");

var rc = jaguar.execute("insert into nodejs values ( 'k1', 'aaa1' );");
process.stdout.write("insert rc=" + rc + "\n" )
jaguar.execute("insert into nodejs values ( 'k2', 'aaa2' );");
jaguar.execute("insert into nodejs values ( 'k3', 'aaa3' )");
jaguar.execute("insert into nodejs values ( 'k4', 'aaa4' )");
process.stdout.write("insert delete update done\n");

jaguar.query("select * from nodejs");
while(jaguar.reply()){
  jaguar.printRow();
  var uid = jaguar.getValue("uid");
  var addr = jaguar.getValue("addr");
  var addr2 = jaguar.getNthValue(2);
  process.stdout.write("uid: " + uid + "  addr: " + addr + "\n");
  process.stdout.write("addr2: " + addr2 + "\n");

  var all = jaguar.getAll();
  var all2 = jaguar.getAllByName("ss4");
  var all3 = jaguar.getAllByIndex(1);
  process.stdout.write("all: " + all + "\n");
  process.stdout.write("all2: " + all2 + "\n");
  process.stdout.write("all3: " + all3 + "\n");
}

process.stdout.write("select done\n");

jaguar.execute("delete from nodejs where uid='k4'");
jaguar.execute("update nodejs set addr='newnew' where uid='k3'");

//jaguar.execute("drop table nodejs");
//process.stdout.write("drop table nodejs done\n");
jaguar.close()
