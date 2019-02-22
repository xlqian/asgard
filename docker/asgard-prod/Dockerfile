FROM navitia/asgard-dev:latest

RUN apt-get update && apt-get install -y --no-install-recommends

ARG BRANCH=master
RUN echo Cloning branch $BRANCH for Asgard \
  && git clone --depth=1 --branch $BRANCH https://github.com/canaltp/asgard asgard

WORKDIR /asgard
RUN sed -i 's,git\@github.com:\([^/]*\)/\(.*\).git,https://github.com/\1/\2,' .gitmodules && git submodule update --init --recursive \
  && mkdir build && cd build \
  && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j$(nproc) && cp asgard/asgard /usr/bin/asgard \
  && cd - && rm -rf asgard \
  && apt-get autoremove -y && apt-get clean

EXPOSE 6000
ENTRYPOINT ["/usr/bin/asgard"]
