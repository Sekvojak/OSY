#include <stdio.h>
#include <stdlib.h>
#include "gen_line.h"
void generate_line(int M, int float_flag, int binary_flag) {
    if (M < 1)
    {
        return;
    }
    int count = 1 + rand() % M;
    long sum = 0;
    for (int i = 0; i < count; i++)
    {
        int value = 10 + rand() % (9999 - 10 + 1);
        sum += value;
        if (float_flag)
        {
            printf("%d.00 ", value);
        } else if (binary_flag)
        {
            printf("0b%b ", value);
        } else {
            printf("%d ", value);
        }
    }
    if (float_flag)
    {
        printf("%ld.00\n", sum);
    } else if (binary_flag)
    {
        printf("0b%lb", sum);
        printf("\n");
    } else {
        printf("%ld\n", sum);
    }
    
}