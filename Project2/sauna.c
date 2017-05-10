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
int NUM_REQUESTS; //numero de pedidos
int VALID_REQUESTS; //numero de pedidos validos
int REJECTED_REQUESTS; //numero de pedidos rejeitados
char* GENERATE_FIFO = "/tmp/entrada";
char* REJECT_FIFO =  "/tmp/rejeitados";

typedef struct {
        int id; //numero do pedido
        char gender; //genero
        int duration; //duracao
        int denials; //numero de rejeicoes
} Request;

Request* requestList[256]; //array de pedidos
Request* validRequests[256]; //array de pedidos validos
Request* rejectedRequests[256]; //array de pedidos validos

char ALLOWED_GENDER;

void printStats() {

        int i;
        for(i = 0; i < VALID_REQUESTS; i++)
                printf(" > SAUNA: P:%i-G:%c-T:%i;\n", validRequests[i]->id, validRequests[i]->gender, validRequests[i]->duration);

}

int validateRequest(Request *request) {

        if(request->gender != ALLOWED_GENDER)
                return 0;
        else return 1;
}

void manageRequests() {

        int i, j = 0, k = 0;
        for(i = 0; i < NUM_REQUESTS; i++) {

                if(validRequests[0] == NULL) {
                        ALLOWED_GENDER = requestList[i]->gender;
                        validRequests[j] = requestList[i];
                        j++;
                } else {

                        if(validateRequest(requestList[i]) != 0) {
                                validRequests[j] = requestList[i];
                                j++;
                        } else {
                                requestList[i]->denials = requestList[i]->denials + 1;
                                rejectedRequests[k] = requestList[i];
                                k++;
                        }
                }
        }

        VALID_REQUESTS = j;
        REJECTED_REQUESTS = k;

        return;
}

void *requestReceptor(void *arg) {

        //abertura do FIFO de entrada

        int fd_generator;
        while ((fd_generator = open(GENERATE_FIFO, O_RDONLY)) == -1) {
                if (errno == EEXIST)
                        printf(" > SAUNA: FIFO 'entrada' doesnt exist! Retrying...\n");
        }

        printf(" > SAUNA: FIFO 'entrada' openned in READONLY mode\n");

        //Ler pedidos do FIFO de entrada

        Request *request = malloc(sizeof(Request));
        int i = 0;

        while(read(fd_generator, request, sizeof(Request)) != 0) {
                requestList[i] = request;
                printf(" > SAUNA: P:%i-G:%c-T:%i;\n", requestList[i]->id, requestList[i]->gender, requestList[i]->duration);
                i++;
                request = malloc(sizeof(Request));
        }

        NUM_REQUESTS = i;

        //fecha FIFO de entrada

        close(fd_generator);

        manageRequests();

        //FIFO DE REJEITADOS

        //criacao FIFO de rejeitados

        if (mkfifo(REJECT_FIFO, S_IRUSR | S_IWUSR) != 0) {
                if (errno == EEXIST)
                        printf(" > SAUNA: FIFO '/tmp/rejeitados' already exists\n");
                else
                        printf("> SAUNA: Can't create FIFO '/tmp/rejeitados'\n");
        }
        else
                printf(" > SAUNA: FIFO created.\n");

        //abertura FIFO de rejeitados

        int fd_reject;

        if ((fd_reject = open(REJECT_FIFO,O_RDWR | O_NONBLOCK)) == -1) {
                printf(" > SAUNA: Could not open fifo!\n"); //TODO: mudar mensagem
                exit(1);
        }

        printf(" > SAUNA: FIFO 'rejeitados' openned in READ and WRITE mode\n");

        //escrita no FIFO

        int j;
        for(j = 0; j < REJECTED_REQUESTS; j++)
                write(fd_reject, rejectedRequests[j], sizeof(Request));

        //fecha FIFO de rejeitados

        close(fd_reject);

        printStats();

        pthread_exit(NULL);
}

int main(int argc, char* argv[]) {

        //Tratamento de argumentos

        if (argc != 2) {
                printf(" > SAUNA: The number of arguments of %s is not correct!", argv[0]);
                exit(1);
        }

        CAPACITY = atoi(argv[1]);
        NUM_REQUESTS = 0;

        //Thread que analisa os pedidos do fifo de entrada
        pthread_t tid1;
        pthread_create(&tid1, NULL, requestReceptor, NULL);

        //Thread que escuta os pedidos rejeitados
        //  pthread_t tid2;
        //pthread_create(&tid2, NULL, escutarPedidosRejeitados, NULL);
        pthread_join(tid1, NULL);
        unlink(REJECT_FIFO);
        exit(0);
}
