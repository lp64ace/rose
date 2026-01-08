#include "MEM_guardedalloc.h"

#include "DNA_ID.h"
#include "DNA_customdata_types.h"
#include "DNA_meshdata_types.h"
#include "DNA_vector_types.h"

#include "LIB_bitmap.h"
#include "LIB_color.hh"
#include "LIB_cpp_type.hh"
#include "LIB_endian_switch.h"
#include "LIB_implicit_sharing.hh"
#include "LIB_index_range.hh"
#include "LIB_math_base.h"
#include "LIB_math_bit.h"
#include "LIB_math_color_blend.h"
#include "LIB_math_matrix.h"
#include "LIB_math_matrix.hh"
#include "LIB_math_vector.h"
#include "LIB_math_vector.hh"
#include "LIB_mempool.h"
#include "LIB_path_utils.h"
#include "LIB_set.hh"
#include "LIB_span.hh"
#include "LIB_string.h"
#include "LIB_string_ref.hh"
#include "LIB_string_utf.h"
#include "LIB_string_utils.h"
#include "LIB_utildefines.h"

#include "KER_customdata.h"
#include "KER_main.h"

#include <inttypes.h>
#include <optional>

/* number of layers to add when growing a CustomData object */
#define CUSTOMDATA_GROW 16

/* ensure typemap size is ok */
ROSE_STATIC_ASSERT(ARRAY_SIZE(((CustomData *)nullptr)->typemap) == CD_NUMTYPES, "size mismatch");

/* -------------------------------------------------------------------- */
/** \name Mesh Mask Utilities
 * \{ */

void CustomData_MeshMasks_update(CustomData_MeshMasks *mask_dst, const CustomData_MeshMasks *mask_src) {
	mask_dst->vmask |= mask_src->vmask;
	mask_dst->emask |= mask_src->emask;
	mask_dst->fmask |= mask_src->fmask;
	mask_dst->pmask |= mask_src->pmask;
	mask_dst->lmask |= mask_src->lmask;
}

bool CustomData_MeshMasks_are_matching(const CustomData_MeshMasks *mask_ref, const CustomData_MeshMasks *mask_required) {
	return (((mask_required->vmask & mask_ref->vmask) == mask_required->vmask) && ((mask_required->emask & mask_ref->emask) == mask_required->emask) && ((mask_required->fmask & mask_ref->fmask) == mask_required->fmask) && ((mask_required->pmask & mask_ref->pmask) == mask_required->pmask) && ((mask_required->lmask & mask_ref->lmask) == mask_required->lmask));
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Layer Type Information
 * \{ */

struct LayerTypeInfo {
	int size; /* The memory size of one element of this layer's data. */

	const char *structname; /* Name of the struct used, for file writting. */
	int structnum;			/* Number of structs per element, for file writting. */

	/**
	 * Default layer name.
	 *
	 * \note when null this is a way to ensure there is only ever one item
	 * see: CustomData_layertype_is_singleton().
	 */
	const char *defaultname;

	/**
	 * A function to copy count elements of this layer's data
	 * (deep copy if appropriate)
	 * if null, memcpy will be used.
	 */
	cd_copy copy;

	/**
	 * A function to free any dynamically allocated components of this
	 * layer's data (note the data pointer itself should not be freed)
	 * size should be the size of one element of this layer's data (e.g.
	 * LayerTypeInfo.size)
	 */
	void (*free)(void *data, int count, int size);

	/**
	 * a function to interpolate between count source elements of this
	 * layer's data and store the result in dest
	 * if weights == null or sub_weights == null, they should default to 1
	 *
	 * weights gives the weight for each element in sources
	 * sub_weights gives the sub-element weights for each element in sources
	 *    (there should be (sub element count)^2 weights per element)
	 * count gives the number of elements in sources
	 *
	 * \note in some cases \a dest pointer is in \a sources
	 *       so all functions have to take this into account and delay
	 *       applying changes while reading from sources.
	 *       See bug #32395 - Campbell.
	 */
	cd_interp interp;

	/** a function to swap the data in corners of the element */
	void (*swap)(void *data, const int *corner_indices);

	/**
	 * Set values to the type's default. If undefined, the default is assumed to be zeroes.
	 * Memory pointed to by #data is expected to be uninitialized.
	 */
	void (*set_default_value)(void *data, int count);
	/**
	 * Construct and fill a valid value for the type. Necessary for non-trivial types.
	 * Memory pointed to by #data is expected to be uninitialized.
	 */
	void (*construct)(void *data, int count);

	/** A function used by mesh validating code, must ensures passed item has valid data. */
	cd_validate validate;

	/** functions necessary for geometry collapse */
	bool (*equal)(const void *data1, const void *data2);
	void (*multiply)(void *data, float fac);
	void (*initminmax)(void *min, void *max);
	void (*add)(void *data1, const void *data2);
	void (*dominmax)(const void *data1, void *min, void *max);
	void (*copyvalue)(const void *source, void *dest, int mixmode, const float mixfactor);

	/** a function to determine max allowed number of layers,
	 * should be null or return -1 if no limit */
	int (*layers_max)();
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name Callbacks for (#MDeformVert, #CD_MDEFORMVERT)
 * \{ */

static void layerCopy_mdeformvert(const void *source, void *dest, const int count) {
	int i, size = sizeof(MDeformVert);

	memcpy(dest, source, count * size);

	for (i = 0; i < count; i++) {
		MDeformVert *dvert = static_cast<MDeformVert *>(POINTER_OFFSET(dest, i * size));

		if (dvert->totweight) {
			MDeformWeight *dw = (MDeformWeight *)MEM_mallocN(sizeof(MDeformWeight) * dvert->totweight, __func__);

			memcpy(dw, dvert->dw, dvert->totweight * sizeof(*dw));
			dvert->dw = dw;
		}
		else {
			dvert->dw = nullptr;
		}
	}
}

static void layerFree_mdeformvert(void *data, const int count, const int /* size */) {
	for (MDeformVert &dvert : rose::MutableSpan(static_cast<MDeformVert *>(data), count)) {
		if (dvert.dw) {
			MEM_freeN(dvert.dw);
			dvert.dw = nullptr;
			dvert.totweight = 0;
		}
	}
}

static void layerInterp_mdeformvert(const void **sources, const float *weights, const float * /* sub_weights */, const int count, void *dest) {
	/* A single linked list of #MDeformWeight's.
	 * use this to avoid double allocations (which #LinkNode would do). */
	struct MDeformWeight_Link {
		MDeformWeight_Link *next;
		MDeformWeight dw;
	};

	MDeformVert *dvert = static_cast<MDeformVert *>(dest);
	MDeformWeight_Link *dest_dwlink = nullptr;
	MDeformWeight_Link *node;

	/* build a list of unique def_nrs for dest */
	int totweight = 0;
	for (int i = 0; i < count; i++) {
		const MDeformVert *source = static_cast<const MDeformVert *>(sources[i]);
		float interp_weight = weights[i];

		for (int j = 0; j < source->totweight; j++) {
			MDeformWeight *dw = &source->dw[j];
			float weight = dw->weight * interp_weight;

			if (weight == 0.0f) {
				continue;
			}

			for (node = dest_dwlink; node; node = node->next) {
				MDeformWeight *tmp_dw = &node->dw;

				if (tmp_dw->def_nr == dw->def_nr) {
					tmp_dw->weight += weight;
					break;
				}
			}

			/* if this def_nr is not in the list, add it */
			if (!node) {
				MDeformWeight_Link *tmp_dwlink = static_cast<MDeformWeight_Link *>(alloca(sizeof(*tmp_dwlink)));
				tmp_dwlink->dw.def_nr = dw->def_nr;
				tmp_dwlink->dw.weight = weight;

				/* Inline linked-list. */
				tmp_dwlink->next = dest_dwlink;
				dest_dwlink = tmp_dwlink;

				totweight++;
			}
		}
	}

	/* Delay writing to the destination in case dest is in sources. */

	/* now we know how many unique deform weights there are, so realloc */
	if (dvert->dw && (dvert->totweight == totweight)) {
		/* pass (fast-path if we don't need to realloc). */
	}
	else {
		if (dvert->dw) {
			MEM_freeN(dvert->dw);
		}

		if (totweight) {
			dvert->dw = (MDeformWeight *)MEM_mallocN(sizeof(MDeformWeight) * totweight, __func__);
		}
	}

	if (totweight) {
		dvert->totweight = totweight;
		int i = 0;
		for (node = dest_dwlink; node; node = node->next, i++) {
			node->dw.weight = std::min(node->dw.weight, 1.0f);
			dvert->dw[i] = node->dw;
		}
	}
	else {
		*dvert = MDeformVert{};
	}
}

static void layerConstruct_mdeformvert(void *data, const int count) {
	std::fill_n(static_cast<MDeformVert *>(data), count, MDeformVert{});
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Callbacks for (#vec3f, #CD_NORMAL)
 * \{ */

static void layerInterp_normal(const void **sources, const float *weights, const float * /*sub_weights*/, const int count, void *dest) {
	/* NOTE: This is linear interpolation, which is not optimal for vectors.
	 * Unfortunately, spherical interpolation of more than two values is hairy,
	 * so for now it will do... */
	float no[3] = {0.0f};

	for (const int i : rose::IndexRange(count)) {
		madd_v3_v3fl(no, (const float *)sources[i], weights[i]);
	}

	/* Weighted sum of normalized vectors will **not** be normalized, even if weights are. */
	normalize_v3_v3((float *)dest, no);
}

static void layerCopyValue_normal(const void *source, void *dest, const int mixmode, const float mixfactor) {
	const float *no_src = (const float *)source;
	float *no_dst = (float *)dest;
	float no_tmp[3];

	if (ELEM(mixmode, CDT_MIX_NOMIX, CDT_MIX_REPLACE_ABOVE_THRESHOLD, CDT_MIX_REPLACE_BELOW_THRESHOLD)) {
		/* Above/below threshold modes are not supported here, fallback to nomix (just in case). */
		copy_v3_v3(no_dst, no_src);
	}
	else { /* Modes that support 'real' mix factor. */
		/* Since we normalize in the end, MIX and ADD are the same op here. */
		if (ELEM(mixmode, CDT_MIX_MIX, CDT_MIX_ADD)) {
			add_v3_v3v3(no_tmp, no_dst, no_src);
			normalize_v3(no_tmp);
		}
		else if (mixmode == CDT_MIX_SUB) {
			sub_v3_v3v3(no_tmp, no_dst, no_src);
			normalize_v3(no_tmp);
		}
		else if (mixmode == CDT_MIX_MUL) {
			mul_v3_v3v3(no_tmp, no_dst, no_src);
			normalize_v3(no_tmp);
		}
		else {
			copy_v3_v3(no_tmp, no_src);
		}
		interp_v3_v3v3_slerp_safe(no_dst, no_dst, no_tmp, mixfactor);
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Callbacks for (#MFloatProperty, #CD_PROP_FLOAT)
 * \{ */

static void layerCopy_propFloat(const void *source, void *dest, const int count) {
	memcpy(dest, source, sizeof(MFloatProperty) * count);
}

static void layerInterp_propFloat(const void **sources, const float *weights, const float * /*sub_weights*/, const int count, void *dest) {
	float result = 0.0f;
	for (int i = 0; i < count; i++) {
		const float interp_weight = weights[i];
		const float src = *(const float *)sources[i];
		result += src * interp_weight;
	}
	*(float *)dest = result;
}

static bool layerValidate_propFloat(void *data, const uint totitems, const bool do_fixes) {
	MFloatProperty *fp = static_cast<MFloatProperty *>(data);
	bool has_errors = false;

	for (uint i = 0; i < totitems; i++, fp++) {
		if (!isfinite(fp->f)) {
			if (do_fixes) {
				fp->f = 0.0f;
			}
			has_errors = true;
		}
	}

	return has_errors;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Callbacks for (#MIntProperty, #CD_PROP_INT32)
 * \{ */

static void layerInterp_propInt(const void **sources, const float *weights, const float * /*sub_weights*/, const int count, void *dest) {
	float result = 0.0f;
	for (const size_t i : rose::IndexRange(count)) {
		const float weight = weights[i];
		const float src = *static_cast<const int *>(sources[i]);
		result += src * weight;
	}
	const int rounded_result = int(round(result));
	*static_cast<int *>(dest) = rounded_result;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Callbacks for (#MStringProperty, #CD_PROP_STRING)
 * \{ */

static void layerCopy_propString(const void *source, void *dest, const int count) {
	memcpy(dest, source, sizeof(MStringProperty) * count);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Callbacks for (#MLoopCol, #CD_PROP_BYTE_COLOR)
 * \{ */

static void layerCopyValue_mloopcol(const void *source, void *dest, const int mixmode, const float mixfactor) {
	const MLoopCol *m1 = static_cast<const MLoopCol *>(source);
	MLoopCol *m2 = static_cast<MLoopCol *>(dest);
	unsigned char tmp_col[4];

	if (ELEM(mixmode, CDT_MIX_NOMIX, CDT_MIX_REPLACE_ABOVE_THRESHOLD, CDT_MIX_REPLACE_BELOW_THRESHOLD)) {
		/* Modes that do a full copy or nothing. */
		if (ELEM(mixmode, CDT_MIX_REPLACE_ABOVE_THRESHOLD, CDT_MIX_REPLACE_BELOW_THRESHOLD)) {
			/* TODO: Check for a real valid way to get 'factor' value of our dest color? */
			const float f = (float(m2->r) + float(m2->g) + float(m2->b)) / 3.0f;
			if (mixmode == CDT_MIX_REPLACE_ABOVE_THRESHOLD && f < mixfactor) {
				return; /* Do Nothing! */
			}
			if (mixmode == CDT_MIX_REPLACE_BELOW_THRESHOLD && f > mixfactor) {
				return; /* Do Nothing! */
			}
		}
		m2->r = m1->r;
		m2->g = m1->g;
		m2->b = m1->b;
		m2->a = m1->a;
	}
	else { /* Modes that support 'real' mix factor. */
		unsigned char src[4] = {m1->r, m1->g, m1->b, m1->a};
		unsigned char dst[4] = {m2->r, m2->g, m2->b, m2->a};

		if (mixmode == CDT_MIX_MIX) {
			blend_color_mix_byte(tmp_col, dst, src);
		}
		else if (mixmode == CDT_MIX_ADD) {
			blend_color_add_byte(tmp_col, dst, src);
		}
		else if (mixmode == CDT_MIX_SUB) {
			blend_color_sub_byte(tmp_col, dst, src);
		}
		else if (mixmode == CDT_MIX_MUL) {
			blend_color_mul_byte(tmp_col, dst, src);
		}
		else {
			memcpy(tmp_col, src, sizeof(tmp_col));
		}

		blend_color_interpolate_byte(dst, dst, tmp_col, mixfactor);

		m2->r = char(dst[0]);
		m2->g = char(dst[1]);
		m2->b = char(dst[2]);
		m2->a = char(dst[3]);
	}
}

static bool layerEqual_mloopcol(const void *data1, const void *data2) {
	const MLoopCol *m1 = static_cast<const MLoopCol *>(data1);
	const MLoopCol *m2 = static_cast<const MLoopCol *>(data2);
	float r, g, b, a;

	r = m1->r - m2->r;
	g = m1->g - m2->g;
	b = m1->b - m2->b;
	a = m1->a - m2->a;

	return r * r + g * g + b * b + a * a < 0.001f;
}

static void layerMultiply_mloopcol(void *data, const float fac) {
	MLoopCol *m = static_cast<MLoopCol *>(data);

	m->r = float(m->r) * fac;
	m->g = float(m->g) * fac;
	m->b = float(m->b) * fac;
	m->a = float(m->a) * fac;
}

static void layerAdd_mloopcol(void *data1, const void *data2) {
	MLoopCol *m = static_cast<MLoopCol *>(data1);
	const MLoopCol *m2 = static_cast<const MLoopCol *>(data2);

	m->r += m2->r;
	m->g += m2->g;
	m->b += m2->b;
	m->a += m2->a;
}

static void layerDoMinMax_mloopcol(const void *data, void *vmin, void *vmax) {
	const MLoopCol *m = static_cast<const MLoopCol *>(data);
	MLoopCol *min = static_cast<MLoopCol *>(vmin);
	MLoopCol *max = static_cast<MLoopCol *>(vmax);

	if (m->r < min->r) {
		min->r = m->r;
	}
	if (m->g < min->g) {
		min->g = m->g;
	}
	if (m->b < min->b) {
		min->b = m->b;
	}
	if (m->a < min->a) {
		min->a = m->a;
	}
	if (m->r > max->r) {
		max->r = m->r;
	}
	if (m->g > max->g) {
		max->g = m->g;
	}
	if (m->b > max->b) {
		max->b = m->b;
	}
	if (m->a > max->a) {
		max->a = m->a;
	}
}

static void layerInitMinMax_mloopcol(void *vmin, void *vmax) {
	MLoopCol *min = static_cast<MLoopCol *>(vmin);
	MLoopCol *max = static_cast<MLoopCol *>(vmax);

	min->r = 255;
	min->g = 255;
	min->b = 255;
	min->a = 255;

	max->r = 0;
	max->g = 0;
	max->b = 0;
	max->a = 0;
}

static void layerDefault_mloopcol(void *data, const int count) {
	MLoopCol default_mloopcol = {255, 255, 255, 255};
	MLoopCol *mlcol = (MLoopCol *)data;
	for (int i = 0; i < count; i++) {
		mlcol[i] = default_mloopcol;
	}
}

static void layerInterp_mloopcol(const void **sources, const float *weights, const float * /*sub_weights*/, int count, void *dest) {
	MLoopCol *mc = static_cast<MLoopCol *>(dest);
	struct {
		float a;
		float r;
		float g;
		float b;
	} col = {0};

	for (int i = 0; i < count; i++) {
		const float interp_weight = weights[i];
		const MLoopCol *src = static_cast<const MLoopCol *>(sources[i]);
		col.r += src->r * interp_weight;
		col.g += src->g * interp_weight;
		col.b += src->b * interp_weight;
		col.a += src->a * interp_weight;
	}

	/* Subdivide smooth or fractal can cause problems without clamping
	 * although weights should also not cause this situation */

	/* Also delay writing to the destination in case dest is in sources. */
	mc->r = round_fl_to_uchar_clamp(col.r);
	mc->g = round_fl_to_uchar_clamp(col.g);
	mc->b = round_fl_to_uchar_clamp(col.b);
	mc->a = round_fl_to_uchar_clamp(col.a);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Callbacks for (#MPropCol, #CD_PROP_COLOR)
 * \{ */

static void layerCopyValue_propcol(const void *source, void *dest, const int mixmode, const float mixfactor) {
	const MPropCol *m1 = static_cast<const MPropCol *>(source);
	MPropCol *m2 = static_cast<MPropCol *>(dest);
	float tmp_col[4];

	if (ELEM(mixmode, CDT_MIX_NOMIX, CDT_MIX_REPLACE_ABOVE_THRESHOLD, CDT_MIX_REPLACE_BELOW_THRESHOLD)) {
		/* Modes that do a full copy or nothing. */
		if (ELEM(mixmode, CDT_MIX_REPLACE_ABOVE_THRESHOLD, CDT_MIX_REPLACE_BELOW_THRESHOLD)) {
			/* TODO: Check for a real valid way to get 'factor' value of our dest color? */
			const float f = (m2->color[0] + m2->color[1] + m2->color[2]) / 3.0f;
			if (mixmode == CDT_MIX_REPLACE_ABOVE_THRESHOLD && f < mixfactor) {
				return; /* Do Nothing! */
			}
			if (mixmode == CDT_MIX_REPLACE_BELOW_THRESHOLD && f > mixfactor) {
				return; /* Do Nothing! */
			}
		}
		copy_v4_v4(m2->color, m1->color);
	}
	else { /* Modes that support 'real' mix factor. */
		if (mixmode == CDT_MIX_MIX) {
			blend_color_mix_float(tmp_col, m2->color, m1->color);
		}
		else if (mixmode == CDT_MIX_ADD) {
			blend_color_add_float(tmp_col, m2->color, m1->color);
		}
		else if (mixmode == CDT_MIX_SUB) {
			blend_color_sub_float(tmp_col, m2->color, m1->color);
		}
		else if (mixmode == CDT_MIX_MUL) {
			blend_color_mul_float(tmp_col, m2->color, m1->color);
		}
		else {
			memcpy(tmp_col, m1->color, sizeof(tmp_col));
		}
		blend_color_interpolate_float(m2->color, m2->color, tmp_col, mixfactor);
	}
}

static bool layerEqual_propcol(const void *data1, const void *data2) {
	const MPropCol *m1 = static_cast<const MPropCol *>(data1);
	const MPropCol *m2 = static_cast<const MPropCol *>(data2);
	float tot = 0;

	for (int i = 0; i < 4; i++) {
		float c = (m1->color[i] - m2->color[i]);
		tot += c * c;
	}

	return tot < 1e-3f;
}

static void layerMultiply_propcol(void *data, const float fac) {
	MPropCol *m = static_cast<MPropCol *>(data);
	mul_v4_fl(m->color, fac);
}

static void layerAdd_propcol(void *data1, const void *data2) {
	MPropCol *m = static_cast<MPropCol *>(data1);
	const MPropCol *m2 = static_cast<const MPropCol *>(data2);
	add_v4_v4(m->color, m2->color);
}

static void layerDoMinMax_propcol(const void *data, void *vmin, void *vmax) {
	const MPropCol *m = static_cast<const MPropCol *>(data);
	MPropCol *min = static_cast<MPropCol *>(vmin);
	MPropCol *max = static_cast<MPropCol *>(vmax);
	minmax_v4v4_v4(min->color, max->color, m->color);
}

static void layerInitMinMax_propcol(void *vmin, void *vmax) {
	MPropCol *min = static_cast<MPropCol *>(vmin);
	MPropCol *max = static_cast<MPropCol *>(vmax);

	copy_v4_fl(min->color, FLT_MAX);
	copy_v4_fl(max->color, FLT_MIN);
}

static void layerDefault_propcol(void *data, const int count) {
	/* Default to white, full alpha. */
	MPropCol default_propcol = {{1.0f, 1.0f, 1.0f, 1.0f}};
	MPropCol *pcol = (MPropCol *)data;
	for (int i = 0; i < count; i++) {
		copy_v4_v4(pcol[i].color, default_propcol.color);
	}
}

static void layerInterp_propcol(const void **sources, const float *weights, const float * /*sub_weights*/, int count, void *dest) {
	MPropCol *mc = static_cast<MPropCol *>(dest);
	float col[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	for (int i = 0; i < count; i++) {
		const float interp_weight = weights[i];
		const MPropCol *src = static_cast<const MPropCol *>(sources[i]);
		madd_v4_v4fl(col, src->color, interp_weight);
	}
	copy_v4_v4(mc->color, col);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Callbacks for (#vec3f, #CD_PROP_FLOAT3)
 * \{ */

static void layerInterp_propfloat3(const void **sources, const float *weights, const float * /*sub_weights*/, int count, void *dest) {
	vec3f result = {0.0f, 0.0f, 0.0f};
	for (int i = 0; i < count; i++) {
		const float interp_weight = weights[i];
		const vec3f *src = static_cast<const vec3f *>(sources[i]);
		madd_v3_v3fl(&result.x, &src->x, interp_weight);
	}
	copy_v3_v3((float *)dest, &result.x);
}

static void layerMultiply_propfloat3(void *data, const float fac) {
	vec3f *vec = static_cast<vec3f *>(data);
	vec->x *= fac;
	vec->y *= fac;
	vec->z *= fac;
}

static void layerAdd_propfloat3(void *data1, const void *data2) {
	vec3f *vec1 = static_cast<vec3f *>(data1);
	const vec3f *vec2 = static_cast<const vec3f *>(data2);
	vec1->x += vec2->x;
	vec1->y += vec2->y;
	vec1->z += vec2->z;
}

static bool layerValidate_propfloat3(void *data, const uint totitems, const bool do_fixes) {
	float *values = static_cast<float *>(data);
	bool has_errors = false;
	for (int i = 0; i < totitems * 3; i++) {
		if (!isfinite(values[i])) {
			if (do_fixes) {
				values[i] = 0.0f;
			}
			has_errors = true;
		}
	}
	return has_errors;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Callbacks for (#vec2f, #CD_PROP_FLOAT2)
 * \{ */

static void layerInterp_propfloat2(const void **sources, const float *weights, const float * /*sub_weights*/, int count, void *dest) {
	vec2f result = {0.0f, 0.0f};
	for (int i = 0; i < count; i++) {
		const float interp_weight = weights[i];
		const vec2f *src = static_cast<const vec2f *>(sources[i]);
		madd_v2_v2fl(&result.x, &src->x, interp_weight);
	}
	copy_v2_v2((float *)dest, &result.x);
}

static void layerMultiply_propfloat2(void *data, const float fac) {
	vec2f *vec = static_cast<vec2f *>(data);
	vec->x *= fac;
	vec->y *= fac;
}

static void layerAdd_propfloat2(void *data1, const void *data2) {
	vec2f *vec1 = static_cast<vec2f *>(data1);
	const vec2f *vec2 = static_cast<const vec2f *>(data2);
	vec1->x += vec2->x;
	vec1->y += vec2->y;
}

static bool layerValidate_propfloat2(void *data, const uint totitems, const bool do_fixes) {
	float *values = static_cast<float *>(data);
	bool has_errors = false;
	for (int i = 0; i < totitems * 2; i++) {
		if (!isfinite(values[i])) {
			if (do_fixes) {
				values[i] = 0.0f;
			}
			has_errors = true;
		}
	}
	return has_errors;
}

static bool layerEqual_propfloat2(const void *data1, const void *data2) {
	const float2 &a = *static_cast<const float2 *>(data1);
	const float2 &b = *static_cast<const float2 *>(data2);
	return rose::math::distance_squared(a, b) < 0.00001f;
}

static void layerInitMinMax_propfloat2(void *vmin, void *vmax) {
	float2 &min = *static_cast<float2 *>(vmin);
	float2 &max = *static_cast<float2 *>(vmax);
	INIT_MINMAX2(min, max);
}

static void layerDoMinMax_propfloat2(const void *data, void *vmin, void *vmax) {
	const float2 &value = *static_cast<const float2 *>(data);
	float2 &a = *static_cast<float2 *>(vmin);
	float2 &b = *static_cast<float2 *>(vmax);
	rose::math::min_max(value, a, b);
}

static void layerCopyValue_propfloat2(const void *source, void *dest, const int mixmode, const float mixfactor) {
	const float2 &a = *static_cast<const float2 *>(source);
	float2 &b = *static_cast<float2 *>(dest);

	/* We only support a limited subset of advanced mixing here-
	 * namely the mixfactor interpolation. */
	if (mixmode == CDT_MIX_NOMIX) {
		b = a;
	}
	else {
		b = rose::math::interpolate(b, a, mixfactor);
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Callbacks for (`bool`, #CD_PROP_BOOL)
 * \{ */

static void layerInterp_propbool(const void **sources, const float *weights, const float * /*sub_weights*/, int count, void *dest) {
	bool result = false;
	for (int i = 0; i < count; i++) {
		const float interp_weight = weights[i];
		const bool src = *(const bool *)sources[i];
		result |= src && (interp_weight > 0.0f);
	}
	*(bool *)dest = result;
}

/** \} */

static const LayerTypeInfo LAYERTYPEINFO[CD_NUMTYPES] = {
	/* UNKOWN = 0 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* UNKOWN = 1 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* CD_MDEFORMVERT = 2 */
	{sizeof(MDeformVert), "MDeformVert", 1, nullptr, layerCopy_mdeformvert, layerFree_mdeformvert, layerInterp_mdeformvert, nullptr, nullptr},
	/* UNKOWN = 3 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* UNKOWN = 4 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* UNKOWN = 5 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* UNKOWN = 6 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* UNKOWN = 7 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* CD_NORMAL = 8 */
	{sizeof(float[3]), "vec3f", 1, nullptr, nullptr, nullptr, layerInterp_normal, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, layerCopyValue_normal},
	/* UNKOWN = 9 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* CD_PROP_FLOAT = 10 */
	{sizeof(MFloatProperty), "MFloatProperty", 1, "Float", layerCopy_propFloat, nullptr, layerInterp_propFloat, nullptr, nullptr, nullptr, layerValidate_propFloat},
	/* CD_PROP_INT32 = 11 */
	{sizeof(MIntProperty), "MIntProperty", 1, "Int", nullptr, nullptr, layerInterp_propInt, nullptr},
	/* CD_PROP_STRING = 12 */
	{sizeof(MStringProperty), "MStringProperty", 1, "String", layerCopy_propString, nullptr, nullptr, nullptr},
	/* UNKOWN = 13 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* UNKOWN = 14 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* UNKOWN = 15 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* UNKOWN = 16 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* CD_PROP_BYTE_COLOR = 17 */
	{sizeof(MLoopCol), "MLoopCol", 1, "Col", nullptr, nullptr, layerInterp_mloopcol, nullptr, layerDefault_mloopcol, nullptr, nullptr, layerEqual_mloopcol, layerMultiply_mloopcol, layerInitMinMax_mloopcol, layerAdd_mloopcol, layerDoMinMax_mloopcol, layerCopyValue_mloopcol, nullptr},
	/* UNKOWN = 18 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* UNKOWN = 19 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* UNKOWN = 20 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* UNKOWN = 21 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* UNKOWN = 22 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* UNKOWN = 23 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* UNKOWN = 24 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* UNKOWN = 25 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* UNKOWN = 26 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* UNKOWN = 27 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* UNKOWN = 28 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* UNKOWN = 29 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* UNKOWN = 30 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* UNKOWN = 31 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* UNKOWN = 32 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* UNKOWN = 33 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* UNKOWN = 34 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* UNKOWN = 35 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* UNKOWN = 36 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* UNKOWN = 37 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* UNKOWN = 38 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* UNKOWN = 39 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* UNKOWN = 40 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* UNKOWN = 41 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* UNKOWN = 42 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* UNKOWN = 43 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* UNKOWN = 44 */
	{sizeof(int), "void", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* CD_PROP_INT8 = 45 */
	{sizeof(int8_t), "MInt8Property", 1, "Int8", nullptr, nullptr, nullptr, nullptr, nullptr},
	/* CD_PROP_INT16_2D = 46 */
	{sizeof(vec2s), "vec2s", 1, "2D 16-Bit Intege", nullptr, nullptr, nullptr, nullptr, nullptr},
	/* CD_PROP_INT32_2D = 47 */
	{sizeof(vec2i), "vec2i", 1, "Int 2D", nullptr, nullptr, nullptr, nullptr, nullptr},
	/* CD_PROP_COLOR = 48 */
	{sizeof(MPropCol), "MPropCol", 1, "Color", nullptr, nullptr, layerInterp_propcol, nullptr, layerDefault_propcol, nullptr, nullptr, layerEqual_propcol, layerMultiply_propcol, layerInitMinMax_propcol, layerAdd_propcol, layerDoMinMax_propcol, layerCopyValue_propcol, nullptr},
	/* CD_PROP_FLOAT3 = 49 */
	{sizeof(float[3]), "vec3f", 1, "Float3", nullptr, nullptr, layerInterp_propfloat3, nullptr, nullptr, nullptr, layerValidate_propfloat3, nullptr, layerMultiply_propfloat3, nullptr, layerAdd_propfloat3},
	/* CD_PROP_FLOAT2 = 50 */
	{sizeof(float[2]), "vec2f", 1, "Float2", nullptr, nullptr, layerInterp_propfloat2, nullptr, nullptr, nullptr, layerValidate_propfloat2, layerEqual_propfloat2, layerMultiply_propfloat2, layerInitMinMax_propfloat2, layerAdd_propfloat2, layerDoMinMax_propfloat2, layerCopyValue_propfloat2},
	/* CD_PROP_BOOL = 51 */
	{sizeof(bool), "bool", 1, "Boolean", nullptr, nullptr, layerInterp_propbool, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
	/* CD_HAIRLENGTH = 52 */
	{sizeof(float), "float", 1, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
};

static const char *LAYERTYPENAMES[CD_NUMTYPES] = {
	/* UNKOWN = 0 */
	"Unkown",
	/* UNKOWN = 1 */
	"Unkown",
	/* UNKOWN = 2 */
	"Unkown",
	/* UNKOWN = 3 */
	"Unkown",
	/* UNKOWN = 4 */
	"Unkown",
	/* UNKOWN = 5 */
	"Unkown",
	/* UNKOWN = 6 */
	"Unkown",
	/* UNKOWN = 7 */
	"Unkown",
	/* UNKOWN = 8 */
	"Unkown",
	/* UNKOWN = 9 */
	"Unkown",
	/* CD_PROP_FLOAT = 10 */
	"CDMFloatProperty",
	/* CD_PROP_STRING = 12 */
	"CDMStringProperty",
	/* UNKOWN = 13 */
	"Unkown",
	/* UNKOWN = 14 */
	"Unkown",
	/* UNKOWN = 15 */
	"Unkown",
	/* UNKOWN = 16 */
	"Unkown",
	/* CD_PROP_BYTE_COLOR = 17 */
	"CDMloopCol",
	/* UNKOWN = 18 */
	"Unkown",
	/* UNKOWN = 19 */
	"Unkown",
	/* UNKOWN = 20 */
	"Unkown",
	/* UNKOWN = 21 */
	"Unkown",
	/* UNKOWN = 22 */
	"Unkown",
	/* UNKOWN = 23 */
	"Unkown",
	/* UNKOWN = 24 */
	"Unkown",
	/* UNKOWN = 25 */
	"Unkown",
	/* UNKOWN = 26 */
	"Unkown",
	/* UNKOWN = 27 */
	"Unkown",
	/* UNKOWN = 28 */
	"Unkown",
	/* UNKOWN = 29 */
	"Unkown",
	/* UNKOWN = 30 */
	"Unkown",
	/* UNKOWN = 31 */
	"Unkown",
	/* UNKOWN = 32 */
	"Unkown",
	/* UNKOWN = 33 */
	"Unkown",
	/* UNKOWN = 34 */
	"Unkown",
	/* UNKOWN = 35 */
	"Unkown",
	/* UNKOWN = 36 */
	"Unkown",
	/* UNKOWN = 37 */
	"Unkown",
	/* UNKOWN = 38 */
	"Unkown",
	/* UNKOWN = 39 */
	"Unkown",
	/* UNKOWN = 40 */
	"Unkown",
	/* UNKOWN = 41 */
	"Unkown",
	/* UNKOWN = 42 */
	"Unkown",
	/* UNKOWN = 43 */
	"Unkown",
	/* UNKOWN = 44 */
	"Unkown",
	/* CD_PROP_INT8 = 45 */
	"CDHairMapping",
	/* CD_PROP_INT16_2D = 46 */
	"CDPropShort2",
	/* CD_PROP_INT32_2D = 47 */
	"CDPoint",
	/* CD_PROP_COLOR = 48 */
	"CDPropCol",
	/* CD_PROP_FLOAT3 = 49 */
	"CDPropFloat3",
	/* CD_PROP_FLOAT2 = 50 */
	"CDPropFloat2",
	/* CD_PROP_BOOL = 51 */
	"CDPropBoolean",
	/* CD_HAIRLENGTH = 52 */
	"CDHairLength",
};

const CustomData_MeshMasks CD_MASK_MESH = {
	/*vmask*/
	(CD_MASK_PROP_FLOAT3 | CD_MASK_PROP_ALL),
	/*emask*/
	(CD_MASK_PROP_ALL),
	/*fmask*/
	0,
	/*pmask*/
	(CD_MASK_PROP_ALL),
	/*lmask*/
	(CD_MASK_PROP_ALL),
};
const CustomData_MeshMasks CD_MASK_EVERYTHING = {
	/*vmask*/
	(CD_MASK_PROP_ALL),
	/*emask*/
	(CD_MASK_PROP_ALL),
	/*fmask*/
	(CD_MASK_NORMAL | CD_MASK_PROP_ALL),
	/*pmask*/
	(CD_MASK_PROP_ALL),
	/*lmask*/
	(CD_MASK_NORMAL | CD_MASK_PROP_ALL),
};

static const LayerTypeInfo *layerType_getInfo(const eCustomDataType type) {
	if (type < 0 || type >= CD_NUMTYPES) {
		return nullptr;
	}

	return &LAYERTYPEINFO[type];
}

static const char *layerType_getName(const eCustomDataType type) {
	if (type < 0 || type >= CD_NUMTYPES) {
		return nullptr;
	}

	return LAYERTYPENAMES[type];
}

void customData_mask_layers__print(const CustomData_MeshMasks *mask) {
	printf("verts mask=0x%" PRIx64 ":\n", mask->vmask);
	for (int i = 0; i < CD_NUMTYPES; i++) {
		if (mask->vmask & CD_TYPE_AS_MASK(i)) {
			printf("  %s\n", layerType_getName(eCustomDataType(i)));
		}
	}

	printf("edges mask=0x%" PRIx64 ":\n", mask->emask);
	for (int i = 0; i < CD_NUMTYPES; i++) {
		if (mask->emask & CD_TYPE_AS_MASK(i)) {
			printf("  %s\n", layerType_getName(eCustomDataType(i)));
		}
	}

	printf("faces mask=0x%" PRIx64 ":\n", mask->fmask);
	for (int i = 0; i < CD_NUMTYPES; i++) {
		if (mask->fmask & CD_TYPE_AS_MASK(i)) {
			printf("  %s\n", layerType_getName(eCustomDataType(i)));
		}
	}

	printf("loops mask=0x%" PRIx64 ":\n", mask->lmask);
	for (int i = 0; i < CD_NUMTYPES; i++) {
		if (mask->lmask & CD_TYPE_AS_MASK(i)) {
			printf("  %s\n", layerType_getName(eCustomDataType(i)));
		}
	}

	printf("polys mask=0x%" PRIx64 ":\n", mask->pmask);
	for (int i = 0; i < CD_NUMTYPES; i++) {
		if (mask->pmask & CD_TYPE_AS_MASK(i)) {
			printf("  %s\n", layerType_getName(eCustomDataType(i)));
		}
	}
}

/* -------------------------------------------------------------------- */
/** \name CustomData Functions
 * \{ */

static void customData_update_offsets(CustomData *data);

static CustomDataLayer *customData_add_layer__internal(CustomData *data, eCustomDataType type, std::optional<eCDAllocType> alloctype, void *layer_data_to_assign, const rose::ImplicitSharingInfo *sharing_info_to_assign, int totelem, rose::StringRef name);

void CustomData_update_typemap(CustomData *data) {
	int lasttype = -1;

	for (int i = 0; i < CD_NUMTYPES; i++) {
		data->typemap[i] = -1;
	}

	for (int i = 0; i < data->totlayer; i++) {
		const eCustomDataType type = eCustomDataType(data->layers[i].type);
		if (type != lasttype) {
			data->typemap[type] = i;
			lasttype = type;
		}
	}
}

#ifndef NDEBUG
static bool customdata_typemap_is_valid(const CustomData *data) {
	CustomData data_copy = *data;
	CustomData_update_typemap(&data_copy);
	return (memcmp(data->typemap, data_copy.typemap, sizeof(data->typemap)) == 0);
}
#else
static bool customdata_typemap_is_valid(const CustomData *data) {
	ROSE_assert_unreachable();
	return true;
}
#endif

static void *copy_layer_data(const eCustomDataType type, const void *data, const int totelem) {
	const LayerTypeInfo &type_info = *layerType_getInfo(type);
	if (type_info.copy) {
		void *new_data = MEM_mallocN((size_t)type_info.size * (size_t)totelem, __func__);
		type_info.copy(data, new_data, totelem);
		return new_data;
	}
	return MEM_dupallocN(data);
}

static void free_layer_data(const eCustomDataType type, const void *data, const int totelem) {
	const LayerTypeInfo &type_info = *layerType_getInfo(type);
	if (type_info.free) {
		type_info.free(const_cast<void *>(data), totelem, type_info.size);
	}
	MEM_freeN(const_cast<void *>(data));
}

static bool customdata_merge_internal(const CustomData *source, CustomData *dest, const eCustomDataMask mask, const std::optional<eCDAllocType> alloctype, const int totelem) {
	bool changed = false;

	int last_type = -1;
	int last_active = 0;
	int last_render = 0;
	int last_clone = 0;
	int last_mask = 0;
	int current_type_layer_count = 0;
	int max_current_type_layer_count = -1;

	for (int i = 0; i < source->totlayer; i++) {
		const CustomDataLayer &src_layer = source->layers[i];
		const eCustomDataType type = eCustomDataType(src_layer.type);
		const int src_layer_flag = src_layer.flag;

		if (type != last_type) {
			current_type_layer_count = 0;
			max_current_type_layer_count = CustomData_layertype_layers_max(type);
			last_active = src_layer.active;
			last_render = src_layer.active_rnd;
			last_clone = src_layer.active_clone;
			last_mask = src_layer.active_mask;
			last_type = type;
		}
		else {
			current_type_layer_count++;
		}

		if (src_layer_flag & CD_FLAG_NOCOPY) {
			/* Don't merge this layer because it's not supposed to leave the source data. */
			continue;
		}
		if (!(mask & CD_TYPE_AS_MASK(type))) {
			/* Don't merge this layer because it does not match the type mask. */
			continue;
		}
		if ((max_current_type_layer_count != -1) && (current_type_layer_count >= max_current_type_layer_count)) {
			/* Don't merge this layer because the maximum amount of layers of this type is reached.
			 */
			continue;
		}
		if (CustomData_get_named_layer_index(dest, type, src_layer.name) != -1) {
			/* Don't merge this layer because it exists in the destination already. */
			continue;
		}

		void *layer_data_to_assign = nullptr;
		const rose::ImplicitSharingInfo *sharing_info_to_assign = nullptr;
		if (!alloctype.has_value()) {
			if (src_layer.data != nullptr) {
				if (src_layer.sharing_info == nullptr) {
					/* Can't share the layer, duplicate it instead. */
					layer_data_to_assign = copy_layer_data(type, src_layer.data, totelem);
				}
				else {
					/* Share the layer. */
					layer_data_to_assign = src_layer.data;
					sharing_info_to_assign = src_layer.sharing_info;
				}
			}
		}

		CustomDataLayer *new_layer = customData_add_layer__internal(dest, type, alloctype, layer_data_to_assign, sharing_info_to_assign, totelem, src_layer.name);

		new_layer->uid = src_layer.uid;
		new_layer->flag |= src_layer_flag & (CD_FLAG_EXTERNAL | CD_FLAG_IN_MEMORY);
		new_layer->active = last_active;
		new_layer->active_rnd = last_render;
		new_layer->active_clone = last_clone;
		new_layer->active_mask = last_mask;
		changed = true;
	}

	CustomData_update_typemap(dest);
	return changed;
}

bool CustomData_merge(const CustomData *source, CustomData *dest, eCustomDataMask mask, int totelem) {
	return customdata_merge_internal(source, dest, mask, std::nullopt, totelem);
}

bool CustomData_merge_layout(const CustomData *source, CustomData *dest, const eCustomDataMask mask, const eCDAllocType alloctype, const int totelem) {
	return customdata_merge_internal(source, dest, mask, alloctype, totelem);
}

static bool RM_attribute_stored_in_rmesh_builtin(const char *name) {
#ifdef _DEBUG
	fprintf(stderr, "Incomplete feature #%s was called regarding customdata '%s'\n", __func__, name);
#endif
	return false;
}

CustomData CustomData_shallow_copy_remove_non_rmesh_attributes(const CustomData *src, const eCustomDataMask mask) {
	rose::Vector<CustomDataLayer> dst_layers;
	for (const CustomDataLayer &layer : rose::Span<CustomDataLayer>{src->layers, (size_t)src->totlayer}) {
		if (RM_attribute_stored_in_rmesh_builtin(layer.name)) {
			continue;
		}
		if (!(mask & CD_TYPE_AS_MASK(layer.type))) {
			continue;
		}
		dst_layers.append(layer);
	}

	CustomData dst = *src;
	dst.layers = static_cast<CustomDataLayer *>(MEM_callocN(sizeof(CustomDataLayer) * dst_layers.size(), __func__));
	dst.maxlayer = dst.totlayer = dst_layers.size();
	memcpy(dst.layers, dst_layers.data(), dst_layers.as_span().size_in_bytes());

	CustomData_update_typemap(&dst);

	return dst;
}

/**
 * An #ImplicitSharingInfo that knows how to free the entire referenced custom data layer
 * (including potentially separately allocated chunks like for vertex groups).
 */
class CustomDataLayerImplicitSharing : public rose::ImplicitSharingInfo {
private:
	const void *data_;
	int totelem_;
	const eCustomDataType type_;

public:
	CustomDataLayerImplicitSharing(const void *data, const int totelem, const eCustomDataType type) : ImplicitSharingInfo(), data_(data), totelem_(totelem), type_(type) {
	}

private:
	void delete_self_with_data() override {
		if (data_ != nullptr) {
			free_layer_data(type_, data_, totelem_);
		}
		MEM_delete(this);
	}

	void delete_data_only() override {
		free_layer_data(type_, data_, totelem_);
		data_ = nullptr;
		totelem_ = 0;
	}
};

/** Create a #ImplicitSharingInfo that takes ownership of the data. */
static const rose::ImplicitSharingInfo *make_implicit_sharing_info_for_layer(const eCustomDataType type, const void *data, const int totelem) {
	return MEM_new<CustomDataLayerImplicitSharing>(__func__, data, totelem, type);
}

/**
 * If the layer data is currently shared (hence it is immutable), create a copy that can be edited.
 */
static void ensure_layer_data_is_mutable(CustomDataLayer &layer, const int totelem) {
	if (layer.data == nullptr) {
		return;
	}
	if (layer.sharing_info == nullptr) {
		/* Can not be shared without implicit-sharing data. */
		return;
	}
	if (layer.sharing_info->is_mutable()) {
		layer.sharing_info->tag_ensured_mutable();
	}
	else {
		const eCustomDataType type = eCustomDataType(layer.type);
		const void *old_data = layer.data;
		/* Copy the layer before removing the user because otherwise the data might be freed while
		 * we're still copying from it here. */
		layer.data = copy_layer_data(type, old_data, totelem);
		layer.sharing_info->remove_user_and_delete_if_last();
		layer.sharing_info = make_implicit_sharing_info_for_layer(type, layer.data, totelem);
	}
}

void CustomData_realloc(CustomData *data, const int old_size, const int new_size) {
	ROSE_assert(new_size >= 0);
	for (int i = 0; i < data->totlayer; i++) {
		CustomDataLayer *layer = &data->layers[i];
		const LayerTypeInfo *typeInfo = layerType_getInfo(eCustomDataType(layer->type));
		const size_t old_size_in_bytes = size_t(old_size) * typeInfo->size;
		const size_t new_size_in_bytes = size_t(new_size) * typeInfo->size;

		void *new_layer_data = MEM_mallocN(new_size_in_bytes, __func__);
		/* Copy data to new array. */
		if (old_size_in_bytes) {
			if (typeInfo->copy) {
				typeInfo->copy(layer->data, new_layer_data, std::min(old_size, new_size));
			}
			else {
				ROSE_assert(layer->data != nullptr);
				memcpy(new_layer_data, layer->data, std::min(old_size_in_bytes, new_size_in_bytes));
			}
		}
		/* Remove ownership of old array */
		if (layer->sharing_info) {
			layer->sharing_info->remove_user_and_delete_if_last();
			layer->sharing_info = nullptr;
		}
		/* Take ownership of new array. */
		layer->data = new_layer_data;
		if (layer->data) {
			layer->sharing_info = make_implicit_sharing_info_for_layer(eCustomDataType(layer->type), layer->data, new_size);
		}

		if (new_size > old_size) {
			/* Initialize new values for non-trivial types. */
			if (typeInfo->construct) {
				const int new_elements_num = new_size - old_size;
				typeInfo->construct(POINTER_OFFSET(layer->data, old_size_in_bytes), new_elements_num);
			}
		}
	}
}

void CustomData_copy(const CustomData *source, CustomData *dest, eCustomDataMask mask, int totelem) {
	CustomData_reset(dest);

	if (source->external) {
		dest->external = static_cast<CustomDataExternal *>(MEM_dupallocN(source->external));
	}

	CustomData_merge(source, dest, mask, totelem);
}

void CustomData_copy_layout(const struct CustomData *source, struct CustomData *dest, eCustomDataMask mask, eCDAllocType alloctype, int totelem) {
	CustomData_reset(dest);

	if (source->external) {
		dest->external = static_cast<CustomDataExternal *>(MEM_dupallocN(source->external));
	}

	CustomData_merge_layout(source, dest, mask, alloctype, totelem);
}

static void customData_free_layer__internal(CustomDataLayer *layer, const int totelem) {
	const eCustomDataType type = eCustomDataType(layer->type);
	if (layer->sharing_info == nullptr) {
		if (layer->data) {
			free_layer_data(type, layer->data, totelem);
		}
	}
	else {
		layer->sharing_info->remove_user_and_delete_if_last();
		layer->sharing_info = nullptr;
	}
}

static void CustomData_external_free(CustomData *data) {
	if (data->external) {
		MEM_freeN(data->external);
		data->external = nullptr;
	}
}

void CustomData_reset(CustomData *data) {
	memset(data, 0, sizeof(*data));
	copy_vn_i(data->typemap, CD_NUMTYPES, -1);
}

void CustomData_free(CustomData *data, const int totelem) {
	for (int i = 0; i < data->totlayer; i++) {
		customData_free_layer__internal(&data->layers[i], totelem);
	}

	if (data->layers) {
		MEM_freeN(data->layers);
	}

	CustomData_external_free(data);
	CustomData_reset(data);
}

void CustomData_free_typemask(CustomData *data, const int totelem, eCustomDataMask mask) {
	for (int i = 0; i < data->totlayer; i++) {
		CustomDataLayer *layer = &data->layers[i];
		if (!(mask & CD_TYPE_AS_MASK(layer->type))) {
			continue;
		}
		customData_free_layer__internal(layer, totelem);
	}

	if (data->layers) {
		MEM_freeN(data->layers);
	}

	CustomData_external_free(data);
	CustomData_reset(data);
}

static void customData_update_offsets(CustomData *data) {
	const LayerTypeInfo *typeInfo;
	int offset = 0;

	for (int i = 0; i < data->totlayer; i++) {
		typeInfo = layerType_getInfo(eCustomDataType(data->layers[i].type));

		data->layers[i].offset = offset;
		offset += typeInfo->size;
	}

	data->totsize = offset;
	CustomData_update_typemap(data);
}

/* to use when we're in the middle of modifying layers */
static int CustomData_get_layer_index__notypemap(const CustomData *data, const eCustomDataType type) {
	for (int i = 0; i < data->totlayer; i++) {
		if (data->layers[i].type == type) {
			return i;
		}
	}

	return -1;
}

/* -------------------------------------------------------------------- */
/* index values to access the layers (offset from the layer start) */

int CustomData_get_layer_index(const CustomData *data, const eCustomDataType type) {
	ROSE_assert(customdata_typemap_is_valid(data));
	return data->typemap[type];
}

int CustomData_get_layer_index_n(const CustomData *data, const eCustomDataType type, const int n) {
	ROSE_assert(n >= 0);
	int i = CustomData_get_layer_index(data, type);

	if (i != -1) {
		ROSE_assert(i + n < data->totlayer);
		i = (data->layers[i + n].type == type) ? (i + n) : (-1);
	}

	return i;
}

int CustomData_get_named_layer_index(const CustomData *data, const eCustomDataType type, const char *name) {
	for (int i = 0; i < data->totlayer; i++) {
		if (data->layers[i].type == type) {
			if (STREQ(data->layers[i].name, name)) {
				return i;
			}
		}
	}

	return -1;
}

int CustomData_get_named_layer_index_notype(const CustomData *data, const char *name) {
	for (int i = 0; i < data->totlayer; i++) {
		if (STREQ(data->layers[i].name, name)) {
			return i;
		}
	}

	return -1;
}

int CustomData_get_active_layer_index(const CustomData *data, const eCustomDataType type) {
	const int layer_index = data->typemap[type];
	ROSE_assert(customdata_typemap_is_valid(data));
	return (layer_index != -1) ? layer_index + data->layers[layer_index].active : -1;
}

int CustomData_get_render_layer_index(const CustomData *data, const eCustomDataType type) {
	const int layer_index = data->typemap[type];
	ROSE_assert(customdata_typemap_is_valid(data));
	return (layer_index != -1) ? layer_index + data->layers[layer_index].active_rnd : -1;
}

int CustomData_get_clone_layer_index(const CustomData *data, const eCustomDataType type) {
	const int layer_index = data->typemap[type];
	ROSE_assert(customdata_typemap_is_valid(data));
	return (layer_index != -1) ? layer_index + data->layers[layer_index].active_clone : -1;
}

int CustomData_get_stencil_layer_index(const CustomData *data, const eCustomDataType type) {
	const int layer_index = data->typemap[type];
	ROSE_assert(customdata_typemap_is_valid(data));
	return (layer_index != -1) ? layer_index + data->layers[layer_index].active_mask : -1;
}

/* -------------------------------------------------------------------- */
/* index values per layer type */

int CustomData_get_named_layer(const CustomData *data, const eCustomDataType type, const char *name) {
	const int named_index = CustomData_get_named_layer_index(data, type, name);
	const int layer_index = data->typemap[type];
	ROSE_assert(customdata_typemap_is_valid(data));
	return (named_index != -1) ? named_index - layer_index : -1;
}

int CustomData_get_active_layer(const CustomData *data, const eCustomDataType type) {
	const int layer_index = data->typemap[type];
	ROSE_assert(customdata_typemap_is_valid(data));
	return (layer_index != -1) ? data->layers[layer_index].active : -1;
}

int CustomData_get_render_layer(const CustomData *data, const eCustomDataType type) {
	const int layer_index = data->typemap[type];
	ROSE_assert(customdata_typemap_is_valid(data));
	return (layer_index != -1) ? data->layers[layer_index].active_rnd : -1;
}

int CustomData_get_clone_layer(const CustomData *data, const eCustomDataType type) {
	const int layer_index = data->typemap[type];
	ROSE_assert(customdata_typemap_is_valid(data));
	return (layer_index != -1) ? data->layers[layer_index].active_clone : -1;
}

int CustomData_get_stencil_layer(const CustomData *data, const eCustomDataType type) {
	const int layer_index = data->typemap[type];
	ROSE_assert(customdata_typemap_is_valid(data));
	return (layer_index != -1) ? data->layers[layer_index].active_mask : -1;
}

const char *CustomData_get_active_layer_name(const CustomData *data, const eCustomDataType type) {
	/* Get the layer index of the active layer of this type. */
	const int layer_index = CustomData_get_active_layer_index(data, type);
	return layer_index < 0 ? nullptr : data->layers[layer_index].name;
}

const char *CustomData_get_render_layer_name(const CustomData *data, const eCustomDataType type) {
	const int layer_index = CustomData_get_render_layer_index(data, type);
	return layer_index < 0 ? nullptr : data->layers[layer_index].name;
}

void CustomData_set_layer_active(CustomData *data, const eCustomDataType type, const int n) {
#ifndef NDEBUG
	const int layer_num = CustomData_number_of_layers(data, type);
#endif
	for (int i = 0; i < data->totlayer; i++) {
		if (data->layers[i].type == type) {
			ROSE_assert(uint(n) < uint(layer_num));
			data->layers[i].active = n;
		}
	}
}

void CustomData_set_layer_render(CustomData *data, const eCustomDataType type, const int n) {
#ifndef NDEBUG
	const int layer_num = CustomData_number_of_layers(data, type);
#endif
	for (int i = 0; i < data->totlayer; i++) {
		if (data->layers[i].type == type) {
			ROSE_assert(uint(n) < uint(layer_num));
			data->layers[i].active_rnd = n;
		}
	}
}

void CustomData_set_layer_clone(CustomData *data, const eCustomDataType type, const int n) {
#ifndef NDEBUG
	const int layer_num = CustomData_number_of_layers(data, type);
#endif
	for (int i = 0; i < data->totlayer; i++) {
		if (data->layers[i].type == type) {
			ROSE_assert(uint(n) < uint(layer_num));
			data->layers[i].active_clone = n;
		}
	}
}

void CustomData_set_layer_stencil(CustomData *data, const eCustomDataType type, const int n) {
#ifndef NDEBUG
	const int layer_num = CustomData_number_of_layers(data, type);
#endif
	for (int i = 0; i < data->totlayer; i++) {
		if (data->layers[i].type == type) {
			ROSE_assert(uint(n) < uint(layer_num));
			data->layers[i].active_mask = n;
		}
	}
}

void CustomData_set_layer_active_index(CustomData *data, const eCustomDataType type, const int n) {
#ifndef NDEBUG
	const int layer_num = CustomData_number_of_layers(data, type);
#endif
	const int layer_index = n - data->typemap[type];
	ROSE_assert(customdata_typemap_is_valid(data));

	for (int i = 0; i < data->totlayer; i++) {
		if (data->layers[i].type == type) {
			ROSE_assert(uint(layer_index) < uint(layer_num));
			data->layers[i].active = layer_index;
		}
	}
}

void CustomData_set_layer_render_index(CustomData *data, const eCustomDataType type, const int n) {
#ifndef NDEBUG
	const int layer_num = CustomData_number_of_layers(data, type);
#endif
	const int layer_index = n - data->typemap[type];
	ROSE_assert(customdata_typemap_is_valid(data));

	for (int i = 0; i < data->totlayer; i++) {
		if (data->layers[i].type == type) {
			ROSE_assert(uint(layer_index) < uint(layer_num));
			data->layers[i].active_rnd = layer_index;
		}
	}
}

void CustomData_set_layer_clone_index(CustomData *data, const eCustomDataType type, const int n) {
#ifndef NDEBUG
	const int layer_num = CustomData_number_of_layers(data, type);
#endif
	const int layer_index = n - data->typemap[type];
	ROSE_assert(customdata_typemap_is_valid(data));

	for (int i = 0; i < data->totlayer; i++) {
		if (data->layers[i].type == type) {
			ROSE_assert(uint(layer_index) < uint(layer_num));
			data->layers[i].active_clone = layer_index;
		}
	}
}

void CustomData_set_layer_stencil_index(CustomData *data, const eCustomDataType type, const int n) {
#ifndef NDEBUG
	const int layer_num = CustomData_number_of_layers(data, type);
#endif
	const int layer_index = n - data->typemap[type];
	ROSE_assert(customdata_typemap_is_valid(data));

	for (int i = 0; i < data->totlayer; i++) {
		if (data->layers[i].type == type) {
			ROSE_assert(uint(layer_index) < uint(layer_num));
			data->layers[i].active_mask = layer_index;
		}
	}
}

void CustomData_set_layer_flag(CustomData *data, const eCustomDataType type, const int flag) {
	for (int i = 0; i < data->totlayer; i++) {
		if (data->layers[i].type == type) {
			data->layers[i].flag |= flag;
		}
	}
}

void CustomData_clear_layer_flag(CustomData *data, const eCustomDataType type, const int flag) {
	const int nflag = ~flag;

	for (int i = 0; i < data->totlayer; i++) {
		if (data->layers[i].type == type) {
			data->layers[i].flag &= nflag;
		}
	}
}

static void customData_resize(CustomData *data, const int grow_amount) {
	data->layers = static_cast<CustomDataLayer *>(MEM_reallocN(data->layers, (data->maxlayer + grow_amount) * sizeof(CustomDataLayer)));
	data->maxlayer += grow_amount;
}

static CustomDataLayer *customData_add_layer__internal(CustomData *data, const eCustomDataType type, const std::optional<eCDAllocType> alloctype, void *layer_data_to_assign, const rose::ImplicitSharingInfo *sharing_info_to_assign, const int totelem, rose::StringRef name) {
	const LayerTypeInfo &type_info = *layerType_getInfo(type);
	int flag = 0;

	/* Some layer types only support a single layer. */
	if (!type_info.defaultname && CustomData_has_layer(data, type)) {
		/* This function doesn't support dealing with existing layer data for these layer types when
		 * the layer already exists. */
		ROSE_assert(layer_data_to_assign == nullptr);
		return &data->layers[CustomData_get_layer_index(data, type)];
	}

	int index = data->totlayer;
	if (index >= data->maxlayer) {
		customData_resize(data, CUSTOMDATA_GROW);
	}

	data->totlayer++;

	/* Keep layers ordered by type. */
	for (; index > 0 && data->layers[index - 1].type > type; index--) {
		data->layers[index] = data->layers[index - 1];
	}

	CustomDataLayer &new_layer = data->layers[index];

	/* Clear remaining data on the layer. The original data on the layer has been moved to another
	 * index. Without this, it can happen that information from the previous layer at that index
	 * leaks into the new layer. */
	memset(&new_layer, 0, sizeof(CustomDataLayer));

	if (alloctype.has_value()) {
		switch (*alloctype) {
			case CD_SET_DEFAULT: {
				if (totelem > 0) {
					if (type_info.set_default_value) {
						new_layer.data = MEM_mallocN((size_t)totelem * (size_t)type_info.size, layerType_getName(type));
						type_info.set_default_value(new_layer.data, totelem);
					}
					else {
						new_layer.data = MEM_callocN((size_t)totelem * (size_t)type_info.size, layerType_getName(type));
					}
				}
				break;
			}
			case CD_CONSTRUCT: {
				if (totelem > 0) {
					new_layer.data = MEM_mallocN((size_t)totelem * (size_t)type_info.size, layerType_getName(type));
					if (type_info.construct) {
						type_info.construct(new_layer.data, totelem);
					}
				}
				break;
			}
		}
	}
	else {
		if (totelem == 0 && sharing_info_to_assign == nullptr) {
			MEM_SAFE_FREE(layer_data_to_assign);
		}
		else {
			new_layer.data = layer_data_to_assign;
			new_layer.sharing_info = sharing_info_to_assign;
			if (new_layer.sharing_info) {
				new_layer.sharing_info->add_user();
			}
		}
	}

	if (new_layer.data != nullptr && new_layer.sharing_info == nullptr) {
		/* Make layer data shareable. */
		new_layer.sharing_info = make_implicit_sharing_info_for_layer(type, new_layer.data, totelem);
	}

	new_layer.type = type;
	new_layer.flag = flag;

	/* Set default name if none exists. Note we only call DATA_()  once
	 * we know there is a default name, to avoid overhead of locale lookups
	 * in the depsgraph. */
	if (name.is_empty() && type_info.defaultname) {
		name = type_info.defaultname;
	}

	if (!name.is_empty()) {
		name.copy(new_layer.name);
		CustomData_set_layer_unique_name(data, index);
	}
	else {
		new_layer.name[0] = '\0';
	}

	if (index > 0 && data->layers[index - 1].type == type) {
		new_layer.active = data->layers[index - 1].active;
		new_layer.active_rnd = data->layers[index - 1].active_rnd;
		new_layer.active_clone = data->layers[index - 1].active_clone;
		new_layer.active_mask = data->layers[index - 1].active_mask;
	}
	else {
		new_layer.active = 0;
		new_layer.active_rnd = 0;
		new_layer.active_clone = 0;
		new_layer.active_mask = 0;
	}

	customData_update_offsets(data);

	return &data->layers[index];
}

void *CustomData_add_layer(CustomData *data, const eCustomDataType type, eCDAllocType alloctype, const int totelem) {
	const LayerTypeInfo *typeInfo = layerType_getInfo(type);

	CustomDataLayer *layer = customData_add_layer__internal(data, type, alloctype, nullptr, nullptr, totelem, typeInfo->defaultname);
	CustomData_update_typemap(data);

	if (layer) {
		return layer->data;
	}

	return nullptr;
}

const void *CustomData_add_layer_with_data(CustomData *data, eCustomDataType type, void *layer_data, int totelem, const ImplicitSharingInfoHandle *sharing_info) {
	const LayerTypeInfo *typeInfo = layerType_getInfo(type);

	CustomDataLayer *layer = customData_add_layer__internal(data, type, std::nullopt, layer_data, sharing_info, totelem, typeInfo->defaultname);
	CustomData_update_typemap(data);

	if (layer) {
		return layer->data;
	}

	return nullptr;
}

void *CustomData_add_layer_named(CustomData *data, eCustomDataType type, eCDAllocType alloctype, int totelem, const char *name) {
	CustomDataLayer *layer = customData_add_layer__internal(data, type, alloctype, nullptr, nullptr, totelem, name);
	CustomData_update_typemap(data);

	if (layer) {
		return layer->data;
	}
	return nullptr;
}

void *CustomData_add_layer_named(CustomData *data, eCustomDataType type, eCDAllocType alloctype, int totelem, const rose::StringRef name) {
	CustomDataLayer *layer = customData_add_layer__internal(data, type, alloctype, nullptr, nullptr, totelem, name);
	CustomData_update_typemap(data);

	if (layer) {
		return layer->data;
	}
	return nullptr;
}

const void *CustomData_add_layer_named_with_data(CustomData *data, eCustomDataType type, void *layer_data, int totelem, const char *name, const ImplicitSharingInfoHandle *sharing_info) {
	CustomDataLayer *layer = customData_add_layer__internal(data, type, std::nullopt, layer_data, sharing_info, totelem, name);
	CustomData_update_typemap(data);

	if (layer) {
		return layer->data;
	}
	return nullptr;
}

const void *CustomData_add_layer_named_with_data(CustomData *data, eCustomDataType type, void *layer_data, int totelem, const rose::StringRef name, const ImplicitSharingInfoHandle *sharing_info) {
	CustomDataLayer *layer = customData_add_layer__internal(data, type, std::nullopt, layer_data, sharing_info, totelem, name);
	CustomData_update_typemap(data);

	if (layer) {
		return layer->data;
	}
	return nullptr;
}

bool CustomData_has_layer_named(const CustomData *data, const eCustomDataType type, const char *name) {
	return CustomData_get_named_layer_index(data, type, name) != -1;
}

bool CustomData_free_layer(CustomData *data, const eCustomDataType type, const int totelem, const int index) {
	const int index_first = CustomData_get_layer_index(data, type);
	const int n = index - index_first;

	ROSE_assert(index >= index_first);
	if ((index_first == -1) || (n < 0)) {
		return false;
	}
	ROSE_assert(data->layers[index].type == type);

	customData_free_layer__internal(&data->layers[index], totelem);

	for (int i = index + 1; i < data->totlayer; i++) {
		data->layers[i - 1] = data->layers[i];
	}

	data->totlayer--;

	/* if layer was last of type in array, set new active layer */
	int i = CustomData_get_layer_index__notypemap(data, type);

	if (i != -1) {
		/* don't decrement zero index */
		const int index_nonzero = n ? n : 1;
		CustomDataLayer *layer;

		for (layer = &data->layers[i]; i < data->totlayer && layer->type == type; i++, layer++) {
			if (layer->active >= index_nonzero) {
				layer->active--;
			}
			if (layer->active_rnd >= index_nonzero) {
				layer->active_rnd--;
			}
			if (layer->active_clone >= index_nonzero) {
				layer->active_clone--;
			}
			if (layer->active_mask >= index_nonzero) {
				layer->active_mask--;
			}
		}
	}

	if (data->totlayer <= data->maxlayer - CUSTOMDATA_GROW) {
		customData_resize(data, -CUSTOMDATA_GROW);
	}

	customData_update_offsets(data);

	return true;
}

bool CustomData_free_layer_named(CustomData *data, const char *name, const int totelem) {
	for (const int i : rose::IndexRange(data->totlayer)) {
		const CustomDataLayer &layer = data->layers[i];
		if (rose::StringRef(layer.name) == name) {
			CustomData_free_layer(data, eCustomDataType(layer.type), totelem, i);
			return true;
		}
	}
	return false;
}

bool CustomData_free_layer_active(CustomData *data, const eCustomDataType type, const int totelem) {
	const int index = CustomData_get_active_layer_index(data, type);
	if (index == -1) {
		return false;
	}
	return CustomData_free_layer(data, type, totelem, index);
}

void CustomData_free_layers(CustomData *data, const eCustomDataType type, const int totelem) {
	const int index = CustomData_get_layer_index(data, type);
	while (CustomData_free_layer(data, type, totelem, index)) {
		/* pass */
	}
}

bool CustomData_has_layer(const CustomData *data, const eCustomDataType type) {
	return (CustomData_get_layer_index(data, type) != -1);
}

int CustomData_number_of_layers(const CustomData *data, const eCustomDataType type) {
	int number = 0;

	for (int i = 0; i < data->totlayer; i++) {
		if (data->layers[i].type == type) {
			number++;
		}
	}

	return number;
}

int CustomData_number_of_layers_typemask(const CustomData *data, const eCustomDataMask mask) {
	int number = 0;

	for (int i = 0; i < data->totlayer; i++) {
		if (mask & CD_TYPE_AS_MASK(data->layers[i].type)) {
			number++;
		}
	}

	return number;
}

void CustomData_free_temporary(CustomData *data, const int totelem) {
	int i, j;
	bool changed = false;
	for (i = 0, j = 0; i < data->totlayer; i++) {
		CustomDataLayer *layer = &data->layers[i];

		if (i != j) {
			data->layers[j] = data->layers[i];
		}

		if ((layer->flag & CD_FLAG_TEMPORARY) == CD_FLAG_TEMPORARY) {
			customData_free_layer__internal(layer, totelem);
			changed = true;
		}
		else {
			j++;
		}
	}

	data->totlayer = j;

	if (data->totlayer <= data->maxlayer - CUSTOMDATA_GROW) {
		customData_resize(data, -CUSTOMDATA_GROW);
		changed = true;
	}

	if (changed) {
		customData_update_offsets(data);
	}
}

static void CustomData_rmesh_set_default_n(CustomData *data, void **block, const int n) {
	const int offset = data->layers[n].offset;
	CustomData_data_set_default_value(eCustomDataType(data->layers[n].type), POINTER_OFFSET(*block, offset));
}

void CustomData_rmesh_set_default(CustomData *data, void **block) {
	if (*block == nullptr) {
		CustomData_rmesh_alloc_block(data, block);
	}

	for (int i = 0; i < data->totlayer; i++) {
		CustomData_rmesh_set_default_n(data, block, i);
	}
}

void CustomData_set_only_copy(const CustomData *data, const eCustomDataMask mask) {
	for (int i = 0; i < data->totlayer; i++) {
		if (!(mask & CD_TYPE_AS_MASK(data->layers[i].type))) {
			data->layers[i].flag |= CD_FLAG_NOCOPY;
		}
	}
}

void CustomData_copy_elements(const eCustomDataType type, void *src_data_ofs, void *dst_data_ofs, const int count) {
	const LayerTypeInfo *typeInfo = layerType_getInfo(type);

	if (typeInfo->copy) {
		typeInfo->copy(src_data_ofs, dst_data_ofs, count);
	}
	else {
		memcpy(dst_data_ofs, src_data_ofs, size_t(count) * typeInfo->size);
	}
}

void CustomData_rmesh_copy_data_exclude_by_type(const CustomData *source, CustomData *dest, void *src_block, void **dest_block, const eCustomDataMask mask_exclude) {
	/* Note that having a version of this function without a 'mask_exclude'
	 * would cause too much duplicate code, so add a check instead. */
	const bool no_mask = (mask_exclude == 0);

	if (*dest_block == nullptr) {
		CustomData_rmesh_alloc_block(dest, dest_block);
		if (*dest_block) {
			memset(*dest_block, 0, dest->totsize);
		}
	}

	/* copies a layer at a time */
	int dest_i = 0;
	for (int src_i = 0; src_i < source->totlayer; src_i++) {

		/* find the first dest layer with type >= the source type
		 * (this should work because layers are ordered by type)
		 */
		while (dest_i < dest->totlayer && dest->layers[dest_i].type < source->layers[src_i].type) {
			CustomData_rmesh_set_default_n(dest, dest_block, dest_i);
			dest_i++;
		}

		/* if there are no more dest layers, we're done */
		if (dest_i >= dest->totlayer) {
			return;
		}

		/* if we found a matching layer, copy the data */
		if (dest->layers[dest_i].type == source->layers[src_i].type && STREQ(dest->layers[dest_i].name, source->layers[src_i].name)) {
			if (no_mask || ((CD_TYPE_AS_MASK(dest->layers[dest_i].type) & mask_exclude) == 0)) {
				const void *src_data = POINTER_OFFSET(src_block, source->layers[src_i].offset);
				void *dest_data = POINTER_OFFSET(*dest_block, dest->layers[dest_i].offset);
				const LayerTypeInfo *typeInfo = layerType_getInfo(eCustomDataType(source->layers[src_i].type));
				if (typeInfo->copy) {
					typeInfo->copy(src_data, dest_data, 1);
				}
				else {
					memcpy(dest_data, src_data, typeInfo->size);
				}
			}

			/* if there are multiple source & dest layers of the same type,
			 * we don't want to copy all source layers to the same dest, so
			 * increment dest_i
			 */
			dest_i++;
		}
	}

	while (dest_i < dest->totlayer) {
		CustomData_rmesh_set_default_n(dest, dest_block, dest_i);
		dest_i++;
	}
}

void CustomData_rmesh_copy_data(const CustomData *source, CustomData *dest, void *src_block, void **dest_block) {
	CustomData_rmesh_copy_data_exclude_by_type(source, dest, src_block, dest_block, static_cast<eCustomDataMask>(0));
}

void CustomData_copy_data_layer(const CustomData *source, CustomData *dest, const int src_layer_index, const int dst_layer_index, const int src_index, const int dst_index, const int count) {
	const LayerTypeInfo *typeInfo;

	const void *src_data = source->layers[src_layer_index].data;
	void *dst_data = dest->layers[dst_layer_index].data;

	typeInfo = layerType_getInfo(eCustomDataType(source->layers[src_layer_index].type));

	const size_t src_offset = size_t(src_index) * typeInfo->size;
	const size_t dst_offset = size_t(dst_index) * typeInfo->size;

	if (!count || !src_data || !dst_data) {
		if (count && !(src_data == nullptr && dst_data == nullptr)) {
			fprintf(stderr, "[Kernel] null data for %s type (%p --> %p), skipping", layerType_getName(eCustomDataType(source->layers[src_layer_index].type)), (void *)src_data, (void *)dst_data);
		}
		return;
	}

	if (typeInfo->copy) {
		typeInfo->copy(POINTER_OFFSET(src_data, src_offset), POINTER_OFFSET(dst_data, dst_offset), count);
	}
	else {
		memcpy(POINTER_OFFSET(dst_data, dst_offset), POINTER_OFFSET(src_data, src_offset), size_t(count) * typeInfo->size);
	}
}

void CustomData_copy_data_named(const CustomData *source, CustomData *dest, const int source_index, const int dest_index, const int count) {
	/* copies a layer at a time */
	for (int src_i = 0; src_i < source->totlayer; src_i++) {

		int dest_i = CustomData_get_named_layer_index(dest, eCustomDataType(source->layers[src_i].type), source->layers[src_i].name);

		/* if we found a matching layer, copy the data */
		if (dest_i != -1) {
			CustomData_copy_data_layer(source, dest, src_i, dest_i, source_index, dest_index, count);
		}
	}
}

void CustomData_copy_data(const CustomData *source, CustomData *dest, const int source_index, const int dest_index, const int count) {
	/* copies a layer at a time */
	int dest_i = 0;
	for (int src_i = 0; src_i < source->totlayer; src_i++) {

		/* find the first dest layer with type >= the source type
		 * (this should work because layers are ordered by type)
		 */
		while (dest_i < dest->totlayer && dest->layers[dest_i].type < source->layers[src_i].type) {
			dest_i++;
		}

		/* if there are no more dest layers, we're done */
		if (dest_i >= dest->totlayer) {
			return;
		}

		/* if we found a matching layer, copy the data */
		if (dest->layers[dest_i].type == source->layers[src_i].type) {
			CustomData_copy_data_layer(source, dest, src_i, dest_i, source_index, dest_index, count);

			/* if there are multiple source & dest layers of the same type,
			 * we don't want to copy all source layers to the same dest, so
			 * increment dest_i
			 */
			dest_i++;
		}
	}
}

void CustomData_copy_layer_type_data(const CustomData *source, CustomData *destination, const eCustomDataType type, int source_index, int destination_index, int count) {
	const int source_layer_index = CustomData_get_layer_index(source, type);
	if (source_layer_index == -1) {
		return;
	}
	const int destinaiton_layer_index = CustomData_get_layer_index(destination, type);
	if (destinaiton_layer_index == -1) {
		return;
	}
	CustomData_copy_data_layer(source, destination, source_layer_index, destinaiton_layer_index, source_index, destination_index, count);
}

void CustomData_free_elem(CustomData *data, const int index, const int count) {
	for (int i = 0; i < data->totlayer; i++) {
		const LayerTypeInfo *typeInfo = layerType_getInfo(eCustomDataType(data->layers[i].type));

		if (typeInfo->free) {
			size_t offset = size_t(index) * typeInfo->size;

			typeInfo->free(POINTER_OFFSET(data->layers[i].data, offset), count, typeInfo->size);
		}
	}
}

#define SOURCE_BUF_SIZE 100

void CustomData_interp(const CustomData *source, CustomData *dest, const int *src_indices, const float *weights, const float *sub_weights, int count, int dest_index) {
	if (count <= 0) {
		return;
	}

	const void *source_buf[SOURCE_BUF_SIZE];
	const void **sources = source_buf;

	/* Slow fallback in case we're interpolating a ridiculous number of elements. */
	if (count > SOURCE_BUF_SIZE) {
		sources = static_cast<const void **>(MEM_mallocN(sizeof(*sources) * (size_t)count, __func__));
	}

	/* If no weights are given, generate default ones to produce an average result. */
	float default_weights_buf[SOURCE_BUF_SIZE];
	float *default_weights = nullptr;
	if (weights == nullptr) {
		default_weights = (count > SOURCE_BUF_SIZE) ? static_cast<float *>(MEM_mallocN(sizeof(*weights) * size_t(count), __func__)) : default_weights_buf;
		copy_vn_fl(default_weights, count, 1.0f / count);
		weights = default_weights;
	}

	/* interpolates a layer at a time */
	int dest_i = 0;
	for (int src_i = 0; src_i < source->totlayer; src_i++) {
		const LayerTypeInfo *typeInfo = layerType_getInfo(eCustomDataType(source->layers[src_i].type));
		if (!typeInfo->interp) {
			continue;
		}

		/* find the first dest layer with type >= the source type
		 * (this should work because layers are ordered by type)
		 */
		while (dest_i < dest->totlayer && dest->layers[dest_i].type < source->layers[src_i].type) {
			dest_i++;
		}

		/* if there are no more dest layers, we're done */
		if (dest_i >= dest->totlayer) {
			break;
		}

		/* if we found a matching layer, copy the data */
		if (dest->layers[dest_i].type == source->layers[src_i].type) {
			void *src_data = source->layers[src_i].data;

			for (int j = 0; j < count; j++) {
				sources[j] = POINTER_OFFSET(src_data, size_t(src_indices[j]) * typeInfo->size);
			}

			typeInfo->interp(sources, weights, sub_weights, count, POINTER_OFFSET(dest->layers[dest_i].data, size_t(dest_index) * typeInfo->size));

			/* if there are multiple source & dest layers of the same type,
			 * we don't want to copy all source layers to the same dest, so
			 * increment dest_i
			 */
			dest_i++;
		}
	}

	if (count > SOURCE_BUF_SIZE) {
		MEM_freeN((void *)sources);
	}
	if (!ELEM(default_weights, nullptr, default_weights_buf)) {
		MEM_freeN(default_weights);
	}
}

void CustomData_swap_corners(CustomData *data, const int index, const int *corner_indices) {
	for (int i = 0; i < data->totlayer; i++) {
		const LayerTypeInfo *typeInfo = layerType_getInfo(eCustomDataType(data->layers[i].type));

		if (typeInfo->swap) {
			const size_t offset = size_t(index) * typeInfo->size;

			typeInfo->swap(POINTER_OFFSET(data->layers[i].data, offset), corner_indices);
		}
	}
}

void CustomData_swap(CustomData *data, const int index_a, const int index_b) {
	char buff_static[256];

	if (index_a == index_b) {
		return;
	}

	for (int i = 0; i < data->totlayer; i++) {
		const LayerTypeInfo *typeInfo = layerType_getInfo(eCustomDataType(data->layers[i].type));
		const size_t size = typeInfo->size;
		const size_t offset_a = size * index_a;
		const size_t offset_b = size * index_b;

		void *buff = size <= sizeof(buff_static) ? buff_static : MEM_mallocN(size, __func__);
		memcpy(buff, POINTER_OFFSET(data->layers[i].data, offset_a), size);
		memcpy(POINTER_OFFSET(data->layers[i].data, offset_a), POINTER_OFFSET(data->layers[i].data, offset_b), size);
		memcpy(POINTER_OFFSET(data->layers[i].data, offset_b), buff, size);

		if (buff != buff_static) {
			MEM_freeN(buff);
		}
	}
}

const void *CustomData_get_layer(const CustomData *data, const eCustomDataType type) {
	int layer_index = CustomData_get_active_layer_index(data, type);
	if (layer_index == -1) {
		return nullptr;
	}

	return data->layers[layer_index].data;
}

void *CustomData_get_layer_for_write(CustomData *data, const eCustomDataType type, const int totelem) {
	const int layer_index = CustomData_get_active_layer_index(data, type);
	if (layer_index == -1) {
		return nullptr;
	}
	CustomDataLayer &layer = data->layers[layer_index];
	ensure_layer_data_is_mutable(layer, totelem);
	return layer.data;
}

const void *CustomData_get_layer_n(const CustomData *data, const eCustomDataType type, const int n) {
	int layer_index = CustomData_get_layer_index_n(data, type, n);
	if (layer_index == -1) {
		return nullptr;
	}
	return data->layers[layer_index].data;
}

const void *CustomData_get_layer_named(const CustomData *data, const eCustomDataType type, const char *name) {
	const int layer_index = CustomData_get_named_layer_index(data, type, name);
	if (layer_index == -1) {
		return nullptr;
	}
	return data->layers[layer_index].data;
}

void *CustomData_get_layer_named_for_write(CustomData *data, const eCustomDataType type, const char *name, const int totelem) {
	const int layer_index = CustomData_get_named_layer_index(data, type, name);
	if (layer_index == -1) {
		return nullptr;
	}
	CustomDataLayer &layer = data->layers[layer_index];
	ensure_layer_data_is_mutable(layer, totelem);
	return layer.data;
}

int CustomData_get_offset(const CustomData *data, const eCustomDataType type) {
	int layer_index = CustomData_get_active_layer_index(data, type);
	if (layer_index == -1) {
		return -1;
	}
	return data->layers[layer_index].offset;
}

int CustomData_get_offset_named(const CustomData *data, const eCustomDataType type, const char *name) {
	int layer_index = CustomData_get_named_layer_index(data, type, name);
	if (layer_index == -1) {
		return -1;
	}

	return data->layers[layer_index].offset;
}

bool CustomData_set_layer_name(CustomData *data, const eCustomDataType type, const int n, const char *name) {
	const int layer_index = CustomData_get_layer_index_n(data, type, n);

	if ((layer_index == -1) || !name) {
		return false;
	}

	LIB_strcpy(data->layers[layer_index].name, sizeof(data->layers[layer_index].name), name);

	return true;
}

const char *CustomData_get_layer_name(const CustomData *data, const eCustomDataType type, const int n) {
	const int layer_index = CustomData_get_layer_index_n(data, type, n);

	return (layer_index == -1) ? nullptr : data->layers[layer_index].name;
}

void CustomData_rmesh_free_block(CustomData *data, void **block) {
	if (*block == nullptr) {
		return;
	}

	for (int i = 0; i < data->totlayer; i++) {
		const LayerTypeInfo *typeInfo = layerType_getInfo(eCustomDataType(data->layers[i].type));

		if (typeInfo->free) {
			int offset = data->layers[i].offset;
			typeInfo->free(POINTER_OFFSET(*block, offset), 1, typeInfo->size);
		}
	}

	if (data->totsize) {
		LIB_memory_pool_free(data->pool, *block);
	}

	*block = nullptr;
}

void CustomData_rmesh_free_block_data_exclude_by_type(CustomData *data, void *block, const eCustomDataMask mask_exclude) {
	if (block == nullptr) {
		return;
	}
	for (int i = 0; i < data->totlayer; i++) {
		if ((CD_TYPE_AS_MASK(data->layers[i].type) & mask_exclude) == 0) {
			const LayerTypeInfo *typeInfo = layerType_getInfo(eCustomDataType(data->layers[i].type));
			const size_t offset = data->layers[i].offset;
			if (typeInfo->free) {
				typeInfo->free(POINTER_OFFSET(block, offset), 1, typeInfo->size);
			}
			memset(POINTER_OFFSET(block, offset), 0, typeInfo->size);
		}
	}
}

void CustomData_rmesh_free_block_data(CustomData *data, void *block) {
	if (block == nullptr) {
		return;
	}
	for (int i = 0; i < data->totlayer; i++) {
		const LayerTypeInfo *typeInfo = layerType_getInfo(eCustomDataType(data->layers[i].type));
		if (typeInfo->free) {
			const size_t offset = data->layers[i].offset;
			typeInfo->free(POINTER_OFFSET(block, offset), 1, typeInfo->size);
		}
	}
	if (data->totsize) {
		memset(block, 0, data->totsize);
	}
}

void CustomData_rmesh_alloc_block(CustomData *data, void **block) {
	if (*block) {
		CustomData_rmesh_free_block(data, block);
	}

	if (data->totsize > 0) {
		*block = LIB_memory_pool_malloc(data->pool);
	}
	else {
		*block = nullptr;
	}
}

void CustomData_data_set_default_value(const eCustomDataType type, void *elem) {
	const LayerTypeInfo *typeInfo = layerType_getInfo(type);
	if (typeInfo->set_default_value) {
		typeInfo->set_default_value(elem, 1);
	}
	else {
		memset(elem, 0, typeInfo->size);
	}
}

bool CustomData_layer_has_math(const CustomData *data, const int layer_n) {
	const LayerTypeInfo *typeInfo = layerType_getInfo(eCustomDataType(data->layers[layer_n].type));

	if (typeInfo->equal && typeInfo->add && typeInfo->multiply && typeInfo->initminmax && typeInfo->dominmax) {
		return true;
	}

	return false;
}

bool CustomData_layer_has_interp(const CustomData *data, const int layer_n) {
	const LayerTypeInfo *typeInfo = layerType_getInfo(eCustomDataType(data->layers[layer_n].type));

	if (typeInfo->interp) {
		return true;
	}

	return false;
}

bool CustomData_has_math(const CustomData *data) {
	/* interpolates a layer at a time */
	for (int i = 0; i < data->totlayer; i++) {
		if (CustomData_layer_has_math(data, i)) {
			return true;
		}
	}

	return false;
}

bool CustomData_rmesh_has_free(const CustomData *data) {
	for (int i = 0; i < data->totlayer; i++) {
		const LayerTypeInfo *typeInfo = layerType_getInfo(eCustomDataType(data->layers[i].type));
		if (typeInfo->free) {
			return true;
		}
	}
	return false;
}

bool CustomData_has_interp(const CustomData *data) {
	/* interpolates a layer at a time */
	for (int i = 0; i < data->totlayer; i++) {
		if (CustomData_layer_has_interp(data, i)) {
			return true;
		}
	}

	return false;
}

void CustomData_data_copy_value(const eCustomDataType type, const void *source, void *dest) {
	const LayerTypeInfo *typeInfo = layerType_getInfo(type);

	if (!dest) {
		return;
	}

	if (typeInfo->copy) {
		typeInfo->copy(source, dest, 1);
	}
	else {
		memcpy(dest, source, typeInfo->size);
	}
}

void CustomData_data_mix_value(const eCustomDataType type, const void *source, void *dest, const int mixmode, const float mixfactor) {
	const LayerTypeInfo *typeInfo = layerType_getInfo(type);

	if (!dest) {
		return;
	}

	if (typeInfo->copyvalue) {
		typeInfo->copyvalue(source, dest, mixmode, mixfactor);
	}
	else {
		/* Mere copy if no advanced interpolation is supported. */
		memcpy(dest, source, typeInfo->size);
	}
}

bool CustomData_data_equals(const eCustomDataType type, const void *data1, const void *data2) {
	const LayerTypeInfo *typeInfo = layerType_getInfo(type);

	if (typeInfo->equal) {
		return typeInfo->equal(data1, data2);
	}

	return !memcmp(data1, data2, typeInfo->size);
}

void CustomData_data_initminmax(const eCustomDataType type, void *min, void *max) {
	const LayerTypeInfo *typeInfo = layerType_getInfo(type);

	if (typeInfo->initminmax) {
		typeInfo->initminmax(min, max);
	}
}

void CustomData_data_dominmax(const eCustomDataType type, const void *data, void *min, void *max) {
	const LayerTypeInfo *typeInfo = layerType_getInfo(type);

	if (typeInfo->dominmax) {
		typeInfo->dominmax(data, min, max);
	}
}

void CustomData_data_multiply(const eCustomDataType type, void *data, const float fac) {
	const LayerTypeInfo *typeInfo = layerType_getInfo(type);

	if (typeInfo->multiply) {
		typeInfo->multiply(data, fac);
	}
}

void CustomData_data_add(const eCustomDataType type, void *data1, const void *data2) {
	const LayerTypeInfo *typeInfo = layerType_getInfo(type);

	if (typeInfo->add) {
		typeInfo->add(data1, data2);
	}
}

int CustomData_sizeof(const eCustomDataType type) {
	const LayerTypeInfo *typeInfo = layerType_getInfo(type);

	return typeInfo->size;
}

const char *CustomData_layertype_name(const eCustomDataType type) {
	return layerType_getName(type);
}

bool CustomData_layertype_is_singleton(const eCustomDataType type) {
	const LayerTypeInfo *typeInfo = layerType_getInfo(type);
	return typeInfo->defaultname == nullptr;
}

bool CustomData_layertype_is_dynamic(const eCustomDataType type) {
	const LayerTypeInfo *typeInfo = layerType_getInfo(type);

	return (typeInfo->free != nullptr);
}

int CustomData_layertype_layers_max(const eCustomDataType type) {
	const LayerTypeInfo *typeInfo = layerType_getInfo(type);

	/* Same test as for singleton above. */
	if (typeInfo->defaultname == nullptr) {
		return 1;
	}
	if (typeInfo->layers_max == nullptr) {
		return -1;
	}

	return typeInfo->layers_max();
}

static bool cd_layer_find_dupe(CustomData *data, const char *name, const eCustomDataType type, const int index) {
	/* see if there is a duplicate */
	for (int i = 0; i < data->totlayer; i++) {
		if (i != index) {
			CustomDataLayer *layer = &data->layers[i];

			if (CD_TYPE_AS_MASK(type) & CD_MASK_PROP_ALL) {
				if ((CD_TYPE_AS_MASK(layer->type) & CD_MASK_PROP_ALL) && STREQ(layer->name, name)) {
					return true;
				}
			}
			else {
				if (i != index && layer->type == type && STREQ(layer->name, name)) {
					return true;
				}
			}
		}
	}

	return false;
}

struct CustomDataUniqueCheckData {
	CustomData *data;
	eCustomDataType type;
	int index;
};

static bool customdata_unique_check(void *arg, const char *name) {
	CustomDataUniqueCheckData *data_arg = static_cast<CustomDataUniqueCheckData *>(arg);
	return cd_layer_find_dupe(data_arg->data, name, data_arg->type, data_arg->index);
}

int CustomData_name_max_length_calc(const rose::StringRef name) {
	if (name.startswith(".")) {
		return MAX_CUSTOMDATA_LAYER_NAME_NO_PREFIX;
	}
	for (const rose::StringRef prefix : {"." UV_VERTSEL_NAME, UV_EDGESEL_NAME ".", UV_PINNED_NAME "."}) {
		if (name.startswith(prefix)) {
			return MAX_CUSTOMDATA_LAYER_NAME;
		}
	}
	return MAX_CUSTOMDATA_LAYER_NAME_NO_PREFIX;
}

void CustomData_set_layer_unique_name(CustomData *data, const int index) {
	CustomDataLayer *nlayer = &data->layers[index];
	const LayerTypeInfo *typeInfo = layerType_getInfo(eCustomDataType(nlayer->type));

	CustomDataUniqueCheckData data_arg{data, eCustomDataType(nlayer->type), index};

	if (!typeInfo->defaultname) {
		return;
	}

	const int max_length = CustomData_name_max_length_calc(nlayer->name);

	/* Set default name if none specified. Note we only call DATA_() when
	 * needed to avoid overhead of locale lookups in the depsgraph. */
	if (nlayer->name[0] == '\0') {
		LIB_strcpy(nlayer->name, sizeof(nlayer->name), typeInfo->defaultname);
	}

	const char *defname = ""; /* Dummy argument, never used as `name` is never zero length. */
	LIB_uniquename_cb(customdata_unique_check, &data_arg, defname, '.', nlayer->name, max_length);
}

void CustomData_validate_layer_name(const CustomData *data, const eCustomDataType type, const char *name, char *outname) {
	int index = -1;

	/* if a layer name was given, try to find that layer */
	if (name[0]) {
		index = CustomData_get_named_layer_index(data, type, name);
	}

	if (index == -1) {
		/* either no layer was specified, or the layer we want has been
		 * deleted, so assign the active layer to name
		 */
		index = CustomData_get_active_layer_index(data, type);
		LIB_strcpy(outname, MAX_CUSTOMDATA_LAYER_NAME, data->layers[index].name);
	}
	else {
		LIB_strcpy(outname, MAX_CUSTOMDATA_LAYER_NAME, name);
	}
}

static bool CustomData_layer_ensure_data_exists(CustomDataLayer *layer, size_t count) {
	ROSE_assert(layer);
	const LayerTypeInfo *typeInfo = layerType_getInfo(eCustomDataType(layer->type));
	ROSE_assert(typeInfo);

	if (layer->data || count == 0) {
		return false;
	}

	switch (layer->type) {
		/* When more instances of corrupt files are found, add them here. */
		case CD_PROP_BOOL:
		case CD_PROP_FLOAT2:
			layer->data = MEM_callocN((size_t)typeInfo->size * (size_t)count, layerType_getName(eCustomDataType(layer->type)));
			ROSE_assert(layer->data);
			if (typeInfo->set_default_value) {
				typeInfo->set_default_value(layer->data, count);
			}
			return true;
			break;

		default:
			/* Log an error so we can collect instances of bad files. */
			fprintf(stderr, "[Kernel] CustomDataLayer::data is NULL for type %d.", layer->type);
			break;
	}
	return false;
}

bool CustomData_layer_validate(CustomDataLayer *layer, const uint totitems, const bool do_fixes) {
	ROSE_assert(layer);
	const LayerTypeInfo *typeInfo = layerType_getInfo(eCustomDataType(layer->type));
	ROSE_assert(typeInfo);

	if (do_fixes) {
		CustomData_layer_ensure_data_exists(layer, totitems);
	}

	ROSE_assert((totitems == 0) || layer->data);
	ROSE_assert(MEM_allocN_length(layer->data) >= totitems * typeInfo->size);

	if (typeInfo->validate != nullptr) {
		return typeInfo->validate(layer->data, totitems, do_fixes);
	}

	return false;
}

/** \} */

size_t CustomData_get_elem_size(const CustomDataLayer *layer) {
	return LAYERTYPEINFO[layer->type].size;
}
