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

//#include <malloc.h>
#include <JagDiskArrayBase.h>
#include <JagHashLock.h>
#include <JagBuffReader.h>
#include <JagBuffBackReader.h>
#include <JagSingleBuffWriter.h>
#include <JagDiskArrayFamily.h>
#include <JagCompFile.h>

JagDiskArrayBase::JagDiskArrayBase( const JagDBServer *servobj,  JagDiskArrayFamily *fam, const Jstr &filePathName, 
									const JagSchemaRecord *record, int index ) 
								: _schemaRecord(record)
{
	_servobj = servobj;
	_family = fam;
	_pathname = filePathName;
	_fulls = 0;
	_index = index; 
	_partials = 0;
	_reads = _writes = _dupwrites = _upserts = 0;
	_insdircnt = _insmrgcnt = 0;
	_lastSyncTime = 0;
	_lastSyncOneTime = 0;
	_isClient = 0;
	_isFlushing = 0;
	_nthserv = 0;
	_numservs = 1;
	_jdfs = nullptr;
}

JagDiskArrayBase::JagDiskArrayBase( const Jstr &filePathName, const JagSchemaRecord *record ) 
    :  _schemaRecord(record)
{	
	_servobj = NULL;
	_family = NULL;
	_pathname = filePathName;
	_fulls = 0;
	_partials = 0;
	_reads = _writes = _dupwrites = _upserts = 0;
	_insdircnt = _insmrgcnt = 0;
	_lastSyncTime = 0;
	_lastSyncOneTime = 0;
	_isClient = 1;
	_isFlushing = 0;
	_nthserv = 0;
	_numservs = 1;
}

JagDiskArrayBase::~JagDiskArrayBase()
{
	destroy();
}

void JagDiskArrayBase::destroy()
{
	if ( _jdfs ) {
		delete _jdfs;
	}
	_jdfs = NULL;
	
}

void JagDiskArrayBase::_getPair( char buffer[], int keylength, int vallength, JagDBPair &pair, bool keyonly ) const 
{
	if (buffer[0] != '\0') {
		if ( keyonly ) {
			pair = JagDBPair( buffer, keylength );
		} else {
			pair = JagDBPair( buffer, keylength, buffer+keylength, vallength );
		}
	} else {
		JagDBPair temp_pair;
		pair = temp_pair;
	}
}

jagint JagDiskArrayBase::size() const 
{ 
	return _jdfs->getCompf()->size(); 
}

bool JagDiskArrayBase::getFirstLast( const JagDBPair &pair, jagint &first, jagint &last )
{
	if ( *pair.key.c_str() == NBT ) {
		first = 0;
		last = first + JagCfg::_BLOCK - 1;
        dn("s09511 JagDiskArrayBase::getFirstLast pair empty first=0 last=BLOCK-1");
		return 1;
	}

	if ( 0==memcmp(pair.key.c_str(), _maxKey, _KLEN ) ) {
		first = this->size()/_KVLEN;
		last = first + JagCfg::_BLOCK - 1;
        dn("s90521 pair == _maxKey first=%ld last=%ld", first, last );
		return 1;
	}

    dn("s90501 use compf to find getFirstLast");
	JagCompFile *compf = getCompf();

	bool rc;
	rc = compf->findFirstLast( pair, first, last );
	if ( ! rc ) {
        dn("s908001  compf->findFirstLast failed, return false");
		return false;
	}

	last = first + JagCfg::_BLOCK - 1;
    dn("s908881 from compf  first=%ld  last=%ld", first, last );
	return true;
}


bool JagDiskArrayBase::findPred( const JagDBPair &pair, jagint *index, jagint first, jagint last, 
								 JagDBPair &retpair, char *diskbuf )
{
    dn("s90661 JagDiskArrayBase::findPred first=%ld  last=%ld pair:", first, last );
    //pair.printkv(true);

	bool found = 0;
	*index = -1;

    JagDBPair arr[JagCfg::_BLOCK];
    JagFixString key, val;

   	char *ldiskbuf = (char*)jagmalloc(JagCfg::_BLOCK*(_KLEN+1)+1);
   	memset( ldiskbuf, 0, JagCfg::_BLOCK*(_KLEN+1) + 1 );

    /***
    ssize_t sz;
   	for (int i = 0; i < JagCfg::_BLOCK; ++i ) {
   		sz = raypread(_jdfs, ldiskbuf+i*_KLEN, _KLEN, (first+i)*_KVLEN);
        // 2/17/2023
        if ( sz <= 0 ) {
            free( ldiskbuf );
            return false;
        }

       	arr[i].point( ldiskbuf+i*_KLEN, _KLEN );
   	}
    ***/
    ssize_t sz;
    bool bad = false;

   	for (int i = 0; i < JagCfg::_BLOCK; ++i ) {
        sz = 1;
        if ( ! bad ) {
   		    sz = raypread(_jdfs, ldiskbuf+i*_KLEN, _KLEN, (first+i)*_KVLEN);
            // 2/18/2023
        }

        if ( sz <= 0 ) {
            bad = true;
        }

       	arr[i].point( ldiskbuf+i*_KLEN, _KLEN );
    }

   	found = binSearchPred( pair, index, arr, JagCfg::_BLOCK, 0, JagCfg::_BLOCK-1 );

    dn("s90861 binSearchPred found=%d index=%ld  pair:", found, *index );
    //pair.printkv(true);

	char *kvbuf = (char*)jagmalloc(_KVLEN+1);
   	memset( kvbuf, 0, _KVLEN+1 );

   	if ( *index != -1 ) {
		sz = raypread(_jdfs, kvbuf, _KVLEN, (first+*index)*_KVLEN );
        dn("s02000 sz=%ld", sz);
	} else {
		sz = raypread(_jdfs, kvbuf, _KVLEN, first*_KVLEN );
        dn("s2102000 sz=%ld", sz);
	}

	retpair = JagDBPair( kvbuf, _KLEN, kvbuf+_KLEN, _VLEN );

   	*index += first;
    dn("s90542 sz=%ld index=%ld first=%ld found=%d", sz, *index, first, found );

	free( kvbuf );
	free( ldiskbuf );

    dn("s093838881 found=%d debug retpair:", found);
    //retpair.printkv( true );

   	return found;
}

jagint JagDiskArrayBase::getRegionElements( jagint first, jagint length )
{
	JagCompFile *compf = getCompf();
	jagint rangeElements = 0;
	first /= JAG_BLOCK_SIZE;
	for ( jagint i = 0; i < length; ++i ) {
		rangeElements += compf->getPartElements( first+i );
	}
	return rangeElements;
}

Jstr JagDiskArrayBase::jdbPath( const Jstr &jdbhome, const Jstr &db, const Jstr &tab )
{
	Jstr fpath = jdbhome + "/" + db + "/" + tab;
	return fpath;
}

Jstr JagDiskArrayBase::jdbPathName( const Jstr &jdbhome, const Jstr &db, const Jstr &tab )
{
	Jstr fpath = jdbhome + "/" + db + "/" + tab + "/" + tab + ".jdb";
	return fpath;
}

void JagDiskArrayBase::logInfo( jagint t1, jagint t2, jagint cnt, const JagDiskArrayBase *jda )
{
	if ( t1 > 1000 ) { 
		t1 /= 1000;
		jd(JAG_LOG_HIGH, "s6028 flib %s %d wt=%ld s flsh=%ld ms %ld/%ld/%ld/%ld/%ld/%ld\n",
			jda->_dbobj.c_str(), cnt, t1, t2, jda->_writes, jda->_upserts, jda->_dupwrites, 
			jda->_reads, jda->_insmrgcnt, jda->_insdircnt );
	} else if ( t1 > 60000 ) {
		t1 /= 60000;
		jd(JAG_LOG_HIGH, "s6028 flib %s %d wt=%ld m flsh=%ld ms %ld/%ld/%ld/%ld/%ld/%ld\n",
			jda->_dbobj.c_str(), cnt, t1, t2, jda->_writes, jda->_upserts, jda->_dupwrites, 
			jda->_reads, jda->_insmrgcnt, jda->_insdircnt );
	} else if ( t1 > 3600000 ) { 
		t1 /= 3600000;
		jd(JAG_LOG_HIGH, "s6028 flib %s %d wt=%ld h flsh=%ld ms %ld/%ld/%ld/%ld/%ld/%ld\n",
			jda->_dbobj.c_str(), cnt, t1, t2, jda->_writes, jda->_upserts, jda->_dupwrites, 
			jda->_reads, jda->_insmrgcnt, jda->_insdircnt );
	} else if ( t1 > 86400000 ) { 
		t1 /= 86400000;
		jd(JAG_LOG_HIGH, "s6028 flib %s %d wt=%ld d flsh=%ld ms %ld/%ld/%ld/%ld/%ld/%ld\n",
			jda->_dbobj.c_str(), cnt, t1, t2, jda->_writes, jda->_upserts, jda->_dupwrites, 
			jda->_reads, jda->_insmrgcnt, jda->_insdircnt );
	} else if ( t1 > 2592000000 ) { 
		t1 /= 2592000000;
		jd(JAG_LOG_HIGH, "s6028 flib %s %d wt=%ld M flsh=%ld ms %ld/%ld/%ld/%ld/%ld/%ld\n",
			jda->_dbobj.c_str(), cnt, t1, t2, jda->_writes, jda->_upserts, jda->_dupwrites, 
			jda->_reads, jda->_insmrgcnt, jda->_insdircnt );
	} else if ( t1 > 31104000000 ) { 
		t1 /= 31104000000;
		jd(JAG_LOG_HIGH, "s6028 flib %s %d wt=%ld Y flsh=%ld ms %ld/%ld/%ld/%ld/%ld/%ld\n",
			jda->_dbobj.c_str(), cnt, t1, t2, jda->_writes, jda->_upserts, jda->_dupwrites, 
			jda->_reads, jda->_insmrgcnt, jda->_insdircnt );
	} else { 
		jd(JAG_LOG_HIGH, "s6028 flib %s %d wt=%ld ms flsh=%ld ms %ld/%ld/%ld/%ld/%ld/%ld\n",
			jda->_dbobj.c_str(), cnt, t1, t2, jda->_writes, jda->_upserts, jda->_dupwrites, 
			jda->_reads, jda->_insmrgcnt, jda->_insdircnt );
	}
}

jagint JagDiskArrayBase::mergeBufferToFile( const JagDBMap *pairmap, const JagVector<JagMergeSeg> &vec )
{
	jagint bytes = _compf->mergeBufferToFile( pairmap, vec );
	return bytes/_KVLEN;;
}


jagint JagDiskArrayBase::flushBufferToNewFile( const JagDBMap *pairmap )
{
	jagint cnt = _compf->flushBufferToNewSimpFile( pairmap );
	return cnt;
}

bool JagDiskArrayBase::checkSetPairCondition( const JagDBServer *servobj, const JagRequest &req, const JagDBPair &pair, char *buffers[], 
												bool uniqueAndHasValueCol, 
												ExprElementNode *root, const JagParseParam *parseParam, int numKeys, 
												const JagSchemaAttribute *schAttr, 
												jagint KLEN, jagint VLEN,
												jagint setposlist[], JagDBPair &retpair, const JagVector<JagValInt> &auto_update_vec )
{
	bool 			rc, needInit = true;
	ExprElementNode *updroot;
	Jstr 			errmsg;
	JagFixString 	strres;
	int 			idx, typeMode = 0, treelength = 0;
	Jstr 			treetype = " ";
	const JagSchemaAttribute* attrs[1];

	jagint   KVLEN = KLEN + VLEN;
	attrs[0] = schAttr;
	
	memcpy(buffers[0], pair.key.c_str(), KLEN);
	memcpy(buffers[0]+KLEN, pair.value.c_str(), VLEN);

	/***
	char  *tbuf = (char*)jagmalloc(KVLEN+1);
	memset( tbuf, 0, KVLEN+1 );
	memcpy(tbuf, pair.key.c_str(), KLEN);
	memcpy(tbuf+KLEN, pair.value.c_str(), VLEN);
	dbNaturalFormatExchange( tbuf, numKeys, schAttr ); 
	***/

	dbNaturalFormatExchange( buffers[0], numKeys, schAttr ); 

	if ( !uniqueAndHasValueCol || 
		root->checkFuncValid( NULL, NULL, attrs, (const char **)buffers, strres, typeMode, treetype, treelength, needInit, 0, 0 ) == 1 ) {

		char  *tbuf = (char*)jagmalloc(KVLEN+1);
		memset( tbuf, 0, KVLEN+1 );
		memcpy(tbuf, pair.key.c_str(), KLEN);
		memcpy(tbuf+KLEN, pair.value.c_str(), VLEN);
		dbNaturalFormatExchange( tbuf, numKeys, schAttr ); 

		for ( int i = 0; i < parseParam->updSetVec.size(); ++i ) {
			updroot = parseParam->updSetVec[i].tree->getRoot();
			needInit = true;
			if ( updroot->checkFuncValid( NULL, NULL, attrs, (const char **)buffers, strres, typeMode, 
										  treetype, treelength, needInit, 0, 0 ) == 1 ) {

				memset(tbuf+schAttr[setposlist[i]].offset, 0, schAttr[setposlist[i]].length);	

				rc = formatOneCol( req.session->timediff, servobj->servtimediff, tbuf, strres.c_str(), errmsg, 
								   parseParam->updSetVec[i].colName, schAttr[setposlist[i]].offset, 
								   schAttr[setposlist[i]].length, schAttr[setposlist[i]].sig, schAttr[setposlist[i]].type );
				if ( !rc ) {
					free( tbuf );
					return false;
				}
			} else {
				free( tbuf );
				return false;
			}
		}

		// check any time auto update coloumns and formatOneCol on tbuf.
		// need: new value, colname, offset, length, sig, type
		dn("sf012201 check and update any time auto update coloumns");
        for ( int i = 0; i < auto_update_vec.size(); ++i) {
            idx = auto_update_vec[i].idx;
            formatOneCol( req.session->timediff, servobj->servtimediff, tbuf,
                          auto_update_vec[i].val.s(), errmsg, auto_update_vec[i].name,
                         schAttr[idx].offset, schAttr[idx].length, schAttr[idx].sig, schAttr[idx].type );
        }

		dbNaturalFormatExchange( buffers[0], numKeys, schAttr ); 
		dbNaturalFormatExchange( tbuf, numKeys, schAttr ); 

		retpair = JagDBPair( tbuf, KLEN, tbuf+KLEN, VLEN );
		free( tbuf );

	} else {
		/***
		free( tbuf );
		return false;
		***/
	}

	//free( tbuf );
	return true;
}
