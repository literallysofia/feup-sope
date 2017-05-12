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
#include <pthread.h>
#include <time.h>

FILE* balFile;

int CAPACITY; //numero de lugares na sauna
int NUM_REQUESTS; //numero de pedidos
int VALID_REQUESTS; //numero de pedidos validos
int REJECTED_REQUESTS; //numero de pedidos rejeitados
char* GENERATE_FIFO = "/tmp/entrada";
char* REJECT_FIFO =  "/tmp/rejeitados";
clock_t beginClock;
pthread_t *threadsTid;
int threadFree = 0;
//int count[CAPACITY];

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

void printStats() { //TODO: change

        int i;
        for(i = 0; i < VALID_REQUESTS; i++)
                printf(" > SAUNA: P:%i-G:%c-T:%i;\n", validRequests[i]->id, validRequests[i]->gender, validRequests[i]->duration);

}

void printFile(Request *request, char* tip){

  clock_t  instClock = clock();

  double inst= (double)(instClock - beginClock) / (CLOCKS_PER_SEC/1000);

  fprintf(balFile, "%-6.2f - %-4d - %-4d - %-4d: %-1c - %-4d - %-10s\n", inst, getpid(), getpid() ,request->id,request->gender, request->duration, tip);

}

int validateRequest(Request *request, int j) {

        if(request->gender != ALLOWED_GENDER)
                return 0;
        else if ((j+1) > CAPACITY)
                return 0;
        else return 1;
}

void *stayingInSauna(void *time) {
    int n = *(int *)time;
    printf(" > SAUNA: thread %lu time - %d\n",pthread_self(), n);
    sleep(n/1000);
    threadFree--;
    pthread_exit(NULL);

}

void manageRequests() {

        int i, j = 0, k = 0;

        char* tip="";

        for(i = 0; i < NUM_REQUESTS; i++) {

              tip="RECEBIDO";
              printFile(requestList[i], tip);

                  if(validRequests[0] == NULL) {
                            ALLOWED_GENDER = requestList[i]->gender;
                            validRequests[j] = requestList[i];
                            printf(" > SAUNA (servido): P:%i-G:%c-T:%i-D:%i;\n", requestList[i]->id, requestList[i]->gender, requestList[i]->duration, requestList[i]->denials);
                            tip="SERVIDO";
                            printFile(requestList[i], tip);
                            pthread_create(&threadsTid[threadFree], NULL, stayingInSauna,&requestList[i]->duration);
                            threadFree++;
                            j++;
                  } else {

                            if(validateRequest(requestList[i], j) != 0) {
                                    validRequests[j] = requestList[i];
                                    printf(" > SAUNA (servido): P:%i-G:%c-T:%i-D:%i;\n", requestList[i]->id, requestList[i]->gender, requestList[i]->duration, requestList[i]->denials);
                                    tip="SERVIDO";
                                    printFile(requestList[i], tip);
                                    pthread_create(&threadsTid[threadFree], NULL, stayingInSauna,&requestList[i]->duration);
                                    threadFree--;
                                    j++;
                            } else {
                                    requestList[i]->denials = requestList[i]->denials + 1;
                                    rejectedRequests[k] = requestList[i];
                                    tip="REJEITADO";
                                    printFile(requestList[i], tip);
                                    k++;
                            }
                    }
        }


        VALID_REQUESTS = j;
        REJECTED_REQUESTS = k;

        return;
}
//void requestReceptor();

void rejectedManager(){

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

  if ((fd_reject = open(REJECT_FIFO,O_WRONLY  | O_NONBLOCK)) == -1) {
          printf(" > SAUNA: Could not open fifo!\n"); //TODO: mudar mensagem
          exit(1);
  }

  printf(" > SAUNA: FIFO 'rejeitados' openned in WRITE mode\n");

  //escrita no FIFO

  int j;
  for(j = 0; j < REJECTED_REQUESTS; j++)
          write(fd_reject, rejectedRequests[j], sizeof(Request));

  //fecha FIFO de rejeitados

  close(fd_reject);

  //requestReceptor();

}

//void *requestReceptor(void *arg) {
void requestReceptor() {

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
                //printf(" > SAUNA: P:%i-G:%c-T:%i;\n", requestList[i]->id, requestList[i]->gender, requestList[i]->duration);
                i++;
                request = malloc(sizeof(Request));
        }

        NUM_REQUESTS = i;

        //fecha FIFO de entrada

        close(fd_generator);

        manageRequests();

        rejectedManager();


}

int main(int argc, char* argv[]) {

        //Começo do clock
        beginClock = clock();

        //Tratamento de argumentos

        if (argc != 2) {
                printf(" > SAUNA: The number of arguments of %s is not correct!", argv[0]);
                exit(1);
        }

        CAPACITY = atoi(argv[1]);

        threadsTid = malloc(CAPACITY * sizeof(pthread_t));

        NUM_REQUESTS = 0;

        //Criaçao do ficheiro de registo
        int pid;
        pid = getpid();
        char balPathname [20];
        sprintf (balPathname, "/tmp/bal.%d", pid);
        balFile=fopen(balPathname, "w");

        if(balFile == NULL)
          printf(" > SAUNA: Error opening balFile\n");


        requestReceptor();

        int k;
        for(k=0; k < CAPACITY; k++){
          pthread_join(threadsTid[k], NULL);
          printf(" > SAUNA: thread %lu FINISHED\n",threadsTid[k]);
        }

        fclose(balFile);
        unlink(REJECT_FIFO);
        exit(0);
}
