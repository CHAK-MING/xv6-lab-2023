/*
 * @Author       : CHAKMING 1766069334@qq.com
 * @Date         : 2024-04-27 11:56:35
 * @LastEditors  : CHAKMING
 * @LastEditTime : 2024-04-27 11:59:04
 * @FilePath     : /lab1:xv6_and_unix_utilities/user/find.c
 * @Description  :
 *
 * Copyright 2024 CHAKMING, All Rights Reserved.
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

// 获取路径上的文件名
char *
fmtname(char *path)
{
    static char buf[DIRSIZ + 1];
    char *p;

    // Find first character after last slash.
    for (p = path + strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;

    // Return blank-padded name.
    if (strlen(p) >= DIRSIZ)
        return p;
    memmove(buf, p, strlen(p));
    memset(buf + strlen(p), '\0', DIRSIZ - strlen(p));
    return buf;
}

// 查找path中，满足pattern的文件，并输出。
void search(char *path, char *pattern)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        printf("Cannot open %s\n", path);
        close(fd);
    }
    struct stat status;
    fstat(fd, &status);
    if (status.type == T_FILE || status.type == T_DEVICE)
    {
        char *short_name = fmtname(path);
        if (!strcmp(short_name, pattern))
            printf("%s\n", path);
    }
    else
    {
        char buf[256], *p;
        struct dirent entry;
        while (read(fd, &entry, sizeof(entry)) == sizeof(entry))
        {
            if (entry.inum == 0)
                continue;
            char *filename = entry.name;
            if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0)
                continue;
            if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf)
            {
                printf("find: path too long\n");
                break;
            }
            strcpy(buf, path);
            p = buf + strlen(buf);
            *p++ = '/';
            strcpy(p, filename);
            search(buf, pattern);
        }
    }
    close(fd);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("usage: find [dirname] [pattern]\n");
        exit(0);
    }

    char *path = argv[1], *pattern = argv[2];
    search(path, pattern);
    exit(0);
}