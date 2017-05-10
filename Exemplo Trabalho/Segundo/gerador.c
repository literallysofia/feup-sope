#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>


#include "auxiliar.h"

float ticksPS;
double timeUnitTick;
char * genericFIFO = "/tmp/fifo";
pthread_mutex_t mutex_f = PTHREAD_MUTEX_INITIALIZER;
clock_t bigBang;



void print_usage(){
		printf("Usage: gerador <T_GER> <U_REL>\n");
}


void mySleep(double ticks) {                                                                  
    double seconds = (double)ticks / ticksPS;
    struct timespec req;
    req.tv_sec = (time_t) (seconds);
    req.tv_nsec = (long) (seconds * pow(10,9) - req.tv_sec * pow(10,9));
    nanosleep(&req, NULL);
}

void writeLog(gLog logLine,clock_t birth, int event) {
    FILE *log_file;
    if ((log_file = fopen("gerador.log", "a")) == NULL){
    	perror("[writelog][open logfile]");
    	exit(1);
    }
    clock_t now = clock();
    fprintf(log_file, "%8d ; ", (int) (now - bigBang));
    fprintf(log_file, "%7d ;", logLine.id);
    fprintf(log_file, "    %c   ; ", logLine.dest);
    fprintf(log_file, "%10d ; ", (int)logLine.parkedT);
    switch(event) {

	   case 0  ://entrada
	      fprintf(log_file, "     ? ; ");
	      fprintf(log_file, "%s\n", "entrada");
	      break;	
	   case 1 ://cheio
	   	  fprintf(log_file, "     ? ; ");
	      fprintf(log_file, "%s\n", "cheio!");
	      break;
	   case 2 ://saida
	   	  fprintf(log_file, "%6d ; ", (int) now);
	      fprintf(log_file, "%s\n", "saída");
	      break;
	   case 3 ://fechado
	   	  fprintf(log_file, "     ? ; ");
	      fprintf(log_file, "%s\n", "      ");
	      break;
	}
    fclose(log_file);
}



void * t_vehicle(void * arg){
	pthread_detach(pthread_self());
	int vid = *(int* ) arg;
	free (arg);

	Vehicle * esteCarrinho = malloc(sizeof(Vehicle));

	printf("Viatura: %d\n",vid);
	
	clock_t birth = clock();
	//Escolhe tempo de permanencia
	int r = (rand() % 10) + 1;
	double tTicks = r * timeUnitTick;

	//Escolhe entrada
	r = (rand() % 4);


	snprintf(esteCarrinho->vFifo,19,"%s%d", genericFIFO ,vid);

	unlink(esteCarrinho->vFifo);
	if(mkfifo(esteCarrinho->vFifo,S_IRWXU) != 0){
		perror("[t_vehicle][mkfifo]");
		exit(1);
	}

	
	esteCarrinho->id = vid;
	esteCarrinho->parked_time = tTicks;
	esteCarrinho->entryFifo = fifoNames[r];

	gLog logLine;

	logLine.id = vid;
	logLine.dest = fifoNames[r][9];
	logLine.parkedT = tTicks;


	int fd;
	int parkStatus; //0 - closed , 1 - full , 2 - parked
	pthread_mutex_lock(&mutex_f);
	fd = open(esteCarrinho->entryFifo,O_WRONLY|O_NONBLOCK);
	if(fd < 0)
		parkStatus = 0;
	else{
		if(write(fd,esteCarrinho,sizeof(Vehicle)) < 0){
			perror("[t_vehicle][write_entryFifo]");
			exit(1);
		}
		if(close(fd) < 0){
			perror("[t_vehicle][close_entryFifo]");
			exit(1);
		}

		if((fd = open(esteCarrinho->vFifo,O_RDONLY)) < 0){
			perror("[t_vehicle][open_VehicleFifo]");
			exit(1);
		}
		if(read(fd,&parkStatus,sizeof(int)) < 0){
			perror("[t_vehicle][read_VehicleFifo]");
			exit(1);
		}
	}
	pthread_mutex_unlock(&mutex_f);

	if(parkStatus == 2){ //carro estacionado
		writeLog(logLine,birth,0);
		//[LOG] entrou!!!
		if(read(fd,&parkStatus,sizeof(int)) < 0){
			perror("[t_vehicle][read_VehicleFifo - saida]");
			exit(1);
		}
		writeLog(logLine,birth,2);
	}
	else
		if(parkStatus == 0){
			//[LOG] closed
			writeLog(logLine,birth,3);
		}
		else if(parkStatus == 1){
			//[LOG] full
			writeLog(logLine,birth,1);
		}

	pthread_exit(NULL);

}

int main(int argc, char *argv[]){
	bigBang = clock();
	if (argc != 3){
		print_usage();
		return 1;
	}
	printf("A GERAR\n");
	FILE *logFile;
	unlink("gerador.log");
    if ((logFile = fopen("gerador.log", "w")) == NULL){
        perror("[main][open logfile]");
        exit(1);
    }
    fprintf(logFile, "t(ticks) ; id_viat ; destin ; t_estacion ; t_vida ; observ\n"); 					                // first row in the gerador.log file
    fclose(logFile);

	srand(time(NULL));

	ticksPS  = sysconf(_SC_CLK_TCK);

	timeUnitTick = (double)atoi(argv[2]);
	double timeUnitSec = timeUnitTick/ticksPS;
	double duration = (double)atoi(argv[1]);
	double t = 0.0;
	int id = 0;
	int ru = 0;

	while(t < duration){
		ru = rand() % 10;
		if(ru < 3){
			mySleep(timeUnitTick);
			t += timeUnitSec;
		}
		else if(ru < 5){
			mySleep(2 * timeUnitTick);
			t += 2 * timeUnitSec;
		}


		int *nr = (int*)malloc(sizeof(int));
		*nr = id++;

		pthread_t tid;
		pthread_create(&tid,NULL,t_vehicle,nr);

	}
	pthread_exit(NULL);

}
