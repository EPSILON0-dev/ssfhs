/**
 * @file utils.c
 * @author epsiii
 * @brief Utility functions used across the SSFHS
 * @date 2025-10-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "ssfhs.h"

//////////////////////////////////////////////////////////////////////////////
//                             String Array                                 //
//////////////////////////////////////////////////////////////////////////////

void string_array_init(StringArray *array)
{
    array->items = NULL;
    array->count = 0;
}

void string_array_add(StringArray *array, const char *item)
{
    array->items = realloc(array->items, sizeof(char*) * (array->count + 1));
    array->items[array->count] = strdup(item);
    array->count++;
}

char* string_array_get(StringArray *array, size_t index)
{
    if (index >= array->count) {
        return NULL;
    }
    return array->items[index];
}

void string_array_free(StringArray *array)
{
    for (size_t i = 0; i < array->count; i++) {
        free(array->items[i]);
    }

    free(array->items);
    array->count = 0;
}

//////////////////////////////////////////////////////////////////////////////
//                              Char Vector                                 //
//////////////////////////////////////////////////////////////////////////////

static void char_vector_realloc(CharVector *vec)
{
    size_t new_size = vec->max_size * 2;
    vec->items = realloc(vec->items, new_size);
    vec->max_size = new_size;
}

void char_vector_init(CharVector *vec, int initial_size)
{
    if (initial_size < 1) initial_size = 1;
    vec->max_size = initial_size;
    vec->items = malloc(initial_size);
    vec->count = 0;
    vec->items[vec->count] = '\0';
}

// Adds a null terminator at the end
void char_vector_push(CharVector *vec, char c)
{
    if (vec->count + 1 >= vec->max_size)
    {
        char_vector_realloc(vec);
    }

    vec->items[vec->count++] = c;
    vec->items[vec->count] = '\0';
}

// Adds a null terminator at the end
void char_vector_push_arr(CharVector *vec, const char *arr, size_t len)
{
    while (vec->count + len >= vec->max_size)
    {
        char_vector_realloc(vec);
    }

    memcpy(&vec->items[vec->count], arr, len);
    vec->count += len;
    vec->items[vec->count] = '\0';
}

char char_vector_get_char(const CharVector *vec, size_t index)
{
    if (index > vec->count)
    {
        return 0;
    }

    return vec->items[index];
}

int char_vector_get(const CharVector *vec, void *dst, size_t index, size_t len)
{
    if (index + len > vec->count)
    {
        return -1;
    }

    memcpy(dst, &vec->items[index], len);
    return len;
}

char* char_vector_get_alloc(const CharVector *vec, size_t index, size_t len)
{
    if (index + len > vec->count)
    {
        return NULL;
    }

    char *buff = malloc(len + 1);
    buff[len] = '\0';
    memcpy(buff, &vec->items[index], len);

    return buff;
}

void char_vector_free(CharVector *vec)
{
    free(vec->items);
}

//////////////////////////////////////////////////////////////////////////////
//                                 Others                                   //
//////////////////////////////////////////////////////////////////////////////

char *strtrim(const char *str)
{
    // Trim 
    int start_index = 0, end_index = strlen(str);
    while (isspace(str[start_index])) { start_index++; }
    while (isspace(str[end_index - 1])) { end_index--; }

    // Create a copy
    int len = end_index - start_index;
    char *newstr = malloc(len + 1);
    memcpy(newstr, &str[start_index], len);
    newstr[len] = '\0';

    // Do NOT free the old string
    // free(str);

    return newstr;
}

//////////////////////////////////////////////////////////////////////////////
//                                  Time                                    //
//////////////////////////////////////////////////////////////////////////////

uint64_t now_ms(void) 
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

uint64_t now_us(void) 
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}