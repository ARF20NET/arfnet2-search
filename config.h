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

*/

#ifndef _CONFIG_H
#define _CONFIG_H

#define BUFF_SIZE           65535
#define INIT_VEC_CAPACITY   256
#define INIT_MAP_CAPACITY   1024 /* index directory initial size */
#define CONFIG_PATH         "search.cfg"

#define DEFAULT_PORT        8888
#define DEFAULT_TMPL_PATH   "index.htm.tmpl"

/* config */
extern unsigned short port;
extern char *tmpl_path, *root, *subdir;


int config_load(const char *conf_path);

#endif /* _CONFIG_H */

