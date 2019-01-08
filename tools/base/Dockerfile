ARG BASE
FROM ${BASE}
ENV TRAVIS=1
COPY . /smf
RUN /smf/tools/docker-deps.sh
RUN /smf/install-deps.sh
