FROM fedora:27
RUN dnf install git -y
RUN git clone https://github.com/senior7515/smf.git
WORKDIR smf/
RUN /smf/tools/docker-deps.sh
RUN git submodule update --init --recursive
RUN ./install-deps.sh
RUN ./src/third_party/seastar/install-dependencies.sh
RUN ./tools/build.sh -rbt

EXPOSE 7000
