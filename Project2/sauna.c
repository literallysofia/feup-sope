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
#include <pthread.h>
#include <time.h>

FILE* balFile;

int CAPACITY; //capacidade  da sauna

char ALLOWED_GENDER = 'X'; //genero quando a sauna esta vazia
int REQUESTS_TO_READ; //numero de pedidos ainda por ler
int NUM_PEOPLE_IN = 0; //numero de pessoas dentro da sauna

int ENTRADA_FIFO_FD; //file descriptor do FIFO de entrada
int REJEITADOS_FIFO_FD; //file descriptor do FIFO de rejeitados

struct timeval begin; //struct que guarda a hora de inicio do programa

pthread_t threadsTid[255]; //array com os tids das threads a correr
int threadPos=0;

int M_SERVIDOS=0;
int M_REJEITADOS=0;
int M_RECEBIDOS=0;
int F_SERVIDOS=0;
int F_REJEITADOS=0;
int F_RECEBIDOS=0;

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
        int id; //numero do pedido
        char gender; //genero
        int duration; //duracao
        int denials; //numero de rejeicoes
} Request;

void printStats(){

        printf(" [SAUNA RECEBIDOS    ] > m: %d\n", M_RECEBIDOS);
        printf(" [SAUNA RECEBIDOS    ] > f: %d\n", F_RECEBIDOS);
        printf(" [SAUNA RECEBIDOS    ] > total: %d\n", M_RECEBIDOS + F_RECEBIDOS);

        printf(" [SAUNA SERVIDOS     ] > m: %d\n", M_SERVIDOS);
        printf(" [SAUNA SERVIDOS     ] > f: %d\n", F_SERVIDOS);
        printf(" [SAUNA SERVIDOS     ] > total: %d\n", M_SERVIDOS + F_SERVIDOS);

        printf(" [SAUNA REJEITADOS   ] > m: %d\n", M_REJEITADOS);
        printf(" [SAUNA REJEITADOS   ] > f: %d\n", F_REJEITADOS);
        printf(" [SAUNA REJEITADOS   ] > total: %d\n", M_REJEITADOS + F_REJEITADOS);
}

void printFile(Request *request, int tid, char* tip){

        struct timeval end; //struct que guarda a hora do instante pretendido
        gettimeofday(&end, NULL);
        double inst = (end.tv_sec - begin.tv_sec)*1000.0f + (end.tv_usec - begin.tv_usec) / 1000.0f; //milissegundos depois do inicio do programa

        fprintf(balFile, "%-9.2f - %-4d - %-20lu - %-4d: %-1c - %-4d - %-10s\n", inst, getpid(), (long)tid,request->id,request->gender, request->duration, tip);

        if(request->gender=='M') {
                if(strcmp(tip,"REJEITADO")==0) M_REJEITADOS++;
                if(strcmp(tip,"RECEBIDO")==0) M_RECEBIDOS++;
                if(strcmp(tip,"SERVIDO")==0) M_SERVIDOS++;
        }
        else{
                if(strcmp(tip,"REJEITADO")==0) F_REJEITADOS++;
                if(strcmp(tip,"RECEBIDO")==0) F_RECEBIDOS++;
                if(strcmp(tip,"SERVIDO")==0) F_SERVIDOS++;
        }

}


void manageRejected(Request* request){

        request->denials = request->denials + 1;

        if(request->denials < 3)
                REQUESTS_TO_READ++;  //como foi rejeitado menos que 3 vezes quer dizer que eventualmente vai chegar à sauna novamente

        //printf(". SAUNA (rejeitado): P:%i-G:%c-T:%i-D:%i;\n", request->id, request->gender, request->duration, request->denials);
        write(REJEITADOS_FIFO_FD, request, sizeof(Request));
}


void *stayingInSauna(void *arg) {

        Request *request = (Request*)arg;

        //printf(". SAUNA: %d entrou\n",request->id);
        usleep(request->duration*1000);

        printFile(request, pthread_self(),"SERVIDO");

        pthread_mutex_lock(&mut);
        NUM_PEOPLE_IN--; //a pessoa sai
        pthread_mutex_unlock(&mut);

        //printf(". SAUNA: %d saiu\n",request->id);

        //printf(". SAUNA: PEOPLE IN SAUNA: %d\n", NUM_PEOPLE_IN);

        if(NUM_PEOPLE_IN==0) {
                ALLOWED_GENDER = 'X';
                //printf(". SAUNA: allowed gender: %c\n",ALLOWED_GENDER);
        }


        pthread_exit(NULL);
}

int validateRequest(Request *request) {

        if(request->gender != ALLOWED_GENDER) //se o genero for diferente do correntemente na sauna
                return 0;
        else if (NUM_PEOPLE_IN >= CAPACITY) //se a sauna estiver cheia
                return 0;
        else return 1;
}


void manageRequest(Request* request){


        if(ALLOWED_GENDER=='X') {     //primeira pessoa na sauna
                ALLOWED_GENDER = request->gender;       //o genero da sauna muda
                //printf(". SAUNA: allowed gender: %c\n",ALLOWED_GENDER);
                //printf(". SAUNA (servido): P:%i-G:%c-T:%i-D:%i;\n", request->id, request->gender, request->duration, request->denials);
                NUM_PEOPLE_IN++;       //numero de pessoas na sauna aumenta
                printFile(request, getpid(),"RECEBIDO");
                pthread_create(&threadsTid[threadPos], NULL, stayingInSauna,request);
                threadPos++;
        } else {
                if(validateRequest(request) != 0) {
                        //printf(". SAUNA (servido): P:%i-G:%c-T:%i-D:%i;\n", request->id, request->gender, request->duration, request->denials);
                        NUM_PEOPLE_IN++;   //numero de pessoas na sauna aumenta
                        printFile(request, getpid(),"RECEBIDO");
                        pthread_create(&threadsTid[threadPos], NULL, stayingInSauna,request);
                        threadPos++;
                } else {
                        printFile(request, getpid(),"RECEBIDO");
                        printFile(request, getpid(), "REJEITADO");
                        manageRejected(request);

                }
        }
        return;

}

void requestsReceptor() {
        Request* request;
        int n;
        while(REQUESTS_TO_READ != 0) { //enquanto houver pedidos para ler
                request = malloc(sizeof(Request));
                n=read(ENTRADA_FIFO_FD, request, sizeof(Request));
                if(n>0) {
                        //printf(". SAUNA REQUESTS TO READ: %d\n", REQUESTS_TO_READ);
                        manageRequest(request);
                        REQUESTS_TO_READ--; //um pedido foi lido, decrementar pedidos para ler

                }
        }
        if(REQUESTS_TO_READ==0) { //fim do pogramas
                Request* request =malloc(sizeof(Request));
                request->id=-1; //quando o gerador recebe um pedido com id -1, para de ler
                write(REJEITADOS_FIFO_FD,  request, sizeof(Request));
                close(REJEITADOS_FIFO_FD); //fechar fifo de rejeitados
                close(ENTRADA_FIFO_FD); //fechar fifo de entrada
        }
        return;
}

void openEntradaFifo() {

        while ((ENTRADA_FIFO_FD = open("/tmp/entrada", O_RDONLY)) == -1) {
                if (errno == EEXIST)
                        printf(". SAUNA: FIFO 'entrada' doesnt exist! Retrying...\n");
        }

        //printf(". SAUNA: FIFO 'entrada' openned in READONLY mode\n");

        return;
}

void makeOpenRejeitadosFifo() {

        //criacao
        if (mkfifo("/tmp/rejeitados", S_IRUSR | S_IWUSR) != 0) {
                if (errno == EEXIST)
                        printf(". SAUNA: FIFO '/tmp/rejeitados' already exists\n");
                else
                        printf("> SAUNA: Can't create FIFO '/tmp/rejeitados'\n");
        }
        //else printf(". SAUNA: FIFO 'rejeitados' created.\n");

        //abertura
        while ((REJEITADOS_FIFO_FD = open("/tmp/rejeitados", O_WRONLY | O_NONBLOCK)) == -1) {
                printf(". SAUNA: Waiting for SAUNA to open 'rejeitados'...\n");
        }
        //printf(". SAUNA: FIFO 'rejeitados' opened in WRITEONLY mode\n");

        return;
}


int main(int argc, char* argv[]) {

        gettimeofday(&begin, NULL); //guardar na struct a hora de inicio do programa

        //Tratamento de argumentos

        if (argc != 2) {
                printf(". SAUNA: The number of arguments of %s is not correct!", argv[0]);
                exit(1);
        }

        CAPACITY = atoi(argv[1]);


        //Criaçao do ficheiro de registo
        int pid;
        pid = getpid();
        char balPathname [20];
        sprintf (balPathname, "/tmp/bal.%d", pid);
        balFile=fopen(balPathname, "w");

        if(balFile == NULL)
                printf(". SAUNA: Error opening balFile\n");

        //Abertura FIFO rejeitados
        openEntradaFifo();
        //Criação e abertura de FIFO de entrada
        makeOpenRejeitadosFifo();

        //Ler numero de pedidos
        read(ENTRADA_FIFO_FD, &REQUESTS_TO_READ, sizeof(int));

        //Receber Pedidos
        requestsReceptor();

        int k;
        for(k=0; k < 255; k++) {
                pthread_join(threadsTid[k], NULL); //espera pelas threads que estão a correr
        }

        sleep(1);
        printStats();
        fclose(balFile);
        unlink("/tmp/rejeitados");

        exit(0);
}
