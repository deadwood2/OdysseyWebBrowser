#!/bin/bash

show_selection ()
{
    printf "    1)  i386-aros\n"
    printf "    2)  x86_64-aros\n"
}

process_selection ()
{
    case $1 in
        1)
        BUILD_PROCESSOR=i386
        SDK_DIR=/ssd/deadwood/repo-github-dd-alt-abiv0/cross-$BUILD_PROCESSOR-aros/Development
        ;;
        2)
        BUILD_PROCESSOR=x86_64
        SDK_DIR=/ssd/deadwood/repo-github-dd-core/cross-$BUILD_PROCESSOR-aros/Development
        ;;
    esac

    BUILD_DIR=$(pwd)/cross-build-$BUILD_PROCESSOR-aros
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
        -DCMAKE_TOOLCHAIN_FILE=../Source/cmake/AROS.cmake \
        -DCMAKE_EXE_LINKER_FLAGS=-static-libstdc++ \
        -DCMAKE_BUILD_TYPE=Release \
        -DPORT=MUI \
        -DCMAKE_SYSTEM_PROCESSOR=$BUILD_PROCESSOR \
        -DCMAKE_C_COMPILER=$BUILD_PROCESSOR-aros-gcc \
        -DCMAKE_CXX_COMPILER=$BUILD_PROCESSOR-aros-g++ \
        -DCAIRO_INCLUDE_DIRS=$SDK_DIR/include/cairo/ \
        -DFREETYPE_INCLUDE_DIRS=$SDK_DIR/include/freetype/ \
        -DLIBXML2_INCLUDE_DIR=$SDK_DIR/include/libxml2/ \
        -DZLIB_LIBRARIES=$SDK_DIR/lib/libz.a \
       ..
}

main

