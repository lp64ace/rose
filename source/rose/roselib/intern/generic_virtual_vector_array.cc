#include "LIB_generic_virtual_vector_array.hh"

namespace rose {

void GVArray_For_GVVectorArrayIndex::get(const size_t index_in_vector, void *r_value) const {
	vector_array_.get_vector_element(index_, index_in_vector, r_value);
}

void GVArray_For_GVVectorArrayIndex::get_to_uninitialized(const size_t index_in_vector, void *r_value) const {
	type_->default_construct(r_value);
	vector_array_.get_vector_element(index_, index_in_vector, r_value);
}

size_t GVVectorArray_For_SingleGVArray::get_vector_size_impl(const size_t index) const {
	return varray_.size();
}

void GVVectorArray_For_SingleGVArray::get_vector_element_impl(const size_t index, const size_t index_in_vector, void *r_value) const {
	varray_.get(index_in_vector, r_value);
}

bool GVVectorArray_For_SingleGVArray::is_single_vector_impl() const {
	return true;
}

size_t GVVectorArray_For_SingleGSpan::get_vector_size_impl(const size_t index) const {
	return span_.size();
}

void GVVectorArray_For_SingleGSpan::get_vector_element_impl(const size_t index, const size_t index_in_vector, void *r_value) const {
	type_->copy_assign(span_[index_in_vector], r_value);
}

bool GVVectorArray_For_SingleGSpan::is_single_vector_impl() const {
	return true;
}

}  // namespace rose
