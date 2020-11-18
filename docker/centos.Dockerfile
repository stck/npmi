ARG TAG=8

FROM centos:${TAG} as builder
RUN yum update -y && \
    yum install -y cmake make gcc gcc-c++
COPY "." "/tmp/"
WORKDIR /tmp/build
RUN cmake --clean-first -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles" --build "../" && \
    cmake --build . --target all -- -j 6 && \
    ctest;

FROM centos:${TAG} as container
COPY --from=builder "/tmp/build/npmci" "/usr/bin/"
