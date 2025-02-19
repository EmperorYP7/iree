# Getting Started on Linux with Vulkan

[Vulkan](https://www.khronos.org/vulkan/) is a new generation graphics and
compute API that provides high-efficiency, cross-platform access to modern GPUs
used in a wide variety of devices from PCs and consoles to mobile phones and
embedded platforms.

IREE includes a Vulkan/[SPIR-V](https://www.khronos.org/registry/spir-v/) HAL
backend designed for executing advanced ML models in a deeply pipelined and
tightly integrated fashion on accelerators like GPUs.

This guide will walk you through using IREE's compiler and runtime Vulkan
components. For generic Vulkan development environment set up and trouble
shooting, please see [this doc](generic_vulkan_env_setup.md).

## Prerequisites

You should already have IREE cloned and building on your Linux machine. See the
[Getting Started on Linux with CMake](getting_started_linux_cmake.md) or
[Getting Started on Linux with Bazel](getting_started_linux_bazel.md) guide for
instructions.

You may have a physical GPU with drivers supporting Vulkan. We also support
using [SwiftShader](https://swiftshader.googlesource.com/SwiftShader/) (a high
performance CPU-based implementation of Vulkan).

Vulkan API version > 1.2 is recommended where available. The
`VK_KHR_timeline_semaphore` extension (part of Vulkan 1.2) is required, and the
[Vulkan-ExtensionLayer](https://github.com/KhronosGroup/Vulkan-ExtensionLayer)
project can be used to enable it for drivers (like SwiftShader) without native
support.

## Vulkan Setup

### Background

Please see
[Generic Vulkan Development Environment Setup and Troubleshooting](generic_vulkan_env_setup.md)
for generic Vulkan concepts and development environment setup.

### Quick Start

The
[dynamic_symbols_test](https://github.com/google/iree/blob/master/iree/hal/vulkan/dynamic_symbols_test.cc)
checks if the Vulkan loader and a valid ICD are accessible.

Run the test:

```shell
# -- CMake --
$ export VK_LOADER_DEBUG=all
$ cmake --build build/ --target iree_hal_vulkan_dynamic_symbols_test
$ ./build/iree/hal/vulkan/iree_hal_vulkan_dynamic_symbols_test

# -- Bazel --
$ bazel test iree/hal/vulkan:dynamic_symbols_test --test_env=VK_LOADER_DEBUG=all
```

Tests in IREE's HAL "Conformance Test Suite" (CTS) actually exercise the Vulkan
HAL, which includes checking for supported layers and extensions.

Run the
[device creation test](https://github.com/google/iree/blob/master/iree/hal/cts/device_creation_test.cc):

```shell
# -- CMake --
$ export VK_LOADER_DEBUG=all
$ cmake --build build/ --target iree_hal_cts_device_creation_test
$ ./build/iree/hal/cts/iree_hal_cts_device_creation_test

# -- Bazel --
$ bazel test iree/hal/cts:device_creation_test --test_env=VK_LOADER_DEBUG=all --test_output=all
```

If these tests pass, you can skip down to the next section.

### Setting up SwiftShader

If your system lacks a physical GPU with compatible Vulkan drivers, or you just
want to use a software driver for predictable performance, you can set up
SwiftShader's Vulkan ICD (Installable Client Driver).

IREE has a
[helper script](https://github.com/google/iree/blob/master/build_tools/third_party/swiftshader/build_vk_swiftshader.sh)
for building SwiftShader from source using CMake:

```shell
$ bash build_tools/third_party/swiftshader/build_vk_swiftshader.sh
```

<!-- TODO(scotttodd): Steps to download prebuilt binaries when they exist -->

After building, set the `VK_ICD_FILENAMES` environment variable so the Vulkan
loader uses the ICD:

```shell
$ export VK_ICD_FILENAMES=$PWD/build-swiftshader/Linux/vk_swiftshader_icd.json
```

### Setting up Vulkan-ExtensionLayer

If you are missing support for `VK_KHR_timeline_semaphore`, setup the extension
layer.

Build:

```shell
# -- CMake --
$ cmake --build build/ --target vk_layer_khronos_timeline_semaphore

# -- Bazel --
$ bazel build @vulkan_extensionlayer//:libVkLayer_khronos_timeline_semaphore.so @vulkan_extensionlayer//:VkLayer_khronos_timeline_semaphore_json
```

You should then also set the `VK_LAYER_PATH` environment variable to include the
path to the built layer:

```shell
# -- CMake --
$ export VK_LAYER_PATH=$PWD/build/third_party/vulkan_extensionlayer/layers/:$VK_LAYER_PATH

# -- Bazel --
$ export VK_LAYER_PATH=$PWD/bazel-bin/external/vulkan_extensionlayer/:$VK_LAYER_PATH
```

### Setting up the Vulkan Loader

IREE relies on the `VK_KHR_timeline_semaphore` extension. The minimal loader
version supporting this extenion is `1.1.124`. So if you see failures regarding
timeline semaphore, in addtion to setting up the extension layer, please also
check to make sure the loader is at a proper version.

### Support in Bazel Tests

Bazel tests run in a sandbox, to which environment variables may be forwarded to
using the `--test_env` flag. A user.bazelrc file supporting each of the steps
above looks like this (substitute for the `{}` paths):

```
test --test_env="LD_LIBRARY_PATH={PATH_TO_VULKAN_SDK}/x86_64/lib/"
test --test_env="LD_PRELOAD=libvulkan.so.1"
test --test_env="VK_ICD_FILENAMES={PATH_TO_IREE}/build-swiftshader/Linux/vk_swiftshader_icd.json"
test --test_env="VK_LAYER_PATH=$VK_LAYER_PATH:{PATH_TO_IREE}/bazel-bin/external/vulkan_extensionlayer/"
```

## Using IREE's Vulkan Compiler Target and Runtime Driver

### Compiling for the Vulkan HAL

Pass the flag `-iree-hal-target-backends=vulkan-spirv` to `iree-translate`:

```shell
# -- CMake --
$ cmake --build build/ --target iree_tools_iree-translate
$ ./build/iree/tools/iree-translate -iree-mlir-to-vm-bytecode-module -iree-hal-target-backends=vulkan-spirv ./iree/tools/test/simple.mlir -o /tmp/module.fb

# -- Bazel --
$ bazel run iree/tools:iree-translate -- -iree-mlir-to-vm-bytecode-module -iree-hal-target-backends=vulkan-spirv $PWD/iree/tools/test/simple.mlir -o /tmp/module.fb
```

> Tip:<br>
> &nbsp;&nbsp;&nbsp;&nbsp;If successful, this may have no output. You can pass
> other flags like `-print-ir-after-all` to control the program.

### Executing modules with the Vulkan driver

Pass the flag `-driver=vulkan` to `iree-run-module`:

```shell
# -- CMake --
$ cmake --build build/ --target iree_tools_iree-run-module
$ ./build/iree/tools/iree-run-module -input_file=/tmp/module.fb -driver=vulkan -entry_function=abs -inputs="i32=-2"

# -- Bazel --
$ bazel run iree/tools:iree-run-module -- -input_file=/tmp/module.fb -driver=vulkan -entry_function=abs -inputs="i32=-2"
```

## Running IREE's Vulkan Samples

Install the [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/), then run:

```shell
# -- CMake --
$ cmake --build build/ --target iree_samples_vulkan_vulkan_inference_gui
$ ./build/iree/samples/vulkan/vulkan_inference_gui

# -- Bazel --
$ bazel run iree/samples/vulkan:vulkan_inference_gui
```

## What's next?

More documentation coming soon...

<!-- TODO(scotttodd): link to Vulkan debugging, developer guides -->
