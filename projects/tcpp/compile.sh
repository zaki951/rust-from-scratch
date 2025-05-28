
CXX_FILES="test_tuntap.cpp checksum_tcp.cpp IpParse.cpp" 
EXE_NAME="tuntap"
CXX_VERSION="-std=c++23"
DEBUG="-g -DDEBUG_BAR"
WERROR="-Werror"
CXX_ARGS="$CXX_VERSION  $DEBUG $WERROR -o $EXE_NAME" 

function compile() {
    rm $EXE_NAME
    echo "g++ $CXX_FILES $CXX_ARGS" 
    g++ $CXX_FILES $CXX_ARGS 
    return $?
}


if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    compile 
fi
