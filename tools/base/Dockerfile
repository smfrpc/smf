ARG BASE
FROM ${BASE}
COPY . /smf
RUN /smf/tools/docker-deps.sh
RUN /smf/install-deps.sh
RUN /smf/src/third_party/seastar/install-dependencies.sh
