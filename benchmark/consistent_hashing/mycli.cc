#include <cassert>
#include <iostream>
#include <thread>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sockpp/tcp_connector.h>

using string = std::string;
using TcpConn = sockpp::tcp_connector;

void usage( const char *prog)
{
    printf("Usage: %s <destHost> <cmd> [options]\n", prog );
    printf("                      cmd: file <server.cfg> (add more nodes)\n");
    printf("                      cmd: scale             (redistribute data)\n");
    printf("                      cmd: send <N> (send  N records)\n");
    printf("                      cmd: count             (ask server to report number of data records)\n");
}

string makeKVData( const string &cmd, const string &key, const string &value )
{
    string data = cmd;

    char buf[5];
    memset(buf, 0, 5 );

    sprintf(buf, "%04ld", key.size() );
    data += buf;
    data += key;

    sprintf(buf, "%04ld", value.size() );
    data += buf;
    data += value;

    return data;
}

string makeCfgData( const string &filePath )
{
    string data;

    FILE *fp = fopen(filePath.c_str(), "r");
    if ( ! fp ) {
        return "";
    }

    data = "F";

    char line[2048];
    string fdata;
    memset(line, 0, 2048 );

    while ( NULL != fgets(line, 2048, fp) ) {
        fdata += line;
        memset(line, 0, 2048 );
    }

    char buf[5];
    memset( buf, 0, 5 );
    sprintf(buf, "%04ld", fdata.size() );

    data += buf;
    data += fdata;

    fclose(fp);
    return data;
}

int main(int argc, char *argv[] )
{
    if (argc < 3 ) {
        usage( argv[0] );
        return 1;
    }

    string destHost = argv[1];

    sockpp::initialize();
    TcpConn *conn = new TcpConn({destHost, 8898});

    string cmd = argv[2];

    if ( cmd == "file" ) {
        string fpath = argv[3];
        string cfgdata = makeCfgData( fpath );
        if ( cfgdata.size() > 0 ) {
            printf("file command cfgdata=[%s] write ...\n", cfgdata.c_str() );
            fflush( stdout );
            int rc = conn->write( cfgdata.c_str(), cfgdata.size() );
            printf("file command cfgdata=[%s] write done rc=%d desthost=[%s]\n", cfgdata.c_str(), rc, destHost.c_str() );
            fflush( stdout );
        } else {
            printf("Error file command cfgdata is empty\n");
            fflush( stdout );
        }
    } else if ( cmd == "send" ) {
        string Ns = argv[3];
        int  N = atoi( Ns.c_str() );

        string key, val, kvdata;
        val = "A Hello, World! program is generally a computer program that ignores any input and outputs or displays a message similar to Hello, World!. A small piece of code in most general-purpose programming languages, this program is used to illustrate a language's basic syntax. Hello, World! programs are often the first a student learns to write in a given language, and they can also be used as a sanity check to ensure computer software intended to compile or run source code is correctly installed, and that its operator understands how to use it.";
        //val = "A Hello, World!";

        char buf[256];
        srand(time(NULL));

        time_t t1 = time(NULL);
        int  rc;
        for ( int i=0; i < N; ++i ) {
            sprintf(buf, "%ld_%06d_%d", time(NULL), rand()%1000000, i );
            key = buf;
            kvdata = makeKVData( "D", key, val );
            rc = conn->write( kvdata.c_str(), kvdata.size() );

            conn->read_n(buf, 1);
        }

        time_t t2 = time(NULL);
        printf("send %d records spent=%ld seconds key.size=%ld  val.size=%ld totalsize=%ld  networksize=%ld bytes \n", 
               N, t2-t1, key.size(), val.size(), key.size() + val.size(), kvdata.size() );
        fflush(stdout);
    }  else if ( cmd == "scale" ) {
        conn->write( "S", 1 );
    }  else if ( cmd == "count" ) {
        conn->write( "C", 1 );
    } else {
        printf("Unknown command [%s]\n", cmd.c_str() );
        fflush(stdout);
    }

    delete conn;
    return 0;
}

