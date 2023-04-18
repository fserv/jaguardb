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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <time.h>
#include <libgen.h>

#ifndef _WINDOWS64_
#include <sys/statvfs.h>
#endif

#include <abax.h>
#include <JagFileMgr.h>
#include <JagStrSplit.h>
#include <JagUtil.h>

void JagFileMgr::makedirPath( const Jstr &fullpath, int mode )
{
	if ( fullpath.length()<1) return;

	JagStrSplit ar(fullpath, '/');
	int len=ar.length();
	int isabs = 0;
	char *pstr=(char*)fullpath.c_str();
	if ( '/' == pstr[0] ) {
		isabs = 1; // absolute path
	}

	Jstr tok;
	Jstr path;
	Jstr seg;

	for ( int i=0; i<len; i++) {
		if (isabs ) path="/"; else path="";
		tok = ar[i];
		for (int j=0; j<=i; j++) {
			seg = ar[j];
			path += seg + "/";
		}

		jagmkdir( path.c_str(), mode );
	}
}

int JagFileMgr::isFile( const Jstr &fullpath )
{
	struct stat   	statbuf;
	if ( stat( fullpath.c_str(), &statbuf) < 0 )
	{ return 0; }

	if ( S_ISREG( statbuf.st_mode ) )
	{ return 1; }

	return 0;
}

int JagFileMgr::isDir( const Jstr &fullpath )
{
	struct stat   	statbuf;
	if ( stat( fullpath.c_str(), &statbuf) < 0 )
	{ return 0; }

	if ( S_ISDIR( statbuf.st_mode ) )
	{ return 1; }

	return 0;
}

// remove recursively an dir (also remove if fullpath is a file)
// finally rmdir top level empty dir if rmtop is true 
void JagFileMgr::rmdir( const Jstr &fullpath, bool rmtop )
{
	struct stat   	statbuf;
	DIR 			*dp;
	struct dirent  	*dirp;
	Jstr nextlevelpath;

	// d("s9293 rmdir fullpath=[%s]\n", fullpath.c_str() );
	if ( stat( fullpath.c_str(), &statbuf) < 0 ) {
		dn("s02939 < 0 stat of [%s] <0", fullpath.c_str() );
		return;
	}

	// no directory, remove file
	if ( S_ISDIR( statbuf.st_mode ) == 0 ) {
		::remove( fullpath.c_str() );
		// d("s9293 error\n" );
		return;
	}

	if ( NULL == (dp=opendir(fullpath.c_str())) ) {
		// d("s9493 error\n" );
		return;
	}

	while( NULL != (dirp=readdir(dp)) ) {
		if ( 0==strcmp(dirp->d_name, ".") || 0==strcmp(dirp->d_name, "..") ) {
			continue;
		}

		nextlevelpath = fullpath + "/" + dirp->d_name;
		JagFileMgr::rmdir( nextlevelpath );
	}

	if ( rmtop ) {
		::rmdir( fullpath.c_str() );
	}

	closedir(dp);
	// d("s0293 OK\n" );
}


// remove folders and files with history longer than historySeconds
// including self fullpath
int JagFileMgr::cleanDir( const Jstr &fullpath, time_t historySeconds )
{
	struct stat   	statbuf;
	DIR 			*dp;
	struct dirent  	*dirp;
	time_t   		modtime;
	time_t   		nowtime;
	Jstr nextlevelpath;

	if ( stat( fullpath.c_str(), &statbuf) < 0 ) {
		return 0;
	}

	modtime = statbuf.st_mtime;
	nowtime = time(NULL);

	// no directory, remove file
	if ( S_ISDIR( statbuf.st_mode ) == 0 ) {
		int n = 0;
		if (  (nowtime - modtime)  > historySeconds ) {
			::remove( fullpath.c_str() );
			n = 1;
		}
		return n;
	}

	if ( NULL == (dp=opendir(fullpath.c_str())) ) {
		return 0;
	}

	int tot = 0;
	while( NULL != (dirp=readdir(dp)) ) {
		if ( 0==strcmp(dirp->d_name, ".") || 0==strcmp(dirp->d_name, "..") ) {
			continue;
		}

		nextlevelpath = fullpath + "/" + dirp->d_name;
		tot += JagFileMgr::cleanDir( nextlevelpath, historySeconds );
	}

	// self fullpath dir time stamp if (  (nowtime - modtime)  > historySeconds )
	{
		::rmdir( fullpath.c_str() );
	}

	closedir(dp);
	return tot;
}

// in bytes
jagint JagFileMgr:: fileSize( const Jstr &fullpath )
{
	struct stat   	statbuf;
	if ( stat( fullpath.c_str(), &statbuf) < 0 )
	{ return 0; }

	return statbuf.st_size;
}

// static
int JagFileMgr::exist( const Jstr &fullpath )
{
	struct stat   	statbuf;
	if ( stat( fullpath.c_str(), &statbuf) < 0 ) {
		return 0;
	}
	return 1;
}

// static
void JagFileMgr::readTextFile(const Jstr &fpath, Jstr &content )
{
	content = "";
    char buf[2];
    FILE *fp = jagfopen(fpath.c_str(), "r");
    if ( ! fp ) return;
	size_t  len;

	while ( 1 ) {
    	memset( (void*) buf, 0, 2);
		len = fread( buf, 1, 1, fp );
        content += buf;
		if ( len < 1 ) break;
    }

    jagfclose(fp);
    return ;
}

// static
void JagFileMgr::writeTextFile(const Jstr &fname, const Jstr &content )
{
    FILE *fp = jagfopen(fname.c_str(), "w");
    if ( ! fp ) return;
	fprintf(fp, "%s", content.c_str() );
    jagfclose(fp);
    return ;
}

// static
void JagFileMgr::writeTextFile(const Jstr &fname, const char *content )
{
    FILE *fp = jagfopen(fname.c_str(), "wb");
    if ( ! fp ) return;
	fprintf(fp, "%s", content );
    jagfclose(fp);
    return ;
}


#ifndef _WINDOWS64_
// 1: OK   0: error
int JagFileMgr::getPathUsage( const char *fpath,  jagint &usedGB, jagint &freeGB )
{
	usedGB = 0;
	freeGB = 0;
    struct statvfs buf;
    if ( 0 != statvfs(fpath, &buf)) {
		return 0;
	}

    jaguint blksize, blocks, freeblks, disk_size, used, free;
    blksize = buf.f_bsize;
    blocks = buf.f_blocks;
    freeblks = buf.f_bfree;

    disk_size = blocks * blksize;
    free = freeblks * blksize;
    used = disk_size - free;

	usedGB = used/ONE_GIGA_BYTES;
	if ( usedGB < 1 ) usedGB = 1;
	freeGB = free/ONE_GIGA_BYTES;

	//d("s2425 usedGB=%lld freeGB=%lld\n", usedGB, freeGB );

	return 1;
}
#else
// WINDOWS
// 1: OK   0: error
int JagFileMgr::getPathUsage( const char *fpath,  jagint &usedGB, jagint &freeGB )
{
	usedGB = 0;
	freeGB = 1024;
	BOOL res;
	unsigned __int64 i64FreeBytesToCaller, i64TotalBytes, i64FreeBytes;
	res = GetDiskFreeSpaceEx (fpath, (PULARGE_INTEGER)&i64FreeBytesToCaller, 
								(PULARGE_INTEGER)&i64TotalBytes, (PULARGE_INTEGER)&i64FreeBytes);
	if ( ! res ) {
		return 0;
	}

	freeGB = (i64FreeBytes/ ONE_GIGA_BYTES);
    jagint totGB = (i64TotalBytes/ONE_GIGA_BYTES);
	usedGB = (totGB - freeGB);
	return 1;
}
#endif

Jstr JagFileMgr::makeLocalLogDir( const Jstr &subdir )
{
    Jstr fpath = getLocalLogDir( subdir );
	if ( ! isDir( fpath ) ) {
    	jagmkdir( fpath.c_str(), 0700 );
	}
    return fpath;
}

Jstr JagFileMgr::getLocalLogDir( const Jstr &subdir )
{
    Jstr fpath;
	if ( subdir.size() > 0 ) {
    	fpath = jaguarHome() + "/log/" + subdir;
	} else {
    	fpath = jaguarHome() + "/log";
	}
    return fpath;
}

#ifndef _WINDOWS64_
// Linux
// get network reads and writes
int JagFileMgr::getIOStat( jaguint &reads, jaguint & writes )
{
	reads = writes = 0;
	FILE *fp = jagfopen( "/proc/diskstats", "rb" );
	if ( ! fp ) return 0;
	Jstr line;

	char buf[1024];
	while ( NULL != ( fgets(buf, 1024, fp ) ) ) {
		line = buf;
		JagStrSplit sp( line, ' ', true );
		if ( sp.length() < 8 ) continue;
		reads += jagatoll( sp[3].c_str() );
		writes += jagatoll( sp[7].c_str() );
	}

	jagfclose(fp);
	return 1;
}
#else
// Windows
int JagFileMgr::getIOStat( jaguint &reads, jaguint & writes )
{
	reads = writes = 0;
	IO_COUNTERS  cntr;
	reads = cntr.ReadTransferCount;
	writes = cntr.WriteTransferCount;
	return 1;
}
#endif

bool JagFileMgr::dirEmpty( const Jstr &fullpath )
{
	struct stat   	statbuf;
	if ( stat( fullpath.c_str(), &statbuf) < 0 )
	{ return false; }

	if ( ! S_ISDIR( statbuf.st_mode ) )
	{ return false; }

	if ( numObjects( fullpath ) > 0 ) {
		return false;
	} else {
		return true;
	}
}


int   JagFileMgr::fallocate(int fd, jagint offset, jagint len)
{
	return 0;
}


// objType: JAG_TABLE or JAG_INDEX
// get files that have pattern  file.N.jdb in a dir
// tab1.1.jdb
// tab1.2.jdb
// tab1.3.jdb
// tab1.4.jdb
// fullpath  /path/aaa
// objname: tab123.jdb  or tab123.idx123.jdb
// return: "/path/aaa/t1.1.jdb|/path/aaa/t1.2.jdb|/path/aaa/t1.3.jdb|/path/aaa/t3.idx1.5.jdb"
Jstr JagFileMgr::getFileFamily( int objType, const Jstr &fullpath, const Jstr &objname )
{
    // table:  fullpath="/../data/test/jbench3"
    // index:  fullpath="/../data/test/jbench3.idxname223"
    dn("s5500398 getFileFamily objtype=%d fullpath=%s objname=%s", objType, fullpath.s(), objname.s() );
	struct stat   	statbuf;
	DIR 			*dp;
	struct dirent  	*dirp;
	Jstr            onepath, tabname, idxname, suffix;
	int             parts = 0;

	Jstr res;
	if ( stat( fullpath.c_str(), &statbuf) < 0 ) {
        dn("s00018 return empty res");
		return res;
	}

	// no directory, remove file
	if ( S_ISDIR( statbuf.st_mode ) == 0 ) {
        dn("s04018 not a dir, return empty res");
		return res;
	}

	if ( NULL == (dp=opendir(fullpath.c_str())) ) {
        dn("s84018 not able to open [%s] dir, return empty res", fullpath.c_str() );
		return res;
	}

	JagStrSplit osp( objname, '.' );
	if ( osp.length() < 2 ) {
        dn("s94018  [%s] has no . return empty res", objname.s() );
		return res;
	}

	if ( osp.length() ==2 ) {
        // table objname="jbench3.jdb"
		tabname = osp[0];
		suffix = osp[1]; // jdb
		parts = 2;
	} else if ( osp.length() == 3 ) {
        // index  objname="jbench3.jidx3.jdb"
		tabname = osp[0];
		idxname = osp[1];
		suffix = osp[2]; // jdb
		parts = 3;
	} else {
        dn("s93939393 retur empty res");
		return res;
	}

    dn("s6627700 tabname=[%s] suffix=[%s] idxname=[%s] parts=%d", tabname.s(), suffix.s(), idxname.s(), parts);

    int splen;
    JagVector<Jstr>  vec;
	while( NULL != (dirp=readdir(dp)) ) {
		if ( 0==strcmp(dirp->d_name, ".") || 0==strcmp(dirp->d_name, "..") ) {
			continue;
		}

		onepath = fullpath + "/" + dirp->d_name;
        // table:  d_name=jbench3.0.jdb
        // index:  d_name=jbench3.idx888.0.jdb
        dn("s0039770 readdir see onepath=[%s]", onepath.s() );

		JagStrSplit sp( dirp->d_name, '.' );
        splen = sp.length();

        if (  objType == JAG_TABLE ) {
            if ( parts == 2 && splen == 3 
                && sp[0] == tabname && isdigit( sp[1].firstChar() ) && sp[2] == suffix ) {

                vec.push_back( onepath );
            }
        } else {
            if ( parts == 3 && splen == 4 
                && sp[0] == tabname && sp[1] == idxname && isdigit( sp[2].firstChar() ) && sp[3] == suffix ) {

                vec.push_back( onepath );
            }
        }
	}

    for ( int i=0; i < vec.size(); ++i ) {
        if ( i == 0 ) {
            res = vec[i];
        } else {
            res += Jstr("|") +  vec[i];
        }
    }

	closedir(dp);

    dn("s95500 res=[%s]", res.s() );
	return res;
}

// objects in fullpath dir
jagint JagFileMgr::numObjects( const Jstr &fullpath )
{
	jagint num = 0;

	struct stat   	statbuf;
	DIR 			*dp;
	struct dirent  	*dirp;
	Jstr nextlevelpath;

	if ( stat( fullpath.c_str(), &statbuf) < 0 ) {
		return 0;
	}

	if ( S_ISDIR( statbuf.st_mode ) == 0 ) {
		return 0;
	}

	if ( NULL == (dp=opendir(fullpath.c_str())) ) {
		return 0;
	}

	while( NULL != (dirp=readdir(dp)) ) {
		if ( 0==strcmp(dirp->d_name, ".") || 0==strcmp(dirp->d_name, "..") ) {
			continue;
		}
		++num;
	}

	closedir(dp);
	return num;
}

// return "a|b|c" where a are object names under fullpath
Jstr JagFileMgr::listObjects( const Jstr &fullpath, const Jstr &substr )
{
	struct stat   	statbuf;
	DIR 			*dp;
	struct dirent  	*dirp;
	Jstr nextlevelpath;

	if ( stat( fullpath.c_str(), &statbuf) < 0 ) {
		return "";
	}

	if ( S_ISDIR( statbuf.st_mode ) == 0 ) {
		return "";
	}

	if ( NULL == (dp=opendir(fullpath.c_str())) ) {
		return "";
	}

	while( NULL != (dirp=readdir(dp)) ) {
		if ( 0==strcmp(dirp->d_name, ".") || 0==strcmp(dirp->d_name, "..") ) {
			continue;
		}

		if ( substr.size() > 0 ) {
			if ( ! strstr(dirp->d_name, substr.c_str()) ) {
				continue;
			}
		}

		if ( nextlevelpath.size() < 1 ) {
			nextlevelpath = dirp->d_name;
		} else {
			nextlevelpath += Jstr("|") + dirp->d_name;
		}
	}

	closedir(dp);
	return nextlevelpath;
}

// recursively remove everything in fullpath dir, except files with suffix of exsuffix
// exsuffix ".tar.gz"
// fullpath is kept
int JagFileMgr::cleanDirExclude( const Jstr &fullpath, const Jstr &exsuffix )
{
	struct stat   	statbuf;
	DIR 			*dp;
	struct dirent  	*dirp;
	Jstr   nextlevelpath;

	if (  exsuffix.size() > 0 && endWithStr( fullpath, exsuffix )  ) {
		return 0;
	}

	if ( stat( fullpath.c_str(), &statbuf) < 0 ) {
		return 0;
	}

	// no directory, remove file
	if ( S_ISDIR( statbuf.st_mode ) == 0 ) {
		::remove( fullpath.c_str() );
		return 1;
	}

	if ( NULL == (dp=opendir(fullpath.c_str())) ) {
		return 0;
	}

	int tot = 0;
	while( NULL != (dirp=readdir(dp)) ) {
		if ( 0==strcmp(dirp->d_name, ".") || 0==strcmp(dirp->d_name, "..") ) {
			continue;
		}

		nextlevelpath = fullpath + "/" + dirp->d_name;
		tot += cleanDirExclude( nextlevelpath, exsuffix );
	}

	closedir(dp);
	return tot;
}


int JagFileMgr::pathWritable( const Jstr &fullpath )
{
	char buf[1024];
	if ( fullpath.size() >= 1024 ) return 0;
	strcpy( buf, fullpath.c_str() );

	if ( 0 == access( dirname( (char*)buf), W_OK )  ) {
		return 1;
	} else {
		return 0;
	}
}


/***
       path       dirname   basename
       /usr/lib   /usr      lib
       /usr/      /         usr
       usr        .         usr
       /          /         /
       .          .         .
       ..         .         ..
***/
Jstr JagFileMgr::dirName( const Jstr &fpath )
{
    Jstr f = fpath;
    return dirname( (char*)f.c_str() );
}

Jstr JagFileMgr::baseName( const Jstr &fpath )
{
    Jstr f = fpath;
    return basename( (char*)f.c_str() );
}

// input: dirpath /home/.../jaguar/log/delta/history
// output: dirpath /home/.../jaguar/log/delta/history/292.138.4.151_1_2023-04-16-15-58-36.todo|...|...
// return <0 for error; vec has *.todo file fullpaths
int JagFileMgr::getToDoFiles( const Jstr &dirpath, JagVector<Jstr> &vec )
{
	struct stat   	statbuf;
	DIR 			*dp;
	struct dirent  	*dirp;

	if ( stat( dirpath.c_str(), &statbuf) < 0 ) {
		return -10;
	}

	if ( S_ISDIR( statbuf.st_mode ) == 0 ) {
		return -20;
	}

	if ( NULL == (dp=opendir(dirpath.c_str())) ) {
		return -30;
	}

	while( NULL != (dirp=readdir(dp)) ) {
		if ( 0==strcmp(dirp->d_name, ".") || 0==strcmp(dirp->d_name, "..") ) {
			continue;
		}

        JagStrSplit sp(dirp->d_name, '.');
        if ( sp.size() >= 2 ) {
            if ( sp.last() == "todo" ) {
                vec.append( dirp->d_name );
            } else {
                continue;
            }

        } else {
            continue;
        }

	}

	closedir(dp);
	return vec.size();
}

