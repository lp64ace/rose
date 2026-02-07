#ifndef DEG_BUILDER_HH
#define DEG_BUILDER_HH

struct Base;
struct ID;
struct Main;
struct Object;
struct PoseChannel;

namespace rose::depsgraph {

struct Depsgraph;
class DepsgraphBuilderCache;

class DepsgraphBuilder {
public:
	virtual ~DepsgraphBuilder() = default;

	virtual bool need_pull_base_into_graph(Base *base);

protected:
	/* NOTE: The builder does NOT take ownership over any of those resources. */
	DepsgraphBuilder(Main *main, Depsgraph *graph, DepsgraphBuilderCache *cache);

	/* State which never changes, same for the whole builder time. */
	Main *main_;
	Depsgraph *graph_;
	DepsgraphBuilderCache *cache_;
};

bool deg_check_id_in_depsgraph(const Depsgraph *graph, ID *id_orig);
bool deg_check_base_in_depsgraph(const Depsgraph *graph, Base *base);
void deg_graph_build_finalize(Main *main, Depsgraph *graph);

}  // namespace rose::depsgraph

#endif	// !DEG_BUILDER_HH
