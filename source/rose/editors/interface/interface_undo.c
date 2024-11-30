#include "MEM_guardedalloc.h"

#include "DNA_screen_types.h"
#include "DNA_vector_types.h"
#include "DNA_windowmanager_types.h"

#include "UI_interface.h"

#include "KER_context.h"
#include "KER_screen.h"

#include "LIB_ghash.h"
#include "LIB_listbase.h"
#include "LIB_rect.h"
#include "LIB_string.h"
#include "LIB_string_utils.h"
#include "LIB_utildefines.h"

#include "WM_api.h"
#include "WM_handler.h"
#include "WM_window.h"

#include "interface_intern.h"

#include "RFT_api.h"

/* -------------------------------------------------------------------- */
/** \name Text Edit
 * \{ */

typedef struct uiUndoStack_Text_State {
	struct uiUndoStack_Text_State *prev, *next;
	int cursor_index;
	char text[0];
} uiUndoStack_Text_State;

typedef struct uiUndoStack_Text {
	ListBase states;
	uiUndoStack_Text_State *current;
} uiUndoStack_Text;

ROSE_STATIC const char *ui_textedit_undo_impl(uiUndoStack_Text *stack, int *cursor_index) {
	if (stack->current == NULL) {
		return NULL;
	}

	/* Travel backwards in the stack and copy information to the caller. */
	if (stack->current->prev != NULL) {
		stack->current = stack->current->prev;

		*cursor_index = stack->current->cursor_index;
		return stack->current->text;
	}
	return NULL;
}

ROSE_STATIC const char *ui_textedit_redo_impl(uiUndoStack_Text *stack, int *cursor_index) {
	if (stack->current == NULL) {
		return NULL;
	}

	/* Only redo if new data has not been entered since the last undo. */
	if (stack->current->next != NULL) {
		stack->current = stack->current->next;

		*cursor_index = stack->current->cursor_index;
		return stack->current->text;
	}
	return NULL;
}

const char *ui_textedit_undo(uiUndoStack_Text *stack, int direction, int *cursor_index) {
	if (direction < 0) {
		return ui_textedit_undo_impl(stack, cursor_index);
	}
	return ui_textedit_redo_impl(stack, cursor_index);
}

void ui_textedit_undo_push(uiUndoStack_Text *stack, const char *text, int cursor_index) {
	if (stack->current) {
		while (stack->current->next) {
			uiUndoStack_Text_State *state = stack->current->next;
			LIB_remlink(&stack->states, state);
			MEM_freeN(state);
		}
	}

	const unsigned int size = LIB_strlen(text) + 1;
	stack->current = MEM_mallocN(sizeof(uiUndoStack_Text_State) + size, "uiUndoStack_Text_State");
	stack->current->cursor_index = cursor_index;
	memcpy(stack->current->text, text, size);
	LIB_addtail(&stack->states, stack->current);
}

uiUndoStack_Text *ui_textedit_undo_stack_create(void) {
	uiUndoStack_Text *stack = MEM_mallocN(sizeof(uiUndoStack_Text), "uiUndoStack_Text");
	stack->current = NULL;
	LIB_listbase_clear(&stack->states);
	return stack;
}

void ui_textedit_undo_stack_destroy(uiUndoStack_Text *stack) {
	LIB_freelistN(&stack->states);
	MEM_freeN(stack);
}

/** \} */
