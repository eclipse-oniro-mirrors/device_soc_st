/*
 * Copyright (c) 2021-2022 Talkweb Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "errno.h"
#include "fcntl.h"
#include "sys/stat.h"
#include "los_fs.h"

#define LITTLEFS_MAX_LFN_LEN 120

int HalFileOpen(const char* path, int oflag, int mode)
{
    char tmpPath[LITTLEFS_MAX_LFN_LEN] = {0};
    (void)snprintf_s(tmpPath, LITTLEFS_MAX_LFN_LEN, LITTLEFS_MAX_LFN_LEN, "/talkweb/%s", path);
	
	return LOS_Open(tmpPath, oflag);   
}

int HalFileClose(int fd)
{
    return LOS_Close(fd);
}

int HalFileRead(int fd, char *buf, unsigned int len)
{
    return LOS_Read(fd, buf, len);
}

int HalFileWrite(int fd, const char *buf, unsigned int len)
{
    return LOS_Write(fd, buf, len);
}

int HalFileDelete(const char *path)
{
    char tmpPath[LITTLEFS_MAX_LFN_LEN] = {0};
    (void)snprintf_s(tmpPath, LITTLEFS_MAX_LFN_LEN, LITTLEFS_MAX_LFN_LEN, "/littlefs/%s", path);
    return LOS_Unlink(tmpPath);
}

int HalFileStat(const char *path, unsigned int *fileSize)
{
    char tmpPath[LITTLEFS_MAX_LFN_LEN] = {0};
    struct stat halStat = {0};
    int ret = 0;
    (void)snprintf_s(tmpPath, LITTLEFS_MAX_LFN_LEN, LITTLEFS_MAX_LFN_LEN, "/littlefs/%s", path);
    ret = LOS_Stat(tmpPath, &halStat);
    *fileSize = halStat.st_size;
    return ret;
}

int HalFileSeek(int fd, int offset, unsigned int whence)
{
    return LOS_Lseek(fd, (off_t)offset, whence);
}
