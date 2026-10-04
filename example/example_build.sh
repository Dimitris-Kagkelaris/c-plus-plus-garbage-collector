#!/usr/bin/env bash
set -euo pipefail

cmake -S . -B build-example
cmake --build build-example --parallel
