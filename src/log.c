/**
 * @file log.c
 * @author epsiii
 * @brief Logging utilities for SSFHS
 * @date 2025-10-31
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include "ssfhs.h"

static FILE *log_file = NULL;

static void log_generate_header(char *log_buffer, int conn_id, bool error)
{
    time_t now = time(NULL);
    struct tm *gmt = gmtime(&now);
    strftime(log_buffer, LOG_BUFFER_SIZE - 1, "[%a, %d %b %Y %H:%M:%S] ", gmt);
    int len = strlen(log_buffer);
    snprintf(&log_buffer[len], LOG_BUFFER_SIZE - len, "[%d] %s: ", conn_id, 
        error ? "ERR" : "OUT");
}

void log_open_file(void)
{
    if (g_server_config.log_file)
    {
        log_file = fopen(g_server_config.log_file, "a");
        if (!log_file)
        {
            fprintf(stderr, "Could not open log file: %s\n", g_server_config.log_file);
            fclose(log_file);
            return;
        }
    }
}

void log_close_file(void)
{
    if (log_file)
    {
        fflush(log_file);
        fclose(log_file);
        log_file = NULL;
    }
}

void log_error(int conn_id, const char *format, ...)
{
    char log_buffer[LOG_BUFFER_SIZE];
    log_generate_header(log_buffer, conn_id, true);

    va_list args;
    va_start(args, format);
    vsnprintf(log_buffer + strlen(log_buffer), LOG_BUFFER_SIZE - strlen(log_buffer), format, args);
    va_end(args);

    fputs(log_buffer, stderr);
    if (g_server_config.log_file)
    {
        fputs(log_buffer, log_file);
    }
}

void log_message(int conn_id, const char *format, ...)
{
    char log_buffer[LOG_BUFFER_SIZE];
    log_generate_header(log_buffer, conn_id, false);

    va_list args;
    va_start(args, format);
    vsnprintf(log_buffer + strlen(log_buffer), LOG_BUFFER_SIZE - strlen(log_buffer), format, args);
    va_end(args);

    fputs(log_buffer, stdout);
    if (g_server_config.log_file)
    {
        fputs(log_buffer, log_file);
    }
}