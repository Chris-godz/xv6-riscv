#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        const char *msg = "Usage: sleep <seconds>\n";
        write(2, msg, strlen(msg));
        exit(1);
    }
    uint32 seconds = atoi(argv[1]);
    sleep(seconds);
    exit(0);
}