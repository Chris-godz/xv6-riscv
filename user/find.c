#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

void find(char *path, char *filename)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;
    if ((fd = open(path, O_RDONLY)) < 0)
    {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }
    if (fstat(fd, &st) < 0)
    {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }
    switch (st.type)
    {
    case T_DEVICE:
    case T_FILE:
        break;
    case T_DIR:
        strcpy(buf, path);
        p = buf + strlen(buf);
        *p++ = '/';
        while (read(fd, &de, sizeof(de)) == sizeof(de))
        {
            if (de.inum == 0) // 跳过空目录项
                continue;
            if (strcmp(filename, de.name) == 0)
            {
                printf("%s/%s\n", path, de.name);
            }
            if (strcmp(de.name, ".") != 0 && strcmp(de.name, "..") != 0)
            {
                int len = strlen(de.name);
                memmove(p, de.name, len);
                p[strlen(de.name)] = 0;
                find(buf, filename);
            }
        }
        break;
    }
    close(fd);
}
int main(int argc, char *argv[])
{
    int i;
    if (argc < 3)
    {
        fprintf(2, "Usage: find <path> <filename>\n");
        exit(1);
    }
    for (i = 2; i < argc; i++)
    {
        find(argv[1], argv[i]);
    }
    exit(0);
}