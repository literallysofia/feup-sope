#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>



#include "auxiliar.h"

pthread_mutex_t mutex_f = PTHREAD_MUTEX_INITIALIZER;
int spaces,totalSpaces;
int stayOpen;
clock_t bigBang;

void print_usage(){
	printf("Usage: parque <N Lugares> <T Abertura>\n");
}

void mySleep(double ticks) {
	float ticksPS  = sysconf(_SC_CLK_TCK);
    double seconds = (double)ticks / ticksPS;
    struct timespec req;
    req.tv_sec = (time_t) (seconds);
    req.tv_nsec = (long) (seconds * pow(10,9) - req.tv_sec * pow(10,9));
    nanosleep(&req, NULL);
}

void writeLog(int idV,int event) {
    FILE *logFile;
    if ((logFile = fopen("parque.log", "a")) == NULL){
    	perror("[writelog][open logfile]");
    	exit(1);
    }
    clock_t now = clock();
    fprintf(logFile, "%8d ; ", (int) now);
    fprintf(logFile, "%4d ; ", totalSpaces - spaces);
    fprintf(logFile, "%7d ; ", idV);
    switch(event) {

	   case 0  ://estacionamento
	      fprintf(logFile, "%s\n", "estacionamento");
	      break;	
	   case 1 ://saida
	      fprintf(logFile, "%s\n", "saída");
	      break;
	   case 2 ://cheio
	      fprintf(logFile, "%s\n", "cheio");
	      break;
	   case 3 ://fechado
	      fprintf(logFile, "%s\n", "saída/encerrado");
	      break;
	}
    fclose(logFile);

}


void * t_parking(void * arg){
	pthread_detach(pthread_self());
	Vehicle toPark = *(Vehicle* ) arg;

	printf("Trabalho...\n");

	int status; //0 - closed , 1 - full , 2 - parked
	int fd;
	
	if((fd = open(toPark.vFifo,O_WRONLY)) < 0){
		perror("[t_parking][open]");
		exit(1);
	}
	
	pthread_mutex_lock(&mutex_f);
	if(spaces > 0){
		spaces--;
		status = 2; //parks vehicle
		writeLog(toPark.id,0);
 	}
	else{
		status = 1; //park is full but not closed
		writeLog(toPark.id,2);
	}
	pthread_mutex_unlock(&mutex_f);

	if(write(fd,&status,sizeof(int)) < 0){
		perror("[t_parking][write]");
		exit(1);
	}

	if(status == 2){
		mySleep(toPark.parked_time); //vehicle remains parked the indicated amount of ticks
		if(write(fd,&status,sizeof(int)) < 0){
			perror("[t_parking][write_saida]");
			exit(1);
		}
		pthread_mutex_lock(&mutex_f);
		spaces++;
		if(stayOpen !=0)
			writeLog(toPark.id,1);
		else
			writeLog(toPark.id,3);
		pthread_mutex_unlock(&mutex_f);
	}

	pthread_exit(NULL);
}

void * t_access(void * arg){
	char * name = (char* ) arg;

	int fd;
	if((fd = open(name,O_RDONLY)) < 0){
		perror("[t_access]");
		exit(1);
	}
	
	Vehicle * invader = malloc(sizeof(Vehicle));
	while(read(fd,invader,sizeof(Vehicle)) > 0){
		if(invader->id >= 0){
			pthread_t tid;
			pthread_create(&tid,NULL,t_parking,invader);
		}
		else
			break;
	}
	if(close(fd) < 0){
		perror("[t_access][closeFD]");
		exit(1);
	}

	pthread_exit(NULL);
}

int main(int argc, char *argv[]){

	if (argc != 3){
		print_usage();
		return 1;
	}
	printf("ABRIU\n");
	spaces = atoi(argv[1]);
	totalSpaces = spaces;
	stayOpen = atoi(argv[2]);

	FILE *logFile;
	unlink("parque.log");
    if ((logFile = fopen("parque.log", "w")) == NULL){
        perror("[main][open logfile]");
        exit(1);
    }
    fprintf(logFile, "t(ticks) ; nlug ; id_viat ; observ\n");
    fclose(logFile);


	int i;
	for(i = 0;i < 4;i++){
		unlink(fifoNames[i]);
		mkfifo(fifoNames[i],0600);
	}

	pthread_t tidN,tidS,tidE,tidO;
	pthread_create(&tidN,NULL,t_access,fifoNames[0]);
	pthread_create(&tidS,NULL,t_access,fifoNames[1]);
	pthread_create(&tidE,NULL,t_access,fifoNames[2]);
	pthread_create(&tidO,NULL,t_access,fifoNames[3]);

	int fd[4];
	for(i = 0 ; i < 4 ; i++){
		if((fd[i] = open(fifoNames[i],O_WRONLY)) < 0){
			perror("[main] opening access fifos");
			exit(1);
		}	
	}

	sleep(stayOpen);
	stayOpen = 0;

	Vehicle * lastCar = malloc(sizeof(Vehicle));
	lastCar->id = -1;
	pthread_mutex_lock(&mutex_f);
	for(i = 0 ; i < 4 ; i++){
		if(write(fd[i],lastCar,sizeof(Vehicle)) < 0){
				perror("[main][write_lastcar]");
				exit(1);
			}
	}
	pthread_mutex_unlock(&mutex_f);

	printf("FECHOU\n");

	pthread_exit(NULL);
}
