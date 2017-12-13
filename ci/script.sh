#!/bin/bash

set -e
set -x

THIS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR=${THIS_DIR}/../

if [ ! -z ${DOCKER_IMAGE+x} ]; then
  # hard drive bloat warning... re-runing builds and unit tests takes too long
  # if the dependencies must be installed in the docker image each time. here
  # we build a base image with all the dependencies to speed things up. but
  # these images can be large, typically over 1 GB each. set NO_DEP_IMG if you
  # want to re-install the dependencies each time.
  if [ -n "${NO_DEP_IMG}" ]; then
    depimg=${DOCKER_IMAGE}
    pre_run="./install-deps.sh"
  else
    depid=$(md5sum ${ROOT_DIR}/install-deps.sh | cut -c -32)
    depimg=smf_test_deps_${DOCKER_IMAGE//:/_}_${depid}
    if [[ "$(docker images -q ${depimg} 2> /dev/null)" == "" ]]; then
      # remove the cidfile file to make docker run happy
      cidfile=$(mktemp); rm ${cidfile}
      docker run -it --cidfile=${cidfile} \
        -e USE_CLANG=1 \
        -v ${ROOT_DIR}/install-deps.sh:/tmp/install-deps.sh:z,ro \
        ${DOCKER_IMAGE} /tmp/install-deps.sh
      cid=$(cat ${cidfile})
      docker commit ${cid} ${depimg}
      rm ${cidfile}
    fi
    pre_run=
  fi

  docker run --rm -v ${ROOT_DIR}:/smf:z,ro \
    -e USE_CLANG -e USE_NINJA -t -w /smf ${depimg} \
    /bin/bash -c "${pre_run} ./ci/run.sh"
else
  ${ROOT_DIR}/install-deps.sh
  ${ROOT_DIR}/ci/run.sh
fi
