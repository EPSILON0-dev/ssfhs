#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include "ssfhs.h"

ServerConfig g_server_config;

int main(int argc, char **argv) 
{
    cli_args_parse(&g_server_config, argc, (const char **)argv);
    config_load(&g_server_config);

    int listen_fd = socket_open(g_server_config.port);

    printf("Server listening on port %d...\n", g_server_config.port);

    CharVector request_vec;
    HTTPRequest request;
    char_vector_init(&request_vec, 16);
    http_request_init(&request);
    for ( ;; )
    {
        int conn_fd = socket_await_connection(listen_fd);
        if (conn_fd < 0) { continue; }

        // 5. Use read/write on the connected socket
        char buffer[1024];
        size_t n = read(conn_fd, buffer, sizeof(buffer)-1);
        if (n > 0) {
            char_vector_push_arr(&request_vec, buffer, n);
        }

        // For now we assume that the requests are short
        // if (http_got_whole_request(&request_vec)) 

        // Parse the request
        if (http_request_parse(&request_vec, &request))
        {
            fprintf(stderr, "Something went wrong when parsing the request\n");
        }

        // Generate the response
        CharVector response;
        char_vector_init(&response, 16);
        if (http_response_generate(&response, &request))
        {
            fprintf(stderr, "Something went wrong when generating response\n");
        }
        else 
        {
            socket_send_response(&response, conn_fd);
        }
        char_vector_free(&response);

        // 6. Close connections
        close(conn_fd);

        // Reset the request vector
        char_vector_free(&request_vec);
        char_vector_init(&request_vec, 16);
    }
    http_request_free(&request);
    char_vector_free(&request_vec);

    close(listen_fd);

    config_free(&g_server_config);
}