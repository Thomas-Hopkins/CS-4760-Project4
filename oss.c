#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <wait.h>
#include <string.h>

#include "queue.h"
#include "shared.h"
#include "config.h"

short bit_vec[MAX_PROCESSES];
extern struct oss_shm* shared_mem;
static char* exe_name;
static int max_secs = -1;
static struct Queue queue_high_pri;
static struct Queue queue_low_pri;
static struct Queue queue_blocked;

void help();
void signal_handler(int signum);
void init_oss();
void dest_oss();
int launch_child(int mode, int pid);

int main(int argc, char** argv) {
    int option;
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
    if (strncmp(logfile, "", 1)) {
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
    printf("%ld:%ld\n", shared_mem->sys_clock.seconds, shared_mem->sys_clock.nanoseconds);

    // Keep track of the last time on the sys clock when we run a process
    unsigned long last_run_seconds = 0;
    unsigned long last_run_nanoseconds = 0;
    unsigned long last_low_run_seconds = 0;

    // Main OSS loop. We handle scheduling processes here.
    while (true) {
        // Simulate some passed time for this loop (1 second and [0, 1000] nanoseconds)
        add_time(&(shared_mem->sys_clock), 1, rand() % 1000);
        // Check if enough time has passed to spawn a new process
        if ((shared_mem->sys_clock.seconds - last_run_seconds > maxTimeBetweenNewProcsSecs) && 
        (shared_mem->sys_clock.nanoseconds - last_run_nanoseconds > maxTimeBetweenNewProcsNS)) {
            // Check process control block availablity
            if (shared_mem->process_table_size < MAX_PROCESSES) {
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
                shared_mem->process_table_size++;

                shared_mem->process_table[sim_pid].total_cpu_time.nanoseconds = 0;
                shared_mem->process_table[sim_pid].total_cpu_time.seconds = 0;
            
                shared_mem->process_table[sim_pid].total_sys_time.nanoseconds = 0;
                shared_mem->process_table[sim_pid].total_sys_time.seconds = 0;

                shared_mem->process_table[sim_pid].last_burst_time.nanoseconds = 0;
                shared_mem->process_table[sim_pid].last_burst_time.seconds = 0;

                shared_mem->process_table[sim_pid].pid = sim_pid;

                // Choose process mode based on a weighted chance
                int process_mode = rand() % 100 > percentChanceIsIO ? CPU_MODE: IO_MODE;
                shared_mem->process_table[sim_pid].priority = process_mode;

                // Insert processes into queue based on priority
                if (process_mode == CPU_MODE) {
                    queue_insert(&queue_high_pri, sim_pid);
                }
                else {
                    queue_insert(&queue_low_pri, sim_pid);
                }

                // Fork and launch child process
                printf("Creating process of sim id: %d\n", sim_pid);
                pid_t pid = fork();
                if (pid == 0) {
                    if (launch_child(process_mode, sim_pid) < 0) {
                        printf("Failed to launch process.\n");
                    }
                } 
                else {
                    // Update it's actual pid
                    shared_mem->process_table[sim_pid].actual_pid = pid;
                }
                // Add some time for generating a process
                add_time(&shared_mem->sys_clock, 2, rand() % 1000);

                last_run_nanoseconds = shared_mem->sys_clock.nanoseconds;
                last_run_seconds = shared_mem->sys_clock.seconds;
            }
        }

        struct message msg;
        strncpy(msg.msg_text, "", MSG_BUFFER_LEN);

        // Select from low priority queue if it is not empty and we've passed 5 seconds since last 
        if (!queue_is_empty(&queue_low_pri) && (last_run_seconds - last_low_run_seconds) > 5 ) {
            last_low_run_seconds = shared_mem->sys_clock.seconds;
            msg.msg_type = queue_pop(&queue_low_pri);
        }
        // Select from high priority if not empty
        else if (!queue_is_empty(&queue_high_pri)) {
            msg.msg_type = queue_pop(&queue_high_pri);
        }
        else {
            continue;
        }

        int sim_pid;
        recieve_msg(&msg, OSS_MSG_SHM, false);

        char* cmd = strtok(msg.msg_text, " ");
        if (cmd != NULL) {
            if (strncmp(cmd, "ready", MSG_BUFFER_LEN) == 0) {
                sim_pid = atoi(strtok(NULL, " "));
                sprintf(msg.msg_text, "run %d %d %d", sim_pid, 0, rand() % 10000000);
                send_msg(&msg, CHILD_MSG_SHM, false);
            }
            else if (strncmp(cmd, "finished", MSG_BUFFER_LEN) == 0) {
                sim_pid = atoi(strtok(NULL, " "));
                bit_vec[sim_pid] = 0;
                shared_mem->process_table_size--;
                printf("Program %d has finished.\n", sim_pid);
            }
            else if (strncmp(cmd, "terminated", MSG_BUFFER_LEN) == 0) {
                sim_pid = atoi(strtok(NULL, " "));
                bit_vec[sim_pid] = 0;
                shared_mem->process_table_size--;
                printf("Program %d has terminated.\n", sim_pid);
            }
            else if (strncmp(cmd, "blocked", MSG_BUFFER_LEN) == 0) {
                sim_pid = atoi(strtok(NULL, " "));
                queue_insert(&queue_blocked, sim_pid);
                printf("a program is blocked!\n");
            }
        }

        // Run next child in queue

        // See if any processes have returned
        pid_t pid = waitpid(-1, &sim_pid, WNOHANG);
		if (pid > 0) {
            // Get the returned simulated pid child exits with
            sim_pid = WEXITSTATUS(sim_pid);
            // Clear up this process for future use
            if (bit_vec[sim_pid] != 0) {
                printf("WARNING: process %d did not cleanup.", sim_pid);
            }
		} 
    }
    dest_oss();
    exit(EXIT_SUCCESS);
}

void help() {
    printf("Operating System Simulator usage\n");
	printf("\n");
	printf("[-h]\tShow this help dialogue.\n");
	printf("-s sec\tSet the maximum runtime before system terminates (Required).\n");
	printf("-l filename\tSet the log file name (Required).\n");
	printf("\n");
}

void signal_handler(int signum) {
    // Issue messages
	if (signum == SIGINT) {
		fprintf(stderr, "\nRecieved SIGINT signal interrupt, terminating children.\n");
	}
	else if (signum == SIGALRM) {
		fprintf(stderr, "\nProcess execution timeout. Failed to finish in %d seconds.\n", max_secs);
	}

    // Kill active children
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (bit_vec[i] == 1) {
            pid_t pid = shared_mem->process_table[i].actual_pid;
            kill(pid, SIGKILL);
        }
    }

    // Cleanup oss shared memory
    dest_oss();

    if (signum == SIGINT) exit(EXIT_SUCCESS);
	if (signum == SIGALRM) exit(EXIT_FAILURE);
}

void init_oss() {
    // Initialize random number gen
    srand((int)time(NULL) + getpid());

    // Attach to shared memory.
    init_shm();
    shared_mem->sys_clock.seconds = 0;
    shared_mem->sys_clock.nanoseconds = 0;
    shared_mem->process_table_size = 0;

    // Initialize bit vector
    for (int i = 0; i < MAX_PROCESSES; i++) {
        bit_vec[i] = 0;
    }

    // Setup queues
    queue_init(&queue_high_pri);
    queue_init(&queue_low_pri);
    queue_init(&queue_blocked);

    // Setup signal handlers
	signal(SIGINT, signal_handler);
	signal(SIGALRM, signal_handler);

	// Terminate in max_secs	
	alarm(max_secs);

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
