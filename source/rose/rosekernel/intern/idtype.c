#include "LIB_listbase.h"

#include "KER_idtype.h"

/* -------------------------------------------------------------------- */
/** \name Static IDTypeInfo variables
 * \{ */

static IDTypeInfo *id_types[INDEX_ID_MAX];

/** \} */

static void id_type_init() {
	int init_types_num = 0;

#define INIT_TYPE(_id_code)                                       \
	{                                                             \
		ROSE_assert(IDType_##_id_code.index == INDEX_##_id_code); \
		id_types[INDEX_##_id_code] = &IDType_##_id_code;          \
		init_types_num++;                                         \
	}                                                             \
	(void)0

	INIT_TYPE(ID_LI);
	INIT_TYPE(ID_ME);
	INIT_TYPE(ID_SCR);
	INIT_TYPE(ID_WM);

	/* Special case. */
	ROSE_assert(IDType_ID_LINK_PLACEHOLDER.index == INDEX_ID_NULL);
	id_types[INDEX_ID_NULL] = &IDType_ID_LINK_PLACEHOLDER;
	init_types_num++;

	ROSE_assert_msg(init_types_num == INDEX_ID_MAX, "Some IDTypeInfo initialization is missing");
	UNUSED_VARS_NDEBUG(init_types_num);

#undef INIT_TYPE
}

/* -------------------------------------------------------------------- */
/** \name Main Methods
 * \{ */

void KER_idtype_init() {
	/** Initialize the data-block types into the #id_types array. */
	id_type_init();
}

const IDTypeInfo *KER_idtype_get_info_from_idtype_index(const int idtype_index) {
	if (idtype_index >= 0 && idtype_index < INDEX_ID_MAX) {
		const IDTypeInfo *id_type = id_types[idtype_index];
		if (id_type && id_type->name[0] != '\0') {
			return id_type;
		}
	}

	return NULL;
}

const IDTypeInfo *KER_idtype_get_info_from_idcode(short idcode) {
	return KER_idtype_get_info_from_idtype_index(KER_idtype_idcode_to_index(idcode));
}
const IDTypeInfo *KER_idtype_get_info_from_id(const ID *id) {
	return KER_idtype_get_info_from_idcode(GS(id->name));
}

const char *KER_idtype_idcode_to_name(short idcode) {
	const IDTypeInfo *id_type = KER_idtype_get_info_from_idcode(idcode);
	ROSE_assert(id_type != NULL);
	return id_type != NULL ? id_type->name : NULL;
}
const char *KER_idtype_idcode_to_name_plural(short idcode) {
	const IDTypeInfo *id_type = KER_idtype_get_info_from_idcode(idcode);
	ROSE_assert(id_type != NULL);
	return id_type != NULL ? id_type->name_plural : NULL;
}

bool KER_idtype_idcode_is_valid(short idcode) {
	return KER_idtype_get_info_from_idcode(idcode) != NULL;
}

ROSE_INLINE const IDTypeInfo *idtype_get_info_from_name(const char *idtype_name) {
	for (int idindex = 0; idindex < INDEX_ID_MAX; idindex++) {
		const IDTypeInfo *id_type = KER_idtype_get_info_from_idtype_index(idindex);
		if (id_type && STREQ(idtype_name, id_type->name)) {
			return id_type;
		}
	}

	return NULL;
}

short KER_idtype_idcode_from_name(const char *idtype_name) {
	const IDTypeInfo *id_type = idtype_get_info_from_name(idtype_name);
	ROSE_assert(id_type);
	return id_type != NULL ? id_type->idcode : 0;
}

int KER_idtype_idcode_to_index(short idcode) {
#define CASE_IDINDEX(_id) \
	case ID_##_id:        \
		return INDEX_ID_##_id

	switch (idcode) {
		CASE_IDINDEX(LI);
		CASE_IDINDEX(ME);
		CASE_IDINDEX(SCR);
		CASE_IDINDEX(WM);
	}

	/* Special naughty boy... */
	if (idcode == ID_LINK_PLACEHOLDER) {
		return INDEX_ID_NULL;
	}

	return -1;

#undef CASE_IDINDEX
}

int KER_idtype_idfilter_to_index(int idfilter) {
#define CASE_IDINDEX(_id) \
	case FILTER_ID_##_id: \
		return INDEX_ID_##_id

	switch (idfilter) {
		CASE_IDINDEX(LI);
		CASE_IDINDEX(ME);
		CASE_IDINDEX(SCR);
		CASE_IDINDEX(WM);
	}

	/* No handling of #ID_LINK_PLACEHOLDER or #INDEX_ID_NULL here. */

	return -1;

#undef CASE_IDINDEX
}

int KER_idtype_idcode_to_idfilter(short idcode) {
	const IDTypeInfo *id_type = KER_idtype_get_info_from_idcode(idcode);
	if (id_type) {
		return id_type->filter;
	}

	return 0;
}

int KER_idtype_idfilter_to_idcode(int idfilter) {
	return KER_idtype_index_to_idcode(KER_idtype_idfilter_to_index(idfilter));
}

int KER_idtype_index_to_idcode(int idindex) {
	const IDTypeInfo *id_type = KER_idtype_get_info_from_idtype_index(idindex);
	if (id_type) {
		return id_type->idcode;
	}

	ROSE_assert_unreachable();
	return -1;
}

int KER_idtype_index_to_idfilter(int idindex) {
	const IDTypeInfo *id_type = KER_idtype_get_info_from_idtype_index(idindex);
	if (id_type) {
		return id_type->filter;
	}

	ROSE_assert_unreachable();
	return -1;
}

short KER_idtype_idcode_iter_step(int *idtype_index) {
	return (*idtype_index < INDEX_ID_MAX) ? KER_idtype_index_to_idcode((*idtype_index)++) : 0;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Placeholder Data-block definition
 * \{ */

IDTypeInfo IDType_ID_LINK_PLACEHOLDER = {
	.idcode = ID_LINK_PLACEHOLDER,

	.filter = 0,
	.depends = 0,
	.index = INDEX_ID_NULL,
	.size = sizeof(ID),

	.name = "LinkPlaceholder",
	.name_plural = "Link Placeholders",

	.flag = IDTYPE_FLAGS_NO_COPY,

	.init_data = NULL,
	.copy_data = NULL,
	.free_data = NULL,

	.foreach_id = NULL,

	.write = NULL,
	.read_data = NULL,
};

/** \} */
