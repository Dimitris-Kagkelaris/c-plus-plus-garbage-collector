#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "usage: $0 {release|test|leakcheck|debug|clean}" >&2
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
    release) opts=(-DOPTIMIZE=ON  -DTESTS=OFF -DSANITIZE=OFF) ;;
    test)    opts=(-DOPTIMIZE=ON  -DTESTS=ON  -DSANITIZE=OFF) ;;
    leakcheck) opts=(-DOPTIMIZE=OFF -DTESTS=ON  -DSANITIZE=OFF) ;;
    debug)   opts=(-DOPTIMIZE=OFF -DTESTS=ON  -DSANITIZE=ON)  ;;
    *)       usage ;;
esac

dir="build/$mode"

cmake -S . -B "$dir" "${opts[@]}"
cmake --build "$dir" --parallel

if [[ $mode != release ]]; then
    ctest --test-dir "$dir" --output-on-failure
fi

if [[ $mode == leakcheck ]]; then
    MallocStackLogging=1 leaks --atExit -- "$dir/tests/cppgc_tests"
fi