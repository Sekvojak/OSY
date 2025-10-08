#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <ctype.h>

void normalize(char *s) {
    if (s[0]) s[0] = toupper((unsigned char)s[0]);
    for (int i = 1; s[i]; i++) {
        s[i] = tolower((unsigned char)s[i]);
    }
}

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

    int R1R2[2];
    pipe(R1R2);

    if (fork() == 0)
    {   
        // prvy potomok
        // printf("I am child number 1 - process %d\n", getpid());
        srand(time(NULL) ^ getpid()); 
        close(R1R2[0]); // zatvorime citaciu cast rury
        for (int i = 0; i < (getpid() % 100) + 10; i++)
        {
            // nahodne vyberieme meno zo suboru
            int random_index = rand() % line_count;
            char* name = names[random_index];
            write(R1R2[1], name, strlen(name)); 
            write(R1R2[1], "\n", 1);
            usleep(100000); // spime 0.1 sekundy
        }
        close(R1R2[1]); // zatvorime zapisovaciu cast rury
        exit(0);
        
    } 

    int R2R3[2];
    pipe(R2R3);

    if (fork() == 0)
    {   
        // druhy potomok
        // printf("I am child number 2 - process %d\n", getpid());
        close(R1R2[1]); 
        close(R2R3[0]);
        FILE* rura_read = fdopen(R1R2[0], "r");
        char line[100];
        int line_number = 1;
        while(fgets(line, sizeof(line), rura_read) != NULL)
        {
            line[strcspn(line, "\n")] = '\0'; // odstranenie noveho riadku
            int length = strlen(line);
            char output[150];
            snprintf(output, sizeof(output), "%d. %s (%d)\n", line_number, line, length);
            write(R2R3[1], output, strlen(output));
            line_number++;
        }
        fclose(rura_read);
        close(R2R3[1]);
        exit(0);

    } 

    int R3R4[2];
    pipe(R3R4);

    int R3R5[2];
    pipe(R3R5);

    if (fork() == 0)
    {
        // treti potomok 
        // printf("I am child number 3 - process %d\n", getpid());
        close(R1R2[0]);
        close(R1R2[1]);  // nepotrebujem ruru 1

        close(R2R3[1]); // nepotrebujem pisat do rury 2

        close(R3R4[0]); // nepotrebujem citat z rury 3 ani 4
        close(R3R5[0]);

        FILE* rura2_read = fdopen(R2R3[0], "r");
        char line[150];
        while(fgets(line, sizeof(line), rura2_read) != NULL)
        {
            // printf("%s", line);
            char *paren = strrchr(line, '(');
            int namelen = 0;
            if (paren) {
                namelen = atoi(paren + 1);
            }

            if (namelen % 2 == 0)
            {
                // parne ide to do R4 - male pismena
                write(R3R4[1], line, strlen(line));

            } else {
                write(R3R5[1], line, strlen(line));
            }
            
        }

        fclose(rura2_read);
        close(R3R4[1]);
        close(R3R5[1]);
        exit(0);
    } 
    
    int R45P[2];
    pipe(R45P);

    if (fork() == 0)
    {
        // stvrty potomok - prijime parne a da na male a posle rodicovi
        close(R1R2[0]); close(R1R2[1]);
        close(R2R3[0]); close(R2R3[1]);
        close(R3R4[1]);
        close(R45P[0]);
        close(R3R5[0]); close(R3R5[1]);

        FILE* in = fdopen(R3R4[0], "r");
        FILE* out = fdopen(R45P[1], "w");
        char line[256];
        while (fgets(line, sizeof(line), in) != NULL)
        {
            for (int i = 0; i < sizeof(line); i++)
            {
                line[i] = tolower((unsigned char)line[i]);
            }
            // printf("[PID %d] R4 mala: %s", getpid(), line);
            fflush(stdout);
            fputs(line, out);
            fflush(out);

        }
        
        fclose(in);
        fclose(out);
        exit(0);
    }

    

    if (fork() == 0)
    {
        // piaty potomok - prijme neparne a da na velke a posle rodicovi
        close(R1R2[0]); close(R1R2[1]);
        close(R2R3[0]); close(R2R3[1]);
        close(R3R4[1]); close(R3R4[0]); 
        close(R3R5[1]);
        close(R45P[0]); 


        FILE* in = fdopen(R3R5[0], "r");
        FILE* out = fdopen(R45P[1], "w");
        char line[256];
        while (fgets(line, sizeof(line), in) != NULL)
        {
            for (int i = 0; i < sizeof(line); i++)
            {
                line[i] = toupper(line[i]);
            }
            // printf("[PID %d] R5 VELKA: %s", getpid(), line);
            fflush(stdout);
            fputs(line, out);
            fflush(out);
        }
        
        fclose(in);
        fclose(out);
        exit(0);
    }
    
    else {

        // rodic
        close(R1R2[0]);
        close(R1R2[1]);
        close(R2R3[0]);
        close(R2R3[1]);
        close(R3R4[1]); 
        close(R3R4[0]);
        close(R3R5[1]); 
        close(R3R5[0]);
        close(R45P[1]);
        

        FILE* rread = fdopen(R45P[0], "r");

        char line[200];
        int count4 = 0;
        int count5 = 0;

        while (fgets(line, sizeof(line), rread)) {

            line[strcspn(line, "\n")] = '\0';

            char* colon = strchr(line, '.');
            int index = -1;

            if (colon) {
                char namebuf[100];
                strncpy(namebuf, colon + 2, sizeof(namebuf));
                namebuf[sizeof(namebuf) - 1] = '\0';

                // odrež na prvej medzere pred "("
                namebuf[strcspn(namebuf, " (")] = '\0';
                if (isupper(namebuf[0]))
                {
                    count5++;
                } else {
                    count4++;
                }
                
                normalize(namebuf);
                for (int i = 0; i < line_count; i++) {
                    if (strcmp(names[i], namebuf) == 0) {
                        index = i + 1;
                        break;
                    }
                }
            }

            printf("[PID %d] Přijat paket: %s -> %d\n", getpid(), line, index);
        }
        
        printf("Router 4 spracoval: %d paketov\n", count4);
        printf("Router 5 spracoval: %d paketov\n", count5); 

        fclose(rread);                
        while (wait(NULL) > 0);
        
    }
            
           
    
    return 0;
}