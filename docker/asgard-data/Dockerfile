FROM navitia/asgard-dev:latest as builder
WORKDIR /data/valhalla

RUN apt-get update && apt-get install -y --no-install-recommends \
      wget \
      tar

# Download the pbf given in argument
ARG pbf_url=http://download.geofabrik.de/europe/france-latest.osm.pbf
RUN wget --progress=bar:force:noscroll $pbf_url \
  && mkdir -p valhalla_tiles \
  && valhalla_build_config --mjolnir-tile-dir ${PWD}/valhalla_tiles --mjolnir-tile-extract ${PWD}/valhalla_tiles.tar --mjolnir-timezone ${PWD}/valhalla_tiles/timezones.sqlite --mjolnir-admin ${PWD}/valhalla_tiles/admins.sqlite > valhalla.json \
  && valhalla_build_tiles -c valhalla.json *.pbf \
  && find valhalla_tiles | sort -n | tar cf valhalla_tiles.tar --no-recursion -T -

FROM alpine:latest
COPY --from=builder /data/valhalla/valhalla_tiles.tar /data/valhalla/
COPY --from=builder /data/valhalla/valhalla.json /data/valhalla/
VOLUME [ "/data/valhalla" ]
ENTRYPOINT date
