/**
 * @file dyn.c
 * @author epsiii
 * @brief Functionality related to the dynamic files
 * @date 2025-10-31
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include "ssfhs.h"

static const char *opening_tag = "<" DYNAMIC_TAG ">";
static const char *closing_tag = "</" DYNAMIC_TAG ">";

static int dynamic_extract_commands(const CharVector *vec, StringArray *dyncmds)
{
    const char *ptr = vec->items;

    while (ptr < vec->items + vec->count)
    {
        // Find the opening tag
        char *opening = strstr(ptr, opening_tag);
        if (!opening) { break; }
        ptr = opening + strlen(opening_tag);

        // Find the closing tag
        char *closing = strstr(ptr, closing_tag);
        if (!closing) { return 1; }
        ptr = closing + strlen(closing_tag);

        // Extract the command
        size_t index = opening - vec->items + strlen(opening_tag);
        size_t len = closing - opening - strlen(opening_tag);
        char *cmd = char_vector_get_alloc(vec, index, len);
        string_array_add(dyncmds, cmd);
        free(cmd);
    }

    if (g_server_config.debug)
    {
        for (size_t i = 0; i < dyncmds->count; i++)
        {
            printf("[DYNAMIC:Extract:%ld] %s\n", i, dyncmds->items[i]);
        }
    }

    return 0;
}

static int dynamic_execute_command(const char *cmd, char **output, int index)
{
    // TODO add timeout
    // TODO set cwd to root dir
    // TODO add stderr print on fail

    // Create pipe FDs
    int fds[2];
    if (pipe(fds) == -1) {
        snprintf(g_log_buffer, LOG_BUFFER_SIZE, 
            "Failed to open pipe for dynamic command: %s\n", strerror(errno));
        log_error(g_log_buffer);
        return 1;
    }

    // Create a fork and execute the command
    pid_t pid = fork();
    if (pid < 0)
    {
        snprintf(g_log_buffer, LOG_BUFFER_SIZE, 
            "Failed to fork for dynamic command: %s\n", strerror(errno));
        log_error(g_log_buffer);
        close(fds[0]);
        close(fds[1]);
        return 1;
    }

    // Child process
    if (!pid) 
    {
        close(fds[0]); // close read end
        dup2(fds[1], STDOUT_FILENO);  // redirect stdout
        close(fds[1]);

        execl("/bin/sh", "sh", "-c", cmd, NULL);
        _exit(127); // only reached if exec fails
    }

    if (g_server_config.debug)
    {
        printf("[Dynamic:Execute:%d] Created fork, PID: %d\n", index, pid);
    }

    // Read the output of the subprocess
    CharVector vec;
    char buf[1024];
    ssize_t n;
    char_vector_init(&vec, 2048);
    close(fds[1]);
    while ((n = read(fds[0], buf, sizeof(buf))) > 0)
    {
        char_vector_push_arr(&vec, buf, n);
    }

    // Get the status of the process
    int status = 0;
    close(fds[0]);
    waitpid(pid, &status, 0);

    if (g_server_config.debug)
    {
        printf("[Dynamic:Execute:%d] Process exited: %d\n", index, status);
        printf("[Dynamic:Execute:%d] Got output: %s\n", index, vec.items);
    }

    // Copy the output to the outputs array
    *output = strdup(vec.items);
    char_vector_free(&vec);

    // TODO fail ignore enable
    if (status != 0)
    {
        snprintf(g_log_buffer, LOG_BUFFER_SIZE, 
            "Dynamic command %d (pid: %d) failed with exit code: %d (ignored)\n", index, pid, status);
        log_error(g_log_buffer);
    }
    
    return 0;
}

static int dynamic_execute_commands(const StringArray *cmds, char **outputs)
{
    for (size_t i = 0; i < cmds->count; i++)
    {
        if (dynamic_execute_command(cmds->items[i], &outputs[i], i + 1))
        {
            return 1;
        }
    }

    return 0;
}

static int dynamic_extract_replace(const CharVector *in_vec, CharVector *out_vec, char **outputs)
{
    const char *ptr = in_vec->items;

    int index = 0;
    while (ptr < in_vec->items + in_vec->count)
    {
        const char *tptr = ptr;

        // Find the opening tag
        char *opening = strstr(tptr, opening_tag);
        if (!opening) { break; }
        tptr = opening + strlen(opening_tag);

        // Find the closing tag
        char *closing = strstr(tptr, closing_tag);
        if (!closing) { return 1; }
        tptr = closing + strlen(closing_tag);

        // Copy the text up to the opening tag and the output
        char_vector_push_arr(out_vec, ptr, opening - ptr);
        char_vector_push_arr(out_vec, outputs[index], strlen(outputs[index]));

        // Move the pointer and increment index
        ptr = tptr;
        index ++;
    }

    if (g_server_config.debug)
    {
        printf("[DYNAMIC:Replace] Replaced %d dynamic tags.\n", index);
    }

    return 0;
}


// Replaces the buffers (deallocates old ones and allocates new ones)
int dynamic_process(void **buff, size_t *buffsz)
{
    // Copy the buffer into a vector
    CharVector vec;
    char_vector_init(&vec, *buffsz + 1);
    char_vector_push_arr(&vec, *buff, *buffsz);

    // Exctract the commands
    StringArray dyncmds;
    string_array_init(&dyncmds);
    dynamic_extract_commands(&vec, &dyncmds);

    // Execute the commands
    char **outputs = malloc(dyncmds.count * sizeof(char*));
    memset(outputs, 0, dyncmds.count * sizeof(char*));
    dynamic_execute_commands(&dyncmds, outputs);

    // Replace the tags with command outputs
    CharVector replvec;
    char_vector_init(&replvec, *buffsz + 1);
    dynamic_extract_replace(&vec, &replvec, outputs);

    // Replace the buffer and cleanup
    free(*buff);
    *buff = replvec.items;
    *buffsz = replvec.count;
    string_array_free(&dyncmds);
    char_vector_free(&vec);

    return 0;
}
