#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/schedinfo.h"
#include "kernel/defs.h"
#include "kernel/param.h"

int main(int argc, char *argv[]){
    int d_flag;
    int n_flag;

    int n_flag = 0;
    int d_flag = 0;

    int n_value = 0;       // Number of frames
    int d_value = 10;      // Refresh interval (default 10)

    int i = 1;
    
    while(i < argc) {

        if(argv[i][0] == '-') {

            if(argv[i][1] == 'n' && argv[i][2] == '\0') {

                if(n_flag == 1 || i + 1 >= argc) {
                    printf("Invalid -n argument\n");
                    exit(1);
                }

                n_value = atoi(argv[i + 1]);

                if(n_value <= 0) {
                    printf("Invalid frame count\n");
                    exit(1);
                }

                n_flag = 1;
                i = i + 2;
            }

            else if(argv[i][1] == 'd' && argv[i][2] == '\0') {

                if(d_flag == 1 || i + 1 >= argc) {
                    printf("Invalid -d argument\n");
                    exit(1);
                }

                d_value = atoi(argv[i + 1]);

                if(d_value <= 0) {
                    printf("Invalid refresh interval\n");
                    exit(1);
                }

                d_flag = 1;
                i = i + 2;
            }

            else {
                printf("Unknown flag\n");
                exit(1);
            }
        }

        else {
            printf("Unexpected argument\n");
            exit(1);
        }
    }
    
    int pids[NPROC];
    struct schedinfo si;

    int max = getpids(&pids, NPROC);

    
}