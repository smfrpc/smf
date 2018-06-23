ARG BASE
FROM ${BASE}
# Always install git
RUN apt-get install -y git || yum install -y  git || true
COPY . /smf
COPY .git /smf/

RUN /smf/install-deps.sh
RUN /smf/src/third_party/seastar/install-dependencies.sh
