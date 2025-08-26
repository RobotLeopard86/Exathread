# Exathread
#### Go beyond C++ threading  

<img src="exathread_logo.png" width="200px" />

## About
Exathread is a powerful C++ thread pool management library for C++20.

## Features
* Work-stealing task queue
* Future aggregation
* Task continuation scheduling
* Yielding to avoid useless blocks

## Building
You will need:  
* Git
* Meson
* Ninja

Configure the build directory with `meson setup build --native-file native.ini`, then run `meson compile -C build` to build the library.  

As a note, on Windows, Exathread builds using the dynamic CRT by default and will enforce this on dependencies.  
If your app does not use the dynamic CRT, append `-Dcrt_static=true` to your configure command, or set the `crt_static` option to `true` in your `subproject` call. 

## Documentation
Documentation is built and deployed automatically to https://robotleopard86.github.io/Exathread.  
If you want to build it yourself, everything is located in the `docs` folder. See the [docs build instructions page](docs/README.md) for more information.

## Licensing
Exathread is licensed under the Apache License 2.0, which can be found in the root directory. All third-party licenses are present in the `licenses` directory.
