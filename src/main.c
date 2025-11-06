#include <stdlib.h>
#include <unistd.h>
#include "ssfhs.h"

ServerConfig g_server_config;

int main(int argc, char **argv) 
{
    cli_args_parse(&g_server_config, argc, (const char **)argv);
    config_load(&g_server_config);

    int listen_fd = socket_open(g_server_config.port);
    log_message(0, "Server listening on port [%d]\n", g_server_config.port);

    for ( ;; )
    {
        socket_accept_connection(listen_fd);
    }

    // Cleanup before exit
    close(listen_fd);
    config_free(&g_server_config);
    return EXIT_SUCCESS;
}

// TODO add on exit handler