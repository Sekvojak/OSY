#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "gen_line.h"
int main(int argc, char *argv[]) {
    if (argc < 3)
    {
        fprintf(stderr,"Pouzitie: %s [M] [N] [-f|-b]\n", argv[0]);
        return 1;
    }
    int M = atoi(argv[1]);
    int N = atoi(argv[2]);
    int float_flag = 0;
    int binary_flag = 0;
    if (argc == 4 && strcmp(argv[3], "-f") == 0)
    {
        float_flag = 1;
    } else if (argc == 4 && strcmp(argv[3], "-b") == 0)
    {
        binary_flag = 1;
    }
    
    if (M <= 0 || N <= 0) {
        fprintf(stderr, "M a N musia byt kladne cele cisla\n");
        return 1;
    }
    srand(time(NULL));
    for (int i = 0; i < N; i++)
    {
        generate_line(M, float_flag, binary_flag);
    }
    
    return 0;
}