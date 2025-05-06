#include "MEM_guardedalloc.h"

#include "LIB_task.h"

#include <memory>
#include <vector>

/* Task Graph */
struct TaskGraph {
	std::vector<std::unique_ptr<TaskNode>> nodes;

#ifdef WITH_CXX_GUARDEDALLOC
	MEM_CXX_CLASS_ALLOC_FUNCS("task_graph:TaskGraph")
#endif
};

/* TaskNode - a node in the task graph. */
struct TaskNode {
	/* Successors to execute after this task, for serial execution fallback. */
	std::vector<TaskNode *> successors;

	/* User function to be executed with given task data. */
	TaskGraphNodeRunFunction run_func;
	void *task_data;
	/* Optional callback to free task data along with the graph. If task data
	 * is shared between nodes, only a single task node should free the data. */
	TaskGraphNodeFreeFunction free_func;

	TaskNode(TaskGraph *task_graph, TaskGraphNodeRunFunction run_func, void *task_data, TaskGraphNodeFreeFunction free_func) : run_func(run_func), task_data(task_data), free_func(free_func) {
		EXPR_NOP(task_graph);
	}

	TaskNode(const TaskNode &other) = delete;
	TaskNode &operator=(const TaskNode &other) = delete;

	~TaskNode() {
		if (task_data && free_func) {
			free_func(task_data);
		}
	}

	void run_serial() {
		run_func(task_data);
		for (TaskNode *successor : successors) {
			successor->run_serial();
		}
	}

#ifdef WITH_CXX_GUARDEDALLOC
	MEM_CXX_CLASS_ALLOC_FUNCS("task_graph:TaskNode")
#endif
};

TaskGraph *LIB_task_graph_create() {
	return new TaskGraph();
}

void LIB_task_graph_free(TaskGraph *task_graph) {
	delete task_graph;
}

void LIB_task_graph_work_and_wait(TaskGraph *task_graph) {
	EXPR_NOP(task_graph);
}

struct TaskNode *LIB_task_graph_node_create(struct TaskGraph *task_graph, TaskGraphNodeRunFunction run, void *user_data, TaskGraphNodeFreeFunction free_func) {
	TaskNode *task_node = new TaskNode(task_graph, run, user_data, free_func);
	task_graph->nodes.push_back(std::unique_ptr<TaskNode>(task_node));
	return task_node;
}

bool LIB_task_graph_node_push_work(struct TaskNode *task_node) {
	task_node->run_serial();
	return true;
}

void LIB_task_graph_edge_create(struct TaskNode *from_node, struct TaskNode *to_node) {
	from_node->successors.push_back(to_node);
}
