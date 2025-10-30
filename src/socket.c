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
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "ssfhs.h"

int socket_open(uint16_t port)
{
    int fd;
    struct sockaddr_in server_addr;

    // Create socket (IPv4, TCP)
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        fprintf(stderr, "Failed to open socket: %d\n", fd);
        exit(EXIT_FAILURE);
    }

    // Bind to a port
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // listen on all interfaces
    server_addr.sin_port = htons(port);        // server port

    int bind_res = bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (bind_res < 0) {
        fprintf(stderr, "Failed to bind socket: %d\n", bind_res);
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Start listening
    int listen_res = listen(fd, 5);
    if (listen_res < 0) {  // backlog = 5
        fprintf(stderr, "Failed to start listener: %d\n", fd);
        close(fd);
        exit(EXIT_FAILURE);
    }

    return fd;
}

int socket_await_connection(int listen_fd)
{
    int conn_fd = accept(listen_fd, NULL, NULL);
    if (conn_fd < 0) {
        fprintf(stderr, "Something went wrong while accepting connection: %d\n", conn_fd);
        close(listen_fd);
        return -1;
    }

    return conn_fd;
}

int socket_send_response(const CharVector *vec, int conn_fd)
{
    int sent = send(conn_fd, vec->items, vec->count, 0);
    if (sent < 0)
    {
        fprintf(stderr, "Something went wrong while sending response: %d\n", conn_fd);
        close(conn_fd);
    }
    return sent;
}