#!/usr/bin/env bash

set -o errexit -o pipefail -o nounset

function find_clang_format { 
    # we use clang-format from 7 - 10 
    maj_min=7
    maj_max=10

    base=clang-format
    format=""

    # Redirect output to stderr.
    exec 1>&2

     # check if clang-format is installed
    type "$base" >/dev/null 2>&1 && format="$base"

    # if not, check all possible versions
    # (i.e. clang-format-<$maj_min-$maj_max>)
    if [ -z "$format" ]
    then
        for j in `seq $maj_min $maj_max`
        do
            type "$base-$j" >/dev/null 2>&1 && format="$base-$j" && break
            [ -z "$format" ] || break
        done
    fi

    # no versions of clang-format are installed
    if [ -z "$format" ]
    then
        echo "$base is not installed. Pre-commit hook will not be executed."
        exit 0
    fi

    echo "$format found."
}
