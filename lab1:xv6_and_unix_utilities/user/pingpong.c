/*
 * @Author       : CHAKMING 1766069334@qq.com
 * @Date         : 2024-04-27 09:23:35
 * @LastEditors  : CHAKMING
 * @LastEditTime : 2024-04-27 09:36:35
 * @FilePath     : /lab1:xv6_and_unix_utilities/user/pingpong.c
 * @Description  : 
 *
 * Copyright 2024 CHAKMING, All Rights Reserved.
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) 
{
    if(argc != 1) {
        fprintf(2, "pingpong: not need any parameter\n");
        exit(1);
    }

    // pipe1: parent->child 
    // pipe2: child->parent
    int pipe1[2], pipe2[2];
    
    if(!(pipe(pipe1) == 0 && pipe(pipe2) == 0)) {
        fprintf(2, "pipe() failed\n");
        exit(1);
    }

    int pid = fork();
    if(pid < 0) {
        fprintf(2, "fork() failed\n");
        exit(1);
    } else if(pid == 0) {
        // child
        int child_pid =  getpid();
        char buf[1024];
        read(pipe1[0], buf, sizeof buf);
        if(buf[0] != 0) {
            printf("%d: received ping\n", child_pid);
            write(pipe2[1], "a", 1);
        }
    } else {
        // parent
        int parent_pid = getpid();
        write(pipe1[1], "a", 1);
        char buf[1024];
        read(pipe2[0], buf, sizeof buf);
        if(buf[0] != 0) {
            printf("%d: received pong\n", parent_pid);
        }
    }
    exit(0);
}