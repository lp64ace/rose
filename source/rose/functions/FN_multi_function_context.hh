#pragma once

/**
 * An #Context is passed along with every call to a multi-function. Right now it does nothing,
 * but it can be used for the following purposes:
 * - Pass debug information up and down the function call stack.
 * - Pass reusable memory buffers to sub-functions to increase performance.
 * - Pass cached data to called functions.
 */

#include "LIB_utildefines.h"

#include "LIB_map.hh"

namespace rose::fn::multi_function {

class Context;

class ContextBuilder {};

class Context {
public:
	Context(ContextBuilder & /*builder*/) {
	}
};

}  // namespace rose::fn::multi_function
