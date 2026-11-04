#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/schedinfo.h"
#include "kernel/param.h"
#include "user.h"

#define MAXH 8

struct schedinfo cur[NPROC];
int pct[NPROC], order[NPROC];
int prev_pid[NPROC], prev_rt[NPROC], nprev;

char *
statename(int s)
{
  switch(s){
  case PS_USED:     return "used";
  case PS_SLEEPING: return "sleep";
  case PS_RUNNABLE: return "runble";
  case PS_RUNNING:  return "run";
  case PS_ZOMBIE:   return "zombie";
  default:          return "?";
  }
}

int
collect(void)
{
  int pids[NPROC];
  int n = getpids(pids, NPROC), k = 0;
  for(int i = 0; i < n; i++)
    if(schedinfo(pids[i], &cur[k]) == 0)   // skip if it exited meanwhile
      k++;
  return k;
}

void
remember(int k)
{
  for(int i = 0; i < k; i++){
    prev_pid[i] = cur[i].pid;
    prev_rt[i] = cur[i].rticks;
  }
  nprev = k;
}

int
prev_rticks(int pid)
{
  for(int i = 0; i < nprev; i++)
    if(prev_pid[i] == pid)
      return prev_rt[i];
  return 0;
}

void
draw(int interval, int nharts)
{
  int k = collect(), busy[MAXH], runnable = 0, me = getpid();

  for(int h = 0; h < MAXH; h++)
    busy[h] = 0;
  for(int i = 0; i < k; i++){
    int d = cur[i].rticks - prev_rticks(cur[i].pid);
    if(d < 0) d = 0;
    pct[i] = d * 100 / interval;
    if(cur[i].lastcpu >= 0 && cur[i].lastcpu < MAXH)
      busy[cur[i].lastcpu] += d;
    if((cur[i].state == PS_RUNNABLE || cur[i].state == PS_RUNNING) && cur[i].pid != me)
      runnable++;
    order[i] = i;
  }
  for(int i = 1; i < k; i++){            // sort by %CPU, highest first
    int t = order[i], j = i - 1;
    while(j >= 0 && pct[order[j]] < pct[t]){
      order[j + 1] = order[j];
      j--;
    }
    order[j + 1] = t;
  }

  printf("\x1b[H\x1b[2J");
  printf("xtop up %d ticks procs %d runnable %d policy %s\n",
         uptime(), k, runnable, schedpolicy(-1) == POLICY_PRIO ? "PRIO" : "RR");
  for(int h = 0; h < nharts; h++){
    int b = busy[h] * 100 / interval;
    printf("hart%d %d%% ", h, b > 100 ? 100 : b);
  }
  printf("\n");
  printf("PID NAME STATE PRIO CPU MASK SLICE %%CPU\n");
  for(int r = 0; r < k; r++){
    struct schedinfo *s = &cur[order[r]];
    printf("%d %s %s %d %d 0x%x %d %d\n", s->pid, s->name, statename(s->state),
           s->prio, s->lastcpu, s->affinity, s->slice, pct[order[r]]);
  }
  remember(k);
}


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
    
    int nharts = 0, mask = sched_getaffinity(1);
    for(int b = 0; b < MAXH; b++)
        if(mask & (1 << b))
            nharts = b + 1;
    if(nharts == 0)
        nharts = 1;

    remember(collect());      
    int last = uptime();
    pause(d_value);           
    int j = 0;
    while (j != n_value) {
        int now = uptime();
        draw(now - last > 0 ? now - last : 1, nharts);
        last = now;//ANSI
        j++;

        pause(d_value);       // busy-wait for 10 ticks
    }
    //So effectively, if frames not provided, j will never attain -1. Hence infy loop. 
    //Else j will hit and break out. 
    exit(0); 
}
