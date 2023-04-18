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
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <iostream>
#include <signal.h>

#include <abax.h>
#include <JagCfg.h>
#include <JagGapVector.h>
#include <JagBlock.h>
#include <JagArray.h>
#include <JagClock.h>
#include <JaguarCPPClient.h>
#include <JagUtil.h>
#include <JagStrSplit.h>

class PassParam {
	public:
		Jstr username;
		Jstr passwd;
		Jstr host;
		Jstr port;
		Jstr dbname;
		Jstr session;
};
PassParam g_param;
int  _debug = 0;

int  	parseArgs( int argc, char *argv[], 
				Jstr &username, Jstr &passwd, Jstr &host, Jstr &port,
				Jstr& dbname, Jstr &sqlcmd, int &echo, Jstr &exclusive, 
				Jstr &fullConnect, Jstr &mcmd, Jstr &clearex, 
				Jstr &vvdebug, Jstr &quiet, Jstr &keepNewline, Jstr &sqlFile );

int queryAndReply( JaguarCPPClient& jcli, const char *query, const Jstr &quiet, bool ishello=false );
void usage( const char *prog );
void* sendStopSignal(void *p);
void  printResult( const JaguarCPPClient &jcli, const JagClock &clock, jagint cnt );
int executeCommands( JagClock &clock, const Jstr &sqlFile, JaguarCPPClient &jcli, 
					 const Jstr &quiet, int echo, int saveNewline, const Jstr &username );

void printit( FILE *outf, const char *fmt, ... );
void handleQueryExpect( JaguarCPPClient& jcli );
void handleReplyExpect( JaguarCPPClient& jcli, int numError, int numOK );

int expectType; // 1: rows  2: message
int expectCorrectRows;
int expectWordSize;
Jstr expectName;
double expectValue;
Jstr expectValueStr;
int expectErrorRows;
bool lastCmdIsExpect = false;
Jstr lastMsg;
Jstr expectMessage;
JagVector<Jstr> expectFileVec;
Jstr expectString;


//  jcli -u test -p test -h 128.22.22.3:2345 -d mydb
int main(int argc, char *argv[])
{
	//char *pcmd;
	int rc;
	JaguarCPPClient jcli;
	Jstr username, passwd, host, port, dbname, exclusive, fullconnect; 
	Jstr sqlcmd, mcmd, clearex, vvdebug, quiet, keepNewline, sqlFile;
	int  echo;
	//int  isEx = 0;
	int  saveNewline = 1;

	host = "127.0.0.1";
	port = "8888";
	username = "";
	passwd = "";
	dbname = "test";
	echo = 0;
	fullconnect = "yes";
	clearex = "no";
	vvdebug = "no";
	quiet = "no";
	// keepNewline = "yes";
	keepNewline = "no";

	JagNet::socketStartup();

	Jstr argPort, unixSocket;
	JagClock clock;
	parseArgs( argc, argv, username, passwd, host, argPort, dbname, sqlcmd, echo, exclusive, 
			   fullconnect,  mcmd, clearex, vvdebug, quiet, keepNewline, sqlFile );
	// printf("u=%s p=%s h=%s port=%s dbname=%s\n", username.c_str(), passwd.c_str(), host.c_str(), port.c_str(), dbname.c_str() );
	// printf("sqlcmd=[%s]\n", sqlcmd.c_str() );
	if ( argPort.size()>0 ) {
		port = argPort;
	}

	char *p = NULL;
	if ( username.size() < 1 ) {
		p = getenv("USERNAME");
		if ( p ) { username = p; }
		if ( username.size() < 1 ) {
			username = "test";
		}
	}

	if ( passwd.size() < 1 ) {
		p = getenv("PASSWORD");
		if ( p ) { passwd = getenv("PASSWORD"); }
	}
	if ( passwd.length() < 1 ) {
        printit( jcli._outf, "Password: ");
		getPassword( passwd );
	}

	if ( passwd.length() < 1 ) {
        printit( jcli._outf, "Please enter the correct password.\n");
		usage( argv[0] );
        exit(1);
	}

	unixSocket = "src=jql";
	if ( exclusive == "yes" ) {
		unixSocket += "/exclusive=yes";
	}

	if ( clearex == "yes" ) {
		unixSocket += "/clearex=yes";
	}

	if ( fullconnect == "yes" ) {
		jcli.setFullConnection( true );
	} else {
		jcli.setFullConnection( false );
	}

	if ( vvdebug == "yes" ) {
		jcli.setDebug( true );
		_debug = 1;
	} else {
		jcli.setDebug( false );
		_debug = 0;
	}

	// put back new line in stdin
	if ( keepNewline == "yes" ) {
		saveNewline = 1;
	} else {
		saveNewline = 0;
	}

	//d("c8293 connect host=[%s] port=[%s] user=[%s] pass=[%s]\n", host.c_str(), port.c_str(), username.c_str(), passwd.c_str() );
    if ( quiet != "yes" ) {
        printit( jcli._outf, "Connecting to %s:%s with user %s ...\n", host.c_str(), port.c_str(), username.c_str() );
    }

    if ( ! jcli.connect( host.c_str(), port.toUshort(), username.c_str(), passwd.c_str(), dbname.c_str(), unixSocket.c_str(), 0 ) ) {
        printit( jcli._outf, "Error connect to [%s:%s/%s] [%s]\n", host.c_str(), port.c_str(), dbname.c_str(), jcli.error() );
        // printf( "%s\n", jcli.getMessage() );
        printit( jcli._outf, "Please make sure you have provided the correct username, password, database, and host:port\n");
        jcli.close();
		usage( argv[0] );
        exit(1);
    }

	g_param.username = username;
	g_param.passwd = passwd;
	g_param.dbname = dbname;
	g_param.host = host;
	g_param.port = port;
	g_param.session = jcli.getSession();

	// setup sig handler SIGINT  ctrl-C entered by user
	#ifndef _WINDOWS64_
	signal( SIGHUP, SIG_IGN );
	signal( SIGCHLD, SIG_IGN);
	#endif

	if ( sqlcmd.length() > 0 ) {
    	rc = queryAndReply( jcli, sqlcmd.c_str(), quiet );
		if ( ! rc ) {
			printit( jcli._outf, "Command [%s] failed. %s\n", sqlcmd.c_str(), jcli.error() );
		}
		return 1;
	}

	if ( mcmd.length() > 1 ) {
		JagStrSplit sp( mcmd, ';', true );
		for ( int i = 0; i < sp.length(); ++i ) {
    		queryAndReply( jcli, sp[i].c_str(), quiet );
		}
		return 1;
	}

	//////// send hello
    rc = queryAndReply( jcli, "hello", quiet, true );
    if ( ! rc ) {
        jcli.close( );
		return 1;
    }

	/////// run commands
	executeCommands( clock, sqlFile, jcli, quiet, echo, saveNewline, username );
    dn("c302091 executeCommands done jcli.close()...");
	jcli.close();

    dn("c302094  socketCleanup ...");
	JagNet::socketCleanup();
    dn("c302094  socketCleanup done");
    return 0;
}

int  parseArgs( int argc, char *argv[], Jstr &username, Jstr &passwd, 
				Jstr &host, Jstr &port, Jstr &dbname, Jstr &sqlcmd,
				int &echo, Jstr &exclusive, Jstr &fullconnect, Jstr &mcmd, Jstr &clearex,
				Jstr & vvdebug, Jstr &quiet, Jstr &keepNewline, Jstr &sqlFile )
{
	int i = 0;
	char *p;

	for ( i = 1; i < argc; ++i )
	{
		if ( 0 == strcmp( argv[i], "-u" ) ) {
			if ( (i+1) <= (argc-1) ) {
				username = argv[i+1];
			} 
		} else if ( 0 == strcmp( argv[i], "-p"  )  ) {
			if ( (i+1) <= (argc-1) && *argv[i+1] != '-' ) {
				passwd = argv[i+1];
			} 
		} else if ( 0 == strcmp( argv[i], "-d"  )  ) {
			if ( (i+1) <= (argc-1) ) {
				dbname = argv[i+1];
			} 
		} else if ( 0 == strcmp( argv[i], "-h"  )  ) {
			if ( (i+1) <= (argc-1) ) {
				if ( NULL != (p=strchr( argv[i+1], ':')) ) {
					if ( argv[i+1][0] == ':' ) {
						++p;
						if ( *p != '\0' ) {
							port = p;
						}
					} else {
    					p = strtok( argv[i+1], ":");
    					host = p;
    					p = strtok( NULL,  ":");
    					if ( p ) {
    						port = p;
    					}
					}
				} else {
					host = argv[i+1];
				}
			}
		} else if ( 0 == strcmp( argv[i], "-e"  )  ) {
			if ( (i+1) <= (argc-1) ) {
				sqlcmd = argv[i+1];
			} 
		} else if ( 0 == strcmp( argv[i], "-m"  )  ) {
			if ( (i+1) <= (argc-1) ) {
				mcmd = argv[i+1];
			} 
		} else if ( 0 == strcmp( argv[i], "-x"  )  ) {
			if ( (i+1) <= (argc-1) ) {
				exclusive = argv[i+1];
			}
		} else if ( 0 == strcmp( argv[i], "-cx"  )  ) {
			if ( (i+1) <= (argc-1) ) {
				clearex = argv[i+1];
			} 
		} else if ( 0 == strcmp( argv[i], "-a"  )  ) {
			if ( (i+1) <= (argc-1) ) {
				fullconnect = argv[i+1];
			} 
		} else if ( 0 == strcmp( argv[i], "-f"  )  ) {
			if ( (i+1) <= (argc-1) ) {
				sqlFile = argv[i+1];
			} 
		} else if ( 0 == strcmp( argv[i], "-v"  )  ) {
			echo = 1;
		} else if ( 0 == strcmp( argv[i], "-vv"  )  ) {
			vvdebug = "yes";
		} else if ( 0 == strcmp( argv[i], "-q"  )  ) {
			quiet = "yes";
		} else if ( 0 == strcmp( argv[i], "-n"  )  ) {
			// keepNewline = "no";
			keepNewline = "yes";
		}
	}

	return 1;
}

// 1: OK
// 0: error
int queryAndReply( JaguarCPPClient &jcli, const char *query, const Jstr &quiet, bool ishello ) 
{
	const char *p;
    int rc = jcli.query( query );
    if ( ! rc ) {
        printit(jcli._outf, "E3020 Error query [%s] rc=%d\n", query, rc );
		return 0;
    }
    
    while ( jcli.reply( false, true) ) {
		p = jcli.getMessage();
		if ( ! p ) continue;
		if ( ishello ) {
            if ( quiet != "yes" ) {
       		    printit(jcli._outf, "%s / Client %s\n\n", p, jcli._version.c_str() );
            }
			JagStrSplit sp(p, ' ', true );
			if ( sp.length() < 3 || sp[3] != jcli._version ) {
				printit( jcli._outf, "Jaguar server version and client version do not match, exit\n");
				jcli.close();
				exit ( 1 );
			} 
		} else {
       		printit(jcli._outf, "%s\n", p );
		}
    } 
	if ( jcli.hasError() ) {
		if ( jcli.error() ) {
			printit( jcli._outf, "%s\n", jcli.error() );
		}
	}

	return 1;
}

void usage( const char *prog )
{
	printf("\n");
	printf("%s [-h HOST:PORT] [-u USERNAME] [-p PASSWORD] [-d DATABASE] [-e COMMAND] [-f sqlFile] [-v FLAG] [-x FLAG] [-cx FLAG] [-a FLAG] [-q] [-n]\n", prog  );
	printf("    [-h HOST:PORT]   ( IP address and port number of the server. Default: 127.0.0.1:8888 )\n");
	printf("    [-u USERNAME]    ( User name in Jaguar. If not given, uses USERNAME environment variable. Finally: test )\n");
	printf("    [-p PASSWORD]    ( Password of USERNAME. If not given, uses PASSWORD environment variable. Finally, prompted to provide )\n");
	printf("    [-d DATABASE]    ( Database name. Default: test )\n");
	printf("    [-e \"command\"]   ( Execute some command. Default: none )\n");
	printf("    [-f INPUTFILE]   ( Execute command from INPUTFILE. Default: standard input)\n");
	printf("    [-v yes/no]      ( Echo input command. Default: no )\n");
	printf("    [-x yes/no]      ( Exclusive login mode for admin. Default: no )\n");
	printf("    [-cx yes]        ( Clear the exclusive login lock inside the server. Default: no )\n");
	printf("    [-a yes/no]      ( Require successful connection to all servers. Default: yes )\n");
	printf("    [-q]             ( Quiet mode. Default: no  )\n");
	printf("    [-n]             ( Keep newline character in command. Default: no )\n");
	printf("\n");
}

void printResult( const JaguarCPPClient &jcli, const JagClock &clock, jagint cnt )
{	
	char val[32];
	jagint millisec = clock.elapsed(); // millisecs
	float  fs;
	if ( millisec < 1   ) {
		fs = (float)clock.elapsedusec()/1000000.0;
		sprintf(val, "%.6f", fs );
	} else {
		fs = (float)millisec/1000.0;
		sprintf(val, "%.4f", fs );
	}

	if (  cnt >= 0 ) {
		if ( 1 == cnt ) printf("Done in %s seconds (1 row) \n", val );
		else  printf("Done in %s seconds (%lld rows)\n", val, cnt );
	} else {
		printf("Done in %s seconds\n", val );
	}

    fflush( stdout );
}


// return -1 for fatal eror
// 0: for OK
int executeCommands( JagClock &clock, const Jstr &sqlFile, JaguarCPPClient &jcli, 
                     const Jstr &quiet, int echo, int saveNewline, const Jstr &username )
{
	char *pcmd;
	Jstr sqlcmd;
	FILE *infp = stdin;
	if ( sqlFile.size() > 0 ) {
		infp = jagfopen( sqlFile.c_str(), "rb" );
		if ( ! infp ) {
			printit( jcli._outf, "Error open [%s] for commands\n", sqlFile.c_str());
			return -1;
		}
	}

	Jstr pass1, pass2;
	Jstr passwd;
	int rc = 0;

	lastCmdIsExpect = false;
	expectCorrectRows = -1;
	expectErrorRows = -1;
	expectWordSize = -1;
	int numOK = 0;
	int numError = 0;

	jagint cnt;
    while ( true ) {
		cnt = 0;

		if ( quiet != "yes" ) {
			printf("jaguar:%s> ", jcli._dbname.c_str() ); fflush( stdout );
		}

		if ( ! jcli.getSQLCommand( sqlcmd, echo, infp, saveNewline ) ) {
			break;
		}

		if ( sqlcmd.length() < 1 || ';' == *(sqlcmd.c_str() ) ) {
			continue;
		}

		pcmd = (char*)sqlcmd.c_str();
		while ( jagisspace(*pcmd) ) ++pcmd;

		if ( echo ) {
			printit( jcli._outf, "%s\n", pcmd );
		}

		if (  pcmd[0] == '#' ) { continue; }
		if (  pcmd[0] == 'q' ) { break; } // quit
		if ( strncasecmp( pcmd, "exit", 4 ) == 0 ) { break; }

		if ( pcmd[0] == 27 ) { continue; } 
		if ( strncasecmp( pcmd, "changepass", 9 ) == 0 ) {
			Jstr tcmd1 = trimTailChar( pcmd, ';' );
			Jstr tcmd2 = trimTailChar( tcmd1, ' ' );
			const char *passtr = NULL;
			if ( ! (passtr = strchr( tcmd2.c_str(), ' ')) ) {
    			// no password is provided
    			printf("New password: "); 
				fflush(stdout);
    			getPassword( pass1 );

    			printf("New password again: ");
				fflush(stdout);
    			getPassword( pass2 );
    			if ( pass1 != pass2 ) {
    				printf("Entered passwords do not match. Please try again\n");
					fflush(stdout);
    				continue;
    			}
    			sqlcmd = "changepass " + username + ":" + pass1;
    			pcmd = (char*) sqlcmd.c_str();
			} else {
				while ( isspace(*passtr) ) ++passtr;
				if ( strchr(passtr, ':') ) {
					sqlcmd = Jstr("changepass ") + passtr;
				} else {
					sqlcmd = Jstr("changepass ") + username + ":" + passtr;
				}
				pcmd = (char*) sqlcmd.c_str();
			}
		} else if ( (strncasecmp( pcmd, "createuser", 9 )==0 || strncasecmp( pcmd, "create user", 10 )==0  )
				    && ! strchr(pcmd, ':') ) {
			if ( strchr(pcmd, ':') ) {
				JagStrSplit sp( pcmd, ':' );
				if ( sp[1].length() < 12 ) {
					printf("Password is too short. It must have at least 12 letters. Please try again\n");
					fflush(stdout);
					continue;
				}
			} else {
    			// no password is provided
    			Jstr cmd = trimTailChar( sqlcmd, ';' );
    			JagStrSplit sp( cmd, ' ', true );
				int t;
				if ( strncasecmp( pcmd, "createuser", 9 )==0 ) {
					t = 1;
    				if ( sp.length() < 2 ) {
    					printf("E6300 Error createuser syntax. Please try again. (createuser YOUR_USER_NAME;)\n");
						fflush(stdout);
    					continue;
    				}
				} else {
					t = 2;
    				if ( sp.length() < 3 ) {
    					printf("E6302 Error createuser syntax. Please try again. (create user YOUR_USER_NAME;)\n");
						fflush(stdout);
    					continue;
    				}
				}

    			printf("New user password: ");
				fflush(stdout);

    			getPassword( pass1 );

    			printf("New user password again: ");
				fflush(stdout);
    			getPassword( pass2 );

    			if ( pass1 != pass2 ) {
    				printf("Entered passwords do not match. Please try again\n");
					fflush(stdout);
    				continue;
    			}

    			if ( pass1.length() < 12 ) {
    				printf("Password is too short. It must have at least 12 letters. Please try again\n");
					fflush(stdout);
    				continue;
    			}
    
    			if ( 1 == t ) {
    				sqlcmd = "createuser " + sp[1] + ":" + pass1;
				} else {
    				sqlcmd = "createuser " + sp[2] + ":" + pass1;
				}

    			pcmd = (char*) sqlcmd.c_str();
			}
		} else if ( strncasecmp( pcmd, "sleep ", 6 ) == 0 ) {
			JagStrSplit sp( pcmd, ' ', true );

			printit( jcli._outf, "sleeping %d seconds ...\n", atoi(sp[1].c_str()) );
			jagsleep(atoi(sp[1].c_str()), JAG_SEC);
			printit( jcli._outf, "sleeping %d seconds done\n", atoi(sp[1].c_str()) );
			continue;
		} else if ( strncasecmp( pcmd, "!", 1 ) == 0 ) {
			++pcmd;
			while ( isspace(*pcmd) ) ++pcmd;
			if ( *pcmd == '\r' || *pcmd == '\n' || *pcmd == ';' || *pcmd == '\0' ) continue;
			Jstr outs = psystem(pcmd);
			printit( jcli._outf, "%s\n", outs.c_str() );
			continue;
		} else if ( strncasecmp( pcmd, "rmfile", 6 ) == 0 ) {
			while ( isspace(*pcmd) ) ++pcmd;
			if ( *pcmd == '\r' || *pcmd == '\n' || *pcmd == ';' || *pcmd == '\0' ) continue;
            trimEndChar( pcmd, ';');
            jagunlink( pcmd );
			printit( jcli._outf, "%s is removed\n", pcmd );
			continue;
		} else if ( strncasecmp( pcmd, "shell", 5 ) == 0 ) {
			pcmd += 5;  // skip shell word
			while ( isspace(*pcmd) ) ++pcmd;
			if ( *pcmd == '\r' || *pcmd == '\n' || *pcmd == ';' || *pcmd == '\0' ) continue;
			Jstr outs = psystem(pcmd);
			printit( jcli._outf, "%s\n", outs.c_str() );
			continue;
		} else if ( strncasecmp( pcmd, "source ", 7 ) == 0 ) {
			JagStrSplit sp( pcmd, ' ', true );
			if ( sp.length() < 2 ) {
				printit( jcli._outf, "Error: a command file must be provided.\n");
			} else {
				rc = executeCommands( clock, trimTailChar(sp[1], ';'), jcli, quiet, echo, saveNewline, username );
				if ( rc < 0 ) {
					printit( jcli._outf, "Execution error in file %s\n", sp[1].c_str() ); 
				}
			}
			continue;
		} else if ( strncasecmp( pcmd, "@", 1 ) == 0 ) {
			if ( strlen(pcmd) >= 3 ) {
				rc = executeCommands( clock, trimTailChar(pcmd+1, ';'), jcli, quiet, echo, saveNewline, username );
				if ( rc < 0 ) {
					printit( jcli._outf, "Execution error in file %s\n", pcmd+1 ); 
				}
			}
			continue;
		} else if ( strncasecmp( pcmd, "expect", 6 ) == 0 ) {
			// expect rows 12;
			// expect okmsg "OK  done"
			// expect errmsg "E2220 error"
			// expect words  "test.tab1 key1 col3"
			// sql command ...;
			JagStrSplit sp( pcmd, ' ', true );
			if ( sp.length() < 3 ) {
				printit( jcli._outf, "Error: expect correct/error rows N\n"); 
			} else {
				if ( sp[1].caseEqual("rows") ) {
					expectCorrectRows = jagatoi( sp[2].c_str() );
					if ( expectCorrectRows < 0 ) {
						printit( jcli._outf, "Error: rows N must be >= 0\n"); 
						continue;
					}
					lastCmdIsExpect = true;
					expectType = 1;
				} else if ( sp[1].caseEqual("errmsg") ) {
					const char *p = strcasestr( pcmd, "errmsg" );
					p += strlen("errmsg");
					while ( *p != '\0' && *p != '"' ) ++p;
					if ( *p == '\0' ) {
						printit( jcli._outf, "Error: expect syntax. Missing start \"\n"); 
						continue;
					}
					++p; // past "
					const char *pstart = p;
					while ( *p != '\0' && *p != '"' ) ++p;
					if ( *p == '\0' ) {
						printit( jcli._outf, "Error: expect syntax. Missing end \"\n"); 
						continue;
					}
					expectMessage = Jstr(pstart, p-pstart);
					lastCmdIsExpect = true;
					expectType = 2;
				} else if ( sp[1].caseEqual("okmsg") ) {
					const char *p = strcasestr( pcmd, "okmsg" );
					p += strlen("okmsg");
					while ( *p != '\0' && *p != '"' ) ++p;
					if ( *p == '\0' ) {
						printit( jcli._outf, "Error: expect syntax. Missing start \"\n"); 
						continue;
					}
					++p; // past "
					const char *pstart = p;
					while ( *p != '\0' && *p != '"' ) ++p;
					if ( *p == '\0' ) {
						printit( jcli._outf, "Error: expect syntax. Missing end \"\n"); 
						continue;
					}
					expectMessage = Jstr(pstart, p-pstart);
					lastCmdIsExpect = true;
					expectType = 3;
				} else if ( sp[1].caseEqual("words") ) {
					const char *p = strcasestr( pcmd, "words" );
					p += strlen("words");
					while ( *p != '\0' && *p != '"' ) ++p;
					if ( *p == '\0' ) {
						printit( jcli._outf, "Error: expect syntax. Missing start \"\n"); 
						continue;
					}
					++p; // past "
					const char *pstart = p;
					while ( *p != '\0' && *p != '"' ) ++p;
					if ( *p == '\0' ) {
						printit( jcli._outf, "Error: expect syntax. Missing end \"\n"); 
						continue;
					}
					expectMessage = Jstr(pstart, p-pstart);
					lastCmdIsExpect = true;
					expectType = 4;
				} else if ( sp[1].caseEqual("errors") ) {
					const char *p = strcasestr( pcmd, "errors" );
					p += strlen("errors");
					while ( *p != '\0' && *p != '"' ) ++p;
					if ( *p == '\0' ) {
						printit( jcli._outf, "Error: expect syntax. Missing start \"\n"); 
						continue;
					}
					++p; // past "
					const char *pstart = p;
					while ( *p != '\0' && *p != '"' ) ++p;
					if ( *p == '\0' ) {
						printit( jcli._outf, "Error: expect syntax. Missing end \"\n"); 
						continue;
					}
					expectMessage = Jstr(pstart, p-pstart);
					lastCmdIsExpect = true;
					expectType = 5;
					printf("c3333030303 errors expectType = 5 expectMessage=[%s]\n", expectMessage.s() );
				} else if ( sp[1].caseEqual("wordsize") ) {
					expectWordSize = jagatoi( sp[2].c_str() );
					if ( expectWordSize < 0 ) {
						printit( jcli._outf, "Error: wordsize N must be >= 0\n"); 
						continue;
					}
					lastCmdIsExpect = true;
					expectType = 6;
				} else if ( sp[1].caseEqual("nowords") ) {
					const char *p = strcasestr( pcmd, "nowords" );
					p += strlen("nowords");
					while ( *p != '\0' && *p != '"' ) ++p;
					if ( *p == '\0' ) {
						printit( jcli._outf, "Error: expect syntax. Missing start \"\n"); 
						continue;
					}
					++p; // past "
					const char *pstart = p;
					while ( *p != '\0' && *p != '"' ) ++p;
					if ( *p == '\0' ) {
						printit( jcli._outf, "Error: expect syntax. Missing end \"\n"); 
						continue;
					}
					expectMessage = Jstr(pstart, p-pstart);
					lastCmdIsExpect = true;
					expectType = 7;
				} else if ( sp[1].caseEqual("value") ) {
					if ( sp.length() < 4 ) {
						printit( jcli._outf, "Error: [expect value name 223.4]  must be given\n"); 
						continue;
					}
					expectName = sp[2];
					expectValueStr = sp[3];
					expectValue = jagatof( sp[3].c_str() );
					lastCmdIsExpect = true;
					expectType = 8;
				} else if ( sp[1].caseEqual("file") || sp[1].caseEqual("files") ) {
					if ( sp.length() < 3 ) {
						printit( jcli._outf, "Error: [expect file fpath]  must be given\n"); 
						continue;
					}
                    expectFileVec.clear();
                    for ( int i = 2; i < sp.size(); ++i ) {
					    expectFileVec.append( sp[i].trimEndChar(';') );
                    }
					lastCmdIsExpect = true;
					expectType = 9;
				} else if ( sp[1].caseEqual("string") ) {
					if ( sp.length() < 4 ) {
						printit( jcli._outf, "Error: [expect string name MYSTRING]  must be given\n"); 
						continue;
					}
					expectName = sp[2];
					expectString = sp[3];
                    expectString.trimEndChar(';');
                    if ( expectString.containsChar('"') ) {
                        expectString = trimChar( expectString, '"');
                    }
					lastCmdIsExpect = true;
					expectType = 10;
				} else {
					printit( jcli._outf, "Error: unknown expect command\"\n"); 
				}
			}
			continue;
		} else if ( strncasecmp( pcmd, "flush", 5 ) == 0 ) {
        	while ( jcli.reply( false, false) ) {
				jcli.printRow();
				Jstr s = jcli.getMessage();
				printf("c33300 s=[%s]\n", s.s() );
			}
			printf("c33300 end\n");
			continue;
		}

		clock.start();	
        rc = jcli.query( pcmd );

        if ( ! rc ) {
			if ( jcli.hasError() ) {
				printit( jcli._outf, "%s\n", jcli.error() );
			}

			if ( lastCmdIsExpect  ) {
				handleQueryExpect( jcli );
			}
			continue;
        }
		
		numOK = 0;
		numError = 0;

		int dbgn = 0;
        while ( jcli.reply( false, true) ) {
			jcli.printRow();
			++dbgn;

			if ( ! jcli.hasError() ) {
				++numOK; 
				++cnt;
			} else {
				++numError;
			}

			jcli.printAll();

			if ( lastCmdIsExpect ) {
				lastMsg = jcli.getMessage();
			}

        } 
		// For inserts: reply() is always false
		dn("q93030330 dbgn=%d", dbgn);

		// Get auto-generated UUID if it is the first key
		// Jstr uuid = jcli.getLastUuid();

		if ( jcli.hasError() ) {
			printit( jcli._outf, "%s\n", jcli.error() );
		}

		while ( jcli.printAll() ) { 
			++numOK; 
			++cnt;
		}

		jcli.flush();
		if ( lastCmdIsExpect ) {
			handleReplyExpect( jcli, numError, numOK );
		}

		if ( strncasecmp( pcmd, "load", 4 ) == 0 ) {
			printit( jcli._outf, "%s\n", jcli.status() );
		}

        jcli.freeResult();
		clock.stop();
		rc = clock.elapsed();

		if ( quiet == "yes" ) {
			// nothing done
		} else {
			if ( jcli._queryCode == JAG_SELECT_OP ) {
				printResult( jcli, clock, cnt );
			} else {
				printResult( jcli, clock, -1 );
			}
		}
    }

	if ( infp ) jagfclose( infp );

	return 0;
}

void printit( FILE *outf, const char *format, ...)
{
	FILE *out = outf;
	if ( ! out ) out = stdout;

	va_list args;
	va_start(args, format);
	vfprintf(out, format, args );
	fflush( out );
	va_end( args);
}

void handleQueryExpect( JaguarCPPClient& jcli )
{
	if ( ! lastCmdIsExpect ) return;

	if ( expectType==2 ) {
		// errmsg
		if ( expectMessage.caseMatch( jcli.error(), " []:;]" ) ) {
			printit( jcli._outf, "RESULT=PASS (expected_msg=[%s])\n", expectMessage.s() );
		} else {
			printit( jcli._outf, "RESULT=FAIL (expected_msg=[%s] actual_error=[%s])\n", expectMessage.s(), jcli.error() );
		}
	} else if ( expectType==5 ) {
		// errors "k1 k3 k5"
		const char *msg = jcli.error();
		bool ok =false;
		if ( msg != NULL && *msg != '\0' ) {
			Jstr res(msg);
			if ( res.containAllWords( expectMessage.c_str(), " ()[]:;,]", true) ) {
				ok = true;
			} else {
				ok = false;
			}
		} else {
			ok = false;
		}

		if ( ok ) {
			printit( jcli._outf, "RESULT=PASS  (expected_error=[%s])\n", expectMessage.s() );
		} else {
			printit( jcli._outf, "RESULT=FAIL  (expected_error=[%s] real_error=[%s]) FAIL\n", expectMessage.s(), msg );
		}
	} else {
	}

	lastCmdIsExpect = false;
	expectCorrectRows = -1;
	expectErrorRows = -1;
	expectWordSize = -1;
	lastMsg = "";
}

void handleReplyExpect( JaguarCPPClient& jcli, int numError, int numOK )
{
	if ( ! lastCmdIsExpect ) {
		return;
	}

	if ( expectType == 1 ) {
		if ( expectErrorRows >= 0 ) {
			if ( expectErrorRows == numError ) {
				printit( jcli._outf, "RESULT=PASS  (expected_error=%d  real_error=%d)\n", expectErrorRows, numError );
			} else {
				printit( jcli._outf, "RESULT=FAIL  (expected_error=%d  real_error=%d error=[%s])\n", expectErrorRows, numError, jcli.error() );
			}
		} else if ( expectCorrectRows >= 0 ) {
			if ( expectCorrectRows == numOK ) {
				printit( jcli._outf, "RESULT=PASS  (expected_correct=%d  real_correct=%d)\n", expectCorrectRows, numOK );
			} else {
				printit( jcli._outf, "RESULT=FAIL  (expected_correct=%d  real_correct=%d)\n", expectCorrectRows, numOK );
			}
		}
	} else if ( expectType == 2 ) {
		// errmsg
		const char *err = jcli.error();
		if ( expectMessage.caseMatch(err , " []:;]" )) {
			printit( jcli._outf, "RESULT=PASS  (expected_msg=[%s])\n", expectMessage.s() );
		} else {
			printit( jcli._outf, "RESULT=FAIL  (expected_msg=[%s] error=[%s])\n", expectMessage.s(), err );
		}
	} else if ( expectType == 3 ) {
		// okmsg
		const char *msg = jcli.message();
		if ( expectMessage.caseMatch(msg, " []:;]") ) {
			printit( jcli._outf, "RESULT=PASS  (expected_msg=[%s])\n", expectMessage.s() );
		} else {
			printit( jcli._outf, "RESULT=FAIL  (expected_msg=[%s] real_msg=[%s]) FAIL\n", expectMessage.s(), msg );
		}
	} else if ( expectType == 4 ) {
		// words "k1 k3 k5"
		const char *msg = jcli.message();
		bool ok =false;

		if ( msg != NULL && *msg != '\0' ) {
			Jstr res(msg);
			if ( res.containAllWords( expectMessage.c_str(), " ()[]:;,]", true) ) {
				ok = true;
			} else {
				ok = false;
			}
		} else {
			ok = false;
		}

		if ( ok ) {
			printit( jcli._outf, "RESULT=PASS  (expected_words=[%s])\n", expectMessage.s() );
		} else {
			printit( jcli._outf, "RESULT=FAIL  (expected_words=[%s] real_msg=[%s]) FAIL\n", expectMessage.s(), msg );
		}
	} else if ( expectType == 5 ) {
		// errors "k1 k3 k5"
		const char *msg = jcli.error();
		bool ok =false;
		if ( msg != NULL && *msg != '\0' ) {
			Jstr res(msg);
			if ( res.containAllWords( expectMessage.c_str(), " ()[]:;,]", true) ) {
				ok = true;
			} else {
				ok = false;
			}
		} else {
			ok = false;
		}

		if ( ok ) {
			printit( jcli._outf, "RESULT=PASS  (expected_errors=[%s])\n", expectMessage.s() );
		} else {
			printit( jcli._outf, "RESULT=FAIL  (expected_errors=[%s] real_error=[%s]) FAIL\n", expectMessage.s(), msg );
		}
	} else if ( expectType == 6 ) {
		const char *msg = jcli.message();
		int len = strlen(msg);
		if ( len == expectWordSize ) {
			printit( jcli._outf, "RESULT=PASS  (expected_size=[%d])\n", expectWordSize );
		} else {
			printit( jcli._outf, "RESULT=FAIL  (expected_size=[%d] real_size=[%d] real_msg=[%s]) FAIL\n", expectWordSize, len, msg);
		}
	} else if ( expectType == 7 ) {
		// nowords "k1 k3 k5"
		const char *msg = jcli.message();
		bool ok =true;
		if ( msg != NULL && *msg != '\0' ) {
			Jstr res(msg);
			if ( res.containAnyWord( expectMessage.c_str(), " []:;]", true) ) {
				ok = false;
			} else {
				ok = true;
			}
		} else {
			ok = true;
		}

		if ( ok ) {
			printit( jcli._outf, "RESULT=PASS  (not_expected_words=[%s])\n", expectMessage.s() );
		} else {
			printit( jcli._outf, "RESULT=FAIL  (not_expected_words=[%s] real_msg=[%s]) FAIL\n", expectMessage.s(), msg );
		}
	} else if ( expectType == 8 ) {
        double val = 0;
        bool OK = false;
		int rc = jcli.getDouble( expectName.s(), &val);
		if ( rc ) {
            if ( strchr( expectValueStr.s(), '.' ) ) { 
                if ( jagEQ( val, expectValue ) ) {
                    OK = true;
                } 
            } else {
                if ( long(val) == long(expectValue) ) {
                    OK = true;
                } 
            }
		} 

        if ( OK) {
			printit( jcli._outf, "RESULT=PASS  (expected_value=[%f])\n", expectValue );
        } else {
			printit( jcli._outf, "RESULT=FAIL  (expected_value=[%f] real_value=[%f] FAIL\n", expectValue, val );
        }
	} else if ( expectType == 9 ) {
		// expect file <filepathfile> <file2>
		bool ok =true;
        Jstr  bads;
        for ( int i = 0; i < expectFileVec.size(); ++i ) {
            if ( 0 != ::access(expectFileVec[i].s(), R_OK) ) {
                ok = false;
                bads += Jstr("{") + expectFileVec[i] + "} " + strerror( errno );
            }
        }

        Jstr allfiles;
        for ( int i = 0; i < expectFileVec.size(); ++i ) {
            allfiles += expectFileVec[i] + " ";
        }

		if ( ok ) {
			printit( jcli._outf, "RESULT=PASS  (%s exist)\n", allfiles.s() );
		} else {
			printit( jcli._outf, "RESULT=FAIL  (%s not exist) FAIL\n",  bads.s() );
		}
	} else if ( expectType == 10 ) {
        bool OK = false;
		char *pval = jcli.getValue( expectName.s() );
        Jstr valStr;
        if ( pval ) {
            valStr = pval;
            free( pval );
        }

        if (  expectString == valStr ) {
            OK = true;
        }

        if ( OK) {
			printit( jcli._outf, "RESULT=PASS  (expected_string=[%s])\n", expectString.s() );
        } else {
			printit( jcli._outf, "RESULT=FAIL  (expected_string=[%s] real_string=[%s] FAIL\n", expectString.s(), valStr.s() );
        }
	} else {
		printit( jcli._outf, "Expect error: unknown expect type %d\n", expectType);
	}

	lastCmdIsExpect = false;
	expectCorrectRows = -1;
	expectErrorRows = -1;
	lastMsg = "";

}
