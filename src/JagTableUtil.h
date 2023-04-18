/*
 * Copyright JaguarDB
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
#ifndef _jag_table_util_h_
#define _jag_table_util_h_

#include <atomic>
#include <abax.h>
#include <JagDef.h>
#include <JagRequest.h>
#include <JagSchemaAttribute.h>
#include <JagParseAttribute.h>
#include <JagMinMax.h>

class JagCfg;
class JagSchemaRecord;
class JaguarCPPClient;
class JagBuffReader;
class JagBuffBackReader;
class JagSingleBuffReader;
class JagDBServer;
class JagTable;
class JagIndex;
class JagParseParam;
class JagMemDiskSortArray;
class JagDiskArrayServer;
class JagDiskArrayFamily;
class JagDataAggregate;
class JagSchemaAttribute;
class JagHashStrInt;
class JagCompFile;

template <class K, class V> class JagHashMap;
template <class Pair> class JagVector;

// for hdr attributes
class SetHdrAttr
{
  public:
    int numKeys;
	Jstr dbobj;
	const JagSchemaRecord *record;
	Jstr sstring;
	
	SetHdrAttr() {
		numKeys = 0;
		record = NULL;
	}
	
	void setattr( int nk, bool ui, Jstr &obj, const JagSchemaRecord *rec ) 
	{
		numKeys = nk;
		dbobj = obj;
		record = rec;
	}
	void setattr( int nk, bool ui, Jstr &obj, const JagSchemaRecord *rec, const char *ss ) 
	{
		numKeys = nk;
		dbobj = obj;
		record = rec;
		sstring = ss;
	}
		
};

// for group by value buffer offset transfer info
class GroupByValueTransfer
{
  public:
	Jstr name;
	jagint objnum;
	jagint offset;
	jagint length;
	
	GroupByValueTransfer() {
		objnum = 0;
		offset = 0;
		length = 0;
	}
};

//for merge reader
class OnefileRange
{
  public:
	JagDiskArrayServer *darr;
	jagint startpos;
	jagint readlen;
	jagint memmax;

	OnefileRange() {
		startpos = 0;
		readlen = 0;
		memmax = -1;
	}	
};

class OnefileRangeFD
{
  public:
	int fd;
	//JagCompFile *compf;
	//JagCompFile *fd;
	jagint startpos;
	jagint readlen;
	jagint memmax;

	OnefileRangeFD() {
		fd = -1;
		//fd = NULL;
		//compf = NULL;
		startpos = 0;
		readlen = 0;
		memmax = -1;
	}	
};

class ParallelCmdPass
{
  public:
	JagTable *ptab;
	JagIndex *pindex;
	int pos;
	jagint sendlen;
	JagParseParam *parseParam;
	JagMemDiskSortArray *gmdarr;
	JagRequest *req;
	JagDataAggregate *jda;
	JagSchemaRecord *nrec;
	JaguarCPPClient *cli;
	Jstr writeName;
	bool nowherecnt;
	std::atomic<jagint> *recordcnt;
	jagint actlimit;
	jagint memlimit;
	char *minbuf;
	char *maxbuf;
	jagint starttime;
	bool timeoutFlag;

	jagint kvlen;
	jagint spos;
	jagint epos;
    bool   useInsertBuffer;

	ParallelCmdPass() {
		ptab = NULL;
		pindex = NULL;
		parseParam = NULL;
		gmdarr = NULL;
		//req->session = NULL;
		jda = NULL;
		nrec = NULL;
		recordcnt = NULL;
		minbuf = NULL;
		maxbuf = NULL;
		cli = NULL;
		sendlen = 1;
		pos = 0;
		nowherecnt = 0;
		actlimit = 0;
		memlimit = 0;
		starttime = 0;
		timeoutFlag = 0;
		kvlen = 0;
		spos = 0;
		epos = 0;
        useInsertBuffer = true;
	}
};

class ParallelJoinPass
{
  public:
	JagTable *ptab;
	JagTable *ptab2;
	JagIndex *pindex;
	JagIndex *pindex2;
	jagint klen;
	jagint klen2;
	jagint vlen;
	jagint vlen2;
	jagint kvlen;
	jagint kvlen2;
	jagint numKeys;
	jagint numKeys2;
	jagint numCols;
	jagint numCols2;
	const JagHashStrInt *maps;
	const JagHashStrInt *maps2;
	const JagSchemaAttribute *attrs;
	const JagSchemaAttribute *attrs2;
	JagDiskArrayFamily *df;
	JagDiskArrayFamily *df2;
	const JagSchemaRecord *rec;
	const JagSchemaRecord *rec2;
	char *minbuf;
	char *minbuf2;
	char *maxbuf;
	char *maxbuf2;
	bool dfSorted;
	bool dfSorted2;
	jagint objelem;
	jagint objelem2;
	jagint jlen;
	jagint jlen2;
	jagint sklen;
	jagint sklen2;
	const JagVector<jagint> *jsvec;
	const JagVector<jagint> *jsvec2;
	jagint tabnum;
	jagint tabnum2;
	jagint pos;
	jagint totlen;
	Jstr jpath;
	Jstr jname;
	JagRequest req;
	JagParseParam *parseParam;
	JagDataAggregate *jda;
	JaguarCPPClient *hcli;
	jagint stime;
	Jstr uhost;
	bool timeout;
	jagint numCPUs;
	JagHashMap<JagFixString, JagFixString> *hmaps;
	
	ParallelJoinPass() {
		ptab = NULL; ptab2 = NULL; pindex = NULL; pindex2 = NULL;
		klen = klen2 = vlen = vlen2 = kvlen = kvlen2 = numKeys = numKeys2 = numCols = numCols2 = 0;
		maps = NULL; maps2 = NULL; attrs = NULL; attrs2 = NULL; df = NULL; df2 = NULL; rec = NULL; rec2 = NULL; 
		minbuf = NULL; minbuf2 = NULL; maxbuf = NULL; maxbuf2 = NULL;
		dfSorted = dfSorted2 = objelem = objelem2 = jlen = jlen2 = sklen = sklen2 = 0;
		jsvec = NULL; jsvec2 = NULL;
		tabnum = tabnum2 = pos = totlen = 0;
		parseParam = NULL; jda = NULL; hcli = NULL;
		stime = timeout = numCPUs = 0;
		hmaps = NULL;
	}
};

class CreateIndexBatchPass
{
  public:
	JagIndex *pindex;
	JaguarCPPClient *tcli;
	Jstr dbobj;
	JagFixString data;
};

class DoIndexPass
{
  public:
	JagDBServer *servobj;
	JagRequest req;
	Jstr dbtable;
	Jstr indexName;
	
	DoIndexPass() {
		// JagSession session;
		// req.session = &session;
		req.session = NULL;
	}
};

// typedef JagBuffReader* AbaxMergePointer;
typedef JagBuffReader* JagBuffReaderPtr;
typedef JagBuffBackReader* JagBuffBackReaderPtr;
typedef JagSingleBuffReader* JagSingleBuffReaderPtr;

#endif
