#ifndef IO_FBX_IMPORT_ARMATURE_HH
#define IO_FBX_IMPORT_ARMATURE_HH

#include "fbx_import_util.hh"

struct Main;
struct FbxElementMapping;
struct Scene;

namespace rose::io::fbx {

void import_armatures(struct Main *main, struct Scene *scene, const ufbx_scene *fbx, FbxElementMapping *mapping);

}  // namespace rose::io::fbx

#endif	// IO_FBX_IMPORT_ARMATURE_HH
