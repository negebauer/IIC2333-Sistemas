#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>


#define ERR_NO_ARGS 53
#define ERR_NO_PROGRAM_NAME 100

int
main (int argc, char *argv[])
{
  printf("Received %d arguments:\n");
  if(argc==0){
    printf("0 arguments received.");
    exit(ERR_NO_PROGRAM_NAME);
  }
  else if(argc==1){
    printf("No arguments given, exiting with code %d\n", ERR_NO_ARGS);
    exit(ERR_NO_ARGS);
  }

  printf("\n");
  printf("Program name may be '%s'\n", argv[0]);
  printf("\n");

  printf("  * ");
  for(int i=1; i<argc; i+=2)
    printf("%s ", argv[i]);
  printf("\n");

  printf("  * ");
  for(int i=2; i<argc; i+=2)
    printf("%s ", argv[i]);
  printf("\n");
  exit(0);
}
