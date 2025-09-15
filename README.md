# Exathread
#### Go beyond C++ threading  

<img src="exathread_logo.png" width="200px" />

## About
Exathread is a powerful header-only C++ thread pool management library for C++20.

## Features
* Lock-free task queue
* Future aggregation
* Task continuations (like JS `Promise.then`)
* Coroutine-based task suspension for blocking tasks
* Automatic batch job parallelization
* No external dependencies
* Simple and clean API

## Usage
CMake and Meson build definitions are provided to make it easier to use the library, however you can also just copy `exathread.hpp` to wherever it's needed.  

## Documentation
Documentation is built and deployed automatically to https://robotleopard86.github.io/Exathread.  
If you want to build it yourself, everything is located in the `docs` folder. See the [docs build instructions page](docs/README.md) for more information.

## Licensing
Exathread is licensed under the Apache License 2.0, which can be found in the root directory. All third-party licenses are present in the `licenses` directory.
