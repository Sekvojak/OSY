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

#include <pthread.h> 
#include <time.h>

#define MAX_CLIENTS 100

pthread_t threads[MAX_CLIENTS];
int sockets[MAX_CLIENTS];
int indexes[MAX_CLIENTS];
int client_count = 0;

#define STR_CLOSE   "close"
#define STR_QUIT    "quit"

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

//***************************************************************************

/*
Server:

drží 10×10 hracie pole so stavmi:
Free, Ship, Hit, Miss

po pripojení klienta mu pošle maskovanú verziu (len H, M, alebo .)

prijíma správy typu shot-C-8

odpovie Hit alebo Miss

zaktualizuje pole a znovu pošle stav

Klient:

posiela strely (shot-C-8)

zobrazuje odpovede a pole


*/
//***************************************************************************

#define SIZE 10
char board[SIZE][SIZE];
char maskedBoard[SIZE][SIZE];

void initBoard() {
    for (int r = 0; r < SIZE; r++)
    {
        for (int c = 0; c < SIZE; c++)
        {
            board[r][c] = '.';
        }
    }

    for (int i = 0; i < 20; i++)
    {
        int randomR = rand() % SIZE;
        int randomC = rand() % SIZE;
        board[randomR][randomC] = 'S';
    }
   
}

void initMaskedBoard() {
    for (int r = 0; r < SIZE; r++)
    {
        for (int c = 0; c < SIZE; c++)
        {
            maskedBoard[r][c] = '.';
        }
    }
}

void sendMaskedBoard(int clientSocket) {
    write(clientSocket, "  12345678910\n", 14);
    for (int r = 0; r < SIZE; r++)
    {
        char row[SIZE + 3];
        row[0] = 'A' + r;          
        row[1] = ' ';  
        for (int c = 0; c < SIZE; c++)
        {
            row[c + 2] = maskedBoard[r][c];
        }
        row[SIZE + 2] = '\n';
        row[SIZE + 3] = '\0';
        write(clientSocket, row, strlen(row));
    }
    write(clientSocket, "\n", 1);
}


//***************************************************************************
// každé vlákno rieši jedného klienta

void* handle_client(void* arg) {

    int my_index = *(int*)arg;
    int client_socket = sockets[my_index];
    write(client_socket, "Enter your nickname: ", 22);

    char buffer[256];
    int len = read(client_socket, buffer, sizeof(buffer));
    buffer[len - 1] = '\0'; 
    log_msg(LOG_INFO, "Client nickname: %s", buffer);

    char welcome[512];
    snprintf(welcome, sizeof(welcome), "Welcome %s!\nHere is your board:\n", buffer);
    write(client_socket, welcome, strlen(welcome));
    sendMaskedBoard(client_socket);
        

    log_msg(LOG_INFO, "New thread started for client %d", client_socket);


    int lread;
    while((lread = read(client_socket, buffer, sizeof(buffer))) > 0) {
        buffer[lread] = '\0';   

        if (!strncasecmp(buffer, STR_CLOSE, strlen(STR_CLOSE))) {
            
            log_msg(LOG_INFO, "Client sent 'close' request to close connection.");
            break; 
        }

        if (!strncasecmp(buffer, "shot-", 5)) {
            // strelil
            char shotR;
            int shotC;
            if (sscanf(buffer, "shot-%c-%d", &shotR, &shotC) == 2)
            {
                int row = shotR - 'A';    
                int col = shotC - 1;

                if (row < 0 || row >= SIZE || col < 0 || col >= SIZE)
                {
                    write(client_socket, "Invalid coordinates!\n", 22);
                    return NULL;
                }

                if (board[row][col] == 'S')
                {
                    // hit
                    board[row][col] = 'H';
                    maskedBoard[row][col] = 'H';
                    write(client_socket, "Hit\n", 5);
                } else if (board[row][col] == '.')
                {
                    // miss
                    board[row][col] = 'M';
                    maskedBoard[row][col] = 'M';
                    write(client_socket, "Miss\n", 6);

                } else {
                    write(client_socket, "Already tried this spot!\n", 26);
                }
                
                sendMaskedBoard(client_socket);
            }
            
        }
        
    }

    close(client_socket);
    log_msg(LOG_INFO, "Connection closed. Waiting for new client.");
    for (int i = 0; i < client_count; i++) {
    if (sockets[i] == client_socket) {
        for (int j = i; j < client_count - 1; j++) {
            sockets[j] = sockets[j + 1]; // posuň všetkých o jedno dozadu
            indexes[j] = j;  
        }
        client_count--;
        break;
        }
    }
    return NULL;

}


//***************************************************************************

int main( int t_narg, char **t_args )
{
    if ( t_narg <= 1 ) help( t_narg, t_args );

    int l_port = 0;

    // parsing arguments
    for ( int i = 1; i < t_narg; i++ )
    {
        if ( !strcmp( t_args[ i ], "-d" ) )
            g_debug = LOG_DEBUG;

        if ( !strcmp( t_args[ i ], "-h" ) )
            help( t_narg, t_args );

        if ( *t_args[ i ] != '-' && !l_port )
        {
            l_port = atoi( t_args[ i ] );
            break;
        }
    }

    if ( l_port <= 0 )
    {
        log_msg( LOG_INFO, "Bad or missing port number %d!", l_port );
        help( t_narg, t_args );
    }

    log_msg( LOG_INFO, "Server will listen on port: %d.", l_port );

    // socket creation
    int l_sock_listen = socket( AF_INET, SOCK_STREAM, 0 );
    if ( l_sock_listen == -1 )
    {
        log_msg( LOG_ERROR, "Unable to create socket.");
        exit( 1 );
    }

    in_addr l_addr_any = { INADDR_ANY };
    sockaddr_in l_srv_addr;
    l_srv_addr.sin_family = AF_INET;
    l_srv_addr.sin_port = htons( l_port );
    l_srv_addr.sin_addr = l_addr_any;

    // Enable the port number reusing
    int l_opt = 1;
    if ( setsockopt( l_sock_listen, SOL_SOCKET, SO_REUSEADDR, &l_opt, sizeof( l_opt ) ) < 0 )
      log_msg( LOG_ERROR, "Unable to set socket option!" );

    // assign port number to socket
    if ( bind( l_sock_listen, (const sockaddr * ) &l_srv_addr, sizeof( l_srv_addr ) ) < 0 )
    {
        log_msg( LOG_ERROR, "Bind failed!" );
        close( l_sock_listen );
        exit( 1 );
    }

    // listenig on set port
    if ( listen( l_sock_listen, 1 ) < 0 )
    {
        log_msg( LOG_ERROR, "Unable to listen on given port!" );
        close( l_sock_listen );
        exit( 1 );
    }

    log_msg( LOG_INFO, "Enter 'quit' to quit server." );

    srand(time(NULL));
    initBoard();
    initMaskedBoard();

    while (1)
    {

        sockaddr_in l_cli_addr;
        socklen_t l_cli_size = sizeof(l_cli_addr);

        int new_client = accept(l_sock_listen, (sockaddr*)&l_cli_addr, &l_cli_size);
        if (new_client < 0)
        {
            log_msg(LOG_ERROR, "Unalble to accept new client");
            continue;
        }
        
        log_msg(LOG_INFO, "Client connected: %s:%d",
                inet_ntoa(l_cli_addr.sin_addr),
                ntohs(l_cli_addr.sin_port));
        

        if (client_count >= MAX_CLIENTS)
        {
            log_msg(LOG_INFO, "Max clients reached, closing new connection.");
            close(new_client);
            continue;
        }
        
        
        sockets[client_count] = new_client;
        indexes[client_count] = client_count; 

        pthread_create(&threads[client_count], NULL, handle_client, &indexes[client_count]); // data ak struct
        pthread_detach(threads[client_count]);
        client_count++;


        
        for (int r = 0; r < SIZE; r++) {
            for (int c = 0; c < SIZE; c++) {
                printf("%c", board[r][c]);
            }
            printf("\n");
        }
            


    }
    close(l_sock_listen);
    return 0;
    
}
