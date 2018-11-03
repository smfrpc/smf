---
layout: page
title: getting started 
---

<p class="message">
  Nice! thanks for trying and don't forget to 
  send the mailing list an email if you get stuck! 
</p>


Currently we have tested the system to work on Fedora, Debian, Ubuntu. 

We require gcc7 or greater.

It is a fully compliant CMake project. If you know CMake you can skip this part.


```bash
git clone https://github.com/smfrpc/smf.git
cd smf
git submodule update --init --recursive

# Install seastar system deps
#

./src/third_party/seastar/install-dependencies.sh

# Install smf deps
#

./install-deps.sh


# We have a wrapper script to build the project
# try build.sh -h for more advanced usage!
# -r == release
#
./tools/build.sh -r
```

Please drop us a line and tell us what you are using 
**smf** for. 

## Sample programs:

Server:

```
# Run on one core (-c 1)! change --ip and --port as needed
#
./build/release/demo_apps/cpp/demo_server -c 1 
```

Client: 
```
# Run on one core (-c 1)! change --ip and --port as needed
#
./build/release/demo_apps/cpp/demo_client -c 1 
```


## Docker

Alternatively, you can use Dockerfile, 
enter `cd tools/local_development`, and then:

```bash
docker build -t smf . 
```

To run a shell inside the docker: 

```bash
docker run -it smf /usr/bin/bash
```

Note: This re-clones the repo inside the Docker image
from the `master` tag. It does not use the current local copy.
The base image is `fedora27`
