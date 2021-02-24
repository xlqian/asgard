#!/usr/bin/env bash
#
# https://gist.github.com/eriknyquist/d8d3c14406a76bd7d9516a3aaaa3423e
#
# This pre-commit hook checks if any versions of clang-format
# are installed, and if so, uses the installed version to format
# the staged changes.

source scripts/find_clang_format.sh
find_clang_format

if git rev-parse --verify HEAD >/dev/null 2>&1
then
	against=HEAD
else
	# Initial commit: diff against an empty tree object
	against=4b825dc642cb6eb9a060e54bf8d69288fbee4904
fi

# do the formatting and adding the file again
echo "Formatting code with ${format}"

for file in `git diff-index --cached --name-only $against`
do
    extension="${file##*.}"
    if [ "$extension" = "cpp" ] || [ "$extension" = "h" ]
    then
        "$format" -style=file -i "$file"
        git add "$file"
    fi
done

