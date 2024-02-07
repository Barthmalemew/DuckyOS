#!/bin/bash

if ! [ $BUILDOS ]
then 
    export BUILDS=1
    export PREFIX="$PWD/../"
    export TARGET=i686-elf
    export PATH="$PREFIX/bin:$PATH"
fi
