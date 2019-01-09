ARG BASE
FROM ${BASE}
ENV CI=1
ENV TRAVIS=1
COPY . /smf
RUN /smf/tools/docker-deps.sh
RUN /smf/install-deps.sh
