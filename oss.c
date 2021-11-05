#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <wait.h>

#include "shared.h"
#include "config.h"

short bit_vec[MAX_PROCESSES];
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
    // Initialize random number gen
    srand((int)time(NULL) + getpid());

    // Attach to shared memory.
    init_shm();
    shared_mem.sys_clock.seconds = 0;
    shared_mem.sys_clock.nanoseconds = 0;
    shared_mem.process_table_size = 0;

    // Initialize bit vector
    for (int i = 0; i < MAX_PROCESSES; i++) {
        bit_vec[i] = 0;
    }

    // Create message queue
    init_msg(true);
}

void dest_oss() {
    dest_shm();
    dest_msg();
}

int launch_child(int mode, int pid) {
    char* program = "./osschild";
    char cmd1[5];
    char cmd2[8];
    sprintf(cmd1, "-m %d", mode);
    sprintf(cmd2, "-p %d", pid);
    return execl(program, program, cmd1, cmd2, NULL);
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
    printf("%ld:%ld\n", shared_mem.sys_clock.seconds, shared_mem.sys_clock.nanoseconds);

    // Keep track of the last time on the sys clock when we run a process
    unsigned long last_run_seconds = 0;
    unsigned long last_run_nanoseconds = 0;

    // Main OSS loop. We handle scheduling processes here.
    while (true) {
        // Check if enough time has passed to spawn a new process
        if ((shared_mem.sys_clock.seconds - last_run_seconds > maxTimeBetweenNewProcsSecs) && 
        (shared_mem.sys_clock.nanoseconds - last_run_nanoseconds > maxTimeBetweenNewProcsNS)) {
            // Check process control block availablity
            if (shared_mem.process_table_size < MAX_PROCESSES) {
                int sim_pid;
                // Find open process in process table
                for (int i = 0; i < MAX_PROCESSES; i++) {
                    if (bit_vec[i] == 0) {
                        sim_pid = i;
                        bit_vec[i] = 1;
                        break;
                    }
                }

                // Initalize process in process table
                shared_mem.process_table_size++;

                shared_mem.process_table[sim_pid].total_cpu_time.nanoseconds = 0;
                shared_mem.process_table[sim_pid].total_cpu_time.seconds = 0;
            
                shared_mem.process_table[sim_pid].total_sys_time.nanoseconds = 0;
                shared_mem.process_table[sim_pid].total_sys_time.seconds = 0;

                shared_mem.process_table[sim_pid].last_burst_time.nanoseconds = 0;
                shared_mem.process_table[sim_pid].last_burst_time.seconds = 0;

                shared_mem.process_table[sim_pid].pid = sim_pid;
                shared_mem.process_table[sim_pid].priority = 0;

                // Choose process mode based on a weighted chance
                int process_mode = rand() % 100 > percentChanceIsIO ? CPU_MODE: IO_MODE;

                // Fork and launch child process
                pid_t pid = fork();
                if (pid == 0) {
                    if (launch_child(process_mode, sim_pid) < 0) {
                        printf("Failed to launch process.");
                    }
                }
                // Add some time for generating a process
                add_time(&shared_mem.sys_clock, 2, rand() % 1000);

                last_run_nanoseconds = shared_mem.sys_clock.nanoseconds;
                last_run_seconds = shared_mem.sys_clock.seconds;
            }
        }

        // TODO: HANDLE TIME SCHEDULING FOR CURRENTLY RUNNING CHILDREN

        // See if any processes have returned
        int simulated_pid;
        pid_t pid = waitpid(-1, &simulated_pid, WNOHANG);
		if (pid > 0) {
            // Get the returned simulated pid child exits with
            simulated_pid = WEXITSTATUS(simulated_pid);
            // Clear up this process for future use
            shared_mem.process_table[simulated_pid].pid = 0;
            bit_vec[simulated_pid] = 0;
			shared_mem.process_table_size--;
		} 

        // Simulate some passed time for this loop (1 second and [0, 1000] nanoseconds)
        add_time(&shared_mem.sys_clock, 1, rand() % 1000);
    }

    printf("%ld:%ld\n", shared_mem.sys_clock.seconds, shared_mem.sys_clock.nanoseconds);
    
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
