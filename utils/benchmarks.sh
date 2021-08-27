#!/usr/bin/env bash
currentDir=$( dirname "$( command -v "$0" )" )
scriptsCommonUtilities="$currentDir/../thirdparty/scripts-common/utilities.sh"
[ ! -f "$scriptsCommonUtilities" ] && echo -e "ERROR: scripts-common utilities not found, you must initialize the repository before running this script:\n./init-repo.sh" >&2 && exit 1
# shellcheck disable=1090
. "$scriptsCommonUtilities"

writeMessage "Checking if Hyperfine is installed"
checkBin hyperfine || errorMessage "This tool requires hyperfine. Install it please, and then run this tool again. (https://github.com/sharkdp/hyperfine)"

ARCH=$(uname -m)
OS=$(uname -s)

rm -rf _builds/
mkdir -p _builds/
mkdir -p bin/

# Build PVW08 OT implementation
cd _builds && cmake .. -DCMAKE_BUILD_TYPE=Release -DVECTOR_ENGINE=SERIAL -DNTT_USE_NOISE_CACHE=ON -DOT_TEST=ON && make && cd ..
cp _builds/pvw/pvw bin/pvw_ot

rm -rf _builds/
mkdir -p _builds/

# Build PVW08 ROT implementation
cd _builds && cmake .. -DCMAKE_BUILD_TYPE=Release -DVECTOR_ENGINE=SERIAL -DNTT_USE_NOISE_CACHE=ON -DOT_TEST=OFF && make && cd ..
cp _builds/pvw/pvw bin/pvw_ot_rotted

rm -rf _builds/
mkdir -p _builds/

# Build Serial implementation
cd _builds && cmake .. -DCMAKE_BUILD_TYPE=Release -DVECTOR_ENGINE=SERIAL -DNTT_USE_NOISE_CACHE=ON -DOT_TEST=ON && make && cd ..
cp _builds/main bin/serial_ot

rm -rf _builds/
mkdir -p _builds/

# Build Serial implementation ROTted
cd _builds && cmake .. -DCMAKE_BUILD_TYPE=Release -DVECTOR_ENGINE=SERIAL -DNTT_USE_NOISE_CACHE=ON -DOT_ROTTED_TEST=ON && make && cd ..
cp _builds/main bin/serial_ot_rotted

rm -rf _builds/
mkdir -p _builds/

if [ "$ARCH" == "x86_64" ]; then
    # Build SSE implementation
    cd _builds && cmake .. -DCMAKE_BUILD_TYPE=Release -DVECTOR_ENGINE=SSE -DNTT_USE_NOISE_CACHE=ON -DOT_TEST=ON && make && cd ..
    cp _builds/main bin/sse_ot

    rm -rf _builds/
    mkdir -p _builds/

    cd _builds && cmake .. -DCMAKE_BUILD_TYPE=Release -DVECTOR_ENGINE=SSE -DNTT_USE_NOISE_CACHE=ON -DOT_ROTTED_TEST=ON && make && cd ..
    cp _builds/main bin/sse_ot_rotted

    rm -rf _builds/
    mkdir -p _builds/

    # Build AVX2 implementation
    cd _builds && cmake .. -DCMAKE_BUILD_TYPE=Release -DVECTOR_ENGINE=AVX2 -DNTT_USE_NOISE_CACHE=ON -DOT_TEST=ON && make && cd ..
    cp _builds/main bin/avx2_ot

    rm -rf _builds/
    mkdir -p _builds/

    cd _builds && cmake .. -DCMAKE_BUILD_TYPE=Release -DVECTOR_ENGINE=AVX2 -DNTT_USE_NOISE_CACHE=ON -DOT_ROTTED_TEST=ON && make && cd ..
    cp _builds/main bin/avx2_ot_rotted

    rm -rf _builds/
    mkdir -p _builds/
else
    # Build NEON implementation
    cd _builds && cmake .. -DCMAKE_BUILD_TYPE=Release -DVECTOR_ENGINE=NEON -DNTT_USE_NOISE_CACHE=ON -DOT_TEST=ON && make && cd ..
    cp _builds/main bin/neon_ot

    rm -rf _builds/
    mkdir -p _builds/

    cd _builds && cmake .. -DCMAKE_BUILD_TYPE=Release -DVECTOR_ENGINE=NEON -DNTT_USE_NOISE_CACHE=ON -DOT_ROTTED_TEST=ON && make && cd ..
    cp _builds/main bin/neon_ot_rotted

    rm -rf _builds/
    mkdir -p _builds/
fi

# Build Serial implementation and PVW
cd _builds && cmake .. -DCMAKE_BUILD_TYPE=Release -DVECTOR_ENGINE=SERIAL -DNTT_USE_NOISE_CACHE=ON -DOT_TEST=OFF && make && cd ..
cp _builds/main bin/serial_rot

rm -rf _builds/
mkdir -p _builds/

if [ "$ARCH" == "x86_64" ]; then
    # Build SSE implementation
    cd _builds && cmake .. -DCMAKE_BUILD_TYPE=Release -DVECTOR_ENGINE=SSE -DNTT_USE_NOISE_CACHE=ON -DOT_TEST=OFF && make && cd ..
    cp _builds/main bin/sse_rot

    rm -rf _builds/
    mkdir -p _builds/

    # Build AVX2 implementation
    cd _builds && cmake .. -DCMAKE_BUILD_TYPE=Release -DVECTOR_ENGINE=AVX2 -DNTT_USE_NOISE_CACHE=ON -DOT_TEST=OFF && make && cd ..
    cp _builds/main bin/avx2_rot

    rm -rf _builds/
    mkdir -p _builds/
else
    # Build NEON implementation
    cd _builds && cmake .. -DCMAKE_BUILD_TYPE=Release -DVECTOR_ENGINE=NEON -DNTT_USE_NOISE_CACHE=ON -DOT_TEST=OFF && make && cd ..
    cp _builds/main bin/neon_rot

    rm -rf _builds/
    mkdir -p _builds/
fi

writeMessage "Running Benchmarks (this may take a while)"
if [ "$ARCH" == "x86_64" ]; then
    cd bin && numactl -C 0 -- hyperfine --warmup 100 -m 1000 './avx2_rot' './serial_rot' './sse_rot'  './avx2_ot' './serial_ot' './sse_ot' './pvw_ot' './pvw_ot_rotted' './avx2_ot_rotted' './serial_ot_rotted' './sse_ot_rotted' && cd ..
elif [ "$OS" == "Darwin" ]; then
    cd bin && hyperfine --warmup 100 -m 1000 './neon_rot' './serial_rot' './neon_ot' './serial_ot' './pvw_ot' './neon_ot_rotted' './serial_ot_rotted' './pvw_ot_rotted' && cd ..
else
    cd bin && numactl -C 0 -- hyperfine --warmup 100 -m 1000 './neon_rot' './serial_rot' './neon_ot' './serial_ot' './pvw_ot' './neon_ot_rotted' './serial_ot_rotted' './pvw_ot_rotted' && cd ..
fi
