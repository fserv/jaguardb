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
#ifndef _jag_row_h_
#define _jag_row_h_

#include <JagDef.h>
#include <abax.h>
#include <JagHashStrStr.h>

class JagParseParam;

class JagRow
{
  public:
    JagRow() { colHash = NULL; }
    ~JagRow() 
    {
		if ( colHash ) delete colHash;
	}

	JagHashStrStr *colHash;
	void ensureHash() 
    {
		if ( colHash ) return;
		colHash = new JagHashStrStr();
	}

	bool     hasSchema;
	Jstr     data;
	char     type;
	char 	 *g_names[JAG_COL_MAX];
	char 	 *g_values[JAG_COL_MAX];
	bool 	 isMeta;
	int		 colCount;
	JagKeyValProp prop[JAG_COL_MAX];
	int      numKeyVals;
    bool     hasRollup;
};


class CliPass
{
  public:
    CliPass() { 
		syncDataCenter = 0; needAll = 0; hasError = 0; 
		isContinue = 1; recvJoinEnd = 0; 
		joinEachCnt = NULL; parseParam = NULL;
		isToGate = 0; 
		noQueryButReply = 0;
		redirectSock = INVALID_SOCKET;
	}
	
	bool hasLimit;
	bool hasError;
	bool needAll;
	bool syncDataCenter;
	bool isToGate;
	bool noQueryButReply;
	int idx;
	int passMode; // 0: ignore reply; 1: print one reply; 2: handshake NG/OK and no reply; 3: handshake NG/OK and one reply
	int selectMode; // 0: update/delete; 1: select point query; 2: select range query; 3: select count(*) query
	int isContinue;
	int recvJoinEnd;
	JAGSOCK redirectSock;
	jagint numHosts;
	jagint start;
	jagint cnt;
	Jstr cmd;
	int cmdlen;
	const char *cmdptr;
	JaguarCPPClient *cli;
	Jstr errmsg;
	JagParseParam *parseParam;
	int *joinEachCnt;
};

class RecoverPass
{
  public:
	RecoverPass() { pcli = NULL; result = 0; readRows = 0; sentRows = 0; }
	Jstr fpath;
	JaguarCPPClient *pcli;
	int result;
    jagint readRows;
    jagint sentRows;
    Jstr   jagHome;
};

#endif
