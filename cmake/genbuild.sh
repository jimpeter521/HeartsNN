#!/usr/bin/env bash
# cmake/genbuild.sh [BUILD_NAME]

function git-root {
    git rev-parse --show-toplevel
}

BUILD_NAME=${1:-$(basename "${BASH_SOURCE[0]}" ".sh")}
REPO_ROOT="$(git-root)"
PLATFORMS=${REPO_ROOT}/cmake/buildconfigs
TOOLCHAIN=${PLATFORMS}/${BUILD_NAME}.cmake
BUILDS=${BUILD_ROOT:-${REPO_ROOT}}

if [ ! -f ${TOOLCHAIN} ]; then
    echo "BUILD NAME IS: ${BUILD_NAME}"
    echo "Toolchain file ${TOOLCHAIN} not found!"
else
    BUILD_DIR=${BUILDS}/${BUILD_NAME}
    mkdir -p ${BUILD_DIR}
    INITIAL_CACHE=${BUILD_DIR}/initial-cache.txt
    grep "set.*CACHE.*FORCE" ${TOOLCHAIN} > ${INITIAL_CACHE}
    cmake -H${REPO_ROOT} -B${BUILD_DIR} -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN} -C${INITIAL_CACHE}
fi
