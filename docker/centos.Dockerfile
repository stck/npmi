ARG TAG=stream9

FROM quay.io/centos/centos:${TAG} as builder
RUN yum update -y && \
    yum install -y cmake make gcc gcc-c++
COPY "." "/tmp/"
WORKDIR /tmp
RUN cmake -S . -B ./build/ -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles" && \
    cmake --build ./build/ && \
    sh -c "cd ./build && ctest";

FROM quay.io/centos/centos:${TAG} as container
COPY --from=builder "/tmp/build/npmci" "/usr/bin/"
