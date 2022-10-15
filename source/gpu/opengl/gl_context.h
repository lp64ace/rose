#pragma once

#include "gpu/intern/gpu_context_private.h"

#include "gl_state.h"

#include <mutex>
#include <set>

#include "lib/lib_vector.h"

namespace rose {
namespace gpu {

class GLVaoCache;

class GLSharedOrphanLists {
public:
	/** Mutex for the below structures. */
	std::mutex mListsMutex;
	/** Buffers and textures are shared across context. Any context can free them. */
	Vector<unsigned int> mTextures;
	Vector<unsigned int> mBuffers;
public:
	void OrphansClear ( );
};

class GLContext : public Context {
public:
	// Capabilities.

	static int MaxCubemabSize;
	static int MaxUboSize;
	static int MaxUboBinds;
	static int MaxSSBOSize;
	static int MaxSSBOBinds;

	// Extensions.

	static bool BaseInstanceSupport;
	static bool ClearTextureSupport;
	static bool CopyImageSupport;
	static bool DebugLayerSupport;
	static bool DirectStateAccessSupport;
	static bool ExplicitLocationSupport;
	static bool GeometryShaderInvocations;
	static bool FixedRestartIndexSupport;
	static bool LayeredRenderingSupport;
	static bool NativeBarycentricSupport;
	static bool MultiBindSupport;
	static bool MultiDrawIndirectSupport;
	static bool ShaderDrawParametersSupport;
	static bool StencilTexturingSupport;
	static bool TextureCubeMapArraySupport;
	static bool TextureFilterAnisotropicSupport;
	static bool TextureGatherSupport;
	static bool TextureStorageSupport;
	static bool VertexAttribBindingSupport;

	// Workarounds.

	static bool DebugLayerWorkaround;
	static bool UnusedFbSlotWorkaround;
	static bool GenerateMipMapWorkaround;
	static float DerivativeSigns [ 2 ];

	// VBO for missing vertex attrib binding. Avoid undefined behavior on some implementation.
	unsigned int mDefaultAttrVbo;
private:
	std::set<class GLVaoCache *> mVaoCaches;

	std::mutex mListsMutex;
	Vector<unsigned int> mOrphanedVertarrays;
	Vector<unsigned int> mOrphanedFramebuffers;
	// GLBackend owns this data.
	GLSharedOrphanLists &mSharedOrphanList;
public:
	GLContext ( void *ghost_window , GLSharedOrphanLists &shared_orphan_list );
	~GLContext ( );

	static GLContext *Get ( );

	void Activate ( );
	void Deactivate ( );

	// Will push all pending commands to the GPU.
	void Flush ( );
	// Will wait until the GPU has finished executing all commands.
	void Finish ( );

	// These need to be called with the context the id was created with.
	void VaoFree ( unsigned int vao_id );
	void FboFree ( unsigned int fbo_id );
	// These can be called by any threads even without OpenGL ctx. Deletion will be delayed.
	static void BufFree ( unsigned int buf_id );
	static void TexFree ( unsigned int tex_id );

	static GLStateManager *StateManagerActiveGet ( ) {
		return static_cast< GLStateManager * >( GLContext::Get ( )->StateManager );
	};

	void GetMemoryStatistics ( int *total , int *free );

	void VaoCacheRegister ( class GLVaoCache *cache );
	void VaoCacheUnregister ( class GLVaoCache *cache );
private:
	static void OrphansAdd ( Vector<unsigned int> &orphan_list , std::mutex &list_mutex , unsigned int id );
	void OrphansClear ( );
};

}
}
