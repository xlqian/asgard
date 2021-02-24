#!/bin/bash

#https://stackoverflow.com/a/50264116

#!/bin/sh
#
# https://gist.github.com/eriknyquist/d8d3c14406a76bd7d9516a3aaaa3423e
#
# This pre-commit hook checks if any versions of clang-format
# are installed, and if so, uses the installed version to format
# the staged changes.

maj_min=7
# The format is clang-format-7 after clang-format-6.0
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

files='.*\.\(cpp\|hpp\|cc\|cxx\|h\)'

output="$(diff -u <(find asgard/ -regex "$files" -exec cat {} \;) <(find asgard/ -regex "$files" -exec clang-format-10 -style=file {} \;))"

if [ "$output" != "" ]
then
    echo "$output"
    echo "The code is not correcly formatted."
    echo "Run git clang-format, then commit."
    exit 1
fi
