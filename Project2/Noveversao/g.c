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

int NUM_REQUESTS;
int MAX_TIME;

int ENTRADA_FIFO_FD;
int REJEITADOS_FIFO_FD;

typedef struct {
        int id; //numero do pedido
        char gender; //genero
        int duration; //duracao
        int denials; //numero de rejeicoes
} Request;



void *escutarPedidosRejeitados(void *arg) {

		Request* request = malloc(sizeof(Request));

    while(read(REJEITADOS_FIFO_FD, request, sizeof(Request)) != 0) {
			if(request->id!=0){
				if(request->id==-1) break;
				printf(" > GERADOR (rejeitado): P:%i-G:%c-T:%i-D:%i;\n", request->id, request->gender, request->duration, request->denials);
				if(request->denials<3) write(ENTRADA_FIFO_FD, request, sizeof(Request));
			}
    }
    pthread_exit(NULL);

}


void *requestGenerator(void *arg) {

	//criacao de pedidos

	int i;
	for (i = 1; i <= NUM_REQUESTS; i++) {
		Request *request = malloc(sizeof(Request));
		request->id = i;
		request->gender = (rand() % 2) ? 'M' : 'F';
		request->duration = rand() % MAX_TIME + 1;
		request->denials = 0;

		write(ENTRADA_FIFO_FD, request, sizeof(Request));
		printf(" > GERADOR (pedido):P:%i-G:%c-T:%i-D:%i;\n", request->id, request->gender, request->duration, request->denials);
	}

	pthread_exit(NULL);
}

void openRejeitadosFifo() {

	while ((REJEITADOS_FIFO_FD = open("/tmp/rejeitados", O_RDONLY)) == -1) {
		if (errno == EEXIST)
			printf(" > GERADOR: FIFO 'rejeitados' doesnt exist! Retrying...\n");
	}

	printf(" > GERADOR: FIFO 'rejeitados' openned in READONLY mode\n");

	return;

}

void makeOpenEntradaFifo() {
	//criacao
	if (mkfifo("/tmp/entrada", S_IRUSR | S_IWUSR) != 0) {
		if (errno == EEXIST)
			printf(" > GERADOR: FIFO '/tmp/entrada' already exists\n");
		else
			printf("> GERADOR: Can't create FIFO '/tmp/entrada'\n");
	}
	else printf(" > GERADOR: FIFO 'entrada' created.\n");

	//abertura
	while ((ENTRADA_FIFO_FD = open("/tmp/entrada", O_WRONLY | O_NONBLOCK)) == -1) {
		printf(" > GERADOR: Waiting for SAUNA to open 'entrada'...\n");
	}
	printf(" > GERADOR: FIFO 'entrada' opened in WRITEONLY mode\n");

	return;
}


int main(int argc, char* argv[]) {

	//Para gerar numeros aleatorios ao longo do programa
	srand(time(NULL));

	//Tratamento de argumentos
	if (argc != 3) {
		printf(" > GERADOR: The number of arguments of %s is not correct!", argv[0]);
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
		printf(" > GERADOR: Error opening gerFile\n");

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

	unlink("/tmp/entrada");

	return 0;
}
