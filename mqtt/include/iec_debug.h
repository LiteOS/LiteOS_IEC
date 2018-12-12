#ifndef __IEC_DEBUG_H__
#define __IEC_DEBUG_H__

#include <assert.h>

#define IEC_ASSERT_MSG_PARAM_NULL  "null param"
#define IEC_ASSERT_MSG_MALLOC_ERR  "malloc error"

#ifdef DEBUG
#define IEC_ASSERT(exp, msg)                                         \
    if(!exp)                                                           \
    {                                                                  \
        do                                                             \
        {                                                              \
            printf("[%s]:[%d]----[%s]\n", __FILE__, __LINE__, msg);    \
        } while(0);                                                    \
    assert(exp);                                                       \
    }
#else
#define IEC_ASSERT(exp, msg)
#endif

#endif
