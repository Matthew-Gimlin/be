#!/bin/bash

shopt -s nullglob

function pass() {
    echo -e "\e[32mpass:\e[0m $@"
}

function fail() {
    echo -e "\e[31mfail:\e[0m $@"
}

function main() {
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
    done 2> /dev/null
}

main
