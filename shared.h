#ifndef __SHARED_H
#define __SHARED_H

#include <stdbool.h>

#define MAX_PROCESSES 18
#define SHM_FILE "shmOSS.shm"
#define MSG_BUFFER_LEN 2048

enum Shared_Mem_Tokens {OSS_SHM, OSS_MSG_SHM, CHILD_MSG_SHM};
enum Process_Modes {IO_MODE, CPU_MODE};

struct time_clock {
    unsigned long nanoseconds;
    unsigned long seconds;    
};

struct message {
    long int msg_type;
    char msg_text[MSG_BUFFER_LEN];
};

struct process_ctrl_block {
    struct time_clock total_cpu_time;
    struct time_clock total_sys_time;
    struct time_clock last_burst_time;
    unsigned int pid;
    unsigned int priority;
    pid_t actual_pid;
};

struct oss_shm {
    struct time_clock sys_clock;
    struct process_ctrl_block process_table[MAX_PROCESSES];
    struct message message_queue[2];
    size_t process_table_size;
};

void init_shm();
void dest_shm();
void init_msg(bool create);
void dest_msg();
void add_time(struct time_clock* Time, unsigned long seconds, unsigned long nanoseconds);
bool sub_time(struct time_clock* Time, unsigned long seconds, unsigned long nanoseconds);
int recieve_msg(struct message* msg, int msg_queue, bool wait);
int send_msg(struct message* msg, int msg_queue, bool wait);


#endif
