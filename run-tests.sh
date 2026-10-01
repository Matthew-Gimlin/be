#!/bin/bash

shopt -s nullglob

function pass() {
    echo -e "\e[32mpass:\e[0m $@"
}

function fail() {
    echo -e "\e[31mfail:\e[0m $@"
}

function compare_ir() {
    diff -B -w -I '^;' $1 $2 > /dev/null
}

function main() {
    local f expected temp
    for f in tests/*.ir; do
        temp=$(mktemp)
        [ -f $f.opt ] && expected=$f.opt || expected=$f
        if ! ./be $f -o $temp; then
            fail "$f: compilation failed"
        elif ! diff -B -w -I '^;' $temp $expected > /dev/null; then
            fail "$f: output differs from expected result"
        else
            pass "$f"
        fi
        rm $temp
    done 2> /dev/null
}

main
