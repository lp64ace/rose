#include "kernel/rke_context.h"

#include "context_private.h"

// struct rose::kernel::Context *active_ctx = nullptr;

namespace rose {
namespace kernel {

Context::Context ( ) {

}

Context::~Context ( ) {

}

bool Context::Activate ( ) {
	return true;
}

void Context::Deactivate ( ) {

}

}
}

using namespace rose;
using namespace rose::kernel;

struct rContext *CTX_create ( ) {
	return wrap ( new Context ( ) );
}

void CTX_free ( struct rContext *_ctx ) {
	Context *ctx = unwrap ( _ctx );
	delete ctx;
}
