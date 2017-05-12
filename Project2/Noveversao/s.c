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
clock_t beginClock;

int CAPACITY;

char ALLOWED_GENDER = 'N';
int REQUESTS_TO_READ;
int NUM_PEOPLE_IN =0;

int ENTRADA_FIFO_FD;
int REJEITADOS_FIFO_FD;

int id;

pthread_t *threadsTid;
int threadPos = 0;

typedef struct {
        int id; //numero do pedido
        char gender; //genero
        int duration; //duracao
        int denials; //numero de rejeicoes
} Request;

void printFile(Request *request, char* tip){

  clock_t  instClock = clock();
  double inst= (double)(instClock - beginClock) / (CLOCKS_PER_SEC/1000);
  fprintf(balFile, "%-6.2f - %-4d - %-4d - %-4d: %-1c - %-4d - %-10s\n", inst, getpid(), getpid() ,request->id,request->gender, request->duration, tip);

}


void manageRejected(Request* request){

    request->denials = request->denials + 1;

    if(request->denials < 3)
      REQUESTS_TO_READ++;

    printf(" > SAUNA (rejeitado): P:%i-G:%c-T:%i-D:%i;\n", request->id, request->gender, request->duration, request->denials);
    write(REJEITADOS_FIFO_FD, request, sizeof(Request));
}


void *stayingInSauna(void *time) {
    threadPos--;
    //int pos= threadPos;
    int idlocal=id;
    int n = *(int *)time;
    printf(" > SAUNA: %d entrou\n",idlocal);
    sleep(n/1000);
    NUM_PEOPLE_IN--;

    printf(" > SAUNA: %d saiu\n",idlocal);

    printf(" > SAUNA PEOPLE IN SAUNA: %d\n", NUM_PEOPLE_IN);

    if(NUM_PEOPLE_IN==0){
      ALLOWED_GENDER = 'N';
      printf(" > SAUNA: allowed gender: %c\n",ALLOWED_GENDER);
    }

    pthread_exit(NULL);
}

int validateRequest(Request *request) {

        if(request->gender != ALLOWED_GENDER)
                return 0;
        else if (NUM_PEOPLE_IN >= CAPACITY)
                return 0;
        else return 1;
}


void manageRequest(Request* request){


            if(ALLOWED_GENDER=='N') {
                      ALLOWED_GENDER = request->gender;
                      printf(" > SAUNA: allowed gender: %c\n",ALLOWED_GENDER);
                      printf(" > SAUNA (servido): P:%i-G:%c-T:%i-D:%i;\n", request->id, request->gender, request->duration, request->denials);
                      NUM_PEOPLE_IN++;
                      id=request->id;
                      printFile(request, "SERVIDO");
                      pthread_create(&threadsTid[threadPos], NULL, stayingInSauna,&request->duration);
                      threadPos++;
            } else {
                      if(validateRequest(request) != 0) {
                          printf(" > SAUNA (servido): P:%i-G:%c-T:%i-D:%i;\n", request->id, request->gender, request->duration, request->denials);
                          NUM_PEOPLE_IN++;
                          id=request->id;
                          printFile(request, "SERVIDO");
                          pthread_create(&threadsTid[threadPos], NULL, stayingInSauna,&request->duration);
                          threadPos++;
                      } else {
                          printFile(request, "REJEITADO");
                          manageRejected(request);

                      }
              }
  return;
}

void requestsReceptor() {
        Request* request;
        int n;
        while(REQUESTS_TO_READ != 0) {
                request = malloc(sizeof(Request));
                n=read(ENTRADA_FIFO_FD, request, sizeof(Request));
                if(n>0){
                  printf(" > SAUNA REQUESTS TO READ: %d\n", REQUESTS_TO_READ);
                  printFile(request, "RECEBIDO");
                  manageRequest(request);
                  REQUESTS_TO_READ--;
                }
        }
        if(REQUESTS_TO_READ==0){
          Request* request =malloc(sizeof(Request));
          request->id=-1;
          write(REJEITADOS_FIFO_FD,  request, sizeof(Request));
          close(REJEITADOS_FIFO_FD);
          close(ENTRADA_FIFO_FD);
        }
        return;
}

void openEntradaFifo() {

	while ((ENTRADA_FIFO_FD = open("/tmp/entrada", O_RDONLY)) == -1) {
		if (errno == EEXIST)
			printf(" > SAUNA: FIFO 'entrada' doesnt exist! Retrying...\n");
	}

	printf(" > SAUNA: FIFO 'entrada' openned in READONLY mode\n");

  return;
}

void makeOpenRejeitadosFifo() {
	//criacao
	if (mkfifo("/tmp/rejeitados", S_IRUSR | S_IWUSR) != 0) {
		if (errno == EEXIST)
			printf(" > SAUNA: FIFO '/tmp/rejeitados' already exists\n");
		else
			printf("> SAUNA: Can't create FIFO '/tmp/rejeitados'\n");
	}
	else printf(" > SAUNA: FIFO 'rejeitados' created.\n");

	//abertura
	while ((REJEITADOS_FIFO_FD = open("/tmp/rejeitados", O_WRONLY | O_NONBLOCK)) == -1) {
		printf(" > SAUNA: Waiting for SAUNA to open 'rejeitados'...\n");
	}
	printf(" > SAUNA: FIFO 'rejeitados' opened in WRITEONLY mode\n");

  return;
}


int main(int argc, char* argv[]) {

        beginClock=clock();

        //Tratamento de argumentos

        if (argc != 2) {
                printf(" > SAUNA: The number of arguments of %s is not correct!", argv[0]);
                exit(1);
        }

        CAPACITY = atoi(argv[1]);

        threadsTid = malloc(CAPACITY * sizeof(pthread_t));

        //Criaçao do ficheiro de registo
        int pid;
        pid = getpid();
        char balPathname [20];
        sprintf (balPathname, "/tmp/bal.%d", pid);
        balFile=fopen(balPathname, "w");

        if(balFile == NULL)
          printf(" > SAUNA: Error opening balFile\n");

        //Abertura FIFO rejeitados
        openEntradaFifo();
        //Criação e abertura de FIFO de entrada
        makeOpenRejeitadosFifo();

        //Ler numero de pedidos
        read(ENTRADA_FIFO_FD, &REQUESTS_TO_READ, sizeof(int));

        //Receber Pedidos
        requestsReceptor();

        int k;
        for(k=0; k <= threadPos; k++){
          pthread_join(threadsTid[threadPos], NULL);
        }

        fclose(balFile);
        unlink("/tmp/rejeitados");
        exit(0);
}
