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

    main.c: Program entry point

*/

#define _XOPEN_SOURCE 700 /* strptime() without destroying clock_gettime() */

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include <microhttpd.h>

#include "config.h"
#include "index.h"

static char *index_format_template = NULL;

static index_t g_index = NULL;

static const char *result_html_header = 
    "<p>%ld results in %f seconds</p>\n"
    "<div class=\"result-header\">\n"
        "<a class=\"sort-name %s\" href=\"%s\">Name %s</a><a class=\"mime %s\" href=\"%s\">mime-type %s</a><br>\n"
        "<a class=\"path %s\" href=\"%s\">path %s</a><div class=\"attrib\">"
            "<a class=\"size %s\" href=\"%s\">Size %s</a>"
            "<a class=\"time %s\" href=\"%s\">Time %s</a></div><br>\n"
    "</div>\n";

static const char *result_html_template = 
    "<div class=\"result\">\n"
        "<span class=\"name\">%s</span><super class=\"mime\">%s</super><br>\n"
        "<a class=\"path\" href=\"%s\">%s</a><div class=\"attrib\">"
            "<span class=\"size\">%s</span>"
            "<span class=\"time\">%s</span></div><br>\n"
    "</div>\n";

static const char *
generate_results_header_html(struct MHD_Connection *connection, const char *baseurl,
    sort_type_t sort_type, int sort_order, size_t nresults, float lookup_time)
{
    static char buff[65535], name_url[256], mime_url[256], path_url[256],
        size_url[256], time_url[256];

    *buff = '\0';

    const char *arrows[] = { "&#8593;", "&#8595;" };

    int name_order = (sort_type == SORT_NAME) && sort_order;
    int mime_order = (sort_type == SORT_MIME) && sort_order;
    int path_order = (sort_type == SORT_PATH) && sort_order;
    int size_order = (sort_type == SORT_SIZE) && sort_order;
    int time_order = (sort_type == SORT_TIME) && sort_order;

    snprintf(name_url, 256, "%s&s=n&o=%c", baseurl, name_order ? 'a' : 'd');
    snprintf(mime_url, 256, "%s&s=m&o=%c", baseurl, mime_order ? 'a' : 'd');
    snprintf(path_url, 256, "%s&s=p&o=%c", baseurl, path_order ? 'a' : 'd');
    snprintf(size_url, 256, "%s&s=s&o=%c", baseurl, size_order ? 'a' : 'd');
    snprintf(time_url, 256, "%s&s=t&o=%c", baseurl, time_order ? 'a' : 'd');

    snprintf(buff, 65535, result_html_header, nresults, lookup_time,
        sort_type == SORT_NAME ? "sort-active" : "", name_url,
            arrows[!name_order],
        sort_type == SORT_MIME ? "sort-active" : "", mime_url,
            arrows[!mime_order],
        sort_type == SORT_PATH ? "sort-active" : "", path_url,
            arrows[!path_order],
        sort_type == SORT_SIZE ? "sort-active" : "", size_url,
            arrows[!size_order],
        sort_type == SORT_TIME ? "sort-active" : "", time_url,
            arrows[!time_order]
    );

    return buff;
}

static const char *
sizestr(size_t size)
{
    static char buf[32];
    if (size < 1024)
        snprintf(buf, 32, "%ld B", size);
    else if (size < 1024LL * 1024LL)
        snprintf(buf, 32, "%.2f KiB", (float)size/1024.f);
    else if (size < 1024LL * 1024LL * 1024LL)
        snprintf(buf, 32, "%.2f MiB", (float)size/(1024.f*1024.f));
    else if (size < 1024LL * 1024LL * 1024LL * 1024LL)
        snprintf(buf, 32, "%.2f GiB", (float)size/(1024.f*1024.f*1024.f));
    else if (size < 1024LL * 1024LL * 1024LL * 1024LL * 1024LL)
        snprintf(buf, 32, "%.2f TiB", (float)size/(1024.f*1024.f*1024.f*1024.f));
    return buf;
}

static const char *
generate_results_html(results_t *results)
{
    static char buff[65535], timebuf[256], urlbuf[4096];
    char *pos = buff;
  
    for (int i = 0; i < results->size; i++) {
        const node_data_t *data = results->results[i];
        struct tm *tm_mtim = gmtime(&data->stat.st_mtime);
        strftime(timebuf, 256, "%b %d %Y", tm_mtim);

        snprintf(urlbuf, 4096, "%s%s", subdir, data->path);

        pos += snprintf(pos, 65535 - (pos - buff),
            result_html_template,
            data->name,
            data->mime ? data->mime : "",
            urlbuf, data->path,
            sizestr(data->stat.st_size), timebuf
        );
    }

    return buff;
}

enum MHD_Result answer_to_connection(
    void *cls, struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *version,
    const char *upload_data,
    size_t *upload_data_size,
    void **ptr
) {
    char buff[BUFF_SIZE];

    const struct sockaddr_in **coninfo =
        (const struct sockaddr_in**)MHD_get_connection_info(
            connection, MHD_CONNECTION_INFO_CLIENT_ADDRESS);

    time_t time_now = time(NULL);
    struct tm *tm_now = gmtime(&time_now);
    static char timestr[256];
    strftime(timestr, 256, "%Y-%m-%d %H:%M:%S", tm_now);

    printf("[%s] [webserver] %s %s %s: ",
        timestr, inet_ntoa((*coninfo)->sin_addr), method, url);

    struct MHD_Response *response;
    int ret;

    if (strcmp(method, "GET") == 0 && strcmp(url, "/") == 0) {
        snprintf(buff, BUFF_SIZE, index_format_template, "", "", "", "", "", "",
            "");

        response = MHD_create_response_from_buffer(strlen(buff), (void*)buff,
            MHD_RESPMEM_PERSISTENT);

        printf("%d\n", 200);
        ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
    }
    else if (strcmp(method, "GET") == 0 && strcmp(url, "/query") == 0) {
        /* get query */
        const char *query = MHD_lookup_connection_value(connection,
            MHD_GET_ARGUMENT_KIND, "q");

        /* get and parse query type */
        lookup_type_t query_type = -1;
        const char *query_type_str = MHD_lookup_connection_value(connection,
            MHD_GET_ARGUMENT_KIND, "t");
        if (!query_type_str)
            query_type_str = "s";

        if (query_type_str) {
            switch (query_type_str[0]) {
            case 's': query_type = LOOKUP_SUBSTR; break;
            case 'i': query_type = LOOKUP_SUBSTR_CASEINSENSITIVE; break;
            case 'e': query_type = LOOKUP_EXACT; break;
            case 'r': query_type = LOOKUP_REGEX; break;
            }
        } else query_type = LOOKUP_SUBSTR;

        /* get and parse sorting */
        sort_type_t sort_type = SORT_NAME;
        int sort_order = 0;
        const char *sort_type_str = MHD_lookup_connection_value(connection,
            MHD_GET_ARGUMENT_KIND, "s");
        const char *sort_order_str = MHD_lookup_connection_value(connection,
            MHD_GET_ARGUMENT_KIND, "o");
        if (sort_type_str) {
            switch (sort_type_str[0]) {
            case 'n': sort_type = SORT_NAME; break;
            case 'm': sort_type = SORT_MIME; break;
            case 'p': sort_type = SORT_PATH; break;
            case 's': sort_type = SORT_SIZE; break;
            case 't': sort_type = SORT_TIME; break;
            }
        }
        if (sort_order_str)
            sort_order = sort_order_str[0] == 'd';

        /* get and parse filters */
        const char *filter_time_low = MHD_lookup_connection_value(connection,
            MHD_GET_ARGUMENT_KIND, "ftl");
        const char *filter_time_high = MHD_lookup_connection_value(connection,
            MHD_GET_ARGUMENT_KIND, "fth");
        const char *filter_size_low = MHD_lookup_connection_value(connection,
            MHD_GET_ARGUMENT_KIND, "fsl");
        const char *filter_size_high = MHD_lookup_connection_value(connection,
            MHD_GET_ARGUMENT_KIND, "fsh");

        filter_t filter = { 0 };

        struct tm filter_tm;
        if (strptime(filter_time_low, "%Y-%m-%d", &filter_tm))
            filter.time_low = mktime(&filter_tm);
        else
            filter.time_low = 0;

        if (strptime(filter_time_high, "%Y-%m-%d", &filter_tm))
            filter.time_high = mktime(&filter_tm);
        else
            filter.time_high = 0;

        filter.size_low = atoi(filter_size_low);
        filter.size_high = atoi(filter_size_high);


        /* build baseurl with query and filters (no sort) for sort links */
        char baseurl[1024];
        snprintf(baseurl, 1024, "/query?q=%s&t=%s&ftl=%s&fth=%s&fsl=%s&fsh=%s",
            query,
            query_type_str,
            filter_time_low ? filter_time_low : "",
            filter_time_high ? filter_time_high : "",
            filter_size_low ? filter_size_low : "",
            filter_size_high ? filter_size_high : ""
        );


        /* lookup query in index with type, mesuring time */
        struct timespec start, finish;
        clock_gettime(CLOCK_REALTIME, &start);

        results_t *results = NULL;
        if (query && g_index)
            results = index_lookup(g_index, query_type, query);

        clock_gettime(CLOCK_REALTIME, &finish);

        /* sort results */
        if (results)
            results_sort(results, sort_type, sort_order);

        /* filter results */
        if (results)
            results = results_filter(results, &filter);

        /* generate response with header, results, and time */
        float lookup_time = (finish.tv_sec + (0.000000001 * finish.tv_nsec)) - 
            (start.tv_sec + (0.000000001 * start.tv_nsec));

        if (query && results)
            snprintf(buff, BUFF_SIZE, index_format_template, query,
                filter_time_low ? filter_time_low : "",
                filter_time_high ? filter_time_high : "",
                filter_size_low ? filter_size_low : "",
                filter_size_high ? filter_size_high : "",
                generate_results_header_html(connection, baseurl, sort_type,
                    sort_order, results->size, lookup_time),
                generate_results_html(results));
        else
            snprintf(buff, BUFF_SIZE, index_format_template, "", "",
                "", "", "", "", "indexing in progress... try again later");

        /* send it */
        response = MHD_create_response_from_buffer(strlen(buff), (void*)buff,
            MHD_RESPMEM_PERSISTENT);

        /* cleanup */
        if (results)
            results_destroy(results);

        printf("%d\n", 200);
        ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
    }
    else {
        response = MHD_create_response_from_buffer(0, (void*)NULL, 0);
        printf("%d\n", 418);
        ret = MHD_queue_response(connection, 418, response);
        MHD_destroy_response(response);
    }
    return ret;
}

int main() {
    printf("ARFNET search (C) 2025 licence GPLv3\n");

    if (config_load(CONFIG_PATH) < 0)
        return 1;

    /* read index template file */
    FILE *tf = fopen(tmpl_path, "r");
    if (!tf) {
        fprintf(stderr, "error opening index template file: %s\n",
            strerror(errno));
        return 1;
    }
    fseek(tf, 0, SEEK_END);
    size_t tfs = ftell(tf);
    rewind(tf);
    index_format_template = malloc(tfs + 1);
    fread(index_format_template, 1, tfs, tf);
    fclose(tf);
    index_format_template[tfs] = '\0';

    /* start server */
    struct MHD_Daemon *daemon;

    daemon = MHD_start_daemon(
        MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_EPOLL,
        port, NULL, NULL,
        &answer_to_connection, NULL, MHD_OPTION_END);

    if (!daemon) {
        fprintf(stderr, "error starting libmicrohttpd daemon\n");
        return 1;
    }

    /* begin indexing */
    if (index_init() < 0)
        return 1;

    time_t time_now = time(NULL);
    struct tm *tm_now = gmtime(&time_now);
    static char timestr[256];
    strftime(timestr, 256, "%Y-%m-%d %H:%M:%S", tm_now);

    printf("[%s] [index] indexeding started...\n", timestr);

    g_index = index_new(INIT_MAP_CAPACITY, root, 1);

    time_now = time(NULL);
    tm_now = gmtime(&time_now);
    strftime(timestr, 256, "%Y-%m-%d %H:%M:%S", tm_now);

    printf("[%s] [index] indexed finished\n", timestr);

    while (1) {
        sleep(1000);
    }
}

