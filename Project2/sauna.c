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

int CAPACITY; //numero de lugares na sauna
char* GENERATE_FIFO = "/tmp/entrada";
char* REJECT_FIFO =  "/tmp/rejeitados";

typedef struct {
        int id; //numero do pedido
        char gender; //genero
        int duration; //duracao
        int denials; //numero de rejeicoes
} Request;

Request* requestList[256]; //array de pedidos

int main(int argc, char* argv[]) {

        //Tratamento de argumentos

        if (argc != 2) {
                printf(" > SAUNA: The number of arguments of %s is not correct!", argv[0]);
                exit(1);
        }

        CAPACITY = atoi(argv[1]);

        //FIFO

        //abertura do FIFO

        int fd_generator;
        while ((fd_generator = open(GENERATE_FIFO, O_RDONLY)) == -1) {
                if (errno == EEXIST)
                        printf(" > SAUNA: FIFO 'entrada' doesnt exist! Retrying...\n");
                sleep(1);
        }

        printf(" > SAUNA: FIFO 'entrada' openned in READONLY mode\n");

        //Ler pedidos do FIFO

        Request *request = malloc(sizeof(Request));

        while(read(fd_generator, request, sizeof(Request)) != 0) {
                printf(" > SAUNA: P:%i-G:%c-T:%i;\n", request->id, request->gender, request->duration);
        }

        //fecha FIFO de entrada

        close(fd_generator);

        exit(0);
}
