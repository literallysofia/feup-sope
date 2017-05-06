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
	//Abrir FIFO de entrada
	int fd;

	fd=open("/tmp/entrada",O_WRONLY);
	if (fd == -1) {
		printf("> ERROR: Oops !!! Server is closed !!!\n"); //TODO: mudar mensagem
		exit(1);
	}

	//Gerar pedidos
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

		//Escrever no FIFO
		int n;
		char pedidoString[255];
		n=sprintf(pedidoString, "P: %i;G: %c;T: %i;", pedido.p, pedido.g, pedido.t);
		write(fd,pedidoString,n);
	}

	//Fechar FIFO de entrada
	close(fd);

 	return NULL;
}

int main(int argc, char* argv[]) {

	unlink("tmp/entrada"); //TODO: apagar

	//Tratamento de argumentos
	if (argc != 3) {
		 printf(" > ERROR: The number of arguments of %s is not correct!", argv[0]);
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
}
