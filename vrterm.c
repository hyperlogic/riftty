#include <stdio.h>
#include <util.h>
#include <string.h>
#include <stdlib.h>
#include <sys/syslimits.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>

struct Pty
{
    int master_fd;
    pid_t child_pid;
    char name[PATH_MAX];
    struct termios term;
    struct winsize win;
};

int Pty_Make(struct Pty **pty_out)
{
    struct Pty *pty = calloc(1, sizeof(struct Pty));
    if (!pty) {
        return 0;
    }

    // NOTE: copied from iTerm2 PTYTask.m
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
    pty->term.c_cc[VDSUSP] = CTRLKEY('Y');
    pty->term.c_cc[VSTART] = CTRLKEY('Q');
    pty->term.c_cc[VSTOP] = CTRLKEY('S');
    pty->term.c_cc[VLNEXT] = CTRLKEY('V');
    pty->term.c_cc[VDISCARD] = CTRLKEY('O');
    pty->term.c_cc[VMIN] = 1;
    pty->term.c_cc[VTIME] = 0;
    pty->term.c_cc[VSTATUS] = CTRLKEY('T');

    pty->term.c_ispeed = B38400;
    pty->term.c_ospeed = B38400;

    pty->win.ws_row = 25;
    pty->win.ws_col = 80;
    pty->win.ws_xpixel = 0;
    pty->win.ws_ypixel = 0;

    // Save real stderr so fprintf writes to it instead of pty
    int STDERR = dup(STDERR_FILENO);
    FILE *parent_stderr = fdopen(STDERR, "w");
    setvbuf(parent_stderr, NULL, _IONBF, 0);

    int pid = forkpty(&pty->master_fd, pty->name, &pty->term, &pty->win);
    if (pid == (pid_t)0) {
        char* const argv[] = {"login", "-pfl", "ajt", NULL};
        int sts = execvp(*argv, argv);

        /* exec error */
        fprintf(parent_stderr, "## exec failed ##\n");
        fprintf(parent_stderr, "filename=%s error=%s\n", *argv, strerror(errno));

        sleep(10);
        exit(-1);
    } else if (pid < (pid_t)0) {
        fprintf(stderr, "error! no forky: %s\n", strerror(errno));
    } else if (pid > (pid_t)0) {
        // parent
        pty->child_pid = pid;
        fprintf(stdout, "pty name = %s\n", pty->name);
        fprintf(stdout, "child pid = %d\n", pid);
        *pty_out = pty;
        return 1;
    }
    return 0;
}

int Pty_Send(struct Pty *pty, const char* buffer)
{
    write(pty->master_fd, buffer, strlen(buffer));
}

int Pty_Read(struct Pty *pty)
{
    struct pollfd pfd = {pty->master_fd, POLLRDBAND, 0};

    poll(&pfd, 1, 0);
    if (pfd.revents & POLLRDBAND) {
        char c;
        size_t bytes_read = read(pty->master_fd, &c, 1);
        printf("%c", c);
        return bytes_read;
    } else {
        return 0;
    }
}

int main(int argc, char *argv[])
{
    struct Pty *pty;

    if (!Pty_Make(&pty)) {
        fprintf(stderr, "Pty_Make failed");
    }

    sleep(1);

    while(Pty_Read(pty)) ;

    Pty_Send(pty, "pwd\n\r");

    while(1) {
        Pty_Read(pty);
    }

    return 0;
}
