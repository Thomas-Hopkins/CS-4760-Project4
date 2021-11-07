#include <stdbool.h>
#include <stdlib.h>

#include "queue.h"

void queue_init(struct Queue* queue) {
    queue->front_ind = 0;
    queue->rear_ind = -1;
    queue->size = 0;
}

int queue_pop(struct Queue* queue) {
    int element = queue->elements[queue->front_ind];
    queue->front_ind++;

    if (queue->front_ind == MAX_ELEMENTS) {
        queue->front_ind = 0;
    }

    queue->size--;
    return element;
}

int queue_peek(struct Queue* queue) {
    return queue->elements[queue->front_ind];
}

void queue_insert(struct Queue* queue, int element) {
    if (!queue_is_full(queue)) {
        if (queue->rear_ind == MAX_ELEMENTS - 1) {
            queue->rear_ind = -1;
        }

        queue->rear_ind++;
        queue->elements[queue->rear_ind] = element;
        queue->size++;
    }
}

bool queue_is_full(struct Queue* queue) {
    return queue->size == MAX_ELEMENTS;
}

bool queue_is_empty(struct Queue* queue) {
    return queue->size == 0;
}

void queue_print(struct Queue* queue) {
    printf("[");
    for (int i = queue->front_ind; i < queue->rear_ind; i++){
        printf("%d, ", queue->elements[i]);
    }
    printf("]");
}
