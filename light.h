#ifndef PHX_LIGHT_H
#define PHX_LIGHT_H

/* light.h

Light (prefix LT) is a library of various helpers and layers which
simplify the use of high-performance graphics APIs such as Vulkan.

To get started, build an LT_panel using your UI library of choice:

#include <SDL/SDL_vulkan.h>

LT_panel panel {
    SDL_Vulkan_CreateSurface,
    SDL_Vulkan_GetDrawableSize,
    SDL_Vulkan_GetInstanceExtensions,
    SDL_Vulkan_GetVkInstanceProcAddr,
    SDL_Vulkan_LoadLibrary,
    SDL_Vulkan_UnloadLibrary,
};

*/

/*
LT_panel is the Light interface to the UI.  Typically this should
contain SDL types.
*/
typedef struct LT_panel {
    void* create_sfc;
} LT_panel;

/*
LT_config contains the necessary pieces to make Light work.  Pass it to
LT_light to load a new Light window.

Parameters:
 - panel: An LT_panel builder.
 */
typedef struct LT_config {
    LT_panel panel;
} LT_config;

/*
LT_light creates and initializes a Light window using the given
LT_config.
 */
void LT_light(LT_config);

#endif /* PHX_LIGHT_H */
