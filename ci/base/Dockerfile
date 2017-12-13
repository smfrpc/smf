ARG BASE
FROM ${BASE}

# install smf dependencies
COPY install-deps.sh /src/smf/
RUN /src/smf/install-deps.sh

# build seastar
RUN apt-get install -y git || yum install -y  git || true
RUN git clone --recursive \
  https://github.com/scylladb/seastar.git /src/seastar && \
  cd /src/seastar && \
  git checkout 0ed15e9 && \
  ./install-dependencies.sh
RUN cd /src/seastar && \
  ./configure.py --mode=release && \
  ninja

COPY . /src/smf
