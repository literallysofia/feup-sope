#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

char* fileName = "";
char* fileType = "";
int fileMode = 0;
char* command = "";
char* toPrint = "NO";
char* toDelete = "NO";
char* dir = "";

void sigint_handler(int sig) { //main handler

	char  c;

	printf("\n > Are you sure you want to terminate (Y/N)? ");
	scanf(" %c", &c);

	if (c == 'y' || c == 'Y')
		exit(0);
	else if (c == 'n' || c == 'N')
		return;
	else {
		printf(" >> ERROR: Character not allowed!");
		sigint_handler(SIGINT);
	}
	return;
}

void fileAction(char* dirName, char* name) {


	if (strcmp(toPrint, "YES") == 0)
		printf("%s\n", dirName);

	if (strcmp(command, "") != 0)
		execlp(command, command, dirName, NULL);

	if (strcmp(toDelete, "YES") == 0)
		remove(dirName);


	return;
}


void verifyFile(char* dirName, char* name, char* type, int perm) {

	int isNameValid = 1;
	int isTypeValid = 1;
	int isPermValid = 1;

	if (strcmp(fileName, "") != 0 && strcmp(fileName, name) != 0)
		isNameValid = 0;

	if (strcmp(fileType, "") != 0 && strcmp(fileType, type) != 0)
		isTypeValid = 0;

	if (fileMode != 0 && fileMode != perm)
		isPermValid = 0;

	if (isNameValid && isTypeValid && isPermValid)
		fileAction(dirName, name);

	return;
}

void process_info(char* dirName, struct dirent *direntp, struct stat stat_buf) {

	char* type;

	//type
	if (S_ISREG(stat_buf.st_mode)) type = "f";

	else if (S_ISDIR(stat_buf.st_mode)) {
		type = "d";
	}
	else type = "l";

	//mode (permissions)
	int userPerm = 0;
	int grpPerm = 0;
	int othrPerm = 0;
	int perm = 0;


	if (stat_buf.st_mode & S_IRUSR) userPerm += 4; //r
	if (stat_buf.st_mode & S_IWUSR) userPerm += 2; //w
	if (stat_buf.st_mode & S_IXUSR) userPerm += 1; //x
	if (stat_buf.st_mode & S_IRGRP) grpPerm += 4; //r
	if (stat_buf.st_mode & S_IWGRP) grpPerm += 2; //w
	if (stat_buf.st_mode & S_IXGRP) grpPerm += 1; //x
	if (stat_buf.st_mode & S_IROTH) othrPerm += 4; //r
	if (stat_buf.st_mode & S_IWOTH) othrPerm += 2; //w
	if (stat_buf.st_mode & S_IXOTH) othrPerm += 1; //x


	perm = userPerm * 100 + grpPerm * 10 + othrPerm;

	verifyFile(dirName, direntp->d_name, type, perm);

	//printf("NAME: %-15s  TYPE: %s  MODE: %d\n", direntp->d_name, type, perm);

	return;

}

void search_dirs(char* dirname) {

	int pid;
	int status;

	pid = fork();

	if (pid > 0) {
		wait(&status);
	}
	else if (pid == 0) {

		//printf("\n > DIR: %s\n", dirname);

		DIR *dirp;
		struct dirent *direntp;
		struct stat stat_buf;
		char name[200];

		dirp = opendir(dirname);

		while ((direntp = readdir(dirp)) != NULL)
		{
			sprintf(name, "%s/%s", dirname, direntp->d_name);

			if (lstat(name, &stat_buf) == -1)
			{
				perror("lstat ERROR");
				exit(3);
			}

			//type
			if (S_ISREG(stat_buf.st_mode))
				process_info(name, direntp, stat_buf);
			else if (S_ISDIR(stat_buf.st_mode))
			{
				if (strcmp(direntp->d_name, ".") != 0 && strcmp(direntp->d_name, "..") != 0) {
					process_info(name, direntp, stat_buf);
					search_dirs(name);
				}
			}
			else process_info(name, direntp, stat_buf);

		}

		closedir(dirp);
		exit(0);
	}

	else {
		printf(" >> ERROR: Fork error.");
		return;
	}

	return;
}

int main(int argc, char* argv[]) {

	//Handling signal
	struct sigaction action;
	action.sa_handler = sigint_handler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	sigaction(SIGINT, &action, NULL);

	dir = argv[1];

	int i;
	for (i = 1; i < argc; i++) {

		if (strcmp(argv[i], "-name") == 0) {
			fileName = argv[i + 1];
			continue;
		}

		if (strcmp(argv[i], "-type") == 0) {
			fileType = argv[i + 1];
			continue;
		}

		if (strcmp(argv[i], "-perm") == 0) {
			fileMode = atoi(argv[i + 1]);
			continue;
		}

		if (strcmp(argv[i], "-exec") == 0) {
			command = argv[i + 1];
			continue;
		}

		if (strcmp(argv[i], "-print") == 0) {
			toPrint = "YES";
			continue;
		}

		if (strcmp(argv[i], "-delete") == 0) {
			toDelete = "YES";
			continue;
		}

	}

	search_dirs(dir);

	return 0;
}
