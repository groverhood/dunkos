# MKUSR OPTS
# -c :: create an image file.
# -d :: source is a directory, otherwise an attempt to read a C source will be made.

using ArgParse: ArgParseSettings, parse_args, @add_arg_table;

struct DunkOSPartitionSpec
    divs::Cint;
    fs::Cint;
    swap::Cint;   
end

fmtdunkimg(imgpath::AbstractString; divs=4, swap=1, fs=3) = 
    ccall((:fmtdunkimg, "../tools/.build/dtools.so"),
            Cvoid, (Cstring, Ref{DunkOSPartitionSpec}), 
            imgpath, Ref(DunkOSPartitionSpec(divs, fs, swap)));

function copydir(dirname::AbstractString)
end

function main(args)
    settings = ArgParseSettings();
    @add_arg_table settings begin
        "-c", "--create"
            help = "create an image file"
            default = nothing
        "-o", "--out"
            help = "output disk image"
            default = "dunkos.img"
        "input"
            help = "input file/directory"
            required = true
    end
    parseresult = parse_args(args, settings, as_symbols=true);

    diskname = parseresult[:out];
    creatsz = parseresult[:create];
    if creatsz !== nothing
        run(`qemu-img create -f raw $diskname $creatsz`);
    end

    fmtdunkimg(diskname);
    input = parseresult[:input];

    if isdir(input)

    else

    end
end

main(ARGS);