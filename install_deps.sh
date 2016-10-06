#!/bin/sh

DEPS_DIR=`pwd`/deps
DEPS_SRC_DIR=$DEPS_DIR/src
DEPS_BUILD_DIR=$DEPS_DIR/build

# Create deps folder
mkdir -p $DEPS_SRC_DIR $DEPS_BUILD_DIR

# GoogleTest
cd $DEPS_SRC_DIR
## Fetch the GoogleTest sources
git clone https://github.com/google/googletest.git && cd googletest/googletest && \
mkdir build && cd build && \
cmake .. -DCMAKE_INSTALL_PREFIX=$DEPS_BUILD_DIR/gtest && \
make && make install
