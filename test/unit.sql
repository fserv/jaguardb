
################## Usage expect command for test cases  ##############
# expect okmsg "test.lstr has 10 rows";
# select count(*) from lstr;

# expect errmsg "test.fg not found [30309]";
# select f from fg;

# expect rows 3;
# select * from lstr;

# expect words "k1 k4 col3 col5"
# desc t5;

# expect wordsize 100;
# select e from lstr where k='9';

# expect errors "E12338 Error rename";
# rename table unittest_old to unittest_new;

# expect value  COLNAME  VALUE
# expect value xm 303;
# select xm from t123 where ...;
#
# expect string COLNAME "STRING";
# expect string v "2018-01-01";
# select v from t123 where ...;

# rmfile output/out22.jpg;
# rmfile output/out23.tiff;
# expect file <file1> <file2> ...
# expect file output/out22.jpg output/out23.tiff;
# getfile jpg into 'output/out22.jpg', tiff into output/out23.tiff from media2 where uid='a' and pt:x=10 and pt:y=20 and name='jy';

######################################################################

drop table if exists df1;
create table df1 ( key: a char(1), value: b double, c char(2));
insert into df1 values ('a', 88888888.123456, 'cccc');
select * from df1;

drop table if exists df2;
create table df2 ( key: a int, value: b double, c char(2));
insert into df2 values (100, 88888888.123456, 'cccc');
select * from df2;

drop table if exists df3;
create table df3 ( key: a int, b double, value: c char(2));
insert into df3 values (100, 88888888.123456, 'cccc');
select * from df3;

drop table if exists int1k;
create table if not exists int1k ( key: k1 int, k2 double, value: addr char(32) );
load int1k.txt into int1k;
expect rows 1;
select * from int1k;


### test inserts and default values
drop table if exists tf1;
create table tf1 ( key: a int, b int, value: c int, d int default '444' );
expect words "tf1 ( key: a int, b int, value: c int, d  default 444";
desc tf1;

insert into tf1 values (137, 328 );

expect value b 328;
select b from tf1 where a=137;

expect value d 444;
select d from tf1 where a=137;

insert into tf1 values (139, 329, 578 );
insert into tf1 values (149, 359, 508, 938 );
insert into tf1 (a, b, d) values (287, 2387, 888);

expect value d 938;
select d from tf1 where a=149;

expect rows 4;
select * from tf1;

### test inserts and uuid
drop table if exists tf2;
create table tf2 ( key: a uuid, value: b int, c int, d int default '444' );
insert into tf2 values ( 137, 328 );

expect value c 328;
select c from tf2 where b=137;

expect value d 444;
select d from tf2 where b=137;

insert into tf2 values (139, 329, 578 );

expect value d 578;
select d from tf2 where b=139;

insert into tf2 (b,c,d) values (189, 399, 518 );

expect value d 518;
select d from tf2 where b=189;

insert into tf2 values (149, 359, 938 );
insert into tf2 ( b, d) values (2387, 888);
insert into tf2 ( a, b, d) values ('', 2487, 888);
insert into tf2 ( a, b, c) values ('', 2489, 1888);

expect value c 1888;
select c from tf2 where b='2489';

insert into tf2 ( b, c) values ( 2483, 1888);

expect value c 1888;
select c from tf2 where b='2483';

### sum and aggregations
drop table if exists agg;
create table agg ( key: a int, value: b double );
insert into agg values ( 137, 10.0 );

expect value sa 137;
select sum(a) sa from agg;

### two uuid columns
drop table if exists tf22;
create table tf22 ( key: a uuid, value: b int, c int, d int default '444', e uuid );

expect words "tf22 key uuid a b c DEFAULT e uuid";
desc tf22;

insert into tf22 values ( 137, 328 );
select * from tf22;

expect value c 328;
select c from tf22 where b=137;

insert into tf22 values (139, 329, 578 );
insert into tf22 (b,c,d) values (189, 399, 518 );

insert into tf22 values (149, 359, 938 );

expect value d 938;
select d from tf22 where b=149;

expect value c 359;
select c from tf22 where b=149;

insert into tf22 ( b, d) values (2387, 888);

expect value d 888;
select d from tf22 where b='2387';

insert into tf22 ( a, b, d) values ('', 2487, 888);
expect value d 888;
select d from tf22 where b='2487';

insert into tf22 ( a, b, c) values ('', 2489, 1888);

expect value d 444;
select d from tf22 where b='2489';

insert into tf22 ( b, c) values ( 2483, 1888);
expect wordsize 32;
select e from tf22 where b='2483';


### test inserts and timestmp
drop table if exists tf3;
create table tf3 ( key: a timestamp, b int, value: c int, d int default '444' );
insert into tf3 values ( '', 137, 328 );

expect value c 328;
select c from tf3 where b=137;

insert into tf3 values ('', 139, 329, 578 );
expect value c 329;
select c from tf3 where b=139;

expect value d 578;
select d from tf3 where b=139;

insert into tf3 (b,c,d) values ( 189, 399, 518 );
expect value d 518;
select d from tf3 where b=189;

insert into tf3 values ('', 149, 359, 938 );
insert into tf3 ( b, d) values (2387, 888);

insert into tf3 ( a, b, d) values ('', 2487, 888);

insert into tf3 ( a, b, c) values ('', 2489, 1888);
expect value c 1888;
select c from tf3 where b='2489';

insert into tf3 ( b, c) values ( 2483, 1888);
insert into tf3 ( a, b, c, d) values ('', 2447, 777, 888);

insert into tf3 ( a, b, c, d) values ('', 2547, 777 );

expect value d 444;
select d from tf3 where b='2547';

insert into tf3 ( b, a, d, c ) values (22234, '', 2597, 677 );

expect value d 2597;
select d from tf3 where b='22234';

expect value c 677;
select c from tf3 where b='22234';

insert into tf3 ( b, a, c ) values (22235, '', 2597 );

expect value c 2597;
select c from tf3 where b='22235';

expect value d 444;
select d from tf3 where b='22235';


drop table if exists jbench;
create table if not exists jbench (key: uid char(16), value: addr char(32) );
truncate table jbench;
insert into jbench values ( '李世民', '胜利街12号，北京' );
insert into jbench values ( '张明', '枫涟路123号，上海');

expect rows 2;
select * from jbench ;

expect rows 1;
select * from jbench where uid='李世民';

load jbench_cn.txt  into jbench;
expect rows 15;
select * from jbench;

expect rows 1;
select  substr(addr, 0, 3, UTF8 ) from jbench where uid='李世民';

insert into jbench values ( 0001, kkkkkkddddd );
insert into jbench values ( 0002, ppspspsps );
insert into jbench values ( zzzzzz, ppspspsps );

expect rows 18;
select * from jbench;

expect rows 1;
select * from jbench where uid='李世民';

expect rows 4;
select length(uid), length(addr) from jbench limit 4;

drop table if exists unittest_old;
create table unittest_old ( key: uid char(32), value: v1 char(16), v2 char(16)), v3 char(16) );


drop table if exists unittest1;
create table unittest1 ( key: uid char(32), value: v1 char(16), v2 char(16)), v3 char(16) );

drop index if exists unittest1_idx1 on unittest1;
create index unittest1_idx1 on unittest1( v2 );

insert into unittest1 ( uid, v1, v2, v3 ) values ( 'kkk1', vvvv1, vvvvvv2, vvvvv3 );
insert into unittest1 ( uid, v1, v2, v3 ) values ( 'kkk2', vvbvv1, vkkvvvvv2, vbvnvvvv3 );
insert into unittest1 ( uid, v1, v2, v3 ) values ( 'kkk3', some, vkkvvvvv2, vbvnvvvv3 );
insert into unittest1 ( uid, v1, v2, v3 ) values ( 'kkk4', some, vkkvvvvv2, vbvnvvvv3 );
insert into unittest1  values ( 'kkk5', somev, vkkv6v, vbvnv8vv );

expect rows 5;
select * from unittest1;

update unittest1 set v1='newv1' where uid='kkk1';

expect rows 1;
select * from unittest1 where uid='kkk1';

expect words "newv1";
select v1 from unittest1 where uid='kkk1';

delete from unittest1 where uid='kkk1';

expect rows 0;
select * from unittest1 where uid='kkk1';

load 100H.txt into unittest1 ;

expect rows 309;
select * from unittest1;

expect okmsg "test.unittest1 has 309 rows";
select count(*) from unittest1;

expect rows 1;
select * from unittest1 where uid='kkk2';


expect words "vkkvvvvv2";
select v2 from  unittest1 where  uid='kkk2';

expect words "vbvnvvvv3";
select v3 from  unittest1 where  uid='kkk2';

expect rows 0;
select * from unittest1 where v3='vvvvv3';

expect rows 3;
select * from unittest1 where v3='vbvnvvvv3';



expect words "unittest1_idx1 v2 char";
desc unittest1_idx1;

expect rows 10;
select * from test.unittest1.unittest1_idx1 limit 10;

expect rows 3;
select * from test.unittest1.unittest1_idx1 limit 10,3;

expect rows 1;
select * from test.unittest1.unittest1_idx1 where v2='vkkvvvvv2';

expect rows 5;
select * from test.unittest1.unittest1_idx1 limit 5;

expect words "unittest1 key uid v2 char";
desc unittest1;

expect words "system test";
show databases;

expect words "unittest1 jbench tf1 tf2 tf3";
show tables;

expect words "unittest1_idx1";
show indexes;

expect words "unittest1_idx1";
show indexes from unittest1;

expect rows 0;
select uid, v3 from unittest1 where uid='ddd' and v2='fdfdfdf';

expect rows 1;
select uid, v3 from unittest1 where uid='kkk2' and v2='vkkvvvvv2';

expect rows 29;
select uid, v3 from unittest1 where uid between 'sss' and 'zzz';

expect rows 0;
select uid, v3 from unittest1 where uid between 'sss' and 'zzz' and v1='fff';

expect rows 1;
select * from unittest1 where uid='Cpple01234567890Apple01234567890';

expect words "green21234567890";
select v1 from unittest1 where uid='Cpple01234567890Apple01234567890';

expect words "sweetsweet212345";
select v2 from unittest1 where uid='Cpple01234567890Apple01234567890';

expect words "Apple21234567890";
select v3 from unittest1 where uid='Cpple01234567890Apple01234567890';

update unittest1 set v3='new value3' where uid='Cpple01234567890Apple01234567890';
expect words "new value3";
select v3 from unittest1 where uid='Cpple01234567890Apple01234567890';

update unittest1 set v2='new value2', v1='fffff1' where uid='Cpple01234567890Apple01234567890';
expect words "new value2";
select v2 from unittest1 where uid='Cpple01234567890Apple01234567890';

expect words "fffff1";
select v1 from unittest1 where uid='Cpple01234567890Apple01234567890';

expect rows 1;
select * from unittest1 where uid='Cpple01234567890Apple01234567890';

expect rows 1;
select * from unittest1 where uid='kkk3';

delete from unittest1 where uid='kkk3';
expect rows 0;
select * from unittest1 where uid='kkk3';


expect rows 1;
select * from unittest1 where v1='some';

delete from unittest1 where v1='some';

expect rows 0;
select * from unittest1 where v1='some';

expect errors "E12338 Error";
rename table unittest_old to unittest_new;

expect words "jbench tf1 tf2 tf22 tf3 unittest1";
show tables;

drop table if exists unittest_old_2;
create table unittest_old_2 ( key: uid char(32), ssn char(16), value: v1 char(16), v2 char(16)), v3 char(16) );
expect words "unittest_old_2 uid ssn v1 v2 v3 char";
desc unittest_old_2;


alter table unittest_old_2 rename ssn to socseckey;
expect words "unittest_old_2 uid socseckey v1 v2 v3 char";
desc unittest_old_2;

insert into unittest_old_2 ( uid, socseckey, v1, v2, v3 ) values ( 'kkk1', 223345, vvvv1, vvvvvv2, vvvvv3 );
insert into unittest_old_2 ( uid, socseckey, v1, v2, v3 ) values ( 'kkk2', 223345, vvvv1, vvvvvv2, vvvvv3 );
insert into unittest_old_2 ( uid, socseckey, v1, v2, v3 ) values ( 'kkk3', 223345, vvvv1, vvvvvv2, vvvvv3 );
insert into unittest_old_2 ( uid, socseckey, v1, v2, v3 ) values ( 'kkk4', 223345, vvvv1, vvvvvv2, vvvvv3 );
insert into unittest_old_2 ( uid, socseckey, v1, v2, v3 ) values ( 'kkk5', 223345, vvvv1, vvvvvv2, vvvvv3 );
insert into unittest_old_2 ( uid, socseckey, v1, v2, v3 ) values ( 'kkk6', 223345, vvvv1, vvvvvv2, vvvvv3 );

expect rows 6;
select * from unittest_old_2;

truncate table unittest_old_2;
expect rows 0;
select * from unittest_old_2;

expect words "jbench tf1 tf2 tf22 tf3 unittest1 unittest_old_2";
show tables;

insert into unittest_old_2 ( uid, socseckey, v1, v2, v3 ) values ( 'kkk1', 223345, vvvv1, vvvvvv2, vvvvv3 );
insert into unittest_old_2 ( uid, socseckey, v1, v2, v3 ) values ( 'kkk2', 223345, vvvv1, vvvvvv2, vvvvv3 );
insert into unittest_old_2 ( uid, socseckey, v1, v2, v3 ) values ( 'kkk3', 223345, vvvv1, vvvvvv2, vvvvv3 );
insert into unittest_old_2 ( uid, socseckey, v1, v2, v3 ) values ( 'kkk4', 223345, vvvv1, vvvvvv2, vvvvv3 );
insert into unittest_old_2 ( uid, socseckey, v1, v2, v3 ) values ( 'kkk5', 223345, vvvv1, vvvvvv2, vvvvv3 );
insert into unittest_old_2 ( uid, socseckey, v1, v2, v3 ) values ( 'kkk6', 223345, vvvv1, vvvvvv2, vvvvv3 );

expect rows 6;
select * from unittest_old_2;

delete from unittest_old_2;
expect rows 0;
select * from unittest_old_2;


insert into unittest_old_2 ( uid, socseckey, v1, v2, v3 ) values ( 'kkk1', 223345, vvvv1, vvvvvv2, vvvvv3 );
insert into unittest_old_2 ( uid, socseckey, v1, v2, v3 ) values ( 'kkk2', 223345, vvvv1, vvvvvv2, vvvvv3 );
insert into unittest_old_2 ( uid, socseckey, v1, v2, v3 ) values ( 'kkk3', 223345, vvvv1, vvvvvv2, vvvvv3 );
insert into unittest_old_2 ( uid, socseckey, v1, v2, v3 ) values ( 'kkk4', 223345, vvvv1, vvvvvv2, vvvvv3 );
insert into unittest_old_2 ( uid, socseckey, v1, v2, v3 ) values ( 'kkk5', 223345, vvvv1, vvvvvv2, vvvvv3 );
insert into unittest_old_2 ( uid, socseckey, v1, v2, v3 ) values ( 'kkk6', 223345, vvvv1, vvvvvv2, vvvvv3 );
expect rows 3;
select * from unittest_old_2 limit 3;

drop table if exists unittest_old_2;

expect words "jbench tf1 tf2 tf22 tf3 unittest1";
show tables;


create table unittest2 ( key: uid char(32), value: v1 char(16), v2 char(16)), v3 char(16) );
expect words "jbench tf1 tf2 tf22 tf3 unittest1 unittest2";
show tables;

expect words "unittest2 uid v1 v2 v3";
desc unittest2;

load 100H2.txt into unittest2 ;
expect rows 405;
select * from unittest2;

load 100H22.txt into unittest2 ;
expect rows 505;
select * from unittest2;

create table unittest3 ( key: uid char(32), value: v1 char(16), v2 char(16)), v3 char(16) );
expect words "jbench tf1 tf2 tf22 tf3 unittest1 unittest2 unittest3";
show tables;
expect words "unittest3 uid v1 v2 v3";
desc unittest3;

create index unittest3_idx on unittest3( v2, v1 );
expect words "unittest3_idx";
show indexes in unittest3;

load 100H3.txt into unittest3 ;
expect okmsg "test.unittest3 has 505 rows";
select count(*) from unittest3;

expect words "unittest3 505 rows";
select count(*) from unittest3;

expect words "unittest3_idx 505 rows";
select count(*) from test.unittest3.unittest3_idx;

#drop table if exists unittest1;
drop table if exists unittest2;
drop table if exists unittest3;

expect nowords "unittest2 unittest3";
show tables;

drop table if exists int10k;
create table if not exists int10k ( key: k1 double, k2 double, value: addr char(32) );
expect words "int10k k1 k2 addr";
desc int10k;

load int10k.txt into int10k;
expect rows 105;
select * from int10k;

expect rows 2;
select k1+k2, sin(k1-k2*2/3.0), cos(k1*k2/(k1-k2)) from int10k limit 2;

expect rows 2;
select (k1+k2) as k1pk2, sin(k1-k2*2/3.0) sinv, cos(k1*k2/(k1-k2)) cosv from int10k limit 2;

expect value sinv -0.28159;
select sin(k1-k2*2/3.0) sinv from int10k where k1=2130324323;

expect value cosv -0.373924;
select cos(k1*k2/(k1-k2)) cosv from int10k where k1=2130324323;

#expect value avg 2090284171.4002;
select avg(k1+k2) avg from int10k;

expect value sum 115853256056;
select sum(k1) sum from int10k;

#expect value v 645024489.340875;
select stddev(k2) v from int10k;

expect value k1 927374;
select k1 from int10k limit 1 order by k1;

expect value v 927374;
select min(k1) v from int10k;

expect value v 2139984211;
select max(k1) v from int10k;


select abs(k1+k2), acos(k2), asin(k1), ceil(k1), cot(k2), floor(k2), log2(k2), log10(k1) from int10k limit 2;

expect rows 2;
select log(k1+k2), ln(k2), pow(k1, 2), mod(k1, 3), sqrt(k1), tan(k2) from int10k limit 2;

expect rows 2;
select radians(k1+k2), degrees(k2), radians(degrees(k1))  from int10k limit 2;

expect rows 1;
select trim(addr), from int10k limit 1;

expect rows 1;
select upper(addr) from int10k limit 1;

expect wordsize 3;
select substr(addr, 1, 3, UTF8 ) from int10k limit 1;

expect wordsize 4;
select  substr(addr, 1, 4, UTF8 ) from int10k limit 1;

expect wordsize 4;
select  substr(addr, 1, 4, 'UTF8' ) from int10k limit 1;

expect wordsize 4;
select  substr(addr, 1, 4, 'UTF-8' ) from int10k limit 1;

expect wordsize 4;
select  substr(addr, 1, 4, 'GBK' ) from int10k limit 1;

expect wordsize 4;
select  substr(addr, 1, 4, 'GB18030' ) from int10k limit 1;

expect wordsize 4;
select  substr(addr, 1, 4, GB18030 ) from int10k limit 1;

drop table if exists int10k_2;
create table if not exists int10k_2 ( key: k1 int, k2 float(16.3), value: addr char(32) );
expect words "int10k_2 k1 k2 addr float char int";
desc int10k_2;

load int10k_2.txt into int10k_2 ;
expect words "105 rows";
select count(*) from int10k_2;

expect rows 105;
select * from int10k_2;

expect rows 39;
select k1+k2 k1pk2, sin(k1-k2*2/3.0) sinv, cos(k1*k2/(k1-k2)) from int10k_2 limit 39;

expect rows 37;
select k1, sum(k2)  from int10k_2 group by k1 limit 37;

expect rows 105;
select k1, sum(k2)  from int10k_2 group by k1;

expect rows 15;
select k1, sum(k2), count(1)  from int10k_2 group by k1 limit 2,15;


drop table if exists service;
create table service ( key: uid int, daytime datetime, value: phone char(10), reason char(16) );
expect words "service uid daytime datetime phone reason char";
desc service;

load service.txt into service ;
expect words "1000 rows";
select count(*) from service;

expect rows 1;
select daytime from service limit 1;
# 2014-02-15 11:40:12.000000

expect value v 14;
select dayofmonth( daytime) v from service where uid=99 and phone=1004832583;

expect value v 5;
select  dayofweek( daytime ) v from service where uid=99 and phone=1004832583;

expect value v 45;
select  dayofyear(daytime) v from service limit where uid=99 and phone=1004832583;

expect rows 1;
select curdate(), curtime(), now() from service limit 1;

drop table if exists callinfo;
create table callinfo ( key: lNumberKey  int, value:  tApplyTime date, tExpirtTime date, szCallNumber char(132),iHomeArea int, szStatus char(12), lMainAccountKey int, lAccT_balance_id int, lAcctid int, szSecond_ower_type  char(4), iBalance_Type_ID int, lAmount int, lInitialAmount int, lReserveAmount      int, lSettleAmount int, szOrigin_type char(4), lOriginID int, szInitialType char(4), lInitial_D int, content char(40), lReserve0 int, tReserve0 date, lReserve1 int, tReserve1 date, lReserve2 int, tReserve2 date, lReserve3 int, tReserve3 date, lReserve4 int, tReserve4 date );

expect words "callinfo lnumberkey tapplytime texpirttime szcallnumber, ihomearea , szstatus lmainaccountkey lacct_balance_id lacctid int, szsecond_ower_type ibalance_type_id lamount linitialamount lreserveamount lsettleamount szorigin_type loriginid szinitialtype linitial_d content lreserve0 treserve0 lreserve1 treserve1 lreserve2  treserve2  lreserve3 treserve3 lreserve4 treserve4"; 
desc callinfo;

create index idx_callinfo_szCallNumber on callinfo(szCallNumber);
expect words "idx_callinfo_szCallNumber szcallnumber";
desc idx_callinfo_szCallNumber;

load callinfo.txt into callinfo quote terminated by '\'';

expect words "1000 rows";
select count(*) from callinfo;

expect words "1000 rows";
select count(*) from test.callinfo.idx_callinfo_szCallNumber;

expect rows 3;
select * from test.callinfo.idx_callinfo_szCallNumber limit 3;


drop table if exists service2;
create table service2 ( key: uid uuid, value: phone char(10), reason char(16) );
expect words "service2 uid uuid phone reason char";
desc service2;

insert into service2 ( reason, phone ) values ( 'sick', '4082230989' );
insert into service2 ( reason, phone ) values ( 'holiday', '4082230989' );
insert into service2  values ( '13482829', 'us-holiday' );

expect rows 3;
select * from service2;

drop table if exists starhost;
create table starhost ( key: k1 char(16), k2 char(16),k3 char(16), value: v1 char(16), v2 char(16), v3 char(16) );
expect words "starhost k1 k2 k3 v1 v2 v3 char";
desc starhost;

load starhost.txt into starhost ;
expect words "100 rows";
select count(*) from starhost;
expect rows 100;
select * from starhost;

expect rows 20;
select k1, k2 from starhost where k1 >= 'awweee' and k1 < 'p9292';

expect rows 5;
select k1, k2 from starhost where k1>='awweee' and k1<'p9292' limit 5;

expect rows 3;
select k1, k2 from starhost where k1>='awweee' and k1<'p9292' and k2 >= 'mm' and k2 <='x999';

expect rows 2;
select k1, k2 from starhost where k1>='awweee' and k1<'p9292' and k2>='mm' and k2 <='x999' limit 2;


expect value v 1083227821;
select avg(lnumberkey) v from callinfo;

expect string v "2016-01-01";
select  min(tapplytime) v from callinfo; 

expect string v "2018-01-01";
select max(tapplytime) v from callinfo; 


### sales table
drop table if exists sales;
create table sales ( key: uid int, daytime datetime, value: amt float(3.1), unit float(3.1), utype char(1) );
expect words "sales uid daytime datetime amt float unit utype char";
desc sales;

load sales.txt into sales ;
expect rows 5;
select * from sales;

insert into sales values (10, '2014-04-23 08:59:52.000000', 0.1, 0.1, 'C' );
insert into sales values (10, '2014-04-23 08:59:59.000000', 0.1, 0.1, 'C' );
insert into sales values (13579, '2014-04-29 08:59:59.000000', 0.1, 0.1, 'D' );
expect rows 8;
select * from sales;

expect rows 5;
select daytime, datediff(day, daytime, '2014-09-30 23:00:00.0001' ) daydiff from sales limit 5;

expect rows 1;
select daytime from sales limit 1;
# daytime=[2014-04-23 08:59:52.000000]

expect value v 154;
select datediff(day, daytime, '2014-09-30 23:00:00.0001' ) v from sales where uid=13579;

expect value v -2;
select daytime, datediff(day, '2014-09-30 23:00:00', '2014-09-28 l2:12:12' ) v from sales where uid=13579;

expect value v 1;
select daytime, datediff(day, '2014-09-29 l2:12:12', '2014-09-30 23:00:00' ) as v from sales where uid=13579;

expect value v 2;
select daytime, datediff(month, '2014-09-29 l2:12:12', '2014-11-30 23:00:00' ) as v from sales where uid=13579;

expect value v 2;
select daytime, datediff(month, '2022-10-12', '2022-12-14' ) v from sales where uid=13579;

expect value v -8;
select daytime, datediff(year, '2022-12-12', '2014-09-30' ) v from sales where uid=13579;

expect value v 53;
select daytime, datediff(second, '2022-12-12 00:00:00', '2022-12-12 00:00:53' ) v from sales where uid=13579;

expect value v 22;
select daytime, datediff(hour, '2022-12-12 01:00:00', '2022-12-12 23:00:00' ) v from sales where uid=13579;

expect value v -22;
select daytime, datediff(hour, '2022-12-12 23:00:00', '2022-12-12 01:00:00' ) v from sales where uid=13579;

expect value v 73;
select daytime, datediff(minute, '2022-12-12 12:30:21', '2022-12-12 13:43:21' ) v from sales where uid=13579;

# select constant
expect value v 73;
select datediff(minute, '2022-12-12 12:30:21', '2022-12-12 13:43:21' ) as v;

expect rows 1;
select daytime as dt6789 from sales limit 1;

expect rows 1;
select date(daytime) as dt0986 from sales limit 1;

expect value v 4;
select month(daytime) v from sales where uid=13579;

expect value v 2014;
select year(daytime) v from sales where uid=13579;

expect value v 8;
select hour(daytime) v from sales where uid=13579;

expect value v 12;
select hour("2011-02-09 12:13:08") v;

expect value v 59;
select minute(daytime) v from sales where uid=13579; 

expect value v 13;
select minute("2011-02-09 12:13:08") v;

expect value v 59;
select second(daytime) v from sales where uid=13579;

expect value v 58;
select second("2011-02-09 12:13:58") v;

expect value v 29;
select dayofmonth(daytime) v from sales where uid=13579;

expect value v 17;
select dayofmonth("2011-09-17") v;

expect value v 19;
select dayofmonth("2011-09-19 03:03:12") v;

expect value v 2;
select dayofweek(daytime) v from sales where uid=13579;

expect value v 4;
select dayofweek("2011-09-29 03:03:12") v;

select dayofweek("2011-09-29 03:03:12") v from system._SYS_;
select dayofweek("2011-09-29 03:03:12") v from _SYS_;

select daytime, dayofyear(daytime) from sales limit 1;
select curdate(), curtime(), now() from sales limit 1;

select curdate() curd from system._SYS_;
select curdate() cd, curtime() ct, now() nw from system._SYS_;
select curdate() cd, curtime() ct, now() nw from _SYS_;

!date;

expect rows 0;
select * from sales limit 10000000,3;

expect rows 4;
select uid, sum(amt)  from sales group by uid limit 4;



createuser test123:testtesttest123456789;
expect words "admin test123";
show users;

dropuser test123;
expect nowords "test123";
show users;

drop table if exists tms1;
create table tms1 ( key: ts timestamp, value: addr char(18) );
expect words "tms1 ts timestamp addr char";
desc tms1;

drop index if exists tms1_idx1  on tms1 ;
create index tms1_idx1 on tms1 ( addr );
expect words "tms1_idx1 addr char";
desc tms1_idx1;

insert into tms1 values ( '', '123 B St' );
insert into tms1 values ( '', '124 B St' );
insert into tms1 values ( '', '125 B St' );
insert into tms1 values ( '', '127 B St' );
insert into tms1 values ( '', '129 B St' );
insert into tms1 (addr) values ( '133 C St' );
insert into tms1 (addr) values ( '135 C St' );

expect rows 7;
select * from tms1;

expect rows 2;
select * from tms1 where addr in ( '123 B St', '125 B St' );


expect rows 7;
select * from test.tms1.tms1_idx1;

expect rows 7;
select * from test.tms1.tms1_idx1 group by addr;

expect rows 1;
select * from test.tms1.tms1_idx1 group by addr limit 1;

expect rows 3;
select * from test.tms1.tms1_idx1 group by addr limit 2,3;


### uuid as key
drop table if exists nokey;
create table nokey ( a int, b int, c real, d text );
expect words "nokey _id uuid a b c";
desc nokey;

insert into nokey values ( 11, 22, 12.4, 'hi there you' );
insert into nokey values ( 11, 22, 12.4, 'hi there' );
insert into nokey values ( 12, 22, 12.5, 'h there' );
insert into nokey values ( 18, 23, 12.5, 'h there' );
insert into nokey values ( 18, 24, 12.5, 'h there' );

expect rows 5;
select * from nokey;

expect rows 5;
select _id from nokey;

expect rows 3;
select a, sum(b) from nokey group by a;

expect rows 3;
select a, sum(b) from nokey group by a order by a desc;

expect rows 3;
select a, sum(b) sm from nokey group by a order by sm desc;

expect  value sm 113;
select sum(b) sm from nokey;

expect  value sm 44;
select sum(b) sm from nokey where a=11;

select sum(b) sm from nokey where a=11 group by a;

expect  value sm 47;
select sum(b) sm from nokey where a=18 group by a;

expect rows 3;
select b, sum(a) from nokey group by b;

expect rows 3;
select b, sum(a) sm from nokey group by b order by sm desc;

expect rows 3;
select b, sum(a) sm from nokey group by b order by sm asc;


expect errmsg "";
show databases;

expect errmsg "";
show tables;

expect errmsg "";
show currentdb;

expect words "TaskID ThreadID User Database StartTime Command";
show task;

expect words "Server  Version";
show server version;

expect words "Client";
show client version;

expect words "Servers Databases Tables Connections Selects Inserts Updates Deletes";
show status;


### join test cases
drop table if exists j0;
create table j0 ( key: k01 int, k02 char(3), value: v01 int, v02 char(3) );
expect words "k01 k02 v01 v02";
desc j0;

insert into j0 values ( '100', 'aaa', '100', 'bbb' );
insert into j0 values ( '100', 'ccc', '100', 'ddd' );
insert into j0 values ( '100', 'eee', '100', 'fff' );
insert into j0 values ( '100', 'ggg', '100', 'hhh' );
insert into j0 values ( '100', 'iii', '100', 'jjj' );
insert into j0 values ( '100', 'kkk', '100', 'lll' );
insert into j0 values ( '100', 'mmm', '100', 'nnn' );
insert into j0 values ( '100', 'ooo', '100', 'ppp' );
insert into j0 values ( '100', 'qqq', '100', 'rrr' );
insert into j0 values ( '100', 'ss9', '100', 'ttt' );

expect rows 10;
select * from j0 order by k01, v01, k02;

expect errmsg "Error order by";
select k02 from j0 order by k01, v01, k02;

expect errmsg "Error order by";
select k02, v01 from j0 order by k01, v01, k02;

expect words "ss9";
select k02, v01, k01 from j0 order by k01, v01, k02 limit 10,1;

drop table if exists def1;
create table def1 ( key: uid int, value: b int default '1', c varchar(32) default 'C' );
insert into def1 ( uid ) values ( 100 );
insert into def1 values ( 200 );
insert into def1 values ( 200, 1000 );
expect rows 2;
select * from def1;

drop table if exists def2;
create table def2 ( key: uid int, value: b int default '1', tm timestamp default current_timestamp on update current_timestamp );
insert into def2 ( uid ) values ( 100 );
insert into def2 values ( 200 );
insert into def2 values ( 300, 1000 );

expect rows 3;
select * from def2;

# todo
update def2 set b=909 where uid=100;
select * from def2;

update def2 set b=959 where uid=200;
select * from def2;

update def2 set b=989 where uid=300;
select * from def2;


#todo

drop table if exists inst1;
create table if not exists inst1 ( key: a char(32), value: b char(21) );
expect words "inst1 a b";
desc inst1;

expect rows 0;
select * from inst1;

drop table if exists inst2;
create table if not exists inst2 ( key: a char(32), value: b char(21), c char(21) );
expect words "inst2 a b c char";
desc inst2;

insert into inst2 values ( 'a1', 'bbb1', 'ffffff' );
insert into inst2 values ( 'a2', 'bbb2', 'ffffff' );
insert into inst2 values ( 'a3', 'bbb3', 'ffffff' );
insert into inst2 values ( 'a4', 'bbb4', 'ffffff' );
insert into inst2 values ( 'a5', 'bbb5', 'ffffff' );
insert into inst2 values ( 'a6', 'bbb6', 'ffffff' );
insert into inst2 values ( 'a7', 'bbb7', 'ffffff' );
insert into inst2 values ( 'a8', 'bbb8', 'ffffff' );
expect rows 8;
select * from inst2;

insert into inst1 (inst1.a, inst1.b) select inst2.a, inst2.b from inst2;
expect rows 8;
select * from inst1;

expect errors "Error";
createdb jdjdj-rirr;

drop table if exists ii;
create table ii ( key: a int, value: b int );
expect words "ii a b ";
desc ii;
insert into ii values ( -15, -150);
insert into ii values ( -25, -250);
insert into ii values ( -35, -350);
insert into ii values ( -45, -450);
insert into ii values ( -55, -550);
insert into ii values ( -65, -660);
insert into ii values ( -75, -750);
insert into ii values ( -85, -850);

expect rows 8;
select * from ii;

expect value b -250;
select b from ii where a=-25;

expect rows 0;
select * from ii where a >= -10;

expect rows 1;
select * from ii where a >= -20;

expect rows 4;
select * from ii where abs(a) >= 50;

expect rows 4;
select a+100 a100 from ii where abs(a) >= 50;

expect rows 2;
select * from ii where a >= -28;

expect rows 3;
select * from ii where a >= -40;

expect rows 3;
select * from ii where a < -55;

expect rows 4;
select * from ii where a <= -55;

expect rows 3;
select * from ii where a > -76 and a < -46;

expect rows 8;
select * from ii where a < 2;

expect rows 8;
select * from ii where b < 2;

expect rows 1;
select * from ii where b < -800;

expect rows 3;
select * from ii where b >= -350;

expect rows 3;
select * from ii where b >= -450 and b <= -250;

expect rows 2;
select * from ii where b > -450 and b <= -250;

insert into ii values ( 100, 1000);
insert into ii values ( 200, 2000);
insert into ii values ( 300, 3000);
insert into ii values ( 400, 4000);

expect rows 3;
select * from ii where a >= 100 and a <= 300;

expect rows 11;
select * from ii where a >= -300 and a <= 300;

expect rows 8;
select * from ii where a > -57 and a <= 300;

expect rows 4;
select * from ii where b > -57 and b <= 9000;

expect rows 8;
select * from ii where b > -500 and b <= 9000;

expect rows 8;
select * from ii where a > -55 and b <= 9000;

expect rows 7;
select * from ii where a > -55 and b <= 3000;

expect rows 6;
select * from ii where a > -55 and b < 3000;

expect rows 6;
select * from ii where b < 3000 and a > -55;


drop table if exists media;
create table media (key: uid int, value: jpg file, a char(23), tiff file );
insert into media values ( 100, 'req.jpg', 'aaaa', random_test.txt );
insert into media values ( 101, 'req.jpg', 'aaaa', callinfo.txt );
expect rows 2;
select * from media;

getfile jpg into 'output/out1.jpg' from media where uid=101;
! /bin/ls -l output/out1.jpg;
! date;

getfile jpg into 'output/out2.jpg' from media where uid=101;
! /bin/ls -l output/out2.jpg;
! date;

getfile jpg into 'output/out3.jpg', tiff into output/out3.tiff from media where uid=101;
! /bin/ls -l output/out3.tiff;
! date;

expect rows 1;
getfile jpg size, tiff time, tiff md5, jpg time from media where uid='100';

expect value jpg_size 51857;
getfile jpg size, tiff time, tiff md5, jpg time from media where uid=101;

expect errors "Error getfile";
getfile jpg size, tiff time, tiff md5, jpg time from media ;



drop table if exists media2;
create table media2 (key: uid char(1), pt point, name char(2),  value: jpg file, a char(23), tiff file );
insert into media2 values ( 'a', point(10 20 ), 'jy', 'req.jpg', 'aaaa1', random_test.txt );
insert into media2 values ( 'b', point(12 22 ), 'mt', 'req.jpg', 'aaaa2', callinfo.txt );
insert into media2 values ( 'c', point(23 24 ), 'sy', 'req.jpg', 'aaaa3', 'callinfo.txt' );

expect rows 3;
select * from media2;

expect rows 1;
select * from media2 where uid='b';

expect rows 1;
select * from media2 where uid='b' and pt:x=12 and pt:y=22;

expect rows 1;
select  *  from media2 where uid='a' and cover(pt, point(10 20));

rmfile output/out22.jpg;
rmfile output/out23.tiff;
expect file output/out22.jpg output/out23.tiff;
getfile jpg into 'output/out22.jpg', tiff into output/out23.tiff from media2 where uid='a' and pt:x=10 and pt:y=20 and name='jy';

expect rows 1;
getfile jpg size, tiff time, tiff md5, jpg time from media2 where uid='a';

expect value jpg_size 51857;
getfile jpg size, tiff time, tiff md5, jpg time from media2 where uid='b';


expect value jpg_size 51857;
getfile jpg size, tiff time, tiff md5, jpg time from media2 where uid='a' and  cover(pt, point(10 20));

expect rows 1;
getfile jpg size, tiff time, tiff md5, jpg time from media2 where cover(pt, point(10 20));


drop table if exists dv;
create table dv ( key: a double, b double, value: c longdouble);
insert into dv values (100, 1000, 10000);
insert into dv values (200, 2000, 20000);
insert into dv values (300, 3000, 30000);
insert into dv values (400, 4000, 40000);
insert into dv values (500, 5000, 50000);
insert into dv values (600, 6000, 60000);

expect value v 350;
select avg(a) v from dv;

expect value v 3500;
select avg(b) v from dv;

expect value v 35000.0;
select avg(c) v from dv;

### vector table
drop table if exists vec1;
create table vec1 ( key: a int, value: v vector, b char(4) );
expect words "vec1 vector b";
desc vec1;

drop index if exists vec1_idx1 on vec1;
create index vec1_idx1 on vec1( v );
expect words "vec1_idx1 vec1";
desc vec1_idx1;

insert into vec1 values (10, vector(1.2, 2.4, 3.2, 4.3, 5.7, 6.3, 100, 103.4), 'west' );
insert into vec1 values (20, vector(1.8, 8.4, 2.2, 1.3, 9.7, 11.3, 10, 38.4), 'sth' );
insert into vec1 values (30, vector(1.6, 4.2, 9.2, 4.3, 1.7, 15.3, 1, 13.4), 'w' );
insert into vec1 values (40, vector(2.6, 1.2, 7.2, 3.3, 7.3, 5.3, 8, 2.4), 's' );
select a, b, similarity(v, vector(1, 2, 3, 4, 5, 13, 1))  sim from vec1 order by sim desc;

select similarity(v, vector(1, 2, 3, 4, 5, 13, 1))  sim from test.vec1.vec1_idx1 order by sim desc;
select similarity(v, vector(1, 2, 3, 4, 5, 13, 1))  sim from test.vec1.vec1_idx1 order by sim desc limit 4;


### vector table
drop table if exists vec2;
create table vec2 ( key: v vector, value: a char(4) );
expect words "vec2 vector a";
desc vec2;
insert into vec2 values ( vector(1.2, 2.4, 3.2, 4.3, 5.7, 6.3, 100, 103.4), 'west' );
insert into vec2 values ( vector(1.8, 8.4, 2.2, 1.3, 9.7, 11.3, 10, 38.4), 'sth' );
select similarity(v, vector(1, 2, 3, 4, 5, 13, 1))  sim, a from vec2 order by sim desc;


drop table if exists p1;
create table p1 (key: a int, value: b int);
create index pidx1 on p1(b);

insert into p1 values (100, 1);
insert into p1 values (200, 1);

expect rows 1;
select * from test.p1.pidx1;


quit;
