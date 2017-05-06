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

int nLugares; //numero de lugares na sauna

int main(int argc, char* argv[]) {

  unlink("tmp/entrada"); //TODO: apagar

  //Tratamento de argumentos
  if (argc != 2) {
    printf(" > ERROR: The number of arguments of %s is not correct!", argv[0]);
    exit(1);
  }
	nLugares = atoi(argv[1]);

  //---->FIFO<----
  //Criar FIFO entrada
  if (mkfifo("/tmp/entrada",0660)<0){
 		if (errno==EEXIST)
      printf(" > ERROR: FIFO '/tmp/entrada' already exists\n");
 		else printf("> ERROR: Can't create FIFO '/tmp/entrada'\n");
		exit(1);
	}

  //Abrir FIFO para leitura
  int fd_entrada;
  fd_entrada=open("/tmp/entrada",O_RDONLY);

  //Ler de FIFO
  int n;
  char str[255];

  do{
    n=read(fd_entrada,str,255);
    if (n>0) printf("%s has arrived\n",str);
  }while (strcmp(str, "SHUTDOWN") != 0); //TODO: mudar condiçao
//  }while (n>0 && *str++ != '\0');

  //Fechar FIFO
  close(fd_entrada);
  unlink("tmp/entrada");

  exit(0);
}
