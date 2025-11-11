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
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "ssfhs.h"

// TODO: Use thread pool instead of creating a new thread for each connection

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

static int socket_send_response(const CharVector *vec, ConnectionDescriptor *cd)
{
    int sent = send(cd->conn_fd, vec->items, vec->count, 0);
    if (sent < 0)
    {
        log_error(cd->conn_id, "Something went wrong while sending response: %s\n", strerror(errno));
        close(cd->conn_id);
    }
    return sent;
}

static void socket_generate_ip_string(char *buff, size_t size, const struct sockaddr_storage *addr)
{
    if (addr->ss_family == AF_INET) {
        struct sockaddr_in *s = (struct sockaddr_in *)addr;
        inet_ntop(AF_INET, &s->sin_addr, buff, size);
    } else { // AF_INET6
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)addr;
        inet_ntop(AF_INET6, &s->sin6_addr, buff, size);
    }
}

static int socket_receive_request(ConnectionDescriptor *cd, CharVector *request_vec)
{
    uint64_t receive_start_time = now_ms();
    for ( ;; )
    {
        int time_elapsed = (int)(now_ms() - receive_start_time);
        if (time_elapsed > g_server_config.request_timeout_ms)
        {
            char ipstr[64];
            socket_generate_ip_string(ipstr, sizeof(ipstr) - 1, &cd->cliaddr);
            log_error(cd->conn_id, "Request from %s timed out after %d ms\n", 
                ipstr, g_server_config.request_timeout_ms);
            return 1;
        }

        struct pollfd pfd;
        pfd.fd = cd->conn_fd;
        pfd.events = POLLIN;
        int poll_res = poll(&pfd, 1, RECEIVE_POLL_GRANULARITY_MS);
        if (poll_res < 0)
        {
            log_error(cd->conn_id, "Something went wrong while polling for data: %s\n", 
                strerror(errno));
            return 1;
        }

        else if (poll_res == 0)
        {
            // Timeout, no data yet
            continue;
        }

        // Data is available to read
        char buffer[1024];
        size_t n = read(cd->conn_fd, buffer, sizeof(buffer)-1);
        char_vector_push_arr(request_vec, buffer, n); 
        if (http_got_whole_request(request_vec)) { break; }
    }

    return 0;
}

static int socket_respond(ConnectionDescriptor *cd, HTTPRequest *request, CharVector *request_vec)
{
    // Generate and send the response
    CharVector response;
    char_vector_init(&response, 16);
    int status = http_response_generate(cd->conn_id, &response, request, request_vec->items);
    socket_send_response(&response, cd);
    char_vector_free(&response);

    // Print log
    char ipstr[64];
    socket_generate_ip_string(ipstr, sizeof(ipstr) - 1, &cd->cliaddr);
    log_message(cd->conn_id, "%s %s %s %d %.1fms\n", 
        ipstr, request->method, request->url, status, 
        (float)(now_us() - cd->start_us) / 1000.0);

    return 0;
}

static int socket_handle_connection(ConnectionDescriptor *cd)
{
    int exit_code = 0;
    CharVector request_vec;
    HTTPRequest request;
    char_vector_init(&request_vec, 16);
    http_request_init(&request);

    if (socket_receive_request(cd, &request_vec))
    {
        exit_code = 1;
        goto exit;
    }

    if (http_request_parse(&request_vec, &request))
    {
        log_error(cd->conn_id, "Something went wrong when parsing the request\n");
        exit_code = 1;
        goto exit;
    }

    socket_respond(cd, &request, &request_vec);

    exit:
    close(cd->conn_fd);
    char_vector_free(&request_vec);
    http_request_free(&request);
    free(cd);
    return exit_code;
}

static void* socket_handle_connection_thread(void *arg)
{
    socket_handle_connection((ConnectionDescriptor*)(arg));
    return NULL;
}

int socket_accept_connection(int listen_fd)
{
    static int conn_id = 0;

    struct sockaddr_storage cliaddr;
    socklen_t addrlen = sizeof(cliaddr);

    conn_id++;
    int conn_fd = accept(listen_fd, (struct sockaddr*)&cliaddr, &addrlen);
    if (conn_fd < 0) 
    { 
        log_error(conn_id, "Something went wrong when accepting a connection\n");
        return 1; 
    }

    ConnectionDescriptor *cd = malloc(sizeof(ConnectionDescriptor));
    cd->conn_id = conn_id;
    cd->conn_fd = conn_fd;
    cd->start_us = now_us();
    memcpy(&cd->cliaddr, &cliaddr, sizeof(struct sockaddr_storage));

    // Create a thread for the new connection
    pthread_t tid;
    int res = pthread_create(&tid, NULL, socket_handle_connection_thread, cd);
    pthread_detach(tid);

    if (!res)
    {
        if (g_server_config.debug)
        {
            printf("[Socket:Accept] Created a thread for connection %d, tid: %ld\n",
                conn_id, tid);
        }
    }
    else
    {
        log_error(conn_id, "Failed to create a thread for connection %d, err: %d\n",
            conn_id, res);
    }

    return 0;
}
