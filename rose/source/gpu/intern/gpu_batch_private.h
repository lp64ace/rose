#pragma once

#include "gpu/gpu_batch.h"
#include "gpu/gpu_context.h"

#include "gpu_vertex_buffer_private.h"
#include "gpu_index_buffer_private.h"

namespace rose {
namespace gpu {

class Batch : public GPU_Batch {
public:
	virtual ~Batch ( ) = default;
	
	virtual void Draw ( int v_first , int v_count , int i_first , int i_count ) = 0;
	virtual void DrawIndirect ( GPU_StorageBuf *indirect , intptr_t offset ) = 0;
	virtual void MultiDrawIndirect ( GPU_StorageBuf *indirect , int count , intptr_t offset , intptr_t stride ) = 0;

	IndexBuf *GetElem ( ) const { return unwrap ( this->Elem ); }
	VertBuf *GetVerts ( const int index ) const { return unwrap ( this->Verts [ index ] ); }
	VertBuf *GetInst ( const int index ) const { return unwrap ( this->Inst [ index ] ); }
};

}
}
