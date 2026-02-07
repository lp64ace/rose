#ifndef DEPSGRAPH_RELATION_HH
#define DEPSGRAPH_RELATION_HH

#include "MEM_guardedalloc.h"

#include "LIB_utildefines.h"

namespace rose::depsgraph {

struct Node;

typedef enum RelationFlag {
	RELATION_FLAG_NONE = 0,
	/**
	 * "cyclic" link - when detecting cycles, this relationship was the one
	 * which triggers a cyclic relationship to exist in the graph.
	 */
	RELATION_FLAG_CYCLIC = (1 << 0),
	/** Update flush will not go through this relation. */
	RELATION_FLAG_NO_FLUSH = (1 << 1),
	/**
	 * Only flush along the relation is update comes from a node which was
	 * affected by user input.
	 */
	RELATION_FLAG_FLUSH_USER_EDIT_ONLY = (1 << 2),
	/** The relation can not be killed by the cyclic dependencies solver. */
	RELATION_FLAG_GODMODE = (1 << 4),
	/* Relation will check existence before being added. */
	RELATION_CHECK_BEFORE_ADD = (1 << 5),
} RelationFlag;

ENUM_OPERATORS(RelationFlag, RELATION_CHECK_BEFORE_ADD);

struct Relation {
	Relation(Node *from, Node *to, const char *description);
	~Relation();

	void unlink();

	/* the nodes in the relationship (since this is shared between the nodes) */
	Node *from;
	Node *to;

	/* relationship attributes */
	const char *name;  /* label for debugging */
	RelationFlag flag; /* Bitmask of RelationFlag */

	MEM_CXX_CLASS_ALLOC_FUNCS("Relation");
};

}  // namespace rose::depsgraph

#endif	// DEPSGRAPH_RELATION_HH
