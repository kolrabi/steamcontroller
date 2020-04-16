#!/bin/bash

set -e
set -x

if [[ "$(uname -s)" == 'Darwin' ]]; then
    source ~/venv3.8.1/bin/activate
    python --version

    pip install conan --upgrade
    pip install conan_package_tools
    conan --version
    conan user
fi
