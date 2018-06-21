ARG BASE
FROM ${BASE}

# install smf dependencies
COPY install-deps.sh /src/smf/
RUN /src/smf/install-deps.sh

# build seastar
RUN apt-get install -y git || yum install -y  git || true


COPY src/third_party/seastar/install-dependencies.sh /src/smf/seastar-deps.sh
RUN /src/smf/seastar-deps.sh

COPY . /src/smf
