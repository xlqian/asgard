FROM navitia/prime-server

VOLUME /data/valhalla
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
      wget \
      unzip \
      lua5.2 \
      python-all-dev \
      vim-common \
      jq \
      gucharmap \
      libprotobuf9 \
      libgeos-3.4.2 \
      liblua5.2 \
      libsqlite3-0 \
      libboost-all-dev \
      libboost-date-time1.55.0 \
      libboost-filesystem1.55.0 \
      libboost-program-options1.55.0 \
      libboost-regex1.55.0 \
      libboost-system1.55.0 \
      libboost-thread1.55.0 \
      libboost-iostreams1.55.0 \
      cmake \
      libzmq-dev \
  && git clone --depth=1 --recursive https://github.com/valhalla/valhalla.git libvalhalla \
  && cd libvalhalla \
  && ./autogen.sh && ./configure --enable-services=no --enable-static && make -j4 install && make clean && ldconfig \
  && cd - && rm -rf libvalhalla
RUN git clone --depth=1 https://github.com/canaltp/asgard asgard && cd asgard \
  && sed -i 's,git\@github.com:\([^/]*\)/\(.*\).git,https://github.com/\1/\2,' .gitmodules && git submodule update --init \
  && mkdir build && cd build \
  && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j4 && cp asgard/asgard /usr/bin/asgard \
  && cd - && rm -rf asgard \
  && apt-get -y purge \
      git \
      autoconf \
      automake \
      make \
      libtool \
      pkg-config \
      g++ \
      gcc \
      protobuf-compiler \
      libboost-all-dev \
      libprotobuf-dev \
      libgeos-dev \
      libgeos++-dev \
      liblua5.2-dev \
      wget \
      unzip \
      python-all-dev \
      vim-common \
      jq \
      gucharmap \
      cmake \
      libboost-all-dev \
 && apt-get autoremove -y && apt-get clean


EXPOSE 6000
CMD ["/usr/bin/asgard"]
