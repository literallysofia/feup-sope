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

int nPedidos; //numero de pedidos
int maxUtil; //maxima utilizaçao em milisegundos

struct Pedido{
	int p; //numero
	char g; //género
	int t; //duraçao
};

void *gerarPedidos(void *arg)
{

	//Gerar pedidos
	struct Pedido pedidos[nPedidos]; //Array de Pedidos

	int i;
	for(i = 1 ; i <= nPedidos; i++){
		struct Pedido pedido;

		//numero
		pedido.p=i;

		//genero
		int g = rand() % 2;
		if(g==0) pedido.g='M';
		if(g==1) pedido.g='F';

		//random duraçao
		int t = rand() % maxUtil+1;
		pedido.t=t;

		//printf("P: %i;G: %c;T: %i;\n", pedido.p, pedido.g, pedido.t); //TODO: apagar

		pedidos[i-1]=pedido;
	}

	///FIFO///

	//Criar FIFO de entrada
	if (mkfifo("/tmp/entrada",0660)<0){
		if (errno==EEXIST)
			printf(" > GERADOR: FIFO '/tmp/entrada' already exists\n");
		else printf("> GERADOR: Can't create FIFO '/tmp/entrada'\n");
	}
	else printf(" > GERADOR: FIFO created.\n");

	//Abrir FIFO de entrada
	int fd_entrada;

	fd_entrada=open("/tmp/entrada",O_WRONLY);
	if (fd_entrada == -1) {
		printf("> GERADOR: Oops !!! Server is closed !!!\n"); //TODO: mudar mensagem
		exit(1);
	}

	printf(" > GERADOR: FIFO 'entrada' openned in WRITEONLY mode\n");

	//Escrever no FIFO

	int j;
	for(j=0; j < nPedidos; j++){
		int n;
		char pedidoString[255];

		if(pedidos[j].p<10&&pedidos[j].t<10){
			n=sprintf(pedidoString, "P:0%i-G:%c-T:0%i;", pedidos[j].p, pedidos[j].g, pedidos[j].t);
		}
		else if(pedidos[j].p<10){
			n=sprintf(pedidoString, "P:0%i-G:%c-T:%i;", pedidos[j].p, pedidos[j].g, pedidos[j].t);
		}
		else if(pedidos[j].t<10){
			n=sprintf(pedidoString, "P:%i-G:%c-T:0%i;", pedidos[j].p, pedidos[j].g, pedidos[j].t);
		}

		//n=sprintf(pedidoString, "P:%i-G:%c-T:%i;\n", pedidos[j].p, pedidos[j].g, pedidos[j].t);
		write(fd_entrada,pedidoString,n);
	}

	//Fechar FIFO de entrada
	close(fd_entrada);

	/*if (unlink("/tmp/entrada")<0)
    printf(" > GERADOR: Error when destroying FIFO '/tmp/entrada'\n");
  else
    printf(" > GERADOR: FIFO '/tmp/entrada' has been destroyed\n");*/
 	return NULL;
}

int main(int argc, char* argv[]) {

	//Tratamento de argumentos
	if (argc != 3) {
		 printf(" > GERADOR: The number of arguments of %s is not correct!", argv[0]);
		 exit(1);
	 }

	nPedidos = atoi(argv[1]);
	maxUtil = atoi (argv[2]);

	//Para gerar numeros aleatorios ao longo do programa
	srand(time(NULL));

	//Thread que gera pedidos aleatorios
	pthread_t tid1;
	pthread_create(&tid1, NULL, gerarPedidos, NULL);

	pthread_exit(NULL);
	return 0;
}
