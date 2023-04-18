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
#include <values.h>

#include <JagDiskArrayFamily.h>
#include <JagDiskArrayServer.h>
#include <JagSingleBuffReader.h>
#include <JagFileMgr.h>
#include <JagFamilyKeyChecker.h>
#include <JagDiskKeyChecker.h>
#include <JagFixKeyChecker.h>
#include <JagDBMap.h>
#include <JagCompFile.h>

JagDiskArrayFamily::JagDiskArrayFamily( int objType,  const JagDBServer *servobj, const Jstr &filePathName, 
                                        const JagSchemaRecord *record, 
									    jagint length, bool buildInitIndex ) : _schemaRecord(record)
{
	dn("s100439 JagDiskArrayFamily ctor _pathname=filePathName=[%s]", filePathName.c_str() );

    _objType = objType; // 1: table; 2: index
	_insdelcnt = 0;
	_KLEN = record->keyLength;
	_VLEN = record->valueLength;
	_KVLEN = _KLEN + _VLEN;
	_servobj = (JagDBServer*)servobj;
	_pathname = filePathName;

    /***
     table _pathname:  jagdatahome + "/" + _dbname + "/" + _tableName + "/" + _tableName;
            db/jbench/jbench.0.jdb/{0,166...}

     index _pathname:  jagdatahome + "/" + _dbname + "/" + _tableName + "/" + _tableName + "." + _indexName;
            db/jbench/jbench.idx3/jbench.0.jdb/{0,166...}
    **/


    dn("s66226  _keyChecker   _KLEN=%d _VLEN=%d",  _KLEN, _VLEN );
	if ( JAG_MEM_LOW == servobj->_memoryMode ) {
        dn("s3007 JAG_MEM_LOW JagDiskKeyChecker() _pathname=[%s]", _pathname.s());
		_keyChecker = new JagDiskKeyChecker( _pathname, _KLEN, _VLEN );
	} else {
        dn("s3007 JAG_MEM_HIGH JagFixKeyChecker() _pathname=[%s]", _pathname.s() );
		_keyChecker = new JagFixKeyChecker( _pathname, _KLEN, _VLEN );
	}

	Jstr existFiles, fullpath, objname;
	const char *p = strrchr( filePathName.c_str(), '/' );
	if ( p == NULL ) {
		i("s7482 error _pathname=%s, exit\n", _pathname.c_str() );
		exit(34);
	}

    fullpath = Jstr( filePathName.c_str(), p-filePathName.c_str() );
    // everything before the last /:  /x/c/v/ddddd/last  --> /x/c/v/ddddd

    _insertBufferMap = new JagDBMap();

	_tablepath = fullpath; 
	_sfilepath = fullpath + "/files";

    dn("s300299 _tablepath=[%s] _sfilepath=[%s]", _tablepath.s(),  _sfilepath.s() );

	JagStrSplit sp(_pathname, '/');
	int splen = sp.length();
	_taboridxname = sp[splen-1];
	_dbname = sp[splen-3];

	objname = _taboridxname + ".jdb";
	_objname = objname;
    // _objname=jbench3.jdb jbench3.jidx3.jdb

    dn("s002282 _dbname=[%s] _taboridxname=[%s] _objname=[%s]", _dbname.s(), _taboridxname.s(), _objname.s() );

	existFiles = JagFileMgr::getFileFamily( _objType, fullpath, objname );
    dn("s872737 objtype=%d _objname=%s getFileFamily fullpath=[%s] objname=[%s]  existFiles=[%s]", 
        _objType, _objname.s(), fullpath.s(), objname.s(), existFiles.s() );

	int kcrc = 0;

	if ( existFiles.size() > 0 ) {	
        dn("s92727 buildInitKeyCheckerFromSigFile ...");
		kcrc = _keyChecker->buildInitKeyCheckerFromSigFile();

		JagStrSplit split( existFiles, '|' );
		JagVector<Jstr> paths( split.length() );
		int filenum;

		for ( int i = 0; i < split.length(); ++i ) {
			const char *ss = strrchr( split[i].c_str(), '/' );
			if ( !ss ) ss = split[i].c_str();
			JagStrSplit split2( ss, '.' );
			if ( split2.length() < 2 ) {
				jd(JAG_LOG_LOW, "s74182 fatal error _pathname=%s, exit\n", split[i].c_str() );
				exit(35);
			}

			filenum = atoi( split2[split2.length()-2].c_str() );
            dn("s80023 filenum=%d split.length=%d", filenum,  split.length() );
			if ( filenum >= split.length() ) {
				jd(JAG_LOG_LOW, "s74183 fatal error family has %d files, but %d %s >= max files\n", 
							split.length(), filenum, split[i].c_str() );
				exit(36);
			}

			const char *q = strrchr( split[i].c_str(), '.' );
			paths[filenum] = Jstr(split[i].c_str(), q-split[i].c_str());
            dn("s8333 i=%d filenum=%d paths[filenum]=[%s]", i, filenum,  paths[filenum].s() );
			paths._elements++;
		}

		jagint cnt;
		for ( int i = 0; i < paths.size(); ++i ) {
            dn("s9228 new JagDiskArrayServer obj path=[%s]", paths[i].s() );
			JagDiskArrayServer *darr = new JagDiskArrayServer( servobj, this, i, paths[i], record, length, buildInitIndex );
			_darrlist.append(darr);
		}

		bool  readJDB = false;
		if ( 0 == kcrc ) {
            /***
            if ( _objType == JAG_TABLE ) {
			    readJDB = true;
            } else {
                dn("s72727 index needs not add keychecker");
            }
            ***/
			readJDB = true;
			jd(JAG_LOG_LOW, "s6281 skip sig or hdb, need to readJDB\n");
		} else {
		}

		if ( readJDB ) {
            dn("s81110 readJDB true, addKeyCheckerFromJDB ...");
			for ( int i = 0; i < _darrlist.size(); ++i ) {
				cnt = addKeyCheckerFromJDB( _darrlist[i], i );
				jd(JAG_LOG_LOW, "s3736 obj=%s %d/%d readjdb cnt=%ld\n", _objname.c_str(), i,  _darrlist.size(), cnt );
			}
		} else {
            dn("s81110 readJDB false, no addKeyCheckerFromJDB");
        }
	}

	_isFlushing = 0;
	_doForceFlush = false;

    dn("s900817 %s _keyChecker.size=%ld", _pathname.s(), _keyChecker->size() );
}

jagint JagDiskArrayFamily::addKeyCheckerFromJDB( JagDiskArrayServer *ldarr, int activepos )
{
    dn("s2376001 _objType=%d addKeyCheckerFromJDB ldarr=%p", _objType, ldarr);
    dn("s2376001  ldarr->_dirPath=[%s]", ldarr->_dirPath.s() );
    dn("s2376001  ldarr->_filePath=[%s]", ldarr->_filePath.s() );
    dn("s2376001  ldarr->_jdfs=[%p]", ldarr->_jdfs );
    dn("s2376001  ldarr->_compf=[%p]", ldarr->_compf );
    dn("s2376001  ldarr->_family=[%p]==this=%p", ldarr->_family, this );
    dn("s2376001  ldarr->_elements=[%ld]", (long)(ldarr->_elements) );
    dn("s2376001  ldarr->_pdbobj=[%s]", ldarr->_pdbobj.s() );
    dn("s2376001  ldarr->_dbobj=[%s]", ldarr->_dbobj.s() );
    dn("s2376001  ldarr->_dbname=[%s]", ldarr->_dbname.s() );
    dn("s2376001  ldarr->_objname=[%s]", ldarr->_objname.s() );
    dn("s272838 activepos=%d", activepos );

	char vbuf[3]; 
    vbuf[2] = '\0';
	int div, rem;

 	char *kvbuf = (char*)jagmalloc( _KVLEN+1 + JAG_KEYCHECKER_VLEN);
	memset( kvbuf, 0,  _KVLEN + 1 + JAG_KEYCHECKER_VLEN );

	jagint rlimit = getBuffReaderWriterMemorySize( ldarr->_arrlen*_KVLEN/1024/1024 );

	div = activepos / (JAG_BYTE_MAX+1);
	rem = activepos % (JAG_BYTE_MAX+1);
	vbuf[0] = div; 
	vbuf[1] = rem;

    dn("s80026 JagSingleBuffReader ldarr->_arrlen=%ld _KLEN=%d _VLEN=%d rlimit=%ld", ldarr->_arrlen, _KLEN, _VLEN, rlimit );
    JagSingleBuffReader nav( ldarr->_compf, ldarr->_arrlen, _KLEN, _VLEN, 0, 0, rlimit );

	jagint cnt = 0;
	jagint cntDone = 0;

    while( nav.getNext( kvbuf ) ) {
		memcpy(kvbuf+_KLEN, vbuf, 2 );
		if ( _keyChecker->addKeyValueNoLock( kvbuf ) ) {
			++cntDone;
		}

        ++cnt;
    }

    dn("s202928 getNext all done cnt=%ld cntDone=%ld", cnt, cntDone );
	free( kvbuf );
	return cnt;
}

jagint JagDiskArrayFamily::addKeyCheckerFromInsertBuffer( int darrNum )
{
	char vbuf[3]; 
	int div, rem;

    vbuf[2] = '\0';

 	char *kvbuf = (char*)jagmalloc( _KLEN+1 + JAG_KEYCHECKER_VLEN);
				
	div = darrNum / (JAG_BYTE_MAX+1);
	rem = darrNum % (JAG_BYTE_MAX+1);
	vbuf[0] = div; 
	vbuf[1] = rem;

	jagint cntDone = 0;
	JagFixMapIterator iter = _insertBufferMap->_map->begin();

	while ( iter != _insertBufferMap->_map->end() ) {
		memset( kvbuf, 0,  _KLEN + 1 + JAG_KEYCHECKER_VLEN );
		memcpy( kvbuf, iter->first.c_str(), iter->first.size() );
		memcpy(kvbuf+_KLEN, vbuf, JAG_KEYCHECKER_VLEN );

        //dn("s58013 objType=%d  _keyChecker->addKeyValueNoLock(%s)", _objType, kvbuf );
		if ( _keyChecker->addKeyValueNoLock( kvbuf ) ) {
			++cntDone;
		}
		++ iter;
	}

	free( kvbuf );
	return cntDone;
}

JagDiskArrayFamily::~JagDiskArrayFamily()
{
	if ( _keyChecker ) {
		delete _keyChecker;
		_keyChecker = NULL;
	}

	for ( int i = 0; i < _darrlist.size(); ++i ) {
		if ( _darrlist[i] ) {
			delete _darrlist[i];
			_darrlist[i] = NULL;
		}
	}

    if ( _insertBufferMap ) { 
		delete _insertBufferMap; 
		_insertBufferMap=NULL; 
	}
}

jagint JagDiskArrayFamily::flushInsertBuffer( bool &hasFlushed ) 
{
	jagint cnt = 0; 
	bool doneFlush = false;

	if ( _darrlist.size() < 1 ) {
		doneFlush = false;  // new darr will be added, see below
        dn("s72335 _darrlist.size() < 1 doneFlush set to false");
	} else {
		JagVector<JagMergeSeg> vec;
		int mtype;
		int which = findMinCostFile( vec, _doForceFlush, mtype ); 
        dn("s77008 findMinCostFile which=%d mtype=%d", which, mtype );
		if ( which < 0 ) {
			jd(JAG_LOG_LOW, "s2647 min-cost file not found, _darrlist.size=%d\n", _darrlist.size() );
		} else {
			if (  mtype == JAG_MEET_TIME ) {  
                dn("s71145 JAG_MEET_TIME mergeBufferToFile vec.size=%d ...", vec.size() );
				cnt = _darrlist[which]->mergeBufferToFile( _insertBufferMap, vec );	
				doneFlush = true;
                dn("s71146 JAG_MEET_TIME mergeBufferToFile done cnt=%d", cnt);

                dn("s6262800 addKeyCheckerFromInsertBuffer whhich=%d", which );
				jagint kn = addKeyCheckerFromInsertBuffer( which );
                dn("s6262800 addKeyCheckerFromInsertBuffer whhich=%d done kn=%ld", which, kn );
			} else {
                dn("s71125 not JAG_MEET_TIME");
			}
		}
	}

    dn("s09112 doneFlush=%d", doneFlush );

	if ( ! doneFlush ) {
        dn("s51610 not doneFlush, flushBufferToNewFile()...");
		JagDiskArrayServer *extraDarr = flushBufferToNewFile();
        dn("s51610 not doneFlush, flushBufferToNewFile() extraDarr=%p", extraDarr);

		if ( extraDarr != nullptr ) {

			_darrlist.append( extraDarr );

			int which = _darrlist.size() - 1; 
			cnt = extraDarr->size()/_KVLEN;

            dn("s66543 addKeyCheckerFromInsertBuffer which=%d/darrlist.size=%ld cnt=%d", which,  _darrlist.size(), cnt );
			jagint kn = addKeyCheckerFromInsertBuffer( which );
            dn("s66543 addKeyCheckerFromInsertBuffer which=%d/darrlist.size=%ld cnt=%d kn=%ld", which,  _darrlist.size(), cnt, kn );

			doneFlush = true;
		} else {
			jd(JAG_LOG_LOW, "s20345 Error flushBufferToNewFile()\n" ); 
		}
	}

    hasFlushed = false;
	if ( doneFlush ) {
		jd(JAG_LOG_LOW, "cleanup insertbuffer size=%lld\n", _insertBufferMap->size() ); 
		_insertBufferMap->clear();
		jd(JAG_LOG_LOW, "after cleanup insertbuffer size=%lld\n", _insertBufferMap->size() ); 

        hasFlushed = true;
        // notify table to flush index buffers

		removeAndReopenWalLog();
	 	jagmalloc_trim(0);
	}

	return cnt;
}

int JagDiskArrayFamily::findMinCostFile( JagVector<JagMergeSeg> &vec, bool forceFlush, int &mtype )
{
	jagint sequentialReadSpeed = _servobj->_cfg->getLongValue("SEQ_READ_SPEED", 200);
	jagint sequentialWriteSpeed = _servobj->_cfg->getLongValue("SEQ_WRITE_SPEED", 150);
	float  allowedFlushTimeLimit  = _servobj->_cfg->getFloatValue("MAX_FLUSH_TIME", 1.0); 

	float spendSeconds = 0;
	float minTime = FLT_MAX;
	int minDarr = -1;
	JagVector<JagMergeSeg> oneVec;

    dn("s450091 findMinCostFile _darrlist.size=%d", _darrlist.size() );

	for ( int i = 0 ; i < _darrlist.size(); ++i ) {
		oneVec.clear();
		spendSeconds = _darrlist[i]->computeMergeCost( _insertBufferMap, sequentialReadSpeed, sequentialWriteSpeed, oneVec );	
        dn("s5000278 i=%d computeMergeCost spendSeconds=%f oneVec.size=%d", i, spendSeconds, oneVec.size() );
		if ( spendSeconds < minTime ) {
			minTime = spendSeconds;
			minDarr = i;
			vec = oneVec;
		}
	}

	if ( forceFlush ) {
		mtype = JAG_MEET_TIME;
		return minDarr;
	}

	if ( minTime <= allowedFlushTimeLimit ) {
		mtype = JAG_MEET_TIME;
		return minDarr;
	} else {
		mtype = 0;
		return -1;
	}
}

int JagDiskArrayFamily::insert( const JagDBPair &pair, bool &hasFlushed )
{
    hasFlushed = false;
    if ( _objType == JAG_TABLE ) {
        return insertTable( pair, hasFlushed);
    } else {
        return insertIndex( pair );
    }
}

int JagDiskArrayFamily::insertTable( const JagDBPair &pair, bool &hasFlushed )
{
    dn("s237702 _objname=[%s] JagDiskArrayFamily::insert key=[%s] ...", _objname.s(), pair.key.c_str() );

	if ( _insertBufferMap->exist( pair ) ) {
		d("s2038271 _pathname=[%s] pair exists\n", _pathname.s() );
		return 0;
	}

	char kbuf[_KLEN+1];
	memset( kbuf, 0, _KLEN+1);
	memcpy( kbuf, pair.key.c_str(), _KLEN );

	if ( _keyChecker && _keyChecker->exist( kbuf ) ) {
		d("s49802 insert exist true, return 0\n");
		return 0;
	}

    int rc;
    rc = _insertBufferMap->insert( pair );

    if ( !rc ) ++_dupwrites;
	d("s22029 fam-this=%p  _insertBufferMap->insert rc=%d elem=%d _insertBufferMap=%p\n", this, rc, _insertBufferMap->elements(), _insertBufferMap );

	jagint  currentCnt = _insertBufferMap->elements();
	jagint  currentMem = currentCnt *_KVLEN;
    
	if ( currentMem < JAG_SIMPFILE_LIMIT_BYTES ) {
        dn("s615102 buffer too small, data stay in buffer");
	    return 1; // done put to buffer
	}

    dn("s20208 objname=[%s] flushInsertBuffer()...", _objname.s() );

    jagint cnt = flushInsertBuffer( hasFlushed );
    dn("s20208 objname=[%s] flushInsertBuffer() done cnt=%ld hasFlushed=%d", _objname.s(), cnt, hasFlushed );

    return 1;
}

int JagDiskArrayFamily::insertIndex( const JagDBPair &pair )
{
    dn("s237702 _objname=[%s] JagDiskArrayFamily::insertIndex key=[%s] ...", _objname.s(), pair.key.c_str() );

	if ( _insertBufferMap->exist( pair ) ) {
		d("s2038271 _pathname=[%s] pair exists\n", _pathname.s() );
		return 0;
	}

	char kbuf[_KLEN+1];
	memset( kbuf, 0, _KLEN+1);
	memcpy( kbuf, pair.key.c_str(), _KLEN );

	if ( _keyChecker && _keyChecker->exist( kbuf ) ) {
		d("s49802 insert exist true, return 0\n");
		return 0;
	}

    int rc;
    rc = _insertBufferMap->insert( pair );

    if ( !rc ) ++_dupwrites;
	d("s22029 _insertBufferMap->insert rc=%d elem=%d\n", rc, _insertBufferMap->elements() );

    return 1;
}

jagint JagDiskArrayFamily::getCount( )
{
	jagint mem = 0;
	jagint kchn = 0;

	if ( _insertBufferMap && _insertBufferMap->size() > 0 ) {
		mem = _insertBufferMap->size();
	}

	kchn = _keyChecker->size();

    dn("s07372 fam-this=%p getCount  memcnt=%ld kchkrn=%ld _objType=%d _insertBufferMap=%p", this, mem, kchn, _objType, _insertBufferMap );

	return mem + kchn;
}

jagint JagDiskArrayFamily::getElements( )
{
	return getCount();
}

bool JagDiskArrayFamily::isFlushing()
{
	return _isFlushing;
}

bool JagDiskArrayFamily::remove( const JagDBPair &pair )
{
	int rc = 0;
	int pos = 0, div, rem;
	char v[2];

	if ( _insertBufferMap && _insertBufferMap->remove( pair ) ) {
		return true;
	}

	char kbuf[_KLEN+1];
	memset( kbuf, 0, _KLEN+1);
	memcpy( kbuf, pair.key.c_str(), _KLEN );

	if ( _keyChecker->getValue( kbuf, v ) ) {
		div = (jagbyte)v[0];
		rem = (jagbyte)v[1];
		pos = div*(JAG_BYTE_MAX+1)+rem;

		rc = _darrlist[pos]->remove( pair );

		if ( rc ) {
			rc = _keyChecker->removeKey( pair.key.c_str() );
		}
		return rc;
	} else {
		return 0;
	}
}

bool JagDiskArrayFamily::exist( JagDBPair &pair )
{
	if ( _insertBufferMap && _insertBufferMap->get( pair ) ) {
		return true;
	}

	int rc = 0, pos = 0;
	int div, rem;
	char v[2];

	char kbuf[_KLEN+1];
	memset( kbuf, 0, _KLEN+1);
	memcpy( kbuf, pair.key.c_str(), _KLEN );

	if ( _keyChecker->getValue( kbuf, v ) ) {
		div = (jagbyte)v[0];
		rem = (jagbyte)v[1];
		pos = div*(JAG_BYTE_MAX+1)+rem;
		rc = _darrlist[pos]->exist( pair );
		return rc;
	} else {
		return 0;
	}
}

bool JagDiskArrayFamily::get( JagDBPair &pair )
{
	if ( _insertBufferMap && _insertBufferMap->get( pair ) ) {
		return true;
	}

	int rc = 0, pos = 0;
	int div, rem;
	char v[2];

	char kbuf[_KLEN+1];
	memset( kbuf, 0, _KLEN+1);
	memcpy( kbuf, pair.key.c_str(), _KLEN );

	if ( _keyChecker->getValue( kbuf, v ) ) {
		div = (jagbyte)v[0];
		rem = (jagbyte)v[1];
		pos = div*(JAG_BYTE_MAX+1)+rem;

		rc = _darrlist[pos]->get( pair );

		return rc;
	} else {
		return 0;
	}
}

bool JagDiskArrayFamily::set( const JagDBPair &pair )
{
	if ( _insertBufferMap && _insertBufferMap->set( pair ) ) {
		return true;
	}

	int rc = 0, pos = 0;
	int div, rem;
	char v[2];
	char kbuf[_KLEN+1];
	memset( kbuf, 0, _KLEN+1);
	memcpy( kbuf, pair.key.c_str(), _KLEN );
	if ( _keyChecker->getValue( kbuf, v ) ) {
		div = (jagbyte)v[0];
		rem = (jagbyte)v[1];
		pos = div*(JAG_BYTE_MAX+1)+rem;

		rc = _darrlist[pos]->set( pair );

		return rc;
	} else {
		return 0;
	}
}

bool JagDiskArrayFamily::setWithRange( const JagRequest &req, JagDBPair &pair, const char *buffers[], bool uniqueAndHasValueCol, 
										ExprElementNode *root, const JagParseParam *pParam, int numKeys, const JagSchemaAttribute *schAttr, 
										jagint setposlist[], JagDBPair &retpair, const JagVector<JagValInt> &avec )
{
	bool rc = get(pair); 
	if ( ! rc ) {
		return false;
	}

    rc = JagDiskArrayBase::checkSetPairCondition( _servobj, req, pair, (char**)buffers, uniqueAndHasValueCol, root, pParam,
                                				   numKeys, schAttr, _KLEN, _VLEN, setposlist, retpair, avec );
    if ( ! rc ) return false;

	if ( _insertBufferMap && _insertBufferMap->set( retpair ) ) {
		return true;
	}

	int pos = 0;
	int div, rem;
	char v[2];

	char kbuf[_KLEN+1];
	memset( kbuf, 0, _KLEN+1);
	memcpy( kbuf, pair.key.c_str(), _KLEN );

	if ( _keyChecker->getValue( kbuf, v ) ) {
		div = (jagbyte)v[0];
		rem = (jagbyte)v[1];
		pos = div*(JAG_BYTE_MAX+1)+rem;

		rc = _darrlist[pos]->set (retpair ); 

		return rc;
	} else {
		return 0;
	}
}


void JagDiskArrayFamily::drop()
{
    dn("s65231 JagDiskArrayFamily::drop() _darrlist.size=%d", _darrlist.size() );
	for ( int i = 0; i < _darrlist.size(); ++i ) {
        dn("s881028 JagDiskArrayFamily::drop() i=%d drop", i );
		_darrlist[i]->drop();
	}

    dn("s029338 removeAllKey()");
	_keyChecker->removeAllKey();
}

void JagDiskArrayFamily::flushBlockIndexToDisk()
{
	for ( int i = 0; i < _darrlist.size(); ++i ) {
		jd(JAG_LOG_LOW, "    Flush Index File %d/%d ...\n", i, _darrlist.size() );
		_darrlist[i]->flushBlockIndexToDisk();
	}
	
	if ( JAG_MEM_HIGH == _servobj->_memoryMode ) {
		jd(JAG_LOG_LOW, "    Flush Memory ...\n" );
		jagint n = flushKeyChecker();
		jd(JAG_LOG_LOW, "    Flush Memory %ld records done\n", n );
	}
}

jagint JagDiskArrayFamily::flushKeyChecker()
{
	if( _darrlist.size() < 1 ) {
		return 0;
	}

    // _pathname "/.../jbench3/jbench3.jidx3"
    // /.../jbench3/jbench3
	Jstr keyCheckerPath = _pathname + ".sig";

    dn("s129804 flushKeyChecker[%s]", keyCheckerPath.c_str() );
	int fd = jagopen( keyCheckerPath.c_str(), O_CREAT|O_RDWR|JAG_NOATIME, S_IRWXU);
	if ( fd < 0 ) {
		i("s3804 error open [%s] for write\n", keyCheckerPath.c_str() );
		return 0;
	}
	
	int klen;
	if ( _KLEN <= JAG_KEYCHECKER_KLEN ) {
		klen = _KLEN;
	} else {
		klen = JAG_KEYCHECKER_KLEN;
	}

	int vlen = 2;
	int kvlen = klen + vlen;

    // hdrbyte
	raysafewrite( fd, "1", 1 ); 

	const char *arr = _keyChecker->array();
	jagint len = _keyChecker->arrayLength();
    dn("s26551 _keyChecker->arrayLength=%ld", len);

    jagint res = 0;
    ssize_t wn;
	if ( len >= 1 ) {
		for ( jagint i = 0; i < len; ++i ) {
			if ( arr[i*kvlen] == '\0' ) continue;
			wn = raysafewrite( fd, arr+i*kvlen, kvlen );
            if ( wn == kvlen ) {
                ++res;
            } else {
                dn("s062626 error raysafewrite kvlen=%d but actually written %ld", kvlen, wn );
            }
		}
	}

    // hdrbyte
	raysafepwrite( fd, "0", 1, 0 );

	jagfdatasync( fd );
	jagclose( fd );

    return res;
}

jagint JagDiskArrayFamily::setFamilyRead( JagMergeReader *&nts, bool useInsertBuffer, const char *minbuf, const char *maxbuf ) 
{
    dn("s8122027 setFamilyRead ...");

	bool        startFlag = false;
    jagint      index = 0, rc, slimit, rlimit;
    JagDBPair   retpair;
	JagFixString value;
	char        *lminbuf = NULL;
	char        *lmaxbuf = NULL;
	jagint      maxPossibleElem = 0;
	JagVector<OnefileRange> fRange(8);
	OnefileRange tempRange;

	if ( !minbuf || !maxbuf ) {
		if ( maxbuf ) {
			lminbuf = (char*)jagmalloc(_KLEN+1);
			memset(lminbuf, 0, _KLEN+1);
			minbuf = lminbuf;
		} else if ( minbuf ) {
			lmaxbuf = (char*)jagmalloc(_KLEN+1);
			memset(lmaxbuf, 255, _KLEN);
			lmaxbuf[_KLEN] = '\0';
			maxbuf = lmaxbuf;
		}
	}
			
	if ( minbuf || maxbuf ) {
		JagDBPair minpair( JagFixString( minbuf, _KLEN, _KLEN ), value );
		JagDBPair maxpair( JagFixString( maxbuf, _KLEN, _KLEN ), value );
		for ( int i = _darrlist.size()-1; i >= 0; --i ) {
			rc = _darrlist[i]->exist( minpair, &index, retpair );
			if ( startFlag && !rc ) ++index;
			slimit = index;
			if ( slimit < 0 ) slimit = 0;
			rc = _darrlist[i]->exist( maxpair, &index, retpair );
			if ( startFlag && !rc ) ++index;
			rlimit = index - slimit + 1;
			if ( rlimit > 0 ) {
				tempRange.darr = _darrlist[i];
				tempRange.startpos = slimit;
				tempRange.readlen = rlimit;
				tempRange.memmax = 128;
				fRange.append( tempRange );
			}
		}
	} else {
		for ( int i = _darrlist.size()-1; i >= 0; --i ) {
			tempRange.darr = _darrlist[i];
			tempRange.startpos = -1;
			tempRange.readlen = -1;
			tempRange.memmax = 128;
			fRange.append( tempRange );
		}
	}
	
	if ( nts ) {
		delete nts;
		nts = NULL;
	}
	
	if ( fRange.size() >= 0 ) {
        if ( useInsertBuffer ) {
		    nts = new JagMergeReader(_insertBufferMap, fRange, _KLEN, _VLEN, minbuf, maxbuf );
        } else {
		    nts = new JagMergeReader(NULL, fRange,_KLEN, _VLEN, minbuf, maxbuf );
        }

	} else {
		nts = NULL;
	}

	if ( lminbuf ) { free( lminbuf ); }
	if ( lmaxbuf ) { free( lmaxbuf ); }
	return maxPossibleElem;
}

jagint JagDiskArrayFamily::setFamilyReadPartial( JagMergeReader *&nts, bool useInsertBuffer, const char *minbuf, const char *maxbuf, 
												  jagint spos, jagint epos, jagint mmax ) 
{
    dn("s123072721 setFamilyReadPartial() minbuf=[%s] maxbuf=[%s] ...", minbuf, maxbuf);

	if ( spos < 0 ) spos = 0;
	if ( epos >= _darrlist.size() ) epos = _darrlist.size()-1;
	if ( epos < 0 ) epos = 0;
	if ( spos > epos ) spos = epos;
	int darrsize = _darrlist.size();

    dn("s022877 setFamilyReadPartial darrsize=%d", darrsize );

	bool        startFlag = false;
    jagint      index = 0, rc, slimit, rlimit;
    JagDBPair   retpair;
	JagFixString value;
	char        *lminbuf = NULL;
	char        *lmaxbuf = NULL;
	jagint      maxPossibleElem = 0;
	JagVector<OnefileRange> fRange(8);
	OnefileRange tempRange;

	if ( !minbuf || !maxbuf ) {
        dn("s6503412  !minbuf || !maxbuf");
		if ( maxbuf ) {
            dn("s20350 maxbuf notnull");
			lminbuf = (char*)jagmalloc(_KLEN+1);
			memset(lminbuf, 0, _KLEN+1);
			minbuf = lminbuf;
		} else if ( minbuf ) {
            dn("s20350 minbuf notnull");
			lmaxbuf = (char*)jagmalloc(_KLEN+1);
			memset(lmaxbuf, 255, _KLEN);
			lmaxbuf[_KLEN] = '\0';
			maxbuf = lmaxbuf;
		}
	}
			
	if ( minbuf || maxbuf ) {
        dn("s662221  minbuf=[%s] || maxbuf=[%s] ", minbuf, maxbuf);
		JagDBPair minpair( JagFixString( minbuf, _KLEN, _KLEN ), value );
		JagDBPair maxpair( JagFixString( maxbuf, _KLEN, _KLEN ), value );

        /** debug
        dn("s79032 minpair:");
        minpair.printkv(true);
        dn("s79032 maxpair:");
        maxpair.printkv(true);
        **/

        dn("epos=%d spos=%d darrsize=%d", epos, spos, darrsize );

		for ( int i = epos; i >= spos && darrsize > 0; --i ) {

            index = 0;

            // locate minpair
			rc = _darrlist[i]->exist( minpair, &index, retpair );
            dn("s78135 i=%d darr minpair exists? rc=%d index=%ld ", i, rc, index );

			if ( startFlag && !rc ) {
                if ( index == -999 ) {
                    dn("s9991021 got -999 in index from minpair exist");
                    index = 0;
                } else {
                    ++index;
                }
            }

            if ( index < 0 ) index = 0;
            dn("s091123 minpair index=%ld", index);
			slimit = index;

            // locate maxpair
            index = 0;
			rc = _darrlist[i]->exist( maxpair, &index, retpair );
			if ( startFlag && !rc ) ++index;
            dn("s091125 maxpair index=%ld", index);

			rlimit = index - slimit + 1;
            dn("s750021 slimit=%ld startFlag=%d index=%ld rlimit=%ld", slimit, startFlag, index, rlimit);

			if ( rlimit > 0 ) {
				tempRange.darr = _darrlist[i];
				tempRange.startpos = slimit;
				tempRange.readlen = rlimit;
				tempRange.memmax = mmax;
				fRange.append( tempRange );
                dn("s34004 added darr %d   startpos=%ld readlen=%ld  memmax=%ld", i, slimit, rlimit, mmax );
			} else {
                dn("s34005 not added darr %d   startpos=%ld readlen=%ld  memmax=%ld", i, slimit, rlimit, mmax );
            }
		}
	} else {
        dn("s236621 minbuf and maxbuf are all NULL");
		for ( int i = epos; i >= spos && darrsize > 0; --i ) {
			tempRange.darr = _darrlist[i];
			tempRange.startpos = -1;
			tempRange.readlen = -1;
			tempRange.memmax = mmax;
			fRange.append( tempRange );
            dn("s090112 added darr %d -1 -1", i );
		}
	}

	if ( nts ) {
        dn("s5002281 delete nts");
		delete nts;
		nts = NULL;
	}
	
	if ( fRange.size() >= 0 ) {
        dn("s50251516 nts = new JagMergeReader _insertBufferMap.size=%ld", _insertBufferMap->size() );
        if ( useInsertBuffer ) {
		    nts = new JagMergeReader( _insertBufferMap, fRange, _KLEN, _VLEN, minbuf, maxbuf );
        } else {
		    nts = new JagMergeReader( NULL, fRange, _KLEN, _VLEN, minbuf, maxbuf );
        }
	} else {
		nts = NULL;
	}

	if ( lminbuf ) { free( lminbuf ); }
	if ( lmaxbuf ) { free( lmaxbuf ); }

	return maxPossibleElem;
}

jagint JagDiskArrayFamily::setFamilyReadBackPartial( JagMergeBackReader *&nts, bool useInsertBuffer, const char *minbuf, const char *maxbuf, 
												  	  jagint spos, jagint epos, jagint mmax ) 
{
	int  darrsize = _darrlist.size();

	if ( spos > epos ) spos = epos;
	bool startFlag = false;
    jagint index = 0, rc, slimit, rlimit;
    JagDBPair retpair;
	JagFixString value;
	char *lminbuf = NULL;
	char *lmaxbuf = NULL;
	jagint maxPossibleElem = 0;
	JagVector<OnefileRange> fRange(8);
	OnefileRange tempRange;

	if ( !minbuf || !maxbuf ) {
		if ( maxbuf ) {
			lminbuf = (char*)jagmalloc(_KLEN+1);
			memset(lminbuf, 0, _KLEN+1);
			minbuf = lminbuf;
		} else if ( minbuf ) {
			lmaxbuf = (char*)jagmalloc(_KLEN+1);
			memset(lmaxbuf, 255, _KLEN);
			lmaxbuf[_KLEN] = '\0';
			maxbuf = lmaxbuf;
		}
	}
			
	if ( minbuf || maxbuf ) {
		JagDBPair minpair( JagFixString( minbuf, _KLEN, _KLEN ), value );
		JagDBPair maxpair( JagFixString( maxbuf, _KLEN, _KLEN ), value );
		for ( int i = epos; i >= spos && darrsize > 0; --i ) {
			rc = _darrlist[i]->exist( minpair, &index, retpair );
			if ( startFlag && !rc ) ++index;
			slimit = index;
			if ( slimit < 0 ) slimit = 0;
			rc = _darrlist[i]->exist( maxpair, &index, retpair );
			if ( startFlag && !rc ) ++index;
			rlimit = index - slimit + 1;
			if ( rlimit > 0 ) {
				tempRange.darr = _darrlist[i];
				tempRange.startpos = slimit+rlimit;
				tempRange.readlen = rlimit;
				tempRange.memmax = mmax;
				fRange.append( tempRange );
			}
		}
	} else {
		for ( int i = epos; i >= spos && darrsize > 0; --i ) {
			tempRange.darr = _darrlist[i];
			tempRange.startpos = -1;
			tempRange.readlen = -1;
			tempRange.memmax = mmax;
			fRange.append( tempRange );
		}
	}

	if ( nts ) {
		delete nts;
		nts = NULL;
	}
	
	if ( fRange.size() >= 0 ) {
        if ( useInsertBuffer ) {
		    nts = new JagMergeBackReader( _insertBufferMap, fRange, _KLEN, _VLEN, minbuf, maxbuf );
        } else {
		    nts = new JagMergeBackReader( NULL, fRange, _KLEN, _VLEN, minbuf, maxbuf );
        }
	} else {
		nts = NULL;
	}

	if ( lminbuf ) { free( lminbuf ); }
	if ( lmaxbuf ) { free( lmaxbuf ); }
	return maxPossibleElem;
}

JagDiskArrayServer* JagDiskArrayFamily::flushBufferToNewFile( )
{
	if (  _insertBufferMap->elements() < 1 ) { 
		d("s183330 flushBufferToNewFile no data in mem, return nullptr\n");
		return nullptr; 
	}

	_isFlushing = 1;
	int darrlistlen = _darrlist.size();
	d("s40726 JagDiskArrayFamily::flushBufferToNewFile darrlistlen=%d\n", darrlistlen );
	jagint len = _insertBufferMap->size();

    dn("s770323 JagDiskArrayFamily::flushBufferToNewFile() _pathname=[%s]", _pathname.s() );
	JagStrSplit sp(_pathname, '/');
	int slen = sp.length();
	Jstr fname= sp[slen-1];

	Jstr filePathName;

    dn("s567002 JagDiskArrayFamily::flushBufferToNewFile _pathname=[%s]", _pathname.c_str() );

	filePathName = _pathname + "." + intToStr( darrlistlen );
    dn("s77752525 filePathName=[%s]", filePathName.s() );

	JagDiskArrayServer *darr = new JagDiskArrayServer( _servobj, this, darrlistlen, filePathName, _schemaRecord, len, false );
	darr->flushBufferToNewFile( _insertBufferMap );

	return darr;
}

void JagDiskArrayFamily::removeAndReopenWalLog()
{
    if ( _objType == JAG_INDEX ) {
        return;
    }

	Jstr dbtab = _dbname + "." + _taboridxname;
    dn("s7702837 removeAndReopenWalLog _dbname=[%s]  _taboridxname=[%s] dbtab=[%s]", _dbname.s(), _taboridxname.s(), dbtab.s() );

    Jstr walfpath = _servobj->_cfg->getWalLogHOME() + "/" + dbtab + ".wallog";
	d("s201226 reset wallog file %s ...\n", walfpath.c_str() );
	jd(JAG_LOG_LOW, "cleanup wallog %s \n", walfpath.s() ); 

    FILE *walFile = _servobj->_walLogMap.ensureFile( walfpath );
	fclose( walFile );
    _servobj->_walLogMap.removeKey( walfpath );
	jagunlink( walfpath.c_str() );

    _servobj->_walLogMap.ensureFile( walfpath );

}

jagint JagDiskArrayFamily::memoryBufferSize() 
{
	if ( NULL == _insertBufferMap ) {
		return 0;
	}

	jagint cnt = _insertBufferMap->size();
	return cnt;
}

void JagDiskArrayFamily::debugPrintBuffer()
{
	if ( NULL == _insertBufferMap ) {
		return;
	}

    JagFixMapIterator iter = _insertBufferMap->_map->begin();

    printf("s0283811 debugPrintBuffer() data in _insertBufferMap: %lld records\n", _insertBufferMap->size() );
    while ( iter != _insertBufferMap->_map->end() ) {
        printf("key:%lld=[", iter->first.size());
        dumpmem( iter->first.c_str(), iter->first.size(), false );

        printf("] val:%lld=[", iter->second.size());
        dumpmem( iter->second.c_str(), iter->second.size(), false );
        printf("]\n");
        ++ iter;
    }
    printf("\n");
    fflush( stdout );
}


