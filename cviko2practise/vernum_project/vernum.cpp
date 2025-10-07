
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "inc/check.h"
extern int debug_mode;
int main(int argc, char *argv[]) {
    int dflag = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0) {
            dflag = 1;
        }
    }
    debug_mode = dflag;
    char line[1024];
    while (fgets(line, sizeof(line), stdin)) {
        check(line);
    }
    return 0;
}