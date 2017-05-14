#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>

FILE* gerFile;

int NUM_REQUESTS; //numero de pedidos gerados
int MAX_TIME; //maximo tempo de utlização na sauna

int ENTRADA_FIFO_FD; //file descriptor do FIFO de entrada
int REJEITADOS_FIFO_FD; //file descriptor do FIFO de rejeitados

struct timeval begin; //struct que guarda a hora de inicio do programa

int M_PEDIDOS=0;
int M_REJEITADOS=0;
int M_DESCARTADOS=0;
int F_PEDIDOS=0;
int F_REJEITADOS=0;
int F_DESCARTADOS=0;

typedef struct {
        int id; //numero do pedido
        char gender; //genero
        int duration; //duracao
        int denials; //numero de rejeicoes
} Request;

void printStats(){

        printf(" [GERADOR PEDIDOS    ] > m: %d\n", M_PEDIDOS);
        printf(" [GERADOR PEDIDOS    ] > f: %d\n", F_PEDIDOS);
        printf(" [GERADOR PEDIDOS    ] > total: %d\n", M_PEDIDOS + F_PEDIDOS);

        printf(" [GERADOR REJEITADOS ] > m: %d\n", M_REJEITADOS);
        printf(" [GERADOR REJEITADOS ] > f: %d\n", F_REJEITADOS);
        printf(" [GERADOR REJEITADOS ] > total: %d\n", M_REJEITADOS+ F_REJEITADOS);

        printf(" [GERADOR DESCARTADOS] > m: %d\n", M_DESCARTADOS);
        printf(" [GERADOR DESCARTADOS] > f: %d\n", F_DESCARTADOS);
        printf(" [GERADOR DESCARTADOS] > total: %d\n", M_DESCARTADOS + F_DESCARTADOS);
}

void printFile(Request *request, char* tip){

        struct timeval end; //struct que guarda a hora do instante pretendido
        gettimeofday(&end, NULL);
        double inst = (end.tv_sec - begin.tv_sec)*1000.0f + (end.tv_usec - begin.tv_usec) / 1000.0f; //milissegundos depois do inicio do programa

        fprintf(gerFile, "%-9.2f - %-4d - %-4d: %-1c - %-4d - %-10s\n", inst, getpid(),request->id,request->gender, request->duration, tip);

        if(request->gender=='M') {
                if(strcmp(tip,"PEDIDO")==0) M_PEDIDOS++;
                if(strcmp(tip,"REJEITADO")==0) M_REJEITADOS++;
                if(strcmp(tip,"DESCARTADO")==0) M_DESCARTADOS++;
        }
        else{
                if(strcmp(tip,"PEDIDO")==0) F_PEDIDOS++;
                if(strcmp(tip,"REJEITADO")==0) F_REJEITADOS++;
                if(strcmp(tip,"DESCARTADO")==0) F_DESCARTADOS++;
        }

}

void *escutarPedidosRejeitados(void *arg) {

        Request* request = malloc(sizeof(Request));

        while(read(REJEITADOS_FIFO_FD, request, sizeof(Request)) != 0) {
                if(request->id!=0) { // le se houver alguma coisa para ler
                        if(request->id==-1) break;  //quando o id do pedido é -1, quer dizer que a sauna nao vai mandar mais pedidos
                        //printf(". GERADOR (rejeitado): P:%i-G:%c-T:%i-D:%i;\n", request->id, request->gender, request->duration, request->denials);
                        if(request->denials<3) {
                                printFile(request, "REJEITADO");
                                write(ENTRADA_FIFO_FD, request, sizeof(Request));
                                printFile(request, "PEDIDO");
                        }
                        else printFile(request, "DESCARTADO");

                }
        }
        pthread_exit(NULL);

}


void *requestGenerator(void *arg) {
        int i;
        for (i = 1; i <= NUM_REQUESTS; i++) {
                Request *request = malloc(sizeof(Request));
                request->id = i;
                request->gender = (rand() % 2) ? 'M' : 'F';
                request->duration = rand() % MAX_TIME + 1;
                request->denials = 0;

                write(ENTRADA_FIFO_FD, request, sizeof(Request));
                printFile(request, "PEDIDO");
                //printf(". GERADOR (pedido):P:%i-G:%c-T:%i-D:%i;\n", request->id, request->gender, request->duration, request->denials);
        }

        pthread_exit(NULL);
}

void openRejeitadosFifo() {

        while ((REJEITADOS_FIFO_FD = open("/tmp/rejeitados", O_RDONLY)) == -1) {
                if (errno == EEXIST)
                        printf(". GERADOR: FIFO 'rejeitados' doesnt exist! Retrying...\n");
        }

        //printf(". GERADOR: FIFO 'rejeitados' openned in READONLY mode\n");

        return;

}

void makeOpenEntradaFifo() {

        if (mkfifo("/tmp/entrada", S_IRUSR | S_IWUSR) != 0) {
                if (errno == EEXIST)
                        printf(". GERADOR: FIFO '/tmp/entrada' already exists\n");
                else
                        printf(". GERADOR: Can't create FIFO '/tmp/entrada'\n");
        }
        //else printf(". GERADOR: FIFO 'entrada' created.\n");

        while ((ENTRADA_FIFO_FD = open("/tmp/entrada", O_WRONLY | O_NONBLOCK)) == -1) {
                printf(". GERADOR: Waiting for SAUNA to open 'entrada'...\n");
        }
        //printf(". GERADOR: FIFO 'entrada' opened in WRITEONLY mode\n");

        return;
}


int main(int argc, char* argv[]) {

        gettimeofday(&begin, NULL); //guardar na struct a hora de inicio do programa

        srand(time(NULL)); //para gerar numeros aleatorios ao longo do programa

        //Tratamento de argumentos
        if (argc != 3) {
                printf(". GERADOR: The number of arguments of %s is not correct!", argv[0]);
                exit(1);
        }

        NUM_REQUESTS = atoi(argv[1]);
        MAX_TIME = atoi(argv[2]);

        //Criaçao do ficheiro de registo
        int pid;
        pid = getpid();
        char gerPathname[20];
        sprintf(gerPathname, "/tmp/ger.%d", pid);
        gerFile = fopen(gerPathname, "w");

        if (gerFile == NULL)
                printf(". GERADOR: Error opening gerFile\n");

        //Criação e abertura de FIFO de entrada
        makeOpenEntradaFifo();

        //Abertura FIFO rejeitados
        openRejeitadosFifo();

        //Escrever numero de pedidos
        write(ENTRADA_FIFO_FD, &NUM_REQUESTS, sizeof(int));

        //Thread que gera pedidos aleatorios
        pthread_t tid1;
        pthread_create(&tid1, NULL, requestGenerator, NULL);

        //Thread que escuta os pedidos rejeitados
        pthread_t tid2;
        pthread_create(&tid2, NULL, escutarPedidosRejeitados, NULL);

        pthread_join(tid1, NULL);
        pthread_join(tid2, NULL);
        printStats();

        unlink("/tmp/entrada");

        return 0;
}
