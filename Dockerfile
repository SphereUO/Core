FROM debian:latest AS build

RUN apt-get update && apt-get install -y git libmariadb3 libmariadb-dev cmake g++

WORKDIR /app

COPY . /app

RUN mkdir build && cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/Linux-GNU-x86_64.cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE="Nightly" -B ./build -S ./ && make -j$(nproc) -C ./build

FROM debian:latest AS base

WORKDIR /app

RUN apt-get update && apt-get install -y libmariadb3

COPY --from=build /app/build/bin-x86_64/SphereSvrX64_nightly /app

CMD ["./SphereSvrX64_nightly"]