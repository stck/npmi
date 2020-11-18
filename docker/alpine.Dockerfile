ARG TAG=3.12

FROM alpine:${TAG} as builder
RUN apk --no-cache add cmake make g++
COPY "." "/tmp/"
WORKDIR /tmp/build
RUN cmake --clean-first -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles" --build "../" && \
    cmake --build . --target all -- -j 6 && \
    ctest;

FROM alpine:${TAG} as container
COPY --from=builder "/tmp/build/npmci" "/usr/bin/"
