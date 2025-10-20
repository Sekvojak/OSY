//***************************************************************************
//
// Program example for labs in subject Operating Systems
//
// Petr Olivka, Dept. of Computer Science, petr.olivka@vsb.cz, 2017
//
// Example of socket server.
//
// This program is example of socket server and it allows to connect and serve
// the only one client.
// The mandatory argument of program is port number for listening.
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
#include <sys/wait.h>   

#define STR_CLOSE   "close"
#define STR_QUIT    "quit"

//***************************************************************************
// log messages

#define LOG_ERROR               0       // errors
#define LOG_INFO                1       // information and notifications
#define LOG_DEBUG               2       // debug messages

// debug flag
int g_debug = LOG_INFO; // globálna premenná určujúca aktuálnu verbóznosť výpisov.
                        // Default: INFO → vypíšu sa ERROR a INFO, ale nie DEBUG.

// t_log_level = jedna z LOG_ERROR/LOG_INFO/LOG_DEBUG
// t_form, ... = variadické argumenty (ako printf)
void log_msg( int t_log_level, const char *t_form, ... )
{
    const char *out_fmt[] = {  // pole formátovacích reťazcov pre jednotlivé levely, indexované 0,1,2
            "ERR: (%d-%s) %s\n",
            "INF: %s\n",
            "DEB: %s\n" };

    if ( t_log_level && t_log_level > g_debug ) return;

    char l_buf[ 1024 ];
    va_list l_arg; // ukazovateľ na miesto v pamäti, kde začínajú tie “ďalšie argumenty” po t_form
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
            "  Socket server example.\n"
            "\n"
            "  Use: %s [-h -d] port_number\n"
            "\n"
            "    -d  debug mode \n"
            "    -h  this help\n"
            "\n", t_args[ 0 ] );

        exit( 0 );
    }

    if ( !strcmp( t_args[ 1 ], "-d" ) )
        g_debug = LOG_DEBUG;
}

void handleClient(int client_socket) {

    pid_t pid = getpid();
    pid_t ppid = getppid();
    log_msg(LOG_INFO, "Child process started. PID = %d, parent PID = %d", pid, ppid);

    char l_buf[ 256 ];

    struct pollfd l_read_poll[1];

    // budeme sledovať len socket klienta
    l_read_poll[0].fd = client_socket;
    l_read_poll[0].events = POLLIN;

    char examples[3][256];
    int exampleCount = 0;

    while (1)
    {
        // select from fds
        int l_poll = poll( l_read_poll, 1, -1 );

        if ( l_poll < 0 )
        {
            log_msg( LOG_ERROR, "Function poll failed!" );
            exit( 1 );
        }

        
        if ( l_read_poll[ 0 ].revents & POLLIN )
            {
                // read data from client
                int l_len = read( client_socket, l_buf, sizeof( l_buf ) );
                if (l_len <= 0)
                {
                    log_msg(LOG_INFO, "Client disconnected.");
                    break;
                }

                l_buf[l_len] = '\0';
                l_buf[strcspn(l_buf, "\r\n")] = 0;
                if (strlen(l_buf) == 0) {
                    log_msg(LOG_INFO, "Empty input ignored.");
                    continue;
                }

                // close request?
                if ( !strncasecmp( l_buf, "close", strlen( STR_CLOSE ) ) )
                {
                    log_msg( LOG_INFO, "Client sent 'close' request to close connection." );
                    close( client_socket );
                    log_msg( LOG_INFO, "Connection closed. Waiting for new client." );
                    break;
                }

                strcpy(examples[exampleCount], l_buf);
                exampleCount++;

                if (exampleCount == 3)
                {
                    char python_commands[4086];
                    for (int i = 0; i < exampleCount; i++)
                    {
                        char temp[1024];
                        snprintf(temp, sizeof(temp),"print(\"%s=\", %s, sep=\"\")\n",examples[i], examples[i]);
                        strcat(python_commands, temp);
                    }

                    
                
                
                /*
                char python_command[1024];
                snprintf(python_command, sizeof(python_command), "print(\"%s=\", %s, sep=\"\")\n", l_buf, l_buf);
                */

                int pipe_in[2]; int pipe_out[2];
                pipe(pipe_in); pipe(pipe_out);

                pid_t child = fork();
                if (child == 0) {
                    // child
                    dup2(pipe_in[0], 0); close(pipe_in[0]);
                    dup2(pipe_out[1], 1); close(pipe_out[1]);
                    close(pipe_in[1]); // pipe_in[0] = READ,  pipe_in[1] = WRITE
                    close(pipe_out[0]); // pipe_out[0] = READ, pipe_out[1] = WRITE

                    execlp("python3", "python3", NULL);
                    exit(1);


                } else if ( child > 0) {
                    // parent
                    close(pipe_in[0]);
                    close(pipe_out[1]);

                    write(pipe_in[1], python_commands, strlen(python_commands));
                    close(pipe_in[1]);

                    char result[512];
                    int rlen;
                    while ((rlen = read(pipe_out[0], result, sizeof(result))) > 0) {
                        write(client_socket, result, rlen);
                        exampleCount = 0;
                    }
                    close(pipe_out[0]);
                    waitpid(child, NULL, 0);

                
                } 
            }


        }
            
    }
    close(client_socket);
    log_msg(LOG_INFO, "Connection closed in child process.");
    
}

void childHandler(int sig) {
    // pozbieraj všetky ukončené deti
    while (waitpid(-1, NULL, WNOHANG) > 0);
}


//***************************************************************************

int main( int t_narg, char **t_args )
{   
    // 🔹 1️⃣ Spracovanie argumentov
    if ( t_narg <= 1 ) help( t_narg, t_args );

    int l_port = 0;

    // parsing arguments
    for ( int i = 1; i < t_narg; i++ )
    {
        if ( !strcmp( t_args[ i ], "-d" ) )
            g_debug = LOG_DEBUG;

        if ( !strcmp( t_args[ i ], "-h" ) )
            help( t_narg, t_args );

        if ( *t_args[ i ] != '-' && !l_port ) // *t_args[i] znamená: zober prvý znak reťazca t_args[i]
        {
            l_port = atoi( t_args[ i ] );
            break;
        }
    }

    // zadane neplatne číslo portu
    if ( l_port <= 0 )
    {
        log_msg( LOG_INFO, "Bad or missing port number %d!", l_port );
        help( t_narg, t_args );
    }

    // vypíše informáciu — na ktorom porte sa server spustí.
    log_msg( LOG_INFO, "Server will listen on port: %d.", l_port );

    // socket creation
    int l_sock_listen = socket( AF_INET, SOCK_STREAM, 0 );
    // Vytvorí nový socket (v podstate "komunikačný koncový bod").
    // AF_INET → IPv4, SOCK_STREAM → TCP (spoľahlivý, prúdový), 
    // 0 → protokol (0 = nech OS vyberie správny pre kombináciu AF_INET + SOCK_STREAM = TCP)
    if ( l_sock_listen == -1 )
    {
        log_msg( LOG_ERROR, "Unable to create socket.");
        exit( 1 );
    }

    // 4️⃣ Nastavenie adresy servera
    in_addr l_addr_any = { INADDR_ANY }; // 🔸 INADDR_ANY → špeciálna adresa 0.0.0.0, znamená „počúvaj na všetkých sieťových rozhraniach“.
    sockaddr_in l_srv_addr; // sockaddr_in (definuje sieťovú adresu a port).
    l_srv_addr.sin_family = AF_INET; // Typ adresy – IPv4 (AF_INET)
    l_srv_addr.sin_port = htons( l_port ); // Číslo portu – musí byť v „network byte order“ → preto htons()
    l_srv_addr.sin_addr = l_addr_any;

    // Enable the port number reusing
    // 5️⃣ Povolenie opätovného použitia portu
    int l_opt = 1;
    if ( setsockopt( l_sock_listen, SOL_SOCKET, SO_REUSEADDR, &l_opt, sizeof( l_opt ) ) < 0 )
      log_msg( LOG_ERROR, "Unable to set socket option!" );

    // assign port number to socket
    // 6️⃣ Priradenie portu k socketu
    if ( bind( l_sock_listen, (const sockaddr * ) &l_srv_addr, sizeof( l_srv_addr ) ) < 0 )
    {
        log_msg( LOG_ERROR, "Bind failed!" );
        close( l_sock_listen );
        exit( 1 );
    }

    // listenig on set port
    // 7️⃣ Spustenie počúvania
    if ( listen( l_sock_listen, 10 ) < 0 ) // ➡️ Prepne socket do „passive“ režimu,
    { // Druhý argument (10) = koľko klientov môže byť v „fronte“ čakajúcich na prijatie.
        log_msg( LOG_ERROR, "Unable to listen on given port!" );
        close( l_sock_listen );
        exit( 1 );
    }

    log_msg( LOG_INFO, "Enter 'quit' to quit server." );
    log_msg(LOG_INFO, "Server main process started. PID = %d", getpid());
    signal(SIGCHLD, childHandler);
    // go!        „server nikdy nekončí — po každom klientovi čaká na ďalšieho.
    while ( 1 )
    {
        int l_sock_client = -1;

        // list of fd sources
        pollfd l_read_poll[ 2 ];

        l_read_poll[ 0 ].fd = STDIN_FILENO; // vstup z klávesnice
        l_read_poll[ 0 ].events = POLLIN; // POLLIN (niečo sa dá čítať)
        l_read_poll[ 1 ].fd = l_sock_listen; // serverový „počúvací“ socket
        l_read_poll[ 1 ].events = POLLIN; // POLLIN (niekto sa pokúša pripojiť)

        // 1️⃣ Čakanie na nového klienta
         // wait for new client
        
        // select from fds
        // čaká, kým sa na niektorom z týchto FD objavia dáta.
        // 2 → počet sledovaných FD (stdin + socket)
        // -1 → čakaj nekonečne dlho (žiadny timeout)
        int l_poll = poll( l_read_poll, 2, -1 );
            
        if ( l_poll < 0 )
        {   
            if (errno == EINTR) continue; 
            log_msg( LOG_ERROR, "Function poll failed!" );
            exit( 1 );
        }
        // 🔹 Ak niekto niečo napíše do stdin
        if ( l_read_poll[ 0 ].revents & POLLIN )
        { // data on stdin
            char buf[ 128 ];
                
            int l_len = read(STDIN_FILENO, buf, sizeof(buf));
            log_msg(LOG_DEBUG, "Read %d bytes from stdin", l_len);
                
            // request to quit?
            if ( !strncmp( buf, STR_QUIT, strlen( STR_QUIT ) ) )
            {
                log_msg( LOG_INFO, "Request to 'quit' entered.");
                close( l_sock_listen );
                exit( 0 );
            }
        }

        // 🔹 Ak sa niekto pokúša pripojiť (klient)
        if ( l_read_poll[ 1 ].revents & POLLIN )
        { // new client?
            sockaddr_in l_rsa; // l_rsa (remote socket address) je štruktúra, kam sa uloží adresa klienta (IP + port), ktorý sa pripája.
            int l_rsa_size = sizeof( l_rsa );
            // new connection
            l_sock_client = accept( l_sock_listen, ( sockaddr * ) &l_rsa, ( socklen_t * ) &l_rsa_size );
            // l_sock_client = nový socket, cez ktorý sa budeš s týmto klientom rozprávať.
            // l_sock_listen ostáva ďalej „počúvací socket“ (čaká na ďalších klientov).
            if ( l_sock_client == -1 )
            {
                log_msg( LOG_ERROR, "Unable to accept new client." );
                continue;
            }
            log_msg(LOG_INFO, "Client connected: %s:%d",
                inet_ntoa(l_rsa.sin_addr), ntohs(l_rsa.sin_port));

            pid_t pid = fork();
            if (pid == 0)
            {
                // DIETA
                close(l_sock_listen);
                handleClient(l_sock_client);
                exit(0);
            }
            else if (pid > 0)
            {
                // RODIČ
                close(l_sock_client);          
                log_msg(LOG_INFO, "New client handled by child PID %d", pid);
                while (waitpid(-1, NULL, WNOHANG) > 0);
            }
            else
            {
                log_msg(LOG_ERROR, "Fork failed!");
            }
        }
    } // while(1)
    return 0;
}