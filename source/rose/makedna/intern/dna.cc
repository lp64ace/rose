#include "RT_context.h"
#include "RT_parser.h"
#include "RT_token.h"

#include "dna.hh"

using namespace rose::dna;

/* -------------------------------------------------------------------- */
/** \name DNATypeInterface
 * \{ */

ROSE_STATIC RTType *unwrap(DNATypeInterface *ptr) {
	return reinterpret_cast<RTType *>(ptr);
}

ROSE_STATIC const RTType *unwrap(const DNATypeInterface *ptr) {
	return reinterpret_cast<const RTType *>(ptr);
}

eDNAType DNATypeInterface::type() const {
#define CASE_RETURN(tp) \
	case TP_##tp:       \
		return DNA_##tp

	switch (unwrap(this)->kind) {
		CASE_RETURN(VOID);
		CASE_RETURN(CHAR);
		CASE_RETURN(INT);
		CASE_RETURN(SHORT);
		CASE_RETURN(LONG);
		CASE_RETURN(LLONG);
		CASE_RETURN(FLOAT);
		CASE_RETURN(DOUBLE);
		CASE_RETURN(LDOUBLE);
		CASE_RETURN(ENUM);
		CASE_RETURN(PTR);
		CASE_RETURN(ARRAY);
		CASE_RETURN(FUNC);
		CASE_RETURN(STRUCT);
		CASE_RETURN(UNION);
		CASE_RETURN(QUALIFIED);
		CASE_RETURN(VARIADIC);
		CASE_RETURN(ELLIPSIS);
	}

#undef CASE_RETURN

	/**
	 * There is a translator interperted type that does not have a corresponding DNA type.
	 */
	ROSE_assert_unreachable();

	return DNA_VOID;
}

bool DNATypeInterface::basic(void) const {
	const RTType *impl = unwrap(this);

	return impl->is_basic;
}

bool DNATypeInterface::same(const DNATypeInterface *other) const {
	const RTType *impl = unwrap(this);

	return impl->same(unwrap(this), unwrap(other));
}
bool DNATypeInterface::compatible(const DNATypeInterface *other) const {
	const RTType *impl = unwrap(this);

	return impl->compatible(unwrap(this), unwrap(other));
}

const DNATypeInterface *DNATypeInterface::composite(void *context, const DNATypeInterface *other) const {
	RTContext *c = reinterpret_cast<RTContext *>(context);

	const RTType *impl = unwrap(this);
	const RTType *ret = impl->composite(c, unwrap(this), unwrap(other));

	return reinterpret_cast<const DNATypeInterface *>(ret);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name DNATypeBasicInterface
 * \{ */

bool DNATypeBasicInterface::is_unsigned() const {
	ROSE_assert(this->basic());
	return unwrap(this)->tp_basic.is_unsigned;
}

size_t DNATypeBasicInterface::rank() const {
	ROSE_assert(this->basic());
	return unwrap(this)->tp_basic.rank;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name DNATypeEnumInterface
 * \{ */

rose::StringRefNull DNATypeEnumItemInterface::identifier() const {
	const RTEnumItem *impl = reinterpret_cast<const RTEnumItem *>(this);

	return RT_token_as_string(impl->identifier);
}

template<typename T> T DNATypeEnumItemInterface::value(const DNATypeEnumInterface *type) const {
	const RTEnumItem *impl = reinterpret_cast<const RTEnumItem *>(this);

	return static_cast<T>(RT_node_evaluate_integer(RT_type_enum_value(unwrap(type), impl->identifier)));
}

const DNATypeInterface *DNATypeEnumInterface::base() const {
	ROSE_assert(this->type() == DNA_ENUM);
	return reinterpret_cast<const DNATypeInterface *>(unwrap(this)->tp_enum.underlying_type);
}

rose::StringRefNull DNATypeEnumInterface::identifier() const {
	ROSE_assert(this->type() == DNA_ENUM);
	return RT_token_as_string(unwrap(this)->tp_enum.identifier);
}

const ListBase *DNATypeEnumInterface::items() const {
	ROSE_assert(this->type() == DNA_ENUM);
	return &unwrap(this)->tp_enum.items;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name DNATypePointerInterface
 * \{ */

const DNATypeInterface *DNATypePointerInterface::pointee() const {
	ROSE_assert(this->type() == DNA_PTR);
	return reinterpret_cast<const DNATypeInterface *>(unwrap(this)->tp_base);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name DNATypeArrayInterface
 * \{ */

const DNATypeInterface *DNATypeArrayInterface::element() const {
	ROSE_assert(this->type() == DNA_ARRAY);
	return reinterpret_cast<const DNATypeInterface *>(unwrap(this)->tp_array.element_type);
}

size_t DNATypeArrayInterface::length() const {
	ROSE_assert(this->type() == DNA_ARRAY);
	return static_cast<size_t>(unwrap(this)->tp_array.length);
}

eDNAArrayBoundary DNATypeArrayInterface::boundary() const {
	ROSE_assert(this->type() == DNA_ARRAY);

#define CASE_RETURN(tp) \
	case tp:            \
		return DNA_##tp
	
	switch (unwrap(this)->tp_array.boundary) {
		CASE_RETURN(ARRAY_UNBOUNDED);
		CASE_RETURN(ARRAY_BOUNDED);
		CASE_RETURN(ARRAY_BOUNDED_STATIC);
		CASE_RETURN(ARRAY_VLA);
		CASE_RETURN(ARRAY_VLA_STATIC);
	}

#undef CASE_RETURN

	/**
	 * There is a translator boundary type that does not have a corresponding DNA boundary type.
	 */
	ROSE_assert_unreachable();

	return DNA_ARRAY_UNBOUNDED;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name DNATypeFunctionInterface
 * \{ */

const DNATypeInterface *DNATypeFunctionFieldInterface::type() const {
	const RTTypeParameter *impl = reinterpret_cast<const RTTypeParameter *>(this);

	return reinterpret_cast<const DNATypeInterface *>(impl->type);
}

const DNATypeInterface *DNATypeFunctionFieldInterface::adjusted() const {
	const RTTypeParameter *impl = reinterpret_cast<const RTTypeParameter *>(this);

	return reinterpret_cast<const DNATypeInterface *>(impl->adjusted);
}

rose::StringRefNull DNATypeFunctionFieldInterface::identifier() const {
	const RTTypeParameter *impl = reinterpret_cast<const RTTypeParameter *>(this);

	return RT_token_as_string(impl->identifier);
}

bool DNATypeFunctionInterface::is_variadic() const {
	ROSE_assert(this->type() == DNA_FUNC);
	return unwrap(this)->tp_function.is_variadic;
}

const DNATypeInterface *DNATypeFunctionInterface::ret() const {
	ROSE_assert(this->type() == DNA_FUNC);
	return reinterpret_cast<const DNATypeInterface *>(unwrap(this)->tp_function.return_type);
}

const ListBase *DNATypeFunctionInterface::params() const {
	ROSE_assert(this->type() == DNA_FUNC);
	return &unwrap(this)->tp_function.parameters;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name DNATypeStructInterface
 * \{ */

const DNATypeInterface *DNATypeStructFieldInterface::type() const {
	const RTField *impl = reinterpret_cast<const RTField *>(this);

	return reinterpret_cast<const DNATypeInterface *>(impl->type);
}

size_t DNATypeStructFieldInterface::alignment() const {
	const RTField *impl = reinterpret_cast<const RTField *>(this);

	return static_cast<size_t>(impl->alignment);
}

rose::StringRefNull DNATypeStructFieldInterface::identifier() const {
	const RTField *impl = reinterpret_cast<const RTField *>(this);

	return RT_token_as_string(impl->identifier);
}

rose::StringRefNull DNATypeStructInterface::identifier() const {
	ROSE_assert(this->type() == DNA_STRUCT);
	return RT_token_as_string(unwrap(this)->tp_struct.identifier);
}

const ListBase *DNATypeStructInterface::fields() const {
	ROSE_assert(this->type() == DNA_STRUCT);
	return &unwrap(this)->tp_struct.fields;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name DNATypeQualInterface
 * \{ */

const DNATypeInterface *DNATypeQualInterface::base() const {
	ROSE_assert(this->type() == DNA_QUALIFIED);
	return reinterpret_cast<const DNATypeInterface *>(unwrap(this)->tp_qualified.base);
}

bool DNATypeQualInterface::is_const() const {
	ROSE_assert(this->type() == DNA_QUALIFIED);
	return unwrap(this)->tp_qualified.qualification.is_constant;
}
bool DNATypeQualInterface::is_restrict() const {
	ROSE_assert(this->type() == DNA_QUALIFIED);
	return unwrap(this)->tp_qualified.qualification.is_restricted;
}
bool DNATypeQualInterface::is_volatile() const {
	ROSE_assert(this->type() == DNA_QUALIFIED);
	return unwrap(this)->tp_qualified.qualification.is_volatile;
}
bool DNATypeQualInterface::is_atomic() const {
	ROSE_assert(this->type() == DNA_QUALIFIED);
	return unwrap(this)->tp_qualified.qualification.is_atomic;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name DNAType
 * \{ */

eDNAType DNA_sdna_type_kind(SDNA *sdna, const DNAType *type) {
	UNUSED_VARS(sdna);
	return reinterpret_cast<const DNATypeInterface *>(type)->type();
}

bool DNA_sdna_type_basic(SDNA *sdna, const DNAType *type) {
	UNUSED_VARS(sdna);
	return reinterpret_cast<const DNATypeInterface *>(type)->basic();
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name DNATypeBasic
 * \{ */

bool DNA_sdna_basic_is_unsigned(SDNA *sdna, const DNATypeBasic *type) {
	UNUSED_VARS(sdna);
	return reinterpret_cast<const DNATypeBasicInterface *>(type)->is_unsigned();
}

size_t DNA_sdna_basic_rank(SDNA *sdna, const DNATypeBasic *type) {
	UNUSED_VARS(sdna);
	return reinterpret_cast<const DNATypeBasicInterface *>(type)->rank();
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name DNATypeEnum
 * \{ */

// DNATypeEnumItem

const char *DNA_sdna_enum_item_identifier(SDNA *sdna, const DNATypeEnum *type, const DNATypeEnumItem *item) {
	UNUSED_VARS(sdna, type);
	return reinterpret_cast<const DNATypeEnumItemInterface *>(item)->identifier().c_str();
}

// DNATypeEnum

const char *DNA_sdna_enum_identifier(SDNA *sdna, const DNATypeEnum *type) {
	UNUSED_VARS(sdna);
	return reinterpret_cast<const DNATypeEnumInterface *>(type)->identifier().c_str();
}

const DNAType *DNA_sdna_enum_underlying_type(SDNA *sdna, const DNATypeEnum *type) {
	UNUSED_VARS(sdna);
	return reinterpret_cast<const DNAType *>(reinterpret_cast<const DNATypeEnumInterface *>(type)->base());
}

const ListBase *DNA_sdna_enum_items(SDNA *sdna, const DNATypeEnum *type) {
	UNUSED_VARS(sdna);
	return reinterpret_cast<const DNATypeEnumInterface *>(type)->items();
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name DNATypePointer
 * \{ */

const DNAType *DNA_sdna_pointer_pointee(SDNA *sdna, const DNATypePointer *type) {
	UNUSED_VARS(sdna);
	return reinterpret_cast<const DNAType *>(reinterpret_cast<const DNATypePointerInterface *>(type)->pointee());
}

/** \} */


/* -------------------------------------------------------------------- */
/** \name DNATypeArray
 * \{ */

const DNAType *DNA_sdna_array_element(SDNA *sdna, const DNATypeArray *type) {
	UNUSED_VARS(sdna);
	return reinterpret_cast<const DNAType *>(reinterpret_cast<const DNATypeArrayInterface *>(type)->element());
}

eDNAArrayBoundary DNA_sdna_array_boundary(SDNA *sdna, const DNATypeArray *type) {
	UNUSED_VARS(sdna);
	return reinterpret_cast<const DNATypeArrayInterface *>(type)->boundary();
}

size_t DNA_sdna_array_length(SDNA *sdna, const DNATypeArray *type) {
	UNUSED_VARS(sdna);
	return reinterpret_cast<const DNATypeArrayInterface *>(type)->length();
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name DNATypeStruct
 * \{ */

// DNATypeStructField

const char *DNA_sdna_struct_field_identifier(SDNA *sdna, const DNATypeStruct *type, const DNATypeStructField *field) {
	UNUSED_VARS(sdna, type);
	return reinterpret_cast<const DNATypeStructFieldInterface *>(field)->identifier().c_str();
}

const DNAType *DNA_sdna_struct_field_type(SDNA *sdna, const DNATypeStruct *type, const DNATypeStructField *field) {
	UNUSED_VARS(sdna, type);
	return reinterpret_cast<const DNAType *>(reinterpret_cast<const DNATypeStructFieldInterface *>(field)->type());
}

size_t DNA_sdna_struct_field_alignment(SDNA *sdna, const DNATypeStruct *type, const DNATypeStructField *field) {
	UNUSED_VARS(sdna, type);
	return reinterpret_cast<const DNATypeStructFieldInterface *>(field)->alignment();
}

// DNATypeStruct

const char *DNA_sdna_struct_identifier(SDNA *sdna, const DNATypeStruct *type) {
	UNUSED_VARS(sdna);
	return reinterpret_cast<const DNATypeStructInterface *>(type)->identifier().c_str();
}

const ListBase *DNA_sdna_struct_fields(SDNA *sdna, const DNATypeStruct *type) {
	UNUSED_VARS(sdna);
	return reinterpret_cast<const DNATypeStructInterface *>(type)->fields();
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name DNATypeQualInterface
 * \{ */

const DNAType *DNA_sdna_qual_type(SDNA *sdna, const DNATypeQual *type) {
	UNUSED_VARS(sdna);
	return reinterpret_cast<const DNAType *>(reinterpret_cast<const DNATypeQualInterface *>(type)->base());
}

bool DNA_sdna_qual_is_const(SDNA *sdna, const DNATypeQual *type) {
	UNUSED_VARS(sdna);
	return reinterpret_cast<const DNATypeQualInterface *>(type)->is_const();
}
bool DNA_sdna_qual_is_restrict(SDNA *sdna, const DNATypeQual *type) {
	UNUSED_VARS(sdna);
	return reinterpret_cast<const DNATypeQualInterface *>(type)->is_restrict();
}
bool DNA_sdna_qual_is_volatile(SDNA *sdna, const DNATypeQual *type) {
	UNUSED_VARS(sdna);
	return reinterpret_cast<const DNATypeQualInterface *>(type)->is_volatile();
}
bool DNA_sdna_qual_is_atomic(SDNA *sdna, const DNATypeQual *type) {
	UNUSED_VARS(sdna);
	return reinterpret_cast<const DNATypeQualInterface *>(type)->is_atomic();
}

/** \} */
