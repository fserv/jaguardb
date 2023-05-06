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
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <assert.h>

#include <JagUUID.h>
#include <JagNet.h>
#include <JagTime.h>
#include <JagMath.h>
#include <JagDef.h>
#include <JagLog.h>
#include <abax.h>
#include <JagMath.h>

#ifdef _WINDOWS64_
#include <winsock2.h>
#include <windows.h>
#endif


JagUUID::JagUUID()
{
	JagNet::socketStartup();
	_hostID = JagNet::getMacAddress();
	_pid = getpid();
	srand( _pid%100);
	_getHostStr();
	_getPidStr();
    _seq = 1;
}

Jstr JagUUID::getStringAt( int n )
{
    Jstr clus;
    JagMath::base62FromULong(clus, (unsigned long)n, 2);

    dn("s203338 getStringAt n=%d clus=[%s]", n, clus.s() );

	int cnt = clus.size();

    if ( _seq >= 10000000 ) _seq = 1;
    // 7 --> 4

	char ds[JAG_UUID_FIELD_LEN+1];
    jaguint nowMicro = JagTime::nowMicroSeconds(); // 16 digits
    // 16->9 b62
    Jstr sMicro;
    JagMath::base62FromULong(sMicro, nowMicro, 9 );

    Jstr seqs;
    JagMath::base62FromULong(seqs, _seq, 4 );

	int rn = JAG_UUID_FIELD_LEN - 9 - 4 - 5 - 3 - 1 - cnt;
	sprintf(ds, "%s%s%s%s%s@%s", sMicro.s(), seqs.s(), AbaxString::randomValue(rn).c_str(), _hostStr.c_str(), _pidStr.c_str(), clus.s() );

    //assert( strlen(ds) == JAG_UUID_FIELD_LEN );
    ++ _seq;

    dn("ju1929 getStringAt n=%d _hostStr=%s _pidStr=%s ds=[%s]", n, _hostStr.c_str(), _pidStr.c_str(), ds );

	return ds;
}

Jstr JagUUID::getGidStringAt( int n )
{
    Jstr clus;
    JagMath::base62FromULong(clus, (unsigned long)n, 2);

    dn("s203338 getStringAt n=%d clus=[%s]", n, clus.s() );

	char ds[16];
    jaguint nowMicro = JagTime::nowMicroSeconds(); // 16 
    // 16->9 b62 
    Jstr sMicro;
    JagMath::base62FromULong(sMicro, nowMicro, 9 );

	int rn = 2;
	sprintf(ds, "%s%s@%s", sMicro.s(), AbaxString::randomValue(rn).c_str(), clus.s() );
    // total 14 bytes

    dn("ju1929 getShortStringAt n=%d _pidStr=%s ds=[%s]", n, ds );

	return ds;
}

// 5 bytes short string
void JagUUID::_getHostStr()
{
	jagint hc = _hostID.hashCode();
    JagMath::base62FromULong(_hostStr, (jaguint)hc, 5 );
    dn("ju0492 _getHostStr _hostStr=[%s]", _hostStr.s() );
}

// 3 bytes short string
void JagUUID::_getPidStr()
{
    JagMath::base62FromULong(_pidStr, (jaguint)_pid, 3 );
    dn("ju0292 _getPidStr _pidStr=[%s]", _pidStr.s() );
}
