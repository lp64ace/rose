#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

void gpu_shader_dependency_init(void);

void gpu_shader_dependency_exit(void);

#if defined(__cplusplus)
}
#endif

#if defined(__cplusplus)

#	include "LIB_string_ref.hh"
#	include "LIB_vector.hh"

#	include "gpu_shader_create_info.hh"

namespace rose::gpu::shader {

BuiltinBits gpu_shader_dependency_get_builtins(const StringRefNull source_name);

Vector<const char *> gpu_shader_dependency_get_resolved_source(const StringRefNull source_name);
StringRefNull gpu_shader_dependency_get_source(const StringRefNull source_name);

/**
 * \brief Find the name of the file from which the given string was generated.
 * \return filename or empty string.
 * \note source_string needs to be identical to the one given by gpu_shader_dependency_get_source()
 */
StringRefNull gpu_shader_dependency_get_filename_from_source_string(const StringRefNull source_string);

}  // namespace rose::gpu::shader

#endif
