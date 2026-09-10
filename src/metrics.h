#ifndef METRICS_H
#define METRICS_H

#include "process.h"

typedef struct {
    int response_time;
    int turnaround_time;
    int waiting_time;
} Metrics;

int metrics_can_calculate(Process* p);
Metrics metrics_calculate(Process* p);

#endif
