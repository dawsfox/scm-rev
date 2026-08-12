# Rev SCM Implementation

To properly run the Rev SCM implementation, it is required that you have the VerilatorSST picorv32 component built and registered with SST.
In order to accomplish that, you will have to build the verilator-sst repo, which has the following prerequisites:
- Verilator >v5.022 (Version 5.026 or greater is required for inout port support)
- SST >16.0.0
- Python (>3.6.8)
- CMake (>3.24.2)

After installing these dependencies, run:
```
git clone https://github.com/tactcomplabs/verilator-sst.git
cd verilator-sst
mkdir build && cd build
cmake ../
make
make install
```
You can also run `make test` to verify the installation works properly.

Now to build Rev SCM, the only additional requirement is a RISCV toolchain.
To build, follow the below steps (see `README.md` for additional info):
```
    git clone https://github.com/dawsfox/scm-rev.git
    git config core.hooksPath .githooks
    cd rev/build
    cmake -DRVCC=/path/to/riscv/c/compiler/exe -DRVCXX=/path/to/riscv/c++/compiler/exe ..
    make -j
    make install
```
Because this project is an early proof-of-concept, you will need to make a few changes 
by hand for the project to be functional. Manually copy the following files from the 
`verilator-sst` repo (after building) and place the `.cpp` and `.h` files into `scm-rev/src/`
and `scm-rev/include/` respectively:
- `verilatorSSTpicorv32.cpp`
- `verilatorComponent.cpp`
- `verilatorSSTAPI.h`
- `verilatorSSTpicorv32.h`
- `verilatorComponent.h`
To verify the installation is functional, you can run `make test`. That being said,
the only test specific to `scm-rev` currently can be run alone with `ctest -R verilator-sst`.
