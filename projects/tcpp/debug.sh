#!/bin/bash
source compile.sh

pid=""

function execute() {
    echo "execute..."
    sudo gdb ./$EXE_NAME -ex "run" -ex "set backtrace limit 0"
}

function init_interface() {
    sleep 10 &&
    sudo ip addr add 10.0.0.1/24 dev tun0 &&
    sudo ip link set up dev tun0
    return $?
}


init_interface & 
compile &&
execute 
