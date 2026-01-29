#include "MEM_guardedalloc.h"

#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "KER_scene.h"

#include "DNA_object_types.h"
#include "DNA_scene_types.h"

#include "DEG_depsgraph.h"

#include "intern/depsgraph.hh"
#include "intern/depsgraph_tag.hh"

#include "intern/eval/deg_eval.hh"
#include "intern/eval/deg_eval_flush.hh"

namespace deg = rose::depsgraph;

static void deg_flush_updates_and_refresh(deg::Depsgraph *deg_graph) {
	/* Update the time on the cow scene. */
	if (deg_graph->scene_cow) {
		KER_scene_frame_set(deg_graph->scene_cow, deg_graph->ctime);
	}

	deg::graph_tag_ids_for_visible_update(deg_graph);
	deg::deg_graph_flush_updates(deg_graph);
	deg::deg_evaluate_on_refresh(deg_graph);
}

void DEG_evaluate_on_refresh(Depsgraph *graph) {
	deg::Depsgraph *deg_graph = reinterpret_cast<deg::Depsgraph *>(graph);
	const float ctime = KER_scene_frame_get(deg_graph->scene);

	if (ctime != deg_graph->ctime) {
		deg_graph->tag_time_source();
		deg_graph->ctime = ctime;
	}
	else if (deg_graph->scene->id.recalc & ID_RECALC_FRAME_CHANGE) {
		/* Comparing depsgraph & scene frame fails in the case of undo,
		 * since the undo state is stored before updates from the frame change have been applied.
		 * In this case reading back the undo state will behave as if no updates on frame change
		 * is needed as the #Depsgraph.ctime & frame will match the values in the input scene.
		 * Use #ID_RECALC_FRAME_CHANGE to detect that recalculation is necessary. see: T66913. */
		deg_graph->tag_time_source();
	}

	deg_flush_updates_and_refresh(deg_graph);
}

void DEG_evaluate_on_framechange(Depsgraph *graph) {
	deg::Depsgraph *deg_graph = reinterpret_cast<deg::Depsgraph *>(graph);

	deg_graph->tag_time_source();
	deg_graph->ctime = KER_scene_frame_get(deg_graph->scene);
	deg_flush_updates_and_refresh(deg_graph);
}
