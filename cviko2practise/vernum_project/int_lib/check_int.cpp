#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "inc/check.h"
int debug_mode = 0;
extern "C"  int check(char* line) {
    long sum = 0;
    long num = 0;
    char* copy = strdup(line);
    char* token = strtok(copy, " \n\t");
    while (token)
    {   
        if (strchr(token, '.') != NULL) {
            printf("Chyba: nie je cele cislo: %s\n", token);
            free(copy);
            return 1;
        }
        if (strncmp(token, "0b", 2) == 0 || strncmp(token, "0B", 2) == 0) {
            printf("Chyba: nie je cele cislo: %s\n", token);
            free(copy);
            return 1;
        }
        num = strtol(token, NULL, 10);
        sum += num;
        token = strtok(NULL, " \n\t");
    }
    sum -= num;
    if (sum != num)
    {
        printf("Suma nie je spravna %ld != %ld", sum, num);
    } else {
        if (debug_mode)
            printf("[DEBUG] Súčet = %ld, riadok = %s", sum, line);
        else
            printf("OK %s", line);
    }
    free(copy);
    return 0;
}