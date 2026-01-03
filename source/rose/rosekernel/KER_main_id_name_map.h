#ifndef KER_MAIN_ID_NAME_MAP_H
#define KER_MAIN_ID_NAME_MAP_H

#include "LIB_utildefines.h"

struct ID;
struct Main;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct IDNameLib_Map IDNameLib_Map;

/**
 * Generate mapping from ID type/name to ID pointer for given \a main.
 *
 * \note When used during undo/redo, there is no guaranty that ID pointers from UI area are not
 * pointing to freed memory (when some IDs have been deleted). To avoid crashes in those cases, one
 * can provide the 'old' (aka current) Main database as reference. #KER_main_idmap_lookup_id will
 * then check that given ID does exist in \a old_bmain before trying to use it.
 *
 * \param create_valid_ids_set: If \a true, generate a reference to prevent freed memory accesses.
 * \param old_main: If not NULL, its IDs will be added the valid references set.
 */
struct IDNameLib_Map *KER_main_idmap_create(struct Main *main, bool create_valid_ids_set, struct Main *old_main);

void KER_main_idmap_clear(struct IDNameLib_Map *id_map);
void KER_main_idmap_free(struct IDNameLib_Map *id_map);

void KER_main_idmap_insert_id(struct IDNameLib_Map *id_map, struct ID *id);
void KER_main_idmap_remove_id(struct IDNameLib_Map *id_map, const struct ID *id);

void *KER_main_idmap_lookup_name(struct IDNameLib_Map *id_map, short idtype, const char *name);

#ifdef __cplusplus
}
#endif

#endif // KER_MAIN_ID_NAME_MAP_H
