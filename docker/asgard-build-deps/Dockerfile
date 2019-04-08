FROM debian:9

RUN apt-get update && apt-get install -y --no-install-recommends git \
      autoconf \
      automake \
      make \
      libtool \
      pkg-config \
      g++ \
      gcc \
      locales \
      protobuf-compiler \
      libboost-all-dev \
      libcurl4-openssl-dev \
      libprotobuf-dev \
      libgeos-dev \
      libgeos++-dev \
      liblua5.2-dev \
      libspatialite-dev \
      libsqlite3-dev \
      spatialite-bin \
      liblz4-dev \
      unzip \
      lua5.2 \
      python-all-dev \
      vim-common \
      jq \
      gucharmap \
      libprotobuf10 \
      libgeos-3.5.1 \
      liblua5.2 \
      libsqlite3-0 \
      libboost-all-dev \
      libboost-date-time1.62.0 \
      libboost-filesystem1.62.0 \
      libboost-program-options1.62.0 \
      libboost-regex1.62.0 \
      libboost-system1.62.0 \
      libboost-thread1.62.0 \
      libboost-iostreams1.62.0 \
      cmake \
      libzmq3-dev \
      ca-certificates \
      clang-format-4.0

# This commit contains the modification in valhalla to use TaxiCost
RUN git clone --depth=1 --recursive https://github.com/valhalla/valhalla.git libvalhalla \
  && cd libvalhalla \
  && git checkout 2345ba1f4936b9b547f1e0e43ba18c1d733a5e52

RUN mkdir -p libvalhalla/build \
  && cd libvalhalla/build \
  && cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_SERVICES=Off -DENABLE_NODE_BINDINGS=Off -DENABLE_PYTHON_BINDINGS=Off -DBUILD_SHARED_LIBS=Off -DBoost_USE_STATIC_LIBS=ON \
  && make -j$(nproc) install && ldconfig

RUN apt-get -y purge \
      autoconf \
      automake \
      libtool \
      libgeos-dev \
      libgeos++-dev \
      liblua5.2-dev \
      unzip \
      python-all-dev \
      vim-common \
      jq \
      gucharmap \
 && apt-get autoremove -y && apt-get clean

