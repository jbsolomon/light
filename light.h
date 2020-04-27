#ifndef PHX_LIGHT_H
#define PHX_LIGHT_H

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/***********************************************************************
light.h

Light (prefix LT) is a simple library for fast Vulkan graphics, compute,
and more.

To get started, create an LT_context:

#include "light.h"

LT_context ctx = LT_context_create(NULL);

You can skip ahead to LT_context if you want, or keep reading to find
out about a few other names that Light defines.
*/

/***********************************************************************
LT_stats is one of the useful parts of a context.  It measures how your
program uses the computer's memory, CPU, and graphics, and how fast
Light is working.
*/
typedef struct LT_stats {
    time_t last_frame;
} LT_stats;

/***********************************************************************
DLT_error is another useful part of a context.  You can use it to find
out what went wrong, if anything goes wrong.  Use LT_error_reason to
explain it.
*/
typedef struct DLT_error DLT_error;

/***********************************************************************
LT_context keeps track of things which other Light objects need.  It is
needed by most Light functions.  Use LT_context_create to create a
context.

If a Light function fails, the "error" field of the context will be set.
You can check it directly:

LT_do_something(ctx);
if (ctx->error != NULL) {
    LT_log_error(ctx, "Light failed to do something", ctx->error);
    LT_bail("My Game bailed after an error.");
}

To clean up and exit normally, use LT_exit(ctx); to bail out after an
error, use LT_bail(ctx, "reason").

Once you have a context, you can read about LT_config, or keep reading
for more details.
*/
typedef struct LT_context {
    LT_stats   stats;
    DLT_error* error;
} LT_context;

/***********************************************************************
LT_context_create sets up an LT_context.  Once you have a context, you
can create Light objects using it.

You can pass NULL to use good defaults.  If you pass a context to it,
the new context will be built using the passed context.

Note: Most Light objects need to be built by giving a context to
LT_<type>_create, where "<type>" is the name of the object's type.
*/
LT_context* LT_context_create(LT_context*);

/***********************************************************************
LT_log_info writes a message to the Light log as INFO.

Writing logs during your Light program's run can help you find out what
happened when things fail (or when they go right):

LT_log_info(ctx, "Creating Light config with SDL");
*/
void LT_log_info(LT_context*, const char*, ...);

/***********************************************************************
LT_log_warning writes a message to the Light log as a WARNING.

LT_log_warning(ctx, "Game has too many creatures, it may run slowly.");
*/
void LT_log_warning(LT_context*, const char*, ...);

/***********************************************************************
LT_log_error writes a message to the Light log as an ERROR.  You must
give it an error to explain.
*/
void LT_log_error(LT_context*, const char*, DLT_error*);

/***********************************************************************
LT_context_purge cleans up the LT_context.  If the context has a parent,
LT_context_purge removes the context from its parent.
*/
void LT_context_purge(LT_context*);

/***********************************************************************
LT_exit cleans up the context and exits the program cleanly.
*/
void LT_exit(LT_context*);

/***********************************************************************
LT_bail cleans up the context and exits the program with an error
message.
*/
void LT_bail(LT_context*, const char*);

/***********************************************************************
LT_config controls how an LT_light object is created and how it works.
Light comes with a few LT_configs made for you, like LT_config_sdl (see
below.)

LT_config is needed by LT_light.  The section on LT_light explains more.
*/
typedef struct LT_config {
    const char* name;
    int         version;
} LT_config;

/***********************************************************************
LT_config_sdl returns a default config for SDL, with the name you give
it.  See LT_light_create for how to use it.
*/
LT_config* LT_config_sdl(LT_context*, const char*);

/***********************************************************************
LT_light is the core Light object which lets you control the computer's
windows and graphics.  You can make one with LT_light_create.
*/
typedef struct LT_light {
    LT_config* config;
} LT_light;

/***********************************************************************
LT_light_create creates an LT_light object using the given context and
config:

LT_light* mylight = LT_light_create(ctx, LT_config_sdl("My Game"));
*/
LT_light* LT_light_create(LT_context*, LT_config*);

/***********************************************************************
LT_light_teardown cleans up the windows and graphics used by LT_light.
You need to run it after you're done using the Light object.
*/
void LT_light_teardown(LT_context*, LT_light*);

/***********************************************************************
Now, use the context to create a window, engine, and render target:

LT_window_config wcfg = { "My game", LT_FULLSCREEN };
LT_window win = LT_window_create(ctx, null, wcfg);

if(ctx.error != null) {
  LT_bail(ctx, "creating window");
  exit(1);
}

LT_window win = LT_window_create(ctx, LT_window{

// ...

To enable advanced mode, #define DR_LIGHT 1.

************************************************************************

It's easy to build, customize, and extend Light.  Please see the
README for more info, or get the source here:
https://github.com/phoenix-engine/light
*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#ifdef DR_LIGHT
#include "light_dr.h"
#endif /* DR_LIGHT */

#endif /* PHX_LIGHT_H */
