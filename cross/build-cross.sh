#!/bin/bash

# This script assumes you have the development packages for 
# lipgmp, libmpfr, and libmpc already installed.
#
# To rebuild the binaries with a fresh download, delete
# 'build-binutils' and 'build-gcc'

export PREFIX=$(pwd)
export TARGET=x86_64-elf
export PATH="$PREFIX/bin:$PATH"

if ! [[ -d build-binutils ]] then
then
    wget https://ftp.gnu.org/gnu/binutils/binutils-2.32.tar.gz
    tar -zxvf binutils-2.32.tar.gz

    mkdir build-binutils
fi

cd build-binutils
../binutils-2.32/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make
make install

cd ..

if ! [[ -d build-gcc ]] then
then
    wget https://ftp.gnu.org/gnu/gcc/gcc-9.2.0/gcc-9.2.0.tar.gz
    tar -zxvf gcc-9.2.0.tar.gz

    mkdir build-gcc
fi

cd build-gcc
../gcc-9.2.0/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers
make all-gcc
make all-target-libgcc
make install-gcc
make install-target-libgcc

cd ../
