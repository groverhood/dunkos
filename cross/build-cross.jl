# This script is rather lengthy in time due to the process of building a gcc
# cross compiler for x86_64-elf executables.

using Glob;

PREFIX = abspath(dirname(PROGRAM_FILE));
TARGET = "x86_64-elf";

BINUTILS = "binutils-2.32";
GCC = "gcc-9.2.0";
ENV["PATH"] = "$PREFIX/bin:$(ENV["PATH"])";


cd(dirname(PROGRAM_FILE)) do
    rm.(vcat(glob("*/"), glob("*.tar.gz")), recursive=true, force=true);
    download("https://ftp.gnu.org/gnu/binutils/$BINUTILS.tar.gz", "$BINUTILS.tar.gz");
    run(`tar -zxvf binutils-2.32.tar.gz`);
    mkdir("build-binutils");

    cd("build-binutils") do
        run(`../binutils-2.32/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror`);
        run(`make`);
        run(`make install`);
    end

    download("https://ftp.gnu.org/gnu/gcc/$GCC/$GCC.tar.gz", "$GCC.tar.gz");
    run(`tar -zxvf gcc-9.2.0.tar.gz`);
    mkdir("build-gcc");

    cd("gcc-9.2.0") do
        run(`contrib/download_prerequisites`);
    end

    cd("build-gcc") do
        run(`../gcc-9.2.0/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers `);
        run(`make all-gcc`); 
        run(`make all-target-libgcc`); 
        run(`make install-gcc`); 
        run(`make install-target-libgcc`);
    end
end