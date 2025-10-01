#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
/*

Upravte si svůj generátor gennum M N čísel z posledního cvičení/přípravy (není nutno ho i nadále mít se statickou knihovnou) tak, 
aby měl stále dva parametry M N, kde M je max. počet čísel na řádek a N bude počet řádků za minutu. Program bude vypisovat řádky, dokud se neukončí pomocí <ctrl-c>.

Pusťte si svůj generátor ./gennum M N > out.txt.

V dalším terminálu si pusťte tail -f out.txt a ten by měl monitorovat, jak narůstá obsah souboru out.txt. 
Svůj generátor můžete zastavit a soubor zkrátit (ne smazat) pomocí truncate -s 0 out.txt a generátor pustit znovu.

*/
void generate_line(int M) {
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
        printf("%d ", value);
    }
    printf(" (sum: %ld)", sum);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Pouzitie: %s M N\n", argv[0]);
        return 1;
    }

    int M = atoi(argv[1]);
    int N = atoi(argv[2]);

    if (M < 1 || N < 1) {
        fprintf(stderr, "M a N musia byt kladne cele cisla.\n");
        return 1;
    }

    srand(time(NULL));
    while (1) {
        generate_line(M);
        printf("\n");
        fflush(stdout);  
        usleep(60000000 / N);
    }
    

    return 0;
}