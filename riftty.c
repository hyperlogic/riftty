#include <stdio.h>
#include "pty.h"

int main(int argc, char *argv[])
{
    struct Pty *pty;

    if (!Pty_Make(&pty)) {
        fprintf(stderr, "Pty_Make failed");
    }

    sleep(1);

    const size_t BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];
    int size;
    while((size = Pty_Read(pty, buffer, BUFFER_SIZE)) > 0) {
        fprintf(stdout, "%s", buffer);
    }
    Pty_Send(pty, "pwd\n", 4);

    while(1) {
        size = Pty_Read(pty, buffer, BUFFER_SIZE);
        if (size > 0)
            fprintf(stdout, "%s", buffer);
    }

    return 0;
}
