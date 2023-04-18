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
#ifndef _file_mgr_h_
#define _file_mgr_h_

#include <sys/stat.h>
#include <unistd.h>
#include <abax.h>
#include <JagVector.h>

class JagFileMgr
{
	public:

		static void 	makedirPath( const Jstr &fullpath, int mode=0755 );
		static void 	rmdir( const Jstr &fullpath, bool rmtop=true );
		static jagint 	fileSize( const Jstr &fullpath );
		static int 		getPathUsage( const char *fpath,  jagint &usedGB, jagint &freeGB );
		static int 		isFile( const Jstr &fullpath );
		static int 		isDir( const Jstr &fullpath );
		static bool 	dirEmpty( const Jstr &fullPathDir );
		static int 		exist( const Jstr &fullpath );
		static void 	readTextFile(const Jstr &fname, Jstr &content );
		static void 	writeTextFile(const Jstr &fname, const Jstr &content );
		static void 	writeTextFile(const Jstr &fname, const char *content );
		static int 		cleanDir( const Jstr &fullpath, time_t historySeconds );
		static int 		cleanDirExclude( const Jstr &fullpath, const Jstr &exsuffix );
		static jagint 	numObjects( const Jstr &fullpath );
		static Jstr 	listObjects( const Jstr &fullpath, const Jstr &substr="" );
		static Jstr     makeLocalLogDir( const Jstr &subdir="" );
		static Jstr     getLocalLogDir( const Jstr &subdir="" );
		static int 		getIOStat( jaguint &reads, jaguint & writes );
		static int      fallocate(int fd, jagint offset, jagint len);
		static Jstr     getFileFamily( int objType, const Jstr &fullpath, const Jstr &objname );
		static int      pathWritable( const Jstr &fullpath );
        static int      getToDoFiles( const Jstr &dirpath, JagVector<Jstr> &vec );
		static Jstr     dirName( const Jstr &fpath );
		static Jstr     baseName( const Jstr &fpath );

};

#endif
