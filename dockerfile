#    docker run --rm pinqergen --list
#    docker run --rm pinqergen -g l20 -n 1000000
#    docker run --rm pinqergen -g bm -n 1000000 -a 0.01 -a 0.05 -a 0.1
#    docker run --rm pinqergen --experiment --csv /tmp/summary.csv
#    docker run --rm pinqergen --help

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
    cmake --build build --target pinqergen --parallel "$(nproc)"

FROM ubuntu:rolling AS runtime

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        libstdc++6 \
        libc6 && \
    rm -rf /var/lib/apt/lists/*

COPY --from=builder /src/build/pinqergen /usr/local/bin/pinqergen

ENTRYPOINT ["/usr/local/bin/pinqergen"]
CMD ["--help"]
