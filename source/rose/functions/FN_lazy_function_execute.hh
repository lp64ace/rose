#pragma once

/**
 * This file contains common utilities for actually executing a lazy-function.
 */

#include "LIB_parameter_pack_utils.hh"

#include "FN_lazy_function.hh"

namespace rose::fn::lazy_function {

/**
 * Most basic implementation of #Params. It does not actually implement any logic for how to
 * retrieve inputs or set outputs. Instead, code using #BasicParams has to implement that.
 */
class BasicParams : public Params {
private:
	const Span<GMutablePointer> inputs_;
	const Span<GMutablePointer> outputs_;
	MutableSpan<std::optional<ValueUsage>> input_usages_;
	Span<ValueUsage> output_usages_;
	MutableSpan<bool> set_outputs_;

public:
	BasicParams(const LazyFunction &fn, Span<GMutablePointer> inputs, Span<GMutablePointer> outputs, MutableSpan<std::optional<ValueUsage>> input_usages, Span<ValueUsage> output_usages, MutableSpan<bool> set_outputs);

	void *try_get_input_data_ptr_impl(const int index) const override;
	void *try_get_input_data_ptr_or_request_impl(const int index) override;
	void *get_output_data_ptr_impl(const int index) override;
	void output_set_impl(const int index) override;
	bool output_was_set_impl(const int index) const override;
	ValueUsage get_output_usage_impl(const int index) const override;
	void set_input_unused_impl(const int index) override;
	bool try_enable_multi_threading_impl() override;
};

/**
 * Wraps an existing #Params. This should be used when a lazy-function internally contains another
 * lazy-function that handles a subset or the inputs and outputs.
 */
class RemappedParams : public Params {
private:
	Params &base_params_;
	Span<int> input_map_;
	Span<int> output_map_;
	bool &multi_threading_enabled_;

public:
	RemappedParams(const LazyFunction &fn, Params &base_params, Span<int> input_map, Span<int> output_map, bool &multi_threading_enabled);

	void *try_get_input_data_ptr_impl(const int index) const override;
	void *try_get_input_data_ptr_or_request_impl(const int index) override;
	void *get_output_data_ptr_impl(const int index) override;
	void output_set_impl(const int index) override;
	bool output_was_set_impl(const int index) const override;
	ValueUsage get_output_usage_impl(const int index) const override;
	void set_input_unused_impl(const int index) override;
	bool try_enable_multi_threading_impl() override;
};

namespace detail {

/**
 * Utility to implement #execute_lazy_function_eagerly.
 */
template<typename... Inputs, typename... Outputs, size_t... InIndices, size_t... OutIndices> inline void execute_lazy_function_eagerly_impl(const LazyFunction &fn, UserData *user_data, LocalUserData *local_user_data, std::tuple<Inputs...> &inputs, std::tuple<Outputs *...> &outputs, std::index_sequence<InIndices...> /*in_indices*/, std::index_sequence<OutIndices...> /*out_indices*/) {
	constexpr size_t InputsNum = sizeof...(Inputs);
	constexpr size_t OutputsNum = sizeof...(Outputs);
	std::array<GMutablePointer, InputsNum> input_pointers;
	std::array<GMutablePointer, OutputsNum> output_pointers;
	std::array<std::optional<ValueUsage>, InputsNum> input_usages;
	std::array<ValueUsage, OutputsNum> output_usages;
	std::array<bool, OutputsNum> set_outputs;
	(
		[&]() {
			constexpr size_t I = InIndices;
			/* Use `typedef` instead of `using` to work around a compiler bug. */
			typedef Inputs T;
			const CPPType &type = CPPType::get<T>();
			input_pointers[I] = {type, &std::get<I>(inputs)};
		}(),
		...);
	(
		[&]() {
			constexpr size_t I = OutIndices;
			/* Use `typedef` instead of `using` to work around a compiler bug. */
			typedef Outputs T;
			const CPPType &type = CPPType::get<T>();
			output_pointers[I] = {type, std::get<I>(outputs)};
		}(),
		...);
	output_usages.fill(ValueUsage::Used);
	set_outputs.fill(false);
	LinearAllocator<> allocator;
	Context context(fn.init_storage(allocator), user_data, local_user_data);
	BasicParams params{fn, input_pointers, output_pointers, input_usages, output_usages, set_outputs};
	fn.execute(params, context);
	fn.destruct_storage(context.storage);

	/* Make sure all outputs have been computed. */
	ROSE_assert(!Span<bool>(set_outputs).contains(false));
}

}  // namespace detail

/**
 * In some cases (mainly for tests), the set of inputs and outputs for a lazy-function is known at
 * compile time and one just wants to compute the outputs based on the inputs, without any
 * laziness.
 *
 * This function does exactly that. It takes all inputs in a tuple and writes the outputs to points
 * provided in a second tuple. Since all inputs have to be provided, the lazy-function has to
 * compute all outputs.
 */
template<typename... Inputs, typename... Outputs> inline void execute_lazy_function_eagerly(const LazyFunction &fn, UserData *user_data, LocalUserData *local_user_data, std::tuple<Inputs...> inputs, std::tuple<Outputs *...> outputs) {
	ROSE_assert(fn.inputs().size() == sizeof...(Inputs));
	ROSE_assert(fn.outputs().size() == sizeof...(Outputs));
	detail::execute_lazy_function_eagerly_impl(fn, user_data, local_user_data, inputs, outputs, std::make_index_sequence<sizeof...(Inputs)>(), std::make_index_sequence<sizeof...(Outputs)>());
}

}  // namespace rose::fn::lazy_function
