#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "queue.h"
#include "logger.h"

#define NUM_QUEUES 3

typedef struct {
    Queue queues[NUM_QUEUES];
    int current_cycle;
    int boost_interval;
    Logger* logger;
} Scheduler;

void scheduler_init(Scheduler* s, int boost_interval);
void scheduler_set_logger(Scheduler* s, Logger* logger);
int scheduler_add_process(Scheduler* s, Process* p);
Process* scheduler_get_next(Scheduler* s);
int scheduler_boost(Scheduler* s);
int scheduler_demote(Scheduler* s, Process* p);
int scheduler_requeue(Scheduler* s, Process* p);
int scheduler_has_higher_priority_ready(const Scheduler* s, int queue_level);
int scheduler_quantum(Scheduler* s, int queue_level);

#endif
