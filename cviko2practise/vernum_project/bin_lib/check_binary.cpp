#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../inc/check.h"
int debug_mode = 0;
extern "C" int check(char* line) {
    unsigned long sum = 0, num = 0, x = 0;
    char* copy = strdup(line);
    char* token = strtok(copy, " \n\t");
    while (token) {
        if (sscanf(token, "0b%lb", &x) == 1) {
            num = x;
            sum += num;
        } 
        
        else {
            printf("Chyba: nie je binarne cislo: %s\n", token);
            free(copy);
            return 1;
        }
            
        token = strtok(NULL, " \n\t");
    }
    sum -= num;
    if (sum != num) {
        printf("Chyba: suma nesedí %lu != %lu\n", sum, num);
    } else {
        if (debug_mode)
            printf("[DEBUG] Súčet = %lu, riadok = %s", sum, line);
        else
            printf("OK %s", line);
    }
    free(copy);
    return 0;
}