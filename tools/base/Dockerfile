ARG BASE
FROM ${BASE}
# Always install git
RUN apt-get install -y git || yum install -y  git || true


# install smf dependencies
COPY install-deps.sh /src/smf/
RUN /src/smf/install-deps.sh


COPY src/third_party/seastar/install-dependencies.sh /src/smf/seastar-deps.sh
RUN /src/smf/seastar-deps.sh

COPY . /src/smf
