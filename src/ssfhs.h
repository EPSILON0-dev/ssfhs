/**
 * @file ssfhs.h
 * @author epsiii
 * @brief Header file for SSFHS project (all types, structures and function prototypes)
 * @date 2025-10-29
 * 
 * @copyright Copyright (c) 2025
 */
#ifndef SSFHS_H
#define SSFHS_H

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

//////////////////////////////////////////////////////////////////////////////
//                                 Defines                                  //
//////////////////////////////////////////////////////////////////////////////

#define SERVER_NAME "SSFHS (Simple Static File HTTP Server)"
#define SERVER_VERSION "v0.1"

//////////////////////////////////////////////////////////////////////////////
//                            Data Structures                               //
//////////////////////////////////////////////////////////////////////////////

typedef struct {
    char **items;
    size_t count;
} StringArray;

void string_array_init(StringArray *array);
void string_array_add(StringArray *array, const char *item);
char* string_array_get(StringArray *array, size_t index);
void string_array_free(StringArray *array);

typedef struct {
    size_t count;
    size_t max_size;
    char *items;
} CharVector;

void  char_vector_init(CharVector *vec, int initial_size);
void  char_vector_push(CharVector *vec, char c);
void  char_vector_push_arr(CharVector *vec, const char *arr, size_t len);
char  char_vector_get_char(const CharVector *vec, size_t index);
int   char_vector_get(const CharVector *vec, void *dst, size_t index, size_t len);
char* char_vector_get_alloc(const CharVector *vec, size_t index, size_t len);
void  char_vector_free(CharVector *vec);

//////////////////////////////////////////////////////////////////////////////
//                               Utilities                                  //
//////////////////////////////////////////////////////////////////////////////

// Creates a copy of a string with whitespaces trimmed at the beginning and
//  at the end
char *strtrim(const char *str);

//////////////////////////////////////////////////////////////////////////////
//                      CLI Arguments & Config File                         //
//////////////////////////////////////////////////////////////////////////////

typedef struct {
    // Settings coming from the CLI
    uint16_t port;
    bool verbose;
    bool debug;
    char *root_dir;
    char *log_file;
    char *config_file;

    // Settings coming from the config
    StringArray protected_files;
    char *index_page_file;
    char *bad_request_page_file;
    char *forbidden_page_file;
    char *not_found_page_file;
} ServerConfig;

void cli_args_parse(ServerConfig *config, int argc, const char **argv);

void config_load(ServerConfig *config);
void config_free(ServerConfig *config);

//////////////////////////////////////////////////////////////////////////////
//                               Network                                    //
//////////////////////////////////////////////////////////////////////////////

int socket_open(uint16_t port);
int socket_await_connection(int listen_fd);
int socket_send_response(const CharVector *vec, int conn_fd);

//////////////////////////////////////////////////////////////////////////////
//                                 HTTP                                     //
//////////////////////////////////////////////////////////////////////////////

typedef struct {
    bool okay;
    char *method;
    char *url;
    char *version;
    StringArray header_keys;
    StringArray header_values;
    void *data;
    size_t data_len;
} HTTPRequest;

bool http_got_whole_request(const CharVector *vec);
void http_request_init(HTTPRequest *request);
int http_request_parse(const CharVector *vec, HTTPRequest *request);
void http_request_free(HTTPRequest *request);
int http_response_generate(CharVector *response, HTTPRequest *request);

//////////////////////////////////////////////////////////////////////////////
//                              Resource                                    //
//////////////////////////////////////////////////////////////////////////////

char *resource_resolve_url_path(const char *path);
bool resource_is_accessible(const char *path);
bool resource_is_protected(const char *path);
char* resource_get_content_type(const char *path);
int resource_get(void **buff, size_t *buffsz, const char *path);

//////////////////////////////////////////////////////////////////////////////
//                           Global Variables                               //
//////////////////////////////////////////////////////////////////////////////
extern ServerConfig g_server_config;

#endif