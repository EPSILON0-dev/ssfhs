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
#include <time.h>
#include "ssfhs.h"

static char date_buffer[64];

static void log_generate_date()
{
    // Generate the date header
    time_t now = time(NULL);
    struct tm *gmt = gmtime(&now);
    strftime(date_buffer, sizeof(date_buffer) - 1, "[%a, %d %b %Y %H:%M:%S] ", gmt);
}

void log_error(const char *msg)
{
    log_generate_date();

    // Print to the terminal
    fputs(date_buffer, stderr);
    fputs(msg, stderr);

    // Log to the file
    if (g_server_config.log_file)
    {
        FILE *f = fopen(g_server_config.log_file, "a");
        fputs(date_buffer, f);
        fputs("STDERR: ", f);
        fputs(msg, f);
        fclose(f);
    }
}

void log_message(const char *msg)
{
    log_generate_date();

    // Print to the terminal
    fputs(date_buffer, stderr);
    fputs(msg, stdout);

    // Log to the file
    if (g_server_config.log_file)
    {
        FILE *f = fopen(g_server_config.log_file, "a");
        fputs(date_buffer, f);
        fputs("STDOUT: ", f);
        fputs(msg, f);
        fclose(f);
    }
}