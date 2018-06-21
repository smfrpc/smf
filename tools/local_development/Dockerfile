FROM fedora:27

MAINTAINER Bartek Kowalczyk <smf-dev@googlegroups.com >

RUN yum install git -y

RUN git clone https://github.com/senior7515/smf.git
WORKDIR smf/
RUN git submodule update --init --recursive 
RUN ./install-deps.sh
RUN ./src/third_party/seastar/install-dependencies.sh
RUN ./tools/run.sh
RUN cmake CMakeLists.txt 
RUN make 

EXPOSE 7000