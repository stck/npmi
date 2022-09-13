ARG TAG=3.15

FROM alpine:${TAG} as builder
RUN apk --no-cache add cmake make g++
COPY "." "/tmp/"
WORKDIR /tmp
RUN cmake -S . -B ./build/ -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles" && \
    cmake --build ./build/ && \
    sh -c "cd ./build && ctest";

FROM alpine:${TAG} as container
COPY --from=builder "/tmp/build/npmci" "/usr/bin/"
