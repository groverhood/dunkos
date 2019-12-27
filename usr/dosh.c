#include <stdio.h>
#include <system.h>

#define _stringify(x) #x
#define stringify(x) _stringify(x)
#define RDBFSZ 255

static char **parseargs(char *cmd)
{
    
}

int main(int argc, char *argv[])
{
    static char rdbuf[RDBFSZ + 1];
    while (scanf("%" stringify(RDBFSZ) "s\n", rdbuf) == 1) {
        char **subargv = parseargs(rdbuf);
        pid_t pr = fork();
        if (pr == 0) {
            exec(subargv[0], subargv);
            exit(-1);
        } else {
            wait(pr);
        }
    }
}