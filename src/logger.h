#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

typedef struct {
    FILE* file;
    int failed;
} Logger;

int logger_init(Logger* logger, const char* filename);
void logger_log(Logger* logger, int cycle, const char* event, const char* format, ...);
int logger_close(Logger* logger);
int logger_has_error(const Logger* logger);

#endif
