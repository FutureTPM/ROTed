FROM rust:1.53

WORKDIR docker/
COPY . .

RUN cargo install hyperfine && \
    apt-get update && \
    apt-get install -y cmake numactl libgmp-dev libmpfr-dev libcunit1-dev libboost-all-dev openssl libssl-dev

CMD ["./utils/benchmarks.sh"]
