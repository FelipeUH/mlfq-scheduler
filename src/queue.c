#include <stdio.h>
#include "queue.h"

void queue_init(Queue* q, int quantum) {
    q->front = 0;
    q->rear = 0;
    q->count = 0;
    q->quantum = quantum;
}

int queue_enqueue(Queue* q, Process* p) {
    if (q->count >= MAX_PROCESSES) {
        fprintf(stderr, "Error: cola llena, no se puede encolar proceso %s\n", p->name);
        return 0;
    }
    q->processes[q->rear] = p;
    q->rear = (q->rear + 1) % MAX_PROCESSES;
    q->count++;
    return 1;
}

Process* queue_dequeue(Queue* q) {
    if (q->count == 0) return NULL;
    Process* p = q->processes[q->front];
    q->front = (q->front + 1) % MAX_PROCESSES;
    q->count--;
    return p;
}

int queue_is_empty(const Queue* q) {
    return q->count == 0;
}

int queue_size(const Queue* q) {
    return q->count;
}
