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
    bool found_header_end = false;

    if (!vec->items)
    {
        printf("No items???\n");
        return false;
    }

    // Check if there's content length in the request
    char *cl_ptr = strstr(vec->items, "Content-Length");

    const char *ptr = vec->items;
    do 
    {
        // Find the end of the line
        const char *lf_ptr = strchr(ptr, '\n');
        if (!lf_ptr) { return false; }

        // Check if the line is 1 char long and only contains CR
        //  We don't count LF into the line length
        if (lf_ptr - ptr == 1 && *ptr == '\r')
        {
            ptr += 2;
            found_header_end = true;
            break;
        }

        // Move the pointer (skip the LF itself)
        ptr = lf_ptr + 1;
    } 
    while (ptr < vec->items + vec->count);

    // If we found content length check if the whole length was received
    if (cl_ptr)
    {
        char *len_start = strchr(cl_ptr, ':');
        char *len_end = strchr(cl_ptr, '\r');
        if (!len_start || !len_end) { return false; }

        char *len_str_raw = char_vector_get_alloc(vec, 
            len_start - vec->items + 1, len_end - len_start - 1);
        char *len_str = strtrim(len_str_raw);

        int len = atoi(len_str);

        free(len_str);
        free(len_str_raw);

        if (!len)
        { 
            return false; 
        }
        else
        {
            return (vec->items + vec->count >= ptr + len);
        }
    }

    // Just return whether we found the end of the headers
    else
    {
        return found_header_end;
    }
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

static int http_response_generate_internal(int request_id, CharVector *vec, 
    const char *status, const char *path, const char *request_str)
{
    char buffer[128];

    // Get the resource
    void *res_buff;
    size_t res_size;
    char *res_type;
    if (path != NULL)
    {
        int result = resource_get(request_id, &res_buff, &res_size, path, request_str);
        res_type = resource_get_content_type(path);
        if (result) { return 1; }
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
        snprintf(buffer, sizeof(buffer) - 1, "Content-Length: %ld\r\n", res_size);
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
        char_vector_push_arr(vec, res_buff, res_size);
        free(res_type);
        free(res_buff);
    }

    return 0;
}

static void http_response_generate_bad_request(int request_id, CharVector *vec)
{
    http_response_generate_internal(request_id, vec,
        "400 Bad Request",
        g_server_config.bad_request_page_file,
        ""
    );
}

static void http_response_generate_not_found(int request_id, CharVector *vec)
{
    http_response_generate_internal(request_id, vec,
        "404 Not Found",
        g_server_config.not_found_page_file,
        ""
    );
}

static void http_response_generate_forbidden(int request_id, CharVector *vec)
{
    http_response_generate_internal(request_id, vec,
        "403 Forbidden",
        g_server_config.forbidden_page_file,
        ""
    );
}

static void http_response_generate_server_error(int request_id, CharVector *vec)
{
    http_response_generate_internal(request_id, vec,
        "500 Internal Server Error",
        g_server_config.server_error_page_file,
        ""
    );
}

int http_response_generate(int request_id, CharVector *response, HTTPRequest *request, const char *request_str)
{
    // If the request wasn't parsed correctly, return 400 Bad Request
    if (!request->okay)
    {
        http_response_generate_bad_request(request_id, response);
        return 400;
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
        http_response_generate_not_found(request_id, response);
        return 404;
    }

    // Return forbidden if resource is protected
    if (resource_is_protected(resolved_path))
    {
        free(resolved_path);
        http_response_generate_forbidden(request_id, response);
        return 403;
    }

    // Return the normal response if there was no problem
    if (g_server_config.debug)
    {
        printf("[HTTP:GenerateResponse] Returning resource: %s\n", resolved_path);
    }

    // Try to return the resource, if that fails return 500, if that fails return empty 500 code
    if (http_response_generate_internal(request_id, response, "200 OK", resolved_path, request_str))
    {
        http_response_generate_server_error(request_id, response);
        return 500;
    }

    // Normal exit
    free(resolved_path);
    return 200;
}

void http_request_free(HTTPRequest *request)
{
    if (request->method) free(request->method);
    if (request->url) free(request->url);
    if (request->data) free(request->data);
    string_array_free(&request->header_keys);
    string_array_free(&request->header_values);
}