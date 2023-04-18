import express from 'express';
import bodyParser from 'body-parser';
import jwt from 'jsonwebtoken';
import config from './config'
var libname = process.env.HOME + "/jaguar/lib/jaguarnode";
const Jag = require( libname );
const path = require('path')
var jaguar = Jag.JaguarAPI();

let app = express();

app.use(bodyParser.json());


app.get("/",(req,res) =>{
    res.send("ok")
})

// app.post("/api/users",(req, res) =>{
//     console.log(req.body);
//     let error = {"error":"failed"};
//     res.status(400).json(error);
// })

app.post("/api/login",(req, res) =>{
    console.log(req.body);
    if (jaguar.connect("192.168.7.120", 8899, req.body.username, req.body.password, "test") == 1) {
        // if(req.body.username == "admin" && req.body.password == "admin"){
        const token = jwt.sign({
            username: req.body.username
        }, config.jwtSecret);
        res.json({token})
    }
    // }else(
    //     // res.send(400).json({"error":"Invalid username or password"})
    // );

    // let error = {"error":"failed"};
    // res.body = "hello weiyi";
})

app.post("/api/leaflet",(req,res) =>{
    console.log(typeof (req.body.sql));
    console.log(req.body.sql);
    jaguar.connect("192.168.7.120", 8899, "admin", "jaguarjaguarjaguar", "test");
    jaguar.query(req.body.sql);
    while (jaguar.reply()) {
        // jaguar.printRow();
        var result = jaguar.getAll(1);
        console.log(result);
        res.json(result);
    }
})

process.env.PORT
app.listen( process.env.PORT, ()=>console.log('running'));
