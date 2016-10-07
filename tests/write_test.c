#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

    char buf[20];
    size_t nbytes;
    ssize_t bytes_written;
    int fd;

    fd = fileno(stdout);
    strcpy(buf, "This is a test\n");
    nbytes = strlen(buf);

    bytes_written = write(fd, buf, nbytes);

    printf("bytes written: %ld\n", bytes_written);

    return 0;
}

/* vi:set ft=c ts=4 sw=4 et fdm=marker: */
