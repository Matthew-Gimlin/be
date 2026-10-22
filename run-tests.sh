#!/usr/bin/env bash

function pass() {
    echo -e "\e[32mpass:\e[0m $@"
}

function fail() {
    echo -e "\e[31mfail:\e[0m $@"
}

function checkerrorhandling() {
    local f rc
    for f in tests/errors/*.ir; do
        ./be $f &> /dev/null
        rc=$?
        if [ $rc -eq 0 ]; then
            fail "$f: failed to catch error"
        elif [ $rc -ne 1 ]; then
            fail "$f: unexpected return code $rc"
        else
            pass "$f: caught error"
        fi
    done
}

function main() {
    if [ ! -f ./be ]; then
        echo -e "\e[31mcompile ./be before running this script!\e[0m"
        exit 1
    fi
    checkerrorhandling
    local f temp
    for f in tests/*.ir; do
        temp=$(mktemp)
        if ! ./be $f -o $temp; then
            fail "$f: compilation failed"
        elif ! diff -B -w $temp $f.opt > /dev/null; then
            fail "$f: output differs from expected result"
        else
            pass "$f"
        fi
        rm $temp
    done
}

main
