#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "shared.h"

extern struct oss_shm* shared_mem;
static struct time_clock needed_time;
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

    // IO Mode proccess will run longer
    int min_time = 2;
    int max_time = 5;
    if (mode == IO_MODE) {
        min_time = 3;
        max_time = 8;
    }

    needed_time.nanoseconds = 0;
    needed_time.seconds = 0;
    // Calculate the needed time for this process to finish
    add_time(&needed_time, (rand() % max_time) + min_time, rand() % 1000000000);
}

void recieve_message(char* message) {
    if (recieve_msg(&msg, CHILD_MSG_SHM, true) < 0) {
        perror("Failed to recieve message");
    }
    strncpy(message, msg.msg_text, MSG_BUFFER_LEN);
}

void send_message(char* message) {
    msg.msg_type = getpid();
    strncpy(msg.msg_text, message, MSG_BUFFER_LEN);
    if (send_msg(&msg, OSS_MSG_SHM, false) < 0) {
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
    
    bool finished = false;
    // Signal OSS that this pid is ready
    char sendmessage[MSG_BUFFER_LEN];
    sprintf(sendmessage, "ready %d", sim_pid);
    send_message(sendmessage);

    // Main loop
    while (true) {
        char message[MSG_BUFFER_LEN];
        char* cmd;
        struct time_clock timeslice;

        // Wait until OSS sends up a message
        recieve_message(message);

        // Parse message
        cmd = strtok(message, " ");
        if (strncmp(cmd, "run", MSG_BUFFER_LEN) == 0) {
            timeslice.seconds = atoi(strtok(NULL, " "));
            timeslice.nanoseconds = atoi(strtok(NULL, " "));
        }

        // subtract timeslice from needed time if possible
        if (!sub_time(&needed_time, timeslice.seconds, timeslice.nanoseconds)) {
            // Needed time is less than the timeslice given. Update timeslice to only
            // Run that long, also update finished flag so we can stop looping.
            timeslice.seconds = needed_time.seconds;
            timeslice.nanoseconds = needed_time.nanoseconds;
            finished = true;
        }

        // TODO: Randomly block/terminate child and send signal to oss

        // Update the amount of time spent on the cpu in this increment
        shared_mem->process_table[sim_pid].last_burst_time.seconds = timeslice.seconds;
        shared_mem->process_table[sim_pid].last_burst_time.nanoseconds = timeslice.nanoseconds;

        // Update total amount of time spent on cpu
        shared_mem->process_table[sim_pid].total_cpu_time.seconds += timeslice.seconds;
        shared_mem->process_table[sim_pid].total_cpu_time.nanoseconds += timeslice.nanoseconds;

        if (finished) {
            break;
        }
        // Tell OSS ready for new schedule
        send_message(sendmessage);
    }
    send_message("finished");

    exit(sim_pid);
}