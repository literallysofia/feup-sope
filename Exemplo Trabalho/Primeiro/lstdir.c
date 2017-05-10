#include <stdio.h>
#include <stdlib.h> 
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <linux/limits.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>

/*
LINE_MAX é o tamanho maximo que uma linha no ficheiro rmdup_files.txt pode ter.
1 - caracter extra de dirent->d_name
10 - tamanho do ficheiro
4 - permissões
14 - data
9 - caracteres que separam os parâmetros
*/
#define LINE_MAX (NAME_MAX+1+PATH_MAX+10+4+14+9)

int main(int argc, char *argv[]){

	int fd = open("/tmp/rmdup_files.txt", O_WRONLY | O_APPEND);
	if(fd < 0){
		perror("[lstdir][open rmdup_files.txt]");
		exit(1);
	}


	DIR * directory = opendir(argv[1]);
	if(directory == NULL){
		perror("[lstdir/opendir]");
		exit(1);
	}

	struct dirent * f_info;
	struct stat f_stat;
	struct tm * mod_date;
	char * dir_path = argv[1];
	char f_path[PATH_MAX + NAME_MAX + 1];
	char f_summary[LINE_MAX];
	

	while((f_info=readdir(directory)) != NULL){

		sprintf(f_path,"%s%s",dir_path,f_info->d_name);

		if(lstat(f_path,&f_stat) < 0){
			perror("[lstdir/lstat]");
			exit(1);
		}

		int status; //para o wait
		if(S_ISREG(f_stat.st_mode)){
			if((mod_date = localtime(&f_stat.st_mtime)) == NULL){
				perror("[lstdir/localtime]");
				exit(1);
			}

			sprintf(f_summary,"%s$%d$%o %d-%d-%d-%d:%d:%d %s\n",
					f_info->d_name,
					(int)f_stat.st_size,
					f_stat.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO),
					mod_date->tm_year+1900,
					mod_date->tm_mon,
					mod_date->tm_mday,
					mod_date->tm_hour,
					mod_date->tm_min,
					mod_date->tm_sec,
					f_path);

			if(write(fd,f_summary,strlen(f_summary)) < 0){
				perror("[lstdir/write]");
				exit(1);	
			}
		}
		else 
			if(S_ISDIR(f_stat.st_mode) && strcmp(f_info->d_name, ".") && strcmp(f_info->d_name, "..")){
				if(fork() == 0){//processo filho chama lstdir para listar o diretorio em f_path
					sprintf(f_path,"%s%s",f_path,"/");
					if(access(f_path,R_OK) == 0)
						execl(argv[0],argv[0],f_path,NULL);
					else
						exit(0);
					}

					if(wait(&status) < 0){
						perror("[lstdir/wait]");
						exit(1);
					}
				}
			


	}
	
	if(closedir(directory) < 0 || close(fd) < 0){
		perror("[lstdir/closedir]");
		exit(1);
	}

	exit(0);
}


