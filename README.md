# Lattice-Based Random Oblivious Transfer

This repository and source code are the artifact for the paper
"ROTed: Random Oblivious Transfer for embedded devices" to be published at
[TCHES 2021](). As such, we strongly recommend reading the paper before using
the software herein.

The repository contains multiple implementations for random and non-random
oblivious transfer (OT) algorithms. All code should build on ARM and x86
machines.

## Dependencies

In order to run the software the user needs to install multiple
dependencies. The names of the dependencies will vary by Linux distribution and
operating system. Some dependencies are optional while others are required for
the code to build. The optional dependencies are only needed if you intend to
replicate our experiments.

### Required

* GMP
* MPFR
* OpenSSL
* CMake3
* CUnit
* Boost

We present the install commands for some Linux distributions:

* Debian/Ubuntu
```bash
sudo apt-get install cmake libgmp-dev libmpfr-dev libcunit1-dev libboost-all-dev openssl libssl-dev
```

### Optional

`rustup` is a toolchain installer for the Rust programming language. Instead of
using `rustup` to install the required tooling for Rust you may choose to
install `rustc` and `cargo` through the package manager in your Linux
distribution. `hyperfine` is a benchmarking tool written in Rust.
`numactl` is used to pin a process to a single core.

* [rustup](https://rustup.rs/)
* [hyperfine](https://github.com/sharkdp/hyperfine)
* [numactl](https://github.com/numactl/numactl)

We present the install commands for some Linux distributions:

* Debian/Ubuntu
    * Using `rustup`
```bash
sudo apt-get install numactl
cargo install hyperfine
```

    * Not using `rustup`
```bash
sudo apt-get install cargo rustc numactl
cargo install hyperfine
```

## Getting Started

After installing all required dependencies, initialize the repo by running:

```bash
./init-repo.sh
```

Our implementations depend on [BLAKE3](https://github.com/BLAKE3-team/BLAKE3).
The `init-repo.sh` will initialize this dependency and connect it to our build
system.

Using cmake one can build the code using:

```bash
  cmake -DCMAKE_BUILD_TYPE=Release -H. -B_builds -G "Unix Makefiles"
  cmake --build _builds/
```

There are other options the user can use to further specify how the OT and ROT
implementations should be compiled.

* `-DVECTOR_ENGINE={x86 => [AVX512 | AVX2 | SSE | SERIAL]; ARM => [NEON | SERIAL]}`
Specify vector engined to be used. If no value is passed, the `cmake` script
will try and figure out what is the widest vector ISA support in your machine
and default to it.
* `-DNTT_USE_NOISE_CACHE=[ON | OFF]` Use a noise cache when generating random
numbers. Default is off.
* `-DOT_TEST=[ON | OFF]` Compile the OT versions of all algorithms.
Default is off.
* `-DOT_ROTTED_TEST=[ON | OFF]` Compile the ROTted versions of all algorithms.
Default is off. If neither `OT_TEST` or `OT_ROTTED_TEST` are set to ON, `cmake`
will build the standard `ROT` versions. In the case both `OT_TEST` or
`OT_ROTTED_TEST` are set to ON, `cmake` will build the ROTted versions.

Variations to build the code on other systems should be available by consulting
the manpages of `cmake` and changing the `-G` flag accordingly.

## Docker

If you don't feel like going through all of the above procedures and just want
to see the code compile and run, you can use [Docker](https://www.docker.com/).
To install docker follow the instructions in the
[official documentation](https://docs.docker.com/get-started/).

To create, run, and benchmark all code execute:

```bash
docker build -t rlweot .
docker run -it --rm --name run-rlweot rlweot
```

*Note: Trying to replicate our experiments through docker is not recommended.
Please refer to the section
[Replicating the Experiments](#replicating-the-experiments) in order to better
understand how one should go about it.*

## Reading the Output

By default all programs run through CUnit to ensure that the protocols have
the expected output. As such, the only output available
will be CUnit's output, _i.e._, it will show if any assertions failed.

## Code Overview

**If you want to reuse our code you should read this section.**
There are 4 relevant folders: `nfl`, `include`, `rel_art`, and `src`.

[NFLlib](https://github.com/quarkslab/NFLlib) is the backend used to perform
lattice arithmetic. We have modified `nfl` in order to support an AVX512
backend for x86 architectures and an NEON
backend for ARM architectures. Specifically we added the files
[avx512.hpp](nfl/include/nfl/opt/arch/avx512.hpp) and
[neon.hpp](nfl/include/nfl/opt/arch/neon.hpp) to `nfl/include/nfl/opt/arch/`.
We added the backend support to the operations in
[ops.hpp](nfl/include/nfl/ops.hpp) and architectural support in
[arch.hpp](nfl/include/nfl/arch.hpp). The cmake script was also modified in
order to support the new backends.

`include` contains all of the data structures used in order to run the proposed
ROTs and OTs. The OT implementation is in [rlweot.hpp](include/rlweot.hpp), and
the ROT implementation is in [rlwerot.hpp](include/rlwerot.hpp). The random
oracle implementations are in [roms.hpp](include/roms.hpp). All
implementations are templated in order to facilitate easy parameters
modifications without sacrificing performance.

`rel_art` contains the implementation for the PVW08 proposal. The
implementation uses OpenSSL as a backend for elliptic curve arithmetic.

`src` contains the instantiation and tests for the proposed OT and ROT
implementations.

## Replicating the Experiments

Each algorithm was executed on multiple machines:

* Raspberry Pi 2 (ARM Cortex-A7 @ 900MHz)
* Raspberry Pi 3 (ARM Cortex-A53 @ 1.4GHz)
* Raspberry Pi 4 (ARM Cortex-A72 @ 1.5GHz)
* Apple Mac Mini Late 2020 (Apple M1 @ 3.2GHz)
* A machine with an Intel i9-10980XE @ 3GHz

The first step to replicate the results is to fix the clock frequency of all
processors to the described values. There are two scripts in `utils/` which
allows the user to read and set the current operating clock frequency.
The user should run `./utils/watch_cpufreq.sh` to get the available clock
frequencies and `./utils/set_cpufreq.sh` to set one.

We strongly encourage people to use the `benchmark.sh` script in order to
replicate the results. This script will build all configurations for the
running architecture and execute all configurations using the benchmark
tool, hyperfine. When executing hyperfine we pin the process to the first core.
This is done in order to ensure that, when a context switch occurs, the process
will be scheduled to run in the same core. Doing so avoids extra cache misses
incurred by scheduling the process to run in a different core. At the end of
the script, hyperfine will output the speedups for each protocol configuration.
Each protocol is executed 1k times. Given that the clock frequency is fixed,
the number of (R)OTs/s = (1k \* (1/FREQ)) / t, where t is the time taken to run
the specified protocol.

The user should not try to replicate results using docker or other
virtualization software. Doing so incurs extra performance penalties due to the
virtualization layer.

## License
