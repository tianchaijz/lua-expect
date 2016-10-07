#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int io_setnonblocking(int *fd) {
    int flags;

    flags = fcntl(*fd, F_GETFL, 0);
    if (flags == 1) {
        return -1;
    }

    return fcntl(*fd, F_SETFL, flags | O_NONBLOCK);
}

int main(int argc, char *argv[]) {
    size_t nbytes;
    ssize_t bytes_read;
    char buf[20];
    int fd;

    fd = fileno(stdin);
    // io_setnonblocking(&fd);

    nbytes = sizeof(buf);
    bytes_read = read(fd, buf, nbytes);

    printf("bytes read: %ld\n", bytes_read);

    return 0;
}

/* vi:set ft=c ts=4 sw=4 et fdm=marker: */
