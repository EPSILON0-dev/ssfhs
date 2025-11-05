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

static char header_buffer[64];

static void log_generate_header(int conn_id)
{
    // Generate the date header
    time_t now = time(NULL);
    struct tm *gmt = gmtime(&now);
    strftime(header_buffer, sizeof(header_buffer) - 1, "[%a, %d %b %Y %H:%M:%S] ", gmt);
    int len = strlen(header_buffer);
    snprintf(&header_buffer[len], 64 - len, "[%d] ", conn_id);
}

// TODO Semaphore lock the file for print duration
void log_error(int conn_id, const char *format, ...)
{
    log_generate_header(conn_id);

    va_list args;
    
    fputs(header_buffer, stderr);
    fputs("ERR: ", stderr);
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    // Log to the file
    if (g_server_config.log_file)
    {
        FILE *f = fopen(g_server_config.log_file, "a");
        fputs(header_buffer, f);
        fputs("ERR: ", f);
        va_start(args, format);
        vfprintf(f, format, args);
        va_end(args);
        fclose(f);
    }
}

void log_message(int conn_id, const char *format, ...)
{
    log_generate_header(conn_id);

    va_list args;
    fputs(header_buffer, stdout);
    fputs("OUT: ", stdout);
    
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);

    // Log to the file
    if (g_server_config.log_file)
    {
        FILE *f = fopen(g_server_config.log_file, "a");
        fputs(header_buffer, f);
        fputs("OUT: ", f);
        va_start(args, format);
        vfprintf(f, format, args);
        va_end(args);
        fclose(f);
    }

}