ARG TAG=bullseye

FROM debian:${TAG} as builder
RUN apt-get update -qq && \
    apt-get install -y -qq cmake make g++
COPY "." "/tmp/"
WORKDIR /tmp/build
RUN cmake --clean-first -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles" --build "../" && \
    cmake --build . --target all -- -j 6 && \
    ctest;

FROM debian:${TAG}-slim as container
COPY --from=builder "/tmp/build/npmci" "/usr/bin/"
