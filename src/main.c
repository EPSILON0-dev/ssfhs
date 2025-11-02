#include <stdio.h>
#include <unistd.h>
#include "ssfhs.h"

ServerConfig g_server_config;
char g_log_buffer[LOG_BUFFER_SIZE];

int main(int argc, char **argv) 
{
    cli_args_parse(&g_server_config, argc, (const char **)argv);
    config_load(&g_server_config);

    int listen_fd = socket_open(g_server_config.port);

    snprintf(g_log_buffer, LOG_BUFFER_SIZE, "Server listening on port [%d]\n", g_server_config.port);
    log_message(g_log_buffer);

    for ( ;; )
    {
        socket_handle_connection(listen_fd);
    }

    // Cleanup before exit
    close(listen_fd);
    config_free(&g_server_config);
}