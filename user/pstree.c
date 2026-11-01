#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char *argv[]) {
  int p_flag = 0;
  int m_flag = 0;
  int detect = 0;


  for (int i = 1; i < argc; i++) {
    char *arg = argv[i];

    if(arg[0] == '-') {

      for(int j = 1; arg[j] != '\0'; j++) {
        if(arg[j] == 'p') {
          p_flag = 1;
          detect++;
        }
        else if (arg[j] == 'm') {
          m_flag = 1;
          detect++;
        }
        else {
         printf("Command not recognized!!\n");
         return -1;
        }
      }

    }
    else {
      printf("INVALID!!\n");
      return -1;
    }
  }

  struct my_data list[NPROC];
  int my_pid = getpid();

  int count = gettree(my_pid, list); //The count will return number of ancestors.

  if (count < 0) {
    printf("pstree error: process with PID %d not found\n", my_pid);
    exit(1);
  }

  //In the implementation of syscall gettree I know, the 1st element is the root, and then the subsequent ones are the ancestors in order.

  for(int i = 0; i < count; i++) {
    for(int j = 0; j < i; j++) {
      printf(" ");
    }
    if(i != 0) {
      printf("-| ");
    }

    printf("%s", (list[count - i - 1]).name);
    if(p_flag) {
      printf("(%d)", (list[count - i - 1]).pid );
    }
    if (m_flag) {
      printf(" [%ldB]", (list[count - i - 1]).sz);
    }
    printf("\n");
  }
}
