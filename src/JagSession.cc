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

#include <JagSession.h>
#include <JagUtil.h>

JagSession::JagSession()
{
	sock = active = done = timediff = connectionTime = 0;
	origserv = 0;
	exclusiveLogin = 0;
	fromShell = 0;
	servobj = NULL;

	replicType = 0;

	drecoverConn = 0;
	spCommandReject = 0;
	hasTimer = 0;
	sessionBroken = 0;
	samePID = 0;
	dcfrom = 0;
	dcto = 0;
	//noInterrupt = false;
}

JagSession::~JagSession() 
{
	sessionBroken = 1;
	if ( hasTimer ) {
		pthread_join( threadTimer, NULL );
	}
}

void JagSession::createTimer()
{
	if ( samePID ) return;
	hasTimer = 1;
	jagpthread_create( &threadTimer, NULL, sessionTimer, (void*)this );
}

void *JagSession::sessionTimer( void *ptr )
{
	JagSession *sess = (JagSession*)ptr;
	while ( !sess->sessionBroken ) {
		jagsleep(10, JAG_SEC); 
		if ( sess->active ) {
			//sendMessageLength2( sess, "Y", 1, "HB" );
			sendMessageLength2( sess, "Y", 1, JAG_MSG_HB, JAG_MSG_NEXT_MORE );
		}
	}
	return NULL;
}
