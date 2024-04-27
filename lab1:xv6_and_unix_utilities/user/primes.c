/*
 * @Author       : CHAKMING 1766069334@qq.com
 * @Date         : 2024-04-27 11:35:19
 * @LastEditors  : CHAKMING
 * @LastEditTime : 2024-04-27 11:49:27
 * @FilePath     : /lab1:xv6_and_unix_utilities/user/primes.c
 * @Description  : 算法就是不断从管道中拿取当前要判断是否是素数的数，然后第一次从old_pipe_output_fd的文件描述符拿到的数据即为素数，
 *                 就是已经经过它前面2~n-1求余判断，均不为0，n为当前的数，故此数肯定为素数。
 *
 * Copyright 2024 CHAKMING, All Rights Reserved.
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void proceed(int old_pipe_output_fd) 
{
    int newly_piped_fds[2];
    pipe(newly_piped_fds);
    int first, now;
    if(read(old_pipe_output_fd, &first, sizeof(int)) <= 0) {
        close(old_pipe_output_fd);
        exit(0);
    }
    printf("prime %d\n", first);

    int pid = fork();

    if(pid != 0) {
        // parent
        close(newly_piped_fds[0]);
        while(read(old_pipe_output_fd, &now, sizeof(int)) > 0) 
            if(now % first != 0) write(newly_piped_fds[1], &now, sizeof(int));
        close(old_pipe_output_fd);
        close(newly_piped_fds[1]);
        wait(0); // 等待子进程
        exit(0);
    } else {
        // child
        close(old_pipe_output_fd);
        close(newly_piped_fds[1]);
        proceed(newly_piped_fds[0]);
    }
}

int main(int argc, char *argv[]) 
{
    int newly_piped_fds[2];
    pipe(newly_piped_fds);
    for(int i = 2; i <= 35; ++i) {
        write(newly_piped_fds[1], &i, sizeof(int));
    }
    close(newly_piped_fds[1]);
    proceed(newly_piped_fds[0]);
    exit(0);
}