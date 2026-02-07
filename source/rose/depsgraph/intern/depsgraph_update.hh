#ifndef DEPSGRAPH_UPDATE_HH
#define DEPSGRAPH_UPDATE_HH

struct DEGEditorUpdateContext;
struct ID;

namespace rose::depsgraph {

void deg_editors_id_update(const DEGEditorUpdateContext *update_ctx, struct ID *id);
void deg_editors_scene_update(const DEGEditorUpdateContext *update_ctx, bool updated);

}  // namespace rose::depsgraph

#endif	// !DEPSGRAPH_UPDATE_HH
