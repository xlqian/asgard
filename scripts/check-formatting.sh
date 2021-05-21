#!/usr/bin/env bash

#https://stackoverflow.com/a/50264116

source scripts/find_clang_format.sh
find_clang_format

echo "Checking format with ${format}"

files_type=".*\.\(cpp\|hpp\|cc\|cxx\|h\)"
files=$(find asgard/ -regex ${files_type})
output="$(${format} --dry-run ${files} 2>&1)"

if [ "$output" != "" ]
then
    echo "$output"
    echo "The code is not correcly formatted."
    echo "Run git clang-format, then commit."
    exit 1
else
    echo "The code is well formatted"
fi
