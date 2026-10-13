#!/usr/bin/env bash

function pass() {
    echo -e "\e[32mpass:\e[0m $@"
}

function fail() {
    echo -e "\e[31mfail:\e[0m $@"
}

function main() {
    if [ ! -f ./be ]; then
        echo -e "\e[31mcompile ./be before running this script!\e[0m"
        exit 1
    fi
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
