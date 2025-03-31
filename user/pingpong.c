#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv)
{
    if (argc > 1)
    {
        fprintf(2, "Usage: pingpong\n");
        exit(1);
    }
    int pipeP2C[2] = {0};
    int pipeC2P[2] = {0};
    if(pipe(pipeP2C) < 0 || pipe(pipeC2P) < 0)
    {
        fprintf(2, "pipe failed\n");
        exit(1);
    }
    int pid = fork();
    if (pid > 0)
    {
        // Parent process
        close(pipeP2C[0]);
        close(pipeC2P[1]);
        char byte[1] = {'P'};
        write(pipeP2C[1],byte,1);
        read(pipeC2P[0],byte,1);
        fprintf(1,"%d: received pong\n",getpid());
    }
    else if (pid == 0)
    {
        close(pipeP2C[1]);
        close(pipeC2P[0]);
        char byte[1];
        read(pipeP2C[0],byte,1);
        fprintf(1,"%d: received ping\n",getpid());
        write(pipeC2P[1],byte,1);
    }
    else
    {
        fprintf(2, "fork failed\n");
        exit(1);
    }
    exit(0);
}