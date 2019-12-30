#include <prog.h>
#include <system.h>
#include <init.h>

/* The user-provided main procedure. */
extern int main(int argc, char *argv[]);

void _start(int argc, char *argv[])
{
    /* Initialize userland libraries. */
    initusr();

    /* Execute the implemented main procedure, and exit with its procured
       status code. */
    exit(main(argc, argv));
}