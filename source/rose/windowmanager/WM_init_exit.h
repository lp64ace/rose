#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Init the window manager modure this will setup the rose application from the default file or load the default settings if
 * the file cannot be found.
 *
 * \param C The context we want to initialize, this can be either an empty context that we are gonna populate or it can be a
 * fully initialized context we want to activate.
 */
void WM_init(struct Context *C);

/**
 * The main loop, beware that this function does not return.
 *
 * \param C The context to use for updates.
 */
void WM_main(struct Context *C);

/**
 * This function is called when the application has to exit, any leftover windows or memory is freed, including the memory of
 * the context.
 */
void WM_exit(struct Context *C);

#ifdef __cplusplus
}
#endif
