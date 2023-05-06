
#include <cassert>
#include <iostream>
#include <thread>
#include <string>
#include <stdio.h>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include <stdarg.h>
#include <pthread.h>

#include <leveldb/db.h>
#include <sockpp/tcp_acceptor.h>
#include <sockpp/tcp_connector.h>

extern "C" {
#include <ketama.h>
}

using string = std::string;

using TcpConn = sockpp::tcp_connector;

ketama_continuum    g_ring;
leveldb::DB         *g_db;
string              g_myip;
std::unordered_map<string, TcpConn*> g_connmap;
string g_serverCfgFile = "ServerCfg.txt";
pthread_mutex_t     g_mutex;

string getIPaddress( const sockpp::tcp_socket &sock )
{
    sockaddr_in *p = (sockaddr_in*) sock.peer_address().sockaddr_ptr();
    return inet_ntoa( p->sin_addr );
}

void logit(bool line, const char * format, va_list args )
{
    char tmstr[48];
    struct tm result;
    struct timeval now;
    gettimeofday( &now, NULL );
    time_t tsec = now.tv_sec;
    int ms = now.tv_usec / 1000;
    gmtime_r( &tsec, &result );
    strftime( tmstr, sizeof(tmstr), "%Y-%m-%d %H:%M:%S", &result );
    char msb[5];
    sprintf(msb, ".%03d", ms);
    strcat( tmstr, msb );

    fprintf(stdout, "%s %d %ld: ", tmstr, getpid(), pthread_self()%10000 );
    vfprintf(stdout, format, args);
    if ( line ) {
        fprintf(stdout, "\n");
    }
    fflush(stdout);
}

void in(const char * format, ...)
{
    va_list args;
    va_start(args, format);
    logit(true, format, args);
    va_end(args);
}

void i(const char * format, ...)
{
    return;

    va_list args;
    va_start(args, format);
    logit(false, format, args);
    va_end(args);
}

// myip "192.168.2.111"  ipport "192.128.2.111:8898"
bool ipMatch( const string &myip, const char *ipport )
{
    if ( strstr(ipport, myip.c_str() ) ) {
        return true;
    }

    return false;
}

void getIPPort(const string &hostport, string &ip, string &port )
{
    string a = hostport;
    char *p = (char*)a.c_str();
    char *c = strchr(p, ':');
    if ( !c ) return;

    *c = '\0';
    ip = p;
    port = c+1;
}

void getMyIP()
{
    FILE *fp = fopen("myip.txt", "r");
    if ( ! fp ) {
        in("Error reading myip.txt");
        exit(1);
    }
    
    char line[256];
    memset(line, 0, 256);

    fgets(line, 256, fp);
    int len = strlen(line);
    if ( '\n' == line[len-1] ) {
        line[len-1] = '\0';
    }

    g_myip = line;
    fclose( fp );
}

void setup_connections()
{
    if ( 0 == g_ring->array ) {
        i( "Error g_ring is empty\n");
        return;
    }

    sockpp::initialize();

    int N = g_ring->numpoints;
    string host;

    mcs  *arr = (mcs*)(g_ring->array);

    string ip, port;
    std::unordered_set<string> okmap;
    for( int a = 0; a < N; a++ ) {
        host = arr[a].ip;
        getIPPort(host, ip, port);

        if ( g_myip == ip ) {
            continue;
        }

        if ( okmap.count( host ) > 0 ) {
            continue;
        }

        in_port_t pt = (in_port_t)atoi(port.c_str());

        i("Connect to %s:%d ...\n", ip.c_str(), pt);

        try {
            TcpConn *conn = new TcpConn({ip, pt});
            g_connmap.emplace( host, conn );
            okmap.emplace( host);
        } catch ( ... ) {
            i("Connect to %s:%d got error, retry \n", ip.c_str(), pt);
            sleep(1);
        }
    }
}

void destroy_connections()
{
    if ( 0 == g_ring->array ) {
        return;
    }

    TcpConn *conn;
    std::unordered_map<string, TcpConn*>::iterator it = g_connmap.begin();

    while (it != g_connmap.end()) {
        conn = it->second;
        delete conn; 
        it = g_connmap.erase(it);
    }
}

int countData()
{
    int cnt = 0;
    leveldb::Iterator* it = g_db->NewIterator(leveldb::ReadOptions());

    time_t t1 = time(NULL);

    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        ++cnt;
    }

    delete it;
    time_t t2 = time(NULL);
    in("Counted %d records spent %lu seconds", cnt, t2 - t1 );
    return cnt;
}


void runScale()
{
    in("runScale ...");
    pthread_mutex_lock ( &g_mutex );

    int cnt1 = countData();

    int cnt = 0;
    leveldb::Iterator* it = g_db->NewIterator(leveldb::ReadOptions());

    string      key, val, data;
    leveldb::WriteOptions write_options;
    write_options.sync = true;
    char buf[5];
    buf[4] = '\0';

    time_t t1 = time(NULL);

    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        key = it->key().ToString();
        mcs* m = ketama_get_server( (char*)key.c_str(), g_ring );

        if ( ! ipMatch(g_myip, m->ip) ) {
            val = it->value().ToString();

            TcpConn *conn = g_connmap[m->ip];
            data = string("W");

            sprintf(buf, "%04ld", key.size() );
            data += buf;
            data += key;

            sprintf(buf, "%04ld", val.size() );
            data += buf;
            data += val;

            i("s2020 write data=[%s] ...\n", data.c_str() );
            conn->write( data.c_str(), data.size() );
            i("s2020 write data done, read 1 byte ...\n" );
            conn->read_n( buf,  1 );
            i("s2020 write data done, read 1 byte done\n" );

            g_db->Delete( write_options, it->key().ToString() );
            ++cnt;
        } else {
        }
    }

    delete it;
    time_t t2 = time(NULL);

    int cnt2 = countData();

    in("Moved %d records spent %lu seconds  rate=%d/second", cnt, t2 - t1, cnt/(t2-t1) );
    in("Initial count cnt1=%d - moved cnt=%d ==> remaining=%d =?= finalCnt2=%d", cnt1, cnt, cnt1-cnt, cnt2);

    pthread_mutex_unlock ( &g_mutex );
}

// return < 0 for error; 0: OK
// peer sends "Fnnnn....."  ..... is cfg data
int receiveSaveCfgFile( sockpp::tcp_socket &sock ) 
{
    string servFileData;
    char b[5];
    b[4] = '\0';

    i("receiveSaveCfgFile(), read 4 bytes ...\n" );
    int len = sock.read_n(b, 4);
    if ( len <= 0 ) { return -5; }

    int sz = atoi(b);
    i("receiveSaveCfgFile(), read 4 bytes done sz=%d\n", sz ); 

    char *buf = (char*)malloc(sz+1);
    memset(buf, 0, sz+1);

    i("receiveSaveCfgFile(), read %d bytes ...\n", sz );
    len = sock.read_n(buf, sz);
    servFileData += buf;
    free( buf );
    i("receiveSaveCfgFile(), read %d bytes done len=%d\n", sz, len );

    in("Received serverCfgFile [%s]", servFileData.c_str() );

    // save the file
    FILE *fp = fopen( g_serverCfgFile.c_str(), "w");
    if ( !fp ) {
        return -20;
    }

    fprintf(fp, "%s", servFileData.c_str() );
    fclose( fp );

    pthread_mutex_lock ( &g_mutex );

    ketama_smoke( g_ring );

    in( "ketama_roll ...");
    int rrc = ketama_roll( &g_ring, (char*)g_serverCfgFile.c_str() );
    in( "ketama_roll done rrc=%d", rrc); 

    destroy_connections();

    in( "setup_connections ..."); 
    setup_connections();

    in( "Updated serverCfgFile"); 
    pthread_mutex_unlock ( &g_mutex );

    return 0;
}

// return < 0 for error; 0: OK
// peer sends "Dnnnn...(key)...nnnn...(value)..."  ..... is key and value
int receiveDistributeData( sockpp::tcp_socket &sock )
{
    string data;
    string key, value;
    string clientIP = getIPaddress( sock );
    char  buf[1024];

    // sending to responsible server for write
    data += "W";

    char b[5];
    b[4] = '\0';

    // key size
    i( "read 4 bytes from client=[%s] ...\n", clientIP.c_str() ); 
    int len = sock.read_n(b, 4);
    i( "read 4 bytes done len=%d\n", len );
    if ( len <= 0 ) { return -10; }

    data += b;

    int sz = atoi(b);
    memset(buf, 0, 1024);

    //read in the key
    i( "read key %d bytes ...\n", sz);
    len = sock.read_n(buf, sz);
    i( "read key %d bytes done len=%d buf=[%s]\n", sz, len, buf);
    if ( len < sz ) {
        return -20;
    }

    data += buf;
    key = buf;

    // value size
    len = sock.read_n(b, 4);
    if ( len <= 0 ) {
        return -30; 
    }
    sz = atoi(b);

    data += b;

    // read in value
    memset(buf, 0, 1024);
    len = sock.read_n(buf, sz);
    if ( len < sz ) {
        return -40;
    }

    data += buf;
    value = buf;

    i("client=[%s] key=[%s] value=[%s]\n", clientIP.c_str(), key.c_str(), value.c_str() );

    // find server for the key 
    mcs* m = ketama_get_server( (char*)key.c_str(), g_ring );

    i("g_myip=[%s] dest m->ip=[%s]\n", g_myip.c_str(), m->ip );

    if ( ! ipMatch(g_myip, m->ip) ) {
        TcpConn *conn = g_connmap[m->ip];
        pthread_mutex_lock ( &g_mutex );
        len = conn->write( data.c_str(), data.size() );
        conn->read( buf,  1 );
        pthread_mutex_unlock ( &g_mutex );
    } else {
        leveldb::WriteOptions write_options;
        write_options.sync = true;
        pthread_mutex_lock ( &g_mutex );
        g_db->Put( write_options, key, value);
        pthread_mutex_unlock ( &g_mutex );
    }

    sock.write("K", 1);

    return 0;
}

// return < 0 for error; 0: OK
// peer sends "Wnnnn...(key)...nnnn...(value)..."  ..... is key and value
int receiveSaveData( sockpp::tcp_socket &sock )
{
    string servFileData;
    string key, value;
    char  buf[1024];

    string clientIP = getIPaddress( sock );

    char b[5];
    memset(b, 0, 5);

    // key size
    int len = sock.read_n(b, 4);
    if ( len <= 0 ) { return -10; }

    int sz = atoi(b);

    memset( buf, 0, 1024);
    len = sock.read_n(buf, sz);

    key = buf;

    i("s201801 client=[%s] key sz=%d len=%d key=[%s]\n", clientIP.c_str(), sz, len, key.c_str() );


    // value size
    memset(b, 0, 5);
    len = sock.read_n(b, 4);
    if ( len <= 0 ) { return -30; }
    sz = atoi(b);
    i("s201302 value sizehdr  b=[%s] read len=%d sz=%d\n", b, len, sz );

    memset( buf, 0, 1024);
    len = sock.read_n(buf, sz);
    value = buf;

    i("s201802 value body sz=%d len=%d value=[%s]\n", sz, len, value.c_str() );

    leveldb::WriteOptions write_options;
    write_options.sync = true;

    pthread_mutex_lock ( &g_mutex );
    g_db->Put( write_options, key, value);

    sock.write("K", 1 );
    pthread_mutex_unlock ( &g_mutex );
    i("s11019 wrote 1 byte ack K\n");

    return 0;
}


// Run commands sent from a client
void runPeerCmd(sockpp::tcp_socket sock)
{
    int  len;
    char buf[1];

    while ( true ) {
        i("read from sock ...\n" );
        len = sock.read_n(buf, 1);
        if ( len <= 0 ) {
            i("reading from client socket is done break\n");
            break;
        }

        i("read from sock buf=[%c]\n", buf[0] );

        if ( 'F' == buf[0] ) {
            // "Fnnnn........"
            // update server file and ring
            // in new thread
            in("receiveSaveCfgFile ..." );
            int rc = receiveSaveCfgFile(sock);
            if ( rc < 0 ) {
                i("Error receiveSaveCfgFile\n");
                continue;
            }
            in("receiveSaveCfgFile rc=%d", rc);

        } else if ( 'W' == buf[0] ) {
            // Wnnnn........nnnn......
            i("receiveSaveData ...\n" ); 
            len = receiveSaveData(sock);
            i("receiveSaveData len=%d\n", len); 
            // sock.write("K");
        } else if ( 'D' == buf[0] ) {
            // Dnnnn........nnnn......
            i("receiveDistributeData ...\n");
            len = receiveDistributeData(sock);
            i("receiveDistributeData len=%d\n", len);
        } else if ( 'S' == buf[0] ) {
            // "S"
            i("runScale() ...\n");
            if ( 1 ) {
                runScale();
            } else {
                std::thread thr(runScale);
                thr.detach();
            }
        } else if ( 'C' == buf[0] ) {
            countData();
        } else {
            i("Unknown cmd=[%c] ignored \n", buf[0]);
        }
    }
}


int main(int argc, char *argv[] )
{
    // set up cluster from server.file
    i("Read [%s] ...\n", g_serverCfgFile.c_str() );
    int rrc = ketama_roll( &g_ring, (char*)g_serverCfgFile.c_str() );
    i("ketama_roll rrc=%d\n", rrc );

    pthread_mutex_init( &g_mutex, NULL );

    // get my ip
    getMyIP();
    i( "My IP is [%s]\n", g_myip.c_str() );

    // connect other servers
    i( "Connect to other servers ...\n");

    std::thread thrconn( setup_connections );
    thrconn.detach();


    // setup database
    i( "Setup database ...\n");
    leveldb::Options options;
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, "./mytestdb", &g_db);
    assert(status.ok());

    // setup server listening on port 8898
    i( "Setup server listening on port 8898 ...\n");
    in_port_t port = 8898;
    sockpp::initialize();

    sockpp::tcp_acceptor acc(port);
    i( "acceptor created. loop ...\n");

    // loop and take commands
    while ( true ) {
        sockpp::inet_address peer;

        sockpp::tcp_socket sock = acc.accept(&peer);
        if ( sock ) {
            i( "accepted a peer runCmd() ...\n");

            std::thread thr(runPeerCmd, std::move(sock));
            thr.detach();
        } else {
            i( "accept peer error\n");
        }
    }

    ketama_smoke(g_ring);
    return 0;
}

