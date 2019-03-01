### NOTICE

To build Asgard:
```bash
git clone https://github.com/CanalTP/asgard.git asgard
cd asgard
git clone --depth=1 --recursive https://github.com/valhalla/valhalla.git libvalhalla
mkdir -p libvalhalla/build
cd libvalhalla/build
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo -DENABLE_SERVICES=Off -DENABLE_NODE_BINDINGS=Off -DENABLE_PYTHON_BINDINGS=Off -DBUILD_SHARED_LIBS=Off -DBoost_USE_STATIC_LIBS=ON -DProtobuf_USE_STATIC_LIBS=ON -DLZ4_USE_STATIC_LIBS=ON
make -j$(nproc) install && ldconfig
cd ../.. && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
make -j$(nproc)
```

The executable will be located in build/asgard.

You can also use the docker located in the docker directory:
```bash
cd docker/asgard-dev
docker build .
```

To run the unit tests :
```bash
cd build/asgard
ctest
```

Clang-format-4.0 is used to format our code.
A pre-commit hook can be used to format de code before each commit.
Before you can run hooks, you need to have the pre-commit package manager installed.
Using pip:
```bash
pip install pre-commit
```

To install the pre-commit hooks just do:
```bash
pre-commit install
```

And if you just want to use one pre-commit hook do:
```bash
pre-commit run <hook_id>
```

The CI will fail if the code is not correctly formated so be careful !

### DOCKER

There are 3 differents docker images in the project :

    - asgard-build-deps : compiles and installs a specific release of Valhalla and contains all the dependencies to run Asgard.
    - asgard-data : creates the configuration file to run asgard and the tiles of a specific pbf. By default it contains the tiles of the whole France.
    - asgard-app : compiles and run Asgard with the data of a asgard-data container.

To run Asgard with docker :
```bash
# Data of the whole France
docker create --name asgard-data navitia/asgard-data:latest
docker run --volumes-from asgard-data --rm -it -p 6000:6000/tcp navitia/asgard-app:latest
```
#### What if I want to use other data ?

It is possible to create an image from the asgard-data dockerfile with any pbf, tag it and create a container from that tagged image.
Let's say you want to create an image from the german pbf and tag it "germany" :
```bash
cd docker/asgard-data
# Create the image and tag it "germany"
docker build --build-arg pbf_url=http://download.geofabrik.de/europe/germany-latest.osm.pbf -t navitia/asgard-data:germany .
# Create a container from this image
docker create --name asgard-data navitia/asgard-data:germany
```
