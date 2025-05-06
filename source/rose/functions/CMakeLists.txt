# -----------------------------------------------------------------------------
# Define Include Directories

set(INC
	# Internal Include Directories
	PUBLIC .
	
	# External Include Directories
	
)

# -----------------------------------------------------------------------------
# Define System Include Directories

set(INC_SYS
	# External System Include Directories
	
)

# -----------------------------------------------------------------------------
# Define Source Files

set(SRC
	FN_field.hh
	FN_lazy_function.hh
	FN_lazy_function_execute.hh
	FN_lazy_function_graph.hh
	FN_lazy_function_graph_executor.hh
	FN_multi_function.hh
	FN_multi_function_builder.hh
	FN_multi_function_context.hh
	FN_multi_function_data_type.hh
	FN_multi_function_params.hh
	FN_multi_function_param_type.hh
	FN_multi_function_procedure.hh
	FN_multi_function_procedure_builder.hh
	FN_multi_function_procedure_executor.hh
	FN_multi_function_procedure_optimization.hh
	FN_multi_function_signature.hh
	
	intern/field.cc
	intern/lazy_function.cc
	intern/lazy_function_execute.cc
	intern/lazy_function_graph.cc
	intern/lazy_function_graph_executor.cc
	intern/multi_function.cc
	intern/multi_function_builder.cc
	intern/multi_function_params.cc
	intern/multi_function_procedure.cc
	intern/multi_function_procedure_builder.cc
	intern/multi_function_procedure_executor.cc
	intern/multi_function_procedure_optimization.cc
)

# -----------------------------------------------------------------------------
# Define Library Dependencies

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	PUBLIC rose::source::roselib
	PUBLIC rose::source::dna
	
	# External Library Dependencies
	
)

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(functions "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::source::functions ALIAS functions)
