#include "MEM_guardedalloc.h"

#include "DNA_screen_types.h"

#include "UI_interface.h"

#include "LIB_array.hh"

#include "interface_intern.h"

/* -------------------------------------------------------------------- */
/** \name Data Types
 * \{ */

struct uiLayoutRoot {
	struct uiLayoutRoot *prev, *next;

	int type;
	int padding;

	struct uiBlock *block;
	struct uiLayout *layout;
};

struct uiItem {
	struct uiItem *prev, *next;

	int type;
	int flag;
};

/** #uiItem->flag */
enum {
	/** The item is "inside" a box item. */
	UI_ITEM_BOX_ITEM = 1 << 0,
	UI_ITEM_FIXED_SIZE = 1 << 1,
};

struct uiButtonItem : public uiItem {
	uiBut *but;
};

struct uiLayout : public uiItem {
	struct uiLayoutRoot *root;
	struct uiLayout *parent;

	char heading[64];

	int x, y;
	int w, h;

	float scale[2];
	float units[2];

	int space;

	ListBase items;
};

struct uiLayoutItemGridFlow : uiLayout {
	/**
	 * When positive, absolute fixed number of columns.
	 * When zero, fully automatic number of columns (based on available width).
	 * When negative, automatic number of columns (multiple of given absolute value).
	 */
	int columns;
	
	int totitems;
	int totrow;
	int totcol;
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name UI Layout
 * \{ */

ROSE_INLINE void ui_layout_add_padding_button(uiLayoutRoot *root) {
	if (root->padding) {
		uiBlock *block = root->block;
		uiLayout *prev_layout = block->layout;

		block->layout = root->layout;
		uiDefBut(block, UI_BTYPE_SEPR, "", root->padding, root->padding, NULL, UI_POINTER_NIL, 0, 0);
		block->layout = prev_layout;
	}
}

uiLayout *UI_block_layout(uiBlock *block, int dir, int type, int x, int y, int size, int padding) {
	uiLayoutRoot *root = static_cast<uiLayoutRoot *>(MEM_callocN(sizeof(uiLayoutRoot), "uiLayoutRoot"));
	root->type = type;
	root->block = block;
	root->padding = padding;
	do {
		uiLayout *layout = static_cast<uiLayout *>(MEM_callocN(sizeof(uiLayout), "uiLayout"));
		layout->type = ITEM_LAYOUT_ROOT;
		layout->flag = 0;
		layout->x = x;
		layout->y = y;
		layout->root = root;
		layout->parent = NULL;
		layout->space = 0;

		if (dir == UI_LAYOUT_HORIZONTAL) {
			layout->w = 0;
			layout->h = size;
		}
		else {
			layout->w = size;
			layout->h = 0;
		}

		LIB_listbase_clear(&layout->items);
		root->layout = layout;
	} while (false);
	block->layout = root->layout;
	LIB_addtail(&block->layouts, root);
	/** called after we set the layout for both root and block! */
	ui_layout_add_padding_button(root);
	return root->layout;
}

void UI_block_layout_set_current(uiBlock *block, uiLayout *layout) {
	block->layout = layout;
}

ROSE_INLINE void ui_item_init_from_parent(uiLayout *layout, uiLayout *parent) {
	layout->root = parent->root;
	layout->parent = parent;
	LIB_addtail(&parent->items, layout);
}

uiLayout *UI_layout_row(uiLayout *parent, int space) {
	uiLayout *layout = MEM_cnew<uiLayout>("uiLayout");
	ui_item_init_from_parent(layout, parent);
	layout->type = ITEM_LAYOUT_ROW;
	layout->space = space;
	UI_block_layout_set_current(layout->root->block, layout);
	return layout;
}

uiLayout *UI_layout_col(uiLayout *parent, int space) {
	uiLayout *layout = MEM_cnew<uiLayout>("uiLayout");
	ui_item_init_from_parent(layout, parent);
	layout->type = ITEM_LAYOUT_COL;
	layout->space = space;
	UI_block_layout_set_current(layout->root->block, layout);
	return layout;
}

uiLayout *UI_layout_grid(uiLayout *parent, int columns) {
	uiLayoutItemGridFlow *layout = MEM_cnew<uiLayoutItemGridFlow>("uiLayoutItemGridFlow");
	ui_item_init_from_parent(layout, parent);
	
	layout->type = ITEM_LAYOUT_GRID;
	layout->columns = columns;
	
	UI_block_layout_set_current(layout->root->block, layout);
	return layout;
}

void UI_block_layout_free(uiBlock *block) {
	LISTBASE_FOREACH_MUTABLE(uiLayoutRoot *, root, &block->layouts) {
		ui_layout_free(root->layout);
		MEM_delete(root);
	}
}

ROSE_INLINE void ui_item_size(const uiItem *vitem, int *r_w, int *r_h) {
	if (vitem->type == ITEM_BUTTON) {
		const uiButtonItem *item = static_cast<const uiButtonItem *>(vitem);
		if (r_w) {
			*r_w = LIB_rctf_size_x(&item->but->rect);
		}
		if (r_h) {
			*r_h = LIB_rctf_size_y(&item->but->rect);
		}
	}
	else {
		const uiLayout *item = static_cast<const uiLayout *>(vitem);
		if (r_w) {
			*r_w = item->w;
		}
		if (r_h) {
			*r_h = item->h;
		}
	}
}

ROSE_INLINE void ui_item_offset(const uiItem *vitem, int *r_x, int *r_y) {
	if (vitem->type == ITEM_BUTTON) {
		const uiButtonItem *item = static_cast<const uiButtonItem *>(vitem);
		if (r_x) {
			*r_x = item->but->rect.xmin;
		}
		if (r_y) {
			*r_y = item->but->rect.ymin;
		}
	}
	else {
		const uiLayout *item = static_cast<const uiLayout *>(vitem);
		if (r_x) {
			*r_x = 0;
		}
		if (r_y) {
			*r_y = 0;
		}
	}
}

struct UILayoutGridFlowInput {
	int litem_w;
	int litem_x;
	int litem_y;
	int space_x;
	int space_y;
	
	int totcol;
	int totrow;
};

struct UILayoutGridFlowOutput {
	int *totitem; // The number of items in the grid layout.
	float *maxitem;	 // The maximum height of a single item in the grid layout.
	float *avgitem; // The average width of each column.
	
	int *x_array; // The x coordinate of each column.
	int *w_array; // The width of each column.
	int *w; // Total width.
	
	int *y_array;
	int *h_array;
	int *h; // Total height;
};

ROSE_INLINE void ui_layout_grid_flow_compute(ListBase *lb, const UILayoutGridFlowInput *in, UILayoutGridFlowOutput *out) {
	float tot_w = 0.0f;
	float tot_h = 0.0f;
	float val_avg_w = 0.0f;
	float val_tot_w = 0.0f;
	float val_max_h = 0.0f;
	
	ROSE_assert(in->totcol != 0 || (out->x_array == NULL && out->w_array == NULL));
	ROSE_assert(in->totrow != 0 || (out->y_array == NULL && out->h_array == NULL));
	
	if (out->totitem) {
		*out->totitem = 0;
	}
	
	if (LIB_listbase_is_empty(lb)) {
		if (out->avgitem) {
			*out->avgitem = 0.0f;
		}
		if (out->maxitem) {
			*out->maxitem = 0.0f;
		}
		return;
	}
	
	rose::Array<float, 64> arr_avg_w(in->totcol, 0.0f);
	rose::Array<float, 64> arr_tot_w(in->totcol, 0.0f);
	rose::Array<int, 64> arr_max_h(in->totrow, 0.0f);
	
	int i;
	LISTBASE_FOREACH_INDEX(uiItem *, vitem, lb, i) {
		int w, h;
		ui_item_size(vitem, &w, &h);
		
		val_avg_w += w * w;
		val_tot_w += w;
		val_max_h = ROSE_MAX(val_max_h, h);
		
		if (in->totrow != 0 && in->totcol != 0) {
			const int r = i / in->totcol;
			const int c = i % in->totcol;
			
			arr_avg_w[c] += w * w;
			arr_tot_w[c] += w;
			arr_max_h[r] = ROSE_MAX(arr_max_h[r], h);
		}
		
		if (out->totitem) {
			(*out->totitem)++;
		}
	}
	
	val_avg_w /= val_tot_w;
	if (in->totcol != 0) {
		for (int i = 0; i < in->totcol; i++) {
			arr_avg_w[i] /= arr_tot_w[i];
			tot_w += arr_avg_w[i];
		}
		if (false /** even_columns */) {
			tot_w = ceilf(val_avg_w) * in->totcol;
		}
	}
	if (in->totrow != 0) {
		for (int i = 0; i < in->totrow; i++) {
			tot_h += arr_max_h[i];
		}
		if (true /** even_rows */) {
			tot_h = val_max_h * in->totrow;
		}
	}
	
	if (out->x_array != NULL && out->w_array != NULL) {
		/** We enlarge/narrow columns evenly to match available width. */
		const float wfac = float(in->litem_w - (in->totcol - 1) * in->space_x) / tot_w;
		
		for (int col = 0; col < in->totcol; col++) {
			out->x_array[col] = col ? out->x_array[col - 1] + out->w_array[col - 1] + in->space_x : in->litem_x;
			if (false /** even_columns */) {
				out->w_array[col] = ((in->litem_w - (out->x_array[col] - in->litem_x)) - (in->totcol - col - 1) * in->space_x) / (in->totcol - col);
			}
			else if (col == in->totcol - 1) {
				out->w_array[col] = in->litem_w - (out->x_array[col] - in->litem_x);
			}
			else {
				out->w_array[col] = arr_avg_w[col] * wfac;
			}
		}
	}
	if (out->y_array != NULL && out->h_array != NULL) {
		for (int row = 0; row < in->totrow; row++) {
			if (true /** even_rows */) {
				out->h_array[row] = val_max_h;
			}
			else {
				out->h_array[row] = arr_max_h[row];
			}
			out->y_array[row] = (row) ? out->y_array[row - 1] - in->space_y - out->h_array[row] : in->litem_y - out->h_array[row];
		}
	}
	
	if (out->avgitem) {
		*out->avgitem = val_avg_w;
	}
	if (out->maxitem) {
		*out->maxitem = val_max_h;
	}
	if (out->w) {
		*out->w = tot_w + (in->totcol - 1) * in->space_x;
	}
	if (out->h) {
		*out->h = tot_h + (in->totrow - 1) * in->space_y;
	}
}

void ui_layout_add_but(uiLayout *layout, uiBut *but) {
	uiButtonItem *item = static_cast<uiButtonItem *>(MEM_callocN(sizeof(uiButtonItem), "uiButtonItem"));
	item->type = ITEM_BUTTON;
	item->but = but;
	int w, h;
	ui_item_size(static_cast<uiItem *>(item), &w, &h);
	/** we can flag the button as not expandable, depending on its size. */
	if (w <= 2 * UI_UNIT_X && (but->name == NULL || but->name[0] == '\0')) {
		item->flag |= UI_ITEM_FIXED_SIZE;
	}
	LIB_addtail(&layout->items, item);
	but->layout = layout;
}

ROSE_STATIC uiButtonItem *ui_layout_find_but(const uiLayout *layout, const uiBut *but) {
	LISTBASE_FOREACH(uiItem *, vitem, &layout->items) {
		if (vitem->type == ITEM_BUTTON) {
			uiButtonItem *item = static_cast<uiButtonItem *>(vitem);
			if (item->but == but) {
				return item;
			}
		}
		else {
			uiButtonItem *nested = ui_layout_find_but(static_cast<uiLayout *>(vitem), but);
			if (nested) {
				return nested;
			}
		}
	}
	return NULL;
}

void ui_layout_free(uiLayout *layout) {
	LISTBASE_FOREACH_MUTABLE(uiItem *, vitem, &layout->items) {
		if (vitem->type == ITEM_BUTTON) {
			uiButtonItem *item = static_cast<uiButtonItem *>(vitem);
			/** Remove the reference to the layout for the button. */
			item->but->layout = NULL;
			MEM_delete(item);
		}
		else {
			uiLayout *item = static_cast<uiLayout *>(vitem);
			ui_layout_free(item);
		}
	}
	LIB_listbase_clear(&layout->items);
	MEM_delete(layout);
}

void ui_layout_remove_but(uiLayout *layout, const uiBut *but) {
	int removed = 0;
	LISTBASE_FOREACH(uiItem *, vitem, &layout->items) {
		if (vitem->type == ITEM_BUTTON) {
			uiButtonItem *item = static_cast<uiButtonItem *>(vitem);
			if (item->but == but) {
				LIB_remlink(&layout->items, &item);
				removed++;
			}
		}
	}
	UNUSED_VARS_NDEBUG(removed);
	ROSE_assert(removed <= 1);
}

bool ui_layout_replace_but_ptr(uiLayout *layout, const void *old, uiBut *but) {
	uiButtonItem *item = ui_layout_find_but(layout, static_cast<const uiBut *>(old));
	if (item) {
		item->but = but;
		return true;
	}
	return false;
}

ROSE_INLINE void ui_item_position(uiItem *vitem, const int x, const int y, const int w, const int h) {
	if (vitem->type == ITEM_BUTTON) {
		uiButtonItem *item = static_cast<uiButtonItem *>(vitem);

		item->but->rect.xmin = x;
		item->but->rect.ymin = y;
		item->but->rect.xmax = x + w;
		item->but->rect.ymax = y + h;

		ui_but_update(item->but);
	}
	else {
		uiLayout *item = static_cast<uiLayout *>(vitem);

		item->x = x;
		item->y = y + h;
		item->w = w;
		item->h = h;
	}
}

ROSE_STATIC void ui_item_scale(uiLayout *layout, const float scale[2]) {
	int x, y, w, h;

	LISTBASE_FOREACH(uiItem *, nested, &layout->items) {
		if (nested->type != ITEM_BUTTON) {
			ui_item_scale(static_cast<uiLayout *>(nested), scale);
		}

		ui_item_size(nested, &w, &h);
		ui_item_offset(nested, &x, &y);

		if (scale[0] != 0.0f) {
			x *= scale[0];
			w *= scale[0];
		}
		if (scale[1] != 0.0f) {
			y *= scale[1];
			h *= scale[1];
		}

		ui_item_position(nested, x, y, w, h);
	}
}

ROSE_INLINE int spaces_after_column_item(const uiLayout *layout, const uiItem *i1, const uiItem *i2) {
	if (i1 == NULL || i2 == NULL) {
		return 0;
	}
	return 1;
}

ROSE_INLINE void ui_item_estimate_column(uiLayout *layout) {
	layout->w = 0;
	layout->h = 0;

	int w, h;
	LISTBASE_FOREACH(uiItem *, nested, &layout->items) {
		ui_item_size(nested, &w, &h);

		layout->w = ROSE_MAX(layout->w, w);
		layout->h = layout->h + h;

		const int cnt = spaces_after_column_item(layout, nested, nested->next);
		layout->h = layout->h + layout->space * cnt;
	}
}

ROSE_INLINE int spaces_after_row_item(const uiLayout *layout, const uiItem *i1, const uiItem *i2) {
	if (i1 == NULL || i2 == NULL) {
		return 0;
	}
	return 1;
}

ROSE_INLINE void ui_item_estimate_row(uiLayout *layout) {
	layout->w = 0;
	layout->h = 0;

	int w, h;
	LISTBASE_FOREACH(uiItem *, nested, &layout->items) {
		ui_item_size(nested, &w, &h);

		layout->w = layout->w + w;
		layout->h = ROSE_MAX(layout->h, h);

		const int cnt = spaces_after_row_item(layout, nested, nested->next);
		layout->w = layout->w + layout->space * cnt;
	}
}

ROSE_INLINE void ui_item_estimate_grid(uiLayout *layout) {
	uiLayoutItemGridFlow *grid = static_cast<uiLayoutItemGridFlow *>(layout);
	
	{
		float avg_w;
		float max_h;
		
		UILayoutGridFlowInput input{};
		input.litem_w = layout->w;
		input.litem_x = layout->x;
		input.litem_y = layout->y;
		input.space_x = 0;
		input.space_y = 0;
		
		UILayoutGridFlowOutput output{};
		output.totitem = &grid->totitems;
		output.h = &grid->totrow;
		output.w = &grid->totcol;
		output.avgitem = &avg_w;
		output.maxitem = &max_h;
		ui_layout_grid_flow_compute(&layout->items, &input, &output);
		
		if (grid->totitems == 0) {
			layout->w = layout->h = 0;
			grid->totcol = grid->totrow = 0;
			return;
		}
		
		if (grid->columns > 0) {
			grid->totcol = grid->columns;
		}
		else {
			if (avg_w == 0.0f) {
				grid->totcol = 1;
			}
			else {
				grid->totcol = ROSE_MIN(ROSE_MAX(layout->w / avg_w, 1), grid->totitems);
			}
		}
		grid->totrow = ceilf(((float)grid->totitems) / grid->totcol);
		
		const int modulo = (grid->columns < -1) ? -grid->columns : 0;
		const int step = modulo ? modulo : 1;
		
		/* Adjust number of columns to be multiple of given modulo. */
		if (modulo && grid->totcol % modulo != 0 && grid->totcol > modulo) {
			grid->totcol = grid->totcol - (grid->totcol % modulo);
		}
		
		grid->totrow = ceilf(float(grid->totitems) / grid->totcol);
		while ((grid->totcol - step) > 0 && ceilf(float(grid->totitems) / (grid->totcol - step)) <= grid->totrow) {
			grid->totcol -= step;
		}
	}
	
	{
		UILayoutGridFlowInput input{};
		input.litem_w = layout->w;
		input.litem_x = layout->x;
		input.litem_y = layout->y;
		input.space_x = 0;
		input.space_y = 0;
		input.totcol = grid->totcol;
		input.totrow = grid->totrow;
		UILayoutGridFlowOutput output{};
		output.w = &layout->w;
		output.h = &layout->h;
		ui_layout_grid_flow_compute(&layout->items, &input, &output);
	}
}

ROSE_INLINE void ui_item_estimate_root(uiLayout *layout) {
	ui_item_estimate_column(layout);
}

ROSE_STATIC void ui_item_estimate(uiItem *vitem) {
	if (vitem->type != ITEM_BUTTON) {
		uiLayout *item = static_cast<uiLayout *>(vitem);

		if (LIB_listbase_is_empty(&item->items)) {
			item->w = 0;
			item->h = 0;
			return;
		}

		LISTBASE_FOREACH(uiItem *, nested, &item->items) {
			ui_item_estimate(nested);
		}

		if (item->scale[0] != 0.0f || item->scale[1] != 0.0f) {
			ui_item_scale(item, item->scale);
		}

#define ESTIMATE(type, call) \
	case type:               \
		call;                \
		break

		switch (item->type) {
			ESTIMATE(ITEM_LAYOUT_ROW, ui_item_estimate_row(item));
			ESTIMATE(ITEM_LAYOUT_COL, ui_item_estimate_column(item));
			ESTIMATE(ITEM_LAYOUT_GRID, ui_item_estimate_grid(item));
			ESTIMATE(ITEM_LAYOUT_ROOT, ui_item_estimate_root(item));
		}

#undef ESTIMATE

		if (item->units[0] > 0) {
			item->w = UI_UNIT_X * item->units[0];  // force fixed width size.
		}
		if (item->units[1] > 0) {
			item->w = UI_UNIT_Y * item->units[1];  // force fixed height size.
		}
	}
}

ROSE_INLINE void ui_item_layout_column(uiLayout *layout) {
	int x = layout->x, y = layout->y;

	int w, h;
	LISTBASE_FOREACH(uiItem *, nested, &layout->items) {
		ui_item_size(nested, &w, &h);

		y -= h;
		ui_item_position(nested, x, y, layout->w, h);
		const int cnt = spaces_after_column_item(layout, nested, nested->next);
		y -= cnt * layout->space;
	}

	layout->h = layout->y - y;
	layout->x = x;
	layout->y = y;
}

ROSE_INLINE void ui_item_layout_row(uiLayout *layout) {
	int x = layout->x, y = layout->y;

	int w, h;
	LISTBASE_FOREACH(uiItem *, nested, &layout->items) {
		ui_item_size(nested, &w, &h);

		ui_item_position(nested, x, y - layout->h, w, layout->h);
		const int cnt = spaces_after_row_item(layout, nested, nested->next);
		x += w + cnt * layout->space;
	}

	layout->w = x - layout->x;
	layout->x = x;
	layout->y = y;
}

ROSE_STATIC void ui_item_layout_grid_postfix(uiItem *item, int r, int c) {
	ROSE_assert(item->type == ITEM_BUTTON);
	uiButtonItem *ibut = static_cast<uiButtonItem *>(item);
	uiBut *but = ibut->but;
	
	/** They are denoted by rows, since we want to select full rows! */
	but->draw = DRAW_FLAG(but->draw | UI_BUT_GRID) | DRAW_INDX(r);
}

ROSE_INLINE void ui_item_layout_grid(uiLayout *layout) {
	uiLayoutItemGridFlow *grid = static_cast<uiLayoutItemGridFlow *>(layout);

	if (grid->totitems == 0) {
		layout->w = layout->h = 0;
		return;
	}

	rose::Array<int, 64> widths(grid->totcol);
	rose::Array<int, 64> heights(grid->totrow);
	rose::Array<int, 64> x(grid->totcol);
	rose::Array<int, 64> y(grid->totrow);

	UILayoutGridFlowInput input{};
	input.litem_x = layout->x;
	input.litem_y = layout->y;
	input.litem_w = layout->w;
	input.totcol = grid->totcol;
	input.totrow = grid->totrow;
	
	UILayoutGridFlowOutput output{};
	output.x_array = x.data();
	output.y_array = y.data();
	output.w_array = widths.data();
	output.h_array = heights.data();
	ui_layout_grid_flow_compute(&layout->items, &input, &output);

	int i;
	LISTBASE_FOREACH_INDEX(uiItem *, vitem, &layout->items, i) {
		const int r = i / grid->totcol;
		const int c = i % grid->totcol;

		const int w = widths[c];
		const int h = heights[r];

		ui_item_position(vitem, x[c], y[r], w, h);
		
		if (grid->columns > 0) {
			/** Update the index of the button so multiple buttons can be grouped together! */
			ui_item_layout_grid_postfix(vitem, r, c);
		}
	}

	layout->h = layout->y - y[grid->totrow - 1];
	layout->x = x[grid->totcol - 1] - layout->x + widths[grid->totcol - 1];
	layout->y = layout->y - layout->h;
}

ROSE_INLINE void ui_item_layout_root(uiLayout *layout) {
	ui_item_layout_column(layout);	// default
}

ROSE_STATIC void ui_item_layout(uiItem *vitem) {
	if (vitem->type != ITEM_BUTTON) {
		uiLayout *item = static_cast<uiLayout *>(vitem);

		if (LIB_listbase_is_empty(&item->items)) {
			return;
		}

#define LAYOUT(type, call) \
	case type:             \
		call;              \
		break

		switch (item->type) {
			LAYOUT(ITEM_LAYOUT_ROW, ui_item_layout_row(item));
			LAYOUT(ITEM_LAYOUT_COL, ui_item_layout_column(item));
			LAYOUT(ITEM_LAYOUT_GRID, ui_item_layout_grid(item));
			LAYOUT(ITEM_LAYOUT_ROOT, ui_item_layout_root(item));
		}

#undef LAYOUT

		LISTBASE_FOREACH(uiLayout *, nested, &item->items) {
			ui_item_layout(nested);
		}
	}
}

ROSE_INLINE void ui_layout_end(uiBlock *block, uiLayout *layout, int *r_x, int *r_y) {
	ui_item_estimate(layout);
	ui_item_layout(layout);

	if (r_x) {
		*r_x = layout->x;
	}
	if (r_y) {
		*r_y = layout->y;
	}
}

void UI_block_layout_resolve(uiBlock *block, int *r_x, int *r_y) {
	ROSE_assert(block->active);

	if (r_x) {
		*r_x = 0;
	}
	if (r_y) {
		*r_y = 0;
	}

	block->layout = NULL;

	LISTBASE_FOREACH_MUTABLE(uiLayoutRoot *, root, &block->layouts) {
		ui_layout_add_padding_button(root);

		ui_layout_end(block, root->layout, r_x, r_y);
		ui_layout_free(root->layout);
		MEM_delete(root);
	}

	LIB_listbase_clear(&block->layouts);
}

/** \} */
