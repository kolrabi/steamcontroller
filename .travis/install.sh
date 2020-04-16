#!/bin/bash

set -e
set -x

if [[ "$(uname -s)" == 'Linux' ]]; then
    # Also install -multilib to cross-compile from x86-64 to x86
    # https://stackoverflow.com/questions/4643197/missing-include-bits-cconfig-h-when-cross-compiling-64-bit-program-on-32-bit
    sudo apt-get install gcc-${GCC_VERSION} g++-${GCC_VERSION} gcc-${GCC_VERSION}-multilib g++-${GCC_VERSION}-multilib libudev-dev 
    sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-${GCC_VERSION} 60 --slave /usr/bin/g++ g++ /usr/bin/g++-${GCC_VERSION}
    sudo update-alternatives --config gcc
    python3 --version
    sudo pip3 install cmake==3.13.3
    pip3 install --upgrade pip --user
    pip --version
    pip install conan --upgrade --user
    pip install conan_package_tools --user
    cmake --version
    conan --version
    conan profile new default --detect --force
    conan profile update settings.compiler.libcxx=libstdc++11 default
fi

