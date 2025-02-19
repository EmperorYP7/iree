# Copyright 2019 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Tests of end-to-end IREE support for individual ops in the XLA HLO dialect.
# Each test file should have a name matching the corresponding XLA HLO op and test only the
# functionality of that op (though may make use of other ops where necessary). Tests should be
# written using the IREE Check framework and should always pass on the reference VMLA backend.
# See https://google.github.io/iree/TestingGuide#iree-core-end-to-end-tests.
# TODO(hanchung): Reorganize/fix existing tests so the above is true.

load("//build_tools/bazel:iree_check_test.bzl", "iree_check_single_backend_test_suite")

package(
    default_visibility = ["//visibility:public"],
    licenses = ["notice"],  # Apache 2.0
)

iree_check_single_backend_test_suite(
    name = "check_vmla_vmla",
    srcs = glob(["*.mlir"]),
    driver = "vmla",
    target_backend = "vmla",
)

iree_check_single_backend_test_suite(
    name = "check_vulkan-spirv_vulkan",
    srcs = [
        "abs.mlir",
        "add.mlir",
        "batch_norm_inference.mlir",
        "broadcast_add.mlir",
        "broadcast.mlir",
        "broadcast_in_dim.mlir",
        "clamp.mlir",
        "compare.mlir",
        "convert.mlir",

        # TODO(GH-1695): Enable when xla.concat can be lowered to linalg
        # effectively.
        # "concatenate.mlir",
        #
        "constant.mlir",

        # TODO(GH-1687): Enable after casting from fp to int is handled
        # on structured ops path in vulkan
        # "convert.mlir",
        #
        "cosine.mlir",
        "divide.mlir",
        "dot.mlir",
        "exponential.mlir",

        # TODO(GH-1696): Enable after standard dialect can support floor
        # operation. Lowering from XLA -> linalg should be easy fix.
        # "floor.mlir",

        # TODO(GH-1694): Enable after xla_hlo.gather can be lowered to linalg.
        # "gather.mlir",
        # "gather_concat.mlir",
        #
        "gemm.mlir",
        "gemm_large.mlir",
        "log.mlir",
        "maximum.mlir",
        "minimum.mlir",
        "multiply.mlir",
        "negate.mlir",
        "pad.mlir",
        "reduce.mlir",
        "remainder.mlir",
        "reshape.mlir",
        # "reverse.mlir",
        "rsqrt.mlir",
        "select.mlir",
        "sine.mlir",

        # TODO(GH-1698): Enable after xla_hlo.slice is lowered to linalg and can
        # be handled effectively.
        # "slice.mlir",
        #
        "sqrt.mlir",
        "subtract.mlir",
        "tanh.mlir",
        "torch_index_select.mlir",
        "transpose.mlir",

        # TODO(GH-1688): Enable after integer widths are handled
        # on structured ops path in vulkan

        # "while.mlir",
    ],
    driver = "vulkan",
    target_backend = "vulkan-spirv",
)

iree_check_single_backend_test_suite(
    name = "check_llvm-ir_llvm",
    srcs = [
        "abs.mlir",
        "add.mlir",
        "batch_norm_inference.mlir",
        "broadcast.mlir",
        "broadcast_in_dim.mlir",
        "clamp.mlir",
        "compare.mlir",
        "constant.mlir",
        "convolution.mlir",
        "cosine.mlir",
        "divide.mlir",
        "dot.mlir",
        "exponential.mlir",
        "gemm.mlir",
        "gemm_large.mlir",
        "log.mlir",
        "maximum.mlir",
        "minimum.mlir",
        "multiply.mlir",
        "negate.mlir",
        "pad.mlir",
        "reduce.mlir",
        "reduce_window.mlir",
        "remainder.mlir",
        "reshape.mlir",
        "reverse.mlir",
        "rsqrt.mlir",
        "select.mlir",
        "sine.mlir",
        "sqrt.mlir",
        "subtract.mlir",
        "torch_index_select.mlir",
        "transpose.mlir",
        "while.mlir",
    ],
    driver = "llvm",
    target_backend = "llvm-ir",
)

test_suite(
    name = "check",
    tests = [
        ":check_llvm-ir_llvm",
        ":check_vmla_vmla",
        ":check_vulkan-spirv_vulkan",
    ],
)
