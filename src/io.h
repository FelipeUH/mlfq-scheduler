#ifndef IO_H
#define IO_H

#include "process.h"

#define MAX_NAME_LEN 16

Process** io_read_processes(const char* filename, int* count);
int io_write_results(const char* filename, Process** processes, int count);

#endif
