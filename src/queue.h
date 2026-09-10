#ifndef QUEUE_H
#define QUEUE_H

#include "process.h"

#define MAX_PROCESSES 100

typedef struct {
    Process* processes[MAX_PROCESSES];
    int front;
    int rear;
    int count;
    int quantum;
} Queue;

void queue_init(Queue* q, int quantum);
int queue_enqueue(Queue* q, Process* p);
Process* queue_dequeue(Queue* q);
int queue_is_empty(const Queue* q);
int queue_size(const Queue* q);

#endif
