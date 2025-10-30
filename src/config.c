/**
 * @file config.c
 * @author epsiii
 * @brief Configuration file handling for SSFHS
 * @date 2025-10-29
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ssfhs.h"

static char* config_resolve_path(char *path)
{
    // Get the directiory of the config file
    char *sep = strrchr(g_server_config.config_file, '/');
    if (!sep) { return NULL; }
    int len = sep - g_server_config.config_file + 1;  // +1 for separator
    char *dir = malloc(len + 1);
    memcpy(dir, g_server_config.config_file, len);
    dir[len] = '\0';

    // Concateate and resolve the path
    char *raw_path = malloc(strlen(dir) + strlen(path) + 1);
    memcpy(raw_path, dir, strlen(dir));
    memcpy(raw_path + strlen(dir), path, strlen(path));
    raw_path[strlen(dir) + strlen(path)] = '\0';
    char *real_path = realpath(raw_path, NULL);
    if (!real_path) { return NULL; }

    free(raw_path);
    free(dir);
    return real_path;
}

void config_load(ServerConfig *config) 
{
    // Open the configuration file (at this point we know it exists)
    const char *file_path = (config->config_file) ? config->config_file : "ssfhs.conf";
    FILE *config_file = fopen(file_path, "r");
    if (!config_file)
    {
        fprintf(stderr, "Could not open config file: %s\n", config->config_file);
        exit(EXIT_FAILURE);
    }

    // Initialize config with default values
    string_array_init(&config->protected_files);
    config->not_found_page_file = NULL;

    // Parse the lines
    int line_index = 0;
    for ( ;; )
    {
        ++line_index;
        bool unknown_token_error = false;

        // Get the line
        char *line = NULL;
        size_t _ilen = 0;
        int line_len = getline(&line, &_ilen, config_file);
        if (line_len == -1)
        {
            free(line);
            break;
        }

        // Continue if the line is empty or is a comment
        if (line[0] == '#' || line[0] == '\n')
        {
            free(line);
            continue;
        }

        // Get the line up to the '=' separator
        char *sep_ptr = strchr(line, '=');
        if (sep_ptr == NULL)
        {
            free(line);
            fprintf(stderr, "Invalid config at line: %d\n", line_index);
            exit(EXIT_FAILURE);
        }
        int key_len = (sep_ptr - line);
        char *key = malloc(key_len + 1);
        strncpy(key, line, key_len);
        key[key_len] = '\0';

        // Create a copy of the value after the token
        //  -1 for the seperator
        int val_len = line_len - key_len - 1;
        // If we have a newline also -1
        if (sep_ptr[val_len] == '\n') { val_len--; }
        // +1 for the separator
        char *val_ptr = sep_ptr + 1;
        char *value = malloc(val_len + 1);
        value[val_len] = '\0';
        strncpy(value, val_ptr, val_len);

        // Parse the key
        if (strcmp(key, "PROTECTED") == 0)
        {
            char *full_path = config_resolve_path(value);
            if (full_path)
            {
                string_array_add(&config->protected_files, full_path);
                if (config->debug)
                {
                    printf("[CONFIG] Registered PROTECTED file: %s\n", value);
                }
                free(full_path);
            }
            else
            {
                fprintf(stderr, "Failed to resolve path: %s (ignored)\n", value);
            }
        }

        else if (strcmp(key, "400_PAGE") == 0)
        {
            config->bad_request_page_file = config_resolve_path(value);
            if (config->debug)
            {
                printf("[CONFIG] 400 Page path set to: %s\n", value);
            }
        }

        else if (strcmp(key, "403_PAGE") == 0)
        {
            config->forbidden_page_file = config_resolve_path(value);
            if (config->debug)
            {
                printf("[CONFIG] 403 Page path set to: %s\n", value);
            }
        }

        else if (strcmp(key, "404_PAGE") == 0)
        {
            config->not_found_page_file = config_resolve_path(value);
            if (config->debug)
            {
                printf("[CONFIG] 404 Page path set to: %s\n", value);
            }
        }

        else if (strcmp(key, "INDEX_PAGE") == 0)
        {
            config->index_page_file = config_resolve_path(value);
            if (config->debug)
            {
                printf("[CONFIG] Index Page path set to: %s\n", value);
            }
        }

        else
        {
            unknown_token_error = true;
        }

        // Release the resources
        free(key);
        free(value);
        free(line);

        if (unknown_token_error)
        {
            fprintf(stderr, "Invalid config file TOKEN one line: %d\n", line_index);
            exit(EXIT_FAILURE);
        }
    }

    // Print debug info
    if (config->debug)
    {
        printf("[CONFIG] Config loaded!\n");
        printf("    Log file: %s\n", config->log_file);
        printf("    Root dir: %s\n", config->root_dir);
        printf("    Config file: %s\n", config->config_file);
        printf("    Index page: %s\n", config->index_page_file);
        printf("    400 page: %s\n", config->bad_request_page_file);
        printf("    403 page: %s\n", config->forbidden_page_file);
        printf("    404 page: %s\n", config->not_found_page_file);
        printf("    Protected files: %ld\n", config->protected_files.count);
    }
}

void config_free(ServerConfig *config)
{
    // CLI stuff
    if (config->root_dir) { free(config->root_dir); }
    if (config->log_file) { free(config->log_file); }
    if (config->config_file) { free(config->config_file); }

    // Config file stuff
    if (config->index_page_file) { free(config->index_page_file); }
    if (config->bad_request_page_file) { free(config->bad_request_page_file); }
    if (config->forbidden_page_file) { free(config->forbidden_page_file); }
    if (config->not_found_page_file) { free(config->not_found_page_file); }
    string_array_free(&config->protected_files);
}