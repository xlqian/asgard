FROM navitia/asgard-build-deps:latest as builder
WORKDIR /data/valhalla

RUN apt-get update && apt-get install -y --no-install-recommends \
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
  && valhalla_build_tiles -c valhalla.json *.pbf \
  && find valhalla_tiles | sort -n | tar cf valhalla_tiles.tar --no-recursion -T - \
  # Make the graph shareable between threads in a thread-safe way
  # This has to be set after the creation of the tiles to avoid seg faults
  && sed -i '/"hierarchy":/a \\t"global_synchronized_cache": true,' valhalla.json

# Create an healthcheck script which will be run by asgard-app
ARG from_coords=""
ARG to_coords=""
RUN echo "/asgard-query directpath \"$from_coords\" \"$to_coords\"" >> healthcheck.sh \
  && chmod +x healthcheck.sh

FROM alpine:latest
# We have to create a group and a user with the same gid and uid than the docker-app
RUN addgroup -S asgard-user --gid 1234 && adduser -S asgard-user -G asgard-user --uid 1234
COPY --from=builder /data/valhalla/valhalla_tiles.tar /data/valhalla/
COPY --from=builder /data/valhalla/valhalla.json /data/valhalla/
COPY --from=builder /data/valhalla/healthcheck.sh /data/valhalla/
RUN chown -R asgard-user:asgard-user /data/valhalla
USER asgard-user
VOLUME [ "/data/valhalla" ]
ENTRYPOINT date
