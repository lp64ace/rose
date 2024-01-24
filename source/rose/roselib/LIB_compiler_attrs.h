#pragma once

#ifdef _MSC_VER

#include <sal.h>

#define ATTR_PRINTF_FORMAT _In_z_ _Printf_format_string_

#else
	
#define ATTR_PRINTF_FORMAT

#endif
