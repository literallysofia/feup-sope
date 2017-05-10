#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <linux/limits.h>
#include <string.h>


#define LINE_SIZE (NAME_MAX+PATH_MAX+10+4+14+1+9)

typedef int bool;
#define true 1
#define false 0

bool sameContent(char * path1, char * path2){
	char buffer1[513];
	char buffer2[513];

	int fd1;
	int fd2;
	fd1 = open(path1,O_RDONLY);
	fd2 = open(path2,O_RDONLY);

	struct stat stat1;
	struct stat stat2;
	fstat(fd1, &stat1);
	fstat(fd2, &stat2);
	
	/*Se f1 e f2 forem harlinks para o mesmo ficheiro, o mais recente
	não será apagado para voltar a ser criado.
	Apesar do conteúdo ser tecnicamente o mesmo.*/
	if(stat1.st_ino == stat2.st_ino){
		close(fd1);
		close(fd2);
		return false;
	}

	FILE * file1 = fdopen(fd1,"r");
	FILE * file2 = fdopen(fd2,"r");

	while(fgets(buffer1,512,file1) != NULL && fgets(buffer2,512,file2) != NULL){
		if(strcmp(buffer1,buffer2)!=0){
			close(fd1);
			close(fd2);
			return false;
		}
	}

	close(fd1);
	close(fd2);
	return true;


	}



int main(int argc, char *argv[]){

	char lstdir_path[PATH_MAX];
	char * last;
	lstdir_path[0]=0;
	if((last = strrchr(argv[0],'/')) != NULL)
		if(argv[0][0]=='/')
			strncat(lstdir_path,argv[0],last-argv[0]);
		else{
			getcwd(lstdir_path,PATH_MAX);
			strcat(lstdir_path,"/");
			strncat(lstdir_path,argv[0],last-argv[0]);
		}
	else
		getcwd(lstdir_path,PATH_MAX);
	strcat(lstdir_path,"/lstdir");

	char folder_path[PATH_MAX];
	strcpy(folder_path,argv[1]);
	if(folder_path[strlen(folder_path)-1] != '/')
		sprintf(folder_path,"%s/",folder_path);
	
	remove("/tmp/rmdup_files.txt"); //limpa ficheiro aux existente

	int file_list;
	if((file_list = open("/tmp/rmdup_files.txt",O_RDWR | O_CREAT,0666)) < 0){
		perror("[rmdup]");
		exit(1);
	}
	
	if(fork() == 0){
		if(access(folder_path,R_OK) == 0)
			execl(lstdir_path,lstdir_path,folder_path,NULL);
		else
			exit(1);
	}

	int status; //não está a ser util, para já
	if(wait(&status) < 0){
		perror("[rmdup/wait]");
		exit(1);
	}


	if(fork() == 0){	
		if(execlp("sort","sort","-d","-o","/tmp/rmdup_files.txt","/tmp/rmdup_files.txt",NULL) < 0){
			perror("[rmdup/sort]");
			exit(1);
		}
	}

	if(wait(&status) < 0){
		perror("[rmdup/wait]");
		exit(1);
	}

	//Processar o ficheiro
	int hardlink_list;
	sprintf(folder_path,"%shlinks.txt",folder_path);

	remove(folder_path); //limpa ficheiro aux existente
	if((hardlink_list = open(folder_path,O_RDWR | O_CREAT,0666)) < 0){
		perror("[rmdup]");
		exit(1);
	}
	

	FILE * sorted_list = fdopen(file_list,"r");
	char fileOriginal[NAME_MAX+1+10+4+2];//10-tamanho 4-permissões 2-simbolos separadores
	char file2[NAME_MAX+1+10+4+2];
	char fileOriginal_path[PATH_MAX];
	char file2_path[PATH_MAX];
	char file2Old_path[NAME_MAX];
	char date[14+5];//5-separadores da data /esta a ser descartada
	
	fscanf(sorted_list, "%s %s %s",fileOriginal,date,fileOriginal_path);
	do{
		if(fscanf(sorted_list, "%s %s %s",file2,date,file2_path) == EOF)
			break;

		if((access(file2_path,W_OK) == 0) && !strcmp(fileOriginal,file2) && sameContent(fileOriginal_path,file2_path)){
			sprintf(file2Old_path,"%s-old",file2_path);
			rename(file2_path,file2Old_path);
			if(link(fileOriginal_path, file2_path) <0){
				rename(file2Old_path,file2_path);
				perror("[link]");
				exit(1);
			}
			if(remove(file2Old_path) <0){
				perror("[remove]");
				exit(1);
			}
			sprintf(file2_path,"%s\n",file2_path);
			if(write(hardlink_list,file2_path,strlen(file2_path)) < 0){
				perror("[lstdir]");
				exit(1);	
			}
		}
		else if(strcmp(fileOriginal,file2)){
				strcpy(fileOriginal,file2);
				strcpy(fileOriginal_path,file2_path);
				}

		}while(1);
		


	exit(0);
}