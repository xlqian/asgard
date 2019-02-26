FROM navitia/asgard-build-deps:latest as builder

RUN git clone --depth=1 https://github.com/canaltp/asgard asgard

WORKDIR /asgard
RUN sed -i 's,git\@github.com:\([^/]*\)/\(.*\).git,https://github.com/\1/\2,' .gitmodules && git submodule update --init --recursive \
  && mkdir build && cd build \
  && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j$(nproc)

FROM debian:9
COPY --from=builder /asgard/build/asgard/asgard /usr/bin/asgard
COPY --from=builder /usr/lib/ /usr/lib/
COPY --from=builder /lib/ /lib/
EXPOSE 6000
ENTRYPOINT ["/usr/bin/asgard"]
