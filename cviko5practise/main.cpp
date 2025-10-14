#include <stdio.h>
#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <fcntl.h>

int main(int argc, char* argv[]) {

    if (argc < 2)
    {
        fprintf(stderr, "Pouzitie: %s <meno suboru>\n", argv[0]);
        return 1;
    }

    char* filename = argv[1];

    int rura1[2];
    pipe(rura1);

    if (fork() == 0)
    {   
        // potomok 1 ---- sort < subor
        close(rura1[0]);

        int fd = open(filename, O_RDONLY);
        if (fd < 0)
        {
            perror("open");
            return 1;   
        }
        
        dup2(fd, 0); // presmerovanie stdin na subor / nech stdin cita z suboru
        close(fd);

        dup2(rura1[1], 1); // presmerovanie stdout na ruru / nech stdout ide do rury
        close(rura1[1]);

        execlp("sort", "sort", NULL);
        perror("execlp");
        return 1;

    }

    int rura2[2];
    pipe(rura2);

    if (fork() == 0)
    {
        // potomok 2
        close(rura1[1]);
        close(rura2[0]);

        dup2(rura1[0], 0); // nech stdin cita z rury1
        close(rura1[0]);


        dup2(rura2[1], 1); // nech stdout ide do rury2
        close(rura2[1]);

        char line[1024];
        char c;
        int line_count = 1;
        int len = 0;

        // bod 2
        while (read(0, &c, 1) > 0)
        {
            if (c == '\n')
            {
                line[len] = '\0';
                len = 0;
                printf("%6d. %s\n", line_count++, line);
                fflush(stdout);
            } else {
                line[len++] = c;
            }
            
        }
        
        
        // bod 1
        /*
        execlp("nl", "nl", "-s. ", (char *)NULL);
        perror("execlp");
        return 1;
        */
        
    }

    if (fork() == 0)
    {
        // potomok 3
        close(rura1[0]); close(rura1[1]);
        close(rura2[1]);

        dup2(rura2[0], 0); // nech stdin cita z rury2
        close(rura2[0]);


        // bod 3
        if (argc == 3)
        {
            int fd_out = open(argv[2], O_WRONLY);
            if (fd_out < 0)
            {
                perror("open");
                return 1;       
            }
            dup2(fd_out, 1); // nech stdout ide do suboru
            close(fd_out);
            
        }
        
        execlp("tr", "tr", "a-z", "A-Z", NULL);
        perror("execlp");
        return 1;

    }
    
    
    close(rura1[0]); close(rura1[1]);
    close(rura2[0]); close(rura2[1]);
    

    while (wait(NULL) > 0);

    return 0;
}