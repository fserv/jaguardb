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
#ifndef _jag_pass_h_
#define _jag_pass_h_

#include <JagNet.h>
class JagDBServer;
class JaguarCPPClient;

class JagPass {
	public:
		JagDBServer *servobj;
		JAGSOCK sock;
		Jstr  ip;
		Jstr  passwd;
		unsigned short port;
};

class JagCliCmdPass {
	public:
		JagDBServer *servobj;
		JaguarCPPClient *cli;
		Jstr cmd;
};

#endif
