FROM navitia/asgard-build-deps:latest as builder
WORKDIR /data/valhalla

RUN apt-get update && apt-get install -y --no-install-recommends \
      wget \
      tar

# Download the pbf given in argument
ARG pbf_url=http://download.geofabrik.de/europe/france-latest.osm.pbf
RUN wget --progress=bar:force:noscroll $pbf_url \
  && mkdir -p valhalla_tiles \
  && valhalla_build_config --mjolnir-tile-dir ${PWD}/valhalla_tiles --mjolnir-tile-extract ${PWD}/valhalla_tiles.tar --mjolnir-timezone ${PWD}/valhalla_tiles/timezones.sqlite --mjolnir-admin ${PWD}/valhalla_tiles/admins.sqlite > valhalla.json \
  # These are default values set after empirically testing asgard with distributed.
  # They seem to give the most coherent results between the time of the matrix and the direct_path
  # And fixing some projection problems in dead-ends or isolated places.
  && sed -i 's,\"minimum_reachability\"\: [[:digit:]]*,\"minimum_reachability\"\: 0,' valhalla.json \
  && sed -i 's,\"radius\"\: [[:digit:]]*,\"radius\"\: 30,' valhalla.json \
  && sed -i 's,\"shortcuts\"\: [^,]*,\"shortcuts\"\: false,' valhalla.json \
  && sed -i 's,\"hierarchy\"\: [^,]*,\"hierarchy\"\: false,' valhalla.json \
  # Make the graph shareable between threads in a thread-safe way
  && sed -i '/"hierarchy":/a \\t"global_synchronized_cache": true,' valhalla.json \
  && valhalla_build_tiles -c valhalla.json *.pbf \
  && find valhalla_tiles | sort -n | tar cf valhalla_tiles.tar --no-recursion -T -

FROM alpine:latest
COPY --from=builder /data/valhalla/valhalla_tiles.tar /data/valhalla/
COPY --from=builder /data/valhalla/valhalla.json /data/valhalla/
VOLUME [ "/data/valhalla" ]
ENTRYPOINT date
