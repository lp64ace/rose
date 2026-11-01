#include "ED_numinput.h"

#include "UI_interface.h"

#include "KER_context.h"
#include "KER_scene.h"

#include "LIB_math_vector.h"
#include "LIB_string.h"
#include "LIB_string_utf.h"
#include "LIB_string_utils.h"
#include "LIB_utildefines.h"

#include "WM_api.h"

/* Numeric input which isn't allowing full numeric editing. */
#define USE_FAKE_EDIT

/** #NumInput->flag (private) */
enum {
	/** Enable full editing, with units and math operators support. */
	NUM_EDIT_FULL = 1 << 9,
	/** Fake edited state (temp, avoids issue with backspace). */
	NUM_FAKE_EDITED = 1 << 10,
};

/** #NumInput->flag_value (private) */
enum {
	/** User has edited this value somehow. */
	NUM_EDITED = (1 << 9),
	/** Current expression for this value is invalid. */
	NUM_INVALID = (1 << 10),
	/** Current expression's result has to be negated. */
	NUM_NEGATE = (1 << 11),
	/** Current expression's result has to be inverted. */
	NUM_INVERSE = (1 << 12),
};

void ED_number_input_init(NumInput *ni) {
	ni->index_max = 0;
	for (int i = 0; i < ARRAY_SIZE(ni->value); i++) {
		ni->flag_value[i] = 0;

		ni->value[i] = 0.0f;
		ni->value_original[i] = 0.0f;
		ni->value_increment[i] = 1.0f;
	}

	ni->txt_index = 0;
	ni->txt_current = 0;
	ni->text[0] = '\0';
}

void ED_number_input_output(NumInput *ni, char *str) {
	const int length = NUM_STR_REP_LEN;

	for (int i = 0; i <= ni->index_max; i++) {
		const int index = (ni->flag & NUM_AFFECT_ALL && ni->txt_index != i && !(ni->flag_value[i] & NUM_EDITED)) ? 0 : i;
		const float factor = 1.0f; // Unit factor here needed here ?!

		int precision = 2;
		if (ni->flag_value[index] & NUM_EDITED) {
			precision = ui_calc_float_precision(precision, ni->value[index]);
			if (index == ni->txt_index) {
				const char *heading_exp = "", *trailing_exp = "";
				char before_cursor[NUM_STR_REP_LEN];
				char value[64];

#ifdef USE_FAKE_EDIT
				if (ni->flag_value[index] & NUM_NEGATE) {
					heading_exp = (ni->flag_value[index] & NUM_INVERSE) ? "-1/(" : "-(";
					trailing_exp = ")";
				}
				else if (ni->flag_value[index] & NUM_INVERSE) {
					heading_exp = "1/(";
					trailing_exp = ")";
				}
#endif

				if (ni->flag_value[index] & NUM_INVALID) {
					LIB_strcpy(value, sizeof(value), "Invalid");
				}
				else {
					LIB_strnformat(value, sizeof(value), "%.*f", precision, ni->value[index] * factor);
				}

				LIB_strncpy(before_cursor, sizeof(before_cursor), ni->text, ni->txt_current);
				LIB_strnformat(&str[i * length], length, "[%s%s|%s%s] = %s", heading_exp, before_cursor, &ni->text[ni->txt_current], trailing_exp, value);
			}
			else {
				const char *cur = (index == ni->txt_index) ? "|" : "";
				char value[NUM_STR_REP_LEN];
				LIB_strnformat(value, sizeof(value), "%.*f", precision, ni->value[index] * factor);
				LIB_strnformat(&str[i * length], length, "%s%s%s", cur, value, cur);
			}
		}
		else {
			const char *cur = (i == ni->txt_index) ? "|" : "";
			LIB_strnformat(&str[i * length], length, "%sNONE%s", cur, cur);
		}
	}
}

bool ED_number_input_has(const NumInput *ni) {
#ifdef USE_FAKE_EDIT
	if (ni->flag & NUM_FAKE_EDITED) {
		return true;
	}
#endif

	for (int index = 0; index <= ni->index_max; index++) {
		if (ni->flag_value[index] & NUM_EDITED) {
			return true;
		}
	}

	return false;
}

bool ED_number_input_apply(NumInput *ni, float *vec) {
	if (ED_number_input_has(ni)) {
		for (int i = 0; i <= ni->index_max; i++) {
			float value;
#ifdef USE_FAKE_EDIT
			if (ni->flag & NUM_FAKE_EDITED) {
				value = ni->value[i];
			}
			else
#endif
			{
				/* if AFFECTALL and no number typed and cursor not on number, use first number */
				int index = (ni->flag & NUM_AFFECT_ALL && ni->txt_index != i && !(ni->flag_value[i] & NUM_EDITED)) ? 0 : i;
				value = (!(ni->flag_value[index] & NUM_EDITED) && ni->flag_value[i] & NUM_NULL_ONE) ? 1.0f : ni->value[index];

				if (ni->flag_value[i] & NUM_NO_NEGATIVE && value < 0.0f) {
					value = 0.0f;
				}
				if (ni->flag_value[i] & NUM_NO_FRACTION && value != floorf(value)) {
					value = floorf(value + 0.5f);
					if (ni->flag_value[i] & NUM_NO_ZERO && value == 0.0f) {
						value = 1.0f;
					}
				}
				else if (ni->flag_value[i] & NUM_NO_ZERO && value == 0.0f) {
					value = 0.0001f;
				}
			}
			vec[i] = value;
		}
#ifdef USE_FAKE_EDIT
		ni->flag &= ~NUM_FAKE_EDITED;
#endif
		return true;
	}

	/* Else, we set the 'org' values for numinput! */
	for (int i = 0; i <= ni->index_max; i++) {
		ni->value[i] = ni->value_original[i] = vec[i];
	}
	return false;
}

ROSE_STATIC void value_to_editstr(NumInput *ni, int index) {
	const int precision = 6;
	ni->txt_current = LIB_strnformat(ni->text, NUM_STR_REP_LEN, "%.*f", precision, ni->value[index]);
}

ROSE_STATIC bool editstr_insert_at_cursor(NumInput *ni, const char *buf, const int buflen) {
	int cur = ni->txt_current;
	int len = LIB_strlen(&ni->text[cur]) + 1;
	int n_cur = cur + buflen;

	if (n_cur + len >= NUM_STR_REP_LEN) {
		return false;
	}

	memmove(&ni->text[n_cur], &ni->text[cur], len);
	memcpy(&ni->text[cur], buf, sizeof(char) * buflen);

	ni->txt_current = n_cur;
	return true;
}

bool user_string_to_member(rContext *C, const char *str, int type, double *r_value, const bool use_single_line_error, char **r_error) {
	*r_value = atof(str);
	return true;
}

static bool editstr_is_simple_numinput(const char ascii) {
	if (ascii >= '0' && ascii <= '9') {
		return true;
	}
	if (ascii == '.') {
		return true;
	}
	return false;
}

bool ED_number_input_handle(rContext *C, NumInput *ni, const wmEvent *event) {
	const char *utf8 = NULL;
	char evt_ascii = LIB_str_utf8_size_or_error(event->input) == 1 ? event->input[0] : '\0';
	char arr_ascii[2] = {'\0', '\0'};
	int index = ni->txt_index;

	if (((event->modifier & (KM_CTRL | KM_ALT)) == 0) && (evt_ascii != '\0') && strchr("01234567890@%^&*-+/{}()[]<>.|", evt_ascii)) {
		if (!(ni->flag & NUM_EDIT_FULL)) {
			ni->flag |= NUM_EDITED;
			ni->flag |= NUM_EDIT_FULL;
			ni->flag_value[index] |= NUM_EDITED;
		}
	}

#ifdef USE_FAKE_EDIT
	/* XXX Hack around keyboards without direct access to '=' nor '*'... */
	if (ELEM(evt_ascii, '=', '*')) {
		if (!(ni->flag & NUM_EDIT_FULL)) {
			ni->flag |= NUM_EDIT_FULL;
			ni->flag_value[index] |= NUM_EDITED;
			return true;
		}
		if (event->modifier & KM_CTRL) {
			ni->flag &= ~NUM_EDIT_FULL;
			return true;
		}
	}
#endif

	bool updated = false;

	int cur = STRCUR_DIR_NEXT;
	int mode = STRCUR_JUMP_NONE;
	int dir = 1;

	switch(event->type) {
		case EVT_BACKSPACEKEY: {
			if (!(ni->flag_value[index] & NUM_EDITED)) {
				copy_v3_v3(ni->value, ni->value_original);
				ni->flag_value[0] &= ~NUM_EDITED;
				ni->flag_value[1] &= ~NUM_EDITED;
				ni->flag_value[2] &= ~NUM_EDITED;
#ifdef USE_FAKE_EDIT
				ni->flag |= NUM_FAKE_EDITED;
#else
				ni->flag |= NUM_EDIT_FULL;
#endif
				updated |= true;
				break;
			}
			else if ((event->modifier & KM_SHIFT) || !ni->text[0]) {
				ni->value[index] = ni->value_original[index];
				ni->flag_value[index] &= ~NUM_EDITED;
				ni->text[0] = '\0';
				ni->txt_current = 0;
				updated |= true;
				break;
			}

			dir = 0;

			/* fallthrough */
		}
		case EVT_DELKEY: {
			if ((ni->flag_value[index] & NUM_EDITED) && ni->text[0]) {
				int t_cur = cur = ni->txt_current;
				if (event->modifier & KM_CTRL) {
					mode = STRCUR_JUMP_DELIM;
				}

				LIB_str_cursor_step_utf8(ni->text, LIB_strlen(ni->text), &t_cur, (eStrCursorJumpDirection)dir, (eStrCursorJumpType)mode, true);

				if (t_cur != cur) {
					if (t_cur < cur) {
						SWAP(int, t_cur, cur);
						ni->txt_current = cur;
					}
					/* +1 for trailing '\0'. */
					memmove(&ni->text[cur], &ni->text[t_cur], strlen(&ni->text[t_cur]) + 1);
					updated |= true;
				}
				if (!ni->text[0]) {
					ni->value[index] = ni->value_original[index];
				}
			}
			else {
				return false;
			}
		} break;
		case EVT_LEFTARROWKEY: {
			dir = STRCUR_DIR_PREV;
			/* fallthrough */
		}
		case EVT_RIGHTARROWKEY: {
			cur = ni->txt_current;
			if (event->modifier & KM_CTRL) {
				mode = STRCUR_JUMP_DELIM;
			}

			LIB_str_cursor_step_utf8(ni->text, LIB_strlen(ni->text), &cur, (eStrCursorJumpDirection)dir, (eStrCursorJumpType)mode, true);

			if (cur != ni->txt_current) {
				ni->txt_current = cur;
				return true;
			}

			return false;
		} break;
		case EVT_HOMEKEY: {
			if (ni->text[0]) {
				ni->txt_current = 0;
				return true;
			}
			return false;
		}
		case EVT_ENDKEY: {
			if (ni->text[0]) {
				ni->txt_current = LIB_strlen(ni->text);
				return true;
			}
			return false;
		}
		case EVT_TABKEY: {
#ifdef USE_FAKE_EDIT
			ni->flag_value[index] &= ~(NUM_NEGATE | NUM_INVERSE);
#endif

			index = (index + ni->index_max + ((event->modifier & KM_SHIFT) ? 0 : 2)) % (ni->index_max + 1);
			ni->txt_index = index;

			if (ni->flag_value[index] & NUM_EDITED) {
				value_to_editstr(ni, index);
			}
			else {
				ni->text[0] = '\0';
				ni->txt_current = 0;
			}
			return true;
		}
		case EVT_PADPERIOD:
		case EVT_PERIODKEY:
			arr_ascii[0] = '.';
			utf8 = arr_ascii;
			break;
		case EVT_PADMINUS:
		case EVT_MINUSKEY: {
			if ((event->modifier & KM_CTRL) || !(ni->flag & NUM_EDIT_FULL)) {
				ni->flag_value[index] ^= NUM_NEGATE;
				updated |= true;
			}
		} break;
		case EVT_PADSLASHKEY:
		case EVT_SLASHKEY: {
			if ((event->modifier & KM_CTRL) || !(ni->flag & NUM_EDIT_FULL)) {
				ni->flag_value[index] ^= NUM_INVERSE;
				updated |= true;
			}
		} break;
	}

	if (!updated && !utf8 && event->input[0]) {
		utf8 = event->input;
		arr_ascii[0] = evt_ascii;
	}

	if (!updated && (event->modifier & KM_CTRL)) {
		return false;
	}

	if ((!utf8 || !utf8[0]) && arr_ascii[0]) {
		utf8 = arr_ascii;
	}

	if (utf8 && utf8[0]) {
		if (!(ni->flag & NUM_EDIT_FULL)) {
			if (!editstr_is_simple_numinput(utf8[0])) {
				return false;
			}
		}

		const unsigned int utf8_length = LIB_str_utf8_size_or_error(utf8);
		ROSE_assert(utf8_length != -1);
		if (!editstr_insert_at_cursor(ni, utf8, utf8_length)) {
			return true;
		}

		ni->flag_value[index] |= NUM_EDITED;
	}
	else if (!updated) {
		return false;
	}

	if (ni->text[0]) {
		const float value_prev = ni->value[index];
		char *error = NULL;

		double value;
		if (user_string_to_member(C, ni->text, 0, &value, false, &error)) {
			ni->value[index] = (float)value;
			ni->flag_value[index] &= ~NUM_INVALID;
		}
		else {
			ni->flag_value[index] |= NUM_INVALID;
		}

#ifdef USE_FAKE_EDIT
		if (ni->flag_value[index] & NUM_NEGATE) {
			ni->value[index] = -ni->value[index];
		}
		if (ni->flag_value[index] & NUM_INVERSE) {
			value = ni->value[index];
			ni->value[index] = (float)value;
		}
#endif

		if (!isfinite(ni->value[index])) {
			ni->value[index] = value_prev;
			ni->flag_value[index] |= NUM_INVALID;
		}
	}

	return true;
}
