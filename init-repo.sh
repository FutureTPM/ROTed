#!/usr/bin/env bash

git submodule init
git submodule update

cp CMakeLists-BLAKE3.txt BLAKE3/c/

echo "Repo ready to use"
