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

char* fileName = NULL;
char* type = NULL;
int mode = NULL;
char* command = NULL;
char* toPrint = "NO";
char* toDelete = "NO";

void sigint_cHandler(int sig) //child handler
{
}

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

void process_files(struct dirent *direntp, struct stat stat_buf) {

	char *type;

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


	printf("NAME: %-15s  TYPE: %s  MODE: %d\n", direntp->d_name, type, perm);

}

void search_dirs(char* dirname) {

	int pid;
	int status;

	pid = fork();

	if (pid > 0) {
		wait(&status);
	}
	else if (pid == 0) {

		//Handling signal
		struct sigaction action;
		action.sa_handler = sigint_cHandler;
		sigemptyset(&action.sa_mask);
		action.sa_flags = 0;
		sigaction(SIGINT, &action, NULL);

		printf(" > DIR: %s\n", dirname);

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
			if (S_ISREG(stat_buf.st_mode)) process_files(direntp, stat_buf);
			else if (S_ISDIR(stat_buf.st_mode))
			{
				if (strcmp(direntp->d_name, ".") != 0 && strcmp(direntp->d_name, "..") != 0) {
					process_files(direntp, stat_buf);
					search_dirs(name);
				}
			}
			else process_files(direntp, stat_buf);

		}

		closedir(dirp);
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

	int i;
	for (i = 1; i < argc; i++) {

		if (strcmp(argv[i], "-name") == 0) {
			fileName = argv[i + 1];
			continue;
		}

		if (strcmp(argv[i], "-type") == 0) {
			type = argv[i + 1];
			continue;
		}

		if (strcmp(argv[i], "-mode") == 0) {
			mode = atoi(argv[i + 1]);
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

	if (fileName == NULL) {
		printf(" >> ERROR: You didnt insert the name of the file you want to find!\n");
		exit(0);
	}

	printf("NAME: %s\n", fileName);
	printf("TYPE: %s\n", type);
	printf("PERM: %i\n", mode);
	printf("EXEC: %s\n", command);
	printf("DELETE: %s\n", toDelete);
	printf("PRINT: %s\n", toPrint);
	printf("\n");

	char* dirname = ".";
	search_dirs(dirname);

	//TODO: DELETE
	for (;;) {}

	return 0;
}
