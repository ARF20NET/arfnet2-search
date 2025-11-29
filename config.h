#ifndef _CONFIG_H
#define _CONFIG_H

#define BUFF_SIZE           65535
#define INIT_VEC_CAPACITY   256
#define CONFIG_PATH         "search.cfg"

#define DEFAULT_PORT        8888
#define DEFAULT_TMPL_PATH   "index.htm.tmpl"

/* config */
extern unsigned short port;
extern char *tmpl_path;
extern char *root;


int config_load(const char *conf_path);

#endif /* _CONFIG_H */

