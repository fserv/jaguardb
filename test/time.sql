
drop table if exists tr1;
create table timeseries(5m) tr1 (  key: k1 int, ts timestamp, value: v1 rollup double );

insert into tr1 ( k1, ts, v1 ) values ('9', '', '247' );
insert into tr1 ( k1, ts, v1 ) values ('13', '', '47' );
insert into tr1 ( k1, ts, v1 ) values ('13', '', '53' );
insert into tr1 ( k1, ts, v1 ) values ('13', '', '200' );

expect rows 3;
select * from tr1@5m;

select * from tr1@5m where k1=13;

expect rows 4;
select * from tr1;

expect rows 1;
select * from tr1 where k1=9;

expect rows 3;
select * from tr1 where k1=13;


drop table if exists tr2;
create table timeseries(5m) tr2 (  key: k1 int, ts timestamp, value: v1 rollup double, v2 rollup double );

insert into tr2 ( k1, ts, v1, v2 ) values ('9', '', '247', '10' );

insert into tr2 ( k1, ts, v1, v2 ) values ('13', '', '47', '20' );
insert into tr2 ( k1, ts, v1, v2 ) values ('13', '', '53', '20' );
insert into tr2 ( k1, ts, v1, v2 ) values ('13', '', '200','30' );

expect rows 4;
select * from tr2;

expect rows 1;
select * from tr2 where k1=9;

expect rows 3;
select * from tr2 where k1=13;

expect rows 3;
select * from tr2@5m;

###############
drop table if exists tc1;
create table timeseries(5m) tc1 (  key: k1 int, c1 char(2), ts timestamp, value: v1 rollup int, v2 int );
expect words "tc1 k1 ts v1 v2";
desc tc1;

drop table if exists ts1;
create table timeseries(5m) ts1 (  key: k1 int, ts timestamp, value: v1 rollup int, v2 int );
expect words "ts1 k1 ts v1 v2";
desc ts1;

insert into ts1 ( k1, v1, v2 ) values ('5', '103', '247' );
insert into ts1 ( k1, v1, v2 ) values ('5', '303', '253' );
insert into ts1 ( k1, v1, v2 ) values ('5', '503', '553' );
insert into ts1 ( k1, v1, v2 ) values ('5', '903', '153' );
insert into ts1 ( k1, v1, v2 ) values ('5', '1903', '153' );
insert into ts1 ( k1, v1, v2 ) values ('6', '10', '29' );
insert into ts1 ( k1, v1, v2 ) values ('6', '100', '29' );

expect rows 7;
select * from ts1;

select * from ts1@5m;

drop table if exists ts1002;
create table timeseries(10s) ts1002 (  key: k1 int, ts timestamp, value: v1 rollup int, v2 int );
expect words "k1 ts v1 v2";
desc ts1002;


insert into ts1002 ( k1, v1, v2 ) values ('5', '100', '200' );

expect rows 1;
select * from ts1002;

expect rows 2;
select * from ts1002@10s;

drop table if exists ts2;
create table timeseries(5m) ts2 (  key: k1 int, ts timestamp, value: v1 rollup int, v2 int, v3 rollup int );
expect words "k1 ts v1 v2 v3";
desc ts2;

drop index if exists ts2idx1 on ts2;
create index ts2idx1 on ts2(v1, k1);

drop index if exists ts2idx2 on ts2;
create index ts2idx2 ticks on ts2(v3, v1, k1);

drop index if exists ts2idx3 on ts2;
create index ts2idx3 ticks on ts2(v3, k1, v1);

drop index if exists ts2idx4 on ts2;
create index ts2idx4 ticks on ts2(key: v3, k1, value: v1);
desc ts2idx4;

insert into ts2 values ('5', '', '100', '200', '111' );
insert into ts2 values ('5', '', '100', '200', '1123' );
insert into ts2 values ('6', '', '100', '200', '213' );
insert into ts2 values ('7', '', '100', '200', '233' );
insert into ts2 values ('6', '', '100', '200', '322' );

expect rows 5;
select * from ts2;

## two 5 keys goto one 5 min window
expect rows 4;
select * from ts2@5m;

insert into ts2 (k1, v2, v3 ) values ('10', '243', '200' );

expect rows 12;
select * from test.ts2.ts2idx4@5m;

expect rows 3;
select * from test.ts2.ts2idx1;

expect rows 6;
select * from test.ts2.ts2idx2;

expect rows 6;
select * from test.ts2.ts2idx3;

expect rows 6;
select * from test.ts2.ts2idx4;


drop table if exists ts2002;
create table timeseries(5m|10m) ts2002 (  key: k1 int, ts timestamp, value: v1 rollup int, v2 int );
insert into ts2002 values ('5', '', '100', '200' );
insert into ts2002 values ('5', '', '100', '200' );
insert into ts2002 values ('6', '', '100', '200' );
insert into ts2002 values ('6', '', '100', '200' );
insert into ts2002 values ('7', '', '100', '200' );
insert into ts2002 values ('6', '', '100', '200' );
insert into ts2002 values ('6', '', '100', '200' );

expect rows 7;
select * from ts2002;

# 3 keys and a key=*
expect rows 4;
select * from ts2002@5m;

drop table if exists ts3;
create table timeseries(1h:0h) ts3 ( key: k1 int, ts timestamp, k2 int, k3 char(10), k4 char(12), k5 char(23), value: b rollup int, c int, c2 int, c3 rollup int, d rollup int, e int, f rollup int );
insert into ts3 values ('1', '', '100', 'k3k', 'k4k', 'k5k',  '200', '300', '400', '456', '222', '333', '321' );
insert into ts3 values ('2', '', '100', 'k3k', 'k4k', 'k5k',  '200', '300', '400', '456', '222', '333', '321' );
insert into ts3 values ('2', '', '100', 'k3k', 'k4k', 'k5k',  '200', '300', '400', '456', '222', '333', '321' );
insert into ts3 values ('3', '', '100', 'k3k', 'k4k', 'k5k',  '200', '300', '400', '456', '222', '333', '321' );
insert into ts3 values ('2', '', '101', 'k3k', 'k4k', 'k5k',  '202', '304', '400', '457', '223', '353', '421' );
insert into ts3 values ('2', '', '101', 'k3k', 'k4k', 'k5k',  '202', '304', '400', '457', '223', '353', '421' );

expect rows 6;
select * from ts3;

expect rows 80;
select * from ts3@1h;

drop table if exists ts4;
create table timeseries(1h:0h, 3M:2y ) ts4 (  key: k1 int, ts timestamp, value: v1 rollup int, v2 int );

drop index if exists ts4_idx1 on ts4;
create index ts4_idx1 on ts4(v1, ts);
desc ts4_idx1;

drop index if exists ts4_idx2 on ts4;
create index ts4_idx2 on ts4(ts, k1);

insert into ts4 ( k1, v1, v2 ) values ('1', '123', '321' );
insert into ts4 ( k1, v1, v2 ) values ('2', '123', '321' );
insert into ts4 ( k1, v1, v2 ) values ('3', '123', '321' );
insert into ts4 ( k1, v1, v2 ) values ('3', '123', '321' );
insert into ts4 values ('5', '', '123', '321' );

expect rows 5;
select * from ts4;

expect rows 5;
select * from ts4@1h;

alter table ts4 add tick(1d);
desc ts4;

alter table ts4 add tick(1D:10D);
desc ts4;
alter table ts4 drop tick(1D);
desc ts4;


expect rows 5;
select * from test.ts4.ts4_idx1;


expect rows 5;
select * from test.ts4.ts4_idx2;

alter table ts4 add tick(1q);
desc ts4;
expect rows 0;
select * from ts4@1q;

insert into ts4 values ('6', '', '123', '321' );
expect rows 2;
select * from ts4@1q;

alter table ts4 retention 0;
desc ts4;
alter table ts4 retention 12M;
desc ts4;
alter table ts4@3M retention 3y;
desc ts4@3M;

drop table if exists ts5;
create table timeseries(1d) ts5 (  key: k1 int, ts timestamp, value: v1 rollup int, v2 int );
insert into ts5 values ('5', '', '100', '200' );
insert into ts5 values ('5', '', '100', '200' );
insert into ts5 values ('5', '', '100', '200' );
insert into ts5 values ('5', '', '100', '200' );
insert into ts5 values ('6', '', '100', '200' );
insert into ts5 values ('6', '', '100', '200' );

expect rows 6;
select * from ts5;

expect rows 3;
select * from ts5@1d;

drop table if exists ts5002;
create table timeseries(3d) ts5002 (  key: k1 int, ts timestamp, value: v1 rollup int, v2 int );
insert into ts5002 values ('5', '', '100', '200' );
insert into ts5002 values ('5', '', '100', '200' );
insert into ts5002 values ('6', '', '100', '200' );
insert into ts5002 values ('6', '', '100', '200' );
insert into ts5002 values ('7', '', '100', '200' );

expect rows 5;
select * from ts5002;

expect rows 4;
select * from ts5002@3d;

drop table if exists ts6;
create table timeseries(1w) ts6 ( key: k1 int, ts timestamp, value: v1 rollup int, v2 int );
insert into ts6 values ('5', '', '100', '200' );
insert into ts6 values ('5', '', '100', '200' );
insert into ts6 values ('5', '', '100', '200' );
insert into ts6 values ('5', '', '100', '200' );
insert into ts6 values ('5', '', '100', '200' );

expect rows 5;
select * from ts6;

expect rows 2;
select * from ts6@1w;

drop table if exists ts7;
create table timeseries(1month) ts7 ( key: k1 int, ts timestamp, value: v1 rollup int, v2 int );
insert into ts7 values ('5', '', '100', '200' );
insert into ts7 values ('5', '', '100', '200' );

expect rows 2;
select * from ts7;

expect rows 2;
select * from ts7@1M;

drop table if exists ts8;
create table timeseries(1year) ts8 ( key: k1 int, ts timestamp, value: v1 rollup int, v2 int );
insert into ts8 values ('5', '', '100', '200' );
insert into ts8 values ('5', '', '100', '200' );

expect rows 2;
select * from ts8;

expect rows 2;
select * from ts8@1y;

drop table if exists ts9;
create table timeseries(1decade) ts9 (  key: k1 int, ts timestamp, value: v1 rollup int, v2 int );
insert into ts9 values ('5', '', '100', '200' );
insert into ts9 values ('5', '', '100', '200' );

expect rows 2;
select * from ts9;

expect rows 2;
select * from ts9@1D;

drop table if exists ts10;
create table timeseries(15s:60s|1h) ts10 ( key: ts timestamp, a int, value: b int default '1000', c rollup int default '234' );
insert into ts10 ( a )values ( 100 );
insert into ts10 ( a )values ( 200 );
insert into ts10 ( a )values ( 300 );
insert into ts10 ( a )values ( 400 );
insert into ts10 ( a )values ( 600 );
insert into ts10 ( a )values ( 700 );

# bug b and c also are 800
insert into ts10 values ( '', 800  );

sleep 18;
insert into ts10 values ( '', 900, 111, 222  );

expect rows 8;
select * from ts10;

expect rows 10;
select * from ts10@15s;

## todo fix bug: insert into ts10 values ( '', 800  ); why b = c = 800? not using their default values?


drop table if exists tss1001;
create table tss1001 ( key: ts timestamp, a int, value: b int default '1000' );
insert into tss1001 ( a )values ( 100 );

expect rows 1;
select * from tss1001;

drop table if exists tss1;
create table tss1 ( key: a int, value: b timestamp );
insert into tss1 values ( 100 );

expect rows 1;
select * from tss1;

drop table if exists tss2;
create table tss2 ( key: a int, value: b timestampsec );
insert into tss2 values ( 100 );
insert into tss2 values ( 200 );

expect rows 2;
select * from tss2;

drop table if exists tss3;
create table tss3 ( key: a int, value: b timestampnano );
insert into tss3 values ( 100 );
insert into tss3 values ( 200 );

expect rows 2;
select * from tss3;

drop table if exists tss4;
create table tss4 ( key: a int, value: b timestampmill );
insert into tss4 values ( 100 );
insert into tss4 values ( 200 );
insert into tss4 values ( 300 );

expect rows 3;
select * from tss4;

drop table if exists tspace1;
create table timeseries(5m) tspace1 (  key: k1 int, ts timestamp, loc point, k2 int default '23', value: v1 rollup int, v2 int, v3 rollup int );
drop index if exists tspace1idx1 on tspace1;
create index tspace1idx1 on tspace1(v1, k1);

drop index if exists tspace1idx2 on tspace1;
create index tspace1idx2 on tspace1(v3, v1, k1);

drop index if exists tspace1idx3 on tspace1;
create index tspace1idx3 on tspace1(v3, k1, v1);


insert into tspace1 (k1, loc, v2 ) values ('10', point(2 3), '243' );
insert into tspace1 (k1, loc, v2, v1, v3 ) values ('10', point(2 3), '243', '1222', '3456' );
insert into tspace1 (k1, loc, v2, v1, v3 ) values ('11', point(4 5), '643', '2222', '4456' );
insert into tspace1 (k1, loc, v2, v1, v3 ) values ('12', point(4 5), '643', '2222', '4456' );

expect rows 4;
select * from tspace1;

expect rows 58;
select * from tspace1@5m;

expect rows 12;
select * from tspace1@5m where nearby(loc, point(34 12), 100  ) and k1=12;


expect rows 3;
select * from test.tspace1.tspace1idx1;

expect rows 3;
select * from test.tspace1.tspace1idx1@5m;

expect rows 3;
select * from test.tspace1.tspace1idx2;

expect rows 3;
select * from test.tspace1.tspace1idx3;

drop table if exists tspace2;
create table timeseries(5m) tspace2 (  key: k1 int, ts timestamp, loc circle, k2 int default '23', value: v1 rollup int, v2 int, v3 rollup int );
drop index if exists tspace2idx1 on tspace2;
create index tspace2idx1 on tspace2(v1, k1);

drop index if exists tspace2idx2 on tspace2;
create index tspace2idx2 on tspace2(v3, v1, k1);

drop index if exists tspace2idx3 on tspace2;
create index tspace2idx3 on tspace2(v3, k1, v1);

insert into tspace2 (k1, loc, v2, v1, v3 ) values ('10', circle(2 3 30), '243', '1292', '3456' );
insert into tspace2 (k1, loc, v2, v1, v3 ) values ('11', circle(4 5 50), '643', '2262', '4456' );
insert into tspace2 (k1, loc, v2, v1, v3 ) values ('12', circle(4 5 45), '645', '2422', '4056' );

expect rows 3;
select * from tspace2;

expect rows 24;
select * from tspace2@5m where nearby(loc, point(34 12), 100  ) and k1=12;

expect rows 4;
select * from tspace2@5m where nearby(loc, point(34 30), 40  ) and k1=12;


expect rows 3;
select * from test.tspace2.tspace2idx1;

expect rows 3;
select * from test.tspace2.tspace2idx2;

expect rows 3;
select * from test.tspace2.tspace2idx3;

expect rows 1;
select * from test.tspace2.tspace2idx3 where v3=4456;

drop table if exists sensorstat;
create table timeseries(5m:1d,1h:48h,1d:3M,1M:20y|5y)
sensorstat (key: sensorID char(16), ts timestamp,
            value: temperature rollup float,
                   pressure rollup float,
                   windspeed rollup float,
                   rpm  rollup float,
                   fuel rollup float,
                   model char(16),
                   type  char(16)
);
insert into sensorstat (sensorid, temperature, pressure, windspeed, rpm, fuel, model, type ) values ( 'drone1-sid1', '20.0', '35.5', '30.2', '1300', '1.3', 'AA212', 'DH' );

insert into sensorstat (sensorid, temperature, pressure, windspeed, rpm, fuel, model, type ) values ( 'drone1-sid1', '20.5', '35.8', '30.7', '1320', '1.5', 'AA212', 'DH' );
insert into sensorstat (sensorid, temperature, pressure, windspeed, rpm, fuel, model, type ) values ( 'drone1-sid2', '21.0', '35.7', '30.8', '1304', '1.2', 'AA213', 'DH' );
insert into sensorstat (sensorid, temperature, pressure, windspeed, rpm, fuel, model, type ) values ( 'drone2-sid1', '22.0', '36.4', '30.3', '1404', '2.2', 'AB213', 'DF' );

expect rows 4;
select * from sensorstat;

drop table if exists delivery;
create table timeseries(1M:1y,1y)
    delivery (key: ts timestamp, courier char(32), customer char(32),
              value: meals rollup bigint, addr char(128) );
expect words "delivery ts courier customer";
desc delivery;

drop index if exists delivery_index_courier on delivery;
create index delivery_index_courier on delivery(courier, customer, meals );
expect words "delivery_index_courier courier customer meals";
desc delivery_index_courier;

drop index if exists delivery_index2_courier on delivery@1M;
create index delivery_index2_courier on delivery@1M(courier, customer, meals::min, meals::max, meals::sum );
expect words "delivery_index2_courier customer meals";
desc delivery_index2_courier;

insert into delivery ( courier, customer, meals, addr ) values ( 'QDEX', 'JohnDoe', '3', '110 A Street, CA 90222' );
insert into delivery ( courier, customer, meals, addr ) values ( 'QDEX', 'JaneDoe', '5', '110 B Street, CA 90001' );
insert into delivery ( courier, customer, meals, addr ) values ( 'QSEND', 'MaryAnn', '3', '100 C Street, CA 92220' );
insert into delivery ( courier, customer, meals, addr ) values ( 'QSEND', 'PaulD', '12', '550 Ivy Road, CA 90221' );

expect rows 4;
select * from delivery;

expect rows 11;
select * from delivery@1M;

expect rows 11;
select * from delivery@1y;


expect rows 4;
select * from test.delivery.delivery_index_courier;

expect rows 4;
select * from test.delivery.delivery_index_courier@1M;

expect rows 4;
select * from test.delivery.delivery_index_courier@1M where courier='*' and customer='JohnDoe';

expect rows 4;
select * from test.delivery.delivery_index_courier@1y;


expect rows 16;
select * from test.delivery.delivery_index2_courier;

expect rows 1;
select * from test.delivery.delivery_index2_courier where courier='*' and customer='PaulD';


drop table if exist t;
create table t (key: a int, value: b timestamp);
insert into t values ( 100 );
select * from t;
select date(b) from t;

drop table if exist tn;
create table tn (key: a int, value: b timestampnano );
insert into tn values ( 101 );
select * from tn;
select date(b) from tn;

drop table if exist tm;
create table tm (key: a int, value: b timestampmill );
insert into tm values ( 102 );
select * from tm;
select date(b) from tm;

drop table if exist ts;
create table ts (key: a int, value: b timestampsec );
insert into ts values ( 103 );
select * from ts;
select date(b) from ts;

drop table if exist tm2;
create table tm2 (key: a int, value: b time );
insert into tm2  values ( 103, '21:23:24.123456' );
select * from tm2;
expect words "21";
select hour(b) from tm2;

drop table if exist tn2;
create table tn2 (key: a int, value: b timenano );
insert into tn2  values ( 103, '20:20:25.1234567891' );
select * from tn2;
expect words "20";
select hour(b) from tn2;


drop table if exist tr;
create table tr (key: a int, value: b range(datetime) );
insert into tr  values ( 103, range('2012-12-10 20:20:25.123456', '2022-12-10 20:29:25.123456') );

expect rows 1;
select * from tr;

expect rows 1;
select * from tr where within(b, range('2010-12-10 20:20:25.123456', '2024-12-10 20:29:25.123456') );

expect rows 1;
select * from tr where equal(b, range('2012-12-10 20:20:25.123456', '2022-12-10 20:29:25.123456') );

expect rows 0;
select * from tr where equal(b, range('2011-12-10 20:20:25.123456', '2022-12-10 20:29:25.123456') );

expect rows 1;
select * from tr where within(b, range('2012-12-10 20:20:25.123455', '2022-12-10 20:29:25.123457') );

expect rows 0;
select * from tr where within(b, range('2012-12-10 20:20:25.123456', '2022-12-10 20:29:25.123456') );


expect rows 1;
select * from tr where intersect(b, range('2012-12-10 20:20:24.123455', '2022-12-10 20:29:25.123457') );

expect rows 1;
select * from tr where intersect(b, range('2012-12-10T20:20:24.123455', '2022-12-10T20:29:25.123457') );

expect rows 0;
select * from tr where intersect(b, range('2023-12-10 20:20:24.123455', '2025-12-10 20:29:25.123457') );

expect rows 1;
select * from tr where cover(b, '2021-12-10 20:20:24.123455');

expect rows 1;
select * from tr where contain(b, '2021-12-10 20:20:24.123455');

expect rows 1;
select * from tr where within( '2021-12-10 20:20:24.123455', b);

expect rows 1;
select * from tr where within( '2021-12-10T20:20:24.123455', b);

expect rows 0;
select * from tr where within( '2028-12-10 20:20:24.123455', b);



quit;
