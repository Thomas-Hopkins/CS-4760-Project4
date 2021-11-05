#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "shared.h"
#include "config.h"

extern struct oss_shm shared_mem;
static char* exe_name;

void help() {
    printf("Operating System Simulator usage\n");
	printf("\n");
	printf("[-h]\tShow this help dialogue.\n");
	printf("-s sec\tSet the maximum runtime before system terminates (Required).\n");
	printf("-l filename\tSet the log file name (Required).\n");
	printf("\n");
}

void init_oss() {
    // Attach to shared memory.
    init_shm();
    shared_mem.sys_clock.seconds = 0;
    shared_mem.sys_clock.nanoseconds = 0;
    shared_mem.process_table_size = 0;

    // Create message queue
    init_msg(true);
}

void dest_oss() {
    dest_shm();
    dest_msg();
}

int launch_child(int mode) {
    char* program = "./osschild";
    return execl(program, program, "-m", mode, NULL);
}

int main(int argc, char** argv) {
    int option;
    int max_secs = -1;
    char* logfile = "";
    exe_name = argv[0];

    // Process arguments
    while ((option = getopt(argc, argv, "hs:l:")) != -1) {
        switch (option) {
            case 'h':
                help();
                exit(EXIT_SUCCESS);
            case 's':
                max_secs = atoi(optarg);
                if (max_secs <= 0) {
                    errno = EINVAL;
                    fprintf(stderr, "%s: ", exe_name);
                    perror("Passed invalid integer for max runtime");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'l':
                logfile = optarg;
                break;
            case '?':
                // Getopt handles error messages
                exit(EXIT_FAILURE);
        }
    }
    if (logfile == NULL) {
        errno = EINVAL;
        fprintf(stderr, "%s: ", exe_name);
        perror("Did not specify logfile with -l");
        exit(EXIT_FAILURE);
    }
    if (max_secs <= 0) {
        errno = EINVAL;
        fprintf(stderr, "%s: ", exe_name);
        perror("Did not specify max runtime with -s");
        exit(EXIT_FAILURE);
    }

    // Initialize the OSS shared memory
    init_oss();

    printf("%ld:%ld", shared_mem.sys_clock.seconds, shared_mem.sys_clock.nanoseconds);
    
    // Destruct OSS shared memory
    dest_oss();
    // TODO: Allocate shared memory for:
    //          process table of 18 process control blocks each containing: 
    //              cpu time used
    //              total time in the system
    //              time used during the last burst
    //              your local simulated pid
    //              process priority
    //          System clock
    //              unsigned int to store nanoseconds
    //              unsigned int to store seconds
    //              viewable by children but not modifible.
    //              children generate random number which OSS adds back to system clock, passed with message queue
    //              OSS itself adds small time to system clock if it has done something that would take time
    //              Every iteration of main loop increment clock 1s and [0, 1000]ns
    //       Create local bit vector of int32 to store if block is filled or not
    //       OSS schedules a child randomly in the future based on system clock
    //           const maxTimeBetweenNewProcsNS
    //           const maxTimeBetweenNewProcsSecs
    //           generate randomly between 0-these constants
    //           child launches once time is reached or exceeded
    //           if process table is full skip, but generate new time
    //           
    //       children will either be IO bound or cpu bounc
    //           const percentChanceIsIO
    //           Generate more cpu bound processes

    exit(EXIT_SUCCESS);
}
