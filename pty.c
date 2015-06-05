#include "pty.h"
#include <stdio.h>
#include <poll.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

int Pty_Make(struct Pty **pty_out)
{
    struct Pty *pty = calloc(1, sizeof(struct Pty));
    if (!pty) {
        return 0;
    }

    // NOTE: shamelessly copied from iTerm2 PTYTask.m
#define CTRLKEY(c) ((c)-'A'+1)

    int isUTF8 = 0;
    // UTF-8 input will be added on demand.
    pty->term.c_iflag = ICRNL | IXON | IXANY | IMAXBEL | BRKINT | (isUTF8 ? IUTF8 : 0);
    pty->term.c_oflag = OPOST | ONLCR;
    pty->term.c_cflag = CREAD | CS8 | HUPCL;
    pty->term.c_lflag = ICANON | ISIG | IEXTEN | ECHO | ECHOE | ECHOK | ECHOKE | ECHOCTL;

    pty->term.c_cc[VEOF] = CTRLKEY('D');
    pty->term.c_cc[VEOL] = -1;
    pty->term.c_cc[VEOL2] = -1;
    pty->term.c_cc[VERASE] = 0x7f;           // DEL
    pty->term.c_cc[VWERASE] = CTRLKEY('W');
    pty->term.c_cc[VKILL] = CTRLKEY('U');
    pty->term.c_cc[VREPRINT] = CTRLKEY('R');
    pty->term.c_cc[VINTR] = CTRLKEY('C');
    pty->term.c_cc[VQUIT] = 0x1c;           // Control+backslash
    pty->term.c_cc[VSUSP] = CTRLKEY('Z');
#ifndef LINUX
    pty->term.c_cc[VDSUSP] = CTRLKEY('Y');
#endif
    pty->term.c_cc[VSTART] = CTRLKEY('Q');
    pty->term.c_cc[VSTOP] = CTRLKEY('S');
    pty->term.c_cc[VLNEXT] = CTRLKEY('V');
    pty->term.c_cc[VDISCARD] = CTRLKEY('O');
    pty->term.c_cc[VMIN] = 1;
    pty->term.c_cc[VTIME] = 0;
#ifndef LINUX
    pty->term.c_cc[VSTATUS] = CTRLKEY('T');
#endif

    pty->term.c_ispeed = B38400;
    pty->term.c_ospeed = B38400;

    pty->win.ws_row = 25;
    pty->win.ws_col = 80;
    pty->win.ws_xpixel = 0;
    pty->win.ws_ypixel = 0;

    /* Save real stderr so we can fprintf to it instead of pty for errors. */
    int STDERR = dup(STDERR_FILENO);
    FILE *parent_stderr = fdopen(STDERR, "w");
    setvbuf(parent_stderr, NULL, _IONBF, 0);

    int pid = forkpty(&pty->master_fd, pty->name, &pty->term, &pty->win);
    if (pid == (pid_t)0) {
        /* child process */
        char* const argv[] = {"login", "-pfl", "ajt", NULL};
        execvp(*argv, argv);

        /* exec error */
        fprintf(parent_stderr, "## exec failed ##\n");
        fprintf(parent_stderr, "filename=%s error=%s\n", *argv, strerror(errno));

        sleep(10);
        exit(-1);
    } else if (pid < (pid_t)0) {
        fprintf(stderr, "forkpty() failed: %s\n", strerror(errno));
    } else if (pid > (pid_t)0) {
        /* parent process */
        pty->child_pid = pid;
        fprintf(stdout, "pty name = %s\n", pty->name);
        fprintf(stdout, "child pid = %d\n", pid);
        *pty_out = pty;
        return 1;
    }
    return 0;
}

int Pty_Destroy(struct Pty *pty)
{
    // TODO:
    return 1;
}

int Pty_Send(struct Pty *pty, const char* buffer, size_t size)
{
    write(pty->master_fd, buffer, size);
    return 1;
}

int Pty_Read(struct Pty *pty, char *buffer, size_t size)
{
    struct pollfd pfd = {pty->master_fd, POLLRDBAND, 0};

    poll(&pfd, 1, 0);
    if (pfd.revents & POLLRDBAND) {
        int bytes_read = read(pty->master_fd, buffer, size - 1);
        // null terminate string.
        if (bytes_read >= 0)
            buffer[bytes_read] = 0;
        return bytes_read;
    } else {
        return 0;
    }
}

