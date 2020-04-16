#!/bin/bash

set -e
set -x

if [[ "$(uname -s)" == 'Darwin' ]]; then
    source ~/venv3.8.1/bin/activate
    python build.py
fi
