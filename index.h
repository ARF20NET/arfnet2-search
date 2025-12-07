/*

    arfnet2-search: Fast file indexer and search
    Copyright (C) 2025 arf20 (Ángel Ruiz Fernandez)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

    index.c: Efficient fast file index

*/

#ifndef _INDEX_H
#define _INDEX_H

#include <sys/stat.h>
#include <stddef.h>

typedef enum {
    LOOKUP_SUBSTR,
    LOOKUP_SUBSTR_CASEINSENSITIVE,
    LOOKUP_EXACT,
    LOOKUP_REGEX
} lookup_type_t;

typedef struct {
    const char *name, *path;
    struct stat stat;
    const char *mime;
} node_data_t;

typedef struct map_s *index_t;

typedef enum {
    SORT_NAME,
    SORT_MIME,
    SORT_PATH,
    SORT_SIZE,
    SORT_TIME
} sort_type_t;

typedef struct {
    time_t time_low, time_high;
    size_t size_low, size_high;
} filter_t;

typedef struct {
    const node_data_t **results;
    size_t size, capacity;
} results_t;

int index_init();
void index_deinit();
index_t index_new(size_t icapacity, const char *root, int examine);
results_t *index_lookup(index_t index, lookup_type_t type, const char *query);
void index_destroy(index_t index);

void results_sort(results_t *results, sort_type_t sort_type, int desc);
results_t *results_filter(results_t *results, const filter_t *filter);
void results_destroy(results_t *results);

#endif /* _INDEX_H */

