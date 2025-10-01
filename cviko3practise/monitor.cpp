#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/*
Napište si svůj program monitor tak, aby se choval stejně jako tail -f .... Pokud není možno zadaný soubor číst, tak program končí. 
Otevřený soubor nezavírejte, používejte open nebo fopen a pro čtení read nebo fread. 
Stav souboru kontrolujte cca 1x za vteřinu pomocí stat/fstat a dle potřeby použijte lseek nebo fseek.

Vylepšete ho tak, že bude nejen detekovat zkrácení souboru, ale vypíše vždy při změně velikosti souboru i informaci v jakém čase a k jaké změně velikosti došlo.
*/

struct monitor_file {
    char name[256];
    FILE *file;
    off_t lastSize;
};

int main(int argc, char *argv[]) {


    if (argc < 2)
    {
        fprintf(stderr, "Pouzitie: %s [subor]\n", argv[0]);
    }
    
    char* fileName = argv[1];

    struct monitor_file files[argc - 1];
    int fcount = 0;

    for (int i = 1; i < argc; i++)
    {
        FILE *file = fopen(argv[i], "r");
        if (!file) {
            perror("fopen");
            return 1;
        }
        struct stat st;
        if (stat(argv[i], &st) != 0)
        {
            perror("stat");
            return 1;   
        }

        strcpy(files[fcount].name, argv[i]);
        files[fcount].file = file;
        files[fcount].lastSize = st.st_size;
        fcount++;
        
    }
    

    while (1) {
        for (int i = 0; i < fcount; i++) {
            FILE *file = files[i].file;
            struct stat fileStat;
            
            if (stat(files[i].name, &fileStat) != 0) {
                perror("stat");
                continue;
            }

            off_t lastSize = files[i].lastSize;
            long diff = fileStat.st_size - lastSize;

            if (fileStat.st_size < lastSize) {
                printf("Subor %s bol zmenšený z %ld na %ld \n", files[i].name, lastSize, fileStat.st_size);    
                fseek(file, 0, SEEK_SET);
            } else if (fileStat.st_size > lastSize){
                printf("Subor %s bol zväčšený z %ld na %ld \n", files[i].name, lastSize, fileStat.st_size);

                char* buffer = (char*)malloc(diff + 1);
                if (!buffer) {
                    perror("malloc");
                    continue;
                }

                fseek(file, lastSize, SEEK_SET);
                size_t n = fread(buffer, 1, diff, file);
                buffer[n] = '\0';
                printf("%s", buffer);
                free(buffer);
            }
            
            files[i].lastSize = fileStat.st_size;

        }
        sleep(1);
    }

    
    for (int i = 0; i < fcount; i++) {
        fclose(files[i].file);
    }

    return 0;
}