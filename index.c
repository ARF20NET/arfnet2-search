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

#define _GNU_SOURCE
#include "index.h"

#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <magic.h>

#include "config.h"

/* closed addressing map */
typedef struct map_s {
    struct node_s *map;
    size_t count, size;
} map_t;

struct node_s {
    node_data_t *data;
    struct node_s *next;
    map_t *child;
};


static magic_t magic_cookie = NULL;


size_t
hash(const char *s, int mod)
{
    size_t hash = 0;
    if (!s)
        return 0;
    while (*s)
        hash = hash * 31 + *s++;
    return hash % mod;
}

map_t *
map_new(size_t size)
{
    map_t *map = malloc(sizeof(map_t));

    map->map = malloc(sizeof(struct node_s) * size);
    memset(map->map, 0, sizeof(struct node_s) * size);
    map->size = size;
    map->count = 0;
    return map;
}

void
map_insert(map_t *map, const char *key, node_data_t *data, map_t *child)
{
    struct node_s *node = &map->map[hash(key, map->size)];
  
    if (node->data) {
        for (; node->next; node = node->next);
        
        node->next = malloc(sizeof(struct node_s));
        node->next->data = data;
        node->next->child = child;
        node->next->next = NULL;
    } else {
        node->data = data;
        node->child = child;
    }

    map->count++;
}

results_t *
results_new()
{
    results_t *r = malloc(sizeof(results_t));
    r->capacity = INIT_VEC_CAPACITY;
    r->results = malloc(sizeof(node_data_t*) * r->capacity);
    memset(r->results, 0, sizeof(node_data_t*) * r->capacity);
    r->size = 0;
    return r;
}

void
results_insert(results_t *results, const node_data_t *result)
{
    if (results->size + 1 >= results->capacity) {
        results->capacity *= 2;
        results->results = realloc(results->results, sizeof(node_data_t*) *
            results->capacity);
    }

    results->results[results->size++] = result;
}

static int
cmp_results(const void *_r1, const void *_r2, void *arg)
{
    const node_data_t *r1 = *(node_data_t**)_r1, *r2 = *(node_data_t**)_r2;
    sort_type_t sort_type = ((int*)arg)[0];
    int desc = ((int*)arg)[1];

    int cmp = 0;

    switch (sort_type) {
    case SORT_NAME:
        cmp = strcmp(r1->name, r2->name);
    break;
    case SORT_PATH:
        cmp = strcmp(r1->path, r2->path);
    break;
    case SORT_MIME:
        if (!r1->mime)
            return 0;
        cmp = strcmp(r1->mime, r2->mime);
    break;
    case SORT_SIZE:
        cmp = r1->stat.st_size - r2->stat.st_size;
    break;
    case SORT_TIME:
        cmp = r1->stat.st_mtime - r2->stat.st_mtime;
    break;
    }
    
    return !desc ? cmp : -cmp;
}

void
results_sort(results_t *results, sort_type_t sort_type, int desc)
{
    int arg[2] = { sort_type, desc };
    qsort_r(results->results, results->size, sizeof(node_data_t*), cmp_results,
        &arg);
}

results_t *
results_filter(results_t *results, const filter_t *filter)
{
    results_t *filtered = results_new();
    for (size_t i = 0; i < results->size; i++) {
        const node_data_t *n = results->results[i];
        if (filter->time_low && (n->stat.st_mtime < filter->time_low))
            continue;
        if (filter->time_high && (n->stat.st_mtime > filter->time_high))
            continue;
        if (filter->size_low && (n->stat.st_size < filter->size_low))
            continue;
        if (filter->size_high && (n->stat.st_size > filter->size_high))
            continue;

        results_insert(filtered, n);
    }
    results_destroy(results);
    return filtered;
}

void
results_destroy(results_t *results)
{
    free(results->results);
    free(results);
}

int
index_init()
{
    magic_cookie = magic_open(MAGIC_MIME);
    if (!magic_cookie) {
        fprintf(stderr, "[index] error magic_open()\n");
        return -1;
    }
    if (magic_load(magic_cookie, NULL) < 0) {
        fprintf(stderr, "[index] error magic_load(): %s\n",
            magic_error(magic_cookie));
        return -1;
    }
    return 0;
}

void
index_deinit()
{
    magic_close(magic_cookie);
}

map_t *
index_recurse(size_t size, const char *dir, int examine, size_t rootlen)
{
    DIR *dirp = opendir(dir);
    if (!dirp) {
        fprintf(stderr, "[index] error opening directory %s: %s\n", dir,
            strerror(errno));
        return NULL;
    }

    map_t *map = map_new(size);

    char path[4096];
    struct dirent *de = NULL;
    while ((de = readdir(dirp))) {
        if (de->d_name[0] == '.') {
            if (de->d_name[1] == '\0')
                continue;
            else if (de->d_name[1] == '.')
                if (de->d_name[2] == '\0')
                    continue;
        }

        snprintf(path, 4096, "%s/%s", dir, de->d_name);

        /* stat it */
        node_data_t *data = malloc(sizeof(node_data_t));
        memset(data, 0, sizeof(node_data_t));
        data->name = strdup(de->d_name);
        data->path = strdup(&path[rootlen]);
        if (stat(path, &data->stat) < 0) {
            fprintf(stderr, "[index] error stat() %s: %s\n", path,
                strerror(errno));
            free(data);
            data = NULL;
        }

        /* examine */
        if (examine) {
            const char *mime = magic_file(magic_cookie, path);
            if (!mime) {
                fprintf(stderr, "[index] error magic_file() %s: %s\n", path,
                    magic_error(magic_cookie));
            } else 
                data->mime = strdup(mime);
        } 

        /* recurse */
        map_t *child = NULL;
        if (de->d_type == DT_DIR) {
            child = index_recurse(size, path, examine, rootlen);
        }

        map_insert(map, de->d_name, data, child);
    }

    closedir(dirp);

    return map;
}

map_t *
index_new(size_t size, const char *dir, int examine) {
    return index_recurse(size, dir, examine, strlen(dir) + 1);
}

void
index_lookup_substr(map_t *index, const char *query,
    results_t *results)
{
    for (size_t i = 0; i < index->size; i++) {
        if (!index->map[i].data)
            continue;

        for (struct node_s *node = &index->map[i]; node; node = node->next) {
            if (strstr(node->data->name, query))
                results_insert(results, node->data);
            if (node->child)
                index_lookup_substr(node->child, query, results);
        }
    }
}

void
index_lookup_substr_caseinsensitive(map_t *index, const char *query,
    results_t *results)
{
    for (size_t i = 0; i < index->size; i++) {
        if (!index->map[i].data)
            continue;

        for (struct node_s *node = &index->map[i]; node; node = node->next) {
            if (strcasestr(node->data->name, query))
                results_insert(results, node->data);
            if (node->child)
                index_lookup_substr(node->child, query, results);
        }
    }
}

void
index_lookup_exact(map_t *index, const char *query, results_t *results)
{
    for (size_t i = 0; i < index->size; i++) {
        if (!index->map[i].data)
            continue;

        for (struct node_s *node = &index->map[i]; node; node = node->next) {
            if (strcmp(node->data->name, query) == 0)
                results_insert(results, node->data);
            if (node->child)
                index_lookup_substr(node->child, query, results);
        }
    }
}

void
index_lookup_regex(map_t *index, const char *query,
    results_t *results)
{

}

results_t *
index_lookup(map_t *index, lookup_type_t type, const char *query)
{
    results_t *results = results_new();

    switch (type) {
    case LOOKUP_SUBSTR:
        index_lookup_substr(index, query, results);
    break;
    case LOOKUP_SUBSTR_CASEINSENSITIVE:
        index_lookup_substr_caseinsensitive(index, query, results);
    break;
    case LOOKUP_EXACT:
        index_lookup_exact(index, query, results);
    break;
    case LOOKUP_REGEX:
        index_lookup_regex(index, query, results);
    break;

    }

    return results;
}

void
index_destroy(index_t index)
{

}


