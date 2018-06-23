ARG BASE
FROM ${BASE}
# Always install git
RUN apt-get install -y git || yum install -y git || true

COPY . /smf
COPY .git /smf/

RUN /smf/install-deps.sh
RUN /smf/src/third_party/seastar/install-dependencies.sh

# XXX add bazel to test multiple language support
# ENV BAZEL_VERSION=0.14.1
# RUN curl -fSsL -O https://github.com/bazelbuild/bazel/releases/download/$BAZEL_VERSION/bazel-$BAZEL_VERSION-installer-linux-x86_64.sh && \
# 	chmod +x bazel-*.sh && \
# 	./bazel-$BAZEL_VERSION-installer-linux-x86_64.sh
