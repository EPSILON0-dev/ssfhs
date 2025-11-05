/**
 * @file socket.c
 * @author epsiii
 * @brief Socket open functionality for SSFHS
 * @date 2025-10-29
 * 
 * @copyright Copyright (c) 2025
 * 
 */
 #include <string.h>
 #include <stdlib.h>
 #include <stdio.h>
 #include <errno.h>
#include <sys/poll.h>
 #include <unistd.h>
 #include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "ssfhs.h"

int socket_open(uint16_t port)
{
    int fd;
    struct sockaddr_in server_addr;

    // Create socket (IPv4, TCP)
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        fprintf(stderr, "Failed to open socket: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Bind to a port
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // listen on all interfaces
    server_addr.sin_port = htons(port);        // server port

    int bind_res = bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (bind_res < 0) {
        fprintf(stderr, "Failed to bind socket: %s\n", strerror(errno));
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Start listening
    int listen_res = listen(fd, 5);
    if (listen_res < 0) {  // backlog = 5
        fprintf(stderr, "Failed to start listener: %s\n", strerror(errno));
        close(fd);
        exit(EXIT_FAILURE);
    }

    return fd;
}

static int socket_send_response(const CharVector *vec, int conn_fd)
{
    int sent = send(conn_fd, vec->items, vec->count, 0);
    if (sent < 0)
    {
        snprintf(g_log_buffer, LOG_BUFFER_SIZE, 
            "Something went wrong while sending response: %s\n", strerror(errno));
        log_error(g_log_buffer);
        close(conn_fd);
    }
    return sent;
}

// TODO break up this function
int socket_handle_connection(int listen_fd)
{
    CharVector request_vec;
    HTTPRequest request;

    // Accept the incoming connection
    struct sockaddr_storage cliaddr;
    socklen_t addrlen = sizeof(cliaddr);
    int conn_fd = accept(listen_fd, (struct sockaddr*)&cliaddr, &addrlen);
    if (conn_fd < 0) 
    { 
        snprintf(g_log_buffer, LOG_BUFFER_SIZE, 
            "Something went wrong when accepting a connection\n");
        log_error(g_log_buffer);
        return 1; 
    }

    char_vector_init(&request_vec, 16);
    http_request_init(&request);

	// TODO Fork for incoming connection
    // Get the connection IP
    char ipstr[64];
    if (cliaddr.ss_family == AF_INET) {
        struct sockaddr_in *s = (struct sockaddr_in *)&cliaddr;
        inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof(ipstr));
    } else { // AF_INET6
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)&cliaddr;
        inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof(ipstr));
    }

    // Read the whole request with timeout
    uint64_t receive_start_time = now_ms();
    for ( ;; )
    {
        int time_elapsed = (int)(now_ms() - receive_start_time);
        if (time_elapsed > g_server_config.request_timeout_ms)
        {
            snprintf(g_log_buffer, LOG_BUFFER_SIZE, 
                "Request from %s timed out after %d ms\n", 
                ipstr, g_server_config.request_timeout_ms);
            log_error(g_log_buffer);
            close(conn_fd);
            char_vector_free(&request_vec);
            return 1;
        }

        struct pollfd pfd;
        pfd.fd = conn_fd;
        pfd.events = POLLIN;
        int poll_res = poll(&pfd, 1, RECEIVE_POLL_GRANULARITY_MS);
        if (poll_res < 0)
        {
            snprintf(g_log_buffer, LOG_BUFFER_SIZE, 
                "Something went wrong while polling for data: %s\n", strerror(errno));
            log_error(g_log_buffer);
            close(conn_fd);
            char_vector_free(&request_vec);
            return 1;
        }

        else if (poll_res == 0)
        {
            // Timeout, no data yet
            continue;
        }

        // Data is available to read
        char buffer[1024];
        size_t n = read(conn_fd, buffer, sizeof(buffer)-1);
        char_vector_push_arr(&request_vec, buffer, n); 
        if (http_got_whole_request(&request_vec)) { break; }
    }

    // Parse the request
    int parse_res = http_request_parse(&request_vec, &request);
    if (parse_res)
    {
        snprintf(g_log_buffer, LOG_BUFFER_SIZE, 
            "Something went wrong when parsing the request\n");
        log_error(g_log_buffer);
    }

    else 
    {
        // Store the request to the request file
        int request_store_error = http_request_store(&request_vec);

        // Generate and send the response
        CharVector response;
        char_vector_init(&response, 16);
        int status = http_response_generate(&response, &request, request_store_error);
        socket_send_response(&response, conn_fd);
        char_vector_free(&response);

        // Print log
        snprintf(g_log_buffer, LOG_BUFFER_SIZE, "%s %s %s %d\n", 
            ipstr, request.method, request.url, status);
        log_message(g_log_buffer);
    }

    // Close connections and reset the request vector
    close(conn_fd);
    char_vector_free(&request_vec);
    http_request_free(&request);
    return 0;
}
