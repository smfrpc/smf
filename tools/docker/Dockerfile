FROM fedora:latest
RUN dnf install git -y
RUN git clone https://github.com/smfrpc/smf.git
WORKDIR smf/
RUN /smf/tools/docker-deps.sh
RUN git submodule update --init --recursive
RUN ./install-deps.sh
RUN ./tools/build.sh -r

EXPOSE 7000
