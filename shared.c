#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/msg.h>
#include <sys/shm.h>

#include "shared.h"

struct oss_shm* shared_mem = NULL;
static int message_queue = -1;

// Private function to get shared memory key
int get_shm(int token) {
	key_t key;

	// Get numeric key of shared memory file
	key = ftok(SHM_FILE, token);
	if (key == -1) {
		return -1;
	}

	// Get shared memory id from the key
	return shmget(key, sizeof(struct oss_shm), 0644 | IPC_CREAT);
}

// Private function to attach shared memory
int attach_shm(int mem_id) {
	if (mem_id == -1) {
		return -1;
	}

	// Attach shared memory to struct pointer
	shared_mem = shmat(mem_id, NULL, 0);
	if (shared_mem == NULL ) {
		return -1;
	}
	return 0;
}

// Public function to initialize message queues
void init_msg(bool create) {
	key_t msg_key = ftok(SHM_FILE, MSG_SHM);

	if (msg_key < 0) {
        printf("Could not get message queue file.");
	}

	if (create) {
		message_queue = msgget(msg_key, 0644 | IPC_CREAT);
	}
	else {
		message_queue = msgget(msg_key, 0644 | IPC_EXCL);
	}

	if (message_queue < 0) {
        printf("Could not attach to message queue.");
	}
}

// Public function to destruct message queue
void dest_msg() {
	if (msgctl(message_queue, IPC_RMID, NULL) < 0) {
		printf("Could not detach message queue.");
	} 
}

// Public function to initialize shared memory
void init_shm() {
    int mem_id = get_shm(OSS_SHM);
    if (mem_id < 0) {
        printf("Could not get shared memory file.");
    }
    if (attach_shm(mem_id) < 0) {
        printf("Could not attach to shared memory.");
    }
}

// Public function to destruct shared memory
void dest_shm() {
	int mem_id = get_shm(OSS_SHM);

	if (mem_id < 0) {
        printf("Could not get shared memory file.");
    }
	if (shmctl(mem_id, IPC_RMID, NULL) < 0) {
        printf("Could not remove shared memory.");
	}

	shared_mem = NULL;
}

void add_time(struct time_clock* Time, unsigned long seconds, unsigned long nanoseconds) {
	Time->seconds += seconds;
	Time->nanoseconds += nanoseconds;
	if (Time->nanoseconds > 1000000000)
	{
		Time->seconds += 1;
		Time->nanoseconds -= 1000000000;
	}
}
