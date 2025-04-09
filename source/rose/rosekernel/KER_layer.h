#ifndef KER_LAYER_H
#define KER_LAYER_H

#include "DNA_layer_types.h"
#include "DNA_screen_types.h"
#include "DNA_space_types.h"

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Returns the default view layer to view in work-spaces if there is none liked to the workspace yet. */
struct ViewLayer *KER_view_layer_default_view(const struct Scene *scene);
/** Returns the default view lauyer to render if we need to render just one. */
struct ViewLayer *KER_view_layer_default_render(const struct Scene *scene);

enum {
	VIEWLAYER_ADD_NEW = 0,
	VIEWLAYER_ADD_EMPTY = 1,
	VIEWLAYER_ADD_COPY = 2,
};

struct ViewLayer *KER_view_layer_find(const struct Scene *scene, const char *layer_name);
/** Adds a new layer, this can either add a new,empty or a copy from source layer, use the respective VIEWLAYER_ADD_XXX enum. */
struct ViewLayer *KER_view_layer_add(struct Scene *scene, const char *name, struct ViewLayer *view_layer_source, int type);

void KER_view_layer_free(struct ViewLayer *view_layer);
void KER_view_layer_free_ex(struct ViewLayer *view_layer, bool us);

/** Tag all the selected objects of a render-layer. */
void KER_view_layer_selected_objects_tag(struct ViewLayer *view_layer, int tag);

struct Base *KER_view_layer_base_find(struct ViewLayer *view_layer, struct Object *object);
void KER_view_layer_base_deselect_all(struct ViewLayer *view_layer);
void KER_view_layer_base_select_and_set_active(struct ViewLayer *view_layer, struct Base *selbase);

void KER_view_layer_copy_data(struct Scene *scene_dst, const struct Scene *scene_src, struct ViewLayer *view_layer_dst, const struct ViewLayer *view_layer_src, int flag);

/** Get the active collection. */
struct LayerCollection *KER_layer_collection_get_active(struct ViewLayer *view_layer);
struct LayerCollection *KER_layer_collection_activate_parent(struct ViewLayer *view_layer, struct LayerCollection *collection);
bool KER_layer_collection_activate(struct ViewLayer *view_layer, struct LayerCollection *collection);

/** Get the total number of collections (including all the nested collections) */
size_t KER_layer_collection_count(const struct ViewLayer *view_layer);
size_t KER_layer_collection_findindex(const struct ViewLayer *view_layer, const struct LayerCollection *collection);
struct LayerCollection *KER_layer_collection_from_index(struct ViewLayer *view_layer, size_t index);

void KER_layer_collection_resync_forbid(void);
void KER_layer_collection_resync_allow(void);

/** Update view layer collection tree from collections used in the scene. */
void KER_layer_collection_sync(const struct Scene *scene, struct ViewLayer *view_layer);

void KER_scene_collection_sync(const struct Scene *scene);
void KER_main_collection_sync(const struct Main *bmain);
void KER_main_collection_sync_remap(const struct Main *bmain);

bool KER_view_layer_has_collection(const struct ViewLayer *view_layer, const struct Collection *collection);
bool KER_scene_has_object(struct Scene *scene, struct Object *object);

bool KER_layer_collection_objects_select(struct ViewLayer *view_layer, struct LayerCollection *collection, bool deselect);
bool KER_layer_collection_has_selected_objects(struct ViewLayer *view_layer, struct LayerCollection *collection);
bool KER_layer_collection_has_layer_collection(struct LayerCollection *parent, struct LayerCollection *collection);

void KER_base_set_visible(struct Scene *scene, struct ViewLayer *view_layer, struct Base *base, bool extend);

void KER_layer_collection_set_visible(struct ViewLayer *view_layer, struct LayerCollection *collection, bool visible, bool hierarchy);
void KER_layer_collection_set_flag(struct LayerCollection *collection, int flag, bool value);

void KER_base_eval_flags(struct Base *base);

#ifdef __cplusplus
}
#endif

#endif	// KER_LAYER_H
