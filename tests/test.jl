
TARGET = "x86_64-elf";
BUILD_DIR = ".build";

cd(dirname(PROGRAM_FILE)) do
    if isdir(BUILD_DIR)
        rm("$BUILD_DIR", recursive=true, force=true);
    end

    mkdir(BUILD_DIR);
    cwd = pwd();
    tests = filter(file -> isdir(file) && file != BUILD_DIR, readdir(cwd));
    for test in tests
        mkdir("$BUILD_DIR/$test");
        cd("$BUILD_DIR/$test") do
            path = pwd();
            run(`gcc-9 -O2 -o $test.out $cwd/$test/$test.c`);
            expected = read(`./$test.out`, String);
            write("$test.ck", expected);
        end
    end
end
