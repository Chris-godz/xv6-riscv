#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void printPrimes(int fd)
{
    int prime, num;
    if (read(fd, &prime, sizeof(prime)) <= 0)
    {
        close(fd);
        return;
    }
    int pipefd[2];
    if (pipe(pipefd) < 0)
    {
        fprintf(2, "pipe failed\n");
        exit(1);
    }
    int pid = fork();
    if (pid > 0)
    {
        close(pipefd[0]);
        fprintf(1, "prime %d\n", prime);
        while (read(fd, &num, sizeof(num)) > 0)
        {
            if (num % prime != 0)
            {
                write(pipefd[1], &num, sizeof(num));
            }
        }
        close(fd);
        close(pipefd[1]);
        if (wait(0) < 0)
        {
            fprintf(2, "no child thread wait failed\n");
            exit(1);
        }
    }
    else if (pid == 0)
    {
        close(fd);
        close(pipefd[1]);
        printPrimes(pipefd[0]);
    }
    else
    {
        fprintf(2, "fork failed\n");
        exit(1);
    }
}
int main(int argc, char *argv[])
{
    if (argc > 2)
    {
        fprintf(2, "Usage: primes [nums]\n");
        exit(1);
    }
    int n = 280;
    if (argc == 2)
    {
        n = atoi(argv[1]);
    }
    int pipefd[2];
    if (pipe(pipefd) < 0)
    {
        fprintf(2, "pipe failed\n");
        exit(1);
    }
    int pid = fork();
    if (pid > 0)
    {
        close(pipefd[0]);
        for (int i = 2; i <= n; i++)
        {
            write(pipefd[1], &i, sizeof(i));
        }
        close(pipefd[1]);
        if (wait(0) < 0)
        {
            fprintf(2, "no child thread wait failed\n");
            exit(1);
        }
    }
    else if (pid == 0)
    {
        close(pipefd[1]);
        printPrimes(pipefd[0]);
    }
    else
    {
        fprintf(2, "fork failed\n");
        exit(1);
    }
    exit(0);
}