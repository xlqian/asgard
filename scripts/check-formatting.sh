#!/bin/bash

#https://stackoverflow.com/a/50264116

files='.*\.\(cpp\|hpp\|cc\|cxx\|h\)'

output="$(diff -u <(find asgard/ -regex "$files" -exec cat {} \;) <(find asgard/ -regex "$files" -exec clang-format-7 -style=file {} \;))"

if [ "$output" != "" ]
then
    echo "$output"
    echo "The code is not correcly formatted."
    echo "Run git clang-format, then commit."
    exit 1
fi
