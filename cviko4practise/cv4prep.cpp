#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

int main(int argc, char *argv[]) {

    if (argc != 2)
    {
        fprintf(stderr,"Pouzitie: %s <subor so zoznamom mien>\n", argv[0]);
        return 1;
    }

    FILE* file = fopen(argv[1], "r");
    if (file == NULL)
    {
        perror("Chyba pri otvarani suboru");
        return 1;
    }
    
    // nacitanie vsetkych mien zo suboru do pola
    char* names[1000];
    int line_count = 0;
    char buffer[100];
    while (fgets(buffer, sizeof(buffer), file) != NULL)
    {   
        buffer[strcspn(buffer, "\n")] = '\0';
        names[line_count] = strdup(buffer);
        line_count++;
    }
    fclose(file);

    int rura[2];
    pipe(rura);

    int rura2[2];
    pipe(rura2);

    int rura3[2];
    pipe(rura3);


    if (fork() == 0)
    {   
        // prvy potomok
        // printf("I am child number 1 - process %d\n", getpid());
        srand(time(NULL) ^ getpid()); 
        close(rura[0]); // zatvorime citaciu cast rury
        for (int i = 0; i < (getpid() % 100) + 10; i++)
        {
            // nahodne vyberieme meno zo suboru
            int random_index = rand() % line_count;
            char* name = names[random_index];
            write(rura[1], name, strlen(name)); 
            write(rura[1], "\n", 1);
            usleep(100000); // spime 0.1 sekundy
        }
        close(rura[1]); // zatvorime zapisovaciu cast rury
        exit(0);
        
    } else {
        if (fork() == 0)
        {   
            // druhy potomok
            // printf("I am child number 2 - process %d\n", getpid());
            close(rura[1]); 
            close(rura2[0]);
            FILE* rura_read = fdopen(rura[0], "r");
            char line[100];
            int line_number = 1;
            while(fgets(line, sizeof(line), rura_read) != NULL)
            {
                line[strcspn(line, "\n")] = '\0'; // odstranenie noveho riadku
                int length = strlen(line);
                char output[150];
                snprintf(output, sizeof(output), "%d: %s (%d)\n", line_number, line, length);
                write(rura2[1], output, strlen(output));
                line_number++;
            }
            fclose(rura_read);
            close(rura2[1]);
            exit(0);

        } else
        {
            if (fork() == 0)
            {
                // treti potomok
                // printf("I am child number 3 - process %d\n", getpid());
                close(rura[0]);
                close(rura[1]);  // nepotrebujem ruru 1

                close(rura2[1]); // nepotrebujem pisat do rury 2

                close(rura3[0]); // nepotrebujem citat z rury 3


                FILE* rura2_read = fdopen(rura2[0], "r");
                char line[150];
                while(fgets(line, sizeof(line), rura2_read) != NULL)
                {
                    // printf("%s", line);
                    write(rura3[1], line, strlen(line));

                }

                fclose(rura2_read);
                close(rura3[1]);
                exit(0);
            } else {

                // rodic
                close(rura[0]);
                close(rura[1]);
                close(rura2[0]);
                close(rura2[1]);
                close(rura3[1]); 

                FILE* rura3_read = fdopen(rura3[0], "r");
                char line[150];
                while (fgets(line, sizeof(line), rura3_read) != NULL)
                {   
                    line[strcspn(line, "\n")] = '\0';
                    // extrahujem meno a pridam poradove číslo
                    char* colon = strstr(line, ":");
                    char* name = colon + 2;
                    name[strcspn(name, " (")] = '\0'; // odstranenie vsetkeho za menom

                    int index;
                    for (int i = 0; i < line_count; i++) {
                        if (strcmp(names[i], name) == 0) {
                            index = i + 1;
                            break;
                        }
                    }
                    printf("%s - %d\n", line, index);
                }
                

                fclose(rura3_read);
                wait(NULL);
                wait(NULL);
                wait(NULL);
            }
            
        }    
        
    }
    
    return 0;
}