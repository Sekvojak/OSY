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

#include "semaphore.h"
#include "pthread.h"

#include <sys/mman.h> // shm_open, mmap
#include <sys/stat.h> // permission flags

#define STR_CLOSE "close"
#define STR_QUIT "quit"

//***************************************************************************
// log messages

#define LOG_ERROR 0 // errors
#define LOG_INFO 1  // information and notifications
#define LOG_DEBUG 2 // debug messages

#define SHM_NAME "/shared_buffer"
#define BUFFER_SIZE 5
#define STRING_LEN 64

typedef struct
{
    int writeIndex;
    int readIndex;
    int count;
    int itemCounter;
    char data[BUFFER_SIZE][STRING_LEN];
} SharedBuffer;

SharedBuffer *sharedBuffer = nullptr;

#define SEM_EMPTY "/sem_empty"
#define SEM_FULL "/sem_full"
#define SEM_MUTEX "/sem_mutex"

#define SEM_CONSUMER_LOCK "/sem_consumer_lock"

sem_t *semEmpty = NULL;
sem_t *semFull = NULL;
sem_t *semMutex = NULL;

sem_t *semConsumerLock = NULL;

void log_msg(int t_log_level, const char *t_form, ...);

void initSharedBuffer(int creatingNew)
{
    if (creatingNew)
    {
        shm_unlink(SHM_NAME); // zmazanie stareho segmentu
    }

    int memfd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0644); // vytvorenie zdieľanej pamäte
    if (memfd == -1)
    {
        perror("shm_open failed");
        return;
    }

    if (ftruncate(memfd, sizeof(SharedBuffer)) == -1) // nastavenie veľkosti pamäte
    {
        perror("ftruncate failed");
        return;
    }

    sharedBuffer = (SharedBuffer *)mmap(NULL, sizeof(SharedBuffer), PROT_READ | PROT_WRITE, MAP_SHARED, memfd, 0);

    if (sharedBuffer == MAP_FAILED)
    {
        perror("mmap failed");
        close(memfd);
        return;
    }

    close(memfd);

    sharedBuffer->writeIndex = 0;
    sharedBuffer->readIndex = 0;
    sharedBuffer->count = 0;
    sharedBuffer->itemCounter = 0;
    memset(sharedBuffer->data, 0, sizeof(sharedBuffer->data));

    log_msg(LOG_INFO, "Zdieľaná pamäť úspešne pripravená.");
}

void initSemaphores(int creatingNew)
{
    if (creatingNew)
    {
        sem_unlink(SEM_EMPTY);
        sem_unlink(SEM_FULL);
        sem_unlink(SEM_MUTEX);
        sem_unlink(SEM_CONSUMER_LOCK);
    }

    semEmpty = sem_open(SEM_EMPTY, O_CREAT, 0644, 1); // binarne prepravka
    semFull = sem_open(SEM_FULL, O_CREAT, 0644, 0);
    semMutex = sem_open(SEM_MUTEX, O_CREAT, 0644, 1); // // len jeden proces môže meniť indexy

    semConsumerLock = sem_open(SEM_CONSUMER_LOCK, O_CREAT, 0644, 1);

    if (semEmpty == SEM_FAILED || semFull == SEM_FAILED || semMutex == SEM_FAILED || semConsumerLock == SEM_FAILED)
    {
        perror("sem_open failed");
        exit(1);
    }

    log_msg(LOG_INFO, "Semafory úspešne pripravené.");
}

void producer(char *item)
{

    sem_wait(semEmpty);
    sem_wait(semMutex);

    sharedBuffer->itemCounter++;

    char numberedItem[128];
    snprintf(numberedItem, sizeof(numberedItem), "%d. %s\n", sharedBuffer->itemCounter, item);
    strcpy(sharedBuffer->data[sharedBuffer->writeIndex], numberedItem);
    sharedBuffer->writeIndex = (sharedBuffer->writeIndex + 1) % BUFFER_SIZE;
    sharedBuffer->count++;

    sem_post(semMutex);

    if (sharedBuffer->count == BUFFER_SIZE)
        sem_post(semFull); // prepravka je plná -> odošli signál consumerovi
    else
        sem_post(semEmpty);
}

void consumer(int client)
{
    sem_wait(semFull);
    sem_wait(semMutex);

    sem_wait(semConsumerLock);

    printf("[SERVER] CONSUMER bere celu prepravku:\n");
    char message[1024] = "";
    char serverPrint[256];
    for (int i = 0; i < sharedBuffer->count; i++)
    {
        strcat(message, sharedBuffer->data[i]);
        strcat(message, "\n");
        snprintf(serverPrint, sizeof(serverPrint), "%s\n", sharedBuffer->data[i]);
        printf("%s", serverPrint);
    }

    sharedBuffer->count = 0;
    sharedBuffer->writeIndex = 0;
    sharedBuffer->readIndex = 0;

    sem_post(semMutex);
    sem_post(semEmpty);

    write(client, message, strlen(message));

    sem_post(semConsumerLock);
}

// debug flag
int g_debug = LOG_INFO;

void log_msg(int t_log_level, const char *t_form, ...)
{
    const char *out_fmt[] = {
        "ERR: (%d-%s) %s\n",
        "INF: %s\n",
        "DEB: %s\n"};

    if (t_log_level && t_log_level > g_debug)
        return;

    char l_buf[1024];
    va_list l_arg;
    va_start(l_arg, t_form);
    vsprintf(l_buf, t_form, l_arg);
    va_end(l_arg);

    switch (t_log_level)
    {
    case LOG_INFO:
    case LOG_DEBUG:
        fprintf(stdout, out_fmt[t_log_level], l_buf);
        break;

    case LOG_ERROR:
        fprintf(stderr, out_fmt[t_log_level], errno, strerror(errno), l_buf);
        break;
    }
}

//***************************************************************************
// help

void help(int t_narg, char **t_args)
{
    if (t_narg <= 1 || !strcmp(t_args[1], "-h"))
    {
        printf(
            "\n"
            "  Socket server example.\n"
            "\n"
            "  Use: %s [-h -d] port_number\n"
            "\n"
            "    -d  debug mode \n"
            "    -h  this help\n"
            "\n",
            t_args[0]);

        exit(0);
    }

    if (!strcmp(t_args[1], "-d"))
        g_debug = LOG_DEBUG;
}

//***************************************************************************

void handle_client(int client_socket, char *role)
{

    char buff[256];
    int len;

    if (strcmp(role, "producer") == 0)
    {
        log_msg(LOG_INFO, "Client is PRODUCER");
        while ((len = read(client_socket, buff, sizeof(buff) - 1)) > 0)
        {
            buff[len] = '\0';
            buff[strcspn(buff, "\r\n")] = 0;

            if (strlen(buff) == 0)
                continue;

            producer(buff);
            printf("[SERVER] dostal od PRODUCERA: %s\n", buff);

            write(client_socket, "OK\n", 3);
        }
    }
    else if (strcmp(role, "consumer") == 0)
    {
        log_msg(LOG_INFO, "Client is CONSUMER");
        // neskôr sem pridáme čítanie zo zdieľanej pamäte (consumer)
        while (1)
        {
            consumer(client_socket);
            int len = read(client_socket, buff, sizeof(buff) - 1);
            if (len <= 0)
                break;
        }
    }
    else
    {
        write(client_socket, "Unknown task\n", 13);
    }

    close(client_socket);
}

int main(int t_narg, char **t_args)
{

    if (t_narg < 3)
        help(t_narg, t_args);

    int l_port = 0;

    // parsing arguments
    l_port = atoi(t_args[1]);
    char *role = t_args[2];

    int create_new = strcmp(role, "producer") == 0;
    initSharedBuffer(create_new);
    initSemaphores(create_new);

    if (l_port <= 0)
    {
        log_msg(LOG_INFO, "Bad or missing port number %d!", l_port);
        help(t_narg, t_args);
    }

    log_msg(LOG_INFO, "Server will listen on port: %d.", l_port);

    // socket creation
    int l_sock_listen = socket(AF_INET, SOCK_STREAM, 0);
    if (l_sock_listen == -1)
    {
        log_msg(LOG_ERROR, "Unable to create socket.");
        exit(1);
    }

    in_addr l_addr_any = {INADDR_ANY};
    sockaddr_in l_srv_addr;
    l_srv_addr.sin_family = AF_INET;
    l_srv_addr.sin_port = htons(l_port);
    l_srv_addr.sin_addr = l_addr_any;

    // Enable the port number reusing
    int l_opt = 1;
    if (setsockopt(l_sock_listen, SOL_SOCKET, SO_REUSEADDR, &l_opt, sizeof(l_opt)) < 0)
        log_msg(LOG_ERROR, "Unable to set socket option!");

    // assign port number to socket
    if (bind(l_sock_listen, (const sockaddr *)&l_srv_addr, sizeof(l_srv_addr)) < 0)
    {
        log_msg(LOG_ERROR, "Bind failed!");
        close(l_sock_listen);
        exit(1);
    }

    // listenig on set port
    if (listen(l_sock_listen, 1) < 0)
    {
        log_msg(LOG_ERROR, "Unable to listen on given port!");
        close(l_sock_listen);
        exit(1);
    }

    log_msg(LOG_INFO, "Enter 'quit' to quit server.");

    // go!
    // list of fd sources
    pollfd l_read_poll[2];

    l_read_poll[0].fd = STDIN_FILENO;
    l_read_poll[0].events = POLLIN;
    l_read_poll[1].fd = l_sock_listen;
    l_read_poll[1].events = POLLIN;

    while (1) // wait for new client
    {
        // select from fds
        int l_poll = poll(l_read_poll, 2, -1);

        if (l_poll < 0)
        {
            log_msg(LOG_ERROR, "Function poll failed!");
            exit(1);
        }

        if (l_read_poll[0].revents & POLLIN)
        { // data on stdin
            char buf[128];

            int l_len = read(STDIN_FILENO, buf, sizeof(buf));
            if (l_len == 0)
            {
                log_msg(LOG_DEBUG, "Stdin closed.");
                exit(0);
            }
            if (l_len < 0)
            {
                log_msg(LOG_DEBUG, "Unable to read from stdin!");
                exit(1);
            }

            log_msg(LOG_DEBUG, "Read %d bytes from stdin", l_len);
            // request to quit?
            if (!strncmp(buf, STR_QUIT, strlen(STR_QUIT)))
            {
                log_msg(LOG_INFO, "Request to 'quit' entered.");
                close(l_sock_listen);
                exit(0);
            }
        }

        if (l_read_poll[1].revents & POLLIN)
        { // new client?
            sockaddr_in l_cli_addr;
            socklen_t l_cli_size = sizeof(l_cli_addr);

            int new_client = accept(l_sock_listen, (sockaddr *)&l_cli_addr, &l_cli_size);
            if (new_client < 0)
            {
                log_msg(LOG_ERROR, "Unalble to accept new client");
                continue;
            }

            log_msg(LOG_INFO, "Client connected: %s:%d",
                    inet_ntoa(l_cli_addr.sin_addr),
                    ntohs(l_cli_addr.sin_port));

            if (strcmp(role, "producer") == 0)
                write(new_client, "producer\n", strlen("producer\n"));
            else if (strcmp(role, "consumer") == 0)
                write(new_client, "consumer\n", strlen("consumer\n"));
            else
                write(new_client, "unknown\n", strlen("unknown\n"));

            pid_t pid = fork();

            if (pid == 0)
            {
                // child proces
                close(l_sock_listen);
                handle_client(new_client, role);
                close(new_client);
                _exit(0);
            }
            else if (pid > 0)
            {
                // parent
                close(new_client);
            }
        }
    }
    close(l_sock_listen);
    return 0;
}
