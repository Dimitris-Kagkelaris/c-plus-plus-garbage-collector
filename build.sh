#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "usage: $0 {release|leakcheck|debug|clean}" >&2
    exit 1
}

[[ $# -eq 1 ]] || usage
mode=$1

# Run from the project root no matter where the script is called from
cd "$(dirname "$0")"

if [[ $mode == clean ]]; then
    rm -rf build
    exit 0
fi

case "$mode" in
    release)   opts=(-DCMAKE_BUILD_TYPE=Release -DCPPGC_BUILD_TESTS=ON -DCPPGC_SANITIZE=OFF) ;;
    leakcheck) opts=(-DCMAKE_BUILD_TYPE=Debug   -DCPPGC_BUILD_TESTS=ON -DCPPGC_SANITIZE=OFF) ;;
    debug)     opts=(-DCMAKE_BUILD_TYPE=Debug   -DCPPGC_BUILD_TESTS=ON -DCPPGC_SANITIZE=ON)  ;;
    *)         usage ;;
esac

dir="build/$mode"

cmake -S . -B "$dir" "${opts[@]}"
cmake --build "$dir" --parallel

ctest --test-dir "$dir" --output-on-failure

if [[ $mode == leakcheck ]]; then
    MallocStackLogging=1 leaks --atExit -- "$dir/tests/cppgc_tests"
fi