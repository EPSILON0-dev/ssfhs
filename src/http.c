/**
 * @file http.c
 * @author epsiii
 * @brief HTTP generation and parsing functionality file access is also handled here
 * @date 2025-10-29
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "ssfhs.h"

//////////////////////////////////////////////////////////////////////////////
//                           Request Parsing                                //
//////////////////////////////////////////////////////////////////////////////

bool http_got_whole_request(const CharVector *vec)
{
    // TODO Add support for PUT & POST methods

    const char *ptr = vec->items;
    do 
    {
        // Find the end of the line
        const char *lf_ptr = strchr(ptr, '\n');

        // XXX Print the line
        /*
        int line_len = lf_ptr - ptr;
        char *line = malloc(line_len);
        line[line_len - 1] = '\0';
        memcpy(line, ptr, line_len - 1);
        printf("Checked line: >%s<\n", line);
        free(line);
        */

        // Check if the line is 1 char long and only contains CR
        //  We don't count LF into the line length
        if (lf_ptr - ptr == 1 && *ptr == '\r')
        {
            return true;
        }

        // Move the pointer (skip the LF itself)
        ptr = lf_ptr + 1;
    } 
    while (ptr < vec->items + vec->count);

    return false;
}

static int http_request_parse_1st_line(const CharVector *vec, HTTPRequest *request, char **ptr)
{
    *ptr = vec->items;

    // Copy the method string
    char *method_end = strchr(*ptr, ' ');
    if (!method_end) { return 1; }
    size_t method_len = method_end - *ptr;

    if (request->method) { free(request->method); }
    request->method = char_vector_get_alloc(vec, *ptr - vec->items, method_len);
    if (!request->method) { return 1; }

    *ptr += method_len + 1;
    if (g_server_config.debug) 
    {
        printf("[HTTP:Parse1Line] Got method: %s\n", request->method);
    }

    // Copy the method string
    char *url_end = strchr(*ptr, ' ');
    if (!url_end) { return 1; }
    size_t url_len = url_end - *ptr;

    if (request->url) { free(request->url); }
    request->url = char_vector_get_alloc(vec, *ptr - vec->items, url_len);
    if (!request->url) { return 1; }

    *ptr += url_len + 1;
    if (g_server_config.debug) 
    {
        printf("[HTTP:Parse1Line] Got url: %s\n", request->url);
    }

    // Drop the procotol version (we don't need it)
    *ptr = strchr(*ptr, '\n');
    if (!*ptr) return 1;
    (*ptr)++;

    // If everything went okay return success
    return 0;
}

// 0 - Read a header, 1 - Done parsing, -1 - Error
static int http_request_parse_header(const CharVector *vec, HTTPRequest *request, char **ptr)
{
    // Exit on CRLF
    char *line_end = strchr(*ptr, '\n');
    if (!line_end) { return -1; }
    if (line_end - *ptr == 1 && **ptr == '\r') { return 1; }

    // Find the separator
    char *sep = strchr(*ptr, ':');
    if (!sep) { return -1; }

    // Split the line into the key and value
    char *raw_key = char_vector_get_alloc(vec, *ptr - vec->items, sep - *ptr);
    char *raw_value = char_vector_get_alloc(vec, sep - vec->items + 1, line_end - sep);

    // Trim the strings
    char *key = strtrim(raw_key);
    char *value = strtrim(raw_value);

    // Store the key and value
    string_array_add(&request->header_keys, key);
    string_array_add(&request->header_values, value);

    // Print debug info
    if (g_server_config.debug)
    {
        printf("[HTTP:ParseHeader] Got header >%s< --- >%s<\n", key, value);
    }

    // Free the resources
    free(key);
    free(value);
    free(raw_key);
    free(raw_value);

    // Move to the next line
    *ptr = line_end + 1;
    return 0;
}

void http_request_init(HTTPRequest *request)
{
    memset(request, 0, sizeof(HTTPRequest));
    string_array_init(&request->header_keys);
    string_array_init(&request->header_values);
}

int http_request_parse(const CharVector *vec, HTTPRequest *request)
{
    char *ptr;

    // Parse the 1'st line
    if (http_request_parse_1st_line(vec, request, &ptr))
    {
        return 1;
    }

    // Parse the headers
    int res;
    do {
        res = http_request_parse_header(vec, request, &ptr);
    } while (res == 0);
    if (res < 0) { return 1; }

    request->okay = true;
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//                        Response Generation                               //
//////////////////////////////////////////////////////////////////////////////

static int http_response_generate_internal(CharVector *vec, const char *status, const char *path)
{
    char buffer[128];

    // Get the resource
    void *res_buff;
    size_t res_buffsz;
    char *res_type;
    if (path != NULL)
    {
        resource_get(&res_buff, &res_buffsz, path);
        res_type = resource_get_content_type(path);
    }

    // Generate the 1st line
    const char *resp_1st_line = "HTTP/1.1 ";
    char_vector_push_arr(vec, resp_1st_line, strlen(resp_1st_line));
    char_vector_push_arr(vec, status, strlen(status));
    const char *resp_crlf = "\r\n";
    char_vector_push_arr(vec, resp_crlf, strlen(resp_crlf));

    // Generate the server version header
    const char *resp_server = "Server: " SERVER_NAME " " SERVER_VERSION "\r\n";
    char_vector_push_arr(vec, resp_server, strlen(resp_server));

    // Generate the date header
    time_t now = time(NULL);
    struct tm *gmt = gmtime(&now);
    strftime(buffer, sizeof(buffer) - 1, "Date: %a, %d %b %Y %H:%M:%S GMT\r\n", gmt);
    char_vector_push_arr(vec, buffer, strlen(buffer));

    if (path != NULL)
    {
        // Generate content length header
        snprintf(buffer, sizeof(buffer) - 1, "Content-Length: %ld\r\n", res_buffsz);
        char_vector_push_arr(vec, buffer, strlen(buffer));

        // Generate content type header
        snprintf(buffer, sizeof(buffer) - 1, "Content-Type: %s\r\n", res_type);
        char_vector_push_arr(vec, buffer, strlen(buffer));
    }

    // Add the content type and connection headers
    const char *resp_headers = 
        "Connection: Close\r\n\r\n";
    char_vector_push_arr(vec, resp_headers, strlen(resp_headers));

    if (path != NULL)
    {
        // Add the message at the end
        char_vector_push_arr(vec, res_buff, res_buffsz);
        free(res_type);
        free(res_buff);
    }

    return 0;
}

static int http_response_generate_bad_request(CharVector *vec)
{
    return http_response_generate_internal(vec,
        "400 Bad Request",
        g_server_config.bad_request_page_file
    );
}

static int http_response_generate_not_found(CharVector *vec)
{
    return http_response_generate_internal(vec,
        "404 Not Found",
        g_server_config.not_found_page_file
    );
}

static int http_response_generate_forbidden(CharVector *vec)
{
    return http_response_generate_internal(vec,
        "403 Forbidden",
        g_server_config.forbidden_page_file
    );
}

int http_response_generate(CharVector *response, HTTPRequest *request)
{
    // If the request wasn't parsed correctly, return 400 Bad Request
    if (!request->okay)
    {
        return http_response_generate_bad_request(response);
    }

    // Resolve "/" to "/index.html", and other paths to their URIs
    char *resolved_path;
    if (strcmp(request->url, "/") == 0)
    {
        resolved_path = strdup(g_server_config.index_page_file);
    }
    else
    {
        resolved_path = resource_resolve_url_path(request->url);
    }

    // Return not found if resource isn't available
    if (!resolved_path || !resource_is_accessible(resolved_path))
    {
        if (resolved_path) { free(resolved_path); }
        return http_response_generate_not_found(response);
    }

    // Return forbidden if resource is protected
    if (resource_is_protected(resolved_path))
    {
        free(resolved_path);
        return http_response_generate_forbidden(response);
    }

    // Return the normal response if there was no problem
    if (g_server_config.debug)
    {
        printf("[HTTP:GenerateResponse] Returning resource: %s\n", resolved_path);
    }
    int res = http_response_generate_internal(response, "200 OK", resolved_path);
    free(resolved_path);
    return res;
}

void http_request_free(HTTPRequest *request)
{
    if (request->method) free(request->method);
    if (request->url) free(request->url);
    if (request->data) free(request->data);
    string_array_free(&request->header_keys);
    string_array_free(&request->header_values);
}