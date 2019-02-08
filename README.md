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
