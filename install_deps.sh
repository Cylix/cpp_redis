#!/bin/sh

DEPS_DIR=`pwd`/deps
DEPS_SRC_DIR=$DEPS_DIR/src

# Create deps folder
mkdir -p $DEPS_SRC_DIR

# GoogleTest
cd $DEPS_SRC_DIR
## Fetch the GoogleTest sources
git clone https://github.com/google/googletest.git
