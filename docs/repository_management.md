# IREE Repository Management

Due to the process by which we synchronize this GitHub project with our internal
Google source code repository, there are some oddities in our workflows and
processes. We aim to minimize these, and especially to minimize their impact on
external contributors, but they are documented here for clarity and
transparency. If any of these things are particularly troublesome or painful for
your workflow, please reach out to us so we can prioritize a fix.

## Dependencies

As a project which brings together compiler, runtime and graphics systems,
dependency management is somewhat complex. We use git submodules for C++
dependencies and, where possible, language specific external package management
for other types of dependencies (e.g. Python).

In addition, dependencies are managed entirely different in the Google internal
source code repository. This imposes constraints on repository management tasks
that may not be obvious.

The git submodule state is not accessible to the tooling that moves the IREE
source between GitHub and Google's source repository. We therefore mirror this
state in a special `SUBMODULE_VERSIONS` file. This file is considered the source
of truth for the correct submodule state and is what is used by the CI. When
updating the submodule state from Google's source repository, only this file is
updated and another
[GitHub Actions workflow](https://github.com/google/iree/blob/master/.github/workflows/synchronize_submodules.yml)
immediately commits a submodule update on top of that.

Shortcut commands (read below for full documentation):

```shell
# Update SUBMODULE_VERSIONS from current git submodule pointers
./scripts/git/submodule_versions.py export

# Update current git submodule pointers based on SUBMODULE_VERSIONS
./scripts/git/submodule_versions.py import
```

### The special relationship with LLVM and TensorFlow

Currently, the two most challenging projects to manage as dependencies are
TensorFlow and LLVM. Both are typically pinned to specific versions that are
integrated in the Google source repository at a cadence up to many times per
day. Further, because LLVM does not ship with Bazel BUILD files, IREE "borrows"
the BUILD files from TensorFlow (for building with Bazel). Just to make it more
interesting, since TensorFlow does not ship with CMakeLists, IREE uses overlay
CMakeLists.txt to build subsets of TensorFlow needed for the compiler (when
built with CMake). While these externally managed build files are written to be
moderately generic, they can and do break and require manual intervention at
times (i.e. there is no guarantee that updating to a new commit of either will
not require some manual work on the build files).

The only combination which is expected to work is the llvm-project commit noted
in the `LLVM_COMMIT` setting in
`third_party/tensorflow/tensorflow/workspace.bzl`. In reality, it is often
possible (especially with the CMake build) to use a newer llvm-project commit,
and private development forks should feel free to do this. However, to submit
upstream, we will need to integrate this newer commit in the internal Google
source of truth prior to accepting code that depends on it.

LLVM is integrated into Google's source repository at a cadence up to several
times a day. When this necessitates updating IREE to changing LLVM and MLIR
APIs, these updates are performed atomically as part of the integration.
Previously this did not trigger a change to the LLVM submodule version in open
source, which would result in breakages due to version skew. We are
transitioning to make each integration of LLVM update IREE's submodule in OSS.
This means that API updates will have some chance of not breaking the build.
However, it can introduce breakages as well, especially in the Bazel build where
we still have to wait to update the BUILD files where necessary. We are
continuing to work on improving this situation.

### Tasks:

#### Adding dependencies

In general, adding dependencies will require coordination with Google engineers
to make the dependency available both on GitHub and in the Google source
repository. It is important to ask on the mailing list prior to expecting to
contribute such changes.

#### Pushing dependency changes

When working on a development branch, feel free to stage changes however makes
sense. However, when sending a PR, note that our integration systems will
overwrite the submodule version updates from the `SUBMODULE_VERSIONS` file in
the repository root. Here is an example:

```text
6ec136281086b71da32b5fb068bd6e46b78a5c79 third_party/abseil-cpp
309de5988eb949a27e077a24a1d83c0687d10d57 third_party/benchmark
4c13807b7d43ff0946b7ffea0ae3aee9e611d778 third_party/dear_imgui
97f3aa91746a7d207513a73725e92cee7c35bb87 third_party/flatbuffers
3d62e9545bd15c5df9ccfdd8453b93d64a6dd8eb third_party/ruy
f2fb48c3b3d79a75a88a99fba6576b25d42ec528 third_party/googletest
a21beccea2020f950845cbb68db663d0737e174c third_party/llvm-project
80d452484c5409444b0ec19383faa84bb7a4d351 third_party/pybind11
b73f111094da3e380a1774b56b15f16c90ae8e23 third_party/sdl2
b252a50953ac4375cb1864e94f4b0234db9d215d third_party/spirv_headers
6652f0b6428777b5a4a3d191cc30d8b31366b999 third_party/swiftshader
9b32b2db1142166b190ea30757f48d8dd0fb00e3 third_party/tensorflow
ba091ba6a947f79623b28fe8bfccdce1ab9fa467 third_party/vulkan_headers
909f36b714c9239ee0b112a321220213a474ba53 third_party/vulkan_memory_allocator
```

If bumping versions, you must include an update to this file with your commit.
To generate it, run:

```shell
# Performs a submodule sync+update and stages an updated SUBMODULE_VERSIONS
# file.
./scripts/git/submodule_versions.py export
```

If you don't know if this is required, you may run:

```shell
# The check command is intended to eventually be usable as a git hook
# for verification of consistency between SUBMODULE_VERSIONS and the
# corresponding local git state.
./scripts/git/submodule_versions.py check
```

#### Pulling dependency changes

If you pull a change to `SUBMODULE_VERSIONS` it is necessary to import it into
the current git state:

```shell
# Updates the commit hash of any entries in SUBMODULE_VERSIONS that differ
# and stages the changes.
./scripts/git/submodule_versions.py import
```

This will stage any needed changes to the submodules to bring them up to date
with the `SUBMODULE_VERSIONS`. If you have local changes, you may get conflicts
on the `submodule update` step at the end, and you will need to manually resolve
as usual.

Any CI systems or other things that retrieve an arbitrary commit should invoke
this prior to running just to make sure that their git view of the submodule
state is consistent.

TODO(laurenzo): Add a GitHub hook to auto-commit submodule updates on
`SUBMODULE_VERSIONS` file changes.

#### Updating TensorFlow and LLVM versions

WARNING: These scripts have not been updated to reflect the new tooling to
automatically update the LLVM hash directly from Google's source repository
rather than through TensorFlow.

```shell
# By default, updates third_party/tensorflow to the remote head and
# third_party/llvm-project to the commit that tensorflow references.
# Also updates the IREE mirrors of key build files. All changes will
# be staged for commit.
./scripts/git/update_tf_llvm_submodules.py

# You can also select various other options:
# Don't update the tensorflow commit, just bring LLVM to the correct
# linked commit.
./scripts/git/update_tf_llvm_submodules.py --tensorflow_commit=KEEP

# Don't update the LLVM commit.
./scripts/git/update_tf_llvm_submodules.py --llvm_commit=KEEP

# Update to a specific TensorFlow commit (and corresponding LLVM).
./scripts/git/update_tf_llvm_submodules.py --tensorflow_commit=<somehash>

# Update to a specific LLVM commit.
./scripts/git/update_tf_llvm_submodules.py --llvm_commit=<somehash>
```
