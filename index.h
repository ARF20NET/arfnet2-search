/*

    arfnet2-search: Fast file indexer and search
    Copyright (C) 2023 arf20 (Ángel Ruiz Fernandez)

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
    LOOKUP_SUBSTR_NOCASE,
    LOOKUP_REGEX
} lookup_type_t;

typedef struct {
    char *name;
    struct stat stat;
    const char *mime;
} node_data_t;

typedef struct map_s *index_t;

int index_init();
void index_deinit();
index_t index_new(size_t icapacity, const char *root);
int index_lookup(index_t index, lookup_type_t type, const char *query,
    const node_data_t **results);
void index_destroy(index_t index);

#endif /* _INDEX_H */

