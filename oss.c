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
static char* logfile;
static int log_line = 0;
static int max_secs = -1;
static int running_pid = -1;
static int iters_since_low = 0;
static struct message msg;
static struct time_clock last_run;
static struct Queue queue_high_pri;
static struct Queue queue_low_pri;
static struct Queue queue_blocked;

static int num_io_proc = 0;
static int num_cpu_proc = 0;
static int num_blocked_proc = 0;
static int num_term_proc = 0;

void help();
void signal_handler(int signum);
void init_oss();
void dest_oss();
int launch_child(int mode, int pid);
void try_spawn_child();
void handle_running();
void try_unblock_process();
void try_schedule_process();
void generate_report();
void save_to_log(char* text);

int main(int argc, char** argv) {
    int option;
    logfile = "";
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
    if (logfile[0] == '\0') {
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

    // Clear logfile
    FILE* file_ptr = fopen(logfile, "w");
    fclose(file_ptr);


    // Initialize the OSS shared memory
    init_oss();

    // Keep track of the last time on the sys clock when we run a process
    last_run.nanoseconds = 0;
    last_run.seconds = 0;

    // Main OSS loop. We handle scheduling processes here.
    while (true) {
        // Simulate some passed time for this loop (1 second and [0, 1000] nanoseconds)
        add_time(&(shared_mem->sys_clock), 1, rand() % 1000);
        // try to spawn a new child if enough time has passed
        try_spawn_child();
        // Run currently running process
        handle_running();
        // Remove processes from blocked queue which have become unblocked
        try_unblock_process();
        // Try to schedule processes
        try_schedule_process();

        // See if any child processes have terminated
        int status_pid;
        pid_t pid = waitpid(-1, &status_pid, WNOHANG);
		if (pid > 0) {
            // Get the returned simulated pid child exits with
            status_pid = WEXITSTATUS(status_pid);
            // Clear up this process for future use
            bit_vec[status_pid] = 0;
            shared_mem->process_table[status_pid].pid = 0;
            if (running_pid == status_pid) {
                running_pid = -1;
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

    // Print out final report
    generate_report();

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

void try_spawn_child() {
    if ((shared_mem->sys_clock.seconds - last_run.seconds > maxTimeBetweenNewProcsSecs) && 
    (shared_mem->sys_clock.nanoseconds - last_run.nanoseconds > maxTimeBetweenNewProcsNS)) {
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

            if (process_mode == IO_MODE) {
                queue_insert(&queue_low_pri, sim_pid);
                num_io_proc++;
            }
            else {
                queue_insert(&queue_high_pri, sim_pid);
                num_cpu_proc++;
            } 

            char output[100];
            snprintf(output, 100, "Generating process with PID %d and putting it in queue %d", sim_pid, process_mode);
            save_to_log(output);
            // Fork and launch child process
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
            add_time(&last_run, shared_mem->sys_clock.seconds, shared_mem->sys_clock.nanoseconds);
        }
    }
}

void handle_running() {
    if (running_pid > -1) {
        struct process_ctrl_block* process_block = &shared_mem->process_table[running_pid];
        strncpy(msg.msg_text, "", MSG_BUFFER_LEN);
        msg.msg_type = process_block->actual_pid;
        recieve_msg(&msg, OSS_MSG_SHM, true);
        // Get first command
        char* cmd = strtok(msg.msg_text, " ");

        // terminated sginal sent
        if (strncmp(cmd, "terminated", MSG_BUFFER_LEN) == 0) {
            char output[100];
            snprintf(output, 100, "Process with PID %d terminated. From queue %d", running_pid, process_block->priority);
            save_to_log(output);

            int percent = atoi(strtok(NULL, " "));
            memcpy(&process_block->total_sys_time, &shared_mem->sys_clock, sizeof(struct time_clock));
            int time = (process_block->priority == IO_MODE? 2 : 1) * (percent / 100) * 1000;

            add_time(&process_block->total_cpu_time, 0, time);
            add_time(&process_block->total_sys_time, 0, time);
            add_time(&shared_mem->sys_clock, 0, time);

            running_pid = -1;
            num_term_proc++;
        }
        // blocked signal sent
        else if (strncmp(cmd, "blocked", MSG_BUFFER_LEN) == 0) {
            char output[100];
            snprintf(output, 100, "Process with PID %d blocked. Moving from %d queue to blocked queue", running_pid, process_block->priority);
            save_to_log(output);
            int percent = atoi(strtok(NULL, " "));
            int time = (process_block->priority == IO_MODE? 2 : 1) * (percent / 100) * 1000;

            add_time(&process_block->total_cpu_time, 0, time);
            add_time(&process_block->total_sys_time, 0, time);
            add_time(&shared_mem->sys_clock, 0, time);

            queue_insert(&queue_blocked, process_block->pid);
            running_pid = -1;
            num_blocked_proc++;
        }
        // finished signal sent
        else if (strncmp(cmd, "finished", MSG_BUFFER_LEN) == 0) {
            char output[100];
            snprintf(output, 100, "Process with PID %d finished. From queue %d", running_pid, process_block->priority);
            save_to_log(output);
            int percent = 100;
            int time = (process_block->priority == IO_MODE? 2 : 1) * (percent / 100) * 1000;

            add_time(&process_block->total_cpu_time, 0, time);
            add_time(&process_block->total_sys_time, 0, time);
            add_time(&shared_mem->sys_clock, 0, time);

            snprintf(output, 100, "Queuing process with PID %d for next run on queue %d", running_pid, process_block->priority);
            save_to_log(output);
            // Queue for another run
            if (process_block->priority == IO_MODE) {
                queue_insert(&queue_low_pri, running_pid);
            }
            else {
                queue_insert(&queue_high_pri, running_pid);
            }

            running_pid = -1;
        }
    }
}

void try_unblock_process() {
    if (running_pid > -1) return;

    struct process_ctrl_block* process_block = NULL;
    // Unblock a process if we have unblocked processes in the blocked queue
    if (!queue_is_empty(&queue_blocked)) {
        for (int i = 0; i < queue_blocked.size; i++) {
            int pid = queue_pop(&queue_blocked);
            strncpy(msg.msg_text, "", MSG_BUFFER_LEN);
            msg.msg_type = shared_mem->process_table[pid].actual_pid;
            recieve_msg(&msg, OSS_MSG_SHM, false);

            if (strncmp(msg.msg_text, "unblocked", MSG_BUFFER_LEN) == 0) {
                process_block = &shared_mem->process_table[pid];
                char output[100];
                snprintf(output, 100, "Process with PID %d unblocked. Moving from blocked queue to queue %d", pid, process_block->priority);
                save_to_log(output);
                if (process_block->priority == IO_MODE) {
                    queue_insert(&queue_low_pri, pid);
                }
                else {
                    queue_insert(&queue_high_pri, pid);
                }
            }
            else {
                queue_insert(&queue_blocked, pid);
            }
        }
    }
}

void try_schedule_process() {
    if (running_pid > -1) return;

    // Run low priority every 5 iterations
    if (iters_since_low > 3 && !queue_is_empty(&queue_low_pri)) {
        running_pid = queue_pop(&queue_low_pri);
        iters_since_low = 0;
    }
    else if (!queue_is_empty(&queue_high_pri)) {
        running_pid = queue_pop(&queue_high_pri);
        iters_since_low++;
    }

    if (running_pid < 0) return;
    snprintf(msg.msg_text, MSG_BUFFER_LEN, "run %d %d", 0, rand() % 1000);
    msg.msg_type = shared_mem->process_table[running_pid].actual_pid;
    send_msg(&msg, CHILD_MSG_SHM, false);

    char output[100];
    snprintf(output, 100, "Process with PID %d selected to run from queue %d", running_pid, iters_since_low == 3? IO_MODE : CPU_MODE);
    save_to_log(output);
}

void generate_report() {
    printf("\nSUMMARY\n");
	printf("\tI/O Bound processes: %d\n", num_io_proc);
	printf("\tCPU Bound processes: %d\n", num_cpu_proc);
	printf("\tBlocked processes: %d\n", num_blocked_proc);
	printf("\tTerminated processes: %d\n", num_term_proc);

	
	printf("TIME SPENT TOTAL\n");
	printf("\tSystem Time: %ld:%ld\n", shared_mem->sys_clock.seconds, shared_mem->sys_clock.nanoseconds);
	
    printf("PROCESS TIME\n");
    struct process_ctrl_block* proc = NULL;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        proc = &shared_mem->process_table[i];
        printf("\tPID: %d|CPU TIME: %ld.%ld|LAST BURST: %ld.%ld|SYS TIME: %ld.%ld|PRIORITY: %d\n", 
                proc->pid, proc->total_cpu_time.seconds, proc->total_cpu_time.nanoseconds,
                proc->last_burst_time.seconds, proc->last_burst_time.nanoseconds,
                proc->total_sys_time.seconds, proc->total_sys_time.nanoseconds, proc->priority
            );
    }
}

void save_to_log(char* text) {
	FILE* file_log = fopen(logfile, "a+");
    log_line++;

    // Make sure file is opened
	if (file_log == NULL) {
		perror("Could not open logfile");
        return;
	}

    fprintf(file_log, "%s: %ld.%ld: %s\n", exe_name, shared_mem->sys_clock.seconds, shared_mem->sys_clock.nanoseconds, text);

    fclose(file_log);
}