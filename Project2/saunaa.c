#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>

int nLugares; //numero de lugares na sauna

int main(int argc, char* argv[]) {

        //Tratamento de argumentos
        if (argc != 2) {
                printf(" > SAUNA: The number of arguments of %s is not correct!", argv[0]);
                exit(1);
        }
        nLugares = atoi(argv[1]);

        ///FIFO///

        //Criar FIFO entrada
        if (mkfifo("/tmp/entrada",0660) < 0) {
                if (errno==EEXIST)
                        printf(" > SAUNA: FIFO '/tmp/entrada' already exists\n");
                else printf("> SAUNA: Can't create FIFO '/tmp/entrada'\n");
        }
        else printf(" > SAUNA: FIFO created.\n");

        //Abrir FIFO para leitura

        int fd_entrada;
        fd_entrada=open("/tmp/entrada",O_RDONLY);
        printf(" > SAUNA: FIFO 'entrada' openned in READONLY mode\n");

        //Ler pedidos do FIFO
        char str[12];
        int n=1;
        do {
                n= read(fd_entrada, str, 20);
                if(n!=0) {
                        printf(" > SAUNA: %s", str);
                }
        } while(n!=0);

        //Fechar FIFO
        close(fd_entrada);

        //Remover FIFO
        if (unlink("/tmp/entrada")<0)
                printf(" > SAUNA: Error when destroying FIFO '/tmp/entrada'\n");
        else
                printf(" > SAUNA: FIFO '/tmp/entrada' has been destroyed\n");

        exit(0);
}
