#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/shm.h>

#include "shared.h"

struct oss_shm* shared_mem = NULL;
static int oss_msg_queue;
static int child_msg_queue;

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
	key_t oss_msg_key = ftok(SHM_FILE, OSS_MSG_SHM);
	key_t child_msg_key = ftok(SHM_FILE, CHILD_MSG_SHM);

	if (oss_msg_key < 0 || child_msg_key < 0) {
        printf("Could not get message queue(s) file.");
	}

	if (create) {
		oss_msg_queue = msgget(oss_msg_key, 0644 | IPC_CREAT);
		child_msg_queue = msgget(child_msg_key, 0644 | IPC_CREAT);
	}
	else {
		oss_msg_queue = msgget(oss_msg_key, 0644 | IPC_EXCL);
		child_msg_queue = msgget(child_msg_key, 0644 | IPC_EXCL);
	}

	if (oss_msg_queue < 0 || child_msg_queue < 0) {
        printf("Could not attach to message queue(s).");
	}
}

// Public function to destruct message queue
void dest_msg() {
	if (msgctl(oss_msg_queue, IPC_RMID, NULL) < 0) {
		printf("Could not detach message queue.");
	}
	if (msgctl(child_msg_queue, IPC_RMID, NULL) < 0) {
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

bool sub_time(struct time_clock* Time, unsigned long seconds, unsigned long nanoseconds) {
	// If subtracting more nanoseconds then is on the clock
	if (nanoseconds > Time->nanoseconds) {
		// See if we can borrow a second
		if (Time->seconds < 1) return false;
		// Borrow a second
		Time->nanoseconds += 1000000000;
		Time->seconds -= 1;
	}
	// Subtract the nanoseconds
	Time->nanoseconds -= nanoseconds;

	// If subtracting more seconds than we have on clock
	if (seconds > Time->seconds) {
		// Add back the nanoseconds we took
		add_time(Time, 0, nanoseconds);
		return false;
	}
	// Subtract the seconds
	Time->seconds -= seconds;
	return true;
}

int recieve_msg(struct message* msg, int msg_queue, bool wait) {
	int msg_queue_id;
	if (msg_queue == OSS_MSG_SHM) {
		msg_queue_id = oss_msg_queue;
	}
	else if (msg_queue == CHILD_MSG_SHM) {
		msg_queue_id = child_msg_queue;
	}
	else {
		printf("Got unexpected message queue ID of %d\n", msg_queue);
	}
	return msgrcv(msg_queue_id, msg, sizeof(struct message), msg->msg_type, wait ? 0 : IPC_NOWAIT);
}

int send_msg(struct message* msg, int msg_queue, bool wait) {
	int msg_queue_id;
	if (msg_queue == OSS_MSG_SHM) {
		msg_queue_id = oss_msg_queue;
	}
	else if (msg_queue == CHILD_MSG_SHM) {
		msg_queue_id = child_msg_queue;
	}
	else {
		printf("Got unexpected message queue ID of %d\n", msg_queue);
	}
	return msgsnd(msg_queue_id, msg, sizeof(struct message), wait ? 0 : IPC_NOWAIT);
}