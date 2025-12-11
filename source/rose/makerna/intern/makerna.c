#include "MEM_guardedalloc.h"

#include "LIB_string.h"
#include "LIB_path_utils.h"

#include "RNA_define.h"
#include "RNA_types.h"

#include "rna_internal_types.h"
#include "rna_internal.h"

#include <limits.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

extern RoseDefRNA DefRNA;

typedef struct RNAProcessItem {
	const char *filename;
	const char *api_filename;
	void (*define)(RoseRNA *brna);
} RNAProcessItem;

RNAProcessItem RNAItems[] = {
	{.filename = "rna_rna.c", .define = RNA_def_rna},
	{.filename = "rna_id.c", .define = RNA_def_ID},
	{.filename = "rna_ob.c", .define = RNA_def_Object},
	{.filename = "rna_pose.c", .define = RNA_def_Pose},
};

ROSE_INLINE void rna_print_c_float(FILE *f, float num) {
	if (num == -FLT_MAX) {
		fprintf(f, "-FLT_MAX");
	}
	else if (num == FLT_MAX) {
		fprintf(f, "FLT_MAX");
	}
	else if (fabsf(num) < INT64_MAX && ((int64_t)num) == num) {
		fprintf(f, "%.1ff", num);
	}
	else if (num == INFINITY) {
		fprintf(f, "INFINITY");
	}
	else if (num == -INFINITY) {
		fprintf(f, "-INFINITY");
	}
	else {
		fprintf(f, "%.10ff", num);
	}
}

ROSE_INLINE void rna_int_print(FILE *f, int64_t num) {
	if (num == INT_MIN) {
		fprintf(f, "INT_MIN");
	}
	else if (num == INT_MAX) {
		fprintf(f, "INT_MAX");
	}
	else if (num == INT64_MIN) {
		fprintf(f, "INT64_MIN");
	}
	else if (num == INT64_MAX) {
		fprintf(f, "INT64_MAX");
	}
	else if (num < INT_MIN || num > INT_MAX) {
		fprintf(f, "%" PRId64 "LL", num);
	}
	else {
		fprintf(f, "%d", (int)num);
	}
}

ROSE_INLINE void rna_print_property_flag(FILE *f, ePropertyFlag flag) {
	ePropertyFlag val[] = {
		PROP_ANIMATABLE,
		PROP_EDITABLE,
		PROP_NEVER_NULL,
		PROP_PTR_NO_OWNERSHIP,
		PROP_IDPROPERTY,
		PROP_ID_REFCOUNT,
	};
	const char *name[] = {
		"PROP_ANIMATABLE",
		"PROP_EDITABLE",
		"PROP_NEVER_NULL",
		"PROP_PTR_NO_OWNERSHIP",
		"PROP_IDPROPERTY",
		"PROP_ID_REFCOUNT",
	};

	int tmp = 0;

	size_t preceding = 0;
	for (size_t index = 0; index < ARRAY_SIZE(val); index++) {
		if ((flag & val[index]) == val[index]) {
			fprintf(f, "%s%s", (preceding++ > 0) ? " | " : "", name[index]);

			tmp |= (int)val[index];
		}
	}

	ROSE_assert(tmp == flag);

	if (!preceding) {
		fprintf(f, "0");
	}
}

ROSE_INLINE void rna_print_struct_flag(FILE *f, eStructFlag flag) {
	eStructFlag val[] = {
		STRUCT_ID,
		STRUCT_ID_REFCOUNT,
		STRUCT_UNDO,
		STRUCT_PUBLIC_NAMESPACE,
		STRUCT_PUBLIC_NAMESPACE_INHERIT,
		STRUCT_RUNTIME,
		STRUCT_FREE_POINTERS,
	};
	const char *name[] = {
		"STRUCT_ID",
		"STRUCT_ID_REFCOUNT",
		"STRUCT_UNDO",
		"STRUCT_PUBLIC_NAMESPACE",
		"STRUCT_PUBLIC_NAMESPACE_INHERIT",
		"STRUCT_FREE_POINTERS",
	};

	int tmp = 0;

	size_t preceding = 0;
	for (size_t index = 0; index < ARRAY_SIZE(val); index++) {
		if ((flag & val[index]) == val[index]) {
			fprintf(f, "%s%s", (preceding++ > 0) ? " | " : "", name[index]);

			tmp |= (int)val[index];
		}
	}

	ROSE_assert(tmp == flag);

	if (!preceding) {
		fprintf(f, "0");
	}
}

ROSE_INLINE void rna_print_c_string(FILE *fpout, const char *str) {
	static const char *escape[] = {"\''", "\"\"", "\??", "\\\\", "\aa", "\bb", "\ff", "\nn", "\rr", "\tt", "\vv", NULL};
	int i, j;

	if (!str) {
		fprintf(fpout, "NULL");
		return;
	}

	fprintf(fpout, "\"");
	for (i = 0; str[i]; i++) {
		for (j = 0; escape[j]; j++) {
			if (str[i] == escape[j][0]) {
				break;
			}
		}

		if (escape[j]) {
			fprintf(fpout, "\\%c", escape[j][1]);
		}
		else {
			fprintf(fpout, "%c", str[i]);
		}
	}
	fprintf(fpout, "\"");
}

ROSE_INLINE const char *rna_property_structname(ePropertyType type) {
	/* clang-format off */
	switch (type) {
		case PROP_BOOLEAN: return "BoolPropertyRNA";
		case PROP_INT: return "IntPropertyRNA";
		case PROP_FLOAT: return "FloatPropertyRNA";
		case PROP_STRING: return "StringPropertyRNA";
		case PROP_ENUM: return "EnumPropertyRNA";
		case PROP_POINTER: return "PointerPropertyRNA";
		case PROP_COLLECTION: return "CollectionPropertyRNA";
		default: return "UnknownPropertyRNA";
	}
	/* clang-format on */
}

ROSE_INLINE StructRNA *rna_find_struct(const char *structname) {
	LISTBASE_FOREACH(StructDefRNA *, defstruct, &DefRNA.structs) {
		if (STREQ(defstruct->ptr->identifier, structname)) {
			return defstruct->ptr;
		}
	}
	return NULL;
}

ROSE_INLINE const char *rna_function_string(void *ptr) {
	return (ptr) ? (const char *)ptr : "NULL";
}

ROSE_INLINE void rna_construct_function_name(char *buffer, int size, const char *structname, const char *propname, const char *type) {
	LIB_strnformat(buffer, size, "%s_%s_%s", structname, propname, type);
}

void *rna_alloc_from_buffer(const char *buffer, int buffer_size) {
	AllocDefRNA *alloc = MEM_mallocN(sizeof(AllocDefRNA), "AllocDefRNA");
	alloc->mem = MEM_mallocN(buffer_size, __func__);
	memcpy(alloc->mem, buffer, buffer_size);
	LIB_addtail(&DefRNA.allocs, alloc);
	return alloc->mem;
}

ROSE_INLINE const char *rna_safe_id(const char *id) {
	if (STREQ(id, "new")) {
		return "create";
	}

	return id;
}

ROSE_INLINE char *rna_alloc_function_name(const char *structname, const char *propname, const char *type) {
	char buffer[2048];
	rna_construct_function_name(buffer, sizeof(buffer), structname, propname, type);
	return (char *)(rna_alloc_from_buffer(buffer, LIB_strlen(buffer) + 1));
}

ROSE_INLINE void rna_print_data_get(FILE *f, PropertyDefRNA *dp) {
	if (dp->dnastructfromname && dp->dnastructfromprop) {
		fprintf(f, "\t%s *data = (%s *)(((%s *)ptr->data)->%s);\n", dp->dnastructname, dp->dnastructname, dp->dnastructfromname, dp->dnastructfromprop);
	}
	else {
		fprintf(f, "\t%s *data = (%s *)(ptr->data);\n", dp->dnastructname, dp->dnastructname);
	}
}

ROSE_INLINE const char *rna_type_type_name(PropertyRNA *property) {
	switch (property->type) {
		case PROP_BOOLEAN: return "bool";
		case PROP_INT: return "int";
		case PROP_FLOAT: return "float";
		case PROP_STRING: return "const char *";
	}

	return NULL;
}

ROSE_INLINE const char *rna_type_type(PropertyRNA *property) {
	const char *ret;
	if ((ret = rna_type_type_name(property))) {
		return ret;
	}

	return "PointerRNA";
}

ROSE_INLINE bool rna_color_quantize(PropertyRNA *prop, PropertyDefRNA *dp) {
	return ((prop->type == PROP_FLOAT) && ELEM(prop->subtype, PROP_COLOR) && (IS_DNATYPE_FLOAT_COMPAT(dp->dnatype) == 0));
}

ROSE_INLINE char *rna_def_property_get_func(FILE *fpout, StructRNA *srna, PropertyRNA *p, PropertyDefRNA *dp, const char *manual) {
	char *func = NULL;

	if (p->flag & PROP_IDPROPERTY && manual == NULL) {
		return NULL;
	}

	if (!manual) {
		if (!dp->dnastructname || !dp->dnaname) {
			fprintf(stderr, "[RNA] \"%s.%s\" has no valid dna info.\n", srna->identifier, p->identifier);
			DefRNA.error = true;
			return NULL;
		}
	}

	func = rna_alloc_function_name(srna->identifier, rna_safe_id(p->identifier), "get");

	/* Type check. */
	if (dp->dnatype) {
		switch (p->type) {
			case PROP_FLOAT: {
				if (!IS_DNATYPE_FLOAT_COMPAT(dp->dnatype)) {
					fprintf(stderr, "[RNA] \"%s.%s\" is not a float in DNA.\n", srna->identifier, p->identifier);
					DefRNA.error = true;
					return NULL;
				}
			} break;
		}
	}

	switch (p->type) {
		case PROP_STRING: {
			StringPropertyRNA *sproperty = (StringPropertyRNA *)p;

			fprintf(fpout, "extern void %s(PointerRNA *ptr, char *value) {\n", func);
			if (manual) {
				fprintf(fpout, "\tPropStringGetFunc fn = %s;\n", manual);
				fprintf(fpout, "\tfn(ptr, value);\n");
			}
			else {
				rna_print_data_get(fpout, dp);
				
				if (dp->dnapointerlevel == 1) {
					/* Handle allocated char pointer properties. */
					fprintf(fpout, "\tif (data->%s == nullptr) {\n", dp->dnaname);
					fprintf(fpout, "\t\t*value = '\\0';\n");
					fprintf(fpout, "\t\treturn;\n");
					fprintf(fpout, "\t}\n");
					fprintf(fpout, "\tstrcpy(value, data->%s);\n", dp->dnaname);
				}
				else {
					/* Handle char array properties. */
					fprintf(fpout, "\tstrcpy(value, data->%s);\n", dp->dnaname);
				}
			}
			fprintf(fpout, "}\n\n");
		} break;
		case PROP_POINTER: {
			PointerPropertyRNA *pproperty = (PointerPropertyRNA *)p;

			fprintf(fpout, "extern PointerRNA %s(PointerRNA *ptr) {\n", func);
			rna_print_data_get(fpout, dp);
			if (dp->dnapointerlevel == 0) {
				fprintf(fpout, "\treturn RNA_pointer_create_with_parent(ptr, &RNA_%s, &data->%s);\n", (const char *)pproperty->srna, dp->dnaname);
			}
			else {
				fprintf(fpout, "\treturn RNA_pointer_create_with_parent(ptr, &RNA_%s, data->%s);\n", (const char *)pproperty->srna, dp->dnaname);
			}
			fprintf(fpout, "}\n\n");
		} break;
		case PROP_COLLECTION: {
			CollectionPropertyRNA *cproperty = (CollectionPropertyRNA *)p;

			fprintf(fpout, "static PointerRNA %s(CollectionPropertyIterator *iter) {\n", func);
			if (manual) {
				if (STREQ(manual, "rna_iterator_listbase_get")) {
					fprintf(fpout, "\treturn RNA_pointer_create_with_parent(&iter->parent, &RNA_%s, %s(iter));\n", (cproperty->element) ? (const char *)cproperty->element : "UnknownType", manual);
				}
				else {
					fprintf(fpout, "\tPropCollectionGetFunc fn = %s;\n", manual);
					fprintf(fpout, "\treturn fn(iter);\n");
				}
			}
			fprintf(fpout, "}\n\n");
			break;
		} break;
		default: {
			if (p->arraydimension) {
				fprintf(fpout, "extern void %s(PointerRNA *ptr, %s values[%u]) {\n", func, rna_type_type(p), p->totarraylength);

				if (manual) {
					/* Assign `fn` to ensure function signatures match. */
					if (p->type == PROP_BOOLEAN) {
						fprintf(fpout, "\tPropBooleanArrayGetFunc fn = %s;\n", manual);
						fprintf(fpout, "\tfn(ptr, values);\n");
					}
					else if (p->type == PROP_INT) {
						fprintf(fpout, "\tPropIntArrayGetFunc fn = %s;\n", manual);
						fprintf(fpout, "\tfn(ptr, values);\n");
					}
					else if (p->type == PROP_FLOAT) {
						fprintf(fpout, "\tPropFloatArrayGetFunc fn = %s;\n", manual);
						fprintf(fpout, "\tfn(ptr, values);\n");
					}
					else {
						ROSE_assert_unreachable(); /* Valid but should be handled by type checks. */
						fprintf(fpout, "\t%s(ptr, values);\n", manual);
					}
				}
				else {
					rna_print_data_get(fpout, dp);

					fprintf(fpout, "\tunsigned int i;\n\n");
					fprintf(fpout, "\tfor (i = 0; i < %u; i++) {\n", p->totarraylength);

					if (dp->dnaarraylength == 1) {
						if (p->type == PROP_BOOLEAN && dp->booleanbit) {
							fprintf(fpout, "\t\tvalues[i] = %s((data->%s & (", (dp->booleannegative) ? "!" : "", dp->dnaname);
							rna_int_print(fpout, dp->booleanbit);
							fprintf(fpout, " << i)) != 0);\n");
						}
						else {
							fprintf(fpout, "\t\tvalues[i] = (%s)%s((&data->%s)[i]);\n", rna_type_type(p), (dp->booleannegative) ? "!" : "", dp->dnaname);
						}
					}
					else {
						if (p->type == PROP_BOOLEAN && dp->booleanbit) {
							fprintf(fpout, "\t\tvalues[i] = %s((data->%s[i] & ", (dp->booleannegative) ? "!" : "", dp->dnaname);
							rna_int_print(fpout, dp->booleanbit);
							fprintf(fpout, ") != 0);\n");
						}
						else if (rna_color_quantize(p, dp)) {
							fprintf(fpout, "\t\tvalues[i] = (%s)(data->%s[i] * (1.0f / 255.0f));\n", rna_type_type(p), dp->dnaname);
						}
						else {
							fprintf(fpout, "\t\tvalues[i] = (%s)%s((data->%s)[i]);\n", rna_type_type(p), (dp->booleannegative) ? "!" : "", dp->dnaname);
						}
					}
					fprintf(fpout, "\t}\n");
				}
				fprintf(fpout, "}\n\n");
			}
			else {
				fprintf(fpout, "extern %s %s(PointerRNA *ptr) {\n", rna_type_type(p), func);

				if (manual) {
					/* Assign `fn` to ensure function signatures match. */
					if (p->type == PROP_BOOLEAN) {
						fprintf(fpout, "\tPropBooleanGetFunc fn = %s;\n", manual);
						fprintf(fpout, "\treturn fn(ptr);\n");
					}
					else if (p->type == PROP_INT) {
						fprintf(fpout, "\tPropIntGetFunc fn = %s;\n", manual);
						fprintf(fpout, "\treturn fn(ptr);\n");
					}
					else if (p->type == PROP_FLOAT) {
						fprintf(fpout, "\tPropFloatGetFunc fn = %s;\n", manual);
						fprintf(fpout, "\treturn fn(ptr);\n");
					}
					else if (p->type == PROP_ENUM) {
						fprintf(fpout, "\tPropEnumGetFunc fn = %s;\n", manual);
						fprintf(fpout, "\treturn fn(ptr);\n");
					}
					else {
						/* Valid but should be handled by type checks. */
						ROSE_assert_unreachable();
						fprintf(fpout, "\treturn %s(ptr);\n", manual);
					}
				}
				else {
					rna_print_data_get(fpout, dp);
					if (p->type == PROP_BOOLEAN && dp->booleanbit) {
						fprintf(fpout, "\treturn %s(((data->%s) & ", (dp->booleannegative) ? "!" : "", dp->dnaname);
						rna_int_print(fpout, dp->booleanbit);
						fprintf(fpout, ") != 0);\n");
					}
					else {
						fprintf(fpout, "\treturn (%s)%s(data->%s);\n", rna_type_type(p), (dp->booleannegative) ? "!" : "", dp->dnaname);
					}
				}

				fprintf(fpout, "}\n\n");
			}
		} break;
	}

	return func;
}

ROSE_INLINE char *rna_def_property_length_func(FILE *fpout, StructRNA *srna, PropertyRNA *p, PropertyDefRNA *dp, const char *manual) {
	char *func = NULL;

	if (p->flag & PROP_IDPROPERTY && manual == NULL) {
		return NULL;
	}

	if (!manual) {
		if (!dp->dnastructname || !dp->dnaname) {
			fprintf(stderr, "[RNA] \"%s.%s\" has no valid dna info.\n", srna->identifier, p->identifier);
			DefRNA.error = true;
			return NULL;
		}
	}

	func = rna_alloc_function_name(srna->identifier, rna_safe_id(p->identifier), "length");

	switch (p->type) {
		case PROP_STRING: {
			StringPropertyRNA *pproperty = (StringPropertyRNA *)p;
			
			if (!manual) {
				if (!dp->dnastructname || !dp->dnaname) {
					fprintf(stderr, "[RNA] \"%s.%s\" has no valid dna info.\n", srna->identifier, p->identifier);
					DefRNA.error = true;
					return NULL;
				}
			}

			fprintf(fpout, "extern int %s(PointerRNA *ptr) {\n", func);
			if (manual) {
				fprintf(fpout, "\tPropStringLengthFunc fn = %s;\n", manual);
				fprintf(fpout, "\treturn fn(ptr);\n");
			}
			else {
				rna_print_data_get(fpout, dp);
				if (dp->dnapointerlevel == 1) {
					/* Handle allocated char pointer properties. */
					fprintf(fpout, "\treturn (data->%s == NULL) ? 0 : strlen(data->%s);\n", dp->dnaname, dp->dnaname);
				}
				else {
					/* Handle char array properties. */
					fprintf(fpout, "\treturn strlen(data->%s);\n", dp->dnaname);
				}
			}
			fprintf(fpout, "}\n\n");
		} break;
	}

	return func;
}

/* defined min/max variables to be used by rna_clamp_value() */
ROSE_INLINE void rna_clamp_value_range(FILE *f, PropertyRNA *prop) {
	if (prop->type == PROP_FLOAT) {
		FloatPropertyRNA *fprop = (FloatPropertyRNA *)prop;
		if (fprop->range) {
			fprintf(f, "\tfloat prop_clamp_min = -FLT_MAX, prop_clamp_max = FLT_MAX, prop_soft_min, prop_soft_max;\n");
			fprintf(f, "\t%s(ptr, &prop_clamp_min, &prop_clamp_max, &prop_soft_min, &prop_soft_max);\n", rna_function_string(fprop->range));
		}
	}
	else if (prop->type == PROP_INT) {
		IntPropertyRNA *iprop = (IntPropertyRNA *)prop;
		if (iprop->range) {
			fprintf(f, "\tint prop_clamp_min = INT_MIN, prop_clamp_max = INT_MAX, prop_soft_min, prop_soft_max;\n");
			fprintf(f, "\t%s(ptr, &prop_clamp_min, &prop_clamp_max, &prop_soft_min, &prop_soft_max);\n", rna_function_string(iprop->range));
		}
	}
}

ROSE_INLINE void rna_clamp_value(FILE *f, PropertyRNA *prop, int array) {
	if (prop->type == PROP_INT) {
		IntPropertyRNA *iprop = (IntPropertyRNA *)prop;

		if (iprop->hardmin != INT_MIN || iprop->hardmax != INT_MAX || iprop->range) {
			if (array) {
				fprintf(f, "std::clamp(values[i], ");
			}
			else {
				fprintf(f, "std::clamp(value, ");
			}
			if (iprop->range) {
				fprintf(f, "prop_clamp_min, prop_clamp_max);\n");
			}
			else {
				rna_int_print(f, iprop->hardmin);
				fprintf(f, ", ");
				rna_int_print(f, iprop->hardmax);
				fprintf(f, ");\n");
			}
			return;
		}
	}
	else if (prop->type == PROP_FLOAT) {
		FloatPropertyRNA *fprop = (FloatPropertyRNA *)prop;

		if (fprop->hardmin != -FLT_MAX || fprop->hardmax != FLT_MAX || fprop->range) {
			if (array) {
				fprintf(f, "std::clamp(values[i], ");
			}
			else {
				fprintf(f, "std::clamp(value, ");
			}
			if (fprop->range) {
				fprintf(f, "prop_clamp_min, prop_clamp_max);\n");
			}
			else {
				rna_print_c_float(f, fprop->hardmin);
				fprintf(f, ", ");
				rna_print_c_float(f, fprop->hardmax);
				fprintf(f, ");\n");
			}
			return;
		}
	}

	if (array) {
		fprintf(f, "values[i];\n");
	}
	else {
		fprintf(f, "value;\n");
	}
}

ROSE_INLINE char *rna_def_property_set_func(FILE *fpout, StructRNA *srna, PropertyRNA *p, PropertyDefRNA *dp, const char *manual) {
	char *func = NULL;

	if (!(p->flag & PROP_EDITABLE)) {
		return NULL;
	}
	if (p->flag & PROP_IDPROPERTY && manual == NULL) {
		return NULL;
	}

	if (!manual) {
		if (!dp->dnastructname || !dp->dnaname) {
			fprintf(stderr, "[RNA] \"%s.%s\" has no valid dna info.\n", srna->identifier, p->identifier);
			DefRNA.error = true;
			return NULL;
		}
	}

	func = rna_alloc_function_name(srna->identifier, rna_safe_id(p->identifier), "set");

	switch (p->type) {
		case PROP_POINTER: {
			PointerPropertyRNA *pproperty = (PointerPropertyRNA *)p;
			if (!manual) {
				fprintf(fpout, "extern void %s(PointerRNA *ptr, PointerRNA value) {\n", func);
				if (manual) {
					fprintf(fpout, "\tPropPointerSetFunc fn = %s;\n", manual);
					fprintf(fpout, "\tfn(ptr, value, reports);\n");
				}
				else {
					rna_print_data_get(fpout, dp);

					StructRNA *type = (pproperty->srna) ? rna_find_struct((const char *)pproperty->srna) : NULL;

					if (p->flag & PROP_ID_REFCOUNT) {
						/* Perform reference counting. */
						fprintf(fpout, "\tif (data->%s) {\n", dp->dnaname);
						fprintf(fpout, "\t\tid_us_min((ID *)data->%s);\n", dp->dnaname);
						fprintf(fpout, "\t}\n");
						fprintf(fpout, "\tif (value.%s) {\n", dp->dnaname);
						fprintf(fpout, "\t\tid_us_min((ID *)value.%s);\n", dp->dnaname);
						fprintf(fpout, "\t}\n");
					}

					fprintf(fpout, "\t*(void **)&data->%s = value.data;\n", dp->dnaname);
				}
				fprintf(fpout, "}\n\n");
			}
		} break;
		default: {
			if (p->arraydimension) {
				fprintf(fpout, "extern void %s(PointerRNA *ptr, const %s values[%u]) {\n", func, rna_type_type(p), p->totarraylength);

				if (manual) {
					/* Assign `fn` to ensure function signatures match. */
					if (p->type == PROP_BOOLEAN) {
						fprintf(fpout, "\tPropBooleanArraySetFunc fn = %s;\n", manual);
						fprintf(fpout, "\tfn(ptr, values);\n");
					}
					else if (p->type == PROP_INT) {
						fprintf(fpout, "\tPropIntArraySetFunc fn = %s;\n", manual);
						fprintf(fpout, "\tfn(ptr, values);\n");
					}
					else if (p->type == PROP_FLOAT) {
						fprintf(fpout, "\tPropFloatArraySetFunc fn = %s;\n", manual);
						fprintf(fpout, "\tfn(ptr, values);\n");
					}
					else {
						/* Valid but should be handled by type checks. */
						ROSE_assert_unreachable();
						fprintf(fpout, "\t%s(ptr, values);\n", manual);
					}
				}
				else {
					rna_print_data_get(fpout, dp);

					fprintf(fpout, "\tunsigned int i;\n\n");
					rna_clamp_value_range(fpout, p);
					fprintf(fpout, "\tfor (i = 0; i < %u; i++) {\n", p->totarraylength);

					if (dp->dnaarraylength == 1) {
						if (p->type == PROP_BOOLEAN && dp->booleanbit) {
							fprintf(fpout, "\t\tif (%svalues[i]) { data->%s |= (", (dp->booleannegative) ? "!" : "", dp->dnaname);
							rna_int_print(fpout, dp->booleanbit);
							fprintf(fpout, " << i); }\n");
							fprintf(fpout, "\t\telse { data->%s &= ~(", dp->dnaname);
							rna_int_print(fpout, dp->booleanbit);
							fprintf(fpout, " << i); }\n");
						}
						else {
							fprintf(fpout, "\t\t(&data->%s)[i] = %s", dp->dnaname, (dp->booleannegative) ? "!" : "");
							rna_clamp_value(fpout, p, 1);
						}
					}
					else {
						if (p->type == PROP_BOOLEAN && dp->booleanbit) {
							fprintf(fpout, "\t\tif (%svalues[i]) { data->%s[i] |= ", (dp->booleannegative) ? "!" : "", dp->dnaname);
							rna_int_print(fpout, dp->booleanbit);
							fprintf(fpout, "; }\n");
							fprintf(fpout, "\t\telse { data->%s[i] &= ~", dp->dnaname);
							rna_int_print(fpout, dp->booleanbit);
							fprintf(fpout, "; }\n");
						}
						else if (rna_color_quantize(p, dp)) {
							fprintf(fpout, "\t\tdata->%s[i] = unit_float_to_uchar_clamp(values[i]);\n", dp->dnaname);
						}
						else {
							if (dp->dnatype) {
								fprintf(fpout, "\t\t(data->%s)[i] = %s", dp->dnaname, (dp->booleannegative) ? "!" : "");
							}
							else {
								fprintf(fpout, "\t\t(data->%s)[i] = %s", dp->dnaname, (dp->booleannegative) ? "!" : "");
							}
							rna_clamp_value(fpout, p, 1);
						}
					}
					fprintf(fpout, "\t}\n");
				}

				fprintf(fpout, "}\n\n");
			}
			else {
				fprintf(fpout, "extern void %s(PointerRNA *ptr, %s value)\n", func, rna_type_type(p));
				fprintf(fpout, "{\n");

				if (manual) {
					/* Assign `fn` to ensure function signatures match. */
					if (p->type == PROP_BOOLEAN) {
						fprintf(fpout, "\tPropBooleanSetFunc fn = %s;\n", manual);
						fprintf(fpout, "\tfn(ptr, value);\n");
					}
					else if (p->type == PROP_INT) {
						fprintf(fpout, "\tPropIntSetFunc fn = %s;\n", manual);
						fprintf(fpout, "\tfn(ptr, value);\n");
					}
					else if (p->type == PROP_FLOAT) {
						fprintf(fpout, "\tPropFloatSetFunc fn = %s;\n", manual);
						fprintf(fpout, "\tfn(ptr, value);\n");
					}
					else if (p->type == PROP_ENUM) {
						fprintf(fpout, "\tPropEnumSetFunc fn = %s;\n", manual);
						fprintf(fpout, "\tfn(ptr, value);\n");
					}
					else {
						ROSE_assert_unreachable(); /* Valid but should be handled by type checks. */
						fprintf(fpout, "\t%s(ptr, value);\n", manual);
					}
				}
				else {
					rna_print_data_get(fpout, dp);
					if (p->type == PROP_BOOLEAN && dp->booleanbit) {
						fprintf(fpout, "\tif (%svalue) { data->%s |= ", (dp->booleannegative) ? "!" : "", dp->dnaname);
						rna_int_print(fpout, dp->booleanbit);
						fprintf(fpout, "; }\n");
						fprintf(fpout, "\telse { data->%s &= ~", dp->dnaname);
						rna_int_print(fpout, dp->booleanbit);
						fprintf(fpout, "; }\n");
					}
					else {
						rna_clamp_value_range(fpout, p);
						/* C++ may require casting to an enum type. */
						fprintf(fpout, "#ifdef __cplusplus\n");
						fprintf(fpout,
								/* If #rna_clamp_value() adds an expression like `std::clamp(...)`
								 * (instead of an `lvalue`), #decltype() yields a reference,
								 * so that has to be removed. */
								"\tdata->%s = %s(std::remove_reference_t<decltype(data->%s)>)",
								dp->dnaname,
								(dp->booleannegative) ? "!" : "",
								dp->dnaname);
						rna_clamp_value(fpout, p, 0);
						fprintf(fpout, "#else\n");
						fprintf(fpout, "\tdata->%s = %s", dp->dnaname, (dp->booleannegative) ? "!" : "");
						rna_clamp_value(fpout, p, 0);
						fprintf(fpout, "#endif\n");
					}
				}
				fprintf(fpout, "}\n\n");
			}
		} break;
	}

	return func;
}

ROSE_INLINE char *rna_def_property_begin_func(FILE *fpout, StructRNA *srna, PropertyRNA *property, PropertyDefRNA *defproperty, const char *manual) {
	char *func, *getfunc;

	if (property->flag & PROP_IDPROPERTY && manual == NULL) {
		return NULL;
	}

	if (!manual) {
		if (!defproperty->dnastructname || !defproperty->dnaname) {
			fprintf(stderr, "[RNA] %s.%s has no valid dna info.\n", srna->identifier, property->identifier);
			DefRNA.error = true;
			return NULL;
		}
	}

	func = rna_alloc_function_name(srna->identifier, rna_safe_id(property->identifier), "begin");

	fprintf(fpout, "extern void %s(CollectionPropertyIterator *iter, PointerRNA *ptr) {\n", func);

	if (!manual) {
		rna_print_data_get(fpout, defproperty);
		fprintf(fpout, "\n");
	}

	fprintf(fpout, "\tmemset(iter, 0, sizeof(CollectionPropertyIterator));\n");
	fprintf(fpout, "\titer->parent = *ptr;\n");
	fprintf(fpout, "\titer->property = (PropertyRNA *)&rna_%s_%s_;\n", srna->identifier, property->identifier);
	fprintf(fpout, "\n");

	if (defproperty->dnalengthname || defproperty->dnalengthfixed) {
		if (manual) {
			fprintf(fpout, "\tPropCollectionBeginFunc fn = %s;\n", manual);
			fprintf(fpout, "\tfn(iter, ptr);\n");
		}
		else {
			if (defproperty->dnalengthname) {
				fprintf(fpout, "\trna_iterator_array_begin(iter, ptr, data->%s, sizeof(data->%s[0]), data->%s, 0, NULL);\n", defproperty->dnaname, defproperty->dnaname, defproperty->dnalengthname);
			}
			else {
				fprintf(fpout, "\trna_iterator_array_begin(iter, ptr, data->%s, sizeof(data->%s[0]), %d, 0, NULL);\n", defproperty->dnaname, defproperty->dnaname, defproperty->dnalengthfixed);
			}
		}
	}
	else {
		if (manual) {
			fprintf(fpout, "\tPropCollectionBeginFunc fn = %s;\n", manual);
			fprintf(fpout, "\tfn(iter, ptr);\n");
		}
		else if (defproperty->dnapointerlevel == 0) {
			fprintf(fpout, "\trna_iterator_listbase_begin(iter, ptr, &data->%s, NULL);\n", defproperty->dnaname);
		}
		else {
			fprintf(fpout, "\trna_iterator_listbase_begin(iter, ptr, data->%s, NULL);\n", defproperty->dnaname);
		}
	}

	getfunc = rna_alloc_function_name(srna->identifier, rna_safe_id(property->identifier), "get");

	fprintf(fpout, "\n");
	fprintf(fpout, "\tif (iter->valid) {\n");
	fprintf(fpout, "\t\titer->ptr = %s(iter);\n", getfunc);
	fprintf(fpout, "\t}\n");
	fprintf(fpout, "}\n\n");

	return func;
}

ROSE_INLINE char *rna_def_property_next_func(FILE *fpout, StructRNA *srna, PropertyRNA *property, PropertyDefRNA *defproperty, const char *manual) {
	char *func, *getfunc;

	if (property->flag & PROP_IDPROPERTY && manual == NULL) {
		return NULL;
	}

	if (!manual) {
		return NULL;
	}

	func = rna_alloc_function_name(srna->identifier, rna_safe_id(property->identifier), "next");

	fprintf(fpout, "extern void %s(CollectionPropertyIterator *iter) {\n", func);
	fprintf(fpout, "\tPropCollectionNextFunc fn = %s;\n", manual);
	fprintf(fpout, "\tfn(iter);\n");
	fprintf(fpout, "\n");

	getfunc = rna_alloc_function_name(srna->identifier, rna_safe_id(property->identifier), "get");

	fprintf(fpout, "\tif (iter->valid) {\n");
	fprintf(fpout, "\t\titer->ptr = %s(iter);\n", getfunc);
	fprintf(fpout, "\t}\n");
	fprintf(fpout, "}\n\n");

	return func;
}

ROSE_INLINE char *rna_def_property_end_func(FILE *fpout, StructRNA *srna, PropertyRNA *property, PropertyDefRNA *defproperty, const char *manual) {
	char *func;

	if (property->flag & PROP_IDPROPERTY && manual == NULL) {
		return NULL;
	}

	func = rna_alloc_function_name(srna->identifier, rna_safe_id(property->identifier), "end");

	fprintf(fpout, "extern void %s(CollectionPropertyIterator *iter) {\n", func);
	if (manual) {
		fprintf(fpout, "\tPropCollectionEndFunc fn = %s;\n", manual);
		fprintf(fpout, "\tfn(iter);\n");
	}
	fprintf(fpout, "}\n\n");

	return func;
}

ROSE_INLINE void rna_def_property_funcs(FILE *fpout, StructRNA *srna, PropertyDefRNA *defproperty) {
	PropertyRNA *property = defproperty->ptr;

	switch (property->type) {
		case PROP_FLOAT: {
			FloatPropertyRNA *fproperty = (FloatPropertyRNA *)property;

			if (!(property->flag & PROP_EDITABLE) && (fproperty->set || fproperty->set_ex)) {
				fprintf(stderr, "[RNA] \"%s.%s\", is read-only but has defines a \"set\" callback.\n", srna->identifier, property->identifier);
				DefRNA.error = true;
			}
			if (!(property->flag & PROP_EDITABLE) && (fproperty->setarray || fproperty->setarray_ex)) {
				fprintf(stderr, "[RNA] \"%s.%s\", is read-only but has defines a \"setarray\" callback.\n", srna->identifier, property->identifier);
				DefRNA.error = true;
			}

			if (!property->arraydimension) {
				if (!fproperty->get && !fproperty->set) {
					// rna_set_raw_property(defproperty, property);
				}

				fproperty->get = (PropFloatGetFunc)(rna_def_property_get_func(fpout, srna, property, defproperty, (const char *)fproperty->get));
				fproperty->set = (PropFloatSetFunc)(rna_def_property_set_func(fpout, srna, property, defproperty, (const char *)fproperty->set));
			}
			else {
				if (!fproperty->getarray && !fproperty->setarray) {
					// rna_set_raw_property(defproperty, property);
				}

				fproperty->getarray = (PropFloatArrayGetFunc)(rna_def_property_get_func(fpout, srna, property, defproperty, (const char *)fproperty->getarray));
				fproperty->setarray = (PropFloatArraySetFunc)(rna_def_property_set_func(fpout, srna, property, defproperty, (const char *)fproperty->setarray));
			}
		} break;
		case PROP_STRING: {
			StringPropertyRNA *sproperty = (StringPropertyRNA *)property;

			if (!(property->flag & PROP_EDITABLE) && (sproperty->set || sproperty->set_ex)) {
				fprintf(stderr, "[RNA] \"%s.%s\", is read-only but has defines a \"set\" callback.\n", srna->identifier, property->identifier);
				DefRNA.error = true;
			}

			sproperty->get = (PropStringGetFunc)rna_def_property_get_func(fpout, srna, property, defproperty, (const char *)sproperty->get);
			sproperty->length = (PropStringLengthFunc)rna_def_property_length_func(fpout, srna, property, defproperty, (const char *)sproperty->length);
		} break;
		case PROP_POINTER: {
			PointerPropertyRNA *pproperty = (PointerPropertyRNA *)property;

			if (!(property->flag & PROP_EDITABLE) && pproperty->set) {
				fprintf(stderr, "[RNA] \"%s.%s\", is read-only but has defined a \"set\" callback.\n", srna->identifier, property->identifier);
				DefRNA.error = true;
			}

			pproperty->get = (PropPointerGetFunc)rna_def_property_get_func(fpout, srna, property, defproperty, (const char *)pproperty->get);
			pproperty->set = (PropPointerSetFunc)rna_def_property_set_func(fpout, srna, property, defproperty, (const char *)pproperty->set);

			if (!pproperty->srna) {
				fprintf(stderr, "[RNA] \"%s.%s\", pointer must have a struct type.", srna->identifier, property->identifier);
				DefRNA.error = true;
			}
		} break;
		case PROP_COLLECTION: {
			CollectionPropertyRNA *cproperty = (CollectionPropertyRNA *)property;
			
			cproperty->get = (PropCollectionGetFunc)rna_def_property_get_func(fpout, srna, property, defproperty, (const char *)cproperty->get);
			cproperty->begin = (PropCollectionBeginFunc)rna_def_property_begin_func(fpout, srna, property, defproperty, (const char *)cproperty->begin);
			cproperty->next = (PropCollectionNextFunc)rna_def_property_next_func(fpout, srna, property, defproperty, (const char *)cproperty->next);
			cproperty->end = (PropCollectionEndFunc)rna_def_property_end_func(fpout, srna, property, defproperty, (const char *)cproperty->end);
		} break;
	}
}

ROSE_INLINE void rna_generate_property(FILE *fpout, StructRNA *srna, const char *nest, PropertyRNA *property) {
	char *strnest = (char *)"", *errnest = (char *)"";
	bool freenest = false;

	if (nest != NULL) {
		size_t len = LIB_strlen(nest);

		strnest = MEM_mallocN(len + 2, "rna_generate_property::strnest");
		errnest = MEM_mallocN(len + 2, "rna_generate_property::errnest");

		strnest[0] = '_';
		memcpy(strnest + 1, nest, len + 1);

		errnest[0] = '.';
		memcpy(errnest + 1, nest, len + 1);

		freenest = true;
	}

	switch (property->type) {
		case PROP_BOOLEAN: {
			BoolPropertyRNA *bproperty = (BoolPropertyRNA *)property;

			if (property->arraydimension && property->totarraylength) {
				fprintf(fpout, "static bool rna_%s%s_%s_default[%u] = {\n\t", srna->identifier, strnest, property->identifier, property->totarraylength);

				for (unsigned int i = 0; i < property->totarraylength; i++) {
					if (bproperty->defaultarray) {
						fprintf(fpout, "%d", bproperty->defaultarray[i]);
					}
					else {
						fprintf(fpout, "%d", bproperty->defaultvalue);
					}
					if (i != property->totarraylength - 1) {
						fprintf(fpout, ",\n\t");
					}
				}

				fprintf(fpout, "\n};\n\n");
			}
		} break;
		case PROP_INT: {
			IntPropertyRNA *iproperty = (IntPropertyRNA *)property;

			if (property->arraydimension && property->totarraylength) {
				fprintf(fpout, "static int rna_%s%s_%s_default[%u] = {\n\t", srna->identifier, strnest, property->identifier, property->totarraylength);

				for (unsigned int i = 0; i < property->totarraylength; i++) {
					if (iproperty->defaultarray) {
						fprintf(fpout, "%d", iproperty->defaultarray[i]);
					}
					else {
						fprintf(fpout, "%d", iproperty->defaultvalue);
					}
					if (i != property->totarraylength - 1) {
						fprintf(fpout, ",\n\t");
					}
				}

				fprintf(fpout, "\n};\n\n");
			}
		} break;
		case PROP_FLOAT: {
			FloatPropertyRNA *fproperty = (FloatPropertyRNA *)property;

			if (property->arraydimension && property->totarraylength) {
				fprintf(fpout, "static float rna_%s%s_%s_default[%u] = {\n\t", srna->identifier, strnest, property->identifier, property->totarraylength);

				for (unsigned int i = 0; i < property->totarraylength; i++) {
					if (fproperty->defaultarray) {
						rna_print_c_float(fpout, fproperty->defaultarray[i]);
					}
					else {
						rna_print_c_float(fpout, fproperty->defaultvalue);
					}
					if (i != property->totarraylength - 1) {
						fprintf(fpout, ",\n\t");
					}
				}

				fprintf(fpout, "\n};\n\n");
			}
		} break;
		case PROP_POINTER: {
			PointerPropertyRNA *pproperty = (PointerPropertyRNA *)property;

			StructRNA *type = rna_find_struct((const char *)pproperty->srna);
			if (type && (type->flag & STRUCT_ID)) {
				RNA_def_property_flag(property, PROP_PTR_NO_OWNERSHIP);
			}
		} break;
		case PROP_COLLECTION: {
			CollectionPropertyRNA *cproperty = (CollectionPropertyRNA *)property;

			StructRNA *element = rna_find_struct((const char *)cproperty->element);
			if (element && (element->flag & STRUCT_ID) && !(property->flagex & PROP_INTERN_PTR_OWNERSHIP_FORCED)) {
				RNA_def_property_flag(property, PROP_PTR_NO_OWNERSHIP);
			}
		} break;
	}

	/**
	 * Generate the RNA-private, type-refined property data.
	 *
	 * See #rna_generate_external_property_prototypes comments for details.
	 */
	fprintf(fpout, "%s rna_%s%s_%s_ = {\n", rna_property_structname(property->type), srna->identifier, strnest, property->identifier);
	fprintf(fpout, "\t.property = {\n");

	do {
		fprintf(fpout, "\t\t.prev = ");
		do {
			if (property->prev) {
				fprintf(fpout, "(PropertyRNA *)&rna_%s%s_%s_", srna->identifier, strnest, property->prev->identifier);
			}
			else {
				fprintf(fpout, "NULL");
			}
		} while (false);
		fprintf(fpout, ",\n");

		fprintf(fpout, "\t\t.next = ");
		do {
			if (property->next) {
				fprintf(fpout, "(PropertyRNA *)&rna_%s%s_%s_", srna->identifier, strnest, property->next->identifier);
			}
			else {
				fprintf(fpout, "NULL");
			}
		} while (false);
		fprintf(fpout, ",\n");

		fprintf(fpout, "\t\t.magic = %d,\n", property->magic);

		do {
			fprintf(fpout, "\t\t.identifier = ");
			rna_print_c_string(fpout, property->identifier);
			fprintf(fpout, ",\n");
		} while (false);

		do {
			fprintf(fpout, "\t\t.flag = ");
			rna_print_property_flag(fpout, property->flag);
			fprintf(fpout, ",\n");
		} while (false);
		fprintf(fpout, "\t\t.type = %s,\n", RNA_property_typename(property->type));
		fprintf(fpout, "\t\t.subtype = %s,\n", RNA_property_subtypename(property->subtype));

		do {
			fprintf(fpout, "\t\t.name = ");
			rna_print_c_string(fpout, property->name);
			fprintf(fpout, ",\n");
		} while (false);
		do {
			fprintf(fpout, "\t\t.description = ");
			rna_print_c_string(fpout, property->description);
			fprintf(fpout, ",\n");
		} while (false);

		fprintf(fpout, "\t\t.arraydimension = %u,\n", property->arraydimension);
		fprintf(fpout, "\t\t.arraylength = {%u, %u, %u},\n", property->arraylength[0], property->arraylength[1], property->arraylength[2]);
		fprintf(fpout, "\t\t.totarraylength = %u,\n", property->totarraylength);

		fprintf(fpout, "\t\t.flagex = %u,\n", 0);

		if ((property->flagex & PROP_INTERN_RAW_ACCESS) != 0) {
			PropertyDefRNA *defproperty = rna_find_struct_property_def(srna, property);

			fprintf(fpout, "\t\t.rawoffset = offsetof(%s, %s),\n", defproperty->dnastructname, defproperty->dnaname);
			fprintf(fpout, "\t\t.rawtype = %s,\n", RNA_property_rawtypename(property->rawtype));
		}
		else {
			fprintf(fpout, "\t\t.rawoffset = 0,\n");
			fprintf(fpout, "\t\t.rawtype = PROP_RAW_UNSET,\n");
		}

		if (property->srna) {
			fprintf(fpout, "\t\t.srna = RNA_%s,\n", property->srna->identifier);
		}
		else {
			fprintf(fpout, "\t\t.srna = NULL,\n");
		}
	} while (false);
	fprintf(fpout, "\t},\n");

	switch (property->type) {
		case PROP_BOOLEAN: {
			BoolPropertyRNA *bproperty = (BoolPropertyRNA *)property;

			fprintf(fpout, "\t.get = %s,\n", rna_function_string(bproperty->get));
			fprintf(fpout, "\t.set = %s,\n", rna_function_string(bproperty->set));
			fprintf(fpout, "\t.getarray = %s,\n", rna_function_string(bproperty->getarray));
			fprintf(fpout, "\t.setarray = %s,\n", rna_function_string(bproperty->setarray));

			fprintf(fpout, "\t.get_ex = %s,\n", rna_function_string(bproperty->get_ex));
			fprintf(fpout, "\t.set_ex = %s,\n", rna_function_string(bproperty->set_ex));
			fprintf(fpout, "\t.getarray_ex = %s,\n", rna_function_string(bproperty->getarray_ex));
			fprintf(fpout, "\t.setarray_ex = %s,\n", rna_function_string(bproperty->setarray_ex));

			fprintf(fpout, "\t.get_default = %s,\n", rna_function_string(bproperty->get_default));
			fprintf(fpout, "\t.get_default_array = %s,\n", rna_function_string(bproperty->get_default_array));
			fprintf(fpout, "\t.defaultvalue = %d,\n", (int)bproperty->defaultvalue);

			if (property->arraydimension && property->totarraylength) {
				fprintf(fpout, "\t.defaultarray = rna_%s%s_%s_default\n", srna->identifier, strnest, property->identifier);
			}
			else {
				fprintf(fpout, "\t.defaultarray = NULL\n");
			}
		} break;
		case PROP_INT: {
			IntPropertyRNA *iproperty = (IntPropertyRNA *)property;

			fprintf(fpout, "\t.get = %s,\n", rna_function_string(iproperty->get));
			fprintf(fpout, "\t.set = %s,\n", rna_function_string(iproperty->set));
			fprintf(fpout, "\t.getarray = %s,\n", rna_function_string(iproperty->getarray));
			fprintf(fpout, "\t.setarray = %s,\n", rna_function_string(iproperty->setarray));
			fprintf(fpout, "\t.range = %s,\n", rna_function_string(iproperty->range));

			fprintf(fpout, "\t.get_ex = %s,\n", rna_function_string(iproperty->get_ex));
			fprintf(fpout, "\t.set_ex = %s,\n", rna_function_string(iproperty->set_ex));
			fprintf(fpout, "\t.getarray_ex = %s,\n", rna_function_string(iproperty->getarray_ex));
			fprintf(fpout, "\t.setarray_ex = %s,\n", rna_function_string(iproperty->setarray_ex));
			fprintf(fpout, "\t.range_ex = %s,\n", rna_function_string(iproperty->range_ex));

			fprintf(fpout, "\t.softmin = %d,\n", iproperty->softmin);
			fprintf(fpout, "\t.softmax = %d,\n", iproperty->softmax);
			fprintf(fpout, "\t.hardmin = %d,\n", iproperty->hardmin);
			fprintf(fpout, "\t.hardmax = %d,\n", iproperty->hardmax);
			fprintf(fpout, "\t.step = %d,\n", iproperty->step);

			fprintf(fpout, "\t.get_default = %s,\n", rna_function_string(iproperty->get_default));
			fprintf(fpout, "\t.get_default_array = %s,\n", rna_function_string(iproperty->get_default_array));
			fprintf(fpout, "\t.defaultvalue = %d,\n", iproperty->defaultvalue);

			if (property->arraydimension && property->totarraylength) {
				fprintf(fpout, "\t.defaultarray = rna_%s%s_%s_default\n", srna->identifier, strnest, property->identifier);
			}
			else {
				fprintf(fpout, "\t.defaultarray = NULL\n");
			}
		} break;
		case PROP_FLOAT: {
			FloatPropertyRNA *fproperty = (FloatPropertyRNA *)property;

			fprintf(fpout, "\t.get = %s,\n", rna_function_string(fproperty->get));
			fprintf(fpout, "\t.set = %s,\n", rna_function_string(fproperty->set));
			fprintf(fpout, "\t.getarray = %s,\n", rna_function_string(fproperty->getarray));
			fprintf(fpout, "\t.setarray = %s,\n", rna_function_string(fproperty->setarray));
			fprintf(fpout, "\t.range = %s,\n", rna_function_string(fproperty->range));

			fprintf(fpout, "\t.get_ex = %s,\n", rna_function_string(fproperty->get_ex));
			fprintf(fpout, "\t.set_ex = %s,\n", rna_function_string(fproperty->set_ex));
			fprintf(fpout, "\t.getarray_ex = %s,\n", rna_function_string(fproperty->getarray_ex));
			fprintf(fpout, "\t.setarray_ex = %s,\n", rna_function_string(fproperty->setarray_ex));
			fprintf(fpout, "\t.range_ex = %s,\n", rna_function_string(fproperty->range_ex));

			do {
				fprintf(fpout, "\t.softmin = ");
				rna_print_c_float(fpout, fproperty->softmin);
				fprintf(fpout, ",\n");
			} while (false);
			do {
				fprintf(fpout, "\t.softmax = ");
				rna_print_c_float(fpout, fproperty->softmax);
				fprintf(fpout, ",\n");
			} while (false);
			do {
				fprintf(fpout, "\t.hardmin = ");
				rna_print_c_float(fpout, fproperty->hardmin);
				fprintf(fpout, ",\n");
			} while (false);
			do {
				fprintf(fpout, "\t.hardmax = ");
				rna_print_c_float(fpout, fproperty->hardmax);
				fprintf(fpout, ",\n");
			} while (false);
			do {
				fprintf(fpout, "\t.step = ");
				rna_print_c_float(fpout, fproperty->step);
				fprintf(fpout, ",\n");
			} while (false);
			fprintf(fpout, "\t.precision = %d,\n", fproperty->precision);

			fprintf(fpout, "\t.get_default = %s,\n", rna_function_string(fproperty->get_default));
			fprintf(fpout, "\t.get_default_array = %s,\n", rna_function_string(fproperty->get_default_array));
			do {
				fprintf(fpout, "\t.defaultvalue = ");
				rna_print_c_float(fpout, fproperty->defaultvalue);
				fprintf(fpout, ",\n");
			} while (false);

			if (property->arraydimension && property->totarraylength) {
				fprintf(fpout, "\t.defaultarray = rna_%s%s_%s_default\n", srna->identifier, strnest, property->identifier);
			}
			else {
				fprintf(fpout, "\t.defaultarray = NULL\n");
			}
		} break;
		case PROP_STRING: {
			StringPropertyRNA *sproperty = (StringPropertyRNA *)property;

			fprintf(fpout, "\t.get = %s,\n", rna_function_string(sproperty->get));
			fprintf(fpout, "\t.length = %s,\n", rna_function_string(sproperty->length));
			fprintf(fpout, "\t.set = %s,\n", rna_function_string(sproperty->set));

			fprintf(fpout, "\t.get_ex = %s,\n", rna_function_string(sproperty->get_ex));
			fprintf(fpout, "\t.length_ex = %s,\n", rna_function_string(sproperty->length_ex));
			fprintf(fpout, "\t.set_ex = %s,\n", rna_function_string(sproperty->set_ex));

			fprintf(fpout, "\t.maxlength = %d,\n", sproperty->maxlength);

			do {
				fprintf(fpout, "\t.defaultvalue = ");
				rna_print_c_string(fpout, sproperty->defaultvalue);
				fprintf(fpout, ",\n");
			} while (false);
		} break;
		case PROP_POINTER: {
			PointerPropertyRNA *pproperty = (PointerPropertyRNA *)property;

			fprintf(fpout, "\t.get = %s,\n", rna_function_string(pproperty->get));
			fprintf(fpout, "\t.set = %s,\n", rna_function_string(pproperty->set));
			fprintf(fpout, "\t.type = %s,\n", rna_function_string(pproperty->type));
			fprintf(fpout, "\t.poll = %s,\n", rna_function_string(pproperty->poll));

			do {
				fprintf(fpout, "\t.srna = ");
				if (pproperty->srna) {
					fprintf(fpout, "&RNA_%s", (const char *)pproperty->srna);
				}
				else {
					fprintf(fpout, "NULL");
				}
				fprintf(fpout, ",\n");
			} while (false);
		} break;
		case PROP_COLLECTION: {
			CollectionPropertyRNA *cproperty = (CollectionPropertyRNA *)property;

			fprintf(fpout, "\t.begin = %s,\n", rna_function_string(cproperty->begin));
			fprintf(fpout, "\t.next = %s,\n", rna_function_string(cproperty->next));
			fprintf(fpout, "\t.end = %s,\n", rna_function_string(cproperty->end));
			fprintf(fpout, "\t.get = %s,\n", rna_function_string(cproperty->get));
			fprintf(fpout, "\t.length = %s,\n", rna_function_string(cproperty->length));
			fprintf(fpout, "\t.lookupint = %s,\n", rna_function_string(cproperty->lookupint));
			fprintf(fpout, "\t.lookupstring = %s,\n", rna_function_string(cproperty->lookupstring));

			if (cproperty->element) {
				fprintf(fpout, "\t.element = &RNA_%s,\n", (const char *)cproperty->element);
			}
			else {
				fprintf(fpout, "\t.element = NULL,\n");
			}
		} break;
	}

	fprintf(fpout, "};\n");
	fprintf(fpout, "\n");
	fprintf(fpout, "PropertyRNA *rna_%s%s_%s = ", srna->identifier, strnest, property->identifier);
	fprintf(fpout, "(PropertyRNA *)&rna_%s%s_%s_.property;\n", srna->identifier, strnest, property->identifier);
	fprintf(fpout, "\n");

	if (freenest) {
		MEM_freeN(strnest);
		MEM_freeN(errnest);
	}
}

ROSE_INLINE void rna_generate_struct(FILE *fpout, RoseRNA *rna, StructRNA *nstruct) {
	fprintf(fpout, "/* %s */\n", nstruct->name);
	fprintf(fpout, "\n");

	LISTBASE_FOREACH(PropertyRNA *, property, &nstruct->container.properties) {
		rna_generate_property(fpout, nstruct, NULL, property);
	}

	fprintf(fpout, "StructRNA RNA_%s = {\n", nstruct->identifier);
	fprintf(fpout, "\t.container = {\n");
	do {
		do {
			fprintf(fpout, "\t\t.prev = ");
			if (nstruct->container.prev) {
				fprintf(fpout, "(ContainerRNA *)&RNA_%s", ((StructRNA *)nstruct->container.prev)->identifier);
			}
			else {
				fprintf(fpout, "NULL");
			}
			fprintf(fpout, ",\n");
		} while (false);
		do {
			fprintf(fpout, "\t\t.next = ");
			if (nstruct->container.next) {
				fprintf(fpout, "(ContainerRNA *)&RNA_%s", ((StructRNA *)nstruct->container.next)->identifier);
			}
			else {
				fprintf(fpout, "NULL");
			}
			fprintf(fpout, ",\n");
		} while (false);

		LISTBASE_FOREACH(PropertyRNA *, property, &nstruct->container.properties) {
			fprintf(fpout, "\t\t/* %s */\n", property->identifier);
		}

		if (!LIB_listbase_is_empty(&nstruct->container.properties)) {
			PropertyRNA *first = (PropertyRNA *)nstruct->container.properties.first;
			PropertyRNA *last = (PropertyRNA *)nstruct->container.properties.last;

			ROSE_assert(first && last);

			fprintf(fpout, "\t\t.properties = {\n");
			fprintf(fpout, "\t\t\t.first = (PropertyRNA *)&rna_%s_%s_,\n", nstruct->identifier, first->identifier);
			fprintf(fpout, "\t\t\t.last = (PropertyRNA *)&rna_%s_%s_,\n", nstruct->identifier, last->identifier);
			fprintf(fpout, "\t\t}\n");
		}
		else {
			fprintf(fpout, "\t\t.properties = {\n");
			fprintf(fpout, "\t\t\t.first = %s,\n", "NULL");
			fprintf(fpout, "\t\t\t.last = %s,\n", "NULL");
			fprintf(fpout, "\t\t},\n");
		}
	} while (false);
	fprintf(fpout, "\t},\n");

	do {
		fprintf(fpout, "\t.identifier = ");
		rna_print_c_string(fpout, nstruct->identifier);
		fprintf(fpout, ",\n");
	} while (false);

	do {
		fprintf(fpout, "\t.flag = ");
		rna_print_struct_flag(fpout, nstruct->flag);
		fprintf(fpout, ",\n");
	} while (false);

	do {
		fprintf(fpout, "\t.name = ");
		rna_print_c_string(fpout, nstruct->name);
		fprintf(fpout, ",\n");
	} while (false);

	do {
		fprintf(fpout, "\t.description = ");
		rna_print_c_string(fpout, nstruct->description);
		fprintf(fpout, ",\n");
	} while (false);

	do {
		fprintf(fpout, "\t.nameproperty = ");
		if (nstruct->nameproperty) {
			StructRNA *base = nstruct;
			PropertyRNA *property = nstruct->nameproperty;
			while (base->base && base->base->nameproperty == property) {
				base = base->base;
			}
			fprintf(fpout, "(PropertyRNA *)&rna_%s_%s_", base->identifier, property->identifier);
		}
		else {
			fprintf(fpout, "NULL");
		}
		fprintf(fpout, ",\n");
	} while (false);

	do {
		fprintf(fpout, "\t.iteratorproperty = ");
		/* We always ought to have iterator property! */ {
			StructRNA *base = nstruct;
			PropertyRNA *property = nstruct->iteratorproperty;
			while (base->base && base->base->iteratorproperty == property) {
				base = base->base;
			}
			fprintf(fpout, "(PropertyRNA *)&rna_%s_rna_properties_", base->identifier);
		}
		fprintf(fpout, ",\n");
	} while (false);

	do {
		fprintf(fpout, "\t.base = ");
		if (nstruct->base) {
			fprintf(fpout, "&RNA_%s", nstruct->base->identifier);
		}
		else {
			fprintf(fpout, "NULL");
		}
		fprintf(fpout, ",\n");
	} while (false);

	do {
		fprintf(fpout, "\t.nested = ");
		if (nstruct->nested) {
			fprintf(fpout, "&RNA_%s", nstruct->nested->identifier);
		}
		else {
			fprintf(fpout, "NULL");
		}
		fprintf(fpout, ",\n");
	} while (false);

	fprintf(fpout, "\t.refine = %s,\n", rna_function_string(nstruct->refine));
	fprintf(fpout, "\t.path = %s,\n", rna_function_string(nstruct->path));

	fprintf(fpout, "};\n");
	fprintf(fpout, "\n");
}

ROSE_INLINE void rna_generate(RoseRNA *rna, FILE *fpout, const char *filename) {
	fprintf(fpout, "/**\n");
	fprintf(fpout, " * Automatically generated struct definitions for the Data API.\n");
	fprintf(fpout, " * Do not edit manually, changes will be overwritten.\n");
	fprintf(fpout, " */\n");
	fprintf(fpout, "\n");
	fprintf(fpout, "#include <float.h>\n");
	fprintf(fpout, "#include <stdio.h>\n");
	fprintf(fpout, "#include <limits.h>\n");
	fprintf(fpout, "#include <string.h>\n");
	fprintf(fpout, "#include <stddef.h>\n");
	fprintf(fpout, "\n");
	fprintf(fpout, "#include \"MEM_guardedalloc.h\"\n");
	fprintf(fpout, "\n");
	fprintf(fpout, "#include \"DNA_include_all.h\"\n");
	fprintf(fpout, "\n");
	fprintf(fpout, "#include \"LIB_listbase.h\"\n");
	fprintf(fpout, "#include \"LIB_string.h\"\n");
	fprintf(fpout, "#include \"LIB_utildefines.h\"\n");
	fprintf(fpout, "\n");
	fprintf(fpout, "#include \"KER_context.h\"\n");
	fprintf(fpout, "#include \"KER_lib_id.h\"\n");
	fprintf(fpout, "#include \"KER_main.h\"\n");
	fprintf(fpout, "\n");
	fprintf(fpout, "#include \"RNA_access.h\"\n");
	fprintf(fpout, "#include \"RNA_prototypes.h\"\n");
	fprintf(fpout, "\n");
	fprintf(fpout, "#include \"rna_internal_types.h\"\n");
	fprintf(fpout, "#include \"rna_internal.h\"\n");
	fprintf(fpout, "\n");

	LISTBASE_FOREACH(StructDefRNA *, defstruct, &DefRNA.structs) {
		if (!filename || STREQ(defstruct->filename, filename)) {
			LISTBASE_FOREACH(PropertyDefRNA *, defproperty, &defstruct->container.properties) {
				rna_def_property_funcs(fpout, defstruct->ptr, defproperty);
			}
		}
	}

	LISTBASE_FOREACH(StructDefRNA *, defstruct, &DefRNA.structs) {
		if (!filename || STREQ(defstruct->filename, filename)) {
			rna_generate_struct(fpout, rna, defstruct->ptr);
		}
	}
}

ROSE_INLINE void rna_generate_struct_rna_prototypes(RoseRNA *rna, FILE *fpout) {
	LISTBASE_FOREACH(StructRNA *, nstruct, &rna->srnabase) {
		fprintf(fpout, "extern struct StructRNA RNA_%s;\n", nstruct->identifier);
	}
	fprintf(fpout, "\n");
}

ROSE_INLINE void rna_generate_external_property_prototypes(RoseRNA *rna, FILE *fpout) {
	fprintf(fpout, "struct StructRNA;\n");
	fprintf(fpout, "struct PropertyRNA;\n");
	fprintf(fpout, "\n");

	rna_generate_struct_rna_prototypes(rna, fpout);

	/* NOTE: Generate generic `PropertyRNA &` references. The actual, type-refined properties data
	 * are static variables in their translation units (the `_gen.cc` files), which are assigned to
	 * these public generic `PointerRNA &` references. */
	LISTBASE_FOREACH(StructRNA *, nstruct, &rna->srnabase) {
		if (LIB_listbase_is_empty(&nstruct->container.properties)) {
			continue;
		}
		fprintf(fpout, "/** struct %s */\n", nstruct->identifier);
		fprintf(fpout, "\n");
		LISTBASE_FOREACH(PropertyRNA *, property, &nstruct->container.properties) {
			fprintf(fpout, "extern struct PropertyRNA *rna_%s_%s;\n", nstruct->identifier, property->identifier);
		}
		fprintf(fpout, "\n");
	}

	/* NOTE: Generate generic `PropertyRNA &` references. The actual, type-refined properties data
	 * are static variables in their translation units (the `_gen.cc` files), which are assigned to
	 * these public generic `PointerRNA &` references. */
	LISTBASE_FOREACH(StructRNA *, nstruct, &rna->srnabase) {
		if (LIB_listbase_is_empty(&nstruct->container.properties)) {
			continue;
		}
		fprintf(fpout, "/** struct %s */\n", nstruct->identifier);
		fprintf(fpout, "\n");
		LISTBASE_FOREACH(PropertyRNA *, property, &nstruct->container.properties) {
			fprintf(fpout, "extern struct %s rna_%s_%s_;\n", rna_property_structname(property->type), nstruct->identifier, property->identifier);
		}
		fprintf(fpout, "\n");
	}
}

ROSE_INLINE int rna_preprocess(const char *source, const char *binary) {
	RoseRNA *rna = RNA_create();

	for (size_t index = 0; index < ARRAY_SIZE(RNAItems); index++) {
		if (RNAItems[index].define) {
			RNAItems[index].define(rna);
		}

		for (StructDefRNA *defstruct = (StructDefRNA *)DefRNA.structs.first; defstruct; defstruct = (StructDefRNA *)defstruct->container.next) {
			if (!defstruct->filename) {
				defstruct->filename = RNAItems[index].filename;
			}
		}
	}

	if (DefRNA.error) {
		fprintf(stderr, "[RNA] There was an error while generating the RNA for RoseRNA.\n");
	}

	fprintf(stdout, "[RNA] Writing RNA output in \"%s\"\n", binary);

	char prototype[1024];
	LIB_path_join(prototype, ARRAY_SIZE(prototype), binary, "../RNA_prototypes.h");

	FILE *fpout = fopen(prototype, "w");
	if (!fpout) {
		fprintf(stderr, "[RNA] Cannot write the output file \"%s\" for RoseRNA.\n", prototype);
	}
	else {
		fprintf(fpout, "#pragma once\n");
		fprintf(fpout, "\n");
		fprintf(fpout, "/**\n");
		fprintf(fpout, " * Automatically generated RNA property declarations, to statically reference \n");
		fprintf(fpout, " * properties as `rna_[struct-name]_[property-name]`. \n");
		fprintf(fpout, " *\n");
		fprintf(fpout, " * DO NOT EDIT MANUALLY, changes will be overwritten.\n");
		fprintf(fpout, " */\n");
		fprintf(fpout, "\n");
		fprintf(fpout, "#ifdef __cplusplus\n");
		fprintf(fpout, "extern \"C\" {\n");
		fprintf(fpout, "#endif\n");
		fprintf(fpout, "\n");
		rna_generate_external_property_prototypes(rna, fpout);
		fprintf(fpout, "#ifdef __cplusplus\n");
		fprintf(fpout, "}\n");
		fprintf(fpout, "#endif\n");
		fclose(fpout);
	}

	for (size_t index = 0; index < ARRAY_SIZE(RNAItems); index++) {
		size_t lenfile = LIB_strlen(RNAItems[index].filename);

		char file[1024];
		LIB_strnformat(file, ARRAY_SIZE(file), "%.*s_gen.c", (unsigned int)lenfile - 2, RNAItems[index].filename);

		char path[1024];
		LIB_path_join(path, ARRAY_SIZE(path), file);

		FILE *fpout = fopen(path, "w");
		if (!fpout) {
			fprintf(stderr, "[RNA] Cannot write the output file \"%s\" for RoseRNA.\n", path);
			continue;
		}
		rna_generate(rna, fpout, RNAItems[index].filename);
		fclose(fpout);
	}

	RNA_free(rna);

	return DefRNA.error;
}

const char *source = NULL;
const char *binary = NULL;

ROSE_INLINE void help(const char *program_name) {
	fprintf(stderr, "Invalid argument list!\n\n");
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "\t%s --src <directory> --bin <directory>\n", program_name);
}

ROSE_INLINE bool init(int argc, char **argv) {
	for (int i = 1; i < argc; i++) {
		if (STREQ(argv[i], "--src") && i + 1 < argc) {
			source = argv[++i];
			continue;
		}
		if (STREQ(argv[i], "--bin") && i + 1 < argc) {
			binary = argv[++i];
			continue;
		}
		return false;  // Invalid argument
	}
	return source && binary;  // Return true only if both arguments are provided
}

int main(int argc, char **argv) {
#ifndef NDEBUG
	MEM_init_memleak_detection();
	MEM_enable_fail_on_memleak();
	MEM_use_guarded_allocator();
#endif

	int status = 0;

	if (!init(argc, argv)) {
		help(argv[0]);
		return -1;
	}

	return rna_preprocess(source, binary);
}

/*

--src "C:/Users/Jim/source/repos/rose/source/rose/makerna" --bin "C:/Users/Jim/source/build/rose/source/rose/makerna/intern"

*/
