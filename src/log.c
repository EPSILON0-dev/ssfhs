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
#include "ssfhs.h"

static char header_buffer[64];
static int log_request_id;

static void log_generate_header(void)
{
    // Generate the date header
    time_t now = time(NULL);
    struct tm *gmt = gmtime(&now);
    strftime(header_buffer, sizeof(header_buffer) - 1, "[%a, %d %b %Y %H:%M:%S] ", gmt);
    int len = strlen(header_buffer);
    snprintf(&header_buffer[len], 64 - len, "[%d] ", log_request_id);
}

void log_set_request_id(int id)
{
    log_request_id = id;
}

void log_error(const char *msg)
{
    log_generate_header();

    // Print to the terminal
    fputs(header_buffer, stderr);
    fputs(msg, stderr);

    // Log to the file
    if (g_server_config.log_file)
    {
        FILE *f = fopen(g_server_config.log_file, "a");
        fputs(header_buffer, f);
        fputs("STDERR: ", f);
        fputs(msg, f);
        fclose(f);
    }
}

void log_message(const char *msg)
{
    log_generate_header();

    // Print to the terminal
    fputs(header_buffer, stderr);
    fputs(msg, stdout);

    // Log to the file
    if (g_server_config.log_file)
    {
        FILE *f = fopen(g_server_config.log_file, "a");
        fputs(header_buffer, f);
        fputs("STDOUT: ", f);
        fputs(msg, f);
        fclose(f);
    }
}