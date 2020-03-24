#!/bin/bash

set -o errexit

_gitRoot()
{
    git rev-parse --show-toplevel
}

gitRoot=$(_gitRoot)

# Run in the Docker image optimized for the dev cycle, or the image specified by DOCKER_IMAGE.
DOCKER_IMAGE="${DOCKER_IMAGE:-heartsnn/heartsnn}"

# Allow Docker volumes used for building to be specified via env var.
BUILDS_VOLUME=${BUILDS_VOLUME:-heartsnn-builds}

# By default, run interactive Bash, but allow any non-interactive command to be specified instead.
if [[ "$1" == "" ]]
then
    run_args=(--interactive --tty)
    args=(bash)
else
    run_args=()
    args=("$@")
fi

# Reduce default parallelism for Docker builds, since it doesn't seem to benefit.
JOBS=${JOBS:-2}

# Point to the Docker volume for build artifacts.
BUILD_ROOT="${gitRoot}/builds/volume"

# Pass through certain env vars, including ones that we set above.
passthru=(BUILD_ROOT CTEST_OUTPUT_ON_FAILURE DEBUG JOBS)
unset env_args
for env in "${passthru[@]}"
do
    export $env
    env_args+=(--env)
    env_args+=($env)
done

# Allow DOCKER_ARGS to be passed through from the parent. Since arrays cannot be encoded in the environment, we simply
# expand the variable without quotes to allow it to contain multiple arguments.
# https://stackoverflow.com/questions/5564418/exporting-an-array-in-bash-script

echo "gitRoot: ${gitRoot}"
echo "BUILD_ROOT: ${BUILD_ROOT}"
echo "BUILDS_VOLUME: ${BUILDS_VOLUME}"
echo "DOCKER_IMAGE: ${DOCKER_IMAGE}"

cwd=$(pwd -P); docker run --tty --rm \
       --cap-add=sys_nice \
       ${DOCKER_ARGS} \
       "${run_args[@]}" \
       "${env_args[@]}" \
       --volume "${BUILDS_VOLUME}":"${BUILD_ROOT}" \
       --volume "$gitRoot:$gitRoot" \
       --volume "$gitRoot/devbin":/vol/devbin \
       --workdir ${gitRoot} \
       "$DOCKER_IMAGE" \
       "${args[@]}"
