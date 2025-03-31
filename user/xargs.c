#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"
#define MAXLEN 128

int main(int argc, char *argv[])
{
    sleep(1);
    char buf[MAXLEN];
    char *args[MAXARG];
    int n;
    
    if (argc <= 1) {
        fprintf(2, "Usage: xargs command [args...]\n");
        exit(1);
    }
    for (int i = 1; i < argc; i++) {
        args[i-1] = argv[i];
    }
    char c;
    int pos = 0;
    while ((n = read(0, &c, 1)) > 0) {
        if (c == '\n' || c == ' ') {
            buf[pos] = '\0';
            if (pos > 0) { // 跳过空行
                // 将该行作为参数添加并终止参数数组
                args[argc-1] = buf;
                args[argc] = 0;
                
                // 执行命令
                int pid = fork();
                if (pid == 0) {
                    exec(args[0], args);
                    fprintf(2, "exec %s failed\n", args[0]);
                    exit(1);
                } else if (pid > 0) {
                    wait(0);
                } else {
                    fprintf(2, "fork failed\n");
                    exit(1);
                }
            }
            pos = 0;
        } else {
            if (pos < MAXLEN - 1) {
                buf[pos++] = c;
            } else {
                fprintf(2, "xargs: line too long\n");
                exit(1);
            }
        }
    }
    // 处理最后一行（如果没有换行符）
    if (pos > 0) {
        buf[pos] = '\0';
        args[argc-1] = buf;
        args[argc] = 0;
        
        int pid = fork();
        if (pid == 0) {
            exec(args[0], args);
            fprintf(2, "exec %s failed\n", args[0]);
            exit(1);
        } else if (pid > 0) {
            wait(0);
        } else {
            fprintf(2, "fork failed\n");
            exit(1);
        }
    }
    exit(0);
}