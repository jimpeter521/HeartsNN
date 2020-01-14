#!/bin/bash

set -o errexit

PROGNAME=$(basename $0)

usage()
{
    cat <<eof 1>&2
Usage: $PROGNAME [-hd] [CMD [ARGS...]]

-d  Start the docker container with the ability to run gdb.
-h  Print this help

If CMD is unspecified, run an interactive Bash shell.
eof

exit 2
}

_gitRoot()
{
    git rev-parse --show-toplevel
}

gitRoot=$(_gitRoot)

privileged=0

while getopts ":hd" opt
do
  case ${opt} in
    h ) usage
      ;;
    d ) privileged=1
      shift
      ;;
    \? ) usage
      ;;
  esac
done


# Run in the Docker image optimized for the dev cycle, or the image specified by DOCKER_IMAGE.
# To build the default image, run `make docker/build` in the repo root.
DOCKER_IMAGE="${DOCKER_IMAGE:-heartsnn}"

# Allow Docker volumes used for building to be specified via env var.
BUILDS_VOLUME=${BUILDS_VOLUME:-builds}

# To run GDB from macOS, we need to use privileged mode a certain way.
if (($privileged))
then
    privileged_args=(--cap-add=SYS_PTRACE --security-opt seccomp=unconfined)
else
    privileged_args=()
fi

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
BUILD_ROOT="$gitRoot/builds/volume"

# Tell build.sh to rsync the build artifacts to the mounted volume after building.
RSYNC_BUILDS="$gitRoot/builds"

# Pass through certain env vars, including ones that we set above.
passthru=(BUILD_ROOT CTEST_OUTPUT_ON_FAILURE DEBUG JOBS RSYNC_BUILDS)
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

cwd=$(pwd -P); docker run --tty --rm \
       --cap-add=sys_nice \
       ${DOCKER_ARGS} \
       "${run_args[@]}" \
       "${env_args[@]}" \
       --volume "${BUILDS_VOLUME}":"${BUILD_ROOT}" \
       --volume "$gitRoot:$gitRoot" \
       --volume "$gitRoot/devbin":/vol/devbin \
       --workdir ${gitRoot} \
       "${privileged_args[@]}" \
       "$DOCKER_IMAGE" \
       "${args[@]}"
