# DunkOS Tools -- mkusr --
#
# Generate the user filesystem from a user-provided path which either points to
# a directory, which will be designated as the root from which all of its sub-
# contents originate, or a singular file (most commonly for testing purposes).
#
# Arguments
# ---------
#
# inputfile [positional (0)] -- The input file or directory.
#
# Options
# -------
#
# --nusr -- Do not include the software inside usr/ if this option is set.

struct Directory

end

if length(ARGS) != 1
    error("Invalid number of arguments given ($(length(ARGS)))");
end

(inputfile, _) = ARGS;

if isdir(inputfile)

else

end