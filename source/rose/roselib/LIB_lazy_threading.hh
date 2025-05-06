#ifndef LIB_LAZY_THREADING_HH
#define LIB_LAZY_THREADING_HH

#include "LIB_function_ref.hh"

namespace rose::lazy_threading {

/**
 * Tell task schedulers on the current thread that it is about to start a long computation
 * and that other waiting tasks should better be moved to another thread if possible.
 */
void send_hint();

/**
 * Used by the task scheduler to receive hints from current tasks that they will take a while.
 * This should only be allocated on the stack.
 */
class HintReceiver {
public:
	/**
	 * The passed in function is called when a task signals that it will take a while.
	 * \note The function has to stay alive after the call to the constructor. So one must not pass a
	 * lambda directly into this constructor but store it in a separate variable on the stack first.
	 */
	HintReceiver(FunctionRef<void()> fn);
	~HintReceiver();
};

/**
 * Used to make sure that lazy-threading hints don't propagate through task isolation. This is
 * necessary to avoid deadlocks when isolated regions are used together with e.g. task pools. For
 * more info see the comment on #LIB_task_isolate.
 */
class ReceiverIsolation {
public:
	ReceiverIsolation();
	~ReceiverIsolation();
};

}  // namespace rose::lazy_threading

#endif	// LIB_LAZY_THREADING_HH
