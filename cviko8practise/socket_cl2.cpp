//***************************************************************************
//
// Program example for subject Operating Systems
//
// Petr Olivka, Dept. of Computer Science, petr.olivka@vsb.cz, 2021
//
// Example of socket server/client.
//
// This program is example of socket client.
// The mandatory arguments of program is IP adress or name of server and
// a port number.
//
//***************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>

#include "pthread.h"

#define STR_CLOSE               "close"

//***************************************************************************
// log messages

#define LOG_ERROR               0       // errors
#define LOG_INFO                1       // information and notifications
#define LOG_DEBUG               2       // debug messages

// debug flag
int g_debug = LOG_INFO;




void log_msg( int t_log_level, const char *t_form, ... )
{
    const char *out_fmt[] = {
            "ERR: (%d-%s) %s\n",
            "INF: %s\n",
            "DEB: %s\n" };

    if ( t_log_level && t_log_level > g_debug ) return;

    char l_buf[ 1024 ];
    va_list l_arg;
    va_start( l_arg, t_form );
    vsprintf( l_buf, t_form, l_arg );
    va_end( l_arg );

    switch ( t_log_level )
    {
    case LOG_INFO:
    case LOG_DEBUG:
        fprintf( stdout, out_fmt[ t_log_level ], l_buf );
        break;

    case LOG_ERROR:
        fprintf( stderr, out_fmt[ t_log_level ], errno, strerror( errno ), l_buf );
        break;
    }
}

//***************************************************************************
// help

void help( int t_narg, char **t_args )
{
    if ( t_narg <= 1 || !strcmp( t_args[ 1 ], "-h" ) )
    {
        printf(
            "\n"
            "  Socket client example.\n"
            "\n"
            "  Use: %s [-h -d] ip_or_name port_number\n"
            "\n"
            "    -d  debug mode \n"
            "    -h  this help\n"
            "\n", t_args[ 0 ] );

        exit( 0 );
    }

    if ( !strcmp( t_args[ 1 ], "-d" ) )
        g_debug = LOG_DEBUG;
}

//***************************************************************************

pthread_t producer_thread;
pthread_t consumer_thread;

int send_rate = 60;

int readLine(int sock, char *buf, int max) {
    int pos = 0; char c;
    while (pos < max-1 && read(sock, &c, 1) == 1) {
        if (c == '\n') break;
        buf[pos++] = c;
    }
    buf[pos] = 0;
    return pos;
}


void produceFile(int sock, const char* file) {
    FILE *f = fopen(file, "r");
    if (!f) { perror("file"); return; }

    char line[256], resp[64];
    while (fgets(line, sizeof(line), f)) {
        write(sock, line, strlen(line));
        readLine(sock, resp, sizeof(resp));
    }
    fclose(f);

    for (int i=0;i<3;i++){
        write(sock, "\n", 1);
        readLine(sock, resp, sizeof(resp));
    }
}


void consumeFile(int sock, const char* file) {
    FILE *f = fopen(file, "w");
    if (!f) { perror("file"); return; }

    char line[256]; int empty = 0;
    while (1) {
        int n = readLine(sock, line, sizeof(line));
        if (n==0) {
            empty++;
            if (empty>=3) break;
            fprintf(f,"\n");
            write(sock,"OK\n",3);
            continue;
        }
        empty = 0;
        fprintf(f,"%s\n", line);
        write(sock,"OK\n",3);
    }
    fclose(f);
}



int main( int t_narg, char **t_args )
{

    if (t_narg < 4) {
        printf("Usage: %s host port filename\n", t_args[0]);
        exit(1);
    }

    char* host = t_args[1];
    int port = atoi(t_args[2]);
    char* filename = t_args[3];

    int is_producer = (port % 2 == 0);

    
    log_msg( LOG_INFO, "Connection to '%s':%d.", host, port );

    addrinfo l_ai_req, *l_ai_ans;
    bzero( &l_ai_req, sizeof( l_ai_req ) );
    l_ai_req.ai_family = AF_INET;
    l_ai_req.ai_socktype = SOCK_STREAM;

    int l_get_ai = getaddrinfo( host, nullptr, &l_ai_req, &l_ai_ans );
    if ( l_get_ai )
    {
        log_msg( LOG_ERROR, "Unknown host name!" );
        exit( 1 );
    }

    sockaddr_in l_cl_addr =  *( sockaddr_in * ) l_ai_ans->ai_addr;
    l_cl_addr.sin_port = htons( port );
    freeaddrinfo( l_ai_ans );

    // socket creation
    int l_sock_server = socket( AF_INET, SOCK_STREAM, 0 );
    if ( l_sock_server == -1 )
    {
        log_msg( LOG_ERROR, "Unable to create socket.");
        exit( 1 );
    }

    // connect to server
    if ( connect( l_sock_server, ( sockaddr * ) &l_cl_addr, sizeof( l_cl_addr ) ) < 0 )
    {
        log_msg( LOG_ERROR, "Unable to connect server." );
        exit( 1 );
    }

    uint l_lsa = sizeof( l_cl_addr );
    // my IP
    getsockname( l_sock_server, ( sockaddr * ) &l_cl_addr, &l_lsa );
    log_msg( LOG_INFO, "My IP: '%s'  port: %d",
             inet_ntoa( l_cl_addr.sin_addr ), ntohs( l_cl_addr.sin_port ) );
    // server IP
    getpeername( l_sock_server, ( sockaddr * ) &l_cl_addr, &l_lsa );
    log_msg( LOG_INFO, "Server IP: '%s'  port: %d",
             inet_ntoa( l_cl_addr.sin_addr ), ntohs( l_cl_addr.sin_port ) );

    log_msg( LOG_INFO, "Enter 'close' to close application." );


    // go!
    
    if (is_producer)
    {   
        printf(">>> PRODUCER sending %s\n", filename);
        produceFile(l_sock_server, filename);

        
    } else
    {      
        printf(">>> CONSUMER writing to %s\n", filename);
        consumeFile(l_sock_server, filename);

    } 
    
        
    

    // close socket
    close( l_sock_server );

    return 0;
  }
