#include "MEM_guardedalloc.h"

#include "LIB_string.h"
#include "LIB_path_utils.h"

#include "RNA_define.h"
#include "RNA_types.h"

#include "rna_internal_types.h"
#include "rna_internal.h"

#include <stdio.h>

extern RoseDefRNA DefRNA;

typedef struct RNAProcessItem {
	const char *filename;
	const char *api_filename;
	void (*define)(RoseRNA *brna);
} RNAProcessItem;

RNAProcessItem RNAItems[] = {
	{.filename = "rna_id.c", .define = RNA_def_ID},
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

ROSE_INLINE void rna_print_property_flag(FILE *f, ePropertyFlag flag) {
	ePropertyFlag val[] = {
		PROP_ANIMATABLE,
		PROP_EDITABLE,
		PROP_NEVER_NULL,
		PROP_PTR_NO_OWNERSHIP,
		PROP_IDPROPERTY,
	};
	const char *name[] = {
		"PROP_ANIMATABLE",
		"PROP_EDITABLE",
		"PROP_NEVER_NULL",
		"PROP_PTR_NO_OWNERSHIP",
		"PROP_IDPROPERTY",
	};

	size_t preceding = 0;
	for (size_t index = 0; index < ARRAY_SIZE(val); index++) {
		if ((flag & val[index]) == val[index]) {
			fprintf(f, "%s%s", (preceding++ > 0) ? " | " : "", name[index]);
		}
	}

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

	size_t preceding = 0;
	for (size_t index = 0; index < ARRAY_SIZE(val); index++) {
		if ((flag & val[index]) == val[index]) {
			fprintf(f, "%s%s", (preceding++ > 0) ? " | " : "", name[index]);
		}
	}

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

			StructRNA *type = rna_find_struct((const char *)pproperty->type);
			if (type && (type->flag & STRUCT_ID)) {
				RNA_def_property_flag(property, PROP_PTR_NO_OWNERSHIP);
			}
		} break;
	}

	/**
	 * Generate the RNA-private, type-refined property data.
	 *
	 * See #rna_generate_external_property_prototypes comments for details.
	 */
	fprintf(fpout, "static %s rna_%s%s_%s_ = {\n", rna_property_structname(property->type), srna->identifier, strnest, property->identifier);
	fprintf(fpout, "\t.property = {\n");

	do {
		fprintf(fpout, "\t\t.prev = ");
		do {
			if (property->prev) {
				fprintf(fpout, "rna_%s%s_%s", srna->identifier, strnest, property->prev->identifier);
			}
			else {
				fprintf(fpout, "NULL");
			}
		} while (false);
		fprintf(fpout, ",\n");

		fprintf(fpout, "\t\t.next = ");
		do {
			if (property->next) {
				fprintf(fpout, "rna_%s%s_%s", srna->identifier, strnest, property->next->identifier);
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
					fprintf(fpout, "&RNA_%s\n", (const char *)pproperty->srna);
				}
				else {
					fprintf(fpout, "NULL\n");
				}
				fprintf(fpout, ",\n");
			} while (false);
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
		if (nstruct->iteratorproperty) {
			StructRNA *base = nstruct;
			PropertyRNA *property = nstruct->iteratorproperty;
			while (base->base && base->base->iteratorproperty == property) {
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
	fprintf(fpout, "#include \"LIB_listbase.h\"\n");
	fprintf(fpout, "#include \"LIB_string.h\"\n");
	fprintf(fpout, "#include \"LIB_utildefines.h\"\n");
	fprintf(fpout, "\n");
	fprintf(fpout, "#include \"KER_context.h\"\n");
	fprintf(fpout, "#include \"KER_lib_id.h\"\n");
	fprintf(fpout, "#include \"KER_main.h\"\n");
	fprintf(fpout, "\n");
	fprintf(fpout, "#include \"RNA_prototypes.h\"\n");
	fprintf(fpout, "\n");
	fprintf(fpout, "#include \"rna_internal_types.h\"\n");
	fprintf(fpout, "#include \"rna_internal.h\"\n");
	fprintf(fpout, "\n");

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
