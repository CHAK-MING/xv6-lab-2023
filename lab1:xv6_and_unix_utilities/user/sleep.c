/*
 * @Author       : CHAKMING 1766069334@qq.com
 * @Date         : 2024-04-27 09:17:27
 * @LastEditors  : CHAKMING
 * @LastEditTime : 2024-04-27 11:58:54
 * @FilePath     : /lab1:xv6_and_unix_utilities/user/sleep.c
 * @Description  : 
 *
 * Copyright 2024 CHAKMING, All Rights Reserved.
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) 
{
    if(argc != 2) {
        fprintf(2, "usage: sleep [ticks]\n");
        exit(1);
    }
    sleep(atoi(argv[1]));
    exit(0);
}