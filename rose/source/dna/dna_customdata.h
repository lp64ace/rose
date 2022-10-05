#pragma once

#ifdef __cplusplus
extern "C" {
#endif

	#define MAX_CUSTOMDATA_LAYER_NAME 64

	struct AnonymousAttributeID;

	typedef struct CustomDataLayer {
		// Type of data in layer.
		int Type;
		// Offset of layer in block.
		int Offset;
		// General purpose flag.
		int Flag;
		// Number of the active layer of this type.
		int Active;
		// Number of the layer to render.
		int ActiveRender;
		// Number of the layer to render.
		int ActiveClone;
		// Number of the layer to render.
		int ActiveMask;
		// Layer name, MAX_CUSTOMDATA_LAYER_NAME.
		char Name [ 64 ];
		// Layer data.
		void *Data;
		/** Run-time identifier for this layer. If no one has a strong reference to this id anymore,
		* the layer can be removed. The custom data layer only has a weak reference to the id, because
		* otherwise there will always be a strong reference and the attribute can't be removed
		* automatically. */
		const struct AnonymousAttributeID *AnonymousId;
	};

	typedef struct CustomData {
		// CustomDataLayers, ordered by type.
		CustomDataLayer *Layers;
		// Runtime Only! - Maps types to indices of first layer of that type.
		int TypeMap [ 16 ];
		// Number of layers, size of layers array.
		int TotLayer , MaxLAyer;
		// Total size of all data layers.
		int TotSize;
		// Memory pool for allocation of blocks.
		struct LIB_mempool *pool;
	};

#ifdef __cplusplus
}
#endif
