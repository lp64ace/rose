#ifndef ED_NUMINPUT_H
#define ED_NUMINPUT_H

#include "LIB_utildefines.h"

#define NUM_STR_REP_LEN 64
#define NUM_MAX_ELEMENTS 3

struct rContext;
struct wmEvent;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NumInput {
	int index_max;
	
	int flag_value[3];
	int flag;

	float value[NUM_MAX_ELEMENTS];
	float value_original[NUM_MAX_ELEMENTS];
	float value_increment[NUM_MAX_ELEMENTS];

	/** String as typed by the user for edited value. */
	char text[NUM_STR_REP_LEN];
	/** Current position of the cursor in edited value str. */
	int txt_current;
	/** Current index of active element/value that we are editing. */
	int txt_index;
} NumInput;

/** #NumInput->flag */
enum {
	NUM_AFFECT_ALL = 1 << 0,
};

/** #NumInput->flag_value */
enum {
	NUM_NULL_ONE = 1 << 0,
	NUM_NO_NEGATIVE = 1 << 1,
	NUM_NO_ZERO = 1 << 2,
	NUM_NO_FRACTION = 1 << 3,
};

/* -------------------------------------------------------------------- */
/** \name NumInput
 * \{ */

void ED_number_input_init(struct NumInput *ni);
void ED_number_input_output(struct NumInput *ni, char *str);

bool ED_number_input_has(const struct NumInput *ni);
bool ED_number_input_apply(struct NumInput *ni, float *vec);
bool ED_number_input_handle(struct rContext *C, struct NumInput *ni, const struct wmEvent *event);

/** \} */

#ifdef __cplusplus
}
#endif

#endif // !ED_NUMINPUT_H
