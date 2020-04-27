#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL.h"
#include "SDL_vulkan.h"

#define VOLK_IMPLEMENTATION
#include "volk/volk.h"

#include "light.h"
#include "light_dr.h"

#define assertm(exp, msg) assert(((void)(msg), (exp)))

#define DLT_STATIC_VEC(name, items, item_size)                         \
    const DLT_vec name = {                                             \
	item_size,                                                     \
	sizeof(items) / item_size,                                     \
	sizeof(items) / item_size,                                     \
	items,                                                         \
    }

#define DLT_VEC_EACH(name, iter, temp, type, body)                     \
    for (iter = 0; iter < name.count; iter++) {                        \
	temp = (type);                                                 \
	name.items[iter * (sizeof(void*) / sizeof(type))];             \
	body;                                                          \
    }

#define DLT_ERRCHECK(ctx, msg, next)                                   \
    if (((DLT_context*)(ctx))->base.error != NULL) {                   \
	DLT_error_push((DLT_context*)(ctx), msg);                      \
	next;                                                          \
    }

#define DLT_VK_CHECK(ctx, vr, msg, next)                               \
    if (vr != VK_SUCCESS) {                                            \
	DLT_error_push(ctx, msg, vk_result_name(vr));                  \
	next;                                                          \
    }

#define DLT_VK_APPLY(ctx, vr, fn, msg, next)                           \
    (vr) = (fn);                                                       \
    DLT_VK_CHECK(ctx, vr, msg, next);

/***********************************************************************
 */
const char* const LT_VERSION        = "Light Engine v0.0.1";
const int         LT_VERSION_NUMBER = 0;
const int         LT_VK_API_VERSION = VK_API_VERSION_1_2;

/***********************************************************************
Vulkan and SDL constants for extensions and layers, specialized by
NDEBUG.
*/

const char* DLT_required_vk_dev_extns[] = {
    VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
    VK_KHR_DEVICE_GROUP_EXTENSION_NAME,
    VK_KHR_MAINTENANCE2_EXTENSION_NAME,
    VK_KHR_MULTIVIEW_EXTENSION_NAME,
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

#ifdef NDEBUG
const SDL_WindowFlags DLT_SDL_DEFAULT_WINDOW_FLAGS =
  SDL_WINDOW_FULLSCREEN | SDL_WINDOW_VULKAN;

/*
const char* DLT_default_vk_layers[] = NULL;

NOTE: If adding an item to DLT_default_vk_layers, revise this. */
const DLT_vec DLT_DEFAULT_VK_LAYERS = {
    sizeof(const char*),
    0,
    0,
    NULL,
};

const char* DLT_required_vk_inst_extns[] = {
    VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
    VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME,
};
#else
const SDL_WindowFlags DLT_SDL_DEFAULT_WINDOW_FLAGS = SDL_WINDOW_VULKAN;

const char* DLT_default_vk_layers[] = {
    "VK_LAYER_KHRONOS_validation",
};

DLT_STATIC_VEC(DLT_DEFAULT_VK_LAYERS,
               DLT_default_vk_layers,
               sizeof(const char*));

const char* DLT_required_vk_inst_extns[] = {
    VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
    VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME,
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
};
#endif /* NDEBUG*/

/***********************************************************************
Static DLT_vecs simply point to their referent, and don't require any
memory instantiation, but don't try to expand or manipulate them!
*/

DLT_STATIC_VEC(DLT_REQUIRED_VK_INST_EXTNS,
               DLT_required_vk_inst_extns,
               sizeof(const char*));

DLT_STATIC_VEC(DLT_REQUIRED_VK_DEVICE_EXTNS,
               DLT_required_vk_dev_extns,
               sizeof(const char*));

/***********************************************************************
 */
#define NAME_CASE(name)                                                \
    case name:                                                         \
	return #name

const char* vk_result_name(VkResult r) {
    switch (r) {
	NAME_CASE(VK_SUCCESS);
	NAME_CASE(VK_NOT_READY);
	NAME_CASE(VK_TIMEOUT);
	NAME_CASE(VK_EVENT_SET);
	NAME_CASE(VK_EVENT_RESET);
	NAME_CASE(VK_INCOMPLETE);
	NAME_CASE(VK_ERROR_OUT_OF_HOST_MEMORY);
	NAME_CASE(VK_ERROR_OUT_OF_DEVICE_MEMORY);
	NAME_CASE(VK_ERROR_INITIALIZATION_FAILED);
	NAME_CASE(VK_ERROR_DEVICE_LOST);
	NAME_CASE(VK_ERROR_MEMORY_MAP_FAILED);
	NAME_CASE(VK_ERROR_LAYER_NOT_PRESENT);
	NAME_CASE(VK_ERROR_EXTENSION_NOT_PRESENT);
	NAME_CASE(VK_ERROR_FEATURE_NOT_PRESENT);
	NAME_CASE(VK_ERROR_INCOMPATIBLE_DRIVER);
	NAME_CASE(VK_ERROR_TOO_MANY_OBJECTS);
	NAME_CASE(VK_ERROR_FORMAT_NOT_SUPPORTED);
	NAME_CASE(VK_ERROR_FRAGMENTED_POOL);
	NAME_CASE(VK_ERROR_OUT_OF_POOL_MEMORY);
	/* and VK_ERROR_OUT_OF_POOL_MEMORY_KHR */
	NAME_CASE(VK_ERROR_INVALID_EXTERNAL_HANDLE);
	/* and VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR */
	NAME_CASE(VK_ERROR_SURFACE_LOST_KHR);
	NAME_CASE(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR);
	NAME_CASE(VK_SUBOPTIMAL_KHR);
	NAME_CASE(VK_ERROR_OUT_OF_DATE_KHR);
	NAME_CASE(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR);
	NAME_CASE(VK_ERROR_VALIDATION_FAILED_EXT);
	NAME_CASE(VK_ERROR_INVALID_SHADER_NV);
	NAME_CASE(
	  VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT);
	NAME_CASE(VK_ERROR_FRAGMENTATION_EXT);
	NAME_CASE(VK_ERROR_NOT_PERMITTED_EXT);
    default:
	return "unknown VkResult";
    }
}

const char* DLT_level_name(DLT_log_level lv) {
    switch (lv) {
	NAME_CASE(DLT_LEVEL_DEBUG);
	NAME_CASE(DLT_LEVEL_INFO);
	NAME_CASE(DLT_LEVEL_WARN);
	NAME_CASE(DLT_LEVEL_ERROR);
	NAME_CASE(DLT_LEVEL_FATAL);
    default:
	return "unknown DLT_log_level";
    }
}

/***********************************************************************
 */
const int DLT_default_vec_size = 4;

/***********************************************************************
 */
DLT_error DLT_error_create() {
    DLT_error result;

    result.err_offset = 0;
    result.str_offset = 0;
    result.why        = NULL;

    return result;
}

/***********************************************************************
 */
DLT_error* set_error(DLT_context* ctx) {
    return ctx->base.error = &(ctx->error);
}

/***********************************************************************
 */
DLT_error* DLT_error_push(DLT_context* ctx, const char* fmt, ...) {
    DLT_error* err;
    size_t     soffs;
    size_t     eoffs;
    size_t     smax;
    char*      data;

    /* printed is the amount by which str_offset will be increased. */
    size_t printed;

    va_list vl;

    assertm(ctx != NULL, "DLT_error_push got NULL context!");
    assertm(fmt != NULL, "DLT_error_push got NULL fmt!");

    err   = &(ctx->error);
    soffs = err->str_offset;
    eoffs = err->err_offset;
    smax  = DLT_ERROR_MEM_SIZE - soffs;
    data  = err->data + soffs;

    va_start(vl, fmt);

    /* Before trying this, check for error conditions. */
    if (soffs >= DLT_ERROR_MEM_SIZE) {
	va_end(vl);
	ctx->error.why = "DLT_error_push found no room for message";
	return set_error(ctx);
    } else if (eoffs >= DLT_ERROR_STACK_MAX) {
	va_end(vl);
	ctx->error.why = "DLT_error_push found no room for more errors";
	return set_error(ctx);
    }

    /* There's room, go ahead. */
    printed = vsnprintf(data, smax, fmt, vl);
    va_end(vl);

    if (printed < 0) {
	/* A vsnprintf error occurred. */
	ctx->error.why = strerror(errno);
	return set_error(ctx);
    }

    /* Increment str_offset by printed; if insufficient buffer, doesn't
     * matter because next time it will fail anyway.
     */
    ctx->error.errs[eoffs] = data;

    /* Set the error cause to the head. */
    ctx->error.why = ctx->error.errs[eoffs];

    /* Point to the next ready error slot. */
    ctx->error.err_offset += 1;

    /* snprintf doesn't count the NULL; we want to. */
    ctx->error.str_offset += printed + 1;

    /* Point the base error to the error value. */
    set_error(ctx);

    return NULL;
}

/***********************************************************************
 */
int DLT_log_errf(DLT_log_level lvl, const char* msg, ...) {
    int result;

    va_list vl;
    va_start(vl, msg);
    result = DLT_log_verrf(lvl, msg, vl);
    va_end(vl);

    return result;
}

/***********************************************************************
 */
int DLT_log_verrf(DLT_log_level level, const char* fmt, va_list vl) {
    assertm(fmt != NULL, "DLT_log_verrf got NULL fmt!");
    /* TODO: Macro-based leveled logging...? */

    if (_LIGHT_LOG_LEVEL > level) {
	return 0;
    }

    return vfprintf(stderr, fmt, vl);
}

/***********************************************************************
 */
DLT_vec DLT_vec_create(DLT_context* ctx,
                       size_t       size,
                       void*        root,
                       unsigned int count) {
    return DLT_vec_create_cap(ctx, size, root, count, count);
}

/***********************************************************************
 */
DLT_vec DLT_vec_create_cap(DLT_context* ctx,
                           size_t       size,
                           void*        root,
                           unsigned int count,
                           unsigned int cap) {

    /* TODO: set error */
    DLT_vec v;

    assertm(ctx != NULL, "DLT_vec_create_cap got NULL context!");
    assertm(cap >= count,
            "DLT_vec_create_cap got cap less than count!");

    if (root != NULL) {
	assertm(count > 0,
	        "DLT_vec_create_cap given a root must not have a zero "
	        "count!");
	assertm(cap >= count,
	        "DLT_vec_create_cap given a root must have capacity >= "
	        "to count!");
    }

    /* If it wasn't set or was zero, just use the count. */
    if (cap == 0) {
	cap = count;
    }

    /*
    If they asked for a zero-sized vec, they will very likely
    want more.
    */
    if (cap == 0) {
	cap = DLT_default_vec_size;
    }

    if (root == NULL) {
	root = DLT_context_alloc(ctx, (size * cap));
    }

    v.size     = size;
    v.count    = count;
    v.capacity = cap;
    v.items    = root;
    return v;
}

/***********************************************************************
 */
DLT_vec DLT_vec_create_strs(DLT_context* ctx, unsigned int num) {
    return DLT_vec_create(ctx, sizeof(const char*), NULL, num);
}

/***********************************************************************
 */
DLT_slice
DLT_vec_slice(DLT_vec v, unsigned int begin, unsigned int end) {
    DLT_slice result;
    size_t    size;

    assertm(begin <= end, "DLT_vec_slice begin after end!");
    assertm(end < v.count, "DLT_vec_slice end past count!");

    size            = v.size;
    result.size     = size;
    result.count    = end - begin;
    result.capacity = v.capacity - begin;
    result.items    = (void*)((ptrdiff_t)(v.items) +
                           (ptrdiff_t)(begin * size));

    return result;
}

/***********************************************************************
 */
DLT_slice
DLT_slice_slice(DLT_slice v, unsigned int begin, unsigned int end) {
    DLT_vec target = v;
    return DLT_vec_slice(target, begin, end);
}

/***********************************************************************
 */
DLT_vec DLT_vec_append(DLT_context* ctx, DLT_vec v, void** item) {
    unsigned int exp_size = (unsigned int)((double)v.capacity * 1.5);

    assertm(ctx != NULL, "DLT_vec_append got NULL context!");

    /* In case of insufficient capacity, expand by 1.5. */
    if (v.count == v.capacity) {
	v = DLT_vec_expand(ctx, v, exp_size);
	DLT_ERRCHECK(ctx, "expanding vec", return v);
    }

    /* This cast makes sure we're incrementing the pointer correctly for
     * the size of the vec's items.  Remember, item is a pointer to the
     * real item, whatever its size (whether the real item value is an
       address or not), so just use it directly as the memcpy source. */
    memcpy(((char*)(v.items) + v.size * v.count), item, v.size);
    v.count++;

    return v;
}

/***********************************************************************
DLT_vec_expand increases the capacity of the vec, if necessary, but not
its count.
 */
DLT_vec DLT_vec_expand(DLT_context* ctx, DLT_vec v, unsigned int cap) {
    DLT_vec result;

    assertm(ctx != NULL, "DLT_vec_expand got NULL context!");

    if (cap <= v.capacity) {
	/* No-op if it already has that much room. */
	return v;
    }

    result = DLT_vec_create_cap(ctx, v.size, NULL, v.count, cap);
    DLT_ERRCHECK(ctx, "failed to create expanded vec", return result);

    DLT_vec_copy(result, v);
    DLT_vec_release(ctx, v);

    return result;
}

/***********************************************************************
DLT_vec_resize alters the count of the vec, expanding its capacity if
necessary.
 */
DLT_vec DLT_vec_resize(DLT_context* ctx, DLT_vec v, unsigned int ct) {
    v = DLT_vec_expand(ctx, v, ct);
    DLT_ERRCHECK(ctx, "failed to expand vec", return v);

    v.count = ct;
    return v;
}

/***********************************************************************
 */
DLT_vec DLT_vec_copy(DLT_vec target, DLT_vec src) {
    assertm(src.size == target.size,
            "DLT_vec_copy got mismatched element size!");

    memcpy(target.items, src.items, src.size * src.count);
    return target;
}

/***********************************************************************
 */
DLT_vec DLT_vec_clone(DLT_context* ctx, DLT_vec from) {
    DLT_vec target;

    assertm(ctx != NULL, "DLT_vec_clone got NULL context!");

    target = DLT_vec_create(ctx, from.size, NULL, from.count);
    DLT_ERRCHECK(ctx, "creating vec for clone", return target);

    return DLT_vec_copy(target, from);
}

/***********************************************************************
 */
DLT_vec DLT_vec_sort(DLT_vec target, int (*sorter)(void*, void*)) {
    assertm(sorter != NULL, "DLT_vec_sort got NULL sorter!");
    qsort(target.items, target.count, target.size, sorter);

    return target;
}

/***********************************************************************
strcmp can't be used directly with qsort since qsort passes addresses of
its target to its sorter.  Deref first, then strcmp.
https://linux.die.net/man/3/qsort
 */
int DLT_vec_sort_strings(void* str1, void* str2) {
    return strcmp(*(const char* const*)str1, *(const char* const*)str2);
}

/***********************************************************************
 */
void DLT_vec_release(DLT_context* ctx, DLT_vec v) {
    assertm(ctx != NULL, "DLT_vec_release got NULL context!");
    DLT_context_free(ctx, v.items);
}

/***********************************************************************
 */
LT_context* LT_context_create(LT_context* ctx) {
    return (LT_context*)DLT_context_create((DLT_context*)ctx);
}

/***********************************************************************
 */
DLT_context* DLT_context_create_child(DLT_context* ctx) {
    DLT_context* c;

    assertm(ctx != NULL, "DLT_context_create_child got NULL context!");

    c = DLT_context_alloc(ctx, sizeof(DLT_context));
    DLT_ERRCHECK(ctx, "context child alloc failed", return NULL);

    memcpy(c, ctx, sizeof(DLT_context));
    c->parent = ctx;

    /* TODO: Reset resource counters? */

    return c;
}

/***********************************************************************
 */
DLT_context* DLT_context_create_fresh() {
    LT_context  base = { 0 };
    DLT_context tmp;
    tmp.base   = base;
    tmp.parent = NULL;
    tmp.dalloc = malloc;
    tmp.dfree  = free;
    tmp.logger = DLT_log_verrf;

    DLT_context* result = DLT_context_alloc(&tmp, sizeof(DLT_context));
    if (tmp.base.error != NULL) {
	LT_log_error(&(tmp.base),
	             "fresh context malloc failed",
	             tmp.base.error);
	return NULL;
    }

    return memcpy(result, &tmp, sizeof(DLT_context));
}

/***********************************************************************
 */
DLT_context* DLT_context_create(DLT_context* ctx) {
    /* TODO: set error. */
    DLT_context* res;
    DLT_error    err = DLT_error_create();

    if (ctx != NULL && (res = DLT_context_create_child(ctx)) == NULL) {
	/* Use the parent context; "inherit" its fields. */
	DLT_error_push(ctx, "failed to create child context");
	return NULL;
    } else if ((res = DLT_context_create_fresh()) == NULL) {
	/* Use defaults for a fresh context. */
	DLT_log_errf(DLT_LEVEL_ERROR,
	             "failed to create fresh context: %s",
	             strerror(errno));
	return NULL;
    }

    /* Reset the error and cleanups in either case. */
    res->error      = err;
    res->base.error = NULL;

    /* After this point the context has been allocated, so we'll need to
     * purge it on failure....
     */
    res->cleanup = DLT_vec_create(res, sizeof(DLT_cleanup), NULL, 0);
    if (res->base.error != NULL) {
	DLT_context_purge(res);
	return NULL;
    }

    res->cleanup_data = DLT_vec_create(res, sizeof(void*), NULL, 0);
    if (res->base.error != NULL) {
	DLT_context_purge(res);
	return NULL;
    }

    return res;
}

/***********************************************************************
TODO: implement me!
 */
DLT_error* DLT_context_cleanup_push(DLT_context* ctx,
                                    DLT_cleanup  cln,
                                    void*        data) {
    return NULL;
}

/***********************************************************************
 */
void* DLT_context_alloc(DLT_context* ctx, size_t mem) {
    void* result;

    assertm(ctx != NULL, "DLT_context_alloc got NULL context!");

    if ((result = ctx->dalloc(mem)) == NULL) {
	DLT_error_push(ctx, "failed to allocate: %s", strerror(errno));
	return NULL;
    }

    return result;
}

/***********************************************************************
 */
void DLT_context_free(DLT_context* ctx, void* mem) {
    assertm(ctx != NULL, "DLT_context_free got NULL context!");
    ctx->dfree(mem);
}

/***********************************************************************
 */
void LT_context_purge(LT_context* ctx) {
    DLT_context_purge((DLT_context*)ctx);
}

/***********************************************************************
 * TODO: Flesh this out.  Particularly, run and free cleanups.  Also be
 *       thoughtful about error conditions: base error, parent error,
 *       etc. since they may model "undefined states".
 */
void DLT_context_purge(DLT_context* ctx) {
    assertm(ctx != NULL, "DLT_context_purge got NULL context!");

    DLT_context* use = ctx;
    if (ctx->parent != NULL) {
	use = ctx->parent;
    }

    DLT_vec_release(ctx, ctx->cleanup);
    DLT_vec_release(ctx, ctx->cleanup_data);

    DLT_context_free(use, ctx);
}

/***********************************************************************
 */
void LT_log_info(LT_context* ctx, const char* msg, ...) {
    va_list vl;
    va_start(vl, msg);
    DLT_vlog((DLT_context*)ctx, DLT_LEVEL_INFO, msg, vl);
    va_end(vl);
}

/***********************************************************************
 */
void LT_log_warning(LT_context* ctx, const char* msg, ...) {
    va_list vl;
    va_start(vl, msg);
    DLT_vlog((DLT_context*)ctx, DLT_LEVEL_WARN, msg, vl);
    va_end(vl);
}

/***********************************************************************
 */
void LT_log_error(LT_context* ctx, const char* msg, DLT_error* err) {
    size_t       i;
    const char*  next = err->why;
    DLT_context* dctx = (DLT_context*)ctx;

    DLT_log(dctx, DLT_LEVEL_ERROR, "Light error: %s: %s\n", msg, next);

    for (i = err->err_offset; i > 0; i--) {
	next = err->errs[i - 1];
	DLT_log(dctx, DLT_LEVEL_ERROR, "  (%02u) %s\n", i, next);
    }
}

/***********************************************************************
 */
int DLT_log(DLT_context* ctx, DLT_log_level lv, const char* fmt, ...) {
    int result;

    va_list vl;
    va_start(vl, fmt);
    result = DLT_vlog(ctx, lv, fmt, vl);
    va_end(vl);

    return result;
}

/***********************************************************************
 */
int DLT_vlog(DLT_context*  ctx,
             DLT_log_level lv,
             const char*   fmt,
             va_list       vl) {

    int result;

    assertm(ctx != NULL, "DLT_vlog got NULL context!");
    assertm(fmt != NULL, "DLT_vlog got NULL msg!");

    if ((result = ctx->logger(lv, fmt, vl)) < 0) {
	DLT_error_push(ctx,
	               "DLT_vlog failed to write \"%s\": %s",
	               fmt,
	               strerror(errno));
	return result;
    }

    return result;
}

/***********************************************************************
 */
void LT_exit(LT_context* ctx) { exit(EXIT_SUCCESS); }

/***********************************************************************
 */
void LT_bail(LT_context* ctx, const char* reason) {
    assertm(ctx != NULL, "LT_bail got NULL context!");
    DLT_log((DLT_context*)ctx, DLT_LEVEL_FATAL, reason);
    LT_context_purge(ctx);
    exit(EXIT_FAILURE);
}

/***********************************************************************
 */
DLT_rect_size DLT_sdl_vk_get_drawable_size(void* d) {
    assertm(d != NULL, "DLT_sdl_vk_get_drawable_size got NULL data!");

    DLT_sdl_panel_data* sd = (DLT_sdl_panel_data*)d;
    DLT_rect_size       result;

    int w, h;

    SDL_Vulkan_GetDrawableSize(sd->window, &w, &h);

    result.x = w;
    result.y = h;

    return result;
}

/***********************************************************************
 */
SDL_WindowFlags default_sdl_flags() {
    return DLT_SDL_DEFAULT_WINDOW_FLAGS;
}

/***********************************************************************
 */
LT_config* LT_config_sdl(LT_context* ctx, const char* name) {
    DLT_context*        dctx;
    DLT_config*         cfg;
    DLT_sdl_config_data scfg;

    assertm(ctx != NULL, "LT_config_sdl got NULL context!");
    assertm(name != NULL, "LT_config_sdl got NULL name!");

    dctx       = (DLT_context*)ctx;
    scfg.flags = default_sdl_flags();

    cfg = DLT_config_create(dctx, name, sizeof(scfg));
    DLT_ERRCHECK(ctx, "failed to create config", return NULL);

    *((DLT_sdl_config_data*)cfg->data) = scfg;
    cfg->build_panel                   = DLT_sdl_panel_create;
    cfg->teardown_panel                = DLT_sdl_panel_teardown;

    return (LT_config*)cfg;
}

DLT_error*
load_shader(DLT_context* ctx, DLT_shader* shader, const char* name) {
    return NULL;
}

/***********************************************************************
 */
DLT_config*
DLT_config_create(DLT_context* ctx, const char* name, size_t size) {
    DLT_config* cfg;

    /* TODO: User-defined pipeline configurator. */
    DLT_vulkan_pipeline_config vkcfg = { load_shader };

    assertm(ctx != NULL, "DLT_config_create got NULL context!");
    assertm(name != NULL, "DLT_config_create got NULL name!");

    if ((cfg = DLT_context_alloc(ctx, sizeof(DLT_config))) == NULL) {
	DLT_error_push(ctx, "failed to allocate config memory");
	return NULL;
    }

    memset(cfg, 0, sizeof(DLT_config));

    cfg->data = DLT_context_alloc((DLT_context*)ctx, size);
    if (ctx->base.error != NULL) {
	DLT_error_push(ctx,
	               "failed to allocate %ul bytes of config data",
	               size);
	DLT_context_free(ctx, cfg);
	return NULL;
    }

    cfg->base.name = name;

#ifndef NDEBUG
    cfg->debugging = 1;
#endif /* NDEBUG */

    cfg->pipeline_cfg = vkcfg;

    return cfg;
}

/***********************************************************************
 */
void DLT_config_destroy(DLT_context* ctx, DLT_config* cfg) {
    assertm(ctx != NULL, "DLT_config_destroy got NULL context!");
    DLT_context_free(ctx, cfg->data);
    DLT_context_free(ctx, cfg);
}

/***********************************************************************
 */
DLT_error* DLT_volk_vk_loadlib(DLT_context* ctx) {
    VkResult    r;
    const char* s;

    if ((r = volkInitialize()) != VK_SUCCESS) {
	s = vk_result_name(r);
	return DLT_error_push(ctx, "Volk failed to load Vulkan: %s", s);
    }

    return NULL;
}

/***********************************************************************
 */
void DLT_volk_vk_unloadlib(void) {}

/***********************************************************************
 */
DLT_error* DLT_sdl_vk_loadlib(DLT_context* ctx) {
    assertm(ctx != NULL, "DLT_sdl_vk_loadlib got NULL context!");

    if (SDL_Vulkan_LoadLibrary(NULL) != 0) {
	return DLT_error_push(ctx,
	                      "SDL failed to load Vulkan: %s",
	                      SDL_GetError());
    }

    return NULL;
}

/***********************************************************************
 */
DLT_vec DLT_sdl_vk_reqd_inst_extns(DLT_context* ctx, DLT_panel pn) {
    DLT_vec             vec = { 0 };
    DLT_sdl_panel_data* data;
    SDL_Window*         window;
    SDL_bool            ok;
    unsigned int        num;

    assertm(ctx != NULL, "DLT_sdl_vk_get_inst_extns got NULL context!");
    assertm(pn.data != NULL,
            "DLT_sdl_vk_get_inst_extns got NULL panel data!");

    data   = pn.data;
    window = data->window;

    /* First get the required count. */
    ok = SDL_Vulkan_GetInstanceExtensions(window, &num, NULL);
    if (ok != SDL_TRUE) {
	DLT_error_push(ctx,
	               "SDL failed to get count of required Vulkan "
	               "instance extensions: %s",
	               SDL_GetError());
	return vec;
    }

    /* Now get the actual extension names. */
    vec = DLT_vec_create_strs(ctx, num);
    DLT_ERRCHECK(ctx, "creating required extn vec", return vec);

    ok = SDL_Vulkan_GetInstanceExtensions(window,
                                          &num,
                                          (const char**)(vec.items));
    if (ok != SDL_TRUE) {
	DLT_vec_release(ctx, vec);
	DLT_error_push(ctx,
	               "SDL failed to get count of required Vulkan "
	               "instance extensions: %s",
	               SDL_GetError());
	return vec;
    }

    return vec;
}

/***********************************************************************
 */
DLT_panel DLT_sdl_panel_create(DLT_context* ctx, DLT_config* cfg) {
    DLT_panel            result = { 0 };
    DLT_sdl_config_data* cfd;
    DLT_sdl_panel_data*  pnd;
    SDL_Window*          w;
    const char*          err;

    assertm(ctx != NULL, "DLT_sdl_panel_create got NULL context!");
    assertm(cfg != NULL, "DLT_sdl_panel_create got NULL config!");
    assertm(cfg->data != NULL, "DLT_sdl_panel_create got NULL data!");

    cfd = (DLT_sdl_config_data*)cfg->data;

    pnd = DLT_context_alloc(ctx, sizeof(DLT_sdl_panel_data));
    if (ctx->base.error != NULL) {
	DLT_error_push(ctx, "failed to allocate SDL panel data");
	return result;
    }

    w = SDL_CreateWindow(cfg->base.name, 0, 0, 1024, 768, cfd->flags);
    if (w == NULL) {
	DLT_context_free(ctx, pnd);

	err = SDL_GetError();
	DLT_error_push(ctx, "SDL_CreateWindow failed: %s", err);
	return result;
    }

    pnd->window = w;

    result.load_lib   = DLT_volk_vk_loadlib;
    result.unload_lib = DLT_volk_vk_unloadlib;

    /* Keep this around for now in case Volk is weak */ /*
    result.load_lib   = DLT_sdl_vk_loadlib;
    result.unload_lib = SDL_Vulkan_UnloadLibrary;
    */

    result.data                = pnd;
    result.create_sfc          = DLT_sdl_create_sfc;
    result.get_drawable_size   = DLT_sdl_vk_get_drawable_size;
    result.get_reqd_inst_extns = DLT_sdl_vk_reqd_inst_extns;

    return result;
}

/***********************************************************************
 */
void DLT_sdl_panel_teardown(DLT_context* ctx, DLT_panel panel) {
    assertm(ctx != NULL, "DLT_sdl_panel_teardown got NULL context!");
    assertm(panel.data != NULL,
            "DLT_sdl_panel_teardown got NULL panel data!");

    assertm(((DLT_sdl_panel_data*)panel.data)->window != NULL,
            "DLT_sdl_panel_teardown got NULL window!");

    SDL_DestroyWindow(((DLT_sdl_panel_data*)panel.data)->window);
}

/***********************************************************************
 */
VkSurfaceKHR
DLT_sdl_create_sfc(DLT_context* ctx, VkInstance inst, void* data) {
    VkSurfaceKHR        sfc;
    DLT_sdl_panel_data* sdata;
    SDL_bool            ok;
    const char*         err;

    assertm(ctx != NULL, "DLT_sdl_create_sfc got NULL context!");
    assertm(inst != NULL, "DLT_sdl_create_sfc got NULL VkInstance!");
    assertm(data != NULL, "DLT_sdl_create_sfc got NULL data!");

    sdata = (DLT_sdl_panel_data*)data;

    ok = SDL_Vulkan_CreateSurface(sdata->window, inst, &sfc);
    if (ok != SDL_TRUE) {
	err = SDL_GetError();
	DLT_error_push(ctx, "SDL failed to create VK surface: %s", err);
	return NULL;
    }

    return sfc;
}

/***********************************************************************
 */
DLT_queue_indices DLT_queue_indices_create(DLT_context* ctx) {
    DLT_queue_indices result;

    result.gfx = DLT_vec_create(ctx, sizeof(uint32_t), NULL, 0);
    DLT_ERRCHECK(ctx, "creating gfx queue list", return result);

    result.compute = DLT_vec_create(ctx, sizeof(uint32_t), NULL, 0);
    DLT_ERRCHECK(ctx, "creating compute queue list", goto rl_gfx);

    result.pres = DLT_vec_create(ctx, sizeof(uint32_t), NULL, 0);
    DLT_ERRCHECK(ctx, "creating presentation queue list", goto rl_comp);

    return result;

rl_comp:
    DLT_vec_release(ctx, result.compute);

rl_gfx:
    DLT_vec_release(ctx, result.gfx);

    return result;
}

/***********************************************************************
 */
void DLT_queue_indices_release(DLT_context* ctx, DLT_queue_indices q) {
    DLT_vec_release(ctx, q.gfx);
    DLT_vec_release(ctx, q.compute);
    DLT_vec_release(ctx, q.pres);
}

/***********************************************************************
 */
DLT_queue_indices DLT_queue_indices_clone(DLT_context*      ctx,
                                          DLT_queue_indices given) {
    DLT_queue_indices result;

    result.gfx = DLT_vec_clone(ctx, given.gfx);
    DLT_ERRCHECK(ctx, "failed to clone gfx", return result);

    result.compute = DLT_vec_clone(ctx, given.compute);
    DLT_ERRCHECK(ctx, "failed to clone compute", goto rl_gfx);

    result.pres = DLT_vec_clone(ctx, given.pres);
    DLT_ERRCHECK(ctx, "failed to clone pres", goto rl_comp);

    return result;

rl_comp:
    DLT_vec_release(ctx, result.compute);

rl_gfx:
    DLT_vec_release(ctx, result.gfx);

    return result;
}

/***********************************************************************
 */
DLT_queue_indices DLT_queue_indices_reset(DLT_queue_indices of) {
    DLT_queue_indices result = of;
    result.compute.count     = 0;
    result.gfx.count         = 0;
    result.pres.count        = 0;
    return result;
}

/***********************************************************************
 */
typedef struct vk_swapchain_caps {
    VkSurfaceCapabilities2KHR sfc_capabilities;
    DLT_vec                   sfc_formats;
    DLT_vec                   present_modes;
} vk_swapchain_caps;

/***********************************************************************
 */
vk_swapchain_caps vk_swapchain_caps_create(DLT_context* ctx,
                                           DLT_vulkan   vk) {
    vk_swapchain_caps swc_caps = { 0 };

    DLT_vec fmts;
    DLT_vec modes;

    size_t ssf = sizeof(VkSurfaceFormat2KHR);
    size_t spm = sizeof(VkPresentModeKHR);

    size_t i;

    uint32_t num;

    VkResult r;

    VkPhysicalDevice pd  = vk.phys_device;
    VkDevice         d   = vk.device;
    VkSurfaceKHR     sfc = vk.surface;

    VkPhysicalDeviceSurfaceInfo2KHR info = {
	VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR,
	NULL,
	vk.surface,
    };

    VkSurfaceCapabilities2KHR caps = {
	VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR,
	NULL
    };

    assertm(ctx != NULL, "vk_swapchain_caps_create got NULL context!");

    /* TODO: fix exception */
    r = vkGetPhysicalDeviceSurfaceCapabilities2KHR(pd, &info, &caps);
    DLT_VK_CHECK(ctx,
                 r,
                 "loading surface capabilities: %s",
                 return swc_caps);

    fmts = DLT_vec_create(ctx, ssf, NULL, 0);
    DLT_ERRCHECK(ctx, "creating formats vec", return swc_caps);

    modes = DLT_vec_create(ctx, spm, NULL, 0);
    DLT_ERRCHECK(ctx, "creating modes vec", goto rl_fmts);

    r = vkGetPhysicalDeviceSurfaceFormats2KHR(pd, &info, &num, NULL);
    DLT_VK_CHECK(ctx, r, "counting surface formats: %s", goto rl_modes);

    fmts = DLT_vec_resize(ctx, fmts, num);
    DLT_ERRCHECK(ctx, "resizing formats vec", goto rl_modes);

    for (i = 0; i < num; i++) {
	((VkSurfaceFormat2KHR*)(fmts.items))[i]
	  .sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR;
	((VkSurfaceFormat2KHR*)(fmts.items))[i].pNext = NULL;
    }

    r = vkGetPhysicalDeviceSurfaceFormats2KHR(
      pd,
      &info,
      &num,
      ((VkSurfaceFormat2KHR*)(fmts.items)));
    DLT_VK_CHECK(ctx, r, "loading surface formats: %s", goto rl_modes);

    r = vkGetPhysicalDeviceSurfacePresentModesKHR(pd, sfc, &num, NULL);
    DLT_VK_CHECK(ctx, r, "counting present modes: %s", goto rl_modes);

    modes = DLT_vec_resize(ctx, modes, num);
    DLT_ERRCHECK(ctx, "resizing modes vec", goto rl_modes);

    r = vkGetPhysicalDeviceSurfacePresentModesKHR(
      pd,
      sfc,
      &num,
      ((VkPresentModeKHR*)(modes.items)));
    DLT_VK_CHECK(ctx, r, "loading present modes: %s", goto rl_modes);

    swc_caps.sfc_capabilities = caps;
    swc_caps.sfc_formats      = fmts;
    swc_caps.present_modes    = modes;

    return swc_caps;

rl_modes:
    DLT_vec_release(ctx, modes);

rl_fmts:
    DLT_vec_release(ctx, fmts);

    return swc_caps;
}

/***********************************************************************
 */
void vk_swapchain_caps_release(DLT_context* ctx, vk_swapchain_caps cp) {
    DLT_vec_release(ctx, cp.sfc_formats);
    DLT_vec_release(ctx, cp.present_modes);
}

/***********************************************************************
 */
typedef struct vk_swapchain_config {
    VkSwapchainCreateInfoKHR swc_config;

    VkSurfaceFormat2KHR fmt;
    VkExtent2D          extent;
} vk_swapchain_config;

/***********************************************************************
 */
vk_swapchain_config
vk_swapchain_config_create(DLT_context*      ctx,
                           DLT_vec*          qv,
                           vk_swapchain_caps caps,
                           DLT_rect_size     size,
                           DLT_vulkan        vk,
                           VkSwapchainKHR*   old_swc) {

    vk_swapchain_config      result;
    VkSwapchainCreateInfoKHR swcfg = { 0 };

    VkSurfaceFormat2KHR      fmt = { 0 };
    VkPresentModeKHR         mode;
    VkSharingMode            shmode;
    VkExtent2D               ext;
    VkSurfaceCapabilitiesKHR cap = caps.sfc_capabilities
                                     .surfaceCapabilities;

    DLT_queue_indices qi = vk.queues.indices;

    uint32_t h, w, mn, mx;

    size_t i;

    DLT_vec qiv = *qv;

    assertm(ctx != NULL, "vk_swapchain_config_from got NULL context!");

    swcfg.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;

    if (caps.sfc_formats.count == 1 &&
        ((VkSurfaceFormat2KHR*)(caps.sfc_formats.items))[0]
            .surfaceFormat.format == VK_FORMAT_UNDEFINED) {
	/* Only one format, "undefined." */
	fmt.surfaceFormat.format     = VK_FORMAT_B8G8R8A8_UNORM;
	fmt.surfaceFormat.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    } else {
	for (i = 0; i < caps.sfc_formats.count; i++) {
	    /* Look for the correct format. */
	    fmt = ((VkSurfaceFormat2KHR*)(caps.sfc_formats.items))[i];

	    if (fmt.surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
	        fmt.surfaceFormat.colorSpace ==
	          VK_COLORSPACE_SRGB_NONLINEAR_KHR) {

		goto found_fmt;
	    }
	}

	/* Fallback, just use whatever the first one was. */
	fmt = ((VkSurfaceFormat2KHR*)(caps.sfc_formats.items))[0];
    }

found_fmt:

    for (i = 0; i < caps.present_modes.count; i++) {
	mode = ((VkPresentModeKHR*)(caps.present_modes.items))[i];
	if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
	    /* Mailbox is supported, so use it. */
	    goto found_mode;
	}
    }
    mode = VK_PRESENT_MODE_FIFO_KHR;

found_mode:

    if (cap.maxImageExtent.width != 0xFFFFFFFF) {
	/* If not configurable, use the current extent. */
	ext = cap.currentExtent;
    } else {
	/* Otherwise, use the minimum of the drawable panel size and the
	 * maximum allowed extent. */
	h = cap.maxImageExtent.height;
	w = cap.maxImageExtent.width;

	ext.height = h < size.y ? h : size.y;
	ext.width  = w < size.x ? w : size.x;
    }

    result.extent = ext;
    result.fmt    = fmt;

    uint32_t gi = ((uint32_t*)(qi.gfx.items))[0];
    uint32_t pi = ((uint32_t*)(qi.pres.items))[0];

    if (gi == pi) {
	shmode = VK_SHARING_MODE_EXCLUSIVE;
	/* Leave the queue info vec empty. */
    } else {
	shmode = VK_SHARING_MODE_CONCURRENT;

	qiv = DLT_vec_resize(ctx, qiv, 2);
	DLT_ERRCHECK(ctx, "resizing queue index vec", return result);

	((uint32_t*)(qiv.items))[0] = gi;
	((uint32_t*)(qiv.items))[1] = pi;
    }

    mn = cap.minImageCount;
    mx = cap.maxImageCount;

    *qv = qiv;

    swcfg.flags            = VK_SWAPCHAIN_CREATE_PROTECTED_BIT_KHR;
    swcfg.surface          = vk.surface;
    swcfg.minImageCount    = mn == 0 ? mn + 1 : mn == mx ? mn : mn + 1;
    swcfg.imageFormat      = fmt.surfaceFormat.format;
    swcfg.imageColorSpace  = fmt.surfaceFormat.colorSpace;
    swcfg.imageExtent      = ext;
    swcfg.imageArrayLayers = 1; /* TODO: Support stereoscopic? */
    swcfg.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swcfg.imageSharingMode = shmode;
    swcfg.queueFamilyIndexCount = qiv.count;
    swcfg.pQueueFamilyIndices   = (const uint32_t*)(qiv.items);
    swcfg.preTransform          = cap.currentTransform;
    swcfg.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swcfg.presentMode           = mode;
    swcfg.clipped               = VK_TRUE;

    if (old_swc != NULL) {
	swcfg.oldSwapchain = *old_swc;
    }

    result.swc_config = swcfg;

    return result;
}

/***********************************************************************
  - Get drawable size
  - Create swapchain
  - Get swapchain images
  - Make "image views"
 */
DLT_vulkan_pipeline DLT_vulkan_pipeline_create(DLT_context* ctx,
                                               DLT_panel    pn,
                                               DLT_vulkan   vk) {

    DLT_vulkan_pipeline result = { 0 };
    DLT_rect_size       draw_size;
    DLT_vec             swcqv, ims, imvs;

    vk_swapchain_caps   swcaps;
    vk_swapchain_config swcfg;

    VkResult                r;
    VkSwapchainKHR          swc;
    VkImageViewCreateInfo   imv_cfg   = { 0 };
    VkComponentMapping      imv_comps = { 0 };
    VkImageSubresourceRange imv_range = { 0, 0, 1, 0, 1 };

    size_t   sim  = sizeof(VkImage);
    size_t   simv = sizeof(VkImageView);
    uint32_t num;

    size_t i;

    assertm(ctx != NULL, "DLT_vulkan_pipeline_create got NULL ctx!");

    imv_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    imv_cfg.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imv_cfg.components       = imv_comps;
    imv_cfg.subresourceRange = imv_range;
    imv_cfg.viewType         = VK_IMAGE_VIEW_TYPE_2D;

    swcqv = DLT_vec_create(ctx, sizeof(uint32_t), NULL, 0);
    DLT_ERRCHECK(ctx, "creating queue info vec", return result);

    /* Get the drawable panel size. */
    draw_size = pn.get_drawable_size(pn.data);
    if (draw_size.x == 0 || draw_size.y == 0) {
	DLT_error_push(ctx, "got 0-sized drawable");
	return result;
    }

    /* Read the swapchain capabilities. */
    swcaps = vk_swapchain_caps_create(ctx, vk);
    DLT_ERRCHECK(ctx, "loading swapchain caps", goto rl_swcqv);

    /* Given those capabilities and drawable size, get the preferred
     * swapchain configuration. */
    swcfg = vk_swapchain_config_create(ctx,
                                       &swcqv,
                                       swcaps,
                                       draw_size,
                                       vk,
                                       NULL);
    DLT_ERRCHECK(ctx, "creating swapchain config", goto rl_swci);

    r = vkCreateSwapchainKHR(vk.device,
                             &(swcfg.swc_config),
                             NULL,
                             &swc);
    DLT_VK_CHECK(ctx, r, "creating swapchain: %s", goto rl_swci);

    r = vkGetSwapchainImagesKHR(vk.device, swc, &num, NULL);
    DLT_VK_CHECK(ctx, r, "counting swapchain images: %s", goto dst_swc);

    ims = DLT_vec_create(ctx, sim, NULL, num);
    DLT_ERRCHECK(ctx, "creating swapchain image vec", goto dst_swc);

    r = vkGetSwapchainImagesKHR(vk.device,
                                swc,
                                &num,
                                (VkImage*)(ims.items));
    DLT_VK_CHECK(ctx, r, "loading swapchain images: %s", goto rl_ims);

    imvs = DLT_vec_create(ctx, simv, NULL, ims.count);
    DLT_ERRCHECK(ctx, "creating image view vec", goto rl_ims);

    imv_cfg.format = swcfg.fmt.surfaceFormat.format;

    for (i = 0; i < ims.count; i++) {
	imv_cfg.image = ((VkImage*)(ims.items))[i];

	r = vkCreateImageView(vk.device,
	                      &imv_cfg,
	                      NULL,
	                      &(((VkImageView*)(imvs.items))[i]));
	DLT_VK_CHECK(ctx, r, "creating image view: %s", goto dst_imvs);
    }

    result.swc_image_views = imvs;
    result.swc_images      = ims;
    result.swc             = swc;

    DLT_vec_release(ctx, swcqv);
    vk_swapchain_caps_release(ctx, swcaps);

    return result;

dst_imvs:
    /* Start where we failed and roll back each previous one. */
    for (; i > 0; i--) {
	vkDestroyImageView(vk.device,
	                   ((VkImageView*)(imvs.items))[i - 1],
	                   NULL);
    }

    DLT_vec_release(ctx, imvs);

    /*
dst_ims:
    Destroying the swapchain destroys its images.
    */

rl_ims:
    DLT_vec_release(ctx, ims);

dst_swc:
    vkDestroySwapchainKHR(vk.device, swc, NULL);

rl_swci:
    vk_swapchain_caps_release(ctx, swcaps);

rl_swcqv:
    DLT_vec_release(ctx, swcqv);

    return result;
}

/***********************************************************************
 */

/***********************************************************************
 */
LT_light* LT_light_create(LT_context* ctx, LT_config* cfg) {
    return (LT_light*)DLT_light_create((DLT_context*)ctx,
                                       (DLT_config*)cfg);
}

/***********************************************************************
 */
DLT_light* DLT_light_create(DLT_context* ctx, DLT_config* cfg) {
    DLT_light* lt;
    LT_light   base;

    assertm(ctx != NULL, "DLT_light_create got NULL context!");
    assertm(cfg != NULL, "DLT_light_create got NULL config!");

    /* Allocate Light */
    lt = DLT_context_alloc(ctx, sizeof(DLT_light));
    DLT_ERRCHECK(ctx, "allocating DLT_light", return NULL);

    base.config = (LT_config*)cfg;
    lt->parent  = ctx;
    lt->base    = base;

    {
	lt->panel = cfg->build_panel(ctx, cfg);
	DLT_ERRCHECK(ctx, "building panel", goto free_lt);
    }

    {
	lt->vulkan = DLT_vulkan_create(ctx, lt, cfg, lt->panel);
	DLT_ERRCHECK(ctx,
	             "creating Vulkan resources",
	             goto cleanup_panel);
    }

    {
	lt->vulkan.pipeline = DLT_vulkan_pipeline_create(ctx,
	                                                 lt->panel,
	                                                 lt->vulkan);
	DLT_ERRCHECK(ctx, "creating Vulkan pipeline", goto dst_vk);
    }

    return lt;

    /******************************************************************
     * Cleanups:
     */
dst_vk:
    DLT_vulkan_teardown(ctx, cfg, lt->panel, lt->vulkan);

cleanup_panel:
    DLT_panel_teardown(ctx, lt->panel, cfg);

free_lt:
    DLT_context_free(ctx, lt);
    return NULL;
}

/***********************************************************************
 */
void DLT_panel_teardown(DLT_context* ctx,
                        DLT_panel    pn,
                        DLT_config*  cfg) {

    assertm(ctx != NULL, "DLT_panel_teardown got NULL context!");
    assertm(cfg != NULL, "DLT_panel_teardown got NULL config!");

    cfg->teardown_panel(ctx, pn);
    DLT_context_free(ctx, pn.data);
}

/***********************************************************************
 */
VkApplicationInfo vk_app_info(DLT_config* cfg) {
    VkApplicationInfo result = { 0 };

    result.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    result.pApplicationName   = cfg->base.name;
    result.applicationVersion = cfg->base.version;
    result.pEngineName        = LT_VERSION;
    result.engineVersion      = LT_VERSION_NUMBER;
    result.apiVersion         = LT_VK_API_VERSION;

    return result;
}

/***********************************************************************
 */
VkInstanceCreateInfo vk_instance_config(DLT_config* cfg,
                                        DLT_vec     layers,
                                        DLT_vec     inst_extns) {

    VkInstanceCreateInfo result = { 0 };

    result.sType               = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    result.enabledLayerCount   = layers.count;
    result.ppEnabledLayerNames = (const char**)layers.items;
    result.enabledExtensionCount   = inst_extns.count;
    result.ppEnabledExtensionNames = (const char**)(inst_extns.items);

    return result;
}

/***********************************************************************
 */
VKAPI_ATTR VkBool32 VKAPI_CALL
DLT_debug_message(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                  VkDebugUtilsMessageTypeFlagsEXT        type_flags,
                  const VkDebugUtilsMessengerCallbackDataEXT* data,
                  void* user_data) {

    DLT_log_level is;
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
	is = DLT_LEVEL_DEBUG;
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
	/*
	is = DLT_LEVEL_INFO;
	*/
	is = DLT_LEVEL_DEBUG;
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	is = DLT_LEVEL_WARN;
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	is = DLT_LEVEL_ERROR;
    else
	is = DLT_LEVEL_WARN;

    DLT_log_errf(is,
                 "LT Vulkan [%s]: %s: %s\n",
                 DLT_level_name(is),
                 data->pMessageIdName,
                 data->pMessage);
    return VK_FALSE;
}

/***********************************************************************
 */
VkDebugUtilsMessengerCreateInfoEXT default_debug_config() {
    VkDebugUtilsMessengerCreateInfoEXT result = { 0 };

    result.pfnUserCallback = DLT_debug_message;
    result.messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    result
      .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    result
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

    return result;
}

/***********************************************************************
 */
int order_extps(void const* a, void const* b) {
    VkExtensionProperties const* ap = (VkExtensionProperties const*)a;
    VkExtensionProperties const* bp = (VkExtensionProperties const*)b;
    return strcmp(ap->extensionName, bp->extensionName);
}

/***********************************************************************
 */
DLT_vec prepare_inst_extns(DLT_context* ctx, DLT_panel pn) {
    DLT_vec spp_inst_ext, inst_extn;

    size_t sextp = sizeof(VkExtensionProperties);
    size_t sch   = sizeof(const char*);

    unsigned int num_items =
      (unsigned int)((float)(sizeof(DLT_required_vk_inst_extns)) /
                     (float)sch);

    const char **items, *tmp_name, *tmp_seeking;

    const VkExtensionProperties* tmp_props;

    size_t i, j;

    VkResult vr;

    assertm(ctx != NULL, "prepare_inst_extns got NULL context!");
    assertm(pn.data != NULL, "prepare_inst_extns got NULL panel data!");

    /* Get required instance extensions from the panel provider.
     * inst_extn.items will be a char**. */
    inst_extn = pn.get_reqd_inst_extns(ctx, pn);
    DLT_ERRCHECK(ctx, "getting required instance extns", goto rl_inst);

    /* inst_extn now has only the ones the panel provider knows
     * about. Add the ones we know Vulkan cares about. */
    items = (const char**)(DLT_REQUIRED_VK_INST_EXTNS.items);
    for (i = 0; i < DLT_REQUIRED_VK_INST_EXTNS.count; i++) {
	inst_extn = DLT_vec_append(ctx, inst_extn, &(items[i]));
	DLT_ERRCHECK(ctx, "appending instance extn", goto rl_inst);
    }

    /* Create supported inst ext vec. */
    vr = vkEnumerateInstanceExtensionProperties(NULL, &num_items, NULL);
    DLT_VK_CHECK(ctx, vr, "querying extension count: %s", goto rl_inst);

    spp_inst_ext = DLT_vec_create(ctx, sextp, NULL, num_items);
    DLT_ERRCHECK(ctx, "creating supported extensions", goto rl_inst);

    /* Check whether we actually have these extensions available. */
    vr = vkEnumerateInstanceExtensionProperties(
      NULL,
      &(spp_inst_ext.count),
      (VkExtensionProperties*)(spp_inst_ext.items));

    DLT_VK_CHECK(ctx, vr, "enumerating instance exts: %s", goto rl_spp);

    /* Sort instance extensions and supported extensions to compare.
     */
    DLT_vec_sort(inst_extn, &DLT_vec_sort_strings);
    DLT_vec_sort(spp_inst_ext, order_extps);

    /* For every item in inst_extn (known plus panel reported), find
     * it in the supported extensions vec. */
    tmp_props = (VkExtensionProperties*)(spp_inst_ext.items);
    for (i = 0, j = 0; i < inst_extn.count; i++) {
	tmp_seeking = inst_extn.items[i];
	for (; j < spp_inst_ext.count; j++) {
	    tmp_name = tmp_props[j].extensionName;
	    if (strcmp(tmp_seeking, tmp_name) == 0) {
		goto loop_ok;
	    }
	}

	/* Failed to find it. */
	DLT_error_push(ctx,
	               "failed to find instance extension %s",
	               tmp_seeking);
	goto rl_spp;

    loop_ok:;
    }

    return inst_extn;

    /******************************************************************
     * Cleanups:
     */

rl_spp:
    /* Cleanup for supported inst ext */
    DLT_vec_release(ctx, spp_inst_ext);

rl_inst:
    /* Cleanup for inst_extn */
    DLT_vec_release(ctx, inst_extn);

    return inst_extn;
}

/***********************************************************************
TODO: Enrich me.
TODO: Selectable graphics, store GPU properties so you don't keep
having to scan these.
TODO: Refactor.
TODO: Audit.
 */
DLT_queue_indices
select_phys_device(DLT_context*               ctx,
                   DLT_vec                    required_dvc_extns,
                   VkPhysicalDeviceFeatures2* features,
                   DLT_vulkan*                with) {
    DLT_vec dvcs;
    DLT_vec eps;
    DLT_vec qps;

    DLT_queue_indices result = { 0 }, result_fallback, qi;

    /* Does the device have graphics, compute, and presentation
     * queues? Is it dedicated or integrated? */
    int      has_gfx, has_comp, has_extns, is_dedi, is_intg;
    VkBool32 has_pres;

    uint32_t count, version;
    size_t   ii, i, j, k;

    size_t sqp  = sizeof(VkQueueFamilyProperties2);
    size_t sep  = sizeof(VkExtensionProperties);
    size_t spd  = sizeof(VkPhysicalDevice);
    size_t spdf = sizeof(VkPhysicalDeviceFeatures2);

    VkResult                    r;
    VkSurfaceKHR                sfc;
    VkPhysicalDevice            d, d_use = NULL, d_fallback = NULL;
    VkPhysicalDeviceType        dt;
    VkPhysicalDeviceProperties2 dp = { 0 };
    VkPhysicalDeviceFeatures2   df = { 0 }, df_fallback = { 0 };
    VkQueueFamilyProperties2    qp;
    VkQueueFamilyProperties2*   qpp;
    VkQueueFlags                qf;
    VkExtensionProperties       ep;

    const char *name1, *name2, *gpu_name;

    assertm(ctx != NULL, "select_phys_device got NULL context!");
    assertm(with != NULL, "select_phys_device got NULL vulkan!");

    dp.sType          = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    df.sType          = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    df_fallback.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

    sfc = with->surface;

    qi = DLT_queue_indices_create(ctx);
    DLT_ERRCHECK(ctx, "creating queue info list", return result);

    qps = DLT_vec_create(ctx, sqp, NULL, 0);
    DLT_ERRCHECK(ctx, "creating device queue props list", goto rl_qi);

    eps = DLT_vec_create(ctx, sep, NULL, 0);
    DLT_ERRCHECK(ctx, "creating ext props list", goto rl_qps);

    r = vkEnumeratePhysicalDevices(with->instance, &count, NULL);
    DLT_VK_CHECK(ctx, r, "counting GPUs: %s", goto rl_eps);

    dvcs = DLT_vec_create(ctx, spd, NULL, count);
    DLT_ERRCHECK(ctx, "creating phys device list", goto rl_eps);

    r = vkEnumeratePhysicalDevices(with->instance,
                                   &count,
                                   (VkPhysicalDevice*)(dvcs.items));
    DLT_VK_CHECK(ctx, r, "enumerating phys dvc: %s", goto rl_dvcs);

    for (i = 0; i < dvcs.count; i++) {
	/* Pick the first dedicated GPU having graphics, compute,
	 * and presentation support.  Check for needed device
	 * extensions too.  If a usable integrated option is found,
	 * keep track of it, and use it if no acceptable dedicated
	 * GPU is found. */

	/* TODO: For each GPU device, check its properties and queue
	 * families for usability.
	 *
	 * e.g.: dedicated GPU > integrated GPU + must have graphics
	 * and compute queues. */

	/* Start over fresh for each device. */
	has_gfx = has_comp = has_extns = is_dedi = is_intg = 0;

	has_pres = VK_FALSE;

	d = ((VkPhysicalDevice*)(dvcs.items))[i];
	/* TODO: look at device features? */
	/* TODO: Keep track of what VK features we're using? */
	vkGetPhysicalDeviceFeatures2KHR(d, &df);

	/* Inspect physical device properties, including queue
	 * families.
	 */
	vkGetPhysicalDeviceProperties2KHR(d, &dp);
	dt = dp.properties.deviceType;

	if (dt & VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
	    is_dedi = 1;
	} else if (dt & VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
	    is_intg = 1;
	}

	/* Inspect device's queue families. */
	vkGetPhysicalDeviceQueueFamilyProperties2KHR(d, &count, NULL);

	qps = DLT_vec_resize(ctx, qps, count);
	DLT_ERRCHECK(ctx, "resizing queue props list", goto rl_dvcs);
	for (ii = 0; ii < count; ii++) {
	    qpp = &(((VkQueueFamilyProperties2*)(qps.items))[ii]);
	    qpp->sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
	    qpp->pNext = NULL;
	}

	vkGetPhysicalDeviceQueueFamilyProperties2KHR(
	  d,
	  &count,
	  (VkQueueFamilyProperties2*)(qps.items));

	for (j = 0; j < qps.count; j++) {
	    /* For each queue family, check for graphics, present,
	     * and compute. */
	    qp = ((VkQueueFamilyProperties2*)(qps.items))[j];
	    qf = qp.queueFamilyProperties.queueFlags;

	    r = vkGetPhysicalDeviceSurfaceSupportKHR(d,
	                                             (uint32_t)j,
	                                             sfc,
	                                             &has_pres);
	    DLT_VK_CHECK(ctx,
	                 r,
	                 "get queue sfc support: %s",
	                 goto rl_dvcs);

	    if (has_pres == VK_TRUE) {
		qi.pres = DLT_vec_append(ctx, qi.pres, (void**)&j);
		DLT_ERRCHECK(ctx, "adding present queue", goto rl_dvcs);
	    }

	    if (qf & VK_QUEUE_GRAPHICS_BIT) {
		has_gfx = 1;
		qi.gfx  = DLT_vec_append(ctx, qi.gfx, (void**)&j);
		DLT_ERRCHECK(ctx, "adding gfx queue", goto rl_dvcs);
	    }
	    if (qf & VK_QUEUE_COMPUTE_BIT) {
		has_comp   = 1;
		qi.compute = DLT_vec_append(ctx,
		                            qi.compute,
		                            (void**)&j);
		DLT_ERRCHECK(ctx, "adding comp queue", goto rl_dvcs);
	    }
	}

	if (!(has_gfx && has_comp && (has_pres == VK_TRUE))) {
	    /* This is not a usable device. */
	    qi = DLT_queue_indices_reset(qi);
	    continue;
	}

	gpu_name = dp.properties.deviceName;
	version  = dp.properties.apiVersion;

	DLT_log(ctx,
	        DLT_LEVEL_DEBUG,
	        "found usable GPU: %s (Vulkan API %d.%d.%d)\n",
	        gpu_name,
	        VK_VERSION_MAJOR(version),
	        VK_VERSION_MINOR(version),
	        VK_VERSION_PATCH(version));

	/* Now check whether it has the device extensions we need.
	 */
	r = vkEnumerateDeviceExtensionProperties(d, NULL, &count, NULL);
	DLT_VK_CHECK(ctx, r, "count dvc extns: %s", goto rl_dvcs);

	eps = DLT_vec_resize(ctx, eps, count);
	DLT_ERRCHECK(ctx, "resizing ext props list", goto rl_dvcs);

	r = vkEnumerateDeviceExtensionProperties(
	  d,
	  NULL,
	  &count,
	  (VkExtensionProperties*)(eps.items));
	DLT_VK_CHECK(ctx, r, "get dvc extns: %s", goto rl_dvcs);

	DLT_vec_sort(eps, order_extps);

	/* Assume all needed device extensions are present; set to
	 * false if any is not found. */

	has_extns = 1;

	for (j = 0, k = 0; j < required_dvc_extns.count; j++) {
	    /* For each required extension, find it in eps. */
	    name1 = ((const char**)(required_dvc_extns.items))[j];
	    for (; k < eps.count; k++) {
		ep    = ((VkExtensionProperties*)(eps.items))[k];
		name2 = ep.extensionName;
		if (strcmp(name1, name2) == 0) {
		    goto loop_ok;
		}
	    }

	    /* Didn't find it in device extensions, fail. */
	    has_extns = 0;
	    break;

	loop_ok:;
	}

	if (!has_extns) {
	    continue;
	}

	/* Was this device satisfactory? */
	if (is_dedi) {
	    if (d_use != NULL) {
		/* TODO: Select the best device, not just the last
		 * dedicated GPU. */

		/* Release the previous answer if we had one. */
		DLT_queue_indices_release(ctx, result);
	    }

	    /* Use this one. */
	    d_use = d;

	    /* Keep track of its features for later. */
	    memcpy(features, &df, spdf);

	    /* Keep track of its queue info. */
	    result = DLT_queue_indices_clone(ctx, qi);
	    qi     = DLT_queue_indices_reset(qi);
	    DLT_ERRCHECK(ctx, "cloning queue info", goto rl_dvcs);

	    DLT_log(ctx,
	            DLT_LEVEL_DEBUG,
	            "selecting dedicated GPU %s (%d)\n",
	            gpu_name,
	            i);
	} else if (is_intg) {
	    if (d_fallback != NULL) {
		/* Release the previous answer if we had one. */
		DLT_queue_indices_release(ctx, result_fallback);
	    }

	    /* This can be the fallback. */
	    d_fallback = d;

	    /* Keep track of fallback features. */
	    memcpy(&df_fallback, &df, spdf);

	    /* Keep track of fallback device queue info. */
	    result_fallback = DLT_queue_indices_clone(ctx, qi);
	    qi              = DLT_queue_indices_reset(qi);
	    DLT_ERRCHECK(ctx, "clone fallbck queue info", goto rl_dvcs);

	    DLT_log(ctx,
	            DLT_LEVEL_DEBUG,
	            "selecting fallback GPU %s (%d)\n",
	            gpu_name,
	            i);
	}
    }

    /* After all is said and done, if we had no primary dedi GPU,
     * use the fallback. */
    if (d_use != NULL) {
	if (d_fallback == NULL) {
	    DLT_error_push(ctx, "found no acceptable fallback GPU");
	    goto rl_dvcs;
	}

	d_use = d_fallback;

	/* Use the fallback device features as the result. */
	memcpy(features, &df_fallback, spdf);

	/* Use the fallback device queue info as the result. */
	result = DLT_queue_indices_clone(ctx, result_fallback);
	DLT_ERRCHECK(ctx, "cloning fallback queue info", goto rl_dvcs);

	DLT_log(ctx, DLT_LEVEL_DEBUG, "using fallback GPU\n");
    }

    /* Finally, add the physical device to the Vulkan struct. */
    with->phys_device = d;

    DLT_vec_release(ctx, dvcs);
    DLT_vec_release(ctx, eps);
    DLT_vec_release(ctx, qps);
    if (d_fallback != NULL) {
	DLT_queue_indices_release(ctx, result_fallback);
    }
    DLT_queue_indices_release(ctx, qi);

    return result;

rl_dvcs:
    DLT_vec_release(ctx, dvcs);

rl_eps:
    DLT_vec_release(ctx, eps);

rl_qps:
    DLT_vec_release(ctx, qps);

rl_qi:
    DLT_queue_indices_release(ctx, qi);

    if (d_use != NULL) {
	DLT_queue_indices_release(ctx, result);
    }
    if (d_fallback != NULL) {
	DLT_queue_indices_release(ctx, result_fallback);
    }

    return result;
}

/***********************************************************************
vk_queue_configs returns a vec of VkDeviceQueueCreateInfo having
correct values based on the given DLT_queue_indices, with queue
priorities qp.  For now, we'll assume we should turn on all device
features.
 */
DLT_vec
vk_queue_configs(DLT_context* ctx, float* qp, DLT_queue_indices qi) {
    size_t  sqc = sizeof(VkDeviceQueueCreateInfo);
    DLT_vec result;

    uint32_t gfx_idx = 0, pres_idx = 0, comp_idx = 0;

    VkDeviceQueueCreateInfo config = { 0 };

    assertm(ctx != NULL, "vk_queue_configs got NULL context!");
    assertm(qi.gfx.count > 0, "vk_queue_configs got no gfx queue!");
    assertm(qi.pres.count > 0, "vk_queue_configs got no pres queue!");
    assertm(qi.compute.count > 0,
            "vk_queue_configs got no compute queue!");

    /* Set up the result vec. */
    result = DLT_vec_create(ctx, sqc, NULL, 0);
    DLT_ERRCHECK(ctx, "creating result vec", return result);

    /* Just use the first graphics, presentation, and compute
     * queues; assume they match for starters. */
    gfx_idx  = ((uint32_t*)(qi.gfx.items))[0];
    pres_idx = ((uint32_t*)(qi.pres.items))[0];
    comp_idx = ((uint32_t*)(qi.compute.items))[0];

    /* Prepare the queue config for the graphics queue. */
    config.sType      = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    config.queueCount = 1;
    config.queueFamilyIndex = gfx_idx;
    config.pQueuePriorities = qp;

    /* Expand the result vec to add the config. */
    result = DLT_vec_resize(ctx, result, 1);
    DLT_ERRCHECK(ctx, "expanding result vec", goto discard);

    /*  Add the config to the result vec. */
    memcpy(&(((VkDeviceQueueCreateInfo*)result.items)[0]),
           &config,
           sqc);

    if (pres_idx == gfx_idx == comp_idx) {
	/* The presentation, graphics, and compute indices matched,
	 * so we're done here. */
	return result;
    }

    if (pres_idx != gfx_idx) {
	/* Presentation queue didn't match, so add a config for it.
	 */
	config.queueFamilyIndex = pres_idx;

	result = DLT_vec_resize(ctx, result, result.count + 1);
	DLT_ERRCHECK(ctx, "expanding result for pres", goto discard);

	/*  Add the config to the result vec. */
	memcpy(
	  &(((VkDeviceQueueCreateInfo*)result.items)[result.count - 1]),
	  &config,
	  sqc);
    }

    if (comp_idx != gfx_idx) {
	/* Compute queue didn't match, so add a config for it. */
	config.queueFamilyIndex = comp_idx;

	result = DLT_vec_resize(ctx, result, result.count + 1);
	DLT_ERRCHECK(ctx, "expanding result for comp", goto discard);

	/*  Add the config to the result vec. */
	memcpy(
	  &(((VkDeviceQueueCreateInfo*)result.items)[result.count - 1]),
	  &config,
	  sqc);
    }

    return result;

discard:
    DLT_vec_release(ctx, result);

    return result;
}
/***********************************************************************
 */
VkDeviceCreateInfo vk_device_config(DLT_context* ctx,
                                    DLT_config*  cfg,
                                    DLT_vec      queue_cfgs,
                                    DLT_vec      extns,
                                    DLT_vec      layers,
                                    VkPhysicalDeviceFeatures2* features,
                                    DLT_vulkan                 vk) {

    VkDeviceCreateInfo result = { 0 };

    result.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    result.pNext                = features;
    result.queueCreateInfoCount = queue_cfgs.count;
    result.pQueueCreateInfos    = (const VkDeviceQueueCreateInfo*)
                                 queue_cfgs.items;
    result.enabledLayerCount       = layers.count;
    result.ppEnabledLayerNames     = (const char* const*)layers.items;
    result.enabledExtensionCount   = extns.count;
    result.ppEnabledExtensionNames = (const char* const*)extns.items;

    return result;
}

/***********************************************************************
 */
DLT_vulkan DLT_vulkan_create(DLT_context* ctx,
                             DLT_light*   lt,
                             DLT_config*  cfg,
                             DLT_panel    pn) {

    DLT_vulkan result = { 0 };

    /* qp is queue priorities.  Keep it simple. */
    float qp[] = { 1.0 };

    VkResult                           vr;
    VkInstance                         inst;
    VkInstanceCreateInfo               info;
    VkDebugUtilsMessengerCreateInfoEXT debug_config;
    VkDeviceCreateInfo                 device_config;
    VkPhysicalDeviceFeatures2          dvc_features;

    DLT_vec layers;
    DLT_vec inst_extns;
    DLT_vec dev_extns;
    DLT_vec queue_configs;

    DLT_queue_indices dvc_queue_inds;

    size_t sch = sizeof(const char*);

    assertm(ctx != NULL, "DLT_vulkan_create got NULL context!");
    assertm(lt != NULL, "DLT_vulkan_create got NULL light!");
    assertm(pn.data != NULL, "DLT_vulkan_create got NULL panel data!");

    /* Load the Vulkan lib. */
    pn.load_lib(ctx);
    DLT_ERRCHECK(ctx, "loading Vulkan lib", return result);

    /* Get a vec of default Vulkan layers (e.g. validator.) */
    {
	layers = DLT_vec_clone(ctx, DLT_DEFAULT_VK_LAYERS);
	DLT_ERRCHECK(ctx, "cloning layers vec", goto unload);
    }

    /* Discover and check Instance Extensions. */
    {
	inst_extns = prepare_inst_extns(ctx, pn);
	DLT_ERRCHECK(ctx, "preparing inst extensions", goto rl_layers);
    }

    /* Create the VkInstance. */
    {
	info = vk_instance_config(cfg, layers, inst_extns);
	vr   = vkCreateInstance(&info, NULL, &(result.instance));
	DLT_VK_CHECK(ctx, vr, "creating VK instance: %s", goto rl_inst);

	volkLoadInstance(result.instance);

	inst = result.instance;
    }

    /* Get a vec of required device extensions. */
    {
	dev_extns = DLT_vec_clone(ctx, DLT_REQUIRED_VK_DEVICE_EXTNS);
	DLT_ERRCHECK(ctx, "cloning device extensions", goto dst_inst);
    }

    /* Create Vulkan surface */
    {
	result.surface = pn.create_sfc(ctx, result.instance, pn.data);
	DLT_ERRCHECK(ctx, "creating Vulkan surface", goto rl_dev);
    }

    /* Select the physical device. */
    {
	dev_extns      = DLT_vec_sort(dev_extns, DLT_vec_sort_strings);
	dvc_queue_inds = select_phys_device(ctx,
	                                    dev_extns,
	                                    &dvc_features,
	                                    &result);
	DLT_ERRCHECK(ctx, "selecting physical device", goto dst_sfc);

	result.queues.indices = dvc_queue_inds;
    }

    /* Create and load the Vulkan debug messenger. */
    if (cfg->debugging) {
	debug_config = default_debug_config();

	vr = vkCreateDebugUtilsMessengerEXT(inst,
	                                    &debug_config,
	                                    NULL,
	                                    &(result.debugger));
	DLT_VK_CHECK(ctx, vr, "creating debug msgr: %s", goto rl_qi);
    } else {
	result.debugger = NULL;
    }

    /* Create the logical device. */
    {
	queue_configs = vk_queue_configs(ctx, qp, dvc_queue_inds);
	DLT_ERRCHECK(ctx, "creating queue configs", goto dst_dbg);

	device_config = vk_device_config(ctx,
	                                 cfg,
	                                 queue_configs,
	                                 dev_extns,
	                                 layers,
	                                 &dvc_features,
	                                 result);
	DLT_ERRCHECK(ctx, "getting vk device config", goto rl_qconf);

	vr = vkCreateDevice(result.phys_device,
	                    &device_config,
	                    NULL,
	                    &(result.device));
	DLT_VK_CHECK(ctx, vr, "creating VK device: %s", goto rl_qconf);

	/* Load device-specific optimized Vulkan function pointers:
	 * https://github.com/zeux/volk/blob/0f9710cc9ea026c8e0892018512ef88ce9d9042a/README.md#optimizing-device-calls
	 */
	volkLoadDevice(result.device);
    }

    /* Get the device's graphics, presentation, and compute queues.
     */
    {
	vkGetDeviceQueue(result.device,
	                 ((uint32_t*)(dvc_queue_inds.gfx.items))[0],
	                 0,
	                 &(result.queues.gfx));
	vkGetDeviceQueue(result.device,
	                 ((uint32_t*)(dvc_queue_inds.pres.items))[0],
	                 0,
	                 &(result.queues.pres));
	vkGetDeviceQueue(result.device,
	                 ((uint32_t*)(dvc_queue_inds.compute.items))[0],
	                 0,
	                 &(result.queues.compute));
    }

    DLT_vec_release(ctx, queue_configs);
    DLT_vec_release(ctx, dev_extns);
    DLT_vec_release(ctx, inst_extns);
    DLT_vec_release(ctx, layers);

    return result;

    /******************************************************************
     * Cleanups:
     */

rl_qconf:
    /* Queue configs vec */
    DLT_vec_release(ctx, queue_configs);

dst_dbg:
    /* Cleanup for debugging layer */
    if (cfg->debugging) {
	vkDestroyDebugUtilsMessengerEXT(inst, result.debugger, NULL);
    }

rl_qi:
    /* VkPhysicalDevice is just an ID, but queue info came back. */
    DLT_queue_indices_release(ctx, dvc_queue_inds);

dst_sfc:
    /* Destroy Vulkan surface */
    vkDestroySurfaceKHR(result.instance, result.surface, NULL);

rl_dev:
    /* Cleanup for dev_extns */
    DLT_vec_release(ctx, dev_extns);

dst_inst:
    /* Cleanup for VkInstance */
    vkDestroyInstance(result.instance, NULL);

rl_inst:
    /* Cleanup for inst_extns */
    DLT_vec_release(ctx, inst_extns);

rl_layers:
    DLT_vec_release(ctx, layers);

unload:
    /* Load lib */
    pn.unload_lib();

    return result;
}

/***********************************************************************
 */
void DLT_vulkan_pipeline_teardown(DLT_context*        ctx,
                                  VkDevice            d,
                                  DLT_vulkan_pipeline p) {

    size_t i;

    VkImageView* imvs;

    assertm(ctx != NULL,
            "DLT_vulkan_pipeline_teardown got NULL context!");

    imvs = (VkImageView*)(p.swc_image_views.items);

    for (i = 0; i < p.swc_image_views.count; i++) {
	vkDestroyImageView(d, imvs[i], NULL);
	/* Note: destroying the swapchain destroys its images. */
    }

    DLT_vec_release(ctx, p.swc_images);
    DLT_vec_release(ctx, p.swc_image_views);

    vkDestroySwapchainKHR(d, p.swc, NULL);
}

/***********************************************************************
 */
void DLT_vulkan_teardown(DLT_context* ctx,
                         DLT_config*  cfg,
                         DLT_panel    panel,
                         DLT_vulkan   vk) {

    DLT_queue_indices_release(ctx, vk.queues.indices);

    DLT_vulkan_pipeline_teardown(ctx, vk.device, vk.pipeline);

    vkDestroyDevice(vk.device, NULL);

    /* Destroy Vulkan surface */
    vkDestroySurfaceKHR(vk.instance, vk.surface, NULL);

    /* Debugging layer */
    if (cfg->debugging) {
	vkDestroyDebugUtilsMessengerEXT(vk.instance, vk.debugger, NULL);
    }

    /* Cleanup for VkInstance */
    vkDestroyInstance(vk.instance, NULL);

    /* Unload Vulkan lib. */
    panel.unload_lib();
}

/***********************************************************************
 */
void LT_light_teardown(LT_context* ctx, LT_light* lt) {
    DLT_light_teardown((DLT_context*)ctx, (DLT_light*)lt);
}

/***********************************************************************
 */
void DLT_light_teardown(DLT_context* ctx, DLT_light* lt) {
    DLT_config* cfg;
    DLT_panel   pn;

    assertm(ctx != NULL, "DLT_light_teardown got NULL context!");
    assertm(lt != NULL, "DLT_light_teardown got NULL light!");

    pn  = lt->panel;
    cfg = (DLT_config*)(lt->base.config);

    DLT_vulkan_teardown(ctx, cfg, lt->panel, lt->vulkan);
    DLT_panel_teardown(ctx, pn, cfg);
    DLT_context_free(ctx, lt);
}
