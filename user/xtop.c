#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/schedinfo.h"
#include "kernel/param.h"
#include "user.h"

int main(int argc, char *argv[]){
    int n_flag = 0;
    int d_flag = 0;

    int n_value = -1;       // Number of frames
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

    int j = 0;
    while (j != n_value) {
        printf("\x1b[H\x1b[2J"); // ANSI
        int max = getpids(pids, NPROC);

        printf("PID     NAME     STATE     PRIO     CPU     MASK     SLICE     %%CPU\n");
        for(int a = 0; a < max; a++){
            if(schedinfo(pids[a], &si) < 0){
                printf("Fail line 83\n");
                return -1;
            }
            printf("%d     %s     %d     %d     %d     %d     %d     IDK\n", si.pid, si.name, si.state, si.prio, si.lastcpu, si.affinity, si.slice);

        }

        j++;
        int start = uptime();
        while(uptime() - start < d_value) {
            // busy-wait for 10 ticks
        }
    }
    //So effectively, if frames not provided, j will never attain -1. Hence infy loop. 
    //Else j will hit and break out. 
}