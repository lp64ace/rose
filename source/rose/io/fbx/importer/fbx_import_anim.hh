#ifndef FBX_IMPORT_ANIM_HH
#define FBX_IMPORT_ANIM_HH

#include "fbx_import_util.hh"

struct Main;
struct FbxElementMapping;
struct Scene;

namespace rose::io::fbx {

void import_animations(struct Main *main, struct Scene *scene, const ufbx_scene *fbx, FbxElementMapping *mapping, const double fps);

}  // namespace rose::io::fbx

#endif // FBX_IMPORT_ANIM_HH
