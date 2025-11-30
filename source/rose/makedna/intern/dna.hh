#pragma once

#include "LIB_endian_switch.h"
#include "LIB_map.hh"
#include "LIB_set.hh"
#include "LIB_vector.hh"
#include "LIB_string_ref.hh"
#include "LIB_utildefines.h"

#include "genfile.h"

namespace rose::dna {

/* -------------------------------------------------------------------- */
/** \name DNATypeInterface
 * \{ */

struct DNATypeInterface {
	/**
	 * Returns the overall type of the struct, one of the #eDNAType values.
	 * \note This function is implemented in dna.cc since it needs to access the RTType enum.
	 */
	eDNAType type() const;

	/**
	 * This could eventually be made static, but since we want to reduce the overhead 
	 * and the fields that we need to keep in sync with the translator module,
	 * we keep it as a member function for now returning the #is_basic field.
	 */
	bool basic(void) const;

	/** These are the same functions used by the translator module, use with care! */

	bool same(const DNATypeInterface *other) const;
	bool compatible(const DNATypeInterface *other) const;

	const DNATypeInterface *composite(void *context, const DNATypeInterface *other) const;
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name DNATypeBasicInterface
 * \{ */

struct DNATypeBasicInterface : public DNATypeInterface {
	/** Returns a boolean indicating wether or not this basic type has the unsigned specifier. */
	bool is_unsigned() const;

	/** Returns the size in bytes of the struct, not this is not used by bitfields! */
	size_t rank() const;
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name DNATypeEnumInterface
 * \{ */

struct DNATypeEnumInterface;

struct DNATypeEnumItemInterface {
	rose::StringRefNull identifier() const;

	template<typename T> T value(const DNATypeEnumInterface *type) const;
};

template<> int8_t DNATypeEnumItemInterface::value(const DNATypeEnumInterface *type) const;
template<> int16_t DNATypeEnumItemInterface::value(const DNATypeEnumInterface *type) const;
template<> int32_t DNATypeEnumItemInterface::value(const DNATypeEnumInterface *type) const;
template<> int64_t DNATypeEnumItemInterface::value(const DNATypeEnumInterface *type) const;

template<> uint8_t DNATypeEnumItemInterface::value(const DNATypeEnumInterface *type) const;
template<> uint16_t DNATypeEnumItemInterface::value(const DNATypeEnumInterface *type) const;
template<> uint32_t DNATypeEnumItemInterface::value(const DNATypeEnumInterface *type) const;
template<> uint64_t DNATypeEnumItemInterface::value(const DNATypeEnumInterface *type) const;

struct DNATypeEnumInterface : public DNATypeInterface {
	/** Returns a pointer to the base type this enum type uses. */
	const DNATypeInterface *base() const;

	rose::StringRefNull identifier() const;

	const ListBase *items() const;
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name DNATypePointerInterface
 * \{ */

struct DNATypePointerInterface : public DNATypeInterface {
	/** Returns a pointer to the pointee type this pointer points at. */
	const DNATypeInterface *pointee() const;
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name DNATypeArrayInterface
 * \{ */

struct DNATypeArrayInterface : public DNATypeInterface {
	/** Returns a pointer to the element type this array uses. */
	const DNATypeInterface *element() const;

	size_t length() const;

	eDNAArrayBoundary boundary() const;
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name DNATypeFunctionInterface
 * \{ */

struct DNATypeFunctionFieldInterface {
	/** Returns a pointer to the element type this array uses. */
	const DNATypeInterface *type() const;
	const DNATypeInterface *adjusted() const;

	rose::StringRefNull identifier() const;
};

struct DNATypeFunctionInterface : public DNATypeInterface {
	bool is_variadic() const;

	/**
	 * The return type of the function, this will never be NULL!
	 */
	const DNATypeInterface *ret() const;

	/** The parameter listbase for the function, this will never be NULL but it might be empty! */
	const ListBase *params() const;
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name DNATypeStructInterface
 * \{ */

struct DNATypeStructFieldInterface {
	/** Returns a pointer to the element type this field uses. */
	const DNATypeInterface *type() const;

	size_t alignment() const;

	rose::StringRefNull identifier() const;
};

struct DNATypeStructInterface : public DNATypeInterface {
	rose::StringRefNull identifier() const;

	const ListBase *fields() const;
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name DNATypeQualInterface
 * \{ */

struct DNATypeQualInterface : public DNATypeInterface {
	/** Returns a pointer to the base type this qualified type uses. */
	const DNATypeInterface *base() const;

	bool is_const() const;
	bool is_restrict() const;
	bool is_volatile() const;
	bool is_atomic() const;
};

/** \} */

}
