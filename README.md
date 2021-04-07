# Asgard

Asgard is the interface between [Navitia](https://github.com/CanalTP/navitia) and [Valhalla](https://github.com/valhalla/valhalla). It can be used to compute street_network direct_paths and matrix.

## How to use

To use Asgard with Navitia, you will need to give Jormungandr a configuration file telling Navitia to use Asgard for every street_network_modes you want. This file should be located in the `INSTANCE_DIR` which is set in the [default_settings.py](https://github.com/CanalTP/navitia/blob/dev/source/jormungandr/jormungandr/default_settings.py#L9). Here's a (not so) basic one:

```json
{
  "key": "my_instance",
  "zmq_socket": "ipc:///tmp/my_instance_kraken",
  "street_network": [
    {
      "modes": [
        "car",
        "walking",
        "bike"
      ],
      "class": "jormungandr.street_network.asgard.Asgard",
      "args": {
        "service_url": "http://localhost",
        "asgard_socket": "tcp://localhost:6000",
        "timeout": 2000
      }
    },
    {
      "args": {
        "street_network": {
          "args": {
            "service_url": "http://localhost",
            "asgard_socket": "tcp://localhost:6000",
            "timeout": 2000
          },
          "class": "jormungandr.street_network.asgard.Asgard",
          "modes": []
        }
      },
      "class": "jormungandr.street_network.Taxi",
      "modes": [
        "taxi"
      ]
    }
  ]
}
```

You can then run Asgard via docker or from the sources.

### With docker and navitia-docker-compose

You can clone the repository navitia-docker-compose available [here](https://github.com/CanalTP/navitia-docker-compose).  
Then launch this command at the root directory of the repo 'navitia-docker-compose': 
`docker-compose -f asgard/docker-compose_asgard.yml up`

You will have a running Asgard docker with data from France.

To stop the docker, launch this command at the root directory of the repo 'navitia-docker-compose': 
`docker-compose -f asgard/docker-compose_asgard.yml down -v`

### From the sources

#### Build from sources

To build Asgard:
```bash
git clone https://github.com/CanalTP/asgard.git asgard
cd asgard/scripts
./build_asgard_for_ubuntu.sh -a <path_to_asgard_project>
```

This will build [Valhalla](https://github.com/valhalla/valhalla) in <path_to_asgard_project>/libvalhalla and install it in <path_to_asgard_project>/valhalla_install.

The executable of Asgard will be located in build/asgard.

Note: Already having a Valhalla library & include directory in respectively `/usr/local/lib` & `/usr/local/include` may lead to build errors as they currently take priority over the library & include directory specified in [build_asgard_for_ubuntu.sh](https://github.com/CanalTP/asgard/blob/ba831b3fed02f54c3cc84df04db3ae8f5141ee12/scripts/build_asgard_for_ubuntu.sh#L102).

#### Prepare data

To create the data download the pfbs you want here http://download.geofabrik.de/. Then run:
```bash
./create_asgard_data.sh -e <path_to_valhalla_executables> -i <path_of_pbf_dir> -o <path_of_output_dir>
```
By default <path_to_valhalla_executables> will be <path_to_asgard_project>/valhalla_install/bin

#### Run unit tests

To run the unit tests :
```bash
cd build/asgard
ctest
```

#### Run Asgard

You should have now Asgard compiled and a data ready to use.

```bash
# Export var env ASGARD_VALHALLA_CONF
export ASGARD_VALHALLA_CONF=/created_data_folder/valhalla.json

# Run Asgard
cd root_project/build/asgard
./asgard
```

The term ouput looks like that :

```bash
~/asgard/build/asgard » ./asgard
2021/02/01 12:41:32.110373 [INFO] Using default log configuration. Logs are going to std_out
2021/02/01 12:41:32.110428 [INFO] Config: ASGARD_SOCKET_PATH=tcp://*:6000
2021/02/01 12:41:32.110432 [INFO] Config: ASGARD_CACHE_SIZE=1000000
2021/02/01 12:41:32.110434 [INFO] Config: ASGARD_NB_THREADS=3
2021/02/01 12:41:32.110436 [INFO] Config: ASGARD_METRICS_BINDING=0.0.0.0:8080
2021/02/01 12:41:32.110440 [INFO] Config: ASGARD_VALHALLA_CONF=/created_data_folder/valhalla.json
2021/02/01 12:41:32.111061 [INFO] metrics available at http://0.0.0.0:8080/metrics
2021/02/01 12:41:32.111648 [INFO] Tile extract successfully loaded with tile count: 40
```

#### Install linters/formatter

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

#### Run linters/formatter

And if you just want to use one pre-commit hook do:
```bash
pre-commit run <hook_id>
```

The CI will fail if the code is not correctly formated so be careful !

## More about the dockers

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

## What if I want to use other data ?

It is possible to create an image from the asgard-data dockerfile with any pbf, tag it and create a container from that tagged image.
Let's say you want to create an image from the german pbf and tag it "germany" :
```bash
cd docker/asgard-data
# Create the image and tag it "germany"
docker build --build-arg pbf_url=http://download.geofabrik.de/europe/germany-latest.osm.pbf -t navitia/asgard-data:germany .
# Create a container from this image
docker create --name asgard-data navitia/asgard-data:germany
```
