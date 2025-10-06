#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>


int getPow(int num) {
    return num * num;
}

int main(int argc, char *argv[]) {

    if (argc != 2)
    {
        fprintf(stderr,"Pouzitie: %s [meno suboru]", argv[0]);
        return 1;
    }

    FILE* file = fopen(argv[1], "r");
    if (file == NULL)
    {
        perror("fopen");
        return 1;
    }
    
    char* numbers[1000];
    int number_count = 0;

    char buffer[10];

    while (fgets(buffer, sizeof(buffer), file) != NULL )
    {
        buffer[strcspn(buffer, "\n")] = '\0';
        numbers[number_count++] = strdup(buffer);
    }
    fclose(file);

    // mam cisla v zozname

    int rura1[2];
    pipe(rura1);

    int rura2[2];
    pipe(rura2);

    int rura3[2];
    pipe(rura3);


    if (fork() == 0)
    {
        // child 1 - vyberie random cislo a posle child 2 
        close(rura1[0]); // nepotrebujem citat z rury 1

        close(rura2[0]);
        close(rura2[1]);

        srand(time(NULL) ^ getpid());
        for (int i = 0; i < (getpid() % 100); i++)
        {
            int index = rand() % number_count;
            char* num = numbers[index];
            write(rura1[1], num, strlen(num));
            write(rura1[1], "\n", 1);
            usleep(200000);
        }
        close(rura1[1]);
        exit(0);

    } else {
        if (fork() == 0)
        {
            // child 2 - precita cislo od Child 1, prida mocninu a posle Child 3
            close(rura1[1]);  // nepotrebujem zapisovat do r1
            close(rura2[0]);   // nepotrebujem citat z r2
            

            char line[100];
            int num;
            FILE* rura1_read = fdopen(rura1[0], "r");
            while (fgets(line, sizeof(line), rura1_read) != NULL)
            {   
                line[strcspn(line, "\n")] = '\0';
                int num = atoi(line);

                char result[1000];
                snprintf(result, sizeof(result), "Povodne cislo: %d, mocnina: %d\n",num, getPow(num));
                write(rura2[1], result, strlen(result));
            }
            fclose(rura1_read);
            close(rura2[1]);
            exit(0);

        } else {
            if (fork() == 0)
            {
                // child 3 - precita, prida pid a posle rodicovi
                close(rura1[0]);
                close(rura1[1]);
                
                close(rura2[1]);

                close(rura3[0]);

                char line[100];
                FILE* rura2_read = fdopen(rura2[0], "r");
                while (fgets(line, sizeof(line), rura2_read) != NULL)
                {   
                    line[strcspn(line, "\n")] = '\0';
                    char result1[1000];
                    snprintf(result1, sizeof(result1), "%s, pid: %d\n", line, getpid());
                    write(rura3[1], result1, strlen(result1));
                }
                fclose(rura2_read);
                close(rura3[1]);
                exit(0);

            } else {
                // rodic - vypise
                close(rura1[0]);
                close(rura1[1]);

                close(rura2[0]);
                close(rura2[1]);

                close(rura3[1]);


                char line[100];
                FILE* rura3_read = fdopen(rura3[0], "r");
                while (fgets(line, sizeof(line), rura3_read) != NULL)
                { 
                    printf("%s", line);
                }

                close(rura3[0]);
                wait(NULL);
                wait(NULL);
                wait(NULL);
            }

        }
        
    }

}