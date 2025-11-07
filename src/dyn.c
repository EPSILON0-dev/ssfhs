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
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "ssfhs.h"

extern char **environ;
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

static char** dynamic_generate_environment(int request_id, const char *request_str)
{
    char env_buf[512];
    char **env = NULL;

    // Compute the size of the parent environment
    size_t penv_count = 0;
    while (environ[penv_count] != NULL)
    {
        penv_count++;
    }

    // Allocate memory for the child environment
    // +1 for the NULL terminator, +2 for ID and REQUEST_STR
    env = malloc((penv_count + 3) * sizeof(char *));
    if (!env) { return NULL; }

    // Copy the parent environment
    for (size_t i = 0; i < penv_count; i++)
    {
        // FIXME Override PWD to root dir
        if (strncmp(environ[i], "PWD=", 4) == 0)
        {
            // Set PWD to root dir
            snprintf(env_buf, sizeof(env_buf) - 1, "PWD=%s", g_server_config.root_dir);
            env[i] = strdup(env_buf);
        } 
        else
        {
            env[i] = strdup(environ[i]);
        }
    }

    // Add REQUEST_STR
    const char *req_var_name = "REQUEST_STR";
    char *req_str_buf = malloc(strlen(req_var_name) + strlen(request_str) + 2);
    if (!req_str_buf) { return NULL; }
    sprintf(req_str_buf, "%s=%s", req_var_name, request_str);
    env[penv_count] = req_str_buf;

    // Add REQUEST_ID
    snprintf(env_buf, sizeof(env_buf) - 1, "REQUEST_ID=%d", request_id);
    env[penv_count + 1] = strdup(env_buf);

    // Add NULL terminator
    env[penv_count + 2] = NULL;

    return env;
}

static int dynamic_open_pipes(DynamicSubprocesses *dsp)
{
    for (int i = 0; i < dsp->count; i++)
    {
        DynamicSubprocess *p = &dsp->processes[i];
        int res1 = pipe(p->pipe_err_fd);
        int res2 = pipe(p->pipe_out_fd);
        if (res1 == -1 || res2 == -1) 
        {
            log_error(dsp->request_id, "Failed to open pipe for dynamic command: %s\n", 
                strerror(errno));
            return 1;
        }

        int flags_err = fcntl(p->pipe_err_fd[0], F_GETFL, 0);
        fcntl(p->pipe_err_fd[0], F_SETFL, flags_err | O_NONBLOCK);
        int flags_out = fcntl(p->pipe_out_fd[0], F_GETFL, 0);
        fcntl(p->pipe_out_fd[0], F_SETFL, flags_out | O_NONBLOCK);
    }

    return 0;
}

static int dynamic_create_forks(DynamicSubprocesses *dsp, char **child_environ)
{
    for (int index = 0; index < dsp->count; index++)
    {
        DynamicSubprocess *p = &dsp->processes[index];

        p->pid = fork();
        if (p->pid < 0)
        {
            log_error(dsp->request_id, "Failed to fork for dynamic command: %s\n", 
                strerror(errno));
            close(p->pipe_out_fd[0]);
            close(p->pipe_out_fd[1]);
            close(p->pipe_err_fd[0]);
            close(p->pipe_err_fd[1]);
            p->status = -1;
            continue;
        }

        char_vector_init(&p->out_vec, 2048);
        char_vector_init(&p->err_vec, 2048);

        if (!p->pid) 
        {
            close(p->pipe_out_fd[0]); // close read end
            close(p->pipe_err_fd[0]); // close reaprocesses[i].d end
            dup2(p->pipe_out_fd[1], STDOUT_FILENO);  // redirect stdout
            dup2(p->pipe_err_fd[1], STDERR_FILENO);  // redirect stderr
            close(p->pipe_out_fd[1]);
            close(p->pipe_err_fd[1]);

            execle("/bin/sh", "sh", "-c", p->cmd, NULL, child_environ);
            _exit(127); // only reached if exec fails
        }

        if (g_server_config.debug)
        {
            printf("[Dynamic:Execute:%d] Created fork, PID: %d\n", index, p->pid);
        }
    }

    return 0;
}

static int dynamic_poll_pipes(DynamicSubprocesses *dsp)
{
    char buf[1024];
    ssize_t n;

    uint64_t start_ms = now_ms();
    int completed_processes = 0;
    while ((int)(now_ms() - start_ms) < g_server_config.dynamic_timeout)
    {
        for (int i = 0; i < dsp->count; i++)
        {
            DynamicSubprocess *p = &dsp->processes[i];

            // Read the out pipe
            while ((n = read(p->pipe_out_fd[0], buf, 1024)) > 0)
            {
                char_vector_push_arr(&p->out_vec, buf, n);
            }

            // Read the error pipe
            while ((n = read(p->pipe_err_fd[0], buf, 1024)) > 0)
            {
                char_vector_push_arr(&p->out_vec, buf, n);
            }

            int status = 0;
            int res = waitpid(p->pid, &status, WNOHANG);
            if (res > 0)
            {
                completed_processes ++;
            }
        }

        // Sleep for a bit before polling again
        usleep(SUBPROCESS_POLL_GRANULARITY_MS * 1000);

        if (completed_processes == dsp->count)
        {
            break;
        }
    }

    return 0;
}

static int dynamic_check_exit_codes(DynamicSubprocesses *dsp)
{
    bool error = false;

    for (int i = 0; i < dsp->count; i++)
    {
        DynamicSubprocess *p = &dsp->processes[i];

        // Check if the process finished execution
        int status = 0;
        pid_t result = waitpid(p->pid, &status, WNOHANG);
        if (result == 0)
        {
            log_error(dsp->request_id, "Dynamic command %d (pid: %d) did not finish in time and was killed.\n",
                i, p->pid);
            kill(p->pid, SIGKILL);
            error = true;
            continue;
        }

        // Check the exit code
        if (status != 0)
        {
            if (g_server_config.ignore_dynamic_errors)
            {
                log_error(dsp->request_id, "Dynamic command %d (pid: %d) failed with exit code: %d (ignored)\n",
                    i, p->pid, status);
            }
            else
            {
                log_error(dsp->request_id, "Dynamic command %d (pid: %d) failed with exit code: %d\n",
                    i, p->pid, status);
                error = true;
            }
        }

        // Output the stderr of the command
        if (p->err_vec.count > 0)
        {
            log_error(dsp->request_id, "[Dynamic:Execute:%d] (pid: %d) stderr: %s\n",
                i, p->pid, p->err_vec.items);
        }

        if (g_server_config.debug)
        {
            printf("[Dynamic:Execute:%d] Process exited: %d\n", i, status);
            printf("[Dynamic:Execute:%d] Got output: %s\n", i, p->out_vec.items);
        }
    }

    return error;
}

static int dynamic_copy_outputs(DynamicSubprocesses *dsp, char **outputs)
{
    for (int i = 0; i < dsp->count; i++)
    {
        outputs[i] = strdup(dsp->processes[i].out_vec.items);
    }
    return 0;
}

static void dynamic_cleanup_processes(DynamicSubprocesses *dsp)
{
    for (int i = 0; i < dsp->count; i++)
    {
        DynamicSubprocess *p = &dsp->processes[i];
        char_vector_free(&p->out_vec);
        char_vector_free(&p->err_vec);
        close(p->pipe_out_fd[0]);
        close(p->pipe_out_fd[1]);
        close(p->pipe_err_fd[0]);
        close(p->pipe_err_fd[1]);
    }
    free(dsp->processes);
}

static int dynamic_execute_commands(int request_id, const StringArray *cmds, char **outputs, const char *request_str)
{
    DynamicSubprocesses dsp;

    size_t process_alloc_size = cmds->count * sizeof(DynamicSubprocess);
    dsp.processes = malloc(process_alloc_size);
    dsp.count = cmds->count;
    dsp.request_id = request_id;
    memset(dsp.processes, 0, cmds->count * sizeof(DynamicSubprocess));
    for (size_t i = 0; i < cmds->count; i++)
    {
        dsp.processes[i].cmd = cmds->items[i];
    }

    char **child_environ = dynamic_generate_environment(request_id, request_str);
    
    bool error =
        dynamic_open_pipes(&dsp) ||
        dynamic_create_forks(&dsp, child_environ) ||
        dynamic_poll_pipes(&dsp) ||
        dynamic_check_exit_codes(&dsp) ||
        dynamic_copy_outputs(&dsp, outputs);

    dynamic_cleanup_processes(&dsp);
    return error;
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
int dynamic_process(int request_id, void **buff, size_t *buffsz, const char *request_str)
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
    bool exit_error = false;
    char **outputs = malloc(dyncmds.count * sizeof(char*));
    memset(outputs, 0, dyncmds.count * sizeof(char*));
    if (dynamic_execute_commands(request_id, &dyncmds, outputs, request_str))
    {
        exit_error = true;
        goto exit;
    }

    // Replace the tags with command outputs
    CharVector replvec;
    char_vector_init(&replvec, *buffsz + 1);
    dynamic_extract_replace(&vec, &replvec, outputs);

    // Replace the buffer and cleanup
    free(*buff);
    *buff = replvec.items;
    *buffsz = replvec.count;

    exit:
    for (size_t i = 0; i < dyncmds.count; i++)
    {
        free(outputs[i]);
    }
    free(outputs);
    string_array_free(&dyncmds);
    char_vector_free(&vec);
    return exit_error;
}
