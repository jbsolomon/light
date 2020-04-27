# TODO

## Objectives

### Unscheduled

- [ ] Documentation
- [ ] See if I can figure out how to make DLT_ methods stack-based
- [ ] Make logging configurable
    - [ ] Add logging console (for GUI mode)
- [ ] Allocators / Context
    - [ ] "Concurrent arena context"
    - [ ] Vulkan allocator
- [ ] Figure out how to make DLT_vec suitable for non-pointer-sized
      types
- [ ] Rewrite all error checks with DLT_ERRCHECK where sensible
- [ ] " " " DLT_VK_CHECK
- [ ] Figure out Vulkan versioning
- [ ] Figure out opaque type management
    - [ ] Decide whether DLT types get "initializers" which separate
          allocation from initialization
        - [ ] Useful for pure-stack API, but adds 1 method per type
    - [ ] Hide or put opaque DLT type pointer in "LT" type?
    - [ ] DLT opaque ptr
        - [ ] Advantages
        - [ ] Disadvantages
    - [ ] LT base + hidden DLT extension
        - [ ] Advantages
        - [ ] Disadvantages
    - [ ] Imitate vulkan.h typedef *handles as plain names?  Yuck... but
- [ ] Figure out dep management
    - [ ] Bundle subfolders and plop in DLLs or whatever
    - [ ] Allow header-only mode?
    - [ ] I guess you could compile your project WITH Light, either by
          cloning Light into a subfolder or just directly adding the
          sources
    - [ ] Multi-step build in a parent folder with Light built last
          (this is how it works now.)
- [ ] Physical device features / config

### v0.0.1

- [x] Figure out this "context" stuff
- [x] Build enough context
    - [x] On create, store parent pointer
    - [x] On purge, use parent free on self if any, last
- [x] Fix broken va_args
- [ ] Add debug logging
- [ ] Add file-based logging
- [x] Fix broken error logging
- [x] Fix prepare_inst_extns
- [x] Fix extension search
- [~] Get rid of compiler warnings
- [x] Get rid of compound literals outside of initializers
- [x] Create SDL window
- [x] Figure out teardowns (stack in context?)
- [ ] Context teardowns
- [ ] Audit DLT_context_alloc for missing free on failure
- [ ] Get Vulkan surface
- [ ] Build Vulkan pipeline
- [ ] Get Vulkan rendering target
- [ ] Figure out Find_SDL stuff

## Functions and types

- [ ] LT_stats
- [ ] LT_context
    - [ ] DLT_error
        - [ ] DLT_error_location
        - [ ] Audit, ensure design is right
        - [x] Add error chaining
            - [x] DLT_error_push
            - [x] Expand DLT_log_error with chaining
    - [x] DLT_context
        - [x] DLT_mem_alloc
        - [x] DLT_mem_free
        - [x] DLT_logger
            - [x] DLT_log_level
            - [x] DLT_log_verrf
        - [x] DLT_context_create
        - [x] DLT_context_alloc
        - [x] DLT_context_free
    - [x] LT_context_create
        - [ ] DLT_context_create
    - [x] LT_context_purge
        - [ ] DLT_context_purge
          - free with self alloc if NULL parent?
    - [ ] DLT_cleanup
        - [ ] DLT_cleanup_push
- [x] DLT_log
    - [x] LT_log_info
    - [x] LT_log_warning
    - [x] LT_log_error
- [x] LT_exit
- [x] LT_bail
- [ ] LT_config
    - [ ] DLT_config
        - [ ] DLT_config_create
            - [ ] Add Vulkan config setup
        - [ ] DLT_config_destroy
    - [ ] LT_config_sdl
- [ ] DLT_panel
    - [x] DLT_panel_create
    - [x] DLT_panel_teardown
    - [x] DLT_sdl_panel_create
    - [x] DLT_sdl_panel_teardown
    - [x] DLT_sdl_vk_get_drawable_size
    - [x] DLT_sdl_vk_loadlib
    - [x] DLT_sdl_vk_get_inst_extns
- [ ] LT_light
    - [ ] DLT_vulkan
        - [ ] DLT_vulkan_create
            - [x] select_phys_device
        - [ ] DLT_vulkan_teardown
    - [ ] DLT_light
        - [ ] DLT_light_create
        - [ ] DLT_light_teardown
    - [ ] LT_light_create
    - [ ] LT_light_teardown
- [ ] DLT_vec
    - [ ] DLT_vec_create
    - [ ] DLT_vec_create_cap
    - [ ] DLT_vec_expand
    - [ ] DLT_vec_push
    - [ ] DLT_vec_release
    - [ ] DLT_slice
        - [ ] DLT_vec_slice
        - [ ] DLT_slice_slice
