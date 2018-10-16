# GameAP Daemon
The server management daemon

Branches        | Build         |
----------------|-------------- | 
develop         |[![Build Status](https://travis-ci.org/GameAP/GDaemon2.svg?branch=develop)](https://travis-ci.org/GameAP/GDaemon2)       |
master          |[![Build Status](https://travis-ci.org/GameAP/GDaemon2.svg?branch=master)](https://travis-ci.org/GameAP/GDaemon2)        |

- [Compiling](#compiling)
  - [Dependencies](#dependencies)
  - [Linux](#compiling-on-linux)
  - [Windows](#compiling-on-windows)
- [Configurating](#configurating)

## Compiling

### Dependencies

* [Binn Library](https://github.com/liteserver/binn)
* [Rest Client C++](https://github.com/mrtazz/restclient-cpp)
* CURL Library
* [Json C++ Library](https://github.com/open-source-parsers/jsoncpp)
* Boost Libraries: Boost System, Boost Iostreams, Boost Thread, Boost Process

### Compiling on Linux

```
git clone https://github.com/GameAP/GDaemon2
cd GDaemon2
cmake .
make gameap-daemon
```

### Compiling on Windows
Coming soon
