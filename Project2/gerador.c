#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

int nPedidos;
int maxUtil;

struct Pedido{
	int p; //numero
	char g; //género
	int t; //duraçao
};

void *gerarPedidos(void *arg)
{
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

		//TODO: mudar prinf para enviar por fifo
		printf("P: %i - G: %c - T: %i\n", pedido.p, pedido.g, pedido.t);
	}

 	return NULL;
}

int main(int argc, char* argv[]) {

	//Tratamento de argumentos
	if (argc != 3) { printf(" > ERROR: The number of arguments is not correct!"); exit(1); }

	nPedidos = atoi(argv[1]);
	maxUtil = atoi (argv[2]);

	//Para gerar numeros aleatorios ao longo do programa
	srand(time(NULL));

	//Thread que gera pedidos aleatorios
	pthread_t tid1;
	pthread_create(&tid1, NULL, gerarPedidos, NULL);

	pthread_exit(NULL);
}
