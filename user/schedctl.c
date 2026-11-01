#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if(argc == 3 && strcmp(argv[1], "policy") == 0){
    printf("old policy %d\n", schedpolicy(atoi(argv[2])));
    exit(0);
  }
  if(argc == 4 && strcmp(argv[1], "prio") == 0){
    printf("setprio: %d\n", setprio(atoi(argv[2]), atoi(argv[3])));
    exit(0);
  }
  printf("usage: schedctl policy N | schedctl prio PID P\n");
  exit(1);
}