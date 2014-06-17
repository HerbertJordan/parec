# A C++ parallel library for recursive algorithms and data structures
A utility set of recursive algorithms and data structures, including degraded cases like list for building composable, parallel applications.

## How to Build

```
mkdir build
cd build
LIBS_HOME=~/libs cmake <path to local repo>
make -j && make test && make valgrind
```
