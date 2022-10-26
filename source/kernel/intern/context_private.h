#pragma once

#include "kernel/rke_context.h"

namespace rose {
namespace kernel {

class Context {
public:
	Context ( );
	~Context ( );

	bool Activate ( );
	void Deactivate ( );
};

/* Syntactic sugar. */
static inline rContext *wrap ( Context *vert ) {
	return reinterpret_cast< rContext * >( vert );
}
static inline Context *unwrap ( rContext *vert ) {
	return reinterpret_cast< Context * >( vert );
}
static inline const Context *unwrap ( const rContext *vert ) {
	return reinterpret_cast< const Context * >( vert );
}

}
}
