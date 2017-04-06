#include <stdio.h>
#include <string.h>
#include <signal.h>

int signal_received = 0;

//SIGINT HANDLER (CTRL-C)
void sigint_handler(int sig) {

  char  c;

  printf("\n > Are you sure you want to terminate (Y/N)? ");
  scanf(" %c", &c);

  if (c == 'y' || c == 'Y')
    exit(0);
  else if (c == 'n' || c == 'N')
    return;
  else{
    printf(" >> ERROR: Character not allowed!");
    sigint_handler(SIGINT);
  }
    return;
 }

int main (int argc, char* argv[]){

  //Handling signal
  struct sigaction action, orig_action;
  action.sa_handler = sigint_handler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;
  sigaction(SIGINT,&action,NULL);

  char* fileName =NULL;
  char* type = NULL;
  char* mode =NULL;
  char* command = NULL;
  char* toPrint = "NO";
  char* toDelete = "NO";

  int i;
  for (i = 1; i < argc; i++){

    if(strcmp(argv[i],"-name") == 0){
        fileName = argv[i+1];
        continue;
    }

    if(strcmp(argv[i],"-type") == 0){
        type = argv[i+1];
        continue;
    }

    if(strcmp(argv[i],"-mode") == 0){
        mode = argv[i+1];
        continue;
    }

    if(strcmp(argv[i],"-exec") == 0){
        command = argv[i+1];
        continue;
    }

    if(strcmp(argv[i],"-print") == 0){
        toPrint = "YES";
        continue;
    }

    if(strcmp(argv[i],"-delete") == 0){
        toDelete = "YES";
        continue;
    }

  }

  if(fileName==NULL){
    printf(" >> ERROR: You didnt insert the name of the file you want to find!\n");
    exit(0);
  }

  printf ("NAME: %s\n", fileName);
  printf ("TYPE: %s\n", type);
  printf ("PERM: %s\n", mode);
  printf ("EXEC: %s\n", command);
  printf ("DELETE: %s\n", toDelete);
  printf ("PRINT: %s\n", toPrint);


  //TODO: DELETE
  for(;;){}

  return 0;
}
