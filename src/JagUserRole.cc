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
#include <JagGlobalDef.h>

#include <JagUserRole.h>
#include <JagParseParam.h>

// ctor
JagUserRole::JagUserRole( int replicType ):JagFixKV( "system", "UserRole", replicType )
{
}

// dtor
JagUserRole::~JagUserRole()
{
	// this->destroy();
}

Jstr JagUserRole::getListRoles()
{
	return this->getListKeys();
}

// role: "S=1|I=0|U=1|D=1|C=0|R=1|A=1|T=0" or "*=1" can be partial
// role: "S or I or U or D or C or R or A or T"
// if no 1 or 0, regard as 1
// rowfilter: "EQ=abc" or "LT=ajd" "LE=eirir" "GT=kdkfj"  "GE=dkd" "LK=kdkd%" "LK=%jdjf" "CT=kdkffk" "GE=jdddk|LE=4494"
bool JagUserRole::addRole( const AbaxString &userid, const AbaxString& db, const AbaxString& tab, const AbaxString& col,
						const AbaxString& role, const AbaxString &rowfilter )

{
	JagReadWriteMutex mutex( _lock, JagReadWriteMutex::WRITE_LOCK );
	//d("s2183 addRole uid=%s role=[%s]\n", userid.c_str(), role.c_str() );
	// memory hashmap storage
	AbaxString key;
	key = userid + "|" + db + "|" + tab + "|" + col;
	//d("s2283 key=[%s]\n", key.c_str() );

	if ( key.size() > KLEN ) {
		//d("E3641 addRole key=[%s] too long\n", key.c_str() );
		return false;
	}

	// parse role and add 
	AbaxString rowKey;
    char *kv = (char*)jagmalloc(KVLEN+1);
	int rc;
	Jstr lower; 

	Jstr perm;
	JagStrSplit sp( role.c_str(), ',', true );
	for ( int i = 0; i < sp.length(); ++i ) {
		// normalize
    	if ( db == "*" ) {
    		key = userid + "|*|*|*"; 
    	} else if ( tab == "*" ) {
    		key = userid + "|" + db + "|*|*";
    	} else if ( role == JAG_ROLE_INSERT ) {
    		key = userid + "|" + db + "|" + tab + "|*";
    	}

		perm = sp[i];
    	lower = makeLowerString( perm.c_str() );
    	if (  lower == "all" ) {
    		rowKey = key + "|*";
    	} else {
    		rowKey = key + "|" + perm;
    	}
    
    	if ( _hashmap->keyExist( rowKey ) ) {
    		 _hashmap->removeKey( rowKey );
    	}
    
    	rc = _hashmap->addKeyValue( rowKey, rowfilter );
    	//d("s3271 addKeyValue([%s] --> [%s] rc=%d\n", rowKey.c_str(), rowfilter.c_str(), rc );
    
    	// add to file 
       	memset( kv, 0, KVLEN + 1 );
    	strcpy( kv, rowKey.c_str() );
       	strcpy( kv+KLEN, rowfilter.c_str() );
    	JagDBPair dpair( kv, KLEN, kv+KLEN, VLEN );
    	JagDBPair pair( kv, KLEN, kv+KLEN, VLEN );
    
    	if ( _darr->exist( dpair ) ) {
    		_darr->remove( dpair );
    	}
    
       	// rc = _darr->insert( pair, insertCode, false, true, retpair );
       	rc = _darr->insert( pair );
		//d("s4831 darr insert rc=%d pair.key=[%s]\n", rc, pair.key.c_str() );
	}
	
	free( kv );
	return 1;
}

bool JagUserRole::dropRole( const AbaxString &userid, const AbaxString &db, const AbaxString &tab, const AbaxString &col, const AbaxString &ops )
{
	JagReadWriteMutex mutex( _lock, JagReadWriteMutex::WRITE_LOCK );
	// Jstr lower = makeLowerString( op );
	bool rc;
	AbaxString mkey, key;
	Jstr op;
	JagStrSplit sp(ops.c_str(), ',', true );
	for ( int i=0; i < sp.length(); ++i ) {
		op = sp[i];

    	mkey = userid + "|" + db + "|" + tab + "|" + col;
    	key = userid + "|" + db + "|" + tab + "|" + col + "|" + op;
    
    	// normalize
    	if ( db == "*" ) {
    		mkey = userid + "|*|*|*";
    		key = userid + "|*|*|*|" + op; 
    	} else if ( tab == "*" ) {
    		mkey = userid + "|" + db + "|*|*";
    		key = userid + "|" + db + "|*|*|" + op;
    	}
    	//d("s3208 dropRole key=[%s]\n", key.c_str() );
    
    	// _hashmap and _darr remove key
    	rc = dropKey( key, false  );
    	//d("s1024 dropRole rc=%d\n", rc );
    
    	// op is *
    	if ( '*' == *(op.c_str()) ) {
    		// strncasecmp if true, remove
    		_hashmap->removeMatchKey( mkey );
    		_darr->removeMatchKey( mkey.c_str(), mkey.size() );
    	}
    	//d("s1025 dropRole rc=%d\n", rc );
	}

	return rc;
}

// true: OK   false: error
/// n is 0 for single table ops; n=0 n=1 for two table/index join
bool JagUserRole::checkUserCommandPermission( const JagSchemaRecord *srec, const JagRequest &req, 
	const JagParseParam &parseParam, int n, Jstr &rowFilter, Jstr &errmsg )
{
	bool rc = true;
	Jstr oneFilter;
	rowFilter = "";

	Jstr op, db, tab, col;
	if ( JAG_INSERT_OP == parseParam.opcode ) {
		op = JAG_ROLE_INSERT;
	} else if ( JAG_UPDATE_OP == parseParam.opcode ) {
		op = JAG_ROLE_UPDATE;
	} else if ( JAG_DELETE_OP == parseParam.opcode ) {
		op = JAG_ROLE_DELETE;	
	} else if ( JAG_SELECT_OP == parseParam.opcode || JAG_GETFILE_OP == parseParam.opcode || 
				JAG_COUNT_OP == parseParam.opcode || parseParam.isJoin() || JAG_INSERTSELECT_OP == parseParam.opcode ) {
		// check cols for select
		op = JAG_ROLE_SELECT;	
		// d("s3220 select \n" );
	} else if ( JAG_CREATETABLE_OP == parseParam.opcode || JAG_CREATECHAIN_OP == parseParam.opcode ||
				JAG_CREATEMEMTABLE_OP == parseParam.opcode || JAG_CREATEINDEX_OP == parseParam.opcode ) {
		op = JAG_ROLE_CREATE;	
	} else if ( JAG_DROPTABLE_OP == parseParam.opcode || JAG_DROPINDEX_OP == parseParam.opcode ) {
		op = JAG_ROLE_DROP;
	} else if ( JAG_TRUNCATE_OP == parseParam.opcode ) {
		op = JAG_ROLE_TRUNCATE;
	} else if ( JAG_ALTER_OP == parseParam.opcode ) {
		op = JAG_ROLE_ALTER;
	} else {
		errmsg = Jstr("E3207 operation not allowed");
		return false;
	}

	if ( op == JAG_ROLE_UPDATE  ) {
		// check update col permission
		db = parseParam.objectVec[n].dbName;
		tab = parseParam.objectVec[n].tableName;
		//d("s4092  parseParam.updSetVec.size=%d db=[%s] tab=[%s]\n", parseParam.updSetVec.size(), db.c_str(), tab.c_str() );
		for ( int i = 0; i < parseParam.updSetVec.size(); ++i ) {
			col  = parseParam.updSetVec[i].colName.c_str();
			// d("s2029 colList=[%s] is empty\n",  parseParam.updSetVec[i].colList.c_str() );
			rc = isAuthed( op, req.session->uid, db, tab, col, rowFilter );
			//d("s4540 isAuthed rc=%d col=[%s]\n", rc, col.c_str() );
			if ( !rc ) { 
				errmsg = getError("E3201", "update", db, tab, col, req.session->uid );
				return false; 
			}
		}
	} else if ( op == JAG_ROLE_SELECT ) {
		// check select col permission
		/***
		d("s0347 objectVec.size()=%d\n", parseParam.objectVec.size() );
		d("s0348 selColVec.size()=%d\n", parseParam.selColVec.size() );
		d("s4218 selectColumnClause=[%s] \n",  parseParam.selectColumnClause.c_str() );
		***/
		if ( JAG_COUNT_OP == parseParam.opcode && srec  ) {
			db = parseParam.objectVec[n].dbName;
			tab = parseParam.objectVec[n].tableName;
    		for ( int j = 0; j < srec->columnVector->size(); ++j ) {
				col = (*srec->columnVector)[j].name.c_str();
				if ( col == "spare_" ) continue;
    			rc = isAuthed( op, req.session->uid, db, tab, col, oneFilter );
				// is any col has select perm, then it is OK
				if ( rc ) return true;
				// if ( oneFilter.size() > 0 ) { rowFilter += oneFilter + "|"; }
			}
    		errmsg = getError("E3102", "select", db, tab, "all columns", req.session->uid );
    		return false;
		} else if ( parseParam.selectColumnClause == "*" && srec ) {
			// get all coumns from schema record
			db = parseParam.objectVec[n].dbName;
			tab = parseParam.objectVec[n].tableName;
    		for ( int j = 0; j < srec->columnVector->size(); ++j ) {
				col = (*srec->columnVector)[j].name.c_str();
    			rc = isAuthed( op, req.session->uid, db, tab, col, oneFilter );
				/***
    			d("s3504 isAuthed rc=%d db=[%s] tab=[%s] col=[%s] oneFilter=[%s]\n", 
						rc, db.c_str(), tab.c_str(), col.c_str(), oneFilter.c_str() );
						***/
    			if ( !rc ) { 
    				errmsg = getError("E3202", "select", db, tab, col, req.session->uid );
    				return false;
    			}

				if ( oneFilter.size() > 0 ) { rowFilter += oneFilter + "|"; }
			}

		} else {
    		for ( int i = 0; i < parseParam.selColVec.size(); ++i ) {
    			// d("s3283 parseParam.selColVec[i].colList=[%s]\n", parseParam.selColVec[i].colList.c_str() );
    			JagStrSplit sp( parseParam.selColVec[i].colList, '|', true );
    			for ( int j = 0; j < sp.length(); ++j ) {
    				JagStrSplit sp2( sp[j], '.' );
    				if ( sp2.length() < 1 ) continue;
    				getDbTabCol( parseParam, i, sp2, db, tab, col );
    				rc = isAuthed( op, req.session->uid, db, tab, col, oneFilter );
    				// d("s4504 isAuthed rc=%d oneFilter=[%s]\n", rc, oneFilter.c_str() );
    				if ( !rc ) { 
    					errmsg = getError("E13202", "select", db, tab, col, req.session->uid );
    					return false;
    				}

					if ( oneFilter.size() > 0 ) { rowFilter += oneFilter + "|"; }
    			}
    		}
		}

		// join
		// d("s2439 joinOnVecn.size=%d\n", parseParam.joinOnVec.size() );
		for ( int i = 0; i < parseParam.joinOnVec.size(); ++i ) {
			//d("s3024 join joinOnVec[i].colList=[%s]\n", parseParam.joinOnVec[i].colList.c_str() );
			JagStrSplit sp( parseParam.joinOnVec[i].colList, '|', true );
			for ( int j = 0; j < sp.length(); ++j ) {
				JagStrSplit sp2( sp[j], '.' );
				if ( sp2.length() < 1 ) continue;
				getDbTabCol( parseParam, i, sp2, db, tab, col );
				rc = isAuthed( op, req.session->uid, db, tab, col, oneFilter );
				//d("s4504 isAuthed rc=%d oneFilter=[%s]\n", rc, oneFilter.c_str() );
				if ( !rc ) {
					errmsg = getError("E3203", "join", db, tab, col, req.session->uid );
					return false;
				}

				if ( oneFilter.size() > 0 ) { rowFilter += oneFilter + "|"; }
			}
		}
	} else {
		// insert /alter/delete etc
		//d("s2239 other command  parseParam.objectVec.size()=%d\n",  parseParam.objectVec.size() );
		for ( int i = 0; i < parseParam.objectVec.size(); ++i ) {
			db = parseParam.objectVec[i].dbName;
			tab = parseParam.objectVec[i].tableName;
			col = "*";
			rc = isAuthed( op, req.session->uid, db, tab, col, rowFilter );
			//d("s4505 isAuthed rc=%d db=[%s] tab=[%s]\n", rc, db.c_str(), tab.c_str() );
			if ( !rc ) {
				errmsg = getError("E3204", "", db, tab, col, req.session->uid );
				return false;
			}
		}
	}


	// finally check where part permission
	//d("s3584 whereVec.size()=%d\n", parseParam.whereVec.size() );
    db = parseParam.objectVec[0].dbName;
    tab = parseParam.objectVec[0].tableName;
	for ( int i = 0; i < parseParam.whereVec.size(); ++i ) {
		//d("s3055 whereVec[i].colLis=[%s]\n", parseParam.whereVec[i].colList.c_str() );
		JagStrSplit sp( parseParam.whereVec[i].colList, '|', true );
		for ( int j = 0; j < sp.length(); ++j ) {
			JagStrSplit sp2( sp[j], '.' );
			if ( sp2.length() < 1 ) continue;
        	if ( sp2.length() == 1 ) {
        		col = sp2[0];
        	} else if ( sp2.length() == 2 ) {
        		tab = sp2[0];
        		col = sp2[1];
        	} else if ( sp2.length() == 3 ) {
        		db = sp2[0];
        		tab = sp2[1];
        		col = sp2[2];
        	} else if ( sp2.length() == 4 ) {
        		db = sp2[0];
        		tab = sp2[1];
        		col = sp2[3];
        	}
			rc = isAuthed( op, req.session->uid, db, tab, col, oneFilter );
			//d("s4548 isAuthed rc=%d\n", rc );
			if ( !rc ) {
				errmsg = getError("E13239", "where", db, tab, col, req.session->uid );
				return false;
			}
			if ( oneFilter.size() > 0 ) { rowFilter += oneFilter + "|"; }
		}
	}

	//d("s4508 final isAuthed rc=true\n" );
	return true;
}

// consider op and all cases
bool JagUserRole::isAuthed( const Jstr &op, const Jstr &userid,  	
						  const Jstr &db, const Jstr &tab, 
						  const Jstr &col, Jstr &rowFilter )
{
	rowFilter = "";
	if ( col == "spare_" ) return true;

	AbaxString rowKey = userid + "|" + db + "|" + tab + "|" + col + "|" + op;
	AbaxString rowKey2 = userid + "|" + db + "|" + tab + "|" + col + "|*";
	JagReadWriteMutex mutex( _lock, JagReadWriteMutex::READ_LOCK );
	AbaxString value;
	const char *p;
	if ( _hashmap->getValue( rowKey, value ) ) {
		p = value.c_str();
		if ( '\0' == *p ) { rowFilter = ""; } else { rowFilter = p; }
		return true;
	}
	//d("s4003 check rowKey=[%s]\n", rowKey.c_str() );
	//d("s4003 check rowKey2=[%s]\n", rowKey2.c_str() );
	if ( _hashmap->getValue( rowKey2, value ) ) {
		p = value.c_str();
		if ( '\0' == *p ) { rowFilter = ""; } else { rowFilter = p; }
		return true;
	}

	// all cols
	rowKey = userid + "|" + db + "|" + tab + "|*|" + op;
	rowKey2 = userid + "|" + db + "|" + tab + "|*|*";
	//d("s4004 check rowKey=[%s]\n", rowKey.c_str() );
	//d("s4004 check rowKey2=[%s]\n", rowKey2.c_str() );
	if ( _hashmap->getValue( rowKey, value ) ) {
		p = value.c_str();
		if ( '\0' == *p ) { rowFilter = ""; } else { rowFilter = p; }
		return true;
	}
	if ( _hashmap->getValue( rowKey2, value ) ) {
		p = value.c_str();
		if ( '\0' == *p ) { rowFilter = ""; } else { rowFilter = p; }
		return true;
	}

	// all tables
	rowKey = userid + "|" + db + "|*|*|" + op;
	rowKey2 = userid + "|" + db + "|*|*|*";
	//d("s4005 check rowKey=[%s]\n", rowKey.c_str() );
	//d("s4005 check rowKey2=[%s]\n", rowKey2.c_str() );
	if ( _hashmap->getValue( rowKey, value ) ) {
		p = value.c_str();
		if ( '\0' == *p ) { rowFilter = ""; } else { rowFilter = p; }
		return true;
	}
	if ( _hashmap->getValue( rowKey2, value ) ) {
		p = value.c_str();
		if ( '\0' == *p ) { rowFilter = ""; } else { rowFilter = p; }
		return true;
	}

	// all dbs
	rowKey = userid + "|*|*|*|" + op;
	rowKey2 = userid + "|*|*|*|*";
	//d("s4006 check rowKey=[%s]\n", rowKey.c_str() );
	//d("s4006 check rowKey2=[%s]\n", rowKey2.c_str() );
	if ( _hashmap->getValue( rowKey, value ) ) {
		p = value.c_str();
		if ( '\0' == *p ) { rowFilter = ""; } else { rowFilter = p; }
		return true;
	}
	if ( _hashmap->getValue( rowKey2, value ) ) {
		p = value.c_str();
		if ( '\0' == *p ) { rowFilter = ""; } else { rowFilter = p; }
		return true;
	}

	rowFilter = "";
	return false;
}

Jstr JagUserRole::showRole( const AbaxString &uid )
{
	AbaxString perm;
	JagReadWriteMutex mutex( _lock, JagReadWriteMutex::READ_LOCK );

	AbaxString matchKey = uid + "|";
    const AbaxPair<AbaxString, AbaxString> *arr = _hashmap->array();
    jagint len = _hashmap->arrayLength();
	Jstr db, tab, col, pm, permStr;

	for ( jagint i = 0; i < len; ++i ) {
		if ( _hashmap->isNull(i) ) continue;
		const AbaxPair<AbaxString, AbaxString> &kv = arr[i];
		if ( 0 == strncmp(kv.key.c_str(), matchKey.c_str(), matchKey.size() ) ) {
			JagStrSplit sp(kv.key.c_str(), '|');
			// d("s4127 showRole kv.key=[%s]\n", kv.key.c_str() );
			if ( sp.length() < 5 ) continue;
			db = sp[1];
			tab = sp[2];
			col = sp[3];
			pm = sp[4];
			permStr = convertToStr( pm );

			perm += AbaxString("database:") + db + " table:" + tab + " column:" + col + " grant:" + permStr;
			if ( kv.value.size() > 0 ) {
				perm += AbaxString(" where: ") + kv.value.c_str();
			}
			perm += "\n";
		}
	}

	return perm.c_str();
}

void JagUserRole::getDbTabCol( const JagParseParam &parseParam, int i, const JagStrSplit &sp2, 
								Jstr &db, Jstr &tab, Jstr &col )
{
	if ( sp2.length() == 1 ) {
		db = parseParam.objectVec[i].dbName;
		tab = parseParam.objectVec[i].tableName;
		col = sp2[0];
	} else if ( sp2.length() == 2 ) {
		db = parseParam.objectVec[i].dbName;
		tab = sp2[0];
		col = sp2[1];
	} else if ( sp2.length() == 3 ) {
		db = sp2[0];
		tab = sp2[1];
		col = sp2[2];
	} else if ( sp2.length() == 4 ) {
		db = sp2[0];
		tab = sp2[1];
		col = sp2[3];
	}
}



Jstr JagUserRole::getError( const Jstr &code, const Jstr &action, 
				const Jstr &db, const Jstr &tab, const Jstr &col, const Jstr &uid )
{
	Jstr errmsg = code + " ";
	errmsg += Jstr("database:") + db + " ";
	errmsg += Jstr("table:") + tab + " ";
	errmsg += Jstr("column:") + col + " ";
	if ( action.size() > 0 ) {
		errmsg += Jstr(" not allowed for ") + action + " by " + uid;
	} else {
		errmsg += Jstr(" operation not allowed by ") + uid;
	}
	return errmsg;
}
