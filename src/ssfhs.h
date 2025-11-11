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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <arpa/inet.h>

//////////////////////////////////////////////////////////////////////////////
//                                 Defines                                  //
//////////////////////////////////////////////////////////////////////////////

#define SERVER_NAME "SSFHS"
#define SERVER_VERSION "v0.1"

#define DYNAMIC_TAG "sshfs-dyn"

#define RECEIVE_POLL_GRANULARITY_MS    50     // ms
#define SUBPROCESS_POLL_GRANULARITY_MS 5      // ms
#define DEFAULT_REQUEST_TIMEOUT        5000   // ms
#define DEFAULT_DYNAMIC_TIMEOUT        100    // ms
#define LOG_BUFFER_SIZE                8192   // bytes

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

// Returns current time in milliseconds (used for timeouts)
uint64_t now_ms(void);

// Returns current time in microseconds (used for time measurements)
uint64_t now_us(void);

//////////////////////////////////////////////////////////////////////////////
//                      CLI Arguments & Config File                         //
//////////////////////////////////////////////////////////////////////////////

typedef struct {
    // Settings coming from the CLI
    uint16_t port;
    bool debug;
    char *root_dir;
    char *log_file;
    char *config_file;

    // Settings coming from the config
    StringArray protected_files;
    StringArray dynamic_files;
    char *index_page_file;
    char *bad_request_page_file;
    char *forbidden_page_file;
    char *not_found_page_file;
    char *server_error_page_file;
    int request_timeout_ms;
    int dynamic_timeout;
    bool ignore_dynamic_errors;
} ServerConfig;

void cli_args_parse(ServerConfig *config, int argc, const char **argv);

void config_load(ServerConfig *config);
void config_free(ServerConfig *config);

//////////////////////////////////////////////////////////////////////////////
//                               Logging                                    //
//////////////////////////////////////////////////////////////////////////////

void log_open_file(void);
void log_close_file(void);
void log_error(int conn_id, const char *format, ...);
void log_message(int conn_id, const char *format, ...);

//////////////////////////////////////////////////////////////////////////////
//                               Network                                    //
//////////////////////////////////////////////////////////////////////////////

typedef struct {
    uint64_t start_us;
    int conn_fd;
    int conn_id;
    struct sockaddr_storage cliaddr;
} ConnectionDescriptor;

int socket_open(uint16_t port);
int socket_accept_connection(int listen_fd);

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
int http_response_generate(int request_id, CharVector *response, HTTPRequest *request, const char *request_str);

//////////////////////////////////////////////////////////////////////////////
//                              Resource                                    //
//////////////////////////////////////////////////////////////////////////////

char *resource_resolve_url_path(const char *path);
bool resource_is_accessible(const char *path);
bool resource_is_protected(const char *path);
bool resource_is_dynamic(const char *path);
char* resource_get_content_type(const char *path);
int resource_get(int request_id, void **buff, size_t *buffsz, const char *path, const char *request_str);

//////////////////////////////////////////////////////////////////////////////
//                          Dynamic Resource                                //
//////////////////////////////////////////////////////////////////////////////

typedef struct {
    CharVector out_vec;
    CharVector err_vec;
    const char *cmd;
    int pipe_out_fd[2];
    int pipe_err_fd[2];
    pid_t pid;
    int status;
} DynamicSubprocess;

typedef struct {
    DynamicSubprocess *processes;
    int count;
    int request_id;
} DynamicSubprocesses;

int dynamic_process(int request_id, void **buff, size_t *buffsz, const char *request_str);

//////////////////////////////////////////////////////////////////////////////
//                           Global Variables                               //
//////////////////////////////////////////////////////////////////////////////

extern ServerConfig g_server_config;

#endif