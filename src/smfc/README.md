# **smfc** is the code generator & compiler


To build *ONLY* the compiler:

```
cd $ROOT
mkdir build
cd build
cmake -DSMF_ENABLE_TESTS=OFF  \
      -DSMF_BUILD_PROGRAMS=OFF  $ROOT
make
```
