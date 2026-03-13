# Narwhal Emulation Framework

Narwhal is a modular emulation framework targeting Armv7-M-based microcontrollers.  

## Usage

```
$ ./build/narwhal --help
Usage: narwhal [--help] [--version] [--loglevel VAR] [--headless] [--trace] [--gdbstub] [--ezcov VAR] [--drcov VAR] [--generate-pystub VAR] config

Positional arguments:
  config             Path to the configuration file to use 

Optional arguments:
  -h, --help         shows help message and exits 
  -v, --version      prints version information and exits 
  --loglevel         Set the log level (default: info) 
  --headless         Start the emulator without a GUI 
  --trace            Trace all instructions to stdout (slow) 
  --gdbstub          Start the emulator with the debugger (halts at the first instruction and waits for a connection) 
  --ezcov            Output EZCOV coverage to file. 
  --drcov            Output DRCOV coverage to file. 
  --generate-pystub  Generate python stubs to folder.
```

## Getting started

Start by creating a new configuration file or using an existing one from the `configs/` directory. Specify any memory regions, peripherals, devices, and connectors needed for the emulation. Finally use the loaders to load any firmware into the memory regions.  

To start the emulation framework run:  
```
./build/narwhal <path to configuration>
```

## Fuzzing

To perform fuzzing with Narwhal, create a Python script with a basic fuzzing harness (see `scripts/fuzz.py`).  
To start fuzzing, run AFL++'s `afl-fuzz` and invoke Narwhal in headless mode:

```
afl-fuzz -U -m none -i <input> -o <output> -- ./build/narwhal --headless <path to narwhal config>
```

## Building

Start by installing the required dependencies:  
**For Arch Linux**:  
```
pacman -S gcc cmake capstone pybind11 sdl3 yaml-cpp mbedtls
```

Configure the project:  
```
cmake -S . -B build/ -DCMAKE_BUILD_TYPE:STRING=Release
```

Build the project:  
```
cmake --build build/
```
