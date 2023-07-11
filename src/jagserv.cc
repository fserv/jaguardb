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
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <JagServer.h>

static void sigsegv_handler(int sig, siginfo_t *si, void *unused)
{
    printf("Got SIGSEGV\n");
}


int main(int argc, char**argv)
{
	char *p = getenv("FROM_SHELL");
	if  ( ! p ) {
		printf("Starting server failure\n");
		printf("jaguar server must be started with jaguarstart program\n");
		exit(51);
	}

    /////////////////////////////////////////
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = sigsegv_handler;
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        printf("error setting up SIGSEGV handler\n");
        exit(10);
    }

	JagServer *serv = new JagServer();
	if ( serv ) {
		serv->main( argc, argv );
		if ( serv ) delete serv;
	}
}

