/**
 * @file res.c
 * @author epsiii
 * @brief Resouce location and access utilities
 * @date 2025-10-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "ssfhs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

char *resource_resolve_url_path(const char *path)
{
    // Combine the root and resource path
    char *root_real_path = realpath(g_server_config.root_dir, NULL);
    int combined_len = strlen(root_real_path) + strlen(path);
    char *combined_path = malloc(combined_len + 1);
    memcpy(combined_path, root_real_path, strlen(root_real_path));
    memcpy(combined_path + strlen(root_real_path), path, strlen(path));
    combined_path[combined_len] = '\0';

    if (g_server_config.debug)
    {
        printf("[RES:PathResolve] Generated combined path: %s\n", combined_path);
    }

    // Resolve the path
    char *resolved_path = realpath(combined_path, NULL);
    free(combined_path);
    if (!resolved_path)
    {
        free(root_real_path);
        return NULL;
    }
    
    // Return NULL if we exited the root
    if (strncmp(resolved_path, root_real_path, strlen(root_real_path)) != 0)
    {
        free(resolved_path);
        free(root_real_path);
        return NULL;
    }
    free(root_real_path);

    if (g_server_config.debug)
    {
        printf("[RES:PathResolve] Generated final path: %s\n", resolved_path);
    }

    return resolved_path;
}

// Path already has to be resolved
bool resource_is_accessible(const char *path)
{
    bool result = (access(path, F_OK) == 0);
    return result;
}

// Path already has to be resolved
bool resource_is_protected(const char *path)
{
    for (size_t i = 0; i < g_server_config.protected_files.count; i++)
    {
        if (strcmp(path, g_server_config.protected_files.items[i]) == 0)
        {
            return true;
        }
    }

    return false;
}

// Path already has to be resolved
bool resource_is_dynamic(const char *path)
{
    for (size_t i = 0; i < g_server_config.dynamic_files.count; i++)
    {
        if (strcmp(path, g_server_config.dynamic_files.items[i]) == 0)
        {
            return true;
        }
    }

    return false;
}

static char* resource_get_extension(const char *path)
{
    // Find the last separator
    char *ptr = strrchr(path, '.');
    if (!ptr) { return NULL; }

    // Duplicate the string after the separator
    return strdup(ptr + 1);
}

char* resource_get_content_type(const char *path)
{
    // Not even near all of them but ig it's good approximation for now
    // Even indices - keys, odd indices - values
    const char *map[] =
    {
        "html",    "text/html",
        "css",     "text/css",
        "csv",     "text/csv",
        "xml",     "text/xml",
        "jpeg",    "image/jpeg",
        "png",     "image/png",
        "gif",     "image/gif",
        "webp",    "image/webp",
        "svg+xml", "image/svg+xml",
        "bmp",     "image/bmp",
        "x-icon",  "image/x-icon",
        "mpeg",    "audio/mpeg",
        "ogg",     "audio/ogg",
        "wav",     "audio/wav",
        "webm",    "audio/webm",
        "flac",    "audio/flac",
        "mp4",     "video/mp4",
        "webm",    "video/webm",
        "ogg",     "video/ogg",
        "pdf",     "application/pdf",
        "zip",     "application/zip",
        "js",      "application/javascript",
    };

    const char *default_type = "application/octet-stream";

    // Try to map the extension
    char *ext = resource_get_extension(path);
    for (size_t i = 0; i < sizeof(map) / sizeof(*map); i += 2)
    {
        if (strcmp(ext, map[i]) == 0)
        {
            free(ext);
            return strdup(map[i + 1]);
        }
    }

    // If failed - fall back to octet-stream
    free(ext);
    return strdup(default_type);
}

int resource_get(void **buff, size_t *buffsz, const char *path)
{
    // Open the file
    FILE *f = fopen(path, "r");
    fseek(f, 0, SEEK_END);
    *buffsz = ftell(f);
    fseek(f, 0, SEEK_SET);

    // Allocate the space for the resource and get it
    *buff = malloc(*buffsz);
    int rb = fread(*buff, 1, *buffsz, f);
    if (rb != (int)(*buffsz))
    {
        fclose(f);
        snprintf(g_log_buffer, LOG_BUFFER_SIZE, 
            "Something went wrong when accessing resource, read %d/%ld\n", rb, *buffsz);
        log_error(g_log_buffer);
        return rb;
    }

    // Close the file
    fclose(f);

    // Process the file it it's a dynamic file
    if (resource_is_dynamic(path))
    {
        dynamic_process(buff, buffsz);
    }

    return 0;
}