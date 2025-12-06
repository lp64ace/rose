#ifndef KER_MAIN_NAME_MAP_H
#define KER_MAIN_NAME_MAP_H

#include "LIB_sys_types.h"

struct Main;
struct ID;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UniqueName_Map UniqueName_Map;

bool KER_main_name_map_get_unique_name(struct Main *main, struct ID *id, char *r_name);

void KER_main_name_map_remove_id(struct Main *main, struct ID *id, const char *name);
void KER_main_name_map_clear(struct Main *main);
void KER_main_namemap_destroy(struct UniqueName_Map **name_map);

#ifdef __cplusplus
}
#endif

#endif