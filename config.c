#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

unsigned short port = 0;
char *tmpl_path = NULL;

int
config_load(const char *conf_path)
{
    FILE *cfgf = fopen(conf_path, "r");
    if (!cfgf) {
        fprintf(stderr, "Error opening config: %s\n", strerror(errno));
        return -1;
    }

    //fseek(cfgf , 0, SEEK_END);
    //size_t cfgsize = ftell(cfgf);
    //rewind(cfgf);

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
        } else if (strcmp(line, "template") == 0) {
            value[strlen(value) - 1] = '\0';
            tmpl_path = strdup(value);
            printf("\ttemplate: %s\n", tmpl_path);
        } else {
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

    return 0;
}

