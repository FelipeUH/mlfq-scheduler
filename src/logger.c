#include <stdarg.h>
#include <stdio.h>
#include "logger.h"

int logger_init(Logger* logger, const char* filename) {
    logger->file = fopen(filename, "w");
    logger->failed = logger->file == NULL;
    if (logger->failed) {
        fprintf(stderr, "Error: no se pudo crear el archivo de log '%s'\n", filename);
        return 0;
    }

    if (fprintf(logger->file, "# MLFQ simulation trace\n") < 0) {
        logger->failed = 1;
        fclose(logger->file);
        logger->file = NULL;
    }
    return !logger->failed;
}

void logger_log(Logger* logger, int cycle, const char* event, const char* format, ...) {
    va_list args;

    if (!logger || !logger->file || logger->failed) return;
    if (fprintf(logger->file, "[cycle=%d] %-16s ", cycle, event) < 0) {
        logger->failed = 1;
        return;
    }

    va_start(args, format);
    if (vfprintf(logger->file, format, args) < 0) logger->failed = 1;
    va_end(args);

    if (!logger->failed && fputc('\n', logger->file) == EOF) logger->failed = 1;
}

int logger_close(Logger* logger) {
    int result = 1;
    int flush_result;
    int close_result;

    if (!logger || !logger->file) return logger ? !logger->failed : 1;
    flush_result = fflush(logger->file);
    close_result = fclose(logger->file);
    if (flush_result != 0 || close_result != 0) result = 0;
    logger->file = NULL;
    if (!result) logger->failed = 1;
    return !logger->failed;
}

int logger_has_error(const Logger* logger) {
    return logger && logger->failed;
}
