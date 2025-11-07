/**
 * @file args.c
 * @author epsiii
 * @brief Command line argument parsing for SSFHS
 * @date 2025-10-29
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "ssfhs.h"

static void print_help_and_exit(const char *prog_name)
{
    printf("Usage: %s [options]\n", prog_name);
    printf("Options:\n");
    printf("  -h, --help                    Show this help message and exit.\n");
    printf("  -p, --port [PORT]             Port to listen for connections (default: 8080).\n");
    printf("  -d, --root-dir [DIR]          Root directory with the server files (required).\n");
    printf("  -l, --log-file [FILE]         File for storing logs (default: ssfhs.log).\n");
    printf("  -c, --config-file [FILE]      Server config file (required).\n");
    printf("  -d, --debug                   Enable debug logs.\n");

    exit(EXIT_SUCCESS);
}

void cli_args_parse(ServerConfig *config, int argc, const char **argv) 
{
    // Clear the config
    memset(config, 0, sizeof(*config));

    // Set default parameters
    config->debug = false;
    config->port = 8080;
    config->config_file = NULL;
    config->log_file = NULL;

    // Parse the arguments
    int arg_index = 1;
    while (arg_index < argc) 
    {
        const char *arg = argv[arg_index];

        // Help
        if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) 
        {
            print_help_and_exit(argv[0]);
        }

        // Listen port
        else if (strcmp(arg, "--port") == 0 || strcmp(arg, "-p") == 0) 
        {
            if (arg_index == argc - 1)
            {
                fprintf(stderr, "Missing argument for --port\n");
                exit(EXIT_FAILURE);
            }

            int port = atoi(argv[arg_index + 1]);
            if (port <= 0 || port > 65535)
            {
                fprintf(stderr, "Invalid port number: %s\n", argv[arg_index + 1]);
                exit(EXIT_FAILURE);
            }

            config->port = port;
            arg_index += 2;
        }

        // Log file
        else if (strcmp(arg, "--log-file") == 0 || strcmp(arg, "-l") == 0) 
        {
            if (arg_index == argc - 1)
            {
                fprintf(stderr, "Missing argument for --log-file\n");
                exit(EXIT_FAILURE);
            }

            config->log_file = strdup(argv[arg_index + 1]);
            arg_index += 2;
        }

        // Config file
        else if (strcmp(arg, "--config-file") == 0 || strcmp(arg, "-c") == 0) 
        {
            if (arg_index == argc - 1)
            {
                fprintf(stderr, "Missing argument for --config-file\n");
                exit(EXIT_FAILURE);
            }

            config->config_file = strdup(argv[arg_index + 1]);
            arg_index += 2;


            if (access(config->config_file, F_OK) != 0)
            {
                fprintf(stderr, "Could not open config file: %s\n", config->config_file);
                exit(EXIT_FAILURE);
            }
        }

        // Config file
        else if (strcmp(arg, "--root-dir") == 0 || strcmp(arg, "-d") == 0) 
        {
            if (arg_index == argc - 1)
            {
                fprintf(stderr, "Missing argument for --root-dir\n");
                exit(EXIT_FAILURE);
            }

            config->root_dir = realpath(argv[arg_index + 1], NULL);
            if (!config->root_dir)
            {
                fprintf(stderr, "Could not resolve root directory: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            arg_index += 2;
        }

        // Debug
        else if (strcmp(arg, "--debug") == 0 || strcmp(arg, "-v") == 0) 
        {
            config->debug = true;
            arg_index += 1;
        }

        else 
        {
            fprintf(stderr, "Unknown argument: %s\n", arg);
            exit(EXIT_FAILURE);
        }
    }
}
