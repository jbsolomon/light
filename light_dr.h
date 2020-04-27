#ifndef DR_LIGHT_H
#define DR_LIGHT_H

#include <stdarg.h>
#include <stddef.h>

#include <SDL.h>

/* See volk/README.md. */
#define VK_NO_PROTOTYPES
#include "vulkan/vulkan.h"

#include "light.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/***********************************************************************
light_dr.h

September 18, 20XX.

Under the hood, Light stores and uses larger "DLT" types which contain
and manage types from third-party libraries, such as Vulkan, SDL2, and
so on.  Including this header causes your code to rely on symbols from
their headers.

The necessary dependencies are:
- Vulkan-Headers: https://github.com/phoenix-engine/Vulkan-Headers
- SDL2: https://github.com/phoenix-engine/SDL

Top-level DLT types mirror their LT equivalents: for example, LT_context
has a matching but larger DLT_context, which includes an allocator.
When the user calls a LT function, it calls the equivalent DLT function
with the expanded type and casts the result back to the simple type.

DLT_ types and functions don't have lifetimes (TODO: or cause
allocation), but you must manage them yourself.  For convenience, you
may want to use LT_*_create functions in most places and cast their
results to DLT_ types yourself when you want extra control.  Otherwise,
you _must_ understand their implementation, because all Light functions
make assumptions about their implementation and the contents of Light
types.
*/

/***********************************************************************
LT_VERSION is the name of this version of Light, such as
"Light v0.0.1".
*/
extern const char* const LT_VERSION;

/***********************************************************************
LT_VERSION_NUMBER is the number of the major version of Light, such as
1.
*/
extern const int LT_VERSION_NUMBER;

/***********************************************************************
LT_VK_API_VERSION is the current Vulkan API version targeted by Light.
*/
extern const int LT_VK_API_VERSION;

/***********************************************************************
DLT_context will be defined later, but DLT_error depends on it, so
declare it first.
*/
typedef struct DLT_context DLT_context;

/***********************************************************************
DLT_ERROR_MEM_SIZE determines the stack size of DLT_error data.  If you
want to change this, you MUST recompile!
*/
#define DLT_ERROR_MEM_SIZE 256

/***********************************************************************
DLT_ERROR_STACK_MAX determines the maximum error stack size.  If you
want to change this, you MUST recompile!
*/
#define DLT_ERROR_STACK_MAX 16

/***********************************************************************
DLT_error is Light's internal error type.  It carries its own memory
with it, so DLT_error_push can snprintf into its data buffer.  When a
new error is pushed, it is added to the end of errs, and err_offset is
incremented.  str_offset is also incremented by the size of the written
string, including terminal NULL.

After errors have been pushed, usable strings are the value of errs[i].
DLT_error memory may be reused by resetting str_offset and err_offset to
0.
*/
struct DLT_error {
    char  data[DLT_ERROR_MEM_SIZE];
    char* errs[DLT_ERROR_STACK_MAX];

    size_t str_offset;
    size_t err_offset;

    /* why can explain why something failed unexpectedly. */
    char* why;
};

/***********************************************************************
DLT_error_create prepares a DLT_error with sane defaults, without
zeroing all buffers.
*/
DLT_error DLT_error_create();

/***********************************************************************
DLT_error_push pushes a new error to the error stack, storing it in the
data field.  It returns a pointer to the error head.

The caller may check for room in the error buffer using:

ctx->error.err_offset < DLT_ERROR_MEM_SIZE
*/
DLT_error* DLT_error_push(DLT_context*, const char*, ...);

/***********************************************************************
DLT_mem_alloc is an allocator with the same signature as malloc.  You
can use malloc or jemalloc, or a custom stack allocator, for example.
Its allocations are tracked in DLT_context and may be cleaned up with
DLT_context_purge.  It should be used with a corresponding DLT_mem_free.
*/
typedef void* (*DLT_mem_alloc)(size_t);

/***********************************************************************
DLT_mem_free is a memory deallocator corresponding to DLT_mem_alloc.
*/
typedef void (*DLT_mem_free)(void*);

/***********************************************************************
DLT_log_level is a level enum for logging levels, with the usual
semantics.  The default log level is DLT_LEVEL_WARN.
*/
typedef enum DLT_log_level {
    DLT_LEVEL_DEBUG,
    DLT_LEVEL_INFO,
    DLT_LEVEL_WARN,
    DLT_LEVEL_ERROR,
    DLT_LEVEL_FATAL
} DLT_log_level;

#ifndef LIGHT_LOG_LEVEL
const int _LIGHT_LOG_LEVEL = DLT_LEVEL_INFO;
/*
const int _LIGHT_LOG_LEVEL = DLT_LEVEL_DEBUG;
*/
#else
const int _LIGHT_LOG_LEVEL = LIGHT_LOG_LEVEL;
#endif

/***********************************************************************
DLT_logger is a logger which can be configured to log to stderr, a file,
a multi-logger, or a game terminal, for example.  It uses the typical
printf formatting string and so on.
*/
typedef int (*DLT_logger)(DLT_log_level, const char*, va_list);

/***********************************************************************
DLT_log_errf is a passthrough to DLT_log_verrf which prints the given
format string to stderr using fprintf.
*/
int DLT_log_errf(DLT_log_level, const char*, ...);

/***********************************************************************
DLT_log_verrf passes the given string through to fprintf to stderr, with
the given logging level.  If the user defines LIGHT_LOG_LEVEL, it only
logs messages with greater or equal severity.  LIGHT_LOG_LEVEL defaults
to DLT_LEVEL_WARN.

Example:

#define LIGHT_LOG_LEVEL DLT_LEVEL_WARN
*/
int DLT_log_verrf(DLT_log_level, const char*, va_list);

/***********************************************************************
DLT_vec is a resizeable vector.  The user should know the type of its
items.
*/
typedef struct DLT_vec {
    /* NOTE: If you ever change the definition of this, you MUST update
     * at least the DLT_STATIC_VEC macro in light.c and recompile! */

    /* size is the item size. */
    size_t       size;
    unsigned int count;
    unsigned int capacity;
    void**       items;
} DLT_vec;

/***********************************************************************
DLT_slice is a slice into a vec.  It does not own its memory; use with
caution.
*/
typedef DLT_vec DLT_slice;

/***********************************************************************
DLT_vec_create creates a DLT_vec of items with the given size, using the
given array root and count as a child of the given context.  To create a
vec with capacity larger than count, use DLT_vec_create_cap.

If root is not NULL, it will be assumed to refer to valid memory up to
at least count items of the given size.  Use with care!
*/
DLT_vec DLT_vec_create(DLT_context*, size_t, void*, unsigned int);

/***********************************************************************
DLT_vec_create_cap creates a DLT_vec of items with the given size,
count, and capacity, in all else being identical to DLT_vec_create.
 */
DLT_vec DLT_vec_create_cap(
  DLT_context*, size_t, void*, unsigned int, unsigned int);

/***********************************************************************
DLT_vec_create_strs is a shortcut to create a DLT_vec of const char*.
*/
DLT_vec DLT_vec_create_strs(DLT_context*, unsigned int);

/***********************************************************************
DLT_vec_slice returns a slice into the vec, beginning at begin, and
ending at the second argument.

WARNING: The slice created by DLT_vec_slice is a view into the given
vec.  It does not own its memory.  Discarding or releasing the original
vec may result in memory leaks or use-after-free errors!
*/
DLT_slice DLT_vec_slice(DLT_vec, unsigned int begin, unsigned int end);

/***********************************************************************
DLT_slice_slice returns a slice into the slice, beginning at begin, and
ending at the second argument.
*/
DLT_slice
DLT_slice_slice(DLT_slice, unsigned int begin, unsigned int end);

/***********************************************************************
DLT_vec_append appends the object pointed to by item to the given vec,
expanding the vec if necessary.  Note that this means the old vec may
become invalid.  It returns the resulting vec.
*/
DLT_vec DLT_vec_append(DLT_context*, DLT_vec, void**);

/***********************************************************************
DLT_vec_cat concatenates the given vecs.  They must have the same item
size.  If the target vec needs to be resized, it will be.
*/
DLT_vec DLT_vec_cat(DLT_context*, DLT_vec, DLT_vec);

/***********************************************************************
DLT_vec_expand increases the capacity of vec to the given amount.  If it
needs to reallocate, it copies the buffer into the new one and releases
the old one.
*/
DLT_vec DLT_vec_expand(DLT_context*, DLT_vec, unsigned int);

/***********************************************************************
DLT_vec_resize is like DLT_vec_expand but also changes the element count
of the vec.
*/
DLT_vec DLT_vec_resize(DLT_context*, DLT_vec, unsigned int);

/***********************************************************************
DLT_vec_copy copies the elements of src (up to count) into target.
src and target must have the same element size.  It returns target.
*/
DLT_vec DLT_vec_copy(DLT_vec target, DLT_vec src);

/***********************************************************************
DLT_vec_clone creates a new vec having the same size and count as
the given vec, and copies the given vec into it.
*/
DLT_vec DLT_vec_clone(DLT_context*, DLT_vec);

/***********************************************************************
DLT_vec_sort sorts the given DLT_vec using the given function on its
elements.
*/
DLT_vec DLT_vec_sort(DLT_vec, int (*)(void*, void*));

/***********************************************************************
DLT_vec_sort_strings is a sorter for DLT_vec_sort suitable for the
case when DLT_vec items are char*s.  qsort takes the address of the
elements of its target, so using it with strcmp directly doesn't
work.
*/
int DLT_vec_sort_strings(void*, void*);

/***********************************************************************
DLT_vec_contains_strings returns 1 if all of the second argument's
strings are present in the first vec.  Both vecs must have sorted char*
items.
*/
int DLT_vec_contains_strings(DLT_vec, DLT_vec);

/***********************************************************************
DLT_vec_release releases vec's memory.
*/
void DLT_vec_release(DLT_context*, DLT_vec);

/***********************************************************************
DLT_cleanup is a cleanup handler for DLT_context.  The second
parameter is a pointer to any custom data needed by the cleanup (or
possibly the resource handle to be cleaned up.)
*/
typedef DLT_error* (*DLT_cleanup)(DLT_context*, void*, void*);

/***********************************************************************
TODO: DLT_context is undocumented because its design needs work.
*/
struct DLT_context {
    LT_context          base;
    struct DLT_context* parent;

    /*
     * error is the value pointed to by base.error when an error has
     * occurred.  This is to avoid allocation in case of OOM.
     */
    DLT_error error;

    DLT_mem_alloc dalloc;
    DLT_mem_free  dfree;
    DLT_logger    logger;

    /* cleanup is a stack of function pointers to cleanups.  If a
     * cleanup needs to store some extra data, it is stored in
     * cleanup_data.
     */
    DLT_vec cleanup;

    /* cleanup_data is a stack of pointers to any data needed by the
     * cleanup with the same index, possibly the resource handle to
     * be cleaned up. */
    DLT_vec cleanup_data;
};

/***********************************************************************
DLT_context_create creates a context.  If a NULL context is passed,
it defaults to using malloc, free, and printf.

If a non-NULL DLT_context is passed, it creates the new context
using the DLT_context and copies the parent's contents, including
all LT_context values such as stats.
*/
DLT_context* DLT_context_create(DLT_context*);

/***********************************************************************
DLT_context_cleanup_push pushes a cleanup to the given context's
cleanups.  The third parameter is any custom data the cleanup may
need.
*/
DLT_error* DLT_cleanup_push(DLT_context*, DLT_cleanup, void*);

/***********************************************************************
DLT_context_alloc uses the given context to allocate the memory.
*/
void* DLT_context_alloc(DLT_context*, size_t);

/***********************************************************************
DLT_context_free uses the given context to free the memory.
*/
void DLT_context_free(DLT_context*, void*);

/***********************************************************************
DLT_log logs using the context's logger with the given log level.
*/
int DLT_log(DLT_context*, DLT_log_level, const char*, ...);

/***********************************************************************
DLT_vlog logs using the context's logger with the given log level,
with va_list args.
*/
int DLT_vlog(DLT_context*, DLT_log_level, const char*, va_list);

/***********************************************************************
DLT_context_purge purges the given context, including pruning it
from its parent, if any.
*/
void DLT_context_purge(DLT_context*);

typedef struct DLT_panel DLT_panel;

/***********************************************************************
DLT_shader is a shader.  It contains a char* vec of the shader data.
TODO: It specifies its inputs and outputs.
*/
typedef struct DLT_shader {
    DLT_vec data;
} DLT_shader;

/***********************************************************************
DLT_shader_loader is a loader for a DLT_shader by name.
*/
typedef DLT_error* (*DLT_shader_loader)(DLT_context*,
                                        DLT_shader*,
                                        const char*);

/***********************************************************************
DLT_vulkan_pipeline_config is a config for a DLT_vulkan_pipeline.
*/
typedef struct DLT_vulkan_pipeline_config {
    DLT_shader_loader loader;
} DLT_vulkan_pipeline_config;

/***********************************************************************
DLT_config parameterizes a DLT_light with e.g. surface management
functions and flags.  data is a custom value that different
implementations, such as SDL, may use to store their state.
build_panel is used in DLT_light_create to create a DLT_panel, and
teardown_panel is used in DLT_light_teardown.
*/
typedef struct DLT_config {
    LT_config base;
    void*     data;

    DLT_panel (*build_panel)(DLT_context*, struct DLT_config*);
    void (*teardown_panel)(DLT_context*, DLT_panel);

    /* 0 if false; 1 if true. */
    int debugging;

    DLT_vulkan_pipeline_config pipeline_cfg;
} DLT_config;

/***********************************************************************
DLT_config_create creates a DLT_config with the given window name
and allocates data memory for the given size.
*/
DLT_config* DLT_config_create(DLT_context*, const char*, size_t);

/***********************************************************************
DLT_config_destroy cleans up the DLT_config.
*/
void DLT_config_destroy(DLT_context*, DLT_config*);

/***********************************************************************
DLT_DEFUALT_SDL_WINDOW_FLAGS is the default window flags for SDL. It
is hardcoded as fullscreen or windowed, etc. depending on whether
NDEBUG is defined.
*/
extern const SDL_WindowFlags DLT_SDL_DEFUALT_WINDOW_FLAGS;

/***********************************************************************
DLT_sdl_config_data is the config data for an SDL config.
*/
typedef struct DLT_sdl_config_data {
    SDL_WindowFlags flags;
} DLT_sdl_config_data;

/***********************************************************************
DLT_vulkan_loader is a shorthand for the Vulkan instance function
loader type.
*/
typedef PFN_vkVoidFunction (*DLT_vulkan_loader)(VkInstance,
                                                const char*);

/***********************************************************************
DLT_vulkan_pipeline is a loaded and usable Vulkan pipeline.
*/
typedef struct DLT_vulkan_pipeline {
    /* TODO: Refactor out. */
    VkSwapchainKHR swc;
    DLT_vec        swc_images;
    DLT_vec        swc_image_views;
    DLT_shader     vert, frag;
} DLT_vulkan_pipeline;

/***********************************************************************
TODO: Document me.
*/
void DLT_vulkan_pipeline_teardown(DLT_context*,
                                  VkDevice,
                                  DLT_vulkan_pipeline);

/***********************************************************************
DLT_queue_indices contains the indices of various Vulkan queues.
*/
typedef struct DLT_queue_indices {
    DLT_vec gfx;
    DLT_vec compute;
    DLT_vec pres;
} DLT_queue_indices;

/***********************************************************************
DLT_vulkan_queues is the interface to Vulkan's graphics, presentation,
and compute queues.
 */
typedef struct DLT_vulkan_queues {
    DLT_queue_indices indices;
    VkQueue           gfx, pres, compute;
} DLT_vulkan_queues;

/***********************************************************************
DLT_vulkan is a container for Vulkan resources.
*/
typedef struct DLT_vulkan {
    DLT_vulkan_loader loader;

    VkInstance instance;

    VkDebugUtilsMessengerEXT debugger;

    VkSurfaceKHR surface;

    VkPhysicalDevice phys_device;
    VkDevice         device;

    DLT_vulkan_queues queues;

    DLT_vulkan_pipeline pipeline;
} DLT_vulkan;

/***********************************************************************
DLT_vulkan_pipeline_create
*/
DLT_vulkan_pipeline
DLT_vulkan_pipeline_create(DLT_context*, DLT_panel, DLT_vulkan);

/***********************************************************************
DLT_default_vk_layers is a list of the default Vulkan layers.  If
NDEBUG is defined, it includes validation and debugging layers.
 */
extern const char* DLT_default_vk_layers[];

/***********************************************************************
DLT_default_vk_extns is a list of the default Vulkan extensions.  If
NDEBUG is defined, it includes the debugging extension.
 */
extern const char* DLT_default_vk_extns[];

/***********************************************************************
DLT_required_vk_inst_extns is a list of required Vulkan instance
extensions.
*/
extern const char* DLT_required_vk_inst_extns[];

/***********************************************************************
DLT_required_vk_device_extns is a list of required Vulkan device
extensions.
*/
extern const char* DLT_required_vk_dev_extns[];

/***********************************************************************
DLT_rect_size is the dimensions (in scaled pixels) of a rectangle.
*/
typedef struct DLT_rect_size {
    unsigned short x;
    unsigned short y;
} DLT_rect_size;

/***********************************************************************
DLT_sdl_vk_get_drawable_size gets the drawable size of an SDL
window. Pass it the SDL data field of the panel.
*/
DLT_rect_size DLT_sdl_vk_get_drawable_size(void*);

/***********************************************************************
DLT_sdl_vk_loadlib is a wrapper for SDL_VulkanLoadLibrary.
*/
DLT_error* DLT_sdl_vk_loadlib(DLT_context*);

/***********************************************************************
DLT_sdl_vk_get_inst_extns wraps SDL_Vulkan_GetInstanceExtensions. It
takes the panel data and a vector as arguments.
*/
DLT_error* DLT_sdl_vk_get_inst_extns(DLT_context*, DLT_vec*, void*);

/***********************************************************************
DLT_panel is the Light interface to the UI.  This might contain SDL
types, for example.  Its operators take some data argument which is
the object owned by the panel's implementation; for example, SDL has
a window, stored as DLT_sdl_panel_data.  This is done using a single
type per implementation which stores all the relevant values, which
the implementation methods know how to make use of.
*/
struct DLT_panel {
    void* data;

    DLT_vec (*get_reqd_inst_extns)(DLT_context*, DLT_panel);

    DLT_error* (*load_lib)(DLT_context*);

    void (*unload_lib)(void);

    VkSurfaceKHR (*create_sfc)(DLT_context*, VkInstance, void*);

    DLT_rect_size (*get_drawable_size)(void*);
};

/***********************************************************************
DLT_panel_create prepares the Light interface to the UI (the panel.)
*/
DLT_panel DLT_panel_create(DLT_context*, DLT_config*);

/***********************************************************************
DLT_panel_teardown tears down the Light interface to the UI (the
panel.)
*/
void DLT_panel_teardown(DLT_context*, DLT_panel, DLT_config*);

/***********************************************************************
DLT_sdl_panel_data is a DLT_panel data type for use by the SDL
implementation of panel.
*/
typedef struct DLT_sdl_panel_data {
    SDL_Window* window;
} DLT_sdl_panel_data;

/***********************************************************************
DLT_sdl_panel_create is a panel builder used in the SDL config.
*/
DLT_panel DLT_sdl_panel_create(DLT_context* ctx, DLT_config* cfg);

/***********************************************************************
DLT_sdl_panel_teardown is a panel teardown used in the SDL config.
 */
void DLT_sdl_panel_teardown(DLT_context*, DLT_panel);

/***********************************************************************
DLT_sdl_create_sfc is a VkInstance builder for DLT_panel.
*/
VkSurfaceKHR
DLT_sdl_create_sfc(DLT_context*, VkInstance inst, void* data);

/***********************************************************************
DLT_light is a container for the resources needed by Light, such as
surface management (e.g. through SDL), Vulkan resources, and so on.
It also takes ownership of a DLT_context and may use it to allocate
memory for temporary resources, for example.
*/
typedef struct DLT_light {
    LT_light     base;
    DLT_context* parent;

    DLT_vulkan vulkan;
    DLT_panel  panel;
} DLT_light;

/***********************************************************************
DLT_light_create creates a DLT_light object using the given context
and config.
*/
DLT_light* DLT_light_create(DLT_context*, DLT_config*);

/***********************************************************************
DLT_vulkan_create prepares a Vulkan pipeline for the given Light
object using the given DLT_config.
*/
DLT_vulkan
DLT_vulkan_create(DLT_context*, DLT_light*, DLT_config*, DLT_panel);

/***********************************************************************
DLT_vulkan_teardown
*/
void DLT_vulkan_teardown(DLT_context*,
                         DLT_config*,
                         DLT_panel,
                         DLT_vulkan);

/***********************************************************************
DLT_light_teardown deinitializes all the components of a DLT_light
cleanly.
 */
void DLT_light_teardown(DLT_context*, DLT_light*);

/***********************************************************************
DLT_light_config gets the config from the given Light object.
*/
DLT_config* DLT_light_config(DLT_light*);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* DR_LIGHT_H */