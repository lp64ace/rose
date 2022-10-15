#pragma once

#include "lib/lib_utildefines.h"
#include "lib/lib_alloc.h"
#include "lib/lib_compiler.h"

#if !defined(NDEBUG)
#  define TRUST_NO_ONE 1
#endif

#ifdef TRUST_NO_ONE
#  include <assert.h>
#endif
