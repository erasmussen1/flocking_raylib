#!/bin/bash
#
#
set -o nounset
# set -x

WORKING_DIR="$(cd $(dirname $0) && cd .. && pwd)"


die() {
    echo "$@"
    exit 1
}

install_benchmark()
{
    if [[ ! -e benchmark/ ]]
    then
        git clone https://github.com/google/benchmark.git
    fi

    # Building and installing
    mkdir -p benchmark/build
    pushd benchmark
    git pull
    pushd build
    cmake -DBENCHMARK_ENABLE_TESTING=OFF ..
    make
    sudo make install
    popd
    popd

    # Cleanup
    # [[ -e benchmark/ ]] && rm -rf benchmark/
}

pushd $WORKING_DIR

install_benchmark

popd

