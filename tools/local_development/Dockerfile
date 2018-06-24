FROM fedora:27
RUN dnf install git -y
RUN git clone https://github.com/senior7515/smf.git
WORKDIR smf/
RUN git submodule update --init --recursive
RUN ./install-deps.sh
RUN ./src/third_party/seastar/install-dependencies.sh
RUN ./tools/install_bazel.sh
RUN ./tools/build -rbt

EXPOSE 7000
