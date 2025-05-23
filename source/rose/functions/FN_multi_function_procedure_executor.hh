#pragma once

/**/

#include "FN_multi_function_procedure.hh"

namespace rose::fn::multi_function {

/** A multi-function that executes a procedure internally. */
class ProcedureExecutor : public MultiFunction {
private:
	Signature signature_;
	const Procedure &procedure_;

public:
	ProcedureExecutor(const Procedure &procedure);

	void call(const IndexMask &mask, Params params, Context context) const override;

private:
	ExecutionHints get_execution_hints() const override;
};

}  // namespace rose::fn::multi_function
