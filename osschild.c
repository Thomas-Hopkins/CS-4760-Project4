#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "shared.h"

extern struct oss_shm shared_mem;
static char* exe_name;

void help() {
    printf("Operating System Simulator Child usage\n");
    printf("Runs as a child of the OSS. Not to be run alone.\n");
}

int main(int argc, char** argv) {
    int option;
    int mode;
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
    init_shm();
    init_msg(false);
    // signal to add to process queue

    // wait until signaled to run

    // generate random time used and update sys clock with it
    // based on the mode of execution (io or cpu bound)

    // Return simulated pid to signal oss to deallocate process block
    exit(sim_pid);
}