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
#include <JagSystem.h>
#include <JagStrSplit.h>
#include <JagUtil.h>
#include <JagFileMgr.h>
#include <JagNet.h>

// 0: OK
// -1: error
int JagSystem::getStat6( jagint &totalDiskGB, jagint &usedDiskGB, jagint &freeDiskGB, jagint &nproc, float &loadvg, jagint &tcp )
{
	totalDiskGB = usedDiskGB = freeDiskGB = 1;
	nproc = 1; loadvg = 0.1; tcp = 10;

	// get rdbhome usage
	Jstr jaghome= jaguarHome();
	jagint usedDisk, freeDisk;
	JagFileMgr::getPathUsage( jaghome.c_str(), usedDisk, freeDisk );
	// d("s2010 getStat6 jaghome=[%s] usedDisk=%lld freeDisk=%lld\n",  jaghome.c_str(), usedDisk, freeDisk );
	totalDiskGB = usedDisk + freeDisk;
	usedDiskGB = usedDisk;
	freeDiskGB = freeDisk;
	nproc = getNumProcs();
	loadvg = getLoadAvg();
	tcp = JagNet::getNumTCPConnections();
	return 0;
}

// static 
#ifndef _WINDOWS64_
// Linux
void JagSystem::initLoad()
{
}

int JagSystem::getCPUStat( jagint &user, jagint &sys, jagint &idle )
{
	user = sys = 0;
	idle = 1;
	FILE *fp = jagfopen( "/proc/stat", "rb" );
	if ( ! fp ) return 0;
	Jstr line;

	jagint tot = 0;
	char buf[1024];
	while ( NULL != ( fgets(buf, 1024, fp ) ) ) {
		if ( strncmp(buf, "cpu", 3 ) == 0 ) {
			line = buf;
			JagStrSplit sp( line, ' ', true );
			if ( sp.length() < 5 ) continue;
			user += jagatoll( sp[1].c_str() );
			sys += jagatoll( sp[3].c_str() );
			idle += jagatoll( sp[4].c_str() );
			break;
		}
	}

	tot = user + sys + idle;
	if ( tot < 1 ) return 0;

	user = (100* user ) / tot;
	sys = (100* sys ) / tot;
	idle = (100* idle ) / tot;

	return 1;
}


#include <sys/sysinfo.h>
int JagSystem::getNumProcs()
{
	// SysInfo sinfo;
	struct sysinfo sinfo;
	sysinfo( &sinfo );
	return sinfo.procs;
}

float JagSystem::getLoadAvg()
{
	FILE *fp = jagfopen("/proc/loadavg", "rb" );
	if ( ! fp ) return 0.0;
	char line[256];
	memset( line, 0, 256 );
	char *pw = fgets(line, 250, fp );
    if ( NULL == pw ) {
        return 0.0;
    }

	JagStrSplit sp(line, ' ', true );
	float loadvg = atof( sp[1].c_str() );
	jagfclose( fp);
	return loadvg;
}

int JagSystem::getNumCPUs() 
{ 
	return sysconf(_SC_NPROCESSORS_ONLN); 
}

// GB
int  JagSystem::getMemInfo( jagint &totm, jagint &freem, jagint &used )
{
	totm = freem = used = 0;
	FILE *fp = jagfopen("/proc/meminfo", "rb" );
	if ( ! fp ) return 0;
	char buf[256];
	while ( NULL != fgets( buf, 256, fp ) ) {
		if ( strstr( buf, "MemTotal" ) ) {
			JagStrSplit sp( buf, ' ', true);
			totm = jagatoll( sp[1].c_str() )/(1024*1024);
		} else if ( strstr( buf, "MemFree" ) ) {
			JagStrSplit sp( buf, ' ', true);
			freem = jagatoll( sp[1].c_str() )/(1024*1024);
		}
	}

	used = totm - freem;
	return 1;
}

#else
// windows
#include <TlHelp32.h>
int JagSystem::getNumCPUs()
{
    SYSTEM_INFO sysInfo;
    GetSystemInfo( &sysInfo );
    return sysInfo.dwNumberOfProcessors;
}

void JagSystem::initLoad()
{
    HANDLE selfHandle = GetCurrentProcess();
    FILETIME  ftime, fsys, fuser;
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    numProcessors = sysInfo.dwNumberOfProcessors;
    GetSystemTimeAsFileTime(&ftime);
    memcpy(&lastCPU, &ftime, sizeof(FILETIME));
    GetProcessTimes(selfHandle, &ftime, &ftime, &fsys, &fuser);
    memcpy(&lastSysCPU, &fsys, sizeof(FILETIME));
    memcpy(&lastUserCPU, &fuser, sizeof(FILETIME));
}

// user 0:100  sys: 0-100 idle: 0-100
int JagSystem::getCPUStat( jagint &ouser, jagint &osys, jagint &oidle )
{
	ouser = osys = 0;
	oidle = 1;

    FILETIME ftime, fsys, fuser;
    ULARGE_INTEGER now, sys, user;
    double syscpu, usercpu;

    GetSystemTimeAsFileTime(&ftime);
    memcpy(&now, &ftime, sizeof(FILETIME));

	HANDLE selfHandle = GetCurrentProcess();
    GetProcessTimes(selfHandle, &ftime, &ftime, &fsys, &fuser);
    memcpy(&sys, &fsys, sizeof(FILETIME));
    memcpy(&user, &fuser, sizeof(FILETIME));

    lastCPU = now;
    lastUserCPU = user;
    lastSysCPU = sys;

    syscpu = (sys.QuadPart - lastSysCPU.QuadPart);
    syscpu /= (now.QuadPart - lastCPU.QuadPart);
    syscpu /= numProcessors;

    usercpu = (user.QuadPart - lastUserCPU.QuadPart);
    usercpu /= (now.QuadPart - lastCPU.QuadPart);
    usercpu /= numProcessors;

	ouser = ( jagint ) usercpu;
    osys = ( jagint ) syscpu;
    oidle = 100 - ( user.QuadPart + sys.QuadPart );
	return 1;
}

float JagSystem::getLoadAvg()
{
	float loadvg = 0.0;
	jagint user, sys, idle;
	getCPUStat( user, sys, idle );
	loadvg = (float)( user + sys ) /2.0;
	return loadvg;
}

int JagSystem::getNumProcs()
{
	HANDLE hSnapshot;
    HANDLE hProcess;
    PROCESSENTRY32 pe32;
    hSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
    if( hSnapshot == INVALID_HANDLE_VALUE ) {
		return 0;
    }
    pe32.dwSize = sizeof( PROCESSENTRY32 );
 
    if( !Process32First( hSnapshot, &pe32 ) ) {
        CloseHandle( hSnapshot ); 
        return 0;
    }
 
	int cnt = 0;
    do {
		++ cnt;
    } while( Process32Next( hSnapshot, &pe32 ) );
    CloseHandle( hSnapshot );
	return cnt;
}
// GB
int  JagSystem::getMemInfo( jagint &totm, jagint &freem, jagint &used )
{

	MEMORYSTATUSEX memInfo;
	GlobalMemoryStatusEx (&memInfo);
	totm = memInfo.ullTotalPhys / ONE_GIGA_BYTES;
	freem = ( memInfo.ullTotalPhys - memInfo.ullAvailPhys ) / ONE_GIGA_BYTES;
	used = totm - freem;
	return 1;
}

#endif

