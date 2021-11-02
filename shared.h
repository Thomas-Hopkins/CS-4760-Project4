#ifndef __SHARED_H
#define __SHARED_H

#define MAX_PROCESSES 18
#define SHM_FILE "shmOSS.shm"

struct time {
    unsigned long nanoseconds;
    unsigned long seconds;    
};

struct process_ctrl_block {
    time total_cpu_time;
    time total_sys_time;
    time last_burst_time;
    unsigned int pid;
    unsigned int priority;
};

struct oss_shm {
    time sys_clock;
    process_ctrl_block process_table[MAX_PROCESSES];
    size_t process_table_size;
};

void init_shm();

#endif
