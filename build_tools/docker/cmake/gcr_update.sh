#!/bin/bash
# Copyright 2020 Google LLC
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

# Builds and pushes the cmake image to gcr.io/iree-oss/

set -x
set -e

# Ensure correct authorization.
gcloud auth configure-docker

# Determine which image tag to update. Updates :latest by default.
TAG="latest"
if [[ "$#" -ne 0  ]]; then
  if [[ "$@" == "--update-prod" ]]; then
    TAG="prod"
  else
    echo "Invalid commandline arguments. Accepts --update-prod or no arguments."
    exit 1
  fi
fi
echo "Updating ${TAG}"


# Build and push the cmake image.
docker build --tag "gcr.io/iree-oss/cmake:${TAG}" build_tools/docker/cmake/
docker push "gcr.io/iree-oss/cmake:${TAG}"
