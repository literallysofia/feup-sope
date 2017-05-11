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

FILE* gerFile;

int NUM_REQUESTS; //numero de pedidos
int MAX_TIME; //maxima utilizaçao em milisegundos
char* GENERATE_FIFO = "/tmp/entrada";
char* REJECT_FIFO =  "/tmp/rejeitados";
clock_t beginClock;

typedef struct {
        int id; //numero do pedido
        char gender; //genero
        int duration; //duracao
        int denials; //numero de rejeicoes
} Request;

Request* requestList[256]; //array de pedidos

void printFile(Request *request, char* tip){

  clock_t  instClock = clock();

  double inst= (double)(instClock - beginClock) / (CLOCKS_PER_SEC/1000);

  fprintf(gerFile, "%-6.2f - %-4d - %-4d: %-1c - %-4d - %-10s\n", inst, getpid() ,request->id,request->gender, request->duration, tip);

}


void *requestGenerator(void *arg) {

        //criacao de pedidos

        int i;
        for(i = 1; i <= NUM_REQUESTS; i++) {

                Request *request = malloc(sizeof(Request));
                request->id = i;
                request->gender = (rand() % 2) ? 'M' : 'F';
                request->duration = rand() % MAX_TIME + 1;
                request->denials = 0;

                requestList[i - 1] = request;
        }

        //FIFO

        //criacao FIFO de entrada

        if (mkfifo(GENERATE_FIFO, S_IRUSR | S_IWUSR) != 0) {
                if (errno == EEXIST)
                        printf(" > GERADOR: FIFO '/tmp/entrada' already exists\n");
                else
                        printf("> GERADOR: Can't create FIFO '/tmp/entrada'\n");
        }
        else
                printf(" > GERADOR: FIFO 'entrada' created.\n");

        //abertura FIFO de entrada

        int fd_generator;

        while ((fd_generator = open(GENERATE_FIFO,O_WRONLY | O_NONBLOCK)) == -1) {
                printf(" > GERADOR: Waiting for SAUNA to open 'entrada'...\n");
        }

        printf(" > GERADOR: FIFO 'entrada' opened in WRITEONLY mode\n");

        //escrita no FIFO
        char* tip="";

        int j;
        for(j = 0; j < NUM_REQUESTS; j++){
          write(fd_generator, requestList[j], sizeof(Request));
          tip = "PEDIDO";
          printFile(requestList[j], tip);
        }


        //fecha FIFO de entrada

        close(fd_generator);

        pthread_exit(NULL);
}

int main(int argc, char* argv[]) {

        //Começo do clock
        beginClock = clock();

        //Tratamento de argumentos
        if (argc != 3) {
                printf(" > GERADOR: The number of arguments of %s is not correct!", argv[0]);
                exit(1);
        }

        NUM_REQUESTS = atoi(argv[1]);
        MAX_TIME = atoi (argv[2]);

        //Criaçao do ficheiro de registo
        int pid;
        pid = getpid();
        char gerPathname [20];
        sprintf (gerPathname, "/tmp/ger.%d", pid);
        gerFile=fopen(gerPathname, "w");

        if(gerFile == NULL)
          printf(" > GERADOR: Error opening gerFile\n");

        //Para gerar numeros aleatorios ao longo do programa
        srand(time(NULL));

        //Thread que gera pedidos aleatorios
        pthread_t tid1;
        pthread_create(&tid1, NULL, requestGenerator, NULL);

        //Thread que escuta os pedidos rejeitados
        //  pthread_t tid2;
        //pthread_create(&tid2, NULL, escutarPedidosRejeitados, NULL);
        pthread_join(tid1, NULL);
        unlink(GENERATE_FIFO);

        return 0;
}
