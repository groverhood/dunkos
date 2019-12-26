#include <prog.h>
#include <system.h>

/* The user-provided main procedure. */
extern int main(int argc, char *argv[]);

void _start(int argc, char *argv[])
{
    exit(main(argc, argv));
}