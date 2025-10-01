#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>


struct mystat {
    char name[256];
    struct stat st;
};


void print_permissions(mode_t mode) {
    char perms[11];

    // typ súboru
    if (S_ISDIR(mode)) perms[0] = 'd';
    else if (S_ISREG(mode)) perms[0] = '-';
    else if (S_ISLNK(mode)) perms[0] = 'l';
    else perms[0] = '?';

    // užívateľ
    perms[1] = (mode & S_IRUSR) ? 'r' : '-';
    perms[2] = (mode & S_IWUSR) ? 'w' : '-';
    perms[3] = (mode & S_IXUSR) ? 'x' : '-';

    // skupina
    perms[4] = (mode & S_IRGRP) ? 'r' : '-';
    perms[5] = (mode & S_IWGRP) ? 'w' : '-';
    perms[6] = (mode & S_IXGRP) ? 'x' : '-';

    // ostatní
    perms[7] = (mode & S_IROTH) ? 'r' : '-';
    perms[8] = (mode & S_IWOTH) ? 'w' : '-';
    perms[9] = (mode & S_IXOTH) ? 'x' : '-';

    perms[10] = '\0';

    printf("%-16s", perms);
}

int cmp_size(const void *a, const void *b) {
    struct mystat *fa = (struct mystat*)a;
    struct mystat *fb = (struct mystat*)b;
    if (fa->st.st_size < fb->st.st_size) return -1;
    if (fa->st.st_size > fb->st.st_size) return 1;
    return 0;
}

void print_file_info(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("fopen");
        return;
    }

    char line[1024];
    int total_lines = 0;

    // linecount
    while (fgets(line, sizeof(line), file) != NULL) {
        total_lines++;
    }

    rewind(file); // back to start
    printf("------------------------------\n");

    if (total_lines <= 10) {
        // less or equal 10 lines → all
        while (fgets(line, sizeof(line), file) != NULL) {
            printf("%s", line);
        }
    } else {
        // more than 10 lines → first 5 and last 5
        for (int i = 0; i < 5; i++) {
            if (fgets(line, sizeof(line), file) != NULL) {
                printf("%s", line);
            }
        }

        printf("....\n");

        // load last 5 lines into buffer
        char *last5[5] = {NULL};
        int idx = 0;
        while (fgets(line, sizeof(line), file) != NULL) {
            if (last5[idx]) free(last5[idx]);
            last5[idx] = strdup(line);
            idx = (idx + 1) % 5;
        }

        for (int i = 0; i < 5; i++) {
            int j = (idx + i) % 5;
            if (last5[j]) {
                printf("%s", last5[j]);
                free(last5[j]);
            }
        }
    }

    printf("------------------------------\n");
    fclose(file);
}

int main(int argc, char *argv[]) {

    if (argc < 2) {
        fprintf(stderr, "Použitie: %s subor1 subor2 ...\n", argv[0]);
        return 1;
    }

    struct mystat files[1024];
    int fcount = 0;
    
    for (int i = 1; i < argc; i++) {
        struct stat st;

        // printf("Spracovavam: %s\n", argv[i]);

        if (stat(argv[i], &st) != 0) {
            perror(argv[i]);
            continue;  
        }

        // vynechaj adresáre
        if (S_ISDIR(st.st_mode)) continue;

        // vynechaj spustiteľné
        //if ((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH))
          //continue;

        // vynechaj nečitateľné
        if (access(argv[i], R_OK) != 0) continue;

        strcpy(files[fcount].name, argv[i]);
        files[fcount].st = st;
        fcount++;
    }
    
    qsort(files, fcount, sizeof(struct mystat), cmp_size);

    for (int i = 0; i < fcount; i++) {
        char *time_str = asctime(localtime(&files[i].st.st_mtime));
        time_str[strlen(time_str)-1] = '\0'; 
        print_permissions(files[i].st.st_mode);
        printf(" %10ld %24s %s\n",
               files[i].st.st_size,
               time_str,
               files[i].name);
    
        print_file_info(files[i].name);
    }



    return 0;
}