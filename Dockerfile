FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    git \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*


# ------------------------------------------------------------
# Build OGDF
# ------------------------------------------------------------

WORKDIR /opt

RUN git clone \
    --depth 1 \
    --branch elderberry-202309 \
    https://github.com/ogdf/ogdf.git

RUN cmake \
    -S /opt/ogdf \
    -B /opt/ogdf-build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS_RELEASE="-O2 -DNDEBUG -march=x86-64 -mtune=generic" \
    -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF \
    -DBUILD_SHARED_LIBS=OFF \
    -DOGDF_WARNING_ERRORS=OFF \
    -DOGDF_SEPARATE_TESTS=OFF \
    -DOGDF_ARCH=x86-64

RUN cmake \
    --build /opt/ogdf-build \
    --target OGDF \
    --parallel 2


# ------------------------------------------------------------
# Build BM Planarity API
# ------------------------------------------------------------

WORKDIR /app

COPY . .

RUN cmake \
    -S . \
    -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS_RELEASE="-O2 -DNDEBUG -march=x86-64 -mtune=generic" \
    -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF \
    -DBM_ENABLE_JSON_TOOLS=ON \
    -DBM_ENABLE_API_SERVER=ON \
    -DBM_ENABLE_OGDF_LAYOUT=ON \
    -DOGDF_DIR=/opt/ogdf-build

RUN cmake \
    --build build \
    --target bm_planarity_api \
    --parallel 2


# ------------------------------------------------------------
# Runtime image
# ------------------------------------------------------------

FROM ubuntu:24.04

RUN apt-get update && apt-get install -y --no-install-recommends \
    libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder \
    /app/build/bm_planarity_api \
    /usr/local/bin/bm_planarity_api

EXPOSE 10000

CMD ["bm_planarity_api"]