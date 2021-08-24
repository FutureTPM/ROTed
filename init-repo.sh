#!/usr/bin/env bash

if ! which git > /dev/null; then
    echo "git command not found. Is it installed?"
    echo "Exiting..."
    exit 1
fi

if [ ! -d .git ]; then
    if ! git rev-parse > /dev/null 2>&1; then
        echo "Couldn't find a git repo. Initializing..."
        git init
    fi
fi

git submodule init
git submodule update

cp CMakeLists-BLAKE3.txt BLAKE3/c/CMakeLists.txt

echo "Repo ready to use"
