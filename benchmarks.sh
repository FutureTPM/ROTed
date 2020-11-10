#!/usr/bin/env bash
currentDir=$( dirname "$( command -v "$0" )" )
scriptsCommonUtilities="$currentDir/scripts-common/utilities.sh"
[ ! -f "$scriptsCommonUtilities" ] && echo -e "ERROR: scripts-common utilities not found, you must initialize your git submodule once after you cloned the repository:\ngit submodule init\ngit submodule update" >&2 && exit 1
# shellcheck disable=1090
. "$scriptsCommonUtilities"

writeMessage "Checking if Hyperfine is installed"
checkBin hyperfine || errorMessage "This tool requires hyperfine. Install it please, and then run this tool again. (https://github.com/sharkdp/hyperfine)"

mkdir -p _builds/
mkdir -p bin/

# Build Serial implementation and art
cd _builds && cmake .. -DCMAKE_BUILD_TYPE=Release -DVECTOR_ENGINE=SERIAL && make && cd ..
cp _builds/main bin/serial
cp _builds/rel_art/main_rel bin/art

# Build SSE implementation
cd _builds && cmake .. -DCMAKE_BUILD_TYPE=Release -DVECTOR_ENGINE=SSE && make && cd ..
cp _builds/main bin/sse

# Build AVX2 implementation
cd _builds && cmake .. -DCMAKE_BUILD_TYPE=Release -DVECTOR_ENGINE=AVX2 && make && cd ..
cp _builds/main bin/avx2

writeMessage "Running Benchmarks (this may take a while)"
cd bin && hyperfine --warmup 100 -m 1000 './avx2' './serial' './sse' './art'
