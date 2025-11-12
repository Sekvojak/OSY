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


void* producer(void *arg) {
    int serverSocket = *(int*)arg;
    free(arg);


    while (1)
    {
        FILE* file = fopen("jmena.txt", "r");
        if (!file) {
            perror("Cannot open jmena.txt");
            return NULL;
        }
        char line[64];


        while (fgets(line, sizeof(line), file))
        {
            write(serverSocket, line, strlen(line));

            // wait for OK
            char resp[32];
            int len = read(serverSocket, resp, sizeof(resp)-1);
            if (len <= 0) {
                fclose(file);
                return NULL;
            }
            resp[len] = '\0';

            printf("Sent: %sGot: %s", line, resp);

            float delay = 60.0 / send_rate;
            usleep(delay * 1000000);
        }

        fclose(file);
    }
    
}

void* consumer(void *arg) {
    int socketServer = *(int*)arg;
    char buff[256];
    int len;
    while ((len = read(socketServer, buff, sizeof(buff))) > 0)
    {   
        

        buff[len] = '\0';

        // buff[strcspn(buff, "\r\n")] = 0;
        if (strlen(buff) == 0) {
            // prázdny riadok? ignoruj ho a OK pošli až pri skutočných dátach
            continue;
        }

        printf("%s", buff);
        write(socketServer, "OK\n", 3);

    }
    return NULL;
}



int main( int t_narg, char **t_args )
{

    if ( t_narg <= 2 ) help( t_narg, t_args );

    int l_port = 0;
    char *l_host = nullptr;

    // parsing arguments
    for ( int i = 1; i < t_narg; i++ )
    {
        if ( !strcmp( t_args[ i ], "-d" ) )
            g_debug = LOG_DEBUG;

        if ( !strcmp( t_args[ i ], "-h" ) )
            help( t_narg, t_args );

        if ( *t_args[ i ] != '-' )
        {
            if ( !l_host )
                l_host = t_args[ i ];
            else if ( !l_port )
                l_port = atoi( t_args[ i ] );
        }
    }

    if ( !l_host || !l_port )
    {
        log_msg( LOG_INFO, "Host or port is missing!" );
        help( t_narg, t_args );
        exit( 1 );
    }

    log_msg( LOG_INFO, "Connection to '%s':%d.", l_host, l_port );

    addrinfo l_ai_req, *l_ai_ans;
    bzero( &l_ai_req, sizeof( l_ai_req ) );
    l_ai_req.ai_family = AF_INET;
    l_ai_req.ai_socktype = SOCK_STREAM;

    int l_get_ai = getaddrinfo( l_host, nullptr, &l_ai_req, &l_ai_ans );
    if ( l_get_ai )
    {
        log_msg( LOG_ERROR, "Unknown host name!" );
        exit( 1 );
    }

    sockaddr_in l_cl_addr =  *( sockaddr_in * ) l_ai_ans->ai_addr;
    l_cl_addr.sin_port = htons( l_port );
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
    
    char role_buf[128];
    int r = read(l_sock_server, role_buf, sizeof(role_buf)-1);
    role_buf[r] = 0;
    role_buf[strcspn(role_buf, "\r\n")] = 0;
    printf("%s", role_buf);

    if (strcmp(role_buf, "producer") == 0)
    {   
        printf(">>> PRODUCER MODE started\n");
        int* serverSocketPtr = (int*)malloc(sizeof(int));
        *serverSocketPtr = l_sock_server;
        pthread_create(&producer_thread, NULL, producer, serverSocketPtr);
        pthread_join(producer_thread, NULL);
        
        
    } else if (strcmp(role_buf, "consumer") == 0)
    {      
        printf(">>> CONSUMER MODE started\n");
        int* serverSocketPtr = (int*)malloc(sizeof(int));
        *serverSocketPtr = l_sock_server;
        pthread_create(&consumer_thread, NULL, consumer, serverSocketPtr);
        pthread_join(consumer_thread, NULL);

        
    } else {
        printf("Unknown role\n");
    }
    
        
    

    // close socket
    close( l_sock_server );

    return 0;
  }
