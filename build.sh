#!/bin/bash

# This suite is based on Ubuntu package release of Clang 12.
#
# Install instruction: https://apt.llvm.org/
#
# The following packages are installed.
#
# sudo apt-get install clang-12 clang-tools-12 clang-12-doc libclang-common-12-dev libclang-12-dev libclang1-12 clang-format-12 clangd-12
# sudo apt-get install libc++-12-dev libc++abi-12-dev

pwd=$(pwd)
export CC=clang-12
export CXX=clang++-12
git clone https://github.com/google/benchmark.git
cd benchmark && git clone https://github.com/google/googletest.git 
cd $pwd
git clone https://github.com/orlp/pdqsort.git
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
NUM_PARALLELISM=$(nproc --all)
make -j $NUM_PARALLELISM all
cd $pwd
