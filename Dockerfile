FROM gcc:latest AS build

RUN apt-get update && apt-get install -y libmariadb3 libmariadb-dev cmake

WORKDIR /app

COPY . /app

RUN mkdir build && cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/Linux-GNU-native.cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE="Nightly" -B ./build -S ./ && make -j$(nproc) -C ./build

FROM debian:12-slim AS base
RUN apt-get update && apt-get install -y libmariadb3

WORKDIR /app

COPY --from=build /app/build/bin-native64/SphereSvrX64_nightly /app

CMD ["./SphereSvrX64_nightly"]