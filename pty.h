#ifndef PTY_H
#define PTY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <util.h>
#include <sys/syslimits.h>

struct Pty
{
    int master_fd;
    pid_t child_pid;
    char name[PATH_MAX];
    struct termios term;
    struct winsize win;
};

// returns 1 on success 0 on failure
int Pty_Make(struct Pty **pty_out);

int Pty_Destroy(struct Pty *pty);

// returns 1 on succes 0 on failure
int Pty_Send(struct Pty *pty, const char *buffer, size_t size);

// returns number of bytes read into buffer.
int Pty_Read(struct Pty *pty, char *buffer, size_t size);

#ifdef __cplusplus
}
#endif

#endif
