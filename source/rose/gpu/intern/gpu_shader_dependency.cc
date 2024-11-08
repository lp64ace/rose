#include <algorithm>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>

#include "LIB_ghash.h"
#include "LIB_map.hh"
#include "LIB_string.h"
#include "LIB_string_ref.hh"

#include "gpu_material_library.h"
#include "gpu_shader_create_info.hh"
#include "gpu_shader_dependency_private.h"

#include "gpu_material_library.h"

#include "GPU_context.h"
#include "GPU_material.h"

extern "C" {
#define SHADER_SOURCE(datatoc, filename, filepath) extern char datatoc[];
#include "gpu_shader_source_list.h"
#undef SHADER_SOURCE
}

namespace rose::gpu {

using GPUSourceDictionnary = Map<StringRef, struct GPUSource *>;
using GPUFunctionDictionnary = Map<StringRef, struct GPUFunction *>;

struct GPUSource {
	StringRefNull fullpath;
	StringRefNull filename;
	StringRefNull source;
	Vector<GPUSource *> dependencies;
	bool dependencies_init = false;
	shader::BuiltinBits builtins = shader::BuiltinBits::NONE;
	std::string processed_source;

	GPUSource(const char *path, const char *file, const char *datatoc, GPUFunctionDictionnary *g_functions) : fullpath(path), filename(file), source(datatoc) {
		/* Scan for builtins. */
		/* FIXME: This can trigger false positive caused by disabled #if blocks. */
		/* TODO(fclem): Could be made faster by scanning once. */
		if (source.find("gl_FragCoord", 0) != StringRef::not_found) {
			builtins |= shader::BuiltinBits::FRAG_COORD;
		}
		if (source.find("gl_FrontFacing", 0) != StringRef::not_found) {
			builtins |= shader::BuiltinBits::FRONT_FACING;
		}
		if (source.find("gl_GlobalInvocationID", 0) != StringRef::not_found) {
			builtins |= shader::BuiltinBits::GLOBAL_INVOCATION_ID;
		}
		if (source.find("gl_InstanceID", 0) != StringRef::not_found) {
			builtins |= shader::BuiltinBits::INSTANCE_ID;
		}
		if (source.find("gl_LocalInvocationID", 0) != StringRef::not_found) {
			builtins |= shader::BuiltinBits::LOCAL_INVOCATION_ID;
		}
		if (source.find("gl_LocalInvocationIndex", 0) != StringRef::not_found) {
			builtins |= shader::BuiltinBits::LOCAL_INVOCATION_INDEX;
		}
		if (source.find("gl_NumWorkGroup", 0) != StringRef::not_found) {
			builtins |= shader::BuiltinBits::NUM_WORK_GROUP;
		}
		if (source.find("gl_PointCoord", 0) != StringRef::not_found) {
			builtins |= shader::BuiltinBits::POINT_COORD;
		}
		if (source.find("gl_PointSize", 0) != StringRef::not_found) {
			builtins |= shader::BuiltinBits::POINT_SIZE;
		}
		if (source.find("gl_PrimitiveID", 0) != StringRef::not_found) {
			builtins |= shader::BuiltinBits::PRIMITIVE_ID;
		}
		if (source.find("gl_VertexID", 0) != StringRef::not_found) {
			builtins |= shader::BuiltinBits::VERTEX_ID;
		}
		if (source.find("gl_WorkGroupID", 0) != StringRef::not_found) {
			builtins |= shader::BuiltinBits::WORK_GROUP_ID;
		}
		if (source.find("gl_WorkGroupSize", 0) != StringRef::not_found) {
			builtins |= shader::BuiltinBits::WORK_GROUP_SIZE;
		}
		/* TODO(fclem): We could do that at compile time. */
		/* Limit to shared header files to avoid the temptation to use C++ syntax in .glsl files. */
		if (filename.endswith(".h") || filename.endswith(".hh")) {
			enum_preprocess();
			quote_preprocess();
		}
		else {
			if (source.find("'") != StringRef::not_found) {
				char_literals_preprocess();
			}
#ifndef NDEBUG
			if (source.find("drw_print") != StringRef::not_found) {
				string_preprocess();
			}
			if ((source.find("drw_debug_") != StringRef::not_found) &&
				/* Avoid this file as it is a false positive match (matches "drw_debug_print_buf"). */
				filename != StringRefNull("draw_debug_print_display_vert.glsl") &&
				/* Avoid these two files where it makes no sense to add the dependency. */
				!ELEM(filename, StringRefNull("common_debug_draw_lib.glsl"), StringRefNull("draw_debug_draw_display_vert.glsl"))) {
				builtins |= shader::BuiltinBits::USE_DEBUG_DRAW;
			}
#endif
			check_no_quotes();
		}

		if (is_from_material_library()) {
			material_functions_parse(g_functions);
		}
	};

	static bool is_in_comment(const StringRef &input, size_t offset) {
		if (input.rfind("/*", offset) != StringRef::not_found) {
			return input.rfind("/*", offset) > input.rfind("*/", offset);
		}
		if (input.rfind("//", offset) != StringRef::not_found) {
			return input.rfind("//", offset) > input.rfind("\n", offset);
		}
		return false;
	}

	template<bool check_whole_word = true, bool reversed = false, typename T> static size_t find_str(const StringRef &input, const T keyword, size_t offset = 0) {
		while (true) {
			if constexpr (reversed) {
				offset = input.rfind(keyword, offset);
			}
			else {
				offset = input.find(keyword, offset);
			}
			if (offset != StringRef::not_found && offset > 0) {
				if constexpr (check_whole_word) {
					/* Fix false positive if something has "enum" as suffix. */
					char previous_char = input[offset - 1];
					if (!ELEM(previous_char, '\n', '\t', ' ', ':', '(', ',')) {
						offset += (reversed) ? -1 : 1;
						continue;
					}
				}
				/* Fix case where the keyword is in a comment. */
				if (is_in_comment(input, offset)) {
					offset += (reversed) ? -1 : 1;
					continue;
				}
			}
			return offset;
		}
	}

#define find_keyword find_str<true, false>
#define rfind_keyword find_str<true, true>
#define find_token find_str<false, false>
#define rfind_token find_str<false, true>

	void print_error(const StringRef &input, size_t offset, const StringRef message) {
		StringRef sub = input.substr(0, offset);
		size_t line_number = std::count(sub.begin(), sub.end(), '\n') + 1;
		size_t line_end = input.find("\n", offset);
		size_t line_start = input.rfind("\n", offset) + 1;
		size_t char_number = offset - line_start + 1;

		/* TODO Use clog. */

		std::cerr << fullpath << ":" << line_number << ":" << char_number;

		std::cerr << " error: " << message << "\n";
		std::cerr << std::setw(5) << line_number << " | " << input.substr(line_start, line_end - line_start) << "\n";
		std::cerr << "      | ";
		for (size_t i = 0; i < char_number - 1; i++) {
			std::cerr << " ";
		}
		std::cerr << "^\n";
	}

#define CHECK(test_value, str, ofs, msg)            \
	if ((test_value) == StringRefBase::not_found) { \
		print_error(str, ofs, msg);                 \
		continue;                                   \
	}

	/**
	 * Some drivers completely forbid quote characters even in unused preprocessor directives.
	 * We fix the cases where we can't manually patch in `enum_preprocess()`.
	 * This check ensure none are present in non-patched sources. (see #97545)
	 */
	void check_no_quotes() {
#ifndef NDEBUG
		size_t pos = -1;
		do {
			pos = source.find('"', pos + 1);
			if (pos == StringRefBase::not_found) {
				break;
			}
			if (!is_in_comment(source, pos)) {
				print_error(source, pos, "Quote characters are forbidden in GLSL files");
			}
		} while (true);
#endif
	}

	/**
	 * Some drivers completely forbid string characters even in unused preprocessor directives.
	 * This fixes the cases we cannot manually patch: Shared headers #includes. (see #97545)
	 * TODO(fclem): This could be done during the datatoc step.
	 */
	void quote_preprocess() {
		if (source.find_first_of('"') == StringRefBase::not_found) {
			return;
		}

		processed_source = source;
		std::replace(processed_source.begin(), processed_source.end(), '"', ' ');

		source = processed_source.c_str();
	}

	/**
	 * Transform C,C++ enum declaration into GLSL compatible defines and constants:
	 *
	 * \code{.cpp}
	 * enum eMyEnum : uint32_t {
	 *   ENUM_1 = 0u,
	 *   ENUM_2 = 1u,
	 *   ENUM_3 = 2u,
	 * };
	 * \endcode
	 *
	 * or
	 *
	 * \code{.c}
	 * enum eMyEnum {
	 *   ENUM_1 = 0u,
	 *   ENUM_2 = 1u,
	 *   ENUM_3 = 2u,
	 * };
	 * \endcode
	 *
	 * becomes
	 *
	 * \code{.glsl}
	 * #define eMyEnum uint
	 * const uint ENUM_1 = 0u, ENUM_2 = 1u, ENUM_3 = 2u;
	 * \endcode
	 *
	 * IMPORTANT: This has some requirements:
	 * - Enums needs to have underlying types specified to uint32_t to make them usable in UBO/SSBO.
	 * - All values needs to be specified using constant literals to avoid compiler differences.
	 * - All values needs to have the 'u' suffix to avoid GLSL compiler errors.
	 */
	void enum_preprocess() {
		const StringRefNull input = source;
		std::string output;
		size_t cursor = -1;
		size_t last_pos = 0;
		const bool is_cpp = filename.endswith(".hh");

		while (true) {
			cursor = find_keyword(input, "enum ", cursor + 1);
			if (cursor == StringRefBase::not_found) {
				break;
			}
			/* Skip matches like `typedef enum myEnum myType;` */
			if (cursor >= 8 && input.substr(cursor - 8, 8) == rose::StringRefNull("typedef ")) {
				continue;
			}
			/* Output anything between 2 enums blocks. */
			output += input.substr(last_pos, cursor - last_pos);

			/* Extract enum type name. */
			size_t name_start = input.find(" ", cursor);

			size_t values_start = find_token(input, '{', cursor);
			CHECK(values_start, input, cursor, "Malformed enum class. Expected \'{\' after typename.");

			StringRef enum_name = input.substr(name_start, values_start - name_start);
			if (is_cpp) {
				size_t name_end = find_token(enum_name, ":");
				CHECK(name_end, input, name_start, "Expected \':\' after C++ enum name.");

				size_t underlying_type = find_keyword(enum_name, "uint32_t", name_end);
				CHECK(underlying_type, input, name_start, "C++ enums needs uint32_t underlying type.");

				enum_name = input.substr(name_start, name_end);
			}

			output += "#define " + enum_name + " uint\n";

			/* Extract enum values. */
			size_t values_end = find_token(input, '}', values_start);
			CHECK(values_end, input, cursor, "Malformed enum class. Expected \'}\' after values.");

			/* Skip opening brackets. */
			values_start += 1;

			StringRef enum_values = input.substr(values_start, values_end - values_start);

			/* Really poor check. Could be done better. */
			size_t token = find_token(enum_values, '{');
			size_t not_found = (token == StringRef::not_found) ? 0 : StringRef::not_found;
			CHECK(not_found, input, values_start + token, "Unexpected \'{\' token inside enum values.");

			/* Do not capture the comma after the last value (if present). */
			size_t last_equal = rfind_token(enum_values, '=', values_end);
			size_t last_comma = rfind_token(enum_values, ',', values_end);
			if (last_comma > last_equal) {
				enum_values = input.substr(values_start, last_comma);
			}

			output += "const uint " + enum_values;

			size_t semicolon_found = (input[values_end + 1] == ';') ? 0 : StringRef::not_found;
			CHECK(semicolon_found, input, values_end + 1, "Expected \';\' after enum type declaration.");

			/* Skip the curly bracket but not the semicolon. */
			cursor = last_pos = values_end + 1;
		}
		/* If nothing has been changed, do not allocate processed_source. */
		if (last_pos == 0) {
			return;
		}

		if (last_pos != 0) {
			output += input.substr(last_pos);
		}

		processed_source = output;
		source = processed_source.c_str();
	};

	void material_functions_parse(GPUFunctionDictionnary *g_functions) {
		const StringRefNull input = source;

		const char whitespace_chars[] = " \r\n\t";

		auto function_parse = [&](const StringRef input, size_t &cursor, StringRef &out_return_type, StringRef &out_name, StringRef &out_args) -> bool {
			cursor = find_keyword(input, "void ", cursor + 1);
			if (cursor == StringRef::not_found) {
				return false;
			}
			size_t arg_start = find_token(input, '(', cursor);
			if (arg_start == StringRef::not_found) {
				return false;
			}
			size_t arg_end = find_token(input, ')', arg_start);
			if (arg_end == StringRef::not_found) {
				return false;
			}
			size_t body_start = find_token(input, '{', arg_end);
			size_t next_semicolon = find_token(input, ';', arg_end);
			if (body_start != StringRef::not_found && next_semicolon != StringRef::not_found && body_start > next_semicolon) {
				/* Assert no prototypes but could also just skip them. */
				ROSE_assert_msg(false, "No prototypes allowed in node GLSL libraries.");
			}
			size_t name_start = input.find_first_not_of(whitespace_chars, input.find(' ', cursor));
			if (name_start == StringRef::not_found) {
				return false;
			}
			size_t name_end = input.find_last_not_of(whitespace_chars, arg_start);
			if (name_end == StringRef::not_found) {
				return false;
			}
			/* Only support void type for now. */
			out_return_type = "void";
			out_name = input.substr(name_start, name_end - name_start);
			out_args = input.substr(arg_start + 1, arg_end - (arg_start + 1));
			return true;
		};

		auto keyword_parse = [&](const StringRef str, size_t &cursor) -> StringRef {
			size_t keyword_start = str.find_first_not_of(whitespace_chars, cursor);
			if (keyword_start == StringRef::not_found) {
				/* No keyword found. */
				return str.substr(0, 0);
			}
			size_t keyword_end = str.find_first_of(whitespace_chars, keyword_start);
			if (keyword_end == StringRef::not_found) {
				/* Last keyword. */
				keyword_end = str.size();
			}
			cursor = keyword_end + 1;
			return str.substr(keyword_start, keyword_end - keyword_start);
		};

		auto arg_parse = [&](const StringRef str, size_t &cursor, StringRef &out_qualifier, StringRef &out_type, StringRef &out_name) -> bool {
			size_t arg_start = cursor + 1;
			if (arg_start >= str.size()) {
				return false;
			}
			cursor = find_token(str, ',', arg_start);
			if (cursor == StringRef::not_found) {
				/* Last argument. */
				cursor = str.size();
			}
			const StringRef arg = str.substr(arg_start, cursor - arg_start);

			size_t keyword_cursor = 0;
			out_qualifier = keyword_parse(arg, keyword_cursor);
			out_type = keyword_parse(arg, keyword_cursor);
			out_name = keyword_parse(arg, keyword_cursor);
			if (out_name.is_empty()) {
				/* No qualifier case. */
				out_name = out_type;
				out_type = out_qualifier;
				out_qualifier = arg.substr(0, 0);
			}
			return true;
		};

		size_t cursor = -1;
		StringRef func_return_type, func_name, func_args;
		while (function_parse(input, cursor, func_return_type, func_name, func_args)) {
			/* Main functions needn't be handled because they are the entry point of the shader. */
			if (func_name == StringRefNull("main")) {
				continue;
			}

			GPUFunction *func = MEM_new<GPUFunction>(__func__);
			func_name.copy(func->name, sizeof(func->name));
			func->source = reinterpret_cast<void *>(this);

			bool insert = g_functions->add(func->name, func);

			/* NOTE: We allow overloading non void function, but only if the function comes from the
			 * same file. Otherwise the dependency system breaks. */
			if (!insert) {
				GPUSource *other_source = reinterpret_cast<GPUSource *>(g_functions->lookup(func_name)->source);
				if (other_source != this) {
					print_error(input, source.find(func_name), "Function redefinition or overload in two different files ...");
					print_error(input, other_source->source.find(func_name), "... previous definition was here");
				}
				else {
					/* Non-void function overload. */
					MEM_delete(func);
				}
				continue;
			}

			if (func_return_type != "void") {
				continue;
			}

			func->totparam = 0;
			size_t args_cursor = StringRef::not_found;
			StringRef arg_qualifier, arg_type, arg_name;
			while (arg_parse(func_args, args_cursor, arg_qualifier, arg_type, arg_name)) {

				if (func->totparam >= ARRAY_SIZE(func->paramtype)) {
					print_error(input, source.find(func_name), "Too much parameter in function");
					break;
				}

				auto parse_qualifier = [](StringRef qualifier) -> GPUFunctionQual {
					if (qualifier == "out") {
						return FUNCTION_QUAL_OUT;
					}
					if (qualifier == "inout") {
						return FUNCTION_QUAL_INOUT;
					}
					return FUNCTION_QUAL_IN;
				};

				auto parse_type = [](StringRef type) -> GPUParameterType {
					if (type == "float") {
						return GPU_FLOAT;
					}
					if (type == "vec2") {
						return GPU_VEC2;
					}
					if (type == "vec3") {
						return GPU_VEC3;
					}
					if (type == "vec4") {
						return GPU_VEC4;
					}
					if (type == "mat3") {
						return GPU_MAT3;
					}
					if (type == "mat4") {
						return GPU_MAT4;
					}
					if (type == "sampler1DArray") {
						return GPU_TEX1D_ARRAY;
					}
					if (type == "sampler2DArray") {
						return GPU_TEX2D_ARRAY;
					}
					if (type == "sampler2D") {
						return GPU_TEX2D;
					}
					if (type == "sampler3D") {
						return GPU_TEX3D;
					}
					if (type == "Closure") {
						return GPU_CLOSURE;
					}
					return GPU_NONE;
				};

				func->paramqual[func->totparam] = parse_qualifier(arg_qualifier);
				func->paramtype[func->totparam] = parse_type(arg_type);

				if (func->paramtype[func->totparam] == GPU_NONE) {
					std::string err = "Unknown parameter type \"" + arg_type + "\"";
					size_t err_ofs = source.find(func_name);
					err_ofs = find_keyword(source, arg_name, err_ofs);
					err_ofs = rfind_keyword(source, arg_type, err_ofs);
					print_error(input, err_ofs, err);
				}

				func->totparam++;
			}
		}
	}

	void char_literals_preprocess() {
		const StringRefNull input = source;
		std::stringstream output;
		size_t cursor = -1;
		size_t last_pos = 0;

		while (true) {
			cursor = find_token(input, '\'', cursor + 1);
			if (cursor == StringRef::not_found) {
				break;
			}
			/* Output anything between 2 print statement. */
			output << input.substr(last_pos, cursor - last_pos);

			/* Extract string. */
			size_t char_start = cursor + 1;
			size_t char_end = find_token(input, '\'', char_start);
			CHECK(char_end, input, cursor, "Malformed char literal. Missing ending `'`.");

			StringRef input_char = input.substr(char_start, char_end - char_start);
			if (input_char.is_empty()) {
				CHECK(StringRef::not_found, input, cursor, "Malformed char literal. Empty character constant");
			}

			uint8_t char_value = input_char[0];

			if (input_char[0] == '\\') {
				if (input_char[1] == 'n') {
					char_value = '\n';
				}
				else {
					CHECK(StringRef::not_found, input, cursor, "Unsupported escaped character");
				}
			}
			else {
				if (input_char.size() > 1) {
					CHECK(StringRef::not_found, input, cursor, "Malformed char literal. Multi-character character constant");
				}
			}

			char hex[8];
			LIB_strnformat(hex, ARRAY_SIZE(hex), "0x%.2Xu", char_value);
			output << hex;

			cursor = last_pos = char_end + 1;
		}
		/* If nothing has been changed, do not allocate processed_source. */
		if (last_pos == 0) {
			return;
		}

		if (last_pos != 0) {
			output << input.substr(last_pos);
		}
		processed_source = output.str();
		source = processed_source.c_str();
	}

	/* Replace print(string) by equivalent drw_print_char4() sequence. */
	void string_preprocess() {
		const StringRefNull input = source;
		std::stringstream output;
		size_t cursor = -1;
		size_t last_pos = 0;

		while (true) {
			cursor = find_keyword(input, "drw_print", cursor + 1);
			if (cursor == StringRef::not_found) {
				break;
			}

			bool do_endl = false;
			StringRef func = input.substr(cursor);
			if (func.startswith("drw_print(")) {
				do_endl = true;
			}
			else if (func.startswith("drw_print_no_endl(")) {
				do_endl = false;
			}
			else {
				continue;
			}

			/* Output anything between 2 print statement. */
			output << input.substr(last_pos, cursor - last_pos);

			/* Extract string. */
			size_t str_start = input.find('(', cursor) + 1;
			size_t semicolon = find_token(input, ';', str_start + 1);
			CHECK(semicolon, input, cursor, "Malformed print(). Missing `;` .");
			size_t str_end = rfind_token(input, ')', semicolon);
			if (str_end < str_start) {
				CHECK(StringRef::not_found, input, cursor, "Malformed print(). Missing closing `)` .");
			}

			std::stringstream sub_output;
			StringRef input_args = input.substr(str_start, str_end - str_start);

			auto print_string = [&](std::string str) -> int {
				size_t len_before_pad = str.length();
				/* Pad string to uint size. */
				while (str.length() % 4 != 0) {
					str += " ";
				}
				/* Keep everything in one line to not mess with the shader logs. */
				sub_output << "/* " << str << "*/";
				sub_output << "drw_print_string_start(" << len_before_pad << ");";
				for (size_t i = 0; i < len_before_pad; i += 4) {
					uint8_t chars[4] = {*(reinterpret_cast<const uint8_t *>(str.c_str()) + i + 0), *(reinterpret_cast<const uint8_t *>(str.c_str()) + i + 1), *(reinterpret_cast<const uint8_t *>(str.c_str()) + i + 2), *(reinterpret_cast<const uint8_t *>(str.c_str()) + i + 3)};
					if (i + 4 > len_before_pad) {
						chars[len_before_pad - i] = '\0';
					}
					char uint_hex[12];
					LIB_strnformat(uint_hex, ARRAY_SIZE(uint_hex), "0x%.2X%.2X%.2X%.2Xu", chars[3], chars[2], chars[1], chars[0]);
					sub_output << "drw_print_char4(" << StringRefNull(uint_hex) << ");";
				}
				return 0;
			};

			std::string func_args = input_args;
			/* Workaround to support function call inside prints. We replace commas by a non control
			 * character `$` in order to use simpler regex later. */
			bool string_scope = false;
			int func_scope = 0;
			for (char &c : func_args) {
				if (c == '"') {
					string_scope = !string_scope;
				}
				else if (!string_scope) {
					if (c == '(') {
						func_scope++;
					}
					else if (c == ')') {
						func_scope--;
					}
					else if (c == ',' && func_scope != 0) {
						c = '$';
					}
				}
			}

			const bool print_as_variable = (input_args[0] != '"') && find_token(input_args, ',') == StringRef::not_found;
			if (print_as_variable) {
				/* Variable or expression debugging. */
				std::string arg = input_args;
				/* Pad align most values. */
				while (arg.length() % 4 != 0) {
					arg += " ";
				}
				print_string(arg);
				print_string("= ");
				sub_output << "drw_print_value(" << input_args << ");";
			}
			else {
				const std::regex arg_regex(
					/* String args. */
					"[\\s]*\"([^\r\n\t\f\v\"]*)\""
					/* OR. */
					"|"
					/* value args. */
					"([^,]+)");
				std::smatch args_match;
				std::string::const_iterator args_search_start(func_args.cbegin());
				while (std::regex_search(args_search_start, func_args.cend(), args_match, arg_regex)) {
					args_search_start = args_match.suffix().first;
					std::string arg_string = args_match[1].str();
					std::string arg_val = args_match[2].str();

					if (arg_string.empty()) {
						for (char &c : arg_val) {
							if (c == '$') {
								c = ',';
							}
						}
						sub_output << "drw_print_value(" << arg_val << ");";
					}
					else {
						print_string(arg_string);
					}
				}
			}

			if (do_endl) {
				sub_output << "drw_print_newline();";
			}

			output << sub_output.str();

			cursor = last_pos = str_end + 1;
		}
		/* If nothing has been changed, do not allocate processed_source. */
		if (last_pos == 0) {
			return;
		}

		if (filename != "common_debug_print_lib.glsl") {
			builtins |= shader::BuiltinBits::USE_DEBUG_PRINT;
		}

		if (last_pos != 0) {
			output << input.substr(last_pos);
		}
		processed_source = output.str();
		source = processed_source.c_str();
	}

#undef find_keyword
#undef rfind_keyword
#undef find_token
#undef rfind_token

	/* Return 1 one error. */
	int init_dependencies(const GPUSourceDictionnary &dict, const GPUFunctionDictionnary &g_functions) {
		if (this->dependencies_init) {
			return 0;
		}
		this->dependencies_init = true;
		size_t pos = -1;

		using namespace shader;
		/* Auto dependency injection for debug capabilities. */
		if ((builtins & BuiltinBits::USE_DEBUG_DRAW) == BuiltinBits::USE_DEBUG_DRAW) {
			dependencies.append_non_duplicates(dict.lookup("common_debug_draw_lib.glsl"));
		}
		if ((builtins & BuiltinBits::USE_DEBUG_PRINT) == BuiltinBits::USE_DEBUG_PRINT) {
			dependencies.append_non_duplicates(dict.lookup("common_debug_print_lib.glsl"));
		}

		while (true) {
			GPUSource *dependency_source = nullptr;

			{
				pos = source.find("pragma ROSE_REQUIRE(", pos + 1);
				if (pos == StringRef::not_found) {
					return 0;
				}
				size_t start = source.find('(', pos) + 1;
				size_t end = source.find(')', pos);
				if (end == StringRef::not_found) {
					print_error(source, start, "Malformed ROSE_REQUIRE: Missing \")\" token");
					return 1;
				}
				StringRef dependency_name = source.substr(start, end - start);
				dependency_source = dict.lookup_default(dependency_name, nullptr);
				if (dependency_source == nullptr) {
					print_error(source, start, "Dependency not found");
					return 1;
				}
			}

			/* Recursive. */
			int result = dependency_source->init_dependencies(dict, g_functions);
			if (result != 0) {
				return 1;
			}

			for (auto *dep : dependency_source->dependencies) {
				dependencies.append_non_duplicates(dep);
			}
			dependencies.append_non_duplicates(dependency_source);
		}
		/* Precedes an eternal loop (quiet CLANG's `unreachable-code` warning). */
		ROSE_assert_unreachable();
		return 0;
	}

	/* Returns the final string with all includes done. */
	void build(Vector<const char *> &result) const {
		for (auto *dep : dependencies) {
			result.append(dep->source.c_str());
		}
		result.append(source.c_str());
	}

	shader::BuiltinBits builtins_get() const {
		shader::BuiltinBits out_builtins = builtins;
		for (auto *dep : dependencies) {
			out_builtins |= dep->builtins;
		}
		return out_builtins;
	}

	bool is_from_material_library() const {
		return (filename.startswith("gpu_shader_material_") || filename.startswith("gpu_shader_common_") || filename.startswith("gpu_shader_compositor_")) && filename.endswith(".glsl");
	}
};

}  // namespace rose::gpu

using namespace rose::gpu;

static GPUSourceDictionnary *g_sources = nullptr;
static GPUFunctionDictionnary *g_functions = nullptr;

void gpu_shader_dependency_init() {
	g_sources = MEM_new<GPUSourceDictionnary>("rose::gpu::GlobalSourceDictionary");
	g_functions = MEM_new<GPUFunctionDictionnary>("rose::gpu::GlobalFunctionDictionary");

#define SHADER_SOURCE(datatoc, filename, filepath) g_sources->add_new(filename, MEM_new<GPUSource>("rose::gpu::Source", filepath, filename, datatoc, g_functions));
#include "gpu_shader_source_list.h"
#undef SHADER_SOURCE

	int errors = 0;
	for (auto *value : g_sources->values()) {
		errors += value->init_dependencies(*g_sources, *g_functions);
	}
	ROSE_assert_msg(errors == 0, "Dependency errors detected: Aborting");
	UNUSED_VARS_NDEBUG(errors);
}

void gpu_shader_dependency_exit() {
	for (auto *value : g_sources->values()) {
		MEM_delete(value);
	}
	for (auto *value : g_functions->values()) {
		MEM_delete(value);
	}
	MEM_delete<GPUSourceDictionnary>(g_sources);
	MEM_delete<GPUFunctionDictionnary>(g_functions);
}

GPUFunction *gpu_material_library_use_function(GSet *used_libraries, const char *name) {
	GPUFunction *function = g_functions->lookup_default(name, nullptr);
	ROSE_assert_msg(function != nullptr, "Requested function not in the function library");
	GPUSource *source = reinterpret_cast<GPUSource *>(function->source);
	LIB_gset_add(used_libraries, const_cast<char *>(source->filename.c_str()));
	return function;
}

namespace rose::gpu::shader {

BuiltinBits gpu_shader_dependency_get_builtins(const StringRefNull shader_source_name) {
	if (shader_source_name.is_empty()) {
		return shader::BuiltinBits::NONE;
	}
	if (g_sources->contains(shader_source_name) == false) {
		std::cerr << "Error: Could not find \"" << shader_source_name << "\" in the list of registered source.\n";
		ROSE_assert(0);
		return shader::BuiltinBits::NONE;
	}
	GPUSource *source = g_sources->lookup(shader_source_name);
	return source->builtins_get();
}

Vector<const char *> gpu_shader_dependency_get_resolved_source(const StringRefNull shader_source_name) {
	Vector<const char *> result;
	GPUSource *src = g_sources->lookup_default(shader_source_name, nullptr);
	if (src == nullptr) {
		std::cerr << "Error source not found : " << shader_source_name << std::endl;
	}
	src->build(result);
	return result;
}

StringRefNull gpu_shader_dependency_get_source(const StringRefNull shader_source_name) {
	GPUSource *src = g_sources->lookup_default(shader_source_name, nullptr);
	if (src == nullptr) {
		std::cerr << "Error source not found : " << shader_source_name << std::endl;
	}
	return src->source;
}

StringRefNull gpu_shader_dependency_get_filename_from_source_string(const StringRefNull source_string) {
	for (auto &source : g_sources->values()) {
		if (source->source.c_str() == source_string.c_str()) {
			return source->filename;
		}
	}
	return "";
}

}  // namespace rose::gpu::shader
