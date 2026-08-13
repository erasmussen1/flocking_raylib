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

install_gtest()
{
    if [[ ! -e googletest/ ]]
    then
        git clone https://github.com/google/googletest.git
    fi

    # Building and installing gtest
    pushd googletest/
    git pull

    # git co release-1.8.1

    cmake .
    make
    sudo make install
    popd

    # Cleanup
    # [[ -e googletest/ ]] && rm -rf googletest/
}

pushd $WORKING_DIR

install_gtest

popd

