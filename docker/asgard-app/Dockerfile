FROM navitia/asgard-build-deps:latest

RUN git clone --depth=1 https://github.com/canaltp/asgard asgard

WORKDIR /asgard
RUN sed -i 's,git\@github.com:\([^/]*\)/\(.*\).git,https://github.com/\1/\2,' .gitmodules && git submodule update --init --recursive \
  && mkdir build && cd build \
  && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j$(nproc) && cp asgard/asgard /usr/bin/asgard \
  && cd - && rm -rf asgard

EXPOSE 6000
ENTRYPOINT ["/usr/bin/asgard"]
