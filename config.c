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

    config.c: Configuration parser

*/

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

unsigned short port = 0;
char *tmpl_path = NULL, *root = NULL, *app_subdir = NULL,
    *result_subdir = NULL;
int magic_enable = 0, period = 86400;

int
config_load(const char *conf_path)
{
    FILE *cfgf = fopen(conf_path, "r");
    if (!cfgf) {
        fprintf(stderr, "Error opening config: %s\n", strerror(errno));
        return -1;
    }

    printf("config:\n");
    
    char line[256];
    while (fgets(line, sizeof(line), cfgf)) {
        if (*line == '#' || *line == '\n')
            continue;

        char *separator = strchr(line, '=');
        if (!separator) {
            fprintf(stderr, "[config] malformed line: %s\n", line);
            continue;
        }

        *separator = '\0';

        char *value = separator + 1;

        if (strcmp(line, "port") == 0) {
            port = atoi(value);
            printf("\tport: %d\n", port);
            if (port == 0) {
                fprintf(stderr, "[config] invalid port: %s\n", line);
                return -1;
            }
        }
        else if (strcmp(line, "template") == 0) {
            value[strlen(value) - 1] = '\0';
            tmpl_path = strdup(value);
            printf("\ttemplate: %s\n", tmpl_path);
        }
        else if (strcmp(line, "root") == 0) {
            value[strlen(value) - 1] = '\0';
            root = strdup(value);
            printf("\troot: %s\n", root);
        }
        else if (strcmp(line, "app_subdir") == 0) {
            value[strlen(value) - 1] = '\0';
            app_subdir = strdup(value);
            printf("\tapp_subdir: %s\n", app_subdir);
        }
        else if (strcmp(line, "result_subdir") == 0) {
            value[strlen(value) - 1] = '\0';
            result_subdir = strdup(value);
            printf("\tresult_subdir: %s\n", result_subdir);
        }
        else if (strcmp(line, "magic") == 0) {
            value[strlen(value) - 1] = '\0';
            magic_enable = (strcmp(value, "true") == 0);
            printf("\tmagic: %d\n", magic_enable);
        }
        else if (strcmp(line, "period") == 0) {
            value[strlen(value) - 1] = '\0';
            period = atoi(value);
            printf("\tperiod: %d\n", period);
        }
        else {
            fprintf(stderr, "[config] unknown key: %s\n", line);
            continue;
        }
    }

    fclose(cfgf);

    if (port == 0) {
        fprintf(stderr, "[config] W: no port, using default\n");
        port = DEFAULT_PORT;
    }

    if (!tmpl_path) {
        fprintf(stderr, "[config] W: no template file given, using default\n");
        tmpl_path = DEFAULT_TMPL_PATH;
    }

    if (!root) {
        fprintf(stderr, "[config] E: no root given\n");
        return -1;
    }

    if (!app_subdir) {
        fprintf(stderr, "[config] E: no application subdirectory given\n");
        return -1;
    }

    if (!result_subdir) {
        fprintf(stderr, "[config] E: no result link subdirectory given\n");
        return -1;
    }

    return 0;
}

