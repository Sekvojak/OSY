#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "inc/check.h"
int debug_mode = 0;
extern "C" int check(char* line) {
    double sum = 0;
    double num = 0;
    char* copy = strdup(line);
    char* token = strtok(copy, " \n\t");
    while (token)
    {   
        if (strncmp(token, "0b", 2) == 0 || strncmp(token, "0B", 2) == 0) {
            printf("Chyba: nie je desatinne cislo: %s\n", token);
            free(copy);
            return 1;
        }
        num = strtod(token, NULL);
        sum += num;
        token = strtok(NULL, " \n\t");
    }
    sum -= num;
    if (sum != num)
    {
        printf("Suma nie je spravna %.2f != %.2f\n", sum, num);
    } else {
        if (debug_mode)
            printf("[DEBUG] Súčet = %.2f, riadok = %s", sum, line);
        else
            printf("OK %s", line);
    }
    free(copy);
    return 0;
}