/*----------------------------------------------------------------------------
 * Copyright (c) <2016-2018>, <Huawei Technologies Co., Ltd>
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific prior written
 * permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *---------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------
 * Notice of Export Control Law
 * ===============================================
 * Huawei LiteOS may be subject to applicable export control laws and regulations, which might
 * include those applicable to Huawei LiteOS of U.S. and the country in which you are located.
 * Import, export and usage of Huawei LiteOS in any manner by you shall be in compliance with such
 * applicable export control laws and regulations.
 *---------------------------------------------------------------------------*/

#include "atiny_adapter.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>
#include <pthread.h>



#define ATINY_CNT_MAX_WAITTIME 0xFFFFFFFF
#define LOG_BUF_SIZE (1024)

#if 0
static uint64_t osKernelGetTickCount (void)
{
    uint64_t ticks;
    UINTPTR uvIntSave;

    if(OS_INT_ACTIVE)
    {
        ticks = 0U;
    }
    else
    {
        uvIntSave = LOS_IntLock();
        ticks = g_ullTickCount;
        LOS_IntRestore(uvIntSave);
    }

    return ticks;
}
#endif

atiny_time_t atiny_gettime_ms(void)
{

    struct timeval tv;
    gettimeofday(&tv,NULL);

    return tv.tv_sec*1000 + tv.tv_usec/1000;
}

void *atiny_malloc(size_t size)
{
    return malloc(size);
}

void atiny_free(void *ptr)
{
    (void)free(ptr);
}

int atiny_snprintf(char *buf, size_t size, const char *format, ...)
{
    int     ret;
    va_list args;

    va_start(args, format);
    ret = vsnprintf(buf, size, format, args);
    va_end(args);

    return ret;
}


int atiny_printf(const char *format, ...)
{
    int ret;
    char str_buf[LOG_BUF_SIZE] = {0};
    va_list list;

    memset(str_buf, 0, LOG_BUF_SIZE);
    va_start(list, format);
    ret = vsnprintf(str_buf, LOG_BUF_SIZE, format, list);
    va_end(list);

    printf("%s", str_buf);

    return ret;
}


char *atiny_strdup(const char *ch)
{
    return strdup(ch);
}

char *atiny_strndup(const char *ch, size_t n)
{
    return strndup(ch,n);
}


#ifdef WITH_LINUX

void *atiny_mutex_create(void)
{
    pthread_mutex_t *mutex = NULL;
    mutex = malloc(sizeof(pthread_mutex_t));
    if(mutex == NULL)
        return NULL;

    if(pthread_mutex_init(mutex, NULL) != 0)
        return NULL;

    return (void *)mutex;
}

void atiny_mutex_destroy(void *mutex)
{
    if(mutex == NULL)
        return;
    (void)pthread_mutex_destroy(mutex);
    free(mutex);
    return;
}

void atiny_mutex_lock(void *mutex)
{
    if (mutex == NULL)
    {
        return;
    }

    (void)pthread_mutex_lock(mutex);
}

void atiny_mutex_unlock(void *mutex)
{
    if (mutex == NULL)
    {
        return;
    }

    (void)pthread_mutex_unlock(mutex);
}

#else

void *atiny_mutex_create(void)
{
    return NULL;
}

void atiny_mutex_destroy(void *mutex)
{
    ((void)mutex);
}

void atiny_mutex_lock(void *mutex)
{
    ((void)mutex);
}

void atiny_mutex_unlock(void *mutex)
{
    ((void)mutex);
}

#endif /* LOSCFG_BASE_IPC_SEM == YES */


int atiny_random(void* output, size_t len)
{
    size_t i;
    unsigned char * p = (unsigned char *) output;

    for (i = 0; i < len; i++)
        p [i] = (unsigned char) (rand () & 255);

    return 0;
}

void atiny_reboot(void)
{

}
