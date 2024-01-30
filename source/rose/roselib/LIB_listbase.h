#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "DNA_listbase.h"

#include "LIB_compiler_attrs.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Insert a new link at the front of a listbase. */
void LIB_addhead(struct ListBase *listbase, void *vlink);
/** Insert a new link at the end of a listbase. */
void LIB_addtail(struct ListBase *listbase, void *vlink);

/** Removes the first element of the listbase and return it. */
void *LIB_pophead(struct ListBase *listbase);
/** Removes the last element of the listbase and return it. */
void *LIB_poptail(struct ListBase *listbase);

void LIB_remlink(struct ListBase *listbase, void *vlink);

/**
 * Iterate over the links in the list and return the first one that contains the string specified after #offset bytes.
 * \param listbase The list we want to iterate over.
 * \param str The string we are looking for.
 * \param offset The offset we want to find the string after the beginning of the link.
 * \return The link that contains the specified string or NULL.
 */
void *LIB_findstring(const struct ListBase *listbase, const char *str, const size_t offset);

/**
 * Iterate over the links in the list and return the last one that contains the string specified after #offset bytes.
 * \param listbase The list we want to iterate over.
 * \param str The string we are looking for.
 * \param offset The offset we want to find the string after the beginning of the link.
 * \return The link that contains the specified string or NULL.
 */
void *LIB_rfindstring(const struct ListBase *listbase, const char *str, const size_t offset);

/**
 * Iterate over the links in the list and return the first one that contains the bytes specified after #offset bytes.
 * \param listbase The list we want to iterate over.
 * \param str The string we are looking for.
 * \param offset The offset we want to find the string after the beginning of the link.
 * \return The link that contains the specified string or NULL.
 */
void *LIB_findbytes(const struct ListBase *listbase, const void *bytes, const size_t length, const size_t offset);

/**
 * Iterate over the links in the list and return the last one that contains the bytes specified after #offset bytes.
 * \param listbase The list we want to iterate over.
 * \param str The string we are looking for.
 * \param offset The offset we want to find the string after the beginning of the link.
 * \return The link that contains the specified string or NULL.
 */
void *LIB_rfindbytes(const struct ListBase *listbase, const void *bytes, const size_t length, const size_t offset);

/**
 * Iterate over the links in the list and return the first one that contains the pointer specified after #offset bytes.
 * \param listbase The list we want to iterate over.
 * \param str The string we are looking for.
 * \param offset The offset we want to find the string after the beginning of the link.
 * \return The link that contains the specified string or NULL.
 */
void *LIB_findptr(const struct ListBase *listbase, void *ptr, const size_t offset);

/**
 * Iterate over the links in the list and return the last one that contains the pointer specified after #offset bytes.
 * \param listbase The list we want to iterate over.
 * \param str The string we are looking for.
 * \param offset The offset we want to find the string after the beginning of the link.
 * \return The link that contains the specified string or NULL.
 */
void *LIB_rfindptr(const struct ListBase *listbase, void *ptr, const size_t offset);

/** Returns true if the listbase is empty, prefer this over counting elements. */
ROSE_INLINE bool LIB_listbase_is_empty(const struct ListBase *listbase);
/** Returns true if the listbase has a single item, prefer this over counting elements. */
ROSE_INLINE bool LIB_listbase_is_single(const struct ListBase *listbase);
/** Removes all the items from this listbase. */
ROSE_INLINE void LIB_listbase_clear(struct ListBase *listbase);

#ifdef __cplusplus
}
#endif

#include "LIB_listbase_inline.h"

#define LISTBASE_FOREACH(type, var, list) \
	for (type var = (type)((list)->first); var != NULL; var = (type)(((Link *)(var))->next))

#define LISTBASE_FOREACH_BACKWARD(type, var, list) \
	for (type var = (type)((list)->last); var != NULL; var = (type)(((Link *)(var))->prev))

/**
 * A version of #LISTBASE_FOREACH that supports incrementing an index variable at every step.
 * Including this in the macro helps prevent mistakes where "continue" mistakenly skips the
 * incrementation.
 */
#define LISTBASE_FOREACH_INDEX(type, var, list, index_var) \
	for (type var = (((void)(index_var = 0)), (type)((list)->first)); var != NULL; \
		 var = (type)(((Link *)(var))->next), index_var++)

/**
 * A version of #LISTBASE_FOREACH that supports removing the item we're looping over.
 */
#define LISTBASE_FOREACH_MUTABLE(type, var, list) \
	for (type var = (type)((list)->first), *var##_iter_next; \
		 ((var != NULL) ? ((void)(var##_iter_next = (type)(((Link *)(var))->next)), 1) : 0); \
		 var = var##_iter_next)

/**
 * A version of #LISTBASE_FOREACH_BACKWARD that supports removing the item we're looping over.
 */
#define LISTBASE_FOREACH_BACKWARD_MUTABLE(type, var, list) \
	for (type var = (type)((list)->last), *var##_iter_prev; \
		 ((var != NULL) ? ((void)(var##_iter_prev = (type)(((Link *)(var))->prev)), 1) : 0); \
		 var = var##_iter_prev)
