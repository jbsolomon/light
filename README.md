# Light

A light graphics toolkit. ☀️

---

Light is a simple library for working with fast, portable graphics APIs.
Light works on Windows.  A user guide can be found in
[doc](doc/README.md).

Light is designed to be as simple as possible to use, but no simpler.

To get started, please follow the
[Getting Started Guide](doc/getting-started.md).

---

## Technical Details

Light is written in portable ANSI C and uses Vulkan.

## Dependencies

Light depends on
[Vulkan-Headers](https://github.com/phoenix-engine/Vulkan-Headers)
and a native surface API, such as the one provided by SDL2 through the
SDL [Light Cell](https://github.com/phoenix-engine/light-cells).

You can install these dependencies on your system, drop them in as local
files in your project, or add them as Git submodules in a CMake build as
shown in
[Light Samples](https://github.com/phoenix-engine/light-samples).

## Samples

To see Light samples, please clone the
[Samples repo](https://github.com/phoenix-engine/light-samples):

```sh
git clone --recursive https://github.com/phoenix-engine/light-samples ../samples
```
