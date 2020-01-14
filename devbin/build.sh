#!/usr/bin/env bash
# devbin/build.sh

# This script can be used to build any of our known builds as specified
# by cmake build configuration files in ${root}/cmake/buildconfigs/${buildConfig}.cmake
#
# NOTE: A build configuration file is a toolchain file, but our convention is to
# include additional configuration settings so that we have a complete configuration
# in one file.
#
# Please see the devbin/README.md for more information on how to take advantage
# of this script.

# set -e

declare me=$(basename $0)
declare myDir=$(dirname $0)

# Behavior depends on which symlink was used to invoke the script.
declare isTst=false
declare isStats=false
if [[ "$me" == "tst" ]]
then
    isTst=true
elif [[ "$me" == "stats" ]]
then
    isStats=true
fi

# Display a usage statement for this script.
function usage
{
    # Allow optional output stream argument.
    declare out="$1"
    if [[ $out == "" ]]
    then
        out=1
    fi

    # Synopsis for the usage statement depends on how it was invoked.
    declare synopsis
    if $isTst
    then
        synopsis="Run tests."
    elif $isStats
    then
        synopsis="Display build performance statistics."
    else
        # Default is for the "bld" alias, or running "build.sh" directly.
        synopsis="Build CMake targets."
    fi

    cat 1>&"$out" <<EOF
Usage : $me [options] [BLD] [TARGET [...TARGET]]

$synopsis

BLD: CMake buildconfig name, corresponding to one of the files: cmake/buildconfigs/*.cmake
TARGET: CMake target name

Both BLD and TARGET can sometimes be inferred from the current working directory.

Options:
    -c|--configure-only: Only run CMake and not the follow-on build command.
    -h|--help: Display (this) usage statement.
    -j|--jobs JOBS: Number of parallel build jobs.
    -k|--keep-going: Keep building even if there is a failure.
    -n|--dry-run: Show the commands but do not execute them.
    -v|--verbose: Debug the "$me" script by printing additional info.

Environment Variables:
BUILD_ROOT: Alternate to "builds" subdirectory of git root.
CTEST_OUTPUT_ON_FAILURE: Pass through to CTest to control failure detail.
BLD: BLD to use.
JOBS: Number of parallel build jobs
RSYNC_BUILDS: Rsync artifacts to this directory when build succeeds.

EOF
}

# Print a log message (to stderr)
function log
{
    echo "$me: ${@}" 1>&2
}

declare configureOnly=false
declare dryRun=false
declare keepGoing=false
declare verbose=false
declare args=()

function verbose
{
    if $verbose
    then
        log "${@}"
    fi
}

function getOpts
{
    declare shortOpts='hckvnj:'
    declare longOpts='help,configure-only,keep-going,verbose,dry-run,jobs:'
    declare getopt=getopt;
    # On macOS, use getopt from homebrew, since OS X's version does not support long options.
    if [[ $(uname) == "Darwin" ]]
    then
        getopt=/usr/local/opt/gnu-getopt/bin/getopt
    fi
    declare opts=$($getopt --options "$shortOpts" --longoptions "$longOpts" \
        --name "$me" -- "${@}")
    if [ $? != 0 ]
    then
        usage 2
        exit 1
    fi
    eval set -- "$opts"

    while true
    do
        case "$1" in
            -h|--help)
                usage
                exit 0
                ;;
            -c|--configure-only)
                configureOnly=true
                ;;
            -k|--keep-going)
                keepGoing=true
                ;;
            -v|--verbose)
                verbose=true
                ;;
            -n|--dry-run)
                dryRun=true
                ;;
            -j|--jobs)
                JOBS="$2"
                shift
                ;;
            --)
                shift
                break
                ;;
            *)
                log "Internal error!"
                exit 1
                ;;
        esac
        shift
    done

    args=("${@}")
}

function awk()
{
    declare awk=/usr/bin/awk;
    # On macOS, use GNU awk from homebrew, since OS X's version is missing some features.
    if [[ $(uname) == "Darwin" ]]
    then
        awk=/usr/local/bin/gawk
    fi
    $awk "${@}"
}

# Either execute or print a command, depending on dryRun mode.
function maybe
{
    if $dryRun
    then
        log "${@}"
    else
        "${@}"
    fi
}

getOpts "${@}"

function git-root
{
    git rev-parse --show-toplevel
}

declare cwd=$(pwd)
verbose "cwd: $cwd"
declare root="$(git-root)"
verbose "root: $root"
if [[ "${root}" == "" ]]
then
    log "The current working directory ${cwd} is not in a git project tree"
    exit 1
fi

function buildConfigFile
{
    declare buildConfig="$1"
    echo ${root}/cmake/buildconfigs/${buildConfig}.cmake
}

# If they don't specify a build that corresponds to a platform file, we'll try adding a default build type suffix.
declare defaultBuildType=opt

declare builds=${BUILD_ROOT:-${root}/builds}
verbose "builds: $builds"

# We expect the most common way this script will be used is with no command line arguments.
# In that case the script will infer the build config and the build target.
# If arguments are provided, the set the *default* values.
# The script will still attempt to infer, but use the defaults if inferrence doesn't work.
# This is the reverse of what might be expected.

declare buildConfigDefault
declare targetDefault
if (( ${#args[@]} >= 2 ))
then
    buildConfigDefault="${args[0]}"
    targetDefault=("${args[@]:1}")
elif (( ${#args[@]} == 1 ))
then
    # We're given only one arg, but is it a build config name, or a build target?
    declare arg="${args[0]}"
    verbose "arg: $arg"
    if [[ -f $(buildConfigFile "$arg") || -f $(buildConfigFile "$arg-$defaultBuildType") ]]
    then
        buildConfigDefault="$arg"
        targetDefault=()
    else
        buildConfigDefault=""
        targetDefault=("$arg")
    fi
else
    buildConfigDefault=""
    targetDefault=()
fi
verbose "buildConfigDefault: $buildConfigDefault"
verbose "targetDefault[@]: ${targetDefault[@]}"
verbose "#targetDefault: ${#targetDefault[@]}"

# If we can, we will also infer the component being worked on from the current working directory
declare component=""
declare componentPath

function componentFromPath()
{
    component=$(basename "$componentPath")
    if [ "${component}" == "tests" ]
    then
        component=$(basename $(dirname "$componentPath"))
    fi
}

declare buildConfig
declare regexBuilds="^${builds}/([^/]+)((/[^/]+)*)$"
declare regexSrc="^${root}((/[^/]+)*)$"
if [[ "$cwd" =~ $regexBuilds ]]
then
    # We're in a build directory. We can get the buildConfig from the path.
    verbose "We're in a build directory"
    buildConfig=${BASH_REMATCH[1]}
    componentPath=${BASH_REMATCH[2]}
    componentFromPath
elif [[ "$cwd" =~ $regexSrc ]]
then
    # We're in a source directory. We must get buildConfig from $buildConfigDefault or $BLD
    verbose "We're in a source directory"
    buildConfig=${buildConfigDefault:-${BLD}}
    componentPath=${BASH_REMATCH[1]}
    componentFromPath
else
    # We don't seem to be in either a build directory or source directory.
    verbose "We're in neither build nor source directory"
    buildConfig=${buildConfigDefault:-${BLD}}
    component=""
fi
verbose "buildConfig: $buildConfig"
verbose "component: $component"

if [ "${buildConfig}" == "" ]
then
    log "Could not infer build configuration."
    exit 1
fi

declare config=$(buildConfigFile "${buildConfig}")
verbose "config: $config"

# If there is no build config as specified, try adding default build type suffix.
if [ ! -f "${config}" ]
then
    declare impliedBuildConfig="${buildConfig}-${defaultBuildType}"
    verbose "impliedBuildConfig: ${impliedBuildConfig}"
    declare impliedConfig=$(buildConfigFile "${impliedBuildConfig}")
    verbose "impliedConfig: ${impliedConfig}"
    if [ -f "${impliedConfig}" ]
    then
        buildConfig="${impliedBuildConfig}"
        config="${impliedConfig}"
    fi
fi

declare build="${builds}/${buildConfig}"
log "Using build directory ${build}"

# If the build configuration file does not exist, then something is wrong
if [ ! -f "${config}" ]
then
    log "${buildConfig} is not a recognized build (no build configuration file)"
    exit 1
fi

function configure
{
    set -e
    maybe "${root}/cmake/genbuild.sh" "${buildConfig}"
    set +e
}

# Check for configure-only option.
if $configureOnly
then
    configure
    exit 0
fi

# If the build directory does not exist, but buildConfig references an existing build configuration file,
# then generate the build.
if [ ! -d ${build} ]
then
    configure
fi

# Determine the CMake target(s).
declare target
declare needsR5Libs=0

if $isTst
then
    # Was this script invoked via the alias `tst`?
    if [[ "${component}" == "" ]]
    then
        # If we are NOT inside a component directory,
        target=(${targetDefault[@]:-test})
    else
        target=(${targetDefault[@]:-run_${component}_tests})
    fi
elif egrep -q "ETA_R5\s+ON" ${config}
then
    # else is this build configuration for the R5?
    target=(${targetDefault[@]:-all})
    if [ ! -f "bsp_r5_0/psu_cortexr5_0/include/xil_printf.h" ]
    then
        needsR5Libs=1
    fi
else
    # otherwise default to the `all` target
    target=(${targetDefault[@]:-all})
fi
verbose "target[@]: ${target[@]}"
verbose "#target: ${#target[@]}"

# Enable CTest failure details output unless explicitly disabled.
export CTEST_OUTPUT_ON_FAILURE=${CTEST_OUTPUT_ON_FAILURE:-1}
verbose "CTEST_OUTPUT_ON_FAILURE: $CTEST_OUTPUT_ON_FAILURE"

# From http://stackoverflow.com/questions/1527049/bash-join-elements-of-an-array
function join {
    local IFS="$1"
    shift
    echo "$*"
}

# Display the Ninja build performance statistics.
function ninjaStats
{
    declare ninjaLog="${build}/.ninja_log"
    if [ ! -f "$ninjaLog" ]
    then
        log "Unable to display build performance statistics because the Ninja log ($ninjaLog) was not found."
        exit 1
    fi
    declare awkScript="$myDir/ninja-build-stats.awk"
    if [ ! -f "$awkScript" ]
    then
        log "Unable to display build performance statistics because the AWK script ($awkScript) was not found."
        exit 1
    fi
    declare targetRegex
    if [[ "${target}" == "all" ]]
    then
        targetRegexp='.*'
    else
        # Assume target names are valid regular expressions.
        targetRegexp=$(join '|' "${target[@]}")
    fi
    verbose "targetRegexp: ${targetRegexp}"
    maybe awk -f "$awkScript" < "$ninjaLog" | grep -E "CMakeFiles/(${targetRegexp})\\.dir/|bin/(${targetRegexp})|lib/lib(${targetRegexp})d?\\.(a|so)"
}

declare status=0

if [ -f "${build}/Makefile" ]
then
    if $isStats
    then
        log "Build performance statistics not available on Make platforms."
        exit 1
    fi
    declare jobs=${JOBS:-10}
    if [ "$needsR5Libs" -eq "1" ]
    then
        maybe make -j${jobs} -C ${build} r5_libs
    fi
    declare keepGoingArg
    if $keepGoing
    then
        keepGoingArg='--keep-going'
    fi
    maybe make $keepGoingArg --no-print-directory -j${jobs} -C ${build} "${target[@]}"
    status=$?
elif [ -f "${build}/build.ninja" ]
then
    if $isStats
    then
        ninjaStats
        exit 0
    fi
    # Only add the -j arg if JOBS is specified. Ninja has an aggressive default based on number of cores.
    declare jobsArg
    if [ -n "$JOBS" ]
    then
        jobsArg="-j $JOBS"
    fi
    if [ "$needsR5Libs" -eq "1" ]
    then
        maybe ninja $jobsArgs -C ${build} r5_libs
    fi
    declare keepGoingArg
    if $keepGoing
    then
        keepGoingArg='-k 0'
    fi
    maybe ninja $jobsArg $keepGoingArg -C ${build} "${target[@]}"
    status=$?
elif $dryRun
then
    log "Dry run mode does not allow determination of build command without preexisting builds directory"
    exit 1
else
    log "${build} does not have Makefile or ninja.build, so CMake may have failed."
    log "You may just need to remove the build directory and try again:"
    log "rm -rf ${build} && ${me} ${buildConfig} ${target[@]}"
    exit 1
fi
verbose "status: $status"

if [ "${BLD}" != "${buildConfig}" ]
then
    log "export BLD=${buildConfig}"
fi

# If requested, rsync the build artifacts.
if [[ "$status" == 0 ]]
then
    if [[ "$RSYNC_BUILDS" != "" ]]
    then
        binSource="$build/bin"
        libSource="$build/lib"
        dataSource="$build/data"
        binTarget="$RSYNC_BUILDS/$buildConfig/bin"
        libTarget="$RSYNC_BUILDS/$buildConfig/lib"
        dataTarget="$RSYNC_BUILDS/$buildConfig/data"
        if [ -d "$binSource" ]
        then
            maybe mkdir -p "$binTarget"
            maybe rsync --archive --verbose "$binSource/" "$binTarget/"
        fi
        if [ -d "$libSource" ]
        then
            # Only rsync shared libraries if there are any.
            for f in "$libSource"/*.so
            do
                if [ -e "$f" ]
                then
                    maybe mkdir -p "$libTarget"
                    maybe rsync --archive --verbose "$libSource"/*.so "$libTarget/"
                fi
                break
            done
        fi
        if [ -d "$dataSource" ]
        then
            maybe mkdir -p "$dataTarget"
            maybe rsync --archive --verbose "$dataSource/" "$dataTarget/"
        fi
    fi
fi

exit ${status}
