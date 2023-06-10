
### geom tables
expect okmsg "";
drop table if exists geom1;

expect okmsg "";
create table if not exists geom1 ( key: a int, value: pt point(srid:4326), b int );

expect words "point spare_ test.geom1";
desc geom1 detail;

expect okmsg "";
drop index if exists geom1_idx1 on geom1;

expect okmsg "";
create index geom1_idx1 on geom1(b,pt);

insert into geom1 values ( 1, point(22 33), 123 );
insert into geom1 (a, pt, b ) values ( 2, point(22 33), 123 );
insert into geom1 (b, pt, a ) values ( 222, point(22 33), 12 );

expect rows 3;
select * from geom1;

expect rows 2;
select * from test.geom1.geom1_idx1;

expect okmsg "";
drop table if exists geom2;

expect okmsg "";
create table if not exists geom2 ( key: a int, value: pt1 point, b int, uid uuid, pt2 point(srid:wgs84) );

expect words "geom2 pt1 pt2";
desc geom2;

drop index if exists geom2_idx1  on geom2;
create index geom2_idx1 on geom2(b,pt2);

insert into geom2 values ( 1, point(22 33), 123, point(99 221) );
insert into geom2 values ( 10, json({"type":"Point", "coordinates": [2,3]}), 123, json({"type":"Point", "coordinates":[5,9]}) );
insert into geom2 (a, pt1, pt2, b ) values ( 2, point(22 33), point(23 421), 123 );
insert into geom2 (b, pt2, pt1, a ) values ( 222, point(22 33), point(90 21), 17 );

expect rows 4;
select * from geom2;


expect rows 4;
select * from test.geom2.geom2_idx1;

drop table if exists geom3;
create table if not exists geom3 ( key: pt1 point, value: b int, uid uuid, a int, pt2 point );

expect words "geom3 pt1 pt2";
desc geom3;

drop index if exists geom3_idx1  on geom3;
create index geom3_idx1 on geom3(b,pt2,uid);

insert into geom3 values ( point(22 33), 123, 2, point(99 221) );
insert into geom3 (b, pt2, pt1, a ) values ( 2, point(25 33), point(23 451), 153 );

expect rows 2;
select * from geom3;


expect rows 2;
select * from test.geom3.geom3_idx1;

drop table if exists d5;
create table if not exists d5 ( key: a int, pt1 point3d, b int, pt2 point3d, value: c int, pt3 point3d, d int, pt4 point3d(srid:wgs84) );
desc d5;

drop index if exists d5_idx  on d5;
create index d5_idx on d5(pt3, pt4, d, c );

insert into d5 values( 1, point3d(22 33 4), 23, point3d(99 22 1), 244, point3d(8 2 3), 234,  point3d(8 2 3) );
insert into d5 values( 2, point3d(32 83 0), 23, point3d(94 82 1), 214, point3d(9 7 2), 234,  point3d(1 2 3) );

expect rows 2;
select * from d5;


expect rows 2;
select * from test.d5.d5_idx;

drop table if exists d6;
create table if not exists d6 ( key: a int, pt1 point, b int, pt2 point, value: c int, pt3 point, d int, pt4 point3d );
desc d6;

drop index if exists d6_idx  on d6;
create index d6_idx on d6(pt4, pt3, d, c );

insert into d6 values( 1, point(22 33 ), 23, point(99 1), 244, point(8  3), 234,  point3d(8 2 3) );
insert into d6 values( 2, point(32 83 ), 23, point(94 82 ), 214, point(9  2), 234,  point3d(1 2 3) );
insert into d6 ( pt2, a, b, pt1 ) values ( json({"type":"Point", "coordinates": [123,321]}), 208, 12, point(91 17) );
insert into d6 ( pt2, a, b, pt1 ) values ( json({"type":"Point", "coordinates": [124,351]}), 209, 13, point(92 19) );

expect rows 4;
select * from d6;


# empty pt4 rows will not have index records, since blank first key is not allowed in table and index
expect rows 2;
select * from test.d6.d6_idx;

drop table if exists cir1;
create table if not exists cir1 ( key: a int, c1 circle(4326), b int, c2 circle, value: c int, c3 circle, d int, c4 circle );
desc cir1 detail;
drop index if exists cir1_idx1 on cir1;
create index cir1_idx1 on cir1(c3, d, c2);

insert into cir1 values ( 100, circle( 22 33 100), 123, circle(99 22 191), 2133, circle(99 22 12), 123, circle(88 33 2211) );
insert into cir1 values ( 101, circle( 92 33 140), 523, circle(99 42 191), 2133, circle(99 42 12), 823, circle(38 43 811) );

expect rows 2;
select * from test.cir1.cir1_idx1;

expect rows 2;
select * from cir1;

drop table if exists sph1;
create table if not exists sph1 ( key: a int, s1 sphere, b int, s2 sphere, value: c int, s3 sphere );
desc sph1 detail;
drop index if exists sph1_idx1 on sph1;
create index sph1_idx1 on sph1(c, s2, a, s1);

insert into sph1 values ( 100, sphere( 2 3 4 123), 321, sphere(99 22 33 20000), 321, sphere(99 223 12020 29292) );
insert into sph1 values ( 102, sphere( 2 3 4 123), 321, sphere(99 22 33 20000), 321, sphere(99 223 12020 29292) );
insert into sph1 (s1, b, a, s2 ) values ( sphere( 2 3 4 123), 921, 234, sphere(99 22 33 20000) );
insert into sph1 (s1, b, a, s2 ) values ( sphere( 2 32 5 123), 951, 534, sphere(99 22 33 20000) );


# if c has no value, no index records are created
expect rows 2;
select * from test.sph1.sph1_idx1;

expect rows 4;
select * from sph1;

drop table if exists sq1;
create table if not exists sq1 ( key: a int, s1 square, b int, s2 square, value: c int, s3 square );
desc sq1 detail;
drop index  if exists sq1_idx1 on sq1;
create index sq1_idx1 on sq1(s3);

insert into sq1 values ( 100, square( 22 453 22222), 100, square(9 3 123), 299, square(82 332 1212) );

expect rows 1;
select * from sq1;

drop table if exists cb1;
create table if not exists cb1 ( key: a int, q1 cube, b int, q2 cube, value: c int, q3 cube );
desc cb1 detail;
drop index  if exists cb1_idx1 on cb1;
create index cb1_idx1 on cb1(q3,q2, b);

insert into cb1 values ( 111, cube( 2 3 4 1233), 1234, cube(233 22 55 9393), 3212, cube(92 92 82 2345) );

expect rows 1;
select * from cb1;


expect rows 1;
select * from test.cb1.cb1_idx1;


drop table if exists rect1;
create table if not exists rect1 ( key: a int, r1 rectangle, value: c int );
expect words "rect1 r1";
desc rect1 detail;

insert into rect1 values ( 1, rectangle(22 33 88 99), 233 );
insert into rect1 ( c, a, r1 ) values ( 22, 31, rectangle(29 13 48 19) );
drop index  if exists rect1_idx1 on rect1;
create index rect1_idx1 on rect1(c, r1);

expect rows 2;
select * from rect1;

drop table if exists bx1;
create table if not exists bx1 ( key: a int, b1 box, value: c int, b2 box );

expect words "bx1 b1 b2";
desc bx1 detail;

drop index  if exists bx1_idx1 on bx1;
create index bx1_idx1 on bx1(b2, c );

insert into bx1 values ( 1, box(22 33 44 88 99 123), 233, box(9 9 9 22 22 33) );
insert into bx1 ( c, a, b1 ) values ( 22, 31, box(29 13 48 19 21 12) );

expect rows 2;
select * from bx1;

expect rows 2;
select distance( point3d(0 0 0), b2, 'max') as maxdist from bx1;

expect rows 2;
select distance( point3d(0 0 0), b2, 'min') as mindist from bx1;


drop table if exists cyn1;
create table if not exists cyn1  ( key: a int, c1 cylinder, value: c int );

expect words "cyn1 c1";
desc cyn1 detail;

drop index  if exists cyn1_idx1 on cyn1;
create index cyn1_idx1 on cyn1(c);

insert into cyn1 values ( 1, cylinder(1 2 3 45 88 0.3), 1239 );
insert into cyn1 ( c, c1, a ) values ( 13, cylinder(1 2 3 45 88), 139 );

expect rows 2;
select * from cyn1;

select * from test.cyn1.cyn1_idx1;

drop table if exists cn1;
create table if not exists cn1  ( key: a int, c1 cone, value: c int, c2 cone );
desc cn1 detail;

drop index  if exists cn1_idx1 on cn1;
drop index  if exists cn1_idx2 on cn1;
create index cn1_idx1 on cn1(c2);
create index cn1_idx2 on cn1(c,c2);

insert into cn1 values ( 1, cone(1 2 3 45 88), 1239, cone(33 22 44 44 99 0.4 0.3) );
insert into cn1 ( c, c1, a  ) values ( 13, cone(1 2 3 45 88), 139 );

expect rows 2;
select * from cn1;


# empty c2 has no records
expect rows 1;
select * from test.cyn1.cn1_idx1;

expect rows 2;
select * from test.cyn1.cn1_idx2;

drop table if exists el1;
create table if not exists el1  ( key: a int, c1 ellipse, value: c int, c2 ellipse );
desc el1 detail;

drop index  if exists el1_idx1 on el1;
drop index  if exists el1_idx2 on el1;
create index el1_idx1 on el1(c2,c);
create index el1_idx2 on el1(c,c2);

insert into el1 values ( 1, ellipse(1 2 45 88), 1239, ellipse(22 44 44 99) );
insert into el1 ( c, c1, a  ) values ( 13, ellipse(2 3 45 88), 139 );

expect rows 2;
select * from el1;

# empty c2 has no records
expect rows 1;
select * from test.el1.el1_idx1;

expect rows 2;
select * from test.el1.el1_idx2;

drop table if exists es1;
create table if not exists es1  ( key: a int, c1 ellipsoid, value: c int, c2 ellipse );
desc es1 detail;


drop index  if exists es1_idx1 on es1;
drop index  if exists es1_idx2 on es1;
drop index  if exists es1_idx3 on es1;
create index es1_idx1 on es1(c2);
create index es1_idx2 on es1(c,c2);
create index es1_idx3 on es1(c1,a,c2);

insert into es1 values ( 1, ellipsoid(1 2 3 45 88 99), 1239, ellipse(22 44 44 99) );
insert into es1 ( c, c1, a  ) values ( 13, ellipsoid(2 3 4 45 88 99), 139 );

expect rows 2;
select * from rect1;

expect rows 2;
select * from es1;


expect rows 1;
select * from test.es1.es1_idx1;

expect rows 2;
select * from test.es1.es1_idx2;

expect rows 2;
select * from test.es1.es1_idx3;

drop table if exists line1;
create table if not exists line1  ( key: a int, c1 line, value: c int, c2 line );
desc line1 detail;
drop index  if exists line1_idx1 on line1;
create index line1_idx1 on line1(c2, c, c1 );

insert into line1 values ( 1, line(1 2, 45 8.3), 1239, line(44 99, 291 9.1 ) );
insert into line1 values ( 3, line(1 20, 4 3), 139, line(4 9, 91 9 ) );

expect rows 2;
select * from line1;


expect rows 2;
select * from test.line1.line1_idx1;


drop table if exists line3d2;
create table if not exists line3d2  ( key: a int, c1 line3d, value: c int, c2 line3d );
desc line3d2 detail;
drop index  if exists line3d2_idx1 on line3d2;
create index line3d2_idx1 on line3d2(c2, c, c1 );

insert into line3d2 values ( 1, line3d(1 2 45 8.3 22 3.3), 1239, line3d(44 99 291 9.1 33 44 ) );

expect rows 1;
select * from line3d2;


expect rows 1;
select * from test.line3d2.line3d2_idx1;

drop table if exists tri1;
create table if not exists tri1 ( key: t1 triangle, value: a int );


insert into tri1 values ( triangle( 11 33 88 99 21 32), 123 );
insert into tri1 values ( triangle( 31 33 18 99 33 44), 223 );
drop index  if exists tri1_idx1 on tri1;
create index tri1_idx1 on tri1( a );

drop index  if exists tri31_idx1  on tri1;
create index tri31_idx1 on tri1( a );

expect rows 2;
select * from test.tri1.tri1_idx1;

drop table if exists tri31;
create table if not exists tri31 ( key: t1 triangle3d, value: a int );

insert into tr31 values ( triangle3d( 11 33 88 99 23 43 9 8 2), 123 );
insert into tr31 values ( triangle3d( 31 33 18, 99 12 34, 9 9 1), 223 );

expect rows 2;
select * from test.tri1.tri31_idx1;


### queries
select * from cir1 where within(point(10 22), c1 );
select * from cir1 where coveredby(point(10 22), c1 );
select * from cir1 where contain(c1, point(10 22) );
select * from cir1 where cover(c1, point(10 22) );

select * from cir1 where within( c1, rectangle(1 2 23 34 0.1) );
###                                            x y  a  b nx
select * from cir1 where disjoint( c1, rectangle(1 2 23 34 0.1) );
select * from cir1 where nearby( c1, rectangle(1 2 23 34 0.1), 200 );
select distance( c1, point(22 33), 'center' ) as dist from cir1;
select distance( c1, point(22 33), 'max' ) as dist from cir1;
select distance( c1, point(22 33), 'min' ) as dist from cir1;
select distance( point(22 33), c1, 'center' ) as dist from cir1;
select distance( point(22 33), c1, 'max' ) as dist from cir1;
select distance( point(22 33), c1, 'min' ) as dist from cir1;

select * from cb1 where within( point3d(100 200 300), q1 );
###                                      x  y    z
select * from cb1 where cover( q1, sphere(11 234 234  100) );
###                                        x   y   z    r

select * from cb1 where nearby( q2, sphere(31 434 235  100), 3000 );

select * from cb1 where nearby( q2, ellipsoid(31 434 235  100 200 200), 3000 );

select * from cb1 where nearby( q3, ellipsoid(31 434 235  100 200 300 0.1 0.2), 3000 );
###                                           x   y   z    a   b   c   nx ny



### linestring

drop table if exists lstr1;
create table lstr1 ( key: a int, value: ls linestring );
 insert into lstr1 values ( 1, linestring( 3 3, 4 4, 5 5 ) );
 select * from lstr1;
 select * from lstr1 where contain(ls, point(3 3) );


drop table if exists linestr1;

create table linestr1 ( key: a int, value: ls1 linestring(wgs84), b int, ls2 linestring );

desc linestr1 detail;

drop index if exists linestr1_idx1 on linestr1;
create index linestr1_idx1 on linestr1( b, ls2 );
expect words "linestr1_idx1 b ls2";
desc linestr1_idx1 detail;

 insert into linestr1 values ( 1, linestring( 10 2,2 33 , 33 44, 55 66, 58 68, 77 88 ), 200, linestring( 11 11, 13 13, 17 17 ) );

 insert into linestr1 values ( 2, linestring( 15.13 2,2.9 33 , 33 44, 5.5 6.6, 55 66, 77 88 ), 210, linestring( 3.3 4.4, 5.5 6.6, 8.9 9 ) );
 insert into linestr1 values ( 3,json('{"type":"LineString","coordinates": [[2,3],[3,4]]}'),121,json({"type":"LineString","coordinates": [[2,3],[3,4]]}) );

expect rows 20;
select * from linestr1;

#expect rows 2;
select * from linestr1 where cover(ls2, point(13 13) );

expect rows 0;
select ls1 from linestr1;

expect rows 20;
select ls1:x, ls1:y from linestr1;

select * from linestr1 where within( ls1, square( 10 10 15938178.1 ) );

expect rows 10;
select * from test.linestr1.linestr1_idx1;


 drop table if exists linestr21;
 create table linestr21 ( key: ls1 linestring(wgs84), value: a int );
 desc linestr21 detail;

drop index if exists linestr21_idx1 on linestr21;
create index linestr21_idx1 on linestr21( a );
desc linestr21_idx1 detail;

 insert into linestr21 values ( linestring( 11 2,2 33 , 33 44, 55 66, 55 66, 77 88 ), 200 );

expect rows 7;
select * from linestr21;

expect rows 1;
select * from linestr21 where within( ls1, square( 10 10 78.1 ) );


expect rows 1;
select * from test.linestr21.linestr21_idx1;
 
 drop table if exists linestr2;
 create table linestr2 ( key: ls1 linestring(wgs84), a int, value: ls2 linestring );
 desc linestr2 detail;

drop index if exists linestr2_idx1 on linestr2;
create index linestr2_idx1 on linestr2( ls2 );

 insert into linestr2 values ( linestring( 1 2,2 33 , 33 44, 55 66, 55 66, 77 88 ), 200, linestring( 33 44, 55 66, 8 9 ) );
 insert into linestr2 values ( linestring( 1.13 2,2.9 33 , 33 44, 5.5 6.6, 55 66, 77 88 ), 210, linestring( 3.3 4.4, 5.5 6.6, 8.9 9 ) );

expect rows 18;
select * from linestr2;

select * from linestr2 where within( ls1, square( 10 10 78.1 ) );

expect words "linestr2_idx1 geo:id geo:col";
desc linestr2_idx1 detail;

expect rows 18;
select * from test.linestr2.linestr2_idx1;

#todo

 drop table if exists linestr3;
 create table linestr3 ( key: ls1 linestring(wgs84), a int, value: ls2 linestring, b int );
 expect words "linestr3 ls1 linestring srid 4326 ls2 a b";
 desc linestr3 detail;

 drop index if exists linestr3_idx1 on linestr3;
 create index linestr3_idx1 on linestr3( b, ls2 );
 expect words "linestr3_idx1 ls2 b";
 desc linestr3_idx1 detail;

 insert into linestr3 values ( linestring( 211 2,2 33 , 33 44, 55 66, 55 66, 77 88 ), 200, linestring( 33 44, 55 66, 8 9 ), 804 );

 insert into linestr3 values ( linestring( 211.13 2,2.9 33 , 33 44, 5.5 6.6, 55 66, 77 88 ), 210, linestring( 3.3 4.4, 5.5 6.6, 8.9 9 ), 805 );

 expect rows 18;
 select * from linestr3;

 select * from linestr3 where within( ls2, square( 10 10 78.1 ) );

 expect rows 1;
 select ls2:x, ls2:y from linestr3 where ls2:x='8.9';

 expect rows 1;
 select ls2:x, ls2:y from linestr3 where ls2:x=8.9;

 expect rows 1;
 select ls2:x, ls2:y from linestr3 where ls2:x = 8.9;

 expect rows 1;
 select ls2:x, ls2:y from linestr3 where ls2:x= 8.9;

 expect rows 1;
 select ls2:x, ls2:y from linestr3 where ls2:x =8.9;

 select ls1 from linestr3 where intersect( ls1, square( 10 10 78.1 ) );

 select ls1, ls2 from linestr3 where intersect( ls1, square( 10 10 78.1 ) ) and intersect( ls2, square( 10 10 1000 ) );

 expect rows 8;
 select * from test.linestr3.linestr3_idx1;


 drop table if exists linestr3d1;
 create table linestr3d1 ( key: ls1 linestring3d(wgs84), a int, value: ls2 linestring, b int );
 expect words "linestr3d1 linestring3d srid 4326 ls2 linestring b";
 desc linestr3d1 detail;

drop index if exists  linestr3d1_idx1 on linestr3d1;
 create index linestr3d1_idx1 on linestr3d1( b, ls2 );

 insert into linestr3d1 values ( linestring3d( 1 2 2,1 2 33 , 8 33 44, 8 55 66  ), 200, linestring( 303 404, 505 606  ), 804 );
 insert into linestr3d1 values ( linestring3d( 1.1 2 2, 2 2.9 3 , 3 3 4, 2 5 6  ), 210, linestring( 3.3 4, 5 6  ), 805 );
 insert into linestr3d1 values ( linestring3d( 0 -10 0, 0 10 0, 2 2.9 3 , 3 3 4, 2 5 6 ), 310, linestring( 3.3 4, 5 6 ), 805 );
 insert into linestr3d1 values ( linestring3d( 0 -20 0, 0 20 0, 2 2.7 3.8  ), 315, linestring( 3.3 4.2, 5.1 6.7 ), 808 );

 expect rows 24;
 select * from linestr3d1;

select * from linestr3d1 where contain(ls2, point(303 404) );

select * from linestr3d1 where contain(ls1, point3d(1 2 3) );

 expect value xm 505;
 select max(ls2:x) xm from linestr3d1;

 expect rows 4;
 select xmax(ls2) xm from linestr3d1;

 expect value xm 35.1;
 select sum(ls1:x) xm from linestr3d1;

 expect value ym 118.5;
 select sum(ls1:y) ym from linestr3d1;

 expect value miny -20;
 select min(ls1:y) miny  from linestr3d1;

 expect value maxy 55;
 select max(ls1:y) maxy  from linestr3d1;


 expect rows 24;
 select ls2:x, ls2:y from linestr3d1;

 expect rows 8;
 select geo:id, geo:col, geo:i, ls2:x, ls2:y from linestr3d1 where ls2:x > 0;

 expect rows 0;
 select geo:id, geo:col, geo:i, ls2:x, ls2:y from linestr3d1 where ls2:x  > 10 and ls2:x < 100;

 select geo:id, geo:col, geo:i, ls2:x, ls2:y from linestr3d1 where ls2:x  > 10 and ls2:x < 400 and ls2:y > 50;

 select * from linestr3d1 where within( ls1, cube( 10 10 10 78.1 ) );

 select ls2:x, ls2:y  from linestr3d1 where within( ls1, cube( 10 10 10 78.1 ) );

 select geojson(ls2) from linestr3d1 where within( ls2, square( 0 0 1000000 ) );

 select * from linestr3d1 where intersect( ls1, cube( 10 10 10 78.1 ) );

 select * from linestr3d1 where intersect( ls1, linestring3d( 0 0 -10, 0 0 10, 10 10 78.1) );

 select geojson(ls1) from linestr3d1 where intersect( ls1, linestring3d( 0 0 -10, 0 0 10, 10 10 78.1) );


 expect words "linestr3d1_idx1 linestr3d1 b ls2";
 desc linestr3d1_idx1 detail;

 expect rows 9;
 select * from test.linestr3d1.linestr3d1_idx1;

 expect rows 9;
 select b from test.linestr3d1.linestr3d1_idx1;

 expect rows 4;
 select ls1:x, ls1:y from test.linestr3d1.linestr3d1_idx1 where ls1:x >= 3;


 drop table if exists lstr;
 create table lstr ( key: a int, value: ls linestring );
 expect words "lstr a ls linestring";
 desc lstr;

 insert into lstr values ( 1, linestring(0.1 0.1, 24 11) );
 insert into lstr (ls, a) values ( linestring(0.2 0.2, 26 50), 121 );
 insert into lstr (a, ls) values ( 124, linestring(1.3 1.3, 84 95) );

 expect rows 6;
 select * from lstr;

 select geojson(ls) from lstr where intersect(ls, linestring(10 -10, 10 10) );

 select geojson(ls) from lstr where within(ls, square(0 0 10000) );

select pointn(ls,1) from lstr;
select pointn(ls,2) from lstr;
select startpoint(ls) from lstr;
select endpoint(ls) from lstr;
select isclosed(ls) from lstr;
select numpoints(ls) from lstr;
select xmin(ls) from lstr;
select ymin(ls) from lstr;
select xmax(ls) from lstr;
select ymax(ls) from lstr;
select convexhull(ls) from lstr;
select closestpoint( point(1 1), ls) from lstr;

#qwer
select interpolate(ls,0.5) from lstr;
select linesubstring(ls,0.2, 0.8 ) from lstr;
select locatepoint(ls, point( 3 9) ) from lstr;
select addpoint( ls, point(234 219) ) from lstr;
select addpoint( ls, point(234 219), 2 ) from lstr;
select setpoint( ls, point(234 219), 1 ) from lstr;
select removepoint( ls, 2 ) from lstr;
select reverse( ls ) from lstr;
select scale( ls, 3 ) from lstr;
select scale( ls, 10, 20 ) from lstr;
select scaleat( ls, point(10 20), 10 ) from lstr;
select scaleat( ls, point(10 20), 10, 20 ) from lstr;
select scalesize( ls, 10 ) from lstr;
select scalesize( ls, 10, 20 ) from lstr;
select translate( ls, 10, 20 ) from lstr;
select transscale( ls, 200, 300, 10 ) from lstr;
select transscale( ls, 200, 300, 10, 20 ) from lstr;
select rotate( ls, 180 ) from lstr;
select rotate( ls, 1.0, 'radian' ) from lstr;
select rotateself( ls, 180 ) from lstr;
select rotateat( ls, 1.80, 'radian', 100, 300 ) from lstr;
select affine( ls, 1, 2,3, 4, 500, 600 )  from lstr;


 
 drop table if exists pol1;
 create table pol1 ( key: a int, value: pol polygon );
 expect words "pol1 a pol polygon";
 desc pol1;

 insert into pol1 values ( 1, polygon( (0 0, 20 0, 88 99, 0 0) ) );
 insert into pol1 values ( 21, polygon( (0 0, 80 0, 80 80, 0 80, 0 0) ) );
 insert into pol1 values ( 2,  json({"type":"Polygon", "coordinates": [[[0,0], [2,0], [8,9], [0, 0]], [[1, 2], [2, 3],[1, 2]]]}) );
 insert into pol1 values ( 3,  json({"type":"Polygon", "coordinates": [[[0,0], [2,0], [8,9], [0, 0]], [[1, 2], [2, 3],[1, 2]]]}) );

 expect rows 23;
 select * from pol1;

 select * from pol1 where intersect( pol, line(0 10 80 10 ) );

 select geojson(pol) from pol1 where intersect(pol, linestring(10 -10, 10 10) );

 select geojson(pol) from pol1 where within(pol, square(0 0 10000) );

 select geojson(pol) from pol1 where intersect(pol, square(0 0 10000) );

 drop table if exists pol2;
 create table pol2 ( key: a int, value: po2 polygon, po3 polygon3d, tm timestamp default current_timestamp, ls linestring );
 expect words "pol2 a po2 po3 polygon polygon3d tm timestamp ls linestring default current_timestamp";
 desc pol2;

 insert into pol2 values( 1, polygon((0 0,2 0,8 9,0 0),(1 2,2 3,1 2)),polygon3d((1 1 1,2 2 2,3 3 3,1 1 1),(2 2 2,3 3 1,2 2 2)), '', linestring(30 40,40 50,5 6));
 insert into pol2 values( 2, json({"type":"Polygon", "coordinates": [[[0,0], [2,0], [8,9], [0, 0]], [[1, 2], [2, 3],[1, 2]]]}),polygon3d((4 1 2,2 2 2,3 3 3,1 9 1, 4 1 2),(2 2 2,3 3 1, 8 2 9, 2 2 2)), '', linestring(30 40,40 50,5 6));

 # 36 data points
 expect rows 36;
 select * from pol2;

 select geojson(po3) from pol2 where within(po3, cube(0 0 0 100000) );

 select geojson(po2) from pol2 where within(po2, square( 0 0 100000) );

 select geojson(po2) from pol2 where intersect(po2, square( 0 0 100000) );


 drop table if exists mp;
 create table mp ( key: a int, value: m1 multipoint, m2 multipoint3d );
 expect words "mp ( key: a int, value: m1 multipoint, m2 multipoint3d )";
 desc mp detail;

 insert into mp ( m1, a, m2 ) values ( multipoint( 1 2 , 3 4, 2 1 ), 100, multipoint3d( 1 2 3, 3 4 5, 2 2 1) );
 insert into mp values ( 123, multipoint( 1 2 , 3 4, 2 1 ), multipoint3d( 1 2 3, 3 4 5, 2 2 1) );
 insert into mp values ( 125, multipoint( 1 2 , 3 4, 2 1 ), json({"type":"MultiPoint", "coordinates": [ [1,2,3],[3,4,5] ] } );

 expect rows 17;
 select * from mp;

 drop table if exists mline;
 create table mline ( key: a int, value: l1 multilinestring, l2 multilinestring3d );
 expect words "mline key: a int, value: l1 multilinestring, l2 multilinestring3d ";
 desc mline detail;

 insert into mline values( 1, multilinestring((0 0,2 0,8 9,0 0),(1 2,2 3,1 2)),multilinestring3d((1 1 1,2 2 2,3 3 3),(2 2 2,3 3 1)));
 insert into mline values( 1024, multilinestring( (1 1, 2 3, 4 5 , 4 9 )), multilinestring3d(( 0 0 0, 1 9 9, 11 12 13, 33 32 34 )) );
 insert into mline values( 3, json({"type":"MultiLineString","coordinates": [ [ [0,0],[2,0],[8,9],[0,0]], [[1,2],[2,3],[1,2]]]}),multilinestring3d((1 1 1,2 2 2,3 3 3),(2 2 2,3 3 1)));

 # 32 data points (including all 2D and 3D points)
 expect rows 32;
 select * from mline;

 select geojson(l1) from mline where intersect(l1, square( 0 0 100000) );

 select geojson(l2) from mline where intersect(l2, cube( 0 0 0 100000) );

select numpoints(l2) from mline;
select numsegments(l2) from mline;
select numrings(l2) from mline;


 drop table if exists mpg;
 create table mpg ( key: a int, value: p1 multipolygon, p2 multipolygon3d );
 expect words "mpg ( key: a int, value: p1 multipolygon, p2 multipolygon3d ) p2:x p2:y";
 desc mpg detail;

 insert into mpg values( 1, 
     multipolygon(((0 0,2 0,8 9,0 0),(1 2,2 3, 7 8,1 2))),
     multipolygon3d(((1 1 1,2 2 2,3 3 3, 1 1 1),(2 2 2,3 3 1, 3 5 6, 2 2 2 ))));

 insert into mpg values( 2, 
 	 multipolygon( ((0 0,2 0,8 9,0 0),(1 2,2 3, 7 8,1 2)), ((0 0, 2 2, 3 3, 0 0)) ),
	 multipolygon3d(((1 1 1,2 2 2,3 3 3, 1 1 1),(2 2 2,3 3 1, 3 5 6, 2 2 2 ))));

 insert into mpg values( 3, 
 	 multipolygon( ((0 0,2 0,8 9,0 0),(1 2,2 3, 7 8,1 2)), ((0.1 0.2, 2.2 2.2, 5 5, 0.1 0.2)) ),
	 multipolygon3d(((1 1 1,2 2 2,3 3 3, 1 1 1),(2 2 2,3 3 1, 3 5 6, 2 2 2 ))));

 insert into mpg values( 30, 
 	 json( { "type":"MultiPolygon","coordinates": [ [[[4,0], [2,0], [8,9], [4, 0]], [[1, 5], [2, 3],[1, 5]]], [[[4,4], [2,0], [8,9], [4, 4]], [[1, 2], [2, 3],[1, 2]]] ] } ),
	 multipolygon3d(((1 1 1,2 2 2,3 3 3, 1 1 1),(2 2 2,3 3 1, 3 5 6, 2 2 2 ))));

 insert into mpg values( 32, 
 	 json( { "type":"MultiPolygon","coordinates": [ [[[4,0], [2,0], [8,9], [4, 0]], [[1, 5], [2, 3],[1, 5]]], [[[4,4], [2,0], [8,9], [4, 4]], [[1, 2], [2, 3],[1, 2]]] ] } ) ) );
	 

 expect rows 92;
 select * from mpg;

 select p1 from mpg where intersect(p1, square( 0 0 100000) );

 expect rows 1;
 select p2 from mpg where intersect(p2, cube( 0 0 0 100000) );

select area(p1) area1, area(p2) area2 from  mpg;

select area(p1) area1 from  mpg;

expect value area1 130.62;
select sum(area(p1)) area1 from  mpg;

expect value area2 0;
select sum(area(p2)) area2 from  mpg;

 expect rows 5;
select dimension(p1), dimension(p2) from mpg;

 expect rows 5;
select geotype(p1) from mpg;

 expect words "MultiPolygon";
select geotype(p1) from mpg limit 1;

 expect words "MultiPolygon";
select geotype(p1) from mpg where a=30;

 expect rows 2;
select geotype(p1) from mpg where a=30;

 expect rows 2;
select geojson(p1) from mpg where a=30;

expect rows 5;
select extent(p1) from mpg;

expect rows 4;
select extent(p2) from mpg;

expect rows 5;
select isclosed(p1) as sis from mpg;

expect rows 5;
select numrings(p1) from mpg;

expect rows 2;
select numrings(p1) nr from mpg where a=1;

expect value nr 2;
select numrings(p1) nr from mpg where a=1;

expect value nr 3;
select numrings(p1) nr from mpg where a=2;

expect value nr 3;
select numrings(p1) nr from mpg where a=3;

expect value nr 2;
select numrings(p2) nr from mpg where a=1;

expect rows 5;
select numpolygons(p1) from mpg;

expect value np 2;
select numpolygons(p1) np from mpg where a=2;

expect value np 1;
select numpolygons(p2) np from mpg where a=2;

expect value srid 0;
select srid(p1) srid from mpg where a=1;

expect value srid 0;
select srid(p2) srid from mpg where a=1;

expect rows 5;
select summary(p1) from mpg;

expect rows 5;
select convexhull(p1) from mpg;

expect rows 5;
select centroid(p1) from mpg;

expect rows 5;
select outerrings(p1) from mpg;

expect rows 5;
select innerrings(p1) from mpg;

select innerrings(p2) from mpg;

select polygonn(p1,1) p from mpg;
select ringn(polygonn(p1,1), 1) rn from mpg;
select geojson(ringn(polygonn(p1,1), 1)) gjs from mpg;


select intersection('polygon((0 0, 8 0, 8 8, 0 8, 0 0),( 3 4, 4 6, 4 2, 3 4 ))', p1 ) dd from mpg;
select geojson(intersection('polygon((0 0, 8 0, 8 8, 0 8, 0 0),( 3 4, 4 6, 4 2, 3 4 ))', p1 )) dd from mpg;

select geojson(intersection(p1, 'polygon((0 0, 8 0, 8 8, 0 8, 0 0),( 3 4, 4 6, 4 2, 3 4 ))'  )) dd from mpg;

# NULL, 2D and 3D shapes have no operation
select geojson(intersection('polygon((0 0, 8 0, 8 8, 0 8, 0 0),( 3 4, 4 6, 4 2, 3 4 ))', p2 )) dd from mpg;

# NULL, 2D and 3D shapes have no operation
select union(p1,p2) from mpg;

select union('polygon((0 0, 8 0, 8 8, 0 8, 0 0),( 3 4, 4 6, 4 2, 3 4 ))', p1 ) dd from mpg;
select geojson(union('polygon((0 0, 8 0, 8 8, 0 8, 0 0),( 3 4, 4 6, 4 2, 3 4 ))', p1 )) dd from mpg;
select union(p1, 'polygon((0 0, 8 0, 8 8, 0 8, 0 0),( 3 4, 4 6, 4 2, 3 4 ))'  ) dd from mpg;


### range tests
 drop table if exists rg2;
 create table rg2 ( key: a int, value: dt datetime, d date, t time, r range(datetime) );
 expect words "table rg2 ( key: a int, value: dt datetime, d date, t time, r range(datetime) );";
 desc rg2;

 insert into rg2 values ( 1, '2018-10-10 01:01:01', '2018-12-12', '12:11:11', range( '2015-10-10 01:01:01', '2028-10-10 01:01:01' ) );
 insert into rg2 values ( 2, '2014-10-10 01:01:01', '2015-12-12', '14:11:11', range( '2010-10-10 01:01:01', '2028-12-31 01:01:01' ) );

 expect rows 2;
 select * from rg2;

# fail ?
 expect rows 2;
 select * from rg2 where within(d, range('2000-10-10', '2030-01-01') );

# fail ?
 expect rows 1;
 select * from rg2 where within(t, range('01:01:01', '13:13:11') );

# fail ?
 expect rows 2;
 select * from rg2 where within(dt, range('1980-01-1 01:01:01', '2019-08-09 13:13:11') );

 expect rows 2;
 select * from rg2 where intersect(r, range('1980-01-1 01:01:01', '2019-08-09 13:13:11') );

 expect rows 0;
 select * from rg2 where intersect(r, range('1980-01-1 01:01:01', '1999-08-09 13:13:11') );


drop table if exists pold;
create table pold ( key: a int , value: name char(64), pol polygon(wgs84));
expect words "pold ( key: a int , value: name char(64), pol polygon srid )";
desc pold;

insert into pold values(1, "California",json({"type":"Polygon","coordinates":[[[-123.23325,42.006187],[-122.37885,42.01166],[-121.037,41.99523],[-120.00186,41.99523],[-119.99638,40.26452],[-120.00186,38.999348],[-118.71478,38.101128],[-117.4989,37.21934],[-116.540436,36.50186],[-115.85034,35.970596],[-114.63446,35.00118],[-114.63446,34.87521],[-114.47015,34.710903],[-114.33323,34.44801],[-114.136055,34.305607],[-114.25655,34.174164],[-114.41538,34.108437],[-114.53587,33.933174],[-114.497536,33.697666],[-114.52492,33.54979],[-114.72757,33.40739],[-114.66184,33.034958],[-114.52492,33.02948],[-114.47015,32.843266],[-114.52492,32.755634],[-114.72209,32.717297],[-116.04751,32.624187],[-117.126465,32.536556],[-117.24696,32.668003],[-117.25243,32.876125],[-117.32912,33.12259],[-117.47151,33.29785],[-117.7837,33.538837],[-118.18352,33.76339],[-118.26019,33.703144],[-118.41355,33.74148],[-118.39164,33.84007],[-118.5669,34.042713],[-118.802414,33.998898],[-119.21866,34.14678],[-119.27891,34.26727],[-119.55823,34.415146],[-119.87589,34.40967],[-120.13879,34.47539],[-120.47288,34.44801],[-120.64814,34.579456],[-120.6098,34.85878],[-120.67005,34.902596],[-120.63171,35.099766],[-120.8946,35.247643],[-120.905556,35.45029],[-121.00414,35.461243],[-121.16845,35.636505],[-121.28346,35.674843],[-121.332756,35.78438],[-121.71614,36.195152],[-121.89688,36.315643],[-121.93522,36.638786],[-121.85854,36.6114],[-121.787346,36.803093],[-121.92974,36.978355],[-122.105,36.956448],[-122.33504,37.11528],[-122.41719,37.24125],[-122.400764,37.36174],[-122.51578,37.520573],[-122.51578,37.783466],[-122.32956,37.783466],[-122.406235,38.15042],[-122.488396,38.112083],[-122.50482,37.931343],[-122.701996,37.893005],[-122.9375,38.029926],[-122.97584,38.265434],[-123.129196,38.451653],[-123.33184,38.56667],[-123.44138,38.698112],[-123.73714,38.95553],[-123.68784,39.032207],[-123.82477,39.366302],[-123.76452,39.552517],[-123.85215,39.83184],[-124.109566,40.105686],[-124.3615,40.25904],[-124.4108,40.43978],[-124.15886,40.877937],[-124.109566,41.025814],[-124.15886,41.14083],[-124.06575,41.442062],[-124.1479,41.715908],[-124.25745,41.78163],[-124.21363,42.00071],[-123.23325 ,42.006187]]]}));

expect rows 93;
select * from pold;

expect rows 93;
select pol:x, pol:y from pold;

expect rows 93;
select pol:x from pold;

expect rows 93;
select pol:y from pold;

drop table if exists lstrm;
create table lstrm ( key: a int, b int, value: ls linestring(srid:4326,metrics:10) );
expect words "lstrm ( key: a int, b int, value: ls linestring(srid:4326,metrics:10) )";
desc lstrm;

insert into lstrm values ( 100, 200, linestring(0 80 100 200 300, 0.1 80.2 300 400 550 600 700, 0.2 80.5 1000 2000 23456, 0.8 80.9 10000 30000) );

expect rows 4;
select * from lstrm;




select convexhull(pol) from pol1;
select volume(po3) from pol2;
select volume(q1), volume(q2) from cb1;
select closestpoint( point(1 1 ), pol) from pol1;
select angle(c1, c2) from line1;
select angle(line(0 0, 2 5), c2) from line1;
select angle(line3d(0 0 0, 3 4 5), l) from line3d;
select buffer(c2,'distance=symmetric:2,join=round:10,end=round,point=circle:20') from line1;
select buffer(c2,'distance=asymmetric:2,join=miter:10,end=flat,point=square:20') from line1;
select length(c2) from line1;
select perimeter(pol) from pol1;
select perimeter(s1) from sq1;
select equal(s1,s2) from sq1;
select issimple(c2) from line1;
select issimple(pol) from pol1;
select isvalid(pol) from pol1;
select isring(pol) from pol1;
select isring(c2) from line1;
select ispolygonccw(pol) from pol1;
select ispolygoncw(pol) from pol1;
select outerring(pol) from pol1;
select ringn(pol,1) from pol1;
select ringn(pol,2) from pol1;
select innerringn(pol,1) from pol1;
select unique(c2) from line1;
select union(c1,c2) from line1;
select union(pol,'polygon((0 0, 2 3, 2 4, 8 2, 3 9, 0 0))') from pol1;
select union(pol,polygon((0 0, 2 3, 2 4, 8 2, 3 9, 0 0))) from pol1;
select collect(pol,polygon((0 0, 2 3, 2 4, 8 2, 3 9, 0 0))) from pol1;
select collect(p1,polygon((0 0, 2 3, 2 4, 8 2, 3 9, 0 0))) from mpg;
select topolygon(c1,30) from cir1;
select topolygon(s1,30) from sq1;
select topolygon(s1) from sq1;
select topolygon(s1) s1pgon, topolygon(s2) s2pgon from sq1;
select text(s1) from sq1;
select difference(line(0 0, 2 2), point(2 2) ) df;
select difference(linestring(0 0, 2 2, 3 4), point(2 2) ) df;
select difference(linestring(0 0, 2 2, 3 4, 4 6), line(2 2, 3 4) ) df;
select difference(pol, polygon((0 0, 8 0, 800 800, 80 80, 0 0),( 3 4, 4 6, 4 2, 3 4 )) ) df from pol1;
select difference( 'polygon((0 0, 8 0, 8 8, 0 8, 0 0),( 3 4, 4 6, 4 2, 3 4 ))', pol ) df from pol1;

select symdifference(line(0 0, 2 2), point(2 2) ) df;
select symdifference(linestring(0 0, 2 2, 3 4), point(2 2) ) df;
select symdifference(linestring(0 0, 2 2, 3 4, 4 6), line(2 2, 3 4) ) df;
select symdifference(linestring(0 0, 2 2, 3 4, 4 6), linestring(2 2, 3 4, 8 9) ) df;
select symdifference(pol, polygon((0 0, 8 0, 800 800, 30 800, 0 0),( 3 4, 4 6, 4 2, 3 4 )) ) df from pol1;
select symdifference(pol, polygon((0 0, 8 0, 800 800, 80 80, 0 0),( 3 4, 4 6, 4 2, 3 4 )) ) df from pol1;
select symdifference( 'polygon((0 0, 8 0, 800 800, 80 80, 0 0),( 3 4, 4 6, 4 2, 3 4 ))', pol ) df from pol1;

select intersection('polygon((0 0, 8 0, 8 8, 0 8, 0 0),( 3 4, 4 6, 4 2, 3 4 ))', 'polygon((1 1, 9 1, 9 9, 1 9, 1 1 ))' ) dd;

select union('polygon((0 0, 8 0, 8 8, 0 8, 0 0),( 3 4, 4 6, 4 2, 3 4 ))', 'polygon((1 1, 9 1, 9 9, 1 9, 1 1 ))' ) dd;

select isconvex(pol) from pol1;


select rotateself( s1, 1.80, 'radian' ) from sq1;
select rotateself( s1, 180 ) from sq1;
select rotateat( s1, 1.80, 'radian', 100, 300 ) from sq1;
select scalesize( s1, 10, 20 ) from sq1;

select ls:x, ls:y, ls:m1, ls:m2, ls:m3, ls:m4 from lstrm where a < 10000;
select voronoipolygons(tomultipoint(ls) ) vp from lstrm;
select voronoipolygons(tomultipoint(ls,100) ) vp from lstrm;
select voronoipolygons(tomultipoint(ls),100,bbox(0 80 0.2 80.2) ) vp from lstrm;
select voronoilines(tomultipoint(ls) ) VL from lstrm;
select voronoilines(tomultipoint(ls),100) ) VL from lstrm;
select voronoilines(tomultipoint(ls),100,bbox(0 80 0.2 80.2) ) VL from lstrm;

select delaunaytriangles(tomultipoint(ls) ) dt from lstrm;
select delaunaytriangles(tomultipoint(ls,100) ) dt from lstrm;

select geojson(ls)  from lstrm;
select geojson(ls, 10000)  from lstrm;
select geojson(c1, 10000,300) from cir1;

select tomultipoint(ls) from lstrm;
select tomultipoint(c1, 300) from cir1;

select wkt(ls) from lstrm;
select minimumboundingcircle(ls) from lstrm;
select minimumboundingsphere(pt3) from d5 where a < 1000;

select isonleft(point(30 40), ls) from lstrm;
select leftratio(point(30 40), ls) from lstrm;
select isonright(point(30 40), ls) from lstrm;
select rightratio(point(30 40), ls) from lstrm;
select knn(ls, point(30 40), 10) from lstrm;
select knn(ls, point(30 40), 10, 10, 100) from lstrm;
select metricn( ls, 2 ) from lstrm;
select metricn( ls, 2, 3 ) from lstrm;

drop table if exists cirm;
create table cirm ( key: a int, value: c circle(metrics:2), d int );
insert into cirm values ( 100, circle( 22 33 100 'PARK' 'tower' ), 209 );

select * from cirm;
select c:x, c:y, c:m1, c:m2 from cirm;

select c:x, c:y, c:m1, c:m2 from cirm where a=100;

quit;
