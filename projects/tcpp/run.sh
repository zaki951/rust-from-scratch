#!/bin/bash
source compile.sh

pid=""

function execute() {
    echo "execute..."
    sudo ./$EXE_NAME &
    pid=$!
    echo $pid
}

function init_interface() {
    sudo ip addr add 10.0.0.1/24 dev tun0 &&
    sudo ip link set up dev tun0
    return $?
}

function wait_prog() {
    trap "kill -9 $pid" INT TERM
    wait $pid
}

compile &&
execute && 
init_interface && 
wait_prog
