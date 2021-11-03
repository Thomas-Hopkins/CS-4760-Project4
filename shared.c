#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/shm.h>

#include "shared.h"

struct oss_shm* shared_mem = NULL; 

// Private function to get shared memory key
int get_shm() {
	key_t key;

	// Get numeric key of shared memory file
	key = ftok(SHM_FILE, 0);
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

// Public function to initialize shared memory
void init_shm() {
    int mem_id = get_shm();
    if (mem_id < 0) {
        printf("Could not get shared memory file.");
    }
    if (attach_shm(mem_id) < 0) {
        printf("Could not attach to shared memory.");
    }
}

// Public function to destruct shared memory
void dest_shm() {
	int mem_id = get_shm();

	if (mem_id < 0) {
        printf("Could not get shared memory file.");
    }
	if (shmctl(mem_id, IPC_RMID, NULL) < 0) {
        printf("Could not remove shared memory.");
	}

	shared_mem = NULL;
}
