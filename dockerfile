#    docker run --rm PINQERGEN --list
#    docker run --rm PINQERGEN -g l20 -n 1000000
#    docker run --rm PINQERGEN -g bm -n 1000000 -a 0.01 -a 0.05 -a 0.1
#    docker run --rm PINQERGEN --experiment --csv /tmp/summary.csv
#    docker run --rm PINQERGEN --help

FROM ubuntu:rolling AS builder

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        g++ \
        cmake \
        make \
        git \
        ca-certificates && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build --target PINQERGEN --parallel "$(nproc)"

FROM ubuntu:rolling AS runtime

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        libstdc++6 \
        libc6 && \
    rm -rf /var/lib/apt/lists/*

COPY --from=builder /src/build/PINQERGEN /usr/local/bin/PINQERGEN

ENTRYPOINT ["/usr/local/bin/PINQERGEN"]
CMD ["--help"]
