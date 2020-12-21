#!/usr/bin/env bash
currentDir=$( dirname "$( command -v "$0" )" )
scriptsCommonUtilities="$currentDir/scripts-common/utilities.sh"
[ ! -f "$scriptsCommonUtilities" ] && echo -e "ERROR: scripts-common utilities not found, you must initialize your git submodule once after you cloned the repository:\ngit submodule init\ngit submodule update" >&2 && exit 1
# shellcheck disable=1090
. "$scriptsCommonUtilities"

writeMessage "Checking if Hyperfine is installed"
checkBin hyperfine || errorMessage "This tool requires hyperfine. Install it please, and then run this tool again. (https://github.com/sharkdp/hyperfine)"

ARCH=$(uname -m)

mkdir -p _builds/
mkdir -p bin/

# Build Serial implementation and art
cd _builds && cmake .. -DCMAKE_BUILD_TYPE=Release -DVECTOR_ENGINE=SERIAL -DNTT_USE_NOISE_CACHE=ON -DOT_TEST=ON && make && cd ..
cp _builds/main bin/serial_ot
cp _builds/rel_art/main_rel bin/art_ot

if [ "$ARCH" == "x86_64" ]; then
# Build SSE implementation
cd _builds && cmake .. -DCMAKE_BUILD_TYPE=Release -DVECTOR_ENGINE=SSE -DNTT_USE_NOISE_CACHE=ON -DOT_TEST=ON && make && cd ..
cp _builds/main bin/sse_ot

# Build AVX2 implementation
cd _builds && cmake .. -DCMAKE_BUILD_TYPE=Release -DVECTOR_ENGINE=AVX2 -DNTT_USE_NOISE_CACHE=ON -DOT_TEST=ON && make && cd ..
cp _builds/main bin/avx2_ot

else

# Build NEON implementation
cd _builds && cmake .. -DCMAKE_BUILD_TYPE=Release -DVECTOR_ENGINE=NEON -DNTT_USE_NOISE_CACHE=ON -DOT_TEST=ON && make && cd ..
cp _builds/main bin/neon_ot

fi

writeMessage "Running Benchmarks (this may take a while)"
if [ "$ARCH" == "x86_64" ]; then
cd bin && numactl -C 0 -- hyperfine --warmup 100 -m 1000 './avx2_ot' './serial_ot' './sse_ot' './art_ot' && cd ..
else
cd bin && numactl -C 0 -- hyperfine --warmup 100 -m 1000 './neon_ot' './art_ot' && cd ..
fi

# Build Serial implementation and art
cd _builds && cmake .. -DCMAKE_BUILD_TYPE=Release -DVECTOR_ENGINE=SERIAL -DNTT_USE_NOISE_CACHE=ON -DOT_TEST=OFF && make && cd ..
cp _builds/main bin/serial_rot

if [ "$ARCH" == "x86_64" ]; then
# Build SSE implementation
cd _builds && cmake .. -DCMAKE_BUILD_TYPE=Release -DVECTOR_ENGINE=SSE -DNTT_USE_NOISE_CACHE=ON -DOT_TEST=OFF && make && cd ..
cp _builds/main bin/sse_rot

# Build AVX2 implementation
cd _builds && cmake .. -DCMAKE_BUILD_TYPE=Release -DVECTOR_ENGINE=AVX2 -DNTT_USE_NOISE_CACHE=ON -DOT_TEST=OFF && make && cd ..
cp _builds/main bin/avx2_rot

else

# Build NEON implementation
cd _builds && cmake .. -DCMAKE_BUILD_TYPE=Release -DVECTOR_ENGINE=NEON -DNTT_USE_NOISE_CACHE=ON -DOT_TEST=OFF && make && cd ..
cp _builds/main bin/neon_rot

fi

writeMessage "Running Benchmarks (this may take a while)"
if [ "$ARCH" == "x86_64" ]; then
cd bin && numactl -C 0 -- hyperfine --warmup 100 -m 1000 './avx2_rot' './serial_rot' './sse_rot' && cd ..
else
cd bin && numactl -C 0 -- hyperfine --warmup 100 -m 1000 './neon_rot' './serial_rot' './sse_rot' && cd ..
fi
