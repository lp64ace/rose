#include "MEM_guardedalloc.h"

#include "LIB_ghash.h"
#include "LIB_map.hh"
#include "LIB_set.hh"
#include "LIB_mempool.h"
#include "LIB_utildefines.h"

#include "DNA_ID.h"

#include "KER_idtype.h"
#include "KER_main.h"
#include "KER_main_id_name_map.h"

struct IDNameLib_Key {
    /** `ID.name + 2`: without the ID type prefix, since each id type gets its own 'map'. */
    const char *name;
};

struct IDNameLib_TypeMap {
    GHash *map;
    short idtype;
};

struct IDNameLib_Map {
    IDNameLib_TypeMap type_maps[INDEX_ID_MAX];
    Main *main;

    rose::Set<const ID *> *valid_id_pointers;

    /* For storage of keys for the #TypeMap #GHash, avoids many single allocations. */
    MemPool *type_maps_keys_pool;
};

rose::Set<const ID *> *KER_main_set_create(Main *main, rose::Set<const ID *> *set) {
    if (set == NULL) {
        set = MEM_new<rose::Set<const ID *>>(__func__);
    }

    ID *id;
    FOREACH_MAIN_ID_BEGIN(main, id) {
        set->add(id);
    }
    FOREACH_MAIN_ID_END;

    return set;
}

ROSE_INLINE IDNameLib_TypeMap *main_idmap_from_idcode(IDNameLib_Map *id_map, short idtype) {
    for (int i = 0; i < INDEX_ID_MAX; i++) {
        if (id_map->type_maps[i].idtype == idtype) {
            return &id_map->type_maps[i];
        }
    }
    return nullptr;
}

IDNameLib_Map *KER_main_idmap_create(Main *main, const bool create_valid_ids_set, Main *old_main) {
    IDNameLib_Map *id_map = static_cast<IDNameLib_Map *>(MEM_mallocN(sizeof(IDNameLib_Map), __func__));
    id_map->main = main;

    int index = 0;
    while(index < INDEX_ID_MAX) {
        IDNameLib_TypeMap *type_map = &id_map->type_maps[index];
        type_map->map = nullptr;
        type_map->idtype = KER_idtype_idcode_iter_step(&index);
        ROSE_assert(type_map->idtype != 0);
    }
    ROSE_assert(index == INDEX_ID_MAX);
    id_map->type_maps_keys_pool = NULL;

    if (create_valid_ids_set) {
        id_map->valid_id_pointers = KER_main_set_create(main, NULL);
        if (old_main != NULL) {
            id_map->valid_id_pointers = KER_main_set_create(old_main, id_map->valid_id_pointers);
        }
    }
    else {
        id_map->valid_id_pointers = NULL;
    }

    return id_map;
}

void KER_main_idmap_clear(IDNameLib_Map *id_map) {
    for (IDNameLib_TypeMap &type_map : id_map->type_maps) {
        if(type_map.map) {
            LIB_ghash_clear(type_map.map, NULL, NULL);
            type_map.map = NULL;
        }
    }

    if (id_map->valid_id_pointers) {
        id_map->valid_id_pointers->clear();
    }
}

void KER_main_idmap_free(IDNameLib_Map *id_map) {
    for (IDNameLib_TypeMap &type_map : id_map->type_maps) {
        if(type_map.map) {
            LIB_ghash_free(type_map.map, NULL, NULL);
            type_map.map = NULL;
        }
    }

    if (id_map->type_maps_keys_pool) {
        LIB_memory_pool_destroy(id_map->type_maps_keys_pool);
        id_map->type_maps_keys_pool = NULL;
    }

    MEM_delete(id_map->valid_id_pointers);
    MEM_freeN(id_map);
}

ROSE_INLINE uint idkey_hash(const void *ptr) {
    const IDNameLib_Key *idkey = static_cast<const IDNameLib_Key *>(ptr);
    uint key = LIB_ghashutil_strhash(idkey->name);
    return key;
}

ROSE_INLINE bool idkey_cmp(const void *a, const void *b) {
    const IDNameLib_Key *idkey_a = static_cast<const IDNameLib_Key *>(a);
    const IDNameLib_Key *idkey_b = static_cast<const IDNameLib_Key *>(b);
    return !STREQ(idkey_a->name, idkey_b->name);
}

void KER_main_idmap_insert_id(IDNameLib_Map *id_map, ID *id) {
    const short idtype = GS(id->name);
    IDNameLib_TypeMap *type_map = main_idmap_from_idcode(id_map, idtype);

    if(type_map != NULL && type_map->map != NULL) {
        ROSE_assert(id_map->type_maps_keys_pool != NULL);

        IDNameLib_Key *key = static_cast<IDNameLib_Key *>(LIB_memory_pool_malloc(id_map->type_maps_keys_pool));
        key->name = id->name + 2;
        LIB_ghash_insert(type_map->map, key, id);
    }
}

void KER_main_idmap_remove_id(IDNameLib_Map *id_map, const ID *id) {
    const short idtype = GS(id->name);
    IDNameLib_TypeMap *type_map = main_idmap_from_idcode(id_map, idtype);

    if(type_map != NULL && type_map->map != NULL) {
        ROSE_assert(id_map->type_maps_keys_pool != NULL);

        IDNameLib_Key lookup_key{id->name + 2};
        LIB_ghash_remove(type_map->map, &lookup_key, NULL, NULL);
    }
}

void *KER_main_idmap_lookup_name(IDNameLib_Map *id_map, short idtype, const char *name) {
    IDNameLib_TypeMap *type_map = main_idmap_from_idcode(id_map, idtype);

    if (type_map == NULL) {
        return NULL;
    }

    if (type_map->map == NULL) {
        if (id_map->type_maps_keys_pool == NULL) {
            id_map->type_maps_keys_pool = LIB_memory_pool_create(sizeof(IDNameLib_Key), 1024, 1024, ROSE_MEMPOOL_NOP);
        }

        GHash *map = type_map->map = LIB_ghash_new(idkey_hash, idkey_cmp, __func__);
        ListBase *lb = which_libbase(id_map->main, idtype);
        LISTBASE_FOREACH(ID *, id, lb) {
            IDNameLib_Key *key = static_cast<IDNameLib_Key *>(LIB_memory_pool_malloc(id_map->type_maps_keys_pool));
            key->name = id->name + 2;
            LIB_ghash_insert(map, key, id);
        }
    }

    const IDNameLib_Key key_lookup{name};
    return LIB_ghash_lookup(type_map->map, &key_lookup);
}
