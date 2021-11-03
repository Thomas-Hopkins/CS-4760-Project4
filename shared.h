#ifndef __SHARED_H
#define __SHARED_H

#define MAX_PROCESSES 18
#define SHM_FILE "shmOSS.shm"

struct time {
    unsigned long nanoseconds;
    unsigned long seconds;    
};

struct process_ctrl_block {
    struct time total_cpu_time;
    struct time total_sys_time;
    struct time last_burst_time;
    unsigned int pid;
    unsigned int priority;
};

struct oss_shm {
    struct time sys_clock;
    struct process_ctrl_block process_table[MAX_PROCESSES];
    size_t process_table_size;
};

void init_shm();
void dest_shm();

#endif
