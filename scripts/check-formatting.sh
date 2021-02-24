#!/usr/bin/env bash

#https://stackoverflow.com/a/50264116

source scripts/find_clang_format.sh
find_clang_format

files='.*\.\(cpp\|hpp\|cc\|cxx\|h\)'

output="$(diff -u <(find asgard/ -regex "$files" -exec cat {} \;) <(find asgard/ -regex "$files" -exec ${format} -style=file {} \;))"

echo "Checking format with ${format}"
if [ "$output" != "" ]
then
    echo "$output"
    echo "The code is not correcly formatted."
    echo "Run git clang-format, then commit."
    exit 1
fi
