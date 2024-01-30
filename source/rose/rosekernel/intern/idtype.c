#include "DNA_ID.h"

#include "KER_idtype.h"

#include "LIB_assert.h"
#include "LIB_string.h"

static IDTypeInfo *id_types[INDEX_ID_MAX] = {NULL};

void KER_idtype_init(void) {

#define INIT_TYPE(_id_code) \
	{ \
		ROSE_assert(IDType_##_id_code.main_listbase_index == INDEX_##_id_code); \
		id_types[INDEX_##_id_code] = &IDType_##_id_code; \
	} \
	(void)0

	INIT_TYPE(ID_LI);
	INIT_TYPE(ID_WM);

#undef INIT_TYPE
}

const IDTypeInfo *KER_idtype_get_info_from_idcode(const short id_code) {
	int id_index = KER_idtype_idcode_to_index(id_code);

	if (id_index >= 0 && id_index < ARRAY_SIZE(id_types) && id_types[id_index] != NULL &&
		id_types[id_index]->name[0] != '\0') {
		return id_types[id_index];
	}

	return NULL;
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
	return KER_idtype_get_info_from_idcode(idcode) != NULL ? true : false;
}

static const IDTypeInfo *idtype_get_info_from_name(const char *idtype_name) {
	for (int i = ARRAY_SIZE(id_types); i--;) {
		if (id_types[i] != NULL && STREQ(idtype_name, id_types[i]->name)) {
			return id_types[i];
		}
	}

	return NULL;
}

short KER_idtype_idcode_from_name(const char *idtype_name) {
	const IDTypeInfo *id_type = idtype_get_info_from_name(idtype_name);
	ROSE_assert(id_type);
	return id_type != NULL ? id_type->id_code : 0;
}

uint64_t KER_idtype_idcode_to_idfilter(short idcode) {
#define CASE_IDFILTER(_id) \
	case ID_##_id: \
		return FILTER_ID_##_id

#define CASE_IDFILTER_NONE(_id) \
	case ID_##_id: \
		return 0

	switch ((ID_Type)idcode) {
		CASE_IDFILTER(LI);
		CASE_IDFILTER(WM);
	}

	ROSE_assert_unreachable();
	return 0;

#undef CASE_IDFILTER
#undef CASE_IDFILTER_NONE
}

short KER_idtype_idcode_from_idfilter(uint64_t idfilter) {
#define CASE_IDFILTER(_id) \
	case FILTER_ID_##_id: \
		return ID_##_id

#define CASE_IDFILTER_NONE(_id) (void)0

	switch (idfilter) {
		CASE_IDFILTER(LI);
		CASE_IDFILTER(WM);
	}

	ROSE_assert_unreachable();
	return 0;

#undef CASE_IDFILTER
#undef CASE_IDFILTER_NONE
}

int KER_idtype_idcode_to_index(short idcode) {
#define CASE_IDINDEX(_id) \
	case ID_##_id: \
		return INDEX_ID_##_id

	switch ((ID_Type)idcode) {
		CASE_IDINDEX(LI);
		CASE_IDINDEX(WM);
	}

	/** Special naughty boy... */
	if (idcode == ID_LINK_PLACEHOLDER) {
		return INDEX_ID_NULL;
	}

	return -1;

#undef CASE_IDINDEX
}

short KER_idtype_idcode_from_index(int index) {
#define CASE_IDCODE(_id) \
	case INDEX_ID_##_id: \
		return ID_##_id

	switch (index) {
		CASE_IDCODE(LI);
		CASE_IDCODE(WM);
	}

	/* Special naughty boy... */
	if (index == INDEX_ID_NULL) {
		return ID_LINK_PLACEHOLDER;
	}

	return -1;

#undef CASE_IDCODE
}

short KER_idtype_idcode_iter_step(int *index) {
	return (*index < ARRAY_SIZE(id_types)) ? KER_idtype_idcode_from_index((*index)++) : 0;
}
