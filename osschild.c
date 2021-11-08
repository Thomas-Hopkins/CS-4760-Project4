#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "shared.h"
#include "config.h"

extern struct oss_shm* shared_mem;
static struct message msg; 
static char* exe_name;
static int mode;

void help() {
    printf("Operating System Simulator Child usage\n");
    printf("Runs as a child of the OSS. Not to be run alone.\n");
}

void init_child() {
    // Init rand gen, shared mem, and msg queues
    srand((int)time(NULL) + getpid());
    init_shm();
    init_msg(false);
}

void recieve_message(char* message) {
    msg.msg_type = getpid();
    if (recieve_msg(&msg, CHILD_MSG_SHM, true) < 0) {
        perror("Failed to recieve message");
    }
    strncpy(message, msg.msg_text, MSG_BUFFER_LEN);
}

void send_message(char* message) {
    msg.msg_type = getpid();
    strncpy(msg.msg_text, message, MSG_BUFFER_LEN);
    if (send_msg(&msg, OSS_MSG_SHM, true) < 0) {
        perror("Failed to send message");
    }
}

int main(int argc, char** argv) {
    int option;
    int sim_pid;
    exe_name = argv[0];

    while ((option = getopt(argc, argv, "hm:p:")) != -1) {
        switch (option)
        {
        case 'h':
            help();
            exit(sim_pid);
        case 'm':
            mode = atoi(optarg);
            break;
        case 'p':
            sim_pid = atoi(optarg);
            break;
        case '?':
            // Getopt handles error messages
            exit(sim_pid);
        }
    }
    init_child();

    // Main loop
    while (true) {
        int percent_completed = 0;
        int priority = shared_mem->process_table[sim_pid].priority;
        char recv_msg_buf[MSG_BUFFER_LEN];
        char send_msg_buf[MSG_BUFFER_LEN];
        recieve_message(recv_msg_buf);

        // Parse message to get time to run for
        struct time_clock runtime;
        char* cmd = strtok(recv_msg_buf, " ");
        if (strncmp(cmd, "run", MSG_BUFFER_LEN) == 0) {
            runtime.seconds = atoi(strtok(NULL, " "));
            runtime.nanoseconds = atoi(strtok(NULL, " "));
        }

        // Calculate chance for a block (twice as likely for IO bound)
        if (rand() % 100 < percentChanceBlock * (priority == IO_MODE? 2 : 1)) {
            percent_completed = (rand() % 99); // 0-99% 
            snprintf(send_msg_buf, MSG_BUFFER_LEN, "blocked %d", percent_completed);
            send_message(send_msg_buf);

            shared_mem->process_table[sim_pid].last_burst_time.seconds = runtime.seconds * percent_completed;
            shared_mem->process_table[sim_pid].last_burst_time.nanoseconds = runtime.nanoseconds * percent_completed;

            shared_mem->process_table[sim_pid].total_cpu_time.seconds += runtime.seconds * percent_completed;
            shared_mem->process_table[sim_pid].total_cpu_time.nanoseconds += runtime.nanoseconds * percent_completed;

            send_message("unblocked");     
        }
        // Chance for terminating 
        else if (rand() % 100 < percentChanceTerminate) {
            percent_completed = (rand() % 99); // 0-99% 
            snprintf(send_msg_buf, MSG_BUFFER_LEN, "terminated %d", percent_completed);
            send_message(send_msg_buf);
            
            shared_mem->process_table[sim_pid].last_burst_time.seconds = runtime.seconds * percent_completed;
            shared_mem->process_table[sim_pid].last_burst_time.nanoseconds = runtime.nanoseconds * percent_completed;

            shared_mem->process_table[sim_pid].total_cpu_time.seconds += runtime.seconds * percent_completed;
            shared_mem->process_table[sim_pid].total_cpu_time.nanoseconds += runtime.nanoseconds * percent_completed;

            exit(sim_pid);
        }
        // Otherwise, finished
        else {
            send_message("finished");

            shared_mem->process_table[sim_pid].last_burst_time.seconds = runtime.seconds;
            shared_mem->process_table[sim_pid].last_burst_time.nanoseconds = runtime.nanoseconds;

            shared_mem->process_table[sim_pid].total_cpu_time.seconds += runtime.seconds;
            shared_mem->process_table[sim_pid].total_cpu_time.nanoseconds += runtime.nanoseconds;
        }
        
    }

    exit(sim_pid);
}