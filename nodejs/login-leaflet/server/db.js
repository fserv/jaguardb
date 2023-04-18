var libname = process.env.HOME + "/jaguar/lib/jaguarnode";
const Jag = require( libname );
const path = require('path')
var jaguar = Jag.JaguarAPI();
jaguar.connect("192.168.7.120", 8899, "admin", "jaguarjaguarjaguar", "test");

app.post('/signin', function (req, res) {
    var sql=req.body.sql;

});

function Hello(sql3) {
    var i = 1;
    var arr = [];
    // var sql3 = "select all(pol) from pol1 where within(pol, square(0 0 10000) );";
    jaguar.query(sql3);
    while (jaguar.reply()) {
        // jaguar.printRow();
        var result = jaguar.getAll(1);
        console.log(result);
    }
    return result;
}

module.exports = Hello;
