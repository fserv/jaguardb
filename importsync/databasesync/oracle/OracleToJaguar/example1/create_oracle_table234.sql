create table table234
(
    partno number,
	name varchar(32),
	sku  varchar(32),
	manufacture varchar(32),
	buydate  timestamp,
	buyer   varchar(32),
	ptype   varchar(2),
	supplier varchar(32),
	primary key(partno,name )
);
