#!/bin/bash

AROS_DEVELOPMENT=/ssd/deadwood/repo-github-dd-alt-abiv0/cross-i386-aros/Development

show_selection ()
{
    printf "    1)  i386-aros\n"
}

process_selection ()
{
    case $1 in
        1)
        BUILD_DIR=$(pwd)/cross-build-i386-aros
        ;;
    esac
}


main ()
{
    printf "rebuild v1.0, select an option:\n"
    printf "    0)  exit\n"

    show_selection

    read input
    if [[ $input = 0 ]]; then
        exit 0
    fi

    process_selection $input

    rm -rf $BUILD_DIR
    mkdir $BUILD_DIR
    cd $BUILD_DIR


    cmake \
        -DCMAKE_CROSSCOMPILING=ON \
        -DCMAKE_TOOLCHAIN_FILE=../Source/cmake/AROS.cmake \
        -DCMAKE_EXE_LINKER_FLAGS=-static-libstdc++ \
        -DCMAKE_BUILD_TYPE=Release \
        -DPORT=MUI \
        -DCMAKE_SYSTEM_PROCESSOR=i386 \
        -DCAIRO_INCLUDE_DIRS=$AROS_DEVELOPMENT/include/cairo/ \
        -DFREETYPE_INCLUDE_DIRS=$AROS_DEVELOPMENT/include/freetype/ \
        -DLIBXML2_INCLUDE_DIR=$AROS_DEVELOPMENT/include/libxml2/ \
       ..
}

main

