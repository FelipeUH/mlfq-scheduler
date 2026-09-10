#include <stdio.h>
#include "scheduler.h"

static const int DEFAULT_QUANTUMS[NUM_QUEUES] = {2, 4, 8};

void scheduler_init(Scheduler* s, int boost_interval) {
    for (int i = 0; i < NUM_QUEUES; i++) {
        queue_init(&s->queues[i], DEFAULT_QUANTUMS[i]);
    }
    s->current_cycle = 0;
    s->boost_interval = boost_interval;
    s->logger = NULL;
}

void scheduler_set_logger(Scheduler* s, Logger* logger) {
    s->logger = logger;
}

int scheduler_add_process(Scheduler* s, Process* p) {
    p->current_queue = 0;
    p->state = STATE_READY;
    p->quantum_remaining = s->queues[0].quantum;
    logger_log(s->logger, s->current_cycle, "READY",
               "process=%s queue=Q0 quantum=%d", p->name, p->quantum_remaining);
    return queue_enqueue(&s->queues[0], p);
}

/* Elige el proceso de la cola de mayor prioridad no vacía (Strategy de selección) */
Process* scheduler_get_next(Scheduler* s) {
    for (int i = 0; i < NUM_QUEUES; i++) {
        if (!queue_is_empty(&s->queues[i])) {
            Process* p = queue_dequeue(&s->queues[i]);
            logger_log(s->logger, s->current_cycle, "DISPATCH",
                       "process=%s queue=Q%d quantum_left=%d",
                       p->name, i, p->quantum_remaining);
            return p;
        }
    }
    return NULL;
}

/* Eleva todos los procesos pendientes a la cola de mayor prioridad (anti-starvation) */
int scheduler_boost(Scheduler* s) {
    for (int i = 1; i < NUM_QUEUES; i++) {
        while (!queue_is_empty(&s->queues[i])) {
            Process* p = queue_dequeue(&s->queues[i]);
            p->current_queue = 0;
            p->quantum_remaining = s->queues[0].quantum;
            logger_log(s->logger, s->current_cycle, "BOOST_MOVE",
                       "process=%s Q%d->Q0 quantum=%d", p->name, i,
                       p->quantum_remaining);
            if (!queue_enqueue(&s->queues[0], p)) return 0;
        }
    }
    return 1;
}

/* Si el proceso está en la última cola, permanece ahí (no se puede demover más) */
int scheduler_demote(Scheduler* s, Process* p) {
    int previous_queue = p->current_queue;
    if (p->current_queue < NUM_QUEUES - 1) {
        p->current_queue++;
    }
    p->state = STATE_READY;
    p->quantum_remaining = s->queues[p->current_queue].quantum;
    logger_log(s->logger, s->current_cycle, "DEMOTE",
               "process=%s Q%d->Q%d quantum=%d", p->name, previous_queue,
               p->current_queue, p->quantum_remaining);
    return queue_enqueue(&s->queues[p->current_queue], p);
}

/* Una interrupción no reinicia el quantum ya consumido por el proceso. */
int scheduler_requeue(Scheduler* s, Process* p) {
    p->state = STATE_READY;
    logger_log(s->logger, s->current_cycle, "PREEMPT_REQUEUE",
               "process=%s queue=Q%d quantum_left=%d", p->name,
               p->current_queue, p->quantum_remaining);
    return queue_enqueue(&s->queues[p->current_queue], p);
}

int scheduler_has_higher_priority_ready(const Scheduler* s, int queue_level) {
    for (int i = 0; i < queue_level; i++) {
        if (!queue_is_empty(&s->queues[i])) return 1;
    }
    return 0;
}

int scheduler_quantum(Scheduler* s, int queue_level) {
    return s->queues[queue_level].quantum;
}
