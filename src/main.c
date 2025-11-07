#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "ssfhs.h"

ServerConfig g_server_config;
static int listen_fd;

void clean_exit(int signal)
{
    (void)signal;

    log_message(0, "Shutting down server...\n");

    close(listen_fd);
    log_close_file();
    config_free(&g_server_config);
    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) 
{
    signal(SIGINT, clean_exit);
    signal(SIGTERM, clean_exit);

    cli_args_parse(&g_server_config, argc, (const char **)argv);
    config_load(&g_server_config);
    log_open_file();

    listen_fd = socket_open(g_server_config.port);
    log_message(0, "Server listening on port [%d]\n", g_server_config.port);

    for ( ;; )
    {
        socket_accept_connection(listen_fd);
    }

    clean_exit(0);
}