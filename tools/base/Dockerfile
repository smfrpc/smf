ARG BASE
FROM ${BASE}
# Always install git
RUN apt-get install -y git || yum install -y git || true

COPY . /smf

RUN /smf/install-deps.sh
RUN /smf/src/third_party/seastar/install-dependencies.sh
RUN /smf/tools/install_bazel.sh
