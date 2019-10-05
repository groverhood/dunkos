#!/bin/bash

export PREFIX=$(pwd)
export TARGET=x86_64-elf
export PATH="$PREFIX/bin:$PATH"

wget -q -N https://ftp.gnu.org/gnu/binutils/binutils-2.32.tar.gz
tar -zxf binutils-2.32.tar.gz

mkdir build-binutils
cd build-binutils
../binutils-2.32/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make
make install

cd ..

wget -q -N https://ftp.gnu.org/gnu/gcc/gcc-9.2.0/gcc-9.2.0.tar.gz
tar -zxf gcc-9.2.0.tar.gz

mkdir build-gcc
cd build-gcc
../gcc-9.2.0/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers
make all-gcc
make all-target-libgcc
make install-gcc
make install-target-libgcc

cd ../
