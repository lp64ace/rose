#pragma once

#include "lib/lib_vector.h"
#include "lib/lib_string_ref.h"

void gpu_shader_dependency_init ( void );
void gpu_shader_dependency_exit ( void );

int gpu_shader_dependency_get_builtin_bits ( const StringRefNull source_name );

Vector<const char *> gpu_shader_dependency_get_resolved_source ( const StringRefNull source_name );
StringRefNull gpu_shader_dependency_get_source ( const StringRefNull source_name );

StringRefNull gpu_shader_dependency_get_filename_from_source_string ( const StringRefNull source_string );
