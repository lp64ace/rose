#ifndef DEG_NODE_FACTORY_HH
#define DEG_NODE_FACTORY_HH

#include "MEM_guardedalloc.h"

#include "intern/depsgraph_types.hh"
#include "intern/node/deg_node.hh"

struct ID;

namespace rose::depsgraph {

struct DepsNodeFactory {
	virtual NodeType type() const = 0;
	virtual const char *type_name() const = 0;

	virtual int id_recalc_tag() const = 0;

	virtual Node *create_node(const ID *id, const char *subdata, const char *name) const = 0;
};

template<class ModeObjectType> struct DepsNodeFactoryImpl : public DepsNodeFactory {
	virtual NodeType type() const override;
	virtual const char *type_name() const override;

	virtual int id_recalc_tag() const override;

	virtual Node *create_node(const ID *id, const char *subdata, const char *name) const override;
};

/* Register typeinfo */
void register_node_typeinfo(DepsNodeFactory *factory);

/* Get typeinfo for specified type */
DepsNodeFactory *type_get_factory(NodeType type);

}  // namespace rose::depsgraph

/* -------------------------------------------------------------------- */
/** \name Node Factory Implementation
 * \{ */

namespace rose::depsgraph {

template<class ModeObjectType> NodeType DepsNodeFactoryImpl<ModeObjectType>::type() const {
	return ModeObjectType::typeinfo.type;
}

template<class ModeObjectType> const char *DepsNodeFactoryImpl<ModeObjectType>::type_name() const {
	return ModeObjectType::typeinfo.type_name;
}

template<class ModeObjectType> int DepsNodeFactoryImpl<ModeObjectType>::id_recalc_tag() const {
	return ModeObjectType::typeinfo.recalc;
}

template<class ModeObjectType> Node *DepsNodeFactoryImpl<ModeObjectType>::create_node(const ID *id, const char *subdata, const char *name) const {
	Node *node = new ModeObjectType();
	/* Populate base node settings. */
	node->type = type();
	/* Set name if provided, or use default type name. */
	if (name[0] != '\0') {
		node->name = name;
	}
	else {
		node->name = type_name();
	}
	node->init(id, subdata);
	return node;
}

}  // namespace rose::depsgraph

/** \} */

#endif
