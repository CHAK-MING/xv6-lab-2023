/*
 * @Author       : CHAKMING 1766069334@qq.com
 * @Date         : 2024-04-27 12:00:36
 * @LastEditors  : CHAKMING
 * @LastEditTime : 2024-04-27 12:03:34
 * @FilePath     : /lab1:xv6_and_unix_utilities/user/xargs.c
 * @Description  :
 *
 * Copyright 2024 CHAKMING, All Rights Reserved.
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/param.h"

int main(int argc, char *argv[MAXARG])
{
    char *new_args[MAXARG];
    int new_argc = 0;
    char now;
    uint idx = 0;
    memset(new_args, 0, sizeof(new_args));
    while (read(0, &now, 1) == 1) // Read 1 char
    {
        if (now == ' ')
        {
            idx = 0;
        }
        else if (now == '\n')
        {
            idx = 0;
            // Execute
            char *final_args[MAXARG];
            memset(final_args, 0, sizeof(final_args));
            int final_argc = argc + new_argc;
            if (final_argc > MAXARG)
            {
                printf("Too many args! (Max %d)\n", MAXARG);
                continue;
            }
            for (int i = 0; i < argc; i++)
                final_args[i] = argv[i];
            for (int i = 0; i < new_argc; i++)
                final_args[argc + i] = new_args[i];
            int pid = fork();
            if (pid == 0) // Child
            {
                exec(final_args[1], final_args + 1);
            }
            wait(0);
            // Clear
            idx = 0;
            new_argc = 0;
            memset(new_args, 0, sizeof(new_args));
        }
        else
        {
            if (idx == 0)
            {
                new_args[new_argc] = malloc(256);
                new_argc++;
            }
            new_args[new_argc - 1][idx] = now;
            idx++;
        }
    }
    exit(0);
}