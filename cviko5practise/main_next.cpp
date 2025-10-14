#include <stdio.h>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <fcntl.h> 

/*
Při testování používejte soubor names.txt z přípravy s více než 58000 jmény!

Upravte si program tak, aby měl nejprve jediný parametr, a to znak velké abecedy, např. ./mujprog B. Budou se hledat jména začínající daným znakem.

Program si vytvoří dva potomky a dvě roury tak, aby provedly příkaz grep ^B names.txt | wc -l a druhou rourou posílá wc výsledek rodiči.

Rodič si výsledek přečte a vypíše i informaci, pro jaký znak má výsledek. Následně waitem počká na potomky.

Upravte program tak, aby bylo možno zadat více znaků, např. ./mujprog A E I O U a příkaz grep ^? names.txt | wc -l a hledání dle předchozího bodu se zopakuje tolikrát, kolik znaků je zadáno. Jednotlivá hledání tedy proběhnou sériově.

Bude-li při spuštění zadáno ./mujprog AZ, prohledá se celá abeceda od A do Z.

Nejprve se vytvoří všech 26x2 potomků a rour, pak si rodič z 26 rour přečte výsledky, a pak 52x počká wait na potomky. Hledání pro všechny znaky tak proběhne paralelně.

Kontrolu svých výsledků si můžete porovnat s výsledky příkazu: for i in ^{A..Z}; do echo -n "$i "; grep $i names.txt | wc -l ; done

*/

int main(int argc, char* argv[]) {

    if (argc < 2)
    {
        fprintf(stderr, "Pouzitie: %s <meno suboru>\n", argv[0]);
        return 1;
    }

    if (argc == 2 && argv[1][0] == 'A' && argv[1][1] == 'Z') 
    {
        // paralelne
        int rura1[26][2];
        int rura2[26][2];
        for (int i = 0; i < 26; i++)
        {   
            pipe(rura1[i]);
            pipe(rura2[i]);
            char ch = 'A' + i;
            if (fork() == 0)
            {
                close(rura2[i][0]); close(rura2[i][1]);
                close(rura1[i][0]);

                dup2(rura1[i][1], 1);
                close(rura1[i][1]);

                char pattern[8];
                snprintf(pattern, sizeof(pattern), "^%c", ch);

                execlp("grep", "grep", pattern, "names.txt", NULL);
                perror("execlp");
                return 1;
                
            }
            
            if (fork() == 0)
            {
                close(rura1[i][1]);
                close(rura2[i][0]);

                dup2(rura1[i][0], 0);
                dup2(rura2[i][1], 1);

                close(rura1[i][0]);
                close(rura2[i][1]);

                execlp("wc", "wc", "-l", NULL);
                perror("execlp");
                return 1;

            }
            
            close(rura1[i][0]); close(rura1[i][1]);
            close(rura2[i][1]);
        }


        for (int i = 0; i < 26; i++)
        {

            char buffer[1024];
            int r;
            while ((r = read(rura2[i][0], buffer, sizeof(buffer))) > 0)
            {
                buffer[r-1] = '\0';
                printf("^%c %s\n", 'A' + i, buffer);
            }
            

            close(rura2[i][0]);
        }

        for (int i = 0; i < 52; i++)
        {
            wait(NULL);
        }



    } else {
        for (int i = 1; i < argc; i++)
        {
            int rura1[2];
            pipe(rura1);

            int rura2[2];
            pipe(rura2);

            if (fork() == 0)
            {   
                // potomok 1 ---- grep ^B names.txt
                
                close(rura2[0]); close(rura2[1]);

                close(rura1[0]);
                dup2(rura1[1], 1); // presmerovanie stdout na ruru / nech stdout ide do rury
                close(rura1[1]);

                
                char pattern[8];
                snprintf(pattern, sizeof(pattern), "^%c", argv[i][0]);

                execlp("grep", "grep", pattern, "names.txt", NULL);
                perror("execlp");
                
                return 1;

            }

            if (fork() == 0)
            {
                // potomok 2
                close(rura1[1]);
                close(rura2[0]);

                dup2(rura1[0], 0); // nech stdin cita z rury1
                close(rura1[0]);

                dup2(rura2[1], 1); // nech stdout ide do rury2
                close(rura2[1]);

                // bod 1
                
                execlp("wc", "wc", "-l", NULL);
                perror("execlp");
                return 1;
                
            }

            close(rura1[0]); close(rura1[1]);
            close(rura2[1]);
            

            char buffer[1024];
            int r;
            while ((r = read(rura2[0], buffer, sizeof(buffer))) > 0)
            {
                buffer[r-1] = '\0';
                printf("^%c %s\n", argv[i][0], buffer);
            }
            
            close(rura2[0]); 


            while (wait(NULL) > 0);

        }


    }
    



    return 0;
}