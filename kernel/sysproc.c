#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"
#include "schedinfo.h" //PA4!!

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if (t == SBRK_EAGER || n < 0) {
    if (growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if (addr + n < addr)
      return -1;
    if (addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if (n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (killed(myproc())) {
      release(&tickslock);
      return -1;
    }
    sleep_prepare(&ticks);
    release(&tickslock);
    sleep();
    acquire(&tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

//CHANGED HERE!!
uint64
sys_getuptime(void)
{
  uint total_ticks;

  acquire(&tickslock);
  total_ticks = ticks;
  release(&tickslock);
  return total_ticks;
}

extern struct proc proc[NPROC];

uint64
sys_activecount(void)
{
  struct proc *p; // A pointer to the struct proc, defined within proc.h
  uint count = 0;
  for (p = proc; p < &proc[NPROC]; p++) { // Iterates over process table
    acquire(&(*p).lock);
    if ((*p).state != UNUSED) {
      count++;
    }
    release(&(*p).lock);
  }

  return count;
}

uint64
sys_lineage(void)
{
  int pid;
  argint(0, &pid);

  struct proc *p;
  struct proc *pidfound = 0;

  for (p = proc; p < &proc[NPROC]; p++) {
    acquire(&(*p).lock);
    if ((*p).state != UNUSED && (*p).pid == pid) {
      pidfound = p;
      release(&(*p).lock);
      break;
    }
    release(&(*p).lock);
  }

  if (pidfound == 0) {
    return -1;
  }

  int ancestors = 0;
  struct proc *curr_pid = pidfound;

  while (curr_pid != 0) {
    acquire(&(*curr_pid).lock);

    if ((*curr_pid).state == UNUSED) {
      release(&(*curr_pid).lock);
      break;
    }

    char name[16];
    safestrcpy(name, (*curr_pid).name, 16);
    int p_val = (*curr_pid).pid;
    struct proc *parent = (*curr_pid).parent;

    release(&(*curr_pid).lock);

    printk("PID %d : %s\n", p_val, name);

    ancestors++;
    curr_pid = parent;
  }

  return ancestors;
}

uint64
sys_getprocsize(void)
{
  int pid;
  argint(0, &pid);

  struct proc *p;
  uint size = -1;
  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&(*p).lock);
    if ((*p).state != UNUSED && (*p).pid == pid) {
      size = (*p).sz;
      release(&(*p).lock);
      break;
    }
    release(&(*p).lock);
  }
  return size;
}

uint64
sys_familyheadcount(void)
{
  int pid;
  argint(0, &pid);
  struct proc *p;
  int count = 0;
  for (p = proc; p < &proc[NPROC]; p++) {
    acquire(&(*p).lock);
    if ((*p).state != UNUSED && (*p).parent != 0) {
      if ((*((*p).parent)).pid == pid) {
        count++;
      }
    }
    release(&(*p).lock);
  }
  return count;
}
//BONUS

uint64
sys_gettree(void)
{
  int pid;
  uint64 d_ptr;

  argint(0, &pid);
  argaddr(1, &d_ptr);

  struct my_data my_data[NPROC];

  struct proc *p;
  struct proc *pidfound = 0;

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&(*p).lock);
    if ((*p).state != UNUSED && (*p).pid == pid) {
      pidfound = p;
      release(&(*p).lock);
      break;
    }
    release(&(*p).lock);
  }

  if (pidfound == 0) {
    return -1;
  }

  int counted = 0;
  struct proc *curr_pid = pidfound;
  struct my_data *m = my_data;

  for(int i = 0; i < NPROC; i++) {
    my_data[i].pid = 0;
    my_data[i].sz = 0;
    my_data[i].name[0] = '\0';
  }

  while(curr_pid != 0) {
    acquire(&(*curr_pid).lock);

    if((*curr_pid).state == UNUSED) {
      release(&(*curr_pid).lock);
      break;
    }

    (*m).pid = (*curr_pid).pid;
    (*m).sz = (*curr_pid).sz;
    safestrcpy((*m).name, (*curr_pid).name, sizeof((*m).name));

    struct proc *parent = (*curr_pid).parent;
    release(&(*curr_pid).lock);

    counted++;
    m++;
    curr_pid = parent;
  }

  struct proc *a = myproc();
  if (copyout((*a).pagetable,(*a).sz, d_ptr, (char *)my_data, sizeof(my_data)) < 0) {
    return -1;
  }

  return counted;
}
//CHANGE ENDS

//PA4 Begins
uint64
sys_getcpu(void) {
  int cpu;
  push_off();
  cpu = cpuid();
  pop_off();
  return cpu;
}

uint64
sys_schedinfo(void) {
  int pid;
  uint64 schedinfo_ptr;

  argint(0, &pid);
  argaddr(1, &schedinfo_ptr);

  struct proc *p;
  struct schedinfo si;

  memset(&si, 0, sizeof(si));

  int found = 0;
  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&(*p).lock);
    if((*p).state != UNUSED && (*p).pid == pid) {
      si.nsched = (*p).nsched;
      si.lastcpu = (*p).lastcpu;
      si.rticks = (*p).rticks;
      si.pid = (*p).pid;
      si.state = (*p).state;
      si.slice = (*p).slice;
      si.ctime = (*p).ctime;
      si.ftime = (*p).ftime;
      si.firstcpu = (*p).firstcpu;
      si.affinity = (*p).affinity & __atomic_load_n(&booted_harts, __ATOMIC_SEQ_CST);
      si.prio = (*p).prio;
      safestrcpy(si.name, (*p).name, sizeof(si.name));
      release(&(*p).lock);
      found = 1;
      break;
    }
    release(&(*p).lock);
  }

  if(found) {
    struct proc *a = myproc();
    if(copyout((*a).pagetable,(*a).sz, schedinfo_ptr, (char *)&si, sizeof(si)) < 0) {
      return -1;
    }
    return 0;
  } else {
    return -1;
  }
}

uint64
sys_setslice(void) {
  int pid;
  int time;

  argint(0, &pid);
  argint(1, &time);

  if(time < 1 || time > 100) {
    return -1; //INVALID
  }

  struct proc *a;

  if(pid == 0) {
    a = myproc();
    acquire(&(*a).lock);
    pid = (*a).pid;
    release(&(*a).lock);
  }

  struct proc *p;
  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&(*p).lock);
    if((*p).pid == pid && (*p).state != UNUSED) {
      (*p).slice = time;
      release(&(*p).lock);
      return 0;
    }
    release(&(*p).lock);
  }

  return -1;//PID not found
}

uint64
sys_forkwatch(void) {
  int on;
  argint(0, &on);
  int prev_val;

  acquire(&printlock);
  prev_val = can_print;

  if(on){
    can_print = 1;
  } else {
    can_print = 0;
  }

  release(&printlock);

  return prev_val;
}

uint64
sys_sched_setaffinity(void)
{
  int pid, mask;
  struct proc *p;
  int booted = __atomic_load_n(&booted_harts, __ATOMIC_SEQ_CST);

  argint(0, &pid);
  argint(1, &mask);
  if((mask & booted) == 0)
    return -1;
  mask &= booted;
  if(pid == 0)
    pid = myproc()->pid;

  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->pid == pid && p->state != UNUSED){
      p->affinity = mask;
      release(&p->lock);
      if(p == myproc()){
        push_off();
        int id = cpuid();
        pop_off();
        if(!(mask & (1 << id)))
          yield();
      }
      return 0;
    }
    release(&p->lock);
  }
  return -1;
}

uint64
sys_sched_getaffinity(void)
{
  int pid, mask;
  struct proc *p;
  int booted = __atomic_load_n(&booted_harts, __ATOMIC_SEQ_CST);

  argint(0, &pid);
  if(pid == 0)
    pid = myproc()->pid;

  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->pid == pid && p->state != UNUSED){
      mask = p->affinity & booted;
      release(&p->lock);
      return mask;
    }
    release(&p->lock);
  }
  return -1;
}

uint64
sys_schedpolicy(void)
{
  int policy;
  argint(0, &policy);

  if(policy == -1)
    return __atomic_load_n(&sched_policy, __ATOMIC_SEQ_CST);
  if(policy != POLICY_RR && policy != POLICY_PRIO)
    return -1;
  return __atomic_exchange_n(&sched_policy, policy, __ATOMIC_SEQ_CST);
}

uint64
sys_setprio(void)
{
  int pid, prio, old;
  struct proc *p;

  argint(0, &pid);
  argint(1, &prio);
  if(prio < PRIO_HIGH || prio > PRIO_LOW)
    return -1;
  if(pid == 0)
    pid = myproc()->pid;

  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->pid == pid && p->state != UNUSED){
      old = p->prio;
      p->prio = prio;
      release(&p->lock);
      if(p == myproc() && prio > old &&
         __atomic_load_n(&sched_policy, __ATOMIC_SEQ_CST) == POLICY_PRIO)
        yield();
      return 0;
    }
    release(&p->lock);
  }
  return -1;
}
